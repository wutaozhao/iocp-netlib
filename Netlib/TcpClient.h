#pragma once

#include "Config.h"
#include "IOContext.h"
#include "SocketHelper.h"

WT_BEGIN

const int TCP_CLIENT_RECV_BUFF_SIZE = 4096;

enum TcpConnState
{
	CONN_CLOSED = 0,
	CONN_CONNECTED,   // 正常通信
	CONN_CLOSING,      // 已请求关闭，等待 I/O 回收
};

class TcpService;
class TcpClient:public IIOCPCompletionCB
{
public:
	TcpClient();
	~TcpClient();

	int Initialize(int maxPacketSize);

	void Reset();

	bool CreateSocket();

	void ShutDown();

	void CloseSocket();

	void SetRecvContextToAccept();

	void SetRecvContextToRecv();

	void SetRecvContextToConnect();

	bool GetPeerAddrInfo();

	bool PostRecv();

	int  PostSend(const void* data, int dataLen);

	int PostSpecial(int opType);

	// only invoked when the service exit
	void CancelIO();

	void OnCompleteOperation(bool bSuccess, IOContext* pContext, unsigned int numberOfBytes, unsigned int  errorCode = 0);

	void SetTimeout();

	long IncreaseRef();
	long DecreaseRef();

	bool IsActive();
	void MarkDestroy();
	bool IsMarkedDestroy();

	bool IsAccepting();

	bool PostConnect(const char* ip, unsigned short port);

private:
	long IncreaseIOCount();
	long DecreaseIOCount();

	int OnRecvCompletion(IOContext* pContext, unsigned int nNumberOfBytes);
	int OnSendCompletion(IOContext* pContext, unsigned int nNumberOfBytes);
	void OnConnectExCompletion(unsigned int errorCode);

	void SetDisconnect(int errCode);

	int SplitPacket(char* pData, int dataLen);

	void CheckRelease();

	void UpdateLastTickTime();

public:
	TcpService*   mTcpService;
	unsigned int  mLogicSocketID;

	volatile long  mPendingIO;
	volatile long  mReferenceCount;

	volatile long  mConnState;

	IOContext      mRecvContext;
	unsigned long long  mLastTickTime;
	SocketHelper   mSocket;

	unsigned long  mRemoteIP;
	unsigned short mRemotePort;

	char           mRecvBuffer[TCP_CLIENT_RECV_BUFF_SIZE];  
	int            mRecvBytes;
	char*          mRecvPacketBuffer;

	LPFN_CONNECTEX mConnectExPtr;

	int            mLastErrorCode;

	bool           mMarkedDestroy;
};

class TcpClientRef
{
public:
	TcpClientRef()
	{
		mClientPtr = NULL;
	}

	~TcpClientRef()
	{
		if (mClientPtr != NULL)
		{
			mClientPtr->DecreaseRef();
			mClientPtr = NULL;
		}
	}

	TcpClientRef(TcpClient* pClient)
	{
		mClientPtr = pClient;
		if (mClientPtr != NULL)
			mClientPtr->IncreaseRef();
	}

	TcpClientRef(const TcpClientRef& rhs)
	{
		mClientPtr = rhs.mClientPtr;
		if (mClientPtr != NULL)
			mClientPtr->IncreaseRef();
	}

	TcpClientRef& operator = (const TcpClientRef& rhs)
	{
		if (mClientPtr)
		{
			mClientPtr->DecreaseRef();
		}
		mClientPtr = rhs.mClientPtr;
		if (mClientPtr != NULL)
			mClientPtr->IncreaseRef();
		return *this;
	}

	TcpClientRef& operator = (TcpClient* pClient)
	{
		if (mClientPtr)
		{
			mClientPtr->DecreaseRef();
		}
		mClientPtr = pClient;
		if (mClientPtr)
			mClientPtr->IncreaseRef();
		return *this;
	}

	TcpClient* operator ->()
	{
		return mClientPtr;
	}

	bool operator !()
	{
		return mClientPtr == NULL;
	}

	void UnRef()
	{
		if (mClientPtr)
		{
			mClientPtr->DecreaseRef();
			mClientPtr = NULL;
		}
	}

	TcpClient* get()
	{
		return mClientPtr;
	}

private:
	TcpClient* mClientPtr;
};

WT_END