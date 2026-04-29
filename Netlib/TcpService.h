#pragma once

#include "Config.h"
#include "tool/ThreadLock.h"
#include "net/NetService.h"
#include "tool/Thread.h"
#include "IOContext.h"
#include "tool/util.h"
#include "TcpClient.h"
#include "tool/DynamicMemoryPool.h"
#include "tool/SnmpStatistic.h"
#include "tool/Semaphore.h"
#include "NetCoreIOCP.h"
#include "net/HttpService.h"

WT_BEGIN

class TcpAcceptor;
class TcpService
{
public:
	typedef std::map<unsigned int, TcpClient*> TcpClientMap;
	TcpService();
	virtual ~TcpService();

	int StartNetService(
		const char* ip,
		unsigned short listenPort,
		int listenBacklog,
		int maxConnection,
		int maxPacketSize,
		int packetSizeOffset,
		int clientTimeoutSec,
		int logLevel,
		NetCore* netCore,
		IIOCallback* callback,
		int protocolType
	);

	void StopNetService();

	int Send(unsigned int socketID, const void* pData, int nDataLen);

	int SendHttpResponse(HttpResponse& resp);

	unsigned int ConnectServer(const char* ip, unsigned short port, int waitMS);

	TcpClient* AllocTcpClient();
	//void ReleaseTcpClient(TcpClient* pClient);

	void Log(int level, const char* format, ...);

	IOContext* AllocSendIOContext();
	void  ReleaseSendIOContext(IOContext* context);

	void* AllocSendPacket();
	void  ReleaseSendPacket(void* packet);

	void* AllocRecvPacket();
	void  ReleaseRecvPacket(void* packet);

	IOContext* AllocExceptionIOContext();
	void  ReleaseExceptionIOContext(IOContext* context);

	void OnAcceptorReleased();

	TcpClientRef FindClient(unsigned int socketID);

	void RemoveClient(unsigned int socketID);

	int CloseSocket(unsigned int socketID);

	int GetConnectionCount();

	NetCoreIOCP* GetCore();

	int GetProtocolType();

	void TickUpdate();

protected:

	int CreateAllPool();
	void ReleaseAllPool();

	int CreateTcpClientPool(int maxConnection);
	int CreateSendIOContextPool(int sendQueueSize);
	int CreateSendPacketPool(int sendQueueSize, int maxSendPacketSize);
	int CreateRecvPacketPool(int recvQueueSize, int maxRecvPacketSize);
	int CreateExceptionContextPool();

	

private:
	void MakeLog(int srvID, unsigned short port, int logLevel);
	void checkStatus();
	void logStats();
	int  GetClientPoolUsed();
	int  GetSendIOContextPoolUsed();
	int  GetSendPoolUsed();
	int  GetRecvPoolUsed();
	int  GetExceptionPoolUsed();

public:
	// callback
	IIOCallback*         mIOCallbackPtr;
	int                  mMaxConnection;
	int                  mMaxPacketSize;
	int                  mMaxExceptionContext;
	int                  mPacketSizeOffset;
	int                  mClientTimeoutSec;

	CSnmpStatistic       mStats;

private:
	NetCore*             mNetCore;
	TcpAcceptor*         mTcpAcceptor;
	HANDLE               mReleaeseAcceptorEvent;
	int                  mServiceID;

	std::string          mLogFileName;

	// tcp client Pool
	CThreadLock          mTcpClientPoolLock;
	DynamicMemoryPool*   mTcpClientPoolPtr;
	TcpClientMap         mTcpClientMap;

	// send context Pool
	CThreadLock          mSendIOContextPoolLock;
	DynamicMemoryPool*   mSendIOContextPoolPtr;

	// send packet Pool
	CThreadLock          mSendPacketPoolLock;
	DynamicMemoryPool*   mSendPacketPoolPtr;

	// recv packet Pool
	CThreadLock          mRecvPacketPoolLock;
	DynamicMemoryPool*   mRecvPacketPoolPtr;

	// exception context Pool
	CThreadLock          mExceptionContextPoolLock;
	DynamicMemoryPool*   mExceptionContextPoolPtr;

	wt::CTimer           mCheckStatusTimer;
	wt::CTimer           mLogStatsTimer;

	unsigned short       mListenPort;

	bool                 mMarkedStop;
	int                  mProtocolType;
};

//

WT_END