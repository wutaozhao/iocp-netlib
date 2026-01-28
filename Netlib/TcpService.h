#pragma once

#include "Config.h"
#include "ThreadLock.h"
#include "NetService.h"
#include "Thread.h"
#include "IOContext.h"
#include "util.h"
#include "TcpClient.h"
#include "ObjectCache.h"
#include "SnmpStatistic.h"
#include "Semaphore.h"
#include "NetCoreIOCP.h"

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
		IIOCallback* callback
	);

	void StopNetService();

	int Send(unsigned int socketID, const void* pData, int nDataLen);

	unsigned int ConnectServer(const char* ip, unsigned short port, int waitMS);

	void PostConnectSucc();

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

protected:

	int CreateAllCache();
	void ReleaseAllCache();

	int CreateTcpClientCache(int maxConnection);
	int CreateSendIOContextCache(int sendQueueSize);
	int CreateSendPacketCache(int sendQueueSize, int maxSendPacketSize);
	int CreateRecvPacketCache(int recvQueueSize, int maxRecvPacketSize);
	int CreateExceptionContextCache();

	void CheckStatusProc();

private:
	void MakeLog(int srvID, unsigned short port, int logLevel);
	void checkStatus();
	void logStats();
	int  GetClientCacheUsed();
	int  GetSendIOContextCacheUsed();
	int  GetSendCacheUsed();
	int  GetRecvCacheUsed();
	int  GetExceptionCacheUsed();

public:
	// callback
	IIOCallback*   mIOCallbackPtr;
	int            mMaxConnection;
	int            mMaxPacketSize;
	int            mMaxExceptionContext;
	int            mPacketSizeOffset;
	int            mClientTimeoutSec;

	CSnmpStatistic mStats;

private:
	NetCore*     mNetCore;
	TcpAcceptor* mTcpAcceptor;
	HANDLE       mReleaeseAcceptorEvent;
	int          mServiceID;

	std::string    mLogFileName;

	// tcp client Cache
	CThreadLock    mTcpClientCacheLock;
	ObjectCache*   mTcpClientCachePtr;
	TcpClientMap   mTcpClientMap;

	// send context Cache
	CThreadLock    mSendIOContextCacheLock;
	ObjectCache*   mSendIOContextCachePtr;

	// send packet Cache
	CThreadLock    mSendPacketCacheLock;
	ObjectCache*   mSendPacketCachePtr;

	// recv packet Cache
	CThreadLock    mRecvPacketCacheLock;
	ObjectCache*   mRecvPacketCachePtr;

	// exception context Cache
	CThreadLock    mExceptionContextCacheLock;
	ObjectCache*   mExceptionContextCachePtr;

	bool           mStopCheckStatusThread;
	wt::Thread*    mCheckStatusThread;
	wt::CTimer     mCheckStatusTimer;
	wt::CTimer     mLogStatsTimer;

	unsigned short mListenPort;

	CSemaphore     mConnectSemaphore;

	bool           mMarkedStop;
};

//

WT_END