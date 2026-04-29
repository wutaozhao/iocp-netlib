#pragma once

#include "Config.h"

enum IOCP_OP_TYPE
{
	IOCP_OP_IDLE = 0,                           
	IOCP_OP_ACCEPT,                         
	IOCP_OP_RECV,                                
	IOCP_OP_SEND,                                
	IOCP_OP_TIMEOUT,
	IOCP_OP_CLOSED,
	IOCP_OP_EXIT,                                
	IOCP_OP_CONNECTEX,
	IOCP_OP_CONNECTEX_TIMEOUT,
};

#pragma pack(1)
class IOContext {
public:
	IOContext(void);
	~IOContext(void);
public:
	void Reset();
public:
	//
	OVERLAPPED    mOverlapped;
	//
	WSABUF        mBuffer;
	//
	int           mOperationType;
	//
	void*         mPointer;
	//
	void*         mTcpClient;

	void*         mTcpService;

	unsigned long long  mTime;
};

#pragma pack()

__interface IIOCPCompletionCB
{
	virtual void OnCompleteOperation(bool bSuccess, IOContext* pPID, unsigned int numberOfBytes, unsigned int  errorCode = 0) = 0;
};
