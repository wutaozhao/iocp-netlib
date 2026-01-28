
#include "TcpClient.h"
#include "TcpService.h"
#include "NetCoreIOCP.h"

WT_BEGIN

TcpClient::TcpClient()
{
	Reset();
}

TcpClient::~TcpClient()
{
	//printf("tcp client destruct\n");
}

long TcpClient::IncreaseRef()
{
	return ::InterlockedIncrement(&mReferenceCount);
}

long TcpClient::DecreaseRef()
{
	return ::InterlockedDecrement(&mReferenceCount);
}

bool TcpClient::IsActive()
{
	return ::InterlockedCompareExchange(&mReferenceCount, 0, 0) > 0;
}

void TcpClient::MarkDestroy()
{
	mMarkedDestroy = true;
}

bool TcpClient::IsMarkedDestroy()
{
	return mMarkedDestroy;
}

void TcpClient::Reset()
{
	mLogicSocketID = 0;
	mRemoteIP = 0;
	mRemotePort = 0;
	mPendingIO = 0;
	mConnState = CONN_CLOSED;
	mReferenceCount = 0;
	
#if (_WIN32_WINNT >= 0x0600)
	mLastTickTime = GetTickCount64();
#else
	mLastTickTime = GetTickCount();
#endif
	memset(&mRecvContext, 0, sizeof(mRecvContext));
	mRecvPacketBuffer = NULL;
	mRecvBytes = 0;
	mLastErrorCode = 0;
	mConnectExPtr = NULL;
	mMarkedDestroy = false;

	mSocket.Close();
}

void TcpClient::ShutDown()
{
	mSocket.Shutdown();
}

void TcpClient::CloseSocket()
{
	mConnState = CONN_CLOSING;
	mSocket.Close();
}


void TcpClient::CancelIO()
{
	//
	if (!mSocket.IsInvalid())
	{
		mSocket.Shutdown();
		mSocket.Close();
	}
}


bool TcpClient::PostRecv()
{
	DWORD dwFlags = 0;
	DWORD dwRecvBytes = 0;

	memset(mRecvBuffer, 0, sizeof(mRecvBuffer));

	IncreaseIOCount();

	SetRecvContextToRecv();

	int nResult = WSARecv(mSocket.GetHandle(), &mRecvContext.mBuffer, 1, &dwRecvBytes, &dwFlags, &mRecvContext.mOverlapped, NULL);
	if ((SOCKET_ERROR == nResult) && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		DecreaseIOCount();
		mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostRecv() get error code:%d -- %d", GetLastError(), WSAGetLastError());
		return false;
	}
	return true;
}

void TcpClient::SetTimeout()
{
	mTcpService->Log(LOG_LEVEL_DEBUG, "client:%s - %d time out", ToStringIP(mRemoteIP).c_str(), mRemotePort);

	// main purpose is to release the ref count in iocp woker,
	// so we can free all tcp client in iocp worker
	PostSpecial(IOCP_OP_TIMEOUT);
}

int TcpClient::PostSpecial(int opType)
{
	bool hasError = false;
	IOContext* ct = NULL;

	do
	{
		//
		IncreaseIOCount();

		//
		ct = mTcpService->AllocExceptionIOContext();
		if (!ct) {
			//
			DecreaseIOCount();
			hasError = true;
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostSpecial() alloc exception context failed, type:%d", opType);
			break;
		}

		ct->Reset();
		ct->mOperationType = opType;
		ct->mTcpClient = this;
		ct->mTcpService = mTcpService;
#if (_WIN32_WINNT >= 0x0600)
		ct->mTime = GetTickCount64();
#else
		ct->mTime = GetTickCount();
#endif

		if (!PostQueuedCompletionStatus(mTcpService->GetCore()->GetIOCPHandle(),
			0,
			(ULONG_PTR)this,
			(LPOVERLAPPED)&ct->mOverlapped)) 
		{
			// 
			mTcpService->ReleaseExceptionIOContext(ct);
			ct = NULL;
			DecreaseIOCount();
			hasError = true;
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostSpecial() post failed, type:%d, err:%d", opType, GetLastError());
			break;
		}
	} while (false);

	if (hasError) {
		//
		CheckRelease();
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

bool TcpClient::CreateSocket()
{
	return mSocket.Create(SOCK_STREAM, WSA_FLAG_OVERLAPPED);
}

void TcpClient::SetRecvContextToAccept()
{
	mRecvContext.mOperationType = IOCP_OP_ACCEPT;
#if (_WIN32_WINNT >= 0x0600)
	mRecvContext.mTime = GetTickCount64();
#else
	mRecvContext.mTime = GetTickCount();
#endif
	mRecvContext.mTcpClient = this;
	mRecvContext.mTcpService = mTcpService;
	mRecvContext.mBuffer.buf = mRecvBuffer;
	mRecvContext.mBuffer.len = TCP_CLIENT_RECV_BUFF_SIZE;
}

void TcpClient::SetRecvContextToRecv()
{
	mRecvContext.mOperationType = IOCP_OP_RECV;
	mRecvContext.mBuffer.buf = mRecvBuffer;
	mRecvContext.mBuffer.len = TCP_CLIENT_RECV_BUFF_SIZE;
	mRecvContext.mTcpClient = this;
	mRecvContext.mTcpService = mTcpService;
#if (_WIN32_WINNT >= 0x0600)
	mRecvContext.mTime = GetTickCount64();
#else
	mRecvContext.mTime = GetTickCount();
#endif
}

void TcpClient::SetRecvContextToConnect()
{
	mRecvContext.mOperationType = IOCP_OP_CONNECTEX;
	mRecvContext.mBuffer.buf = mRecvBuffer;
	mRecvContext.mBuffer.len = TCP_CLIENT_RECV_BUFF_SIZE;
	mRecvContext.mTcpClient = this;
	mRecvContext.mTcpService = mTcpService;
#if (_WIN32_WINNT >= 0x0600)
	mRecvContext.mTime = GetTickCount64();
#else
	mRecvContext.mTime = GetTickCount();
#endif
}

bool TcpClient::GetPeerAddrInfo()
{
	return mSocket.GetPeerAddr(mRemoteIP, mRemotePort);
}

long TcpClient::IncreaseIOCount()
{
	return ::InterlockedIncrement(&mPendingIO);
}

long TcpClient::DecreaseIOCount()
{
	return ::InterlockedDecrement(&mPendingIO);
}

int TcpClient::PostSend(const void* data, int dataLen)
{
	int ret = 0;
	IOContext* context = NULL;
	void* sendPacket = NULL;

	do
	{
		/*
		if (::InterlockedCompareExchange(
			&mConnState,
			CONN_ACTIVE,
			CONN_ACTIVE) != CONN_ACTIVE) {
			ret = NSE_CONNECTION_CLOSED;
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostSend find socket invalid");
			break;
		}*/

		//
		context = mTcpService->AllocSendIOContext();
		if (!context) {
			ret = NSE_SEND_QUEUE_FULL;
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostSend find alloc send io context failed");
			break;
		}
		context->Reset();

		sendPacket = mTcpService->AllocSendPacket();
		if (!sendPacket) {
			ret = NSE_SEND_QUEUE_FULL;
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostSend find alloc send packet failed");
			break;
		}

		memcpy(sendPacket, data, dataLen);

		context->mPointer = sendPacket;
		context->mOperationType = IOCP_OP_SEND;
		context->mBuffer.buf = (char*)sendPacket;
		context->mBuffer.len = dataLen;
		context->mTcpClient = this;
		context->mTcpService = mTcpService;

		IncreaseIOCount();
		DWORD dwFlags = 0;
		DWORD dwSendBytes = 0;
		int nResult = WSASend(mSocket.GetHandle(), &context->mBuffer, 1, &dwSendBytes, dwFlags, &context->mOverlapped, NULL);
		DWORD dwLastError = WSAGetLastError();
		if ((SOCKET_ERROR == nResult) && (ERROR_IO_PENDING != dwLastError))
		{
			CheckRelease();
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostSend invoke WSASend failed, lastError:%u -- %d", dwLastError, GetLastError());
			ret = NSE_SYSTEM_ERROR;
			break;
		}

	} while (false);

	if (ret != 0) {
		if (context) {
			mTcpService->ReleaseSendIOContext(context);
		}
		if (sendPacket) {
			mTcpService->ReleaseSendPacket(sendPacket);
		}
	}

	return ret;
}

void TcpClient::OnCompleteOperation(bool bSuccess, IOContext* pContext, unsigned int numberOfBytes, unsigned int errorCode)
{
	UpdateLastTickTime();

	try {
		if (!bSuccess) {
			unsigned errCode = NSE_EXCEPTION;
			mTcpService->Log(LOG_LEVEL_DEBUG, "TcpClient OnCompleteOperation get error:%d, op:%d", errorCode, pContext->mOperationType);
			if (pContext->mOperationType == IOCP_OP_SEND) {
				// 
				mTcpService->ReleaseSendPacket(pContext->mPointer);
				mTcpService->ReleaseSendIOContext(pContext);
			}
			else if (pContext->mOperationType == IOCP_OP_RECV) {
				errCode = NSE_REMOTE_DISCONNECT;
			}
			else if (pContext->mOperationType == IOCP_OP_CONNECTEX) {
				mTcpService->Log(LOG_LEVEL_ERROR, "connect ex get error:%d, ip:%s, port:%d", errorCode, ToStringIP(mRemoteIP).c_str(), mRemotePort);
				OnConnectExCompletion(NSE_CONNECT_FAILED);
			}

			SetDisconnect(errCode);
		}
		else {
			int ret = 0;
			if (pContext->mOperationType == IOCP_OP_RECV)
			{
				ret = OnRecvCompletion(pContext, numberOfBytes);
			}
			else if (pContext->mOperationType == IOCP_OP_SEND)
			{
				ret = OnSendCompletion(pContext, numberOfBytes);
			}
			else if (pContext->mOperationType == IOCP_OP_TIMEOUT) {
				mTcpService->ReleaseExceptionIOContext(pContext);
				ret = NSE_TIMEOUT;
			}
			else if (pContext->mOperationType == IOCP_OP_CLOSED) {
				mTcpService->ReleaseExceptionIOContext(pContext);
				ret = NSE_BE_CLOSED;
			}
			else if (pContext->mOperationType == IOCP_OP_CONNECTEX) {
				OnConnectExCompletion(0);
			}
			else {
				mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::OnCompleteOperation find invalid op type:%d", pContext->mOperationType);
			}
			if (ret != 0)
			{
				SetDisconnect(ret);
			}
		}
	}
	catch (...) {
		mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::OnCompleteOperation get exception");
	}

	CheckRelease();
}

void TcpClient::CheckRelease()
{
	//只有当连接关闭时，才会触发Decrease后值为0
	if (DecreaseIOCount() == 0) {
		LONG old = InterlockedCompareExchange(
			&mConnState,
			CONN_CLOSED,
			CONN_CLOSING
		);
		if (old == CONN_CLOSING) {
			if (mRecvPacketBuffer != NULL) {
				mTcpService->ReleaseRecvPacket(mRecvPacketBuffer);
				mRecvPacketBuffer = NULL;
			}
			mTcpService->mIOCallbackPtr->OnClosed(mLogicSocketID, mRemoteIP, mRemotePort, mLastErrorCode);
		}

		//printf("remove client in CheckRelease\n");
		mTcpService->RemoveClient(mLogicSocketID);
	}
}

int TcpClient::OnRecvCompletion(IOContext* pContext, unsigned int nNumberOfBytes)
{
	int ret = 0;

	do
	{
		if (nNumberOfBytes == 0) {
			ret = NSE_REMOTE_DISCONNECT;
			break;
		}

		ret = SplitPacket(pContext->mBuffer.buf, nNumberOfBytes);
		if (ret != 0){
			break;
		}

		if (!PostRecv()) {
			ret = NSE_RECV_FAILED;
			break;
		}
	} while (false);

	return ret;
}

int TcpClient::OnSendCompletion(IOContext* pContext, unsigned int nNumberOfBytes)
{
	if (nNumberOfBytes != pContext->mBuffer.len) {
		mTcpService->Log(LOG_LEVEL_ERROR, "OnSendCompletion find len not same:%u -- %u", nNumberOfBytes, pContext->mBuffer.len);
		mTcpService->ReleaseSendIOContext(pContext);
		return NSE_SEND_NOT_COMPLETE;
	}

	mTcpService->ReleaseSendPacket(pContext->mPointer);
	mTcpService->ReleaseSendIOContext(pContext);

	return 0;
}

void TcpClient::OnConnectExCompletion(unsigned int errorCode)
{
	if (errorCode == 0) {
		mConnState = CONN_CONNECTED;

		BOOL sopt = TRUE;
		mSocket.SetSockOpt(SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
		mSocket.SetSockOpt(IPPROTO_TCP, TCP_NODELAY, (char*)&sopt, sizeof(BOOL));
		mSocket.SetSockOpt(SOL_SOCKET, SO_DONTLINGER, (char*)&sopt, sizeof(BOOL));

		if (PostRecv()) {
			mTcpService->PostConnectSucc();
			mTcpService->mIOCallbackPtr->OnConnected(mLogicSocketID, mRemoteIP, mRemotePort);
		}
		else {
			SetDisconnect(NSE_EXCEPTION);
		}
	}
}

void TcpClient::SetDisconnect(int errCode)
{
	// close socket should be seperated from release tcp client
	LONG old = InterlockedCompareExchange(
		&mConnState,
		CONN_CLOSING,
		CONN_CONNECTED
	);

	if (old != CONN_CONNECTED) {
		return;
	}

	mLastErrorCode = errCode;

	if (!mSocket.IsInvalid())
	{
		mSocket.Shutdown();
		mSocket.Close();
	}
}

int TcpClient::SplitPacket(char* pData, int dataLen)
{
	int    leftSize = dataLen;
	char*  pTemp = pData;

	while (leftSize > 0) {
		if (mRecvPacketBuffer == NULL) {
			mRecvPacketBuffer = (char*)mTcpService->AllocRecvPacket();
			if (!mRecvPacketBuffer) {
				mTcpService->Log(LOG_LEVEL_ERROR, "split packet recv queue full");
				return NSE_RECV_QUEUE_FULL;
			}

			//
			mRecvBytes = 0;
		}

		int copySize = min(leftSize, mTcpService->mMaxPacketSize - mRecvBytes);
		leftSize -= copySize;

		// copy first, then add
		memcpy(mRecvPacketBuffer + mRecvBytes, pTemp, copySize);
		mRecvBytes += copySize;
		pTemp += copySize;

		if (mRecvBytes < (mTcpService->mPacketSizeOffset + (int)sizeof(int)))
		{
			return 0;
		}

		int packetSize = *(int*)(mRecvPacketBuffer + mTcpService->mPacketSizeOffset);
		if (packetSize < sizeof(int) || packetSize > mTcpService->mMaxPacketSize) {
			return NSE_ILLEGAL_RECV_PACKET;
		}
		if (mRecvBytes < packetSize) {
			return 0;
		}

		int ret = mTcpService->mIOCallbackPtr->OnReceived(mLogicSocketID, mRemoteIP, mRemotePort, mRecvPacketBuffer, packetSize);
		if (ret != 0) {
			mTcpService->Log(LOG_LEVEL_ERROR, "split packet on recv get ret:%d, closed", ret);
			SetDisconnect(NSE_BE_CLOSED);
			return NSE_BE_CLOSED;
		}
		mRecvBytes -= packetSize;

		memmove(mRecvPacketBuffer, mRecvPacketBuffer + packetSize, mRecvBytes);
	}

	return 0;
}

void TcpClient::UpdateLastTickTime()
{
#if (_WIN32_WINNT >= 0x0600)
	mLastTickTime = GetTickCount64();
#else
	mLastTickTime = GetTickCount();
#endif
}

bool TcpClient::IsAccepting()
{
	return mRecvContext.mOperationType == IOCP_OP_ACCEPT;
}

bool TcpClient::PostConnect(const char* ip, unsigned short port)
{
	bool bRet = false;

	do
	{
		#if (_WIN32_WINNT >= 0x0600)
		if (inet_pton(AF_INET, ip, &mRemoteIP) != 1)
		{
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostConnect invalid ip format:%s", ip);
			return false;
		}
        #else
		mRemoteIP = inet_addr(ip);
        #endif
		mRemotePort = port;

		int ret = mTcpService->GetCore()->AttachSocketToIOCP(mSocket.GetHandle(), (ULONG_PTR)this);
		if (ret != 0) {
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostConnect attach socket to iocp failed, ret:%d", ret);
			break;
		}

		GUID GuidConnectEx = WSAID_CONNECTEX;
		DWORD dwBytes = 0;

		int nResult = WSAIoctl(
			mSocket.GetHandle(),
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidConnectEx,
			sizeof(GuidConnectEx),
			&mConnectExPtr,
			sizeof(mConnectExPtr),
			&dwBytes,
			NULL,
			NULL
		);
		if (SOCKET_ERROR == nResult)
		{
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostConnect get connectex failed, err:%d -- %d", GetLastError(), WSAGetLastError());
			break;
		}

		if (!mSocket.Bind(0, "0.0.0.0"))
		{
			mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostConnect bind failed, err:%d -- %d", GetLastError(), WSAGetLastError());
			break;
		}

		SetRecvContextToConnect();

		IncreaseIOCount();

		SOCKADDR_IN   saddr;
		::ZeroMemory(&saddr, sizeof(SOCKADDR_IN));
		saddr.sin_addr.S_un.S_addr = mRemoteIP;
		saddr.sin_family = AF_INET;
		saddr.sin_port = ::htons(mRemotePort);

		DWORD dwSent = 0;
		if (!mConnectExPtr(
			mSocket.GetHandle(), 
			(SOCKADDR*)&saddr, 
			sizeof(SOCKADDR_IN), 
			NULL,
			0,
			&dwSent,
			&mRecvContext.mOverlapped))
		{
			DWORD dwError = ::WSAGetLastError();
			if (dwError != WSAEWOULDBLOCK && dwError != ERROR_IO_PENDING)
			{
				DecreaseIOCount();
				mTcpService->Log(LOG_LEVEL_ERROR, "TcpClient::PostConnect post connectex failed, err:%d -- %d", GetLastError(), dwError);
				break;
			}
		}

		bRet = true;

	} while (false);
	
	if (!bRet) {
		mSocket.Close();
	}

	return bRet;
}

WT_END