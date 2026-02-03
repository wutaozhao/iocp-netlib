
#include "TcpAcceptor.h"
#include "TcpService.h"
#include "net/NetService.h"
#include "TcpClient.h"
#include "NetCoreIOCP.h"

WT_BEGIN

TcpAcceptor::TcpAcceptor(void* tcpService)
{
	mTcpService = tcpService;
	mAcceptExPointer = NULL;
	mPendingAcceptCount = 0;
}

TcpAcceptor::~TcpAcceptor()
{
	UnInitialize();
}

int TcpAcceptor::Initialize(const std::string& listenIP, unsigned short listenPort, int backLog)
{
	TcpService* pService = reinterpret_cast<TcpService*>(mTcpService);

	int ret = 0;

	do
	{
		if (!pService) {
			ret = NSE_SYSTEM_ERROR;
			break;
		}
		if (!mSocket.Create(SOCK_STREAM, WSA_FLAG_OVERLAPPED)) {
			pService->Log(LOG_LEVEL_ERROR, "create listen socket failed, lastError:%d -- %d", GetLastError(), WSAGetLastError());
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		if (!mSocket.Bind(listenPort, listenIP.c_str())) {
			pService->Log(LOG_LEVEL_ERROR, "bind socket failed, lastError:%d -- %d", GetLastError(), WSAGetLastError());
			ret = NSE_BIND_ADDR_FAILED;
			break;
		}

		//
		if (!mSocket.Listen()) {
			pService->Log(LOG_LEVEL_ERROR, "listen socket failed, lastError:%d -- %d", GetLastError(), WSAGetLastError());
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		//
		ret = pService->GetCore()->AttachSocketToIOCP(mSocket.GetHandle(), (ULONG_PTR)this);
		if (ret != 0){
			pService->Log(LOG_LEVEL_ERROR, "Attack socket failed, lastError:%d -- %d", GetLastError(), WSAGetLastError());
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		mAcceptExPointer = NULL;
		static GUID g_GUIDAcceptEx = WSAID_ACCEPTEX;
		DWORD dwResult = 0;
		int nResult = ::WSAIoctl(mSocket.GetHandle(),
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&g_GUIDAcceptEx,
			sizeof(g_GUIDAcceptEx),
			&mAcceptExPointer,
			sizeof(mAcceptExPointer),
			&dwResult,
			NULL,
			NULL);
		if (SOCKET_ERROR == nResult)
		{
			pService->Log(LOG_LEVEL_ERROR, "get pointer to acceptex failed, lastError:%d -- %d", GetLastError(), WSAGetLastError());
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		for (int i = 0; i < backLog; i++) {
			if (!PostAccept()) {
				pService->Log(LOG_LEVEL_ERROR, "post accept failed, i:%d", i);
				ret = NSE_SYSTEM_ERROR;
				break;
			}
			else {
				pService->Log(LOG_LEVEL_ERROR, "post accept success", i);
			}
		}

	} while (false);

	if (ret != 0) {
		UnInitialize();
	}
	
	return ret;
}

void TcpAcceptor::UnInitialize()
{
	mSocket.Close();
}

long TcpAcceptor::IncrementPending()
{
	return InterlockedIncrement(&mPendingAcceptCount);
}

long TcpAcceptor::DecrementPending()
{
	return InterlockedDecrement(&mPendingAcceptCount);
}

void TcpAcceptor::PostCancel()
{
	TcpService* pService = reinterpret_cast<TcpService*>(mTcpService);
	pService->Log(LOG_LEVEL_ERROR, "will cancel io of acceptor");

	//
	if (!mSocket.IsInvalid())
	{
		mSocket.Shutdown();
		mSocket.Close();
	}
	else {
		pService->Log(LOG_LEVEL_ERROR, "acceptor socket invalid");
	}
}

bool TcpAcceptor::PostAccept()
{
	TcpService* pService = reinterpret_cast<TcpService*>(mTcpService);

	do
	{
		if (!pService) {
			break;
		}
		if (mSocket.IsInvalid()) {
			pService->Log(LOG_LEVEL_ERROR, "post accept error 1");
			break;
		}
		if (!pService) {
			pService->Log(LOG_LEVEL_ERROR, "post accept error 2");
			break;
		}

		//
		TcpClient* pClient = pService->AllocTcpClient();
		if (!pClient) {
			pService->Log(LOG_LEVEL_ERROR, "post accept error 3");
			break;
		}

		pClient->SetRecvContextToAccept();

		IncrementPending();

		DWORD dwBytes = 0;
		BOOL bRet = mAcceptExPointer(
			mSocket.GetHandle(),
			pClient->mSocket.GetHandle(),
			pClient->mRecvContext.mBuffer.buf,
			0,
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			&dwBytes,
			&pClient->mRecvContext.mOverlapped
		);
		if (!bRet) {
			DWORD dwError = WSAGetLastError();
			if (dwError != ERROR_IO_PENDING)
			{
				DecrementPending();
				pService->RemoveClient(pClient->mLogicSocketID);
				pService->Log(LOG_LEVEL_ERROR, "post accept accept ex failed, error:%u", dwError);
				break;
			}
		}

		return true;
	} while (false);

	return false;
}

void TcpAcceptor::OnCompleteOperation(bool bSuccess, IOContext* pPID, unsigned int numberOfBytes, unsigned int  errorCode)
{
	TcpService* pService = reinterpret_cast<TcpService*>(mTcpService);

	TcpClient* pClient = reinterpret_cast<TcpClient*>(pPID->mTcpClient);

	bool acceptSuccess = false;
	if (bSuccess) {
		pService->mStats["Accept"]++;
		if (AcceptConnection(pClient))
		{
			acceptSuccess = true;
			pService->mStats["AcceptSucc"]++;
		}
		else {
			pService->Log(LOG_LEVEL_ERROR, "OnCompleteOperation accept connection failed");
		}
	}

	if (!acceptSuccess) {
		pClient->CloseSocket();
		pClient->Reset();
		pService->RemoveClient(pClient->mLogicSocketID);
		pService->Log(LOG_LEVEL_ERROR, "acceptor complete get error:%u", errorCode);
	}

	PostAccept();

	if (DecrementPending() == 0) {
		pService->Log(LOG_LEVEL_ERROR, "acceptor find zero pending", errorCode);
		mSocket.Shutdown();
		mSocket.Close();
		pService->OnAcceptorReleased();
		delete this;
	}
}

bool TcpAcceptor::AcceptConnection(TcpClient* pClient)
{
	TcpService* pService = reinterpret_cast<TcpService*>(mTcpService);

	do
	{
		if (!pService) {
			break;
		}

		if (pClient->mRecvContext.mOperationType != IOCP_OP_ACCEPT) {
			pService->Log(LOG_LEVEL_ERROR, "AcceptConnection find invalid op type:%d", pClient->mRecvContext.mOperationType);
			break;
		}

		int nLen = 0;
		int nSize = sizeof(int);
		int nRet = getsockopt(pClient->mSocket.GetHandle(), SOL_SOCKET, SO_SNDBUF, (char*)&nLen, &nSize);

		// assign the same attribute
		SOCKET hListenSocket = mSocket.GetHandle();
		if (SOCKET_ERROR == setsockopt(pClient->mSocket.GetHandle(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&hListenSocket, sizeof(hListenSocket)))
		{
			pService->Log(LOG_LEVEL_ERROR, "AcceptConnection update socket failed, err:%d -- %d", GetLastError(), WSAGetLastError());
			break;
		}

		// set nodealy
		int nValue = 1;
		if (SOCKET_ERROR == setsockopt(pClient->mSocket.GetHandle(), IPPROTO_TCP, TCP_NODELAY, (char*)&nValue, sizeof(int)))
		{
			pService->Log(LOG_LEVEL_ERROR, "AcceptConnection set nodelay failed, err:%d -- %d", GetLastError(), WSAGetLastError());
			break;
		}

		// attach to iocp
		int ret = pService->GetCore()->AttachSocketToIOCP(pClient->mSocket.GetHandle(), (ULONG_PTR)pClient);
		if (ret != 0) {
			pService->Log(LOG_LEVEL_ERROR, "AcceptConnection attatch socket failed");
			break;
		}

		// get peer addr info
		if (!pClient->GetPeerAddrInfo()) {
			pService->Log(LOG_LEVEL_ERROR, "AcceptConnection get remote addr failed");
			break;
		}

		pClient->mConnState = CONN_CONNECTED;

		if (!pClient->PostRecv()) {
			pService->Log(LOG_LEVEL_ERROR, "AcceptConnection pos recv failed");
			break;
		}

		// new connection call back
		pService->mIOCallbackPtr->OnConnected(pClient->mLogicSocketID, pClient->mRemoteIP, pClient->mRemotePort);

		return true;
	} while (false);
	
	return false;
}

WT_END