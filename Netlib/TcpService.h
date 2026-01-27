#pragma once

#include "Config.h"
#include "ThreadLock.h"
#include "NetService.h"
#include "Thread.h"
#include "IOContext.h"
#include "util.h"
#include "TcpClient.h"
#include "ObjectPool.h"
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
		int maxSendPacketSize,
		int sendQueueSize,
		int maxRecvPacketSize,
		int recvQueueSize,
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

	int CreateAllPool();
	void ReleaseAllPool();

	int CreateTcpClientPool(int maxConnection);
	int CreateSendIOContextPool(int sendQueueSize);
	int CreateSendPacketPool(int sendQueueSize, int maxSendPacketSize);
	int CreateRecvPacketPool(int recvQueueSize, int maxRecvPacketSize);
	int CreateExceptionContextPool();

	void CheckStatusProc();

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
	IIOCallback*   mIOCallbackPtr;
	int            mMaxConnection;
	int            mMaxSendPacketSize;
	int            mSendQueueSize;
	int            mMaxRecvPacketSize;
	int            mRecvQueueSize;
	int            mMaxExcptionContext;
	int            mPacketSizeOffset;
	int            mClientTimeoutSec;

	CSnmpStatistic mStats;

private:
	NetCore*     mNetCore;
	TcpAcceptor* mTcpAcceptor;
	HANDLE       mReleaeseAcceptorEvent;
	int          mServiceID;

	std::string    mLogFileName;

	// tcp client pool
	CThreadLock    mTcpClientPoolLock;
	CObjectPool*   mTcpClientPoolPtr;
	TcpClientMap   mTcpClientMap;

	// send context pool
	CThreadLock    mSendIOContextPoolLock;
	CObjectPool*   mSendIOContextPoolPtr;

	// send packet pool
	CThreadLock    mSendPacketPoolLock;
	CObjectPool*   mSendPacketPoolPtr;

	// recv packet pool
	CThreadLock    mRecvPacketPoolLock;
	CObjectPool*   mRecvPacketPoolPtr;

	// exception context pool
	CThreadLock    mExceptionContextPoolLock;
	CObjectPool*   mExceptionContextPoolPtr;

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