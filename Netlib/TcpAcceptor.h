#pragma once

#include "Config.h"
#include "SocketHelper.h"
#include "IOContext.h"

WT_BEGIN

/*we test cancelIO on Acceptor later, */

class TcpClient;
class TcpAcceptor:public IIOCPCompletionCB {
public:
	TcpAcceptor(void* tcpService);
	virtual ~TcpAcceptor();

	int Initialize(const std::string& listenIP, unsigned short listenPort, int backLog);

	void UnInitialize();

	void OnCompleteOperation(bool bSuccess, IOContext* pPID, unsigned int numberOfBytes, unsigned int  errorCode = 0);

	void PostCancel();

protected:
	bool PostAccept();

	bool AcceptConnection(TcpClient* pClient);

	long IncrementPending();
	long DecrementPending();

private:
	void*           mTcpService;
	SocketHelper    mSocket;
	LPFN_ACCEPTEX   mAcceptExPointer;
	volatile long   mPendingAcceptCount;
};

WT_END