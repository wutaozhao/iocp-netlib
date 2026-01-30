
#include "TcpService.h"
#include "NetService.h"
#include "SocketHelper.h"
#include "TcpClient.h"
#include "TcpAcceptor.h"

WT_BEGIN

TcpService::TcpService():mServiceID(0)
{
	mNetCore = NULL;
	mTcpClientCachePtr = NULL;
	mSendIOContextCachePtr = NULL;
	mSendPacketCachePtr = NULL;
	mRecvPacketCachePtr = NULL;
	mExceptionContextCachePtr = NULL;

	mTcpAcceptor = NULL;
	mReleaeseAcceptorEvent = NULL;

	mIOCallbackPtr = NULL;

	mStopCheckStatusThread = false;

	mListenPort = 0;
	mMaxConnection = 0;
	mMaxPacketSize = 0;

	mMaxExceptionContext = 0;

	mPacketSizeOffset = 0;

	mMarkedStop = false;

	mStats[def("ClientCacheUsed")+
		def("SendIOContextUsed")+
		def("SendCacheUsed")+ 
		def("RecvCacheUsed")+
	    def("ExceptionContextUsed")+
	    def("Timeout")+
	    def("Accept")+
		def("AcceptSucc")+
	    def("Connection")];
}

TcpService::~TcpService()
{
	StopNetService();
}

int TcpService::StartNetService(
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
)
{
	int ret = 0;

	do
	{
		Log(LOG_LEVEL_INFO, "begin init service");
		mMarkedStop = false;
		mStopCheckStatusThread = false;
		mNetCore = netCore;
		//
		if (mServiceID == 0) {
			// increment
			mServiceID = reinterpret_cast<NetCoreIOCP*>(mNetCore->GetCore())->GetNextServiceID();
			MakeLog(mServiceID, listenPort, logLevel);
		}

		mListenPort = listenPort;
		mMaxConnection = maxConnection;
		mMaxPacketSize = maxPacketSize;
		mPacketSizeOffset = packetSizeOffset;
		mClientTimeoutSec = clientTimeoutSec;

		mMaxExceptionContext = maxConnection;
		
		mIOCallbackPtr = callback;

		// create pools
		ret = CreateAllCache();
		if (ret != 0) {
			break;
		}

		mCheckStatusTimer.SetTimer(5000);
		mLogStatsTimer.SetTimer(60 * 1000);
		mCheckStatusThread = new(std::nothrow) wt::Thread(this, &TcpService::CheckStatusProc);
		if (!mCheckStatusThread)
		{
			Log(LOG_LEVEL_ERROR, "create check timeout thread failed");
			ret = NSE_SYSTEM_ERROR;
			break;
		}
		if (!mCheckStatusThread->Start()) {
			delete mCheckStatusThread;
			mCheckStatusThread = NULL;
			Log(LOG_LEVEL_ERROR, "start check timeout thread failed");
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		Log(LOG_LEVEL_INFO, "init service will check listen port");

		if (listenPort == 0)
			break;

		mReleaeseAcceptorEvent = ::CreateEvent(NULL, true, false, NULL);
		if (NULL == mReleaeseAcceptorEvent)
		{
			Log(LOG_LEVEL_ERROR, "create tcp acceptor release event failed");
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		// listen
		mTcpAcceptor = new(std::nothrow) TcpAcceptor(this);
		if (NULL == mTcpAcceptor) {
			Log(LOG_LEVEL_ERROR, "new tcp acceptor failed");
			ret = NSE_SYSTEM_ERROR;
			break;
		}
		if (listenBacklog == 0){
			listenBacklog = 8;
		}
		ret = mTcpAcceptor->Initialize(ip, listenPort, listenBacklog);
		if (ret != 0)
		{
			delete mTcpAcceptor;
			mTcpAcceptor = NULL;
			Log(LOG_LEVEL_ERROR, "init tcp acceptor failed, ret: %d", ret);
			break;
		}

		Log(LOG_LEVEL_INFO, "init tcp acceptor success");

	} while (false);

	return ret;
}

void TcpService::StopNetService()
{
	if (mMarkedStop) {
		return;
	}

	Log(LOG_LEVEL_INFO, "begin stop net service");

	mStopCheckStatusThread = true;
	mMarkedStop = true;
	if (mTcpAcceptor != NULL && mReleaeseAcceptorEvent != NULL) {
		mTcpAcceptor->PostCancel();
		// here we don't delete mTcpAcceptor, delete it on iocp worker thread
#if (_WIN32_WINNT >= 0x0600)
		Log(LOG_LEVEL_INFO, "begin wait releas acceptor, time:%llu", GetTickCount64());
#else
		Log(LOG_LEVEL_INFO, "begin wait releas acceptor, time:%u", GetTickCount());
#endif
		WaitForSingleObject(mReleaeseAcceptorEvent, 1000);
#if (_WIN32_WINNT >= 0x0600)
		Log(LOG_LEVEL_INFO, "finish wait releas acceptor, time:%llu", GetTickCount64());
#else
		Log(LOG_LEVEL_INFO, "finish wait releas acceptor, time:%u", GetTickCount());
#endif
		CloseHandle(mReleaeseAcceptorEvent);
		mReleaeseAcceptorEvent = NULL;
		mTcpAcceptor = NULL;
	}

	Log(LOG_LEVEL_INFO, "begin cancel all client");
	{
		LOCK_GUARD(mSendIOContextCacheLock);
		TcpClientMap::iterator itor = mTcpClientMap.begin();
		while (itor != mTcpClientMap.end() && !itor->second->IsAccepting()) {
			itor->second->CancelIO();
			++itor;
		}
	}

	CTimer exitTimer;
	exitTimer.SetTimer(600);
	while (!exitTimer.IsTimed()) {
		LOCK_GUARD(mSendIOContextCacheLock);
		if (mTcpClientMap.size() == 0)
		{
			Log(LOG_LEVEL_INFO, "begin cancel all client");
			break;
		}
		else {
			Sleep(100);
		}
	}

	ReleaseAllCache();
}

void TcpService::OnAcceptorReleased()
{
	Log(LOG_LEVEL_INFO, "acceptor released...");
	SetEvent(mReleaeseAcceptorEvent);
}

int TcpService::CreateAllCache()
{
	int ret = 0;

	do
	{
		ret = CreateTcpClientCache(mMaxConnection);
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create tcp client pool failed");
			break;
		}

		ret = CreateSendIOContextCache(mMaxConnection);
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create send io context pool failed");
			break;
		}

		ret = CreateSendPacketCache(mMaxConnection, mMaxPacketSize);
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create send packet pool failed");
			break;
		}

		ret = CreateRecvPacketCache(mMaxConnection, mMaxPacketSize);
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create recv packet pool failed");
			break;
		}

		ret = CreateExceptionContextCache();
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create exception context pool failed");
			break;
		}
	} while (false);

	return ret;
}

void TcpService::ReleaseAllCache()
{
	Log(LOG_LEVEL_INFO, "begin release all pool service");
	// release tcp client pool
	if (true) {
		LOCK_GUARD(mTcpClientCacheLock);
		SafeDelete(mTcpClientCachePtr);
	}
	// release send io context pool
	if (true) {
		LOCK_GUARD(mSendIOContextCacheLock);
		SafeDelete(mSendIOContextCachePtr);
	}
	// release send packet pool
	if (true) {
		LOCK_GUARD(mSendPacketCacheLock);
		SafeDelete(mSendPacketCachePtr);
	}
	if (true) {
		LOCK_GUARD(mRecvPacketCacheLock);
		SafeDelete(mRecvPacketCachePtr);
	}
	if (true) {
		LOCK_GUARD(mExceptionContextCacheLock);
		SafeDelete(mExceptionContextCachePtr);
	}
	Log(LOG_LEVEL_INFO, "end release all pool service");
}

void TcpService::MakeLog(int srvID, unsigned short port, int logLevel)
{
	//
	time_t ulNow = { 0 };
	time(&ulNow);
	struct tm pNow;
	localtime_s(&pNow, &ulNow);

	char  szTimeBuffer[256] = { 0 };
	_snprintf_s(szTimeBuffer,
		255,
		_TRUNCATE,
		"%04d%02d%02d%02d%02d%02d_%d_%d.log",
		pNow.tm_year + 1900,
		pNow.tm_mon + 1,
		pNow.tm_mday,
		pNow.tm_hour,
		pNow.tm_min,
		pNow.tm_sec,
		(int)port,
		srvID);
	mLogFileName = szTimeBuffer;

	GetCore()->CreateLog(mLogFileName.c_str(), logLevel);
}

NetCoreIOCP* TcpService::GetCore()
{
	if (!mNetCore || !mNetCore->GetCore())
		return NULL;

	NetCoreIOCP* core = reinterpret_cast<NetCoreIOCP*>(mNetCore->GetCore());
	return core;
}

void TcpService::Log(int level, const char* format, ...)
{
	NetCoreIOCP* core = GetCore();
	if (!core)
		return;

	va_list argp;
	va_start(argp, format);
	core->LogV(mLogFileName.c_str(), level, format, argp);
	va_end(argp);
}

int TcpService::CreateTcpClientCache(int maxConnection)
{
	mTcpClientCachePtr = new(std::nothrow) ObjectCache(sizeof(TcpClient), maxConnection);
	if (NULL == mTcpClientCachePtr) {
		Log(LOG_LEVEL_ERROR, "create tcp client pool failed");
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

TcpClient* TcpService::AllocTcpClient()
{
	if (mMarkedStop) 
	{
		return NULL;
	}
	unsigned int nextID = GetCore()->GetNextLogicSocketID();
	LOCK_GUARD(mTcpClientCacheLock);
	if (NULL == mTcpClientCachePtr) {
		return NULL;
	}

	int code = 0;
	do
	{
		TcpClient* pClient = mTcpClientCachePtr->Create<TcpClient>();
		if (!pClient) {
			code = 1;
			break;
		}
		pClient->Reset();

		// allocate logic socket and normal socket
		pClient->mLogicSocketID = nextID;
		if (!pClient->CreateSocket()) {
			code = 2;
			mTcpClientCachePtr->Free(pClient);
			break;
		}

		pClient->mTcpService = this;

		mTcpClientMap.insert(std::make_pair(nextID, pClient));

		return pClient;
	} while (false);

	Log(LOG_LEVEL_ERROR, "AllocTcpClient failed, code:%d", code);
	
	return NULL;
}

/*
void TcpService::ReleaseTcpClient(TcpClient* pClient)
{
	if (NULL == pClient)
		return;

	LOCK_GUARD(mTcpClientCacheLock);
	mTcpClientMap.erase(pClient->mLogicSocketID);
	mTcpClientCachePtr->Free(pClient);
}*/

TcpClientRef TcpService::FindClient(unsigned int socketID)
{
	LOCK_GUARD(mTcpClientCacheLock);
	TcpClientMap::iterator itor = mTcpClientMap.find(socketID);
	if (itor == mTcpClientMap.end()) {
		return NULL;
	}

	return itor->second;
}

void TcpService::RemoveClient(unsigned int socketID)
{
	LOCK_GUARD(mTcpClientCacheLock);
	TcpClientMap::iterator itor = mTcpClientMap.find(socketID);
	if (itor == mTcpClientMap.end())
		return;
	if (itor->second->IsActive()) {
		Log(LOG_LEVEL_ERROR, "find client is active");
		itor->second->MarkDestroy();
	}
	else {
		mTcpClientCachePtr->Free(itor->second);
		mTcpClientMap.erase(itor);
	}
}

int TcpService::CreateSendIOContextCache(int sendQueueSize)
{
	mSendIOContextCachePtr = new(std::nothrow) ObjectCache(sizeof(IOContext), sendQueueSize);
	if (NULL == mSendIOContextCachePtr) {
		Log(LOG_LEVEL_ERROR, "create send io context pool failed");
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

IOContext* TcpService::AllocSendIOContext()
{
	LOCK_GUARD(mSendIOContextCacheLock);
	if (NULL == mSendIOContextCachePtr)
	{
		return NULL;
	}

	return mSendIOContextCachePtr->Create<IOContext>();
}

void TcpService::ReleaseSendIOContext(IOContext* context)
{
	if (NULL == context)
		return;

	LOCK_GUARD(mSendIOContextCacheLock);
	mSendIOContextCachePtr->Free(context);
}

int TcpService::CreateSendPacketCache(int sendQueueSize, int maxSendPacketSize)
{
	mSendPacketCachePtr = new(std::nothrow) ObjectCache(maxSendPacketSize, sendQueueSize);
	if (NULL == mSendPacketCachePtr) {
		Log(LOG_LEVEL_ERROR, "create send packet pool failed");
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

void* TcpService::AllocSendPacket()
{
	LOCK_GUARD(mSendPacketCacheLock);
	if (NULL == mSendPacketCachePtr)
	{
		return NULL;
	}

	void* packet = mSendPacketCachePtr->Create();
	if (!packet) {
		return NULL;
	}
	memset(packet, 0, mMaxPacketSize);

	return packet;
}

void  TcpService::ReleaseSendPacket(void* packet)
{
	if (NULL == packet)
		return;

	LOCK_GUARD(mSendPacketCacheLock);
	mSendPacketCachePtr->Free(packet);
}

int TcpService::CreateRecvPacketCache(int recvQueueSize, int maxRecvPacketSize)
{
	mRecvPacketCachePtr = new(std::nothrow) ObjectCache(maxRecvPacketSize, recvQueueSize);
	if (NULL == mRecvPacketCachePtr) {
		Log(LOG_LEVEL_ERROR, "create recv packet pool failed");
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

void* TcpService::AllocRecvPacket()
{
	LOCK_GUARD(mRecvPacketCacheLock);
	if (NULL == mRecvPacketCachePtr)
	{
		return NULL;
	}

	void* packet = mRecvPacketCachePtr->Create();
	if (!packet)
		return NULL;

	memset(packet, 0, mMaxPacketSize);

	return packet;
}

void  TcpService::ReleaseRecvPacket(void* packet)
{
	if (NULL == packet)
		return;

	LOCK_GUARD(mRecvPacketCacheLock);
	mRecvPacketCachePtr->Free(packet);
}

int TcpService::CreateExceptionContextCache()
{
	mExceptionContextCachePtr = new(std::nothrow) ObjectCache(sizeof(IOContext), mMaxExceptionContext);
	if (NULL == mExceptionContextCachePtr) {
		Log(LOG_LEVEL_ERROR, "create exception io context failed");
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

IOContext* TcpService::AllocExceptionIOContext()
{
	LOCK_GUARD(mExceptionContextCacheLock);
	if (NULL == mExceptionContextCachePtr)
	{
		return NULL;
	}

	IOContext* context = mExceptionContextCachePtr->Create<IOContext>();
	return context;
}

void  TcpService::ReleaseExceptionIOContext(IOContext* context)
{
	if (NULL == context)
		return;
	LOCK_GUARD(mExceptionContextCacheLock);
	mExceptionContextCachePtr->Free(context);
}

int TcpService::Send(unsigned int socketID, const void* pData, int nDataLen)
{
	// use a memeroy block to cache the data, release the memory after send completion
	// so here we need define the send queue size according to the memory pool
	if (nDataLen > mMaxPacketSize) {
		return NSE_ILLEGAL_SEND_PACKET;
	}

	/**/
	TcpClientRef clientRef = FindClient(socketID);
	if (!clientRef) {
		return NSE_INVALID_SOCKET;
	}

	return clientRef->PostSend(pData, nDataLen);
}

void TcpService::CheckStatusProc()
{
	Log(LOG_LEVEL_INFO, "TcpService::CheckStatusProc() start");
	while (!mStopCheckStatusThread) {
		if (mCheckStatusTimer.IsTimed()) {
			checkStatus();
		}
		else if (mLogStatsTimer.IsTimed()) {
			logStats();
		}
		else {
			Sleep(100);
		}
	}
	Log(LOG_LEVEL_INFO, "TcpService::CheckStatusProc() stop");
}

void TcpService::checkStatus()
{
	bool hasClientNotReleased = false;
	do
	{
#if (_WIN32_WINNT >= 0x0600)
		unsigned long long now = GetTickCount64();
#else
		unsigned int now = GetTickCount();
#endif
		LOCK_GUARD(mTcpClientCacheLock);
		TcpClientMap::iterator itor = mTcpClientMap.begin();
		while (itor != mTcpClientMap.end()) {
			if (!itor->second->IsAccepting()) {
				hasClientNotReleased = true;
				if (mListenPort != 0) {
#if (_WIN32_WINNT >= 0x0600)
					if ((now > itor->second->mLastTickTime) && (now - itor->second->mLastTickTime) > (mClientTimeoutSec * 1000))
#else
					if ((now > itor->second->mLastTickTime) && GetMsInterval(now, itor->second->mLastTickTime) > ((unsigned int)mClientTimeoutSec * 1000))
#endif
					
					{
						itor->second->SetTimeout();
						mStats["Timeout"]++;
					}
				}

				if (itor->second->IsMarkedDestroy() && !itor->second->IsActive()) {
					Log(LOG_LEVEL_INFO, "TcpService::checkStatus() find client marked to destroy:%u", itor->second->mLogicSocketID);
					mTcpClientCachePtr->Free(itor->second);
					itor = mTcpClientMap.erase(itor);
					continue;
				}
			}
			
			++itor;
		}
	} while (false);
	
	/*
	if (!hasClientNotReleased && mMarkedStop) {
		ReleaseAllCache();
		mStopCheckStatusThread = true;
	}
	*/
}

int  TcpService::GetClientCacheUsed()
{
	LOCK_GUARD(mTcpClientCacheLock);
	return mTcpClientCachePtr ? (int)mTcpClientCachePtr->GetUsedCount() : 0;
}

int  TcpService::GetSendIOContextCacheUsed()
{
	LOCK_GUARD(mSendIOContextCacheLock);
	return mSendIOContextCachePtr ? (int)mSendIOContextCachePtr->GetUsedCount() : 0;
}

int  TcpService::GetSendCacheUsed()
{
	LOCK_GUARD(mSendPacketCacheLock);
	return mSendPacketCachePtr ? (int)mSendPacketCachePtr->GetUsedCount() : 0;
}

int  TcpService::GetRecvCacheUsed()
{
	LOCK_GUARD(mRecvPacketCacheLock);
	return mRecvPacketCachePtr ? (int)mRecvPacketCachePtr->GetUsedCount() : 0;
}

int  TcpService::GetExceptionCacheUsed()
{
	LOCK_GUARD(mExceptionContextCacheLock);
	return mExceptionContextCachePtr ? (int)mExceptionContextCachePtr->GetUsedCount() : 0;
}

void TcpService::logStats()
{
	mStats["ClientCacheUsed"] = GetClientCacheUsed();
	mStats["SendIOContextUsed"] = GetSendIOContextCacheUsed();
	mStats["SendCacheUsed"] = GetSendCacheUsed();
	mStats["RecvCacheUsed"] = GetRecvCacheUsed();
	mStats["ExceptionContextUsed"] = GetExceptionCacheUsed();
	mStats["Connection"] = GetConnectionCount();

	std::string stats;
	mStats.Snapshot();
	mStats.dump(stats);
	Log(LOG_LEVEL_INFO, "%s", stats.c_str());
	mStats.Reset();
}

int TcpService::CloseSocket(unsigned int socketID)
{
	TcpClientRef c = FindClient(socketID);
	if (!c) {
		return NSE_INVALID_SOCKET;
	}
	
	return c->PostSpecial(IOCP_OP_CLOSED);
}

unsigned int TcpService::ConnectServer(const char* ip, unsigned short port, int waitMS)
{
	unsigned int nSocketID = 0;

	TcpClientRef c = NULL;
	do
	{
		Log(LOG_LEVEL_INFO, "TcpService::ConnectServer post connect:%d", (int)port);
		if (mListenPort != 0) {
			Log(LOG_LEVEL_ERROR, "TcpService::ConnectServer find having listen port:%d", (int)mListenPort);
			break;
		}

		if (!mConnectSemaphore.Create()) {
			Log(LOG_LEVEL_ERROR, "TcpService::ConnectServer create sem failed, err:%d", GetLastError());
			break;
		}

		c = AllocTcpClient();
		if (!c) {
			Log(LOG_LEVEL_ERROR, "TcpService::ConnectServer cannot alloc client");
			break;
		}
		nSocketID = c->mLogicSocketID;

		
		if (!c->PostConnect(ip, port))
		{
			Log(LOG_LEVEL_ERROR, "TcpService::ConnectServer post connect failed");
			nSocketID = 0;
			break;
		}

		if (!mConnectSemaphore.Wait(waitMS)) {
			TcpClient* client = c.get();
			c.UnRef();
			client->CancelIO();
			Log(LOG_LEVEL_ERROR, "TcpService::ConnectServer wait timeout:%d", (int)port);
			nSocketID = 0;
			break;
		}
		
	} while (false);
	
	return nSocketID;
}

void TcpService::PostConnectSucc()
{
	mConnectSemaphore.Post();
}

int TcpService::GetConnectionCount()
{
	int count = 0;
	LOCK_GUARD(mTcpClientCacheLock);
	TcpClientMap::iterator itor = mTcpClientMap.begin();
	while (itor != mTcpClientMap.end()) {
		if (!itor->second->IsAccepting()) {
			count++;
		}
		++itor;
	}
	return count;
}

WT_END