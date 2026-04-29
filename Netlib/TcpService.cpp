
#include "TcpService.h"
#include "net/NetService.h"
#include "SocketHelper.h"
#include "TcpClient.h"
#include "TcpAcceptor.h"

WT_BEGIN

TcpService::TcpService():mServiceID(0)
{
	mNetCore = NULL;
	mTcpClientPoolPtr = NULL;
	mSendIOContextPoolPtr = NULL;
	mSendPacketPoolPtr = NULL;
	mRecvPacketPoolPtr = NULL;
	mExceptionContextPoolPtr = NULL;

	mTcpAcceptor = NULL;
	mReleaeseAcceptorEvent = NULL;

	mIOCallbackPtr = NULL;

	mListenPort = 0;
	mMaxConnection = 0;
	mMaxPacketSize = 0;

	mMaxExceptionContext = 0;

	mPacketSizeOffset = 0;

	mMarkedStop = false;
	mProtocolType = NET_PROTOCOL_TCP;

	mStats[def("ClientPoolUsed")+
		def("SendIOContextUsed")+
		def("SendPoolUsed")+ 
		def("RecvPoolUsed")+
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
	IIOCallback* callback,
	int protocolType
)
{
	int ret = 0;

	do
	{
		Log(LOG_LEVEL_INFO, "begin init service");
		mMarkedStop = false;
		mNetCore = netCore;
		//
		if (mServiceID == 0) {
			// increment
			mServiceID = reinterpret_cast<NetCoreIOCP*>(mNetCore->GetCore())->GetNextServiceID();

			NetCoreIOCP* core = GetCore();
			if (core) {
				core->AddService(mServiceID, this);
			}

			MakeLog(mServiceID, listenPort, logLevel);
		}

		mListenPort = listenPort;
		mMaxConnection = maxConnection;
		mMaxPacketSize = maxPacketSize;
		mPacketSizeOffset = packetSizeOffset;
		mClientTimeoutSec = clientTimeoutSec;

		mMaxExceptionContext = maxConnection;
		
		mIOCallbackPtr = callback;
		mProtocolType = protocolType;

		// create pools
		ret = CreateAllPool();
		if (ret != 0) {
			break;
		}

		mCheckStatusTimer.SetTimer(5000);
		mLogStatsTimer.SetTimer(60 * 1000);

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
	mMarkedStop = true;

	NetCoreIOCP* core = GetCore();
	if (core) {
		core->RemoveService(mServiceID);
	}

	Log(LOG_LEVEL_INFO, "begin stop net service");
	// stop acceptor
	if (mTcpAcceptor != NULL && mReleaeseAcceptorEvent != NULL) {
		Log(LOG_LEVEL_INFO, "begin wait release acceptor");
		mTcpAcceptor->PostCancel();
		// here we don't delete mTcpAcceptor, delete it on iocp worker thread
		WaitForSingleObject(mReleaeseAcceptorEvent, 1000);
		CloseHandle(mReleaeseAcceptorEvent);
		mReleaeseAcceptorEvent = NULL;
		mTcpAcceptor = NULL;
		Log(LOG_LEVEL_INFO, "end wait release acceptor");
	}

	Log(LOG_LEVEL_INFO, "begin cancel all client");
	// cancel all client
	{
		LOCK_GUARD(mTcpClientPoolLock);
		TcpClientMap::iterator itor = mTcpClientMap.begin();
		while (itor != mTcpClientMap.end()) {
			if (!itor->second->IsAccepting())
			{
				itor->second->CancelIO();
			}
			++itor;
		}
	}

	// stop check thread, maybe some clients are active but unref
	Log(LOG_LEVEL_INFO, "enc cancel all client");

	// wait a while for clients exit
	CTimer exitTimer;
	exitTimer.SetTimer(600);
	while (!exitTimer.IsTimed()) {
		LOCK_GUARD(mTcpClientPoolLock);
		if (mTcpClientMap.size() == 0)
		{
			Log(LOG_LEVEL_INFO, "all client canceled");
			break;
		}
		else {
			Sleep(100);
		}
	}

	// release all pool
	ReleaseAllPool();

	Log(LOG_LEVEL_INFO, "stop finished, time:%llu", GetCPUTickTime());
}

void TcpService::OnAcceptorReleased()
{
	Log(LOG_LEVEL_INFO, "acceptor released...");
	SetEvent(mReleaeseAcceptorEvent);
}

int TcpService::CreateAllPool()
{
	int ret = 0;

	do
	{
		ret = CreateTcpClientPool(mMaxConnection);
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create tcp client pool failed");
			break;
		}

		ret = CreateSendIOContextPool(mMaxConnection);
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create send io context pool failed");
			break;
		}

		ret = CreateSendPacketPool(mMaxConnection, mMaxPacketSize);
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create send packet pool failed");
			break;
		}

		ret = CreateRecvPacketPool(mMaxConnection, mMaxPacketSize);
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create recv packet pool failed");
			break;
		}

		ret = CreateExceptionContextPool();
		if (ret != 0) {
			Log(LOG_LEVEL_ERROR, "create exception context pool failed");
			break;
		}
	} while (false);

	return ret;
}

void TcpService::ReleaseAllPool()
{
	Log(LOG_LEVEL_INFO, "begin release all pool service");
	// release tcp client pool
	if (true) {
		LOCK_GUARD(mTcpClientPoolLock);
		SafeDelete(mTcpClientPoolPtr);
	}
	// release send io context pool
	if (true) {
		LOCK_GUARD(mSendIOContextPoolLock);
		SafeDelete(mSendIOContextPoolPtr);
	}
	// release send packet pool
	if (true) {
		LOCK_GUARD(mSendPacketPoolLock);
		SafeDelete(mSendPacketPoolPtr);
	}
	if (true) {
		LOCK_GUARD(mRecvPacketPoolLock);
		SafeDelete(mRecvPacketPoolPtr);
	}
	if (true) {
		LOCK_GUARD(mExceptionContextPoolLock);
		SafeDelete(mExceptionContextPoolPtr);
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

int TcpService::CreateTcpClientPool(int maxConnection)
{
	mTcpClientPoolPtr = new(std::nothrow) DynamicMemoryPool(sizeof(TcpClient), maxConnection);
	if (NULL == mTcpClientPoolPtr) {
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
	LOCK_GUARD(mTcpClientPoolLock);
	if (NULL == mTcpClientPoolPtr) {
		return NULL;
	}

	int code = 0;
	do
	{
		TcpClient* pClient = mTcpClientPoolPtr->Create<TcpClient>();
		if (!pClient) {
			code = 1;
			break;
		}
		pClient->Reset();

		// allocate logic socket and normal socket
		pClient->mLogicSocketID = nextID;
		if (!pClient->CreateSocket()) {
			code = 2;
			mTcpClientPoolPtr->Free(pClient);
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

	LOCK_GUARD(mTcpClientPoolLock);
	mTcpClientMap.erase(pClient->mLogicSocketID);
	mTcpClientPoolPtr->Free(pClient);
}*/

TcpClientRef TcpService::FindClient(unsigned int socketID)
{
	LOCK_GUARD(mTcpClientPoolLock);
	TcpClientMap::iterator itor = mTcpClientMap.find(socketID);
	if (itor == mTcpClientMap.end()) {
		return NULL;
	}

	return itor->second;
}

void TcpService::RemoveClient(unsigned int socketID)
{
	LOCK_GUARD(mTcpClientPoolLock);
	TcpClientMap::iterator itor = mTcpClientMap.find(socketID);
	if (itor == mTcpClientMap.end())
		return;
	if (itor->second->IsActive()) {
		Log(LOG_LEVEL_ERROR, "find client is active");
		itor->second->MarkDestroy();
	}
	else {
		mTcpClientPoolPtr->Free(itor->second);
		mTcpClientMap.erase(itor);
	}
}

int TcpService::CreateSendIOContextPool(int sendQueueSize)
{
	mSendIOContextPoolPtr = new(std::nothrow) DynamicMemoryPool(sizeof(IOContext), sendQueueSize);
	if (NULL == mSendIOContextPoolPtr) {
		Log(LOG_LEVEL_ERROR, "create send io context pool failed");
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

IOContext* TcpService::AllocSendIOContext()
{
	LOCK_GUARD(mSendIOContextPoolLock);
	if (NULL == mSendIOContextPoolPtr)
	{
		return NULL;
	}

	return mSendIOContextPoolPtr->Create<IOContext>();
}

void TcpService::ReleaseSendIOContext(IOContext* context)
{
	if (NULL == context)
		return;

	LOCK_GUARD(mSendIOContextPoolLock);
	mSendIOContextPoolPtr->Free(context);
}

int TcpService::CreateSendPacketPool(int sendQueueSize, int maxSendPacketSize)
{
	mSendPacketPoolPtr = new(std::nothrow) DynamicMemoryPool(maxSendPacketSize, sendQueueSize);
	if (NULL == mSendPacketPoolPtr) {
		Log(LOG_LEVEL_ERROR, "create send packet pool failed");
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

void* TcpService::AllocSendPacket()
{
	LOCK_GUARD(mSendPacketPoolLock);
	if (NULL == mSendPacketPoolPtr)
	{
		return NULL;
	}

	void* packet = mSendPacketPoolPtr->Create();
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

	LOCK_GUARD(mSendPacketPoolLock);
	mSendPacketPoolPtr->Free(packet);
}

int TcpService::CreateRecvPacketPool(int recvQueueSize, int maxRecvPacketSize)
{
	mRecvPacketPoolPtr = new(std::nothrow) DynamicMemoryPool(maxRecvPacketSize, recvQueueSize);
	if (NULL == mRecvPacketPoolPtr) {
		Log(LOG_LEVEL_ERROR, "create recv packet pool failed");
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

void* TcpService::AllocRecvPacket()
{
	LOCK_GUARD(mRecvPacketPoolLock);
	if (NULL == mRecvPacketPoolPtr)
	{
		return NULL;
	}

	void* packet = mRecvPacketPoolPtr->Create();
	if (!packet)
		return NULL;

	memset(packet, 0, mMaxPacketSize);

	return packet;
}

void  TcpService::ReleaseRecvPacket(void* packet)
{
	if (NULL == packet)
		return;

	LOCK_GUARD(mRecvPacketPoolLock);
	mRecvPacketPoolPtr->Free(packet);
}

int TcpService::CreateExceptionContextPool()
{
	mExceptionContextPoolPtr = new(std::nothrow) DynamicMemoryPool(sizeof(IOContext), mMaxExceptionContext);
	if (NULL == mExceptionContextPoolPtr) {
		Log(LOG_LEVEL_ERROR, "create exception io context failed");
		return NSE_SYSTEM_ERROR;
	}

	return 0;
}

IOContext* TcpService::AllocExceptionIOContext()
{
	LOCK_GUARD(mExceptionContextPoolLock);
	if (NULL == mExceptionContextPoolPtr)
	{
		return NULL;
	}

	IOContext* context = mExceptionContextPoolPtr->Create<IOContext>();
	return context;
}

void  TcpService::ReleaseExceptionIOContext(IOContext* context)
{
	if (NULL == context)
		return;
	LOCK_GUARD(mExceptionContextPoolLock);
	mExceptionContextPoolPtr->Free(context);
}

int TcpService::Send(unsigned int socketID, const void* pData, int nDataLen)
{
	// use a memeroy block to Pool the data, release the memory after send completion
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

int TcpService::SendHttpResponse(HttpResponse& resp)
{
	/**/
	TcpClientRef clientRef = FindClient(resp.socketId);
	if (!clientRef) {
		return NSE_INVALID_SOCKET;
	}

	return clientRef->PostSendHttp(resp);
}

void TcpService::TickUpdate()
{
	if (mCheckStatusTimer.IsTimed()) {
		checkStatus();
	}
	if (mLogStatsTimer.IsTimed()) {
		logStats();
	}
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
		LOCK_GUARD(mTcpClientPoolLock);
		TcpClientMap::iterator itor = mTcpClientMap.begin();
		while (itor != mTcpClientMap.end()) {
			if (!itor->second->IsAccepting()) {
				hasClientNotReleased = true;
#if (_WIN32_WINNT >= 0x0600)
				if ((now > itor->second->mLastTickTime) && (now - itor->second->mLastTickTime) > (mClientTimeoutSec * 1000))
#else
				if ((now > itor->second->mLastTickTime) && GetMsInterval(now, itor->second->mLastTickTime) > ((unsigned int)mClientTimeoutSec * 1000))
#endif
				{
					itor->second->SetTimeout();
					mStats["Timeout"]++;
				}

				if (itor->second->IsMarkedDestroy() && !itor->second->IsActive()) {
					Log(LOG_LEVEL_INFO, "TcpService::checkStatus() find client marked to destroy:%u", itor->second->mLogicSocketID);
					mTcpClientPoolPtr->Free(itor->second);
					itor = mTcpClientMap.erase(itor);
					continue;
				}
			}
			
			++itor;
		}
	} while (false);
	
	/*
	if (!hasClientNotReleased && mMarkedStop) {
		ReleaseAllPool();
		mStopCheckStatusThread = true;
	}
	*/
}

int  TcpService::GetClientPoolUsed()
{
	LOCK_GUARD(mTcpClientPoolLock);
	return mTcpClientPoolPtr ? (int)mTcpClientPoolPtr->GetUsedCount() : 0;
}

int  TcpService::GetSendIOContextPoolUsed()
{
	LOCK_GUARD(mSendIOContextPoolLock);
	return mSendIOContextPoolPtr ? (int)mSendIOContextPoolPtr->GetUsedCount() : 0;
}

int  TcpService::GetSendPoolUsed()
{
	LOCK_GUARD(mSendPacketPoolLock);
	return mSendPacketPoolPtr ? (int)mSendPacketPoolPtr->GetUsedCount() : 0;
}

int  TcpService::GetRecvPoolUsed()
{
	LOCK_GUARD(mRecvPacketPoolLock);
	return mRecvPacketPoolPtr ? (int)mRecvPacketPoolPtr->GetUsedCount() : 0;
}

int  TcpService::GetExceptionPoolUsed()
{
	LOCK_GUARD(mExceptionContextPoolLock);
	return mExceptionContextPoolPtr ? (int)mExceptionContextPoolPtr->GetUsedCount() : 0;
}

void TcpService::logStats()
{
	mStats["ClientPoolUsed"] = GetClientPoolUsed();
	mStats["SendIOContextUsed"] = GetSendIOContextPoolUsed();
	mStats["SendPoolUsed"] = GetSendPoolUsed();
	mStats["RecvPoolUsed"] = GetRecvPoolUsed();
	mStats["ExceptionContextUsed"] = GetExceptionPoolUsed();
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

		CSemaphore mConnectSemaphore;
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
			// release client
			c.UnRef();
			// check can remove successfully
			RemoveClient(nSocketID);
			nSocketID = 0;
			break;
		}

		if (!c->mConnctRemoteSem.Wait(waitMS)) {
			c->PostSpecial(IOCP_OP_CONNECTEX_TIMEOUT);
			c.UnRef();
			nSocketID = 0;
			break;
		}
		else {
			if (c->mConnState != CONN_CONNECTED) {
				nSocketID = 0;
			}
		}
		
	} while (false);
	
	return nSocketID;
}

int TcpService::GetConnectionCount()
{
	int count = 0;
	LOCK_GUARD(mTcpClientPoolLock);
	TcpClientMap::iterator itor = mTcpClientMap.begin();
	while (itor != mTcpClientMap.end()) {
		if (!itor->second->IsAccepting()) {
			count++;
		}
		++itor;
	}
	return count;
}

int TcpService::GetProtocolType()
{
	return mProtocolType;
}

WT_END