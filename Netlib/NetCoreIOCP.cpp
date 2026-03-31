
#include "NetCoreIOCP.h"
#include "net/NetService.h"
#include "SocketHelper.h"
#include "IOContext.h"
#include "TcpService.h"
#include "TcpAcceptor.h"
#include "TcpClient.h"

const int IOCP_SERVICE_VERSION = 10001;

WT_BEGIN

NetCoreIOCP::NetCoreIOCP()
{
	mIOCPHandle = NULL;
	mNextLogicSocketID = 0;
	mNextServiceID = 0;

	mThreadCount = 0;
	mThreadExitHandles = NULL;
	mInitOnce = false;
}

NetCoreIOCP::~NetCoreIOCP()
{

}

int NetCoreIOCP::Initialize(int threadCount)
{
	int ret = 0;

	do
	{
		if (mInitOnce) {
			break;
		}

		MakeLog();

		// init Socket
		SocketHelper::InitSocket();

		mIOCPHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (NULL == mIOCPHandle) {
			Log("system.log", LOG_LEVEL_DEBUG, "create iocp failed, error:%d", GetLastError());
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		if (threadCount == 0) {
			SYSTEM_INFO sysinfo;
			::GetSystemInfo(&sysinfo);
			threadCount = sysinfo.dwNumberOfProcessors;
		}

		mThreadCount = threadCount;
		mThreadExitHandles = new HANDLE[threadCount];
		for (int i = 0; i < threadCount; i++)
		{
			mThreadExitHandles[i] = NULL;
		}
		
		for (int i = 0; i < threadCount; i++) {
			HANDLE hEvent = CreateEvent(NULL, true, false, NULL);
			mThreadExitHandles[i] = hEvent;
			wt::Thread* pThread = new(std::nothrow) wt::Thread(this, &NetCoreIOCP::IOCPWorkProc, i);
			if (pThread)
			{
				if (hEvent && pThread->Start()) {
					mThreads.push_back(pThread);
				}
				else {
					delete pThread;
					break;
				}
			}
		}
		if (mThreads.size() != threadCount) {
			Log("system.log", LOG_LEVEL_DEBUG, "create iocp thread failed:%d -- %d", mThreads.size(), threadCount);
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		Log("system.log", LOG_LEVEL_DEBUG, "create iocp thread succ, need:%d, actual:%d, version:%d", threadCount, mThreads.size(), IOCP_SERVICE_VERSION);

		mInitOnce = true;
	} while (false);

	if (ret != 0) {
		UnInitialize();
	}

	return ret;
}

void NetCoreIOCP::UnInitialize()
{
	//
	if (NULL != mIOCPHandle) {
		int count = (int)mThreads.size();
		for (int i = 0; i < count; i++) {
			// create event
			PostQueuedCompletionStatus(mIOCPHandle, 0, NULL, NULL);
		}

		// To do: wait for multiple events
		for (int i = 0; i < mThreadCount; i++)
		{
			if (mThreadExitHandles[i] != NULL) {
				Log("system.log", LOG_LEVEL_DEBUG, "begin wait thread:%d exit", i);
				WaitForSingleObject(mThreadExitHandles[i], INFINITE);
				CloseHandle(mThreadExitHandles[i]); 
				mThreadExitHandles[i] = NULL;
				Log("system.log", LOG_LEVEL_DEBUG, "wait thread:%d exit finished", i);
			}
		}

		CloseHandle(mIOCPHandle);
		mIOCPHandle = NULL;
	}

	for (size_t i = 0; i < mThreads.size(); i++) {
		mThreads[i]->Join(100);
		delete mThreads[i];
	}
	mThreads.clear();

	mInitOnce = false;

	Log("system.log", LOG_LEVEL_DEBUG, "UnInitialize() exit.");
}

unsigned int NetCoreIOCP::GetNextServiceID()
{ 
	return ::InterlockedIncrement(&mNextServiceID);
}

HANDLE NetCoreIOCP::GetIOCPHandle()
{
	return mIOCPHandle;
}

int NetCoreIOCP::AttachSocketToIOCP(SOCKET sock, ULONG_PTR completionKey)
{
	HANDLE h = ::CreateIoCompletionPort((HANDLE)sock, mIOCPHandle, completionKey, 0);
	if (NULL == h)
	{
		Log("system.log", LOG_LEVEL_DEBUG, "AttachSocketToIOCP last error:%d -- %d", GetLastError(), WSAGetLastError());
		return -1;
	}
	return 0;
}

void NetCoreIOCP::IOCPWorkProc(int threadIndex)
{
	Log("system.log", LOG_LEVEL_DEBUG, "IOCP worker start, threadIndex:%d", threadIndex);

	BOOL                bResult = FALSE;
	OVERLAPPED*         lpOverLapped = NULL;
	IIOCPCompletionCB*  pCompletionKey = NULL;
	DWORD               dwNumberOfBytes = 0;
	while (true) {
		try {
			// initialize
			dwNumberOfBytes = 0;
			pCompletionKey = NULL;
			lpOverLapped = NULL;

			bResult = ::GetQueuedCompletionStatus(mIOCPHandle, &dwNumberOfBytes, (PULONG_PTR)&pCompletionKey, &lpOverLapped, INFINITE);

			if (!bResult) {
				if (lpOverLapped && pCompletionKey) {
					pCompletionKey->OnCompleteOperation(false, reinterpret_cast<IOContext*>(lpOverLapped), dwNumberOfBytes, ::WSAGetLastError());
				}
			}
			else {
				if (lpOverLapped && pCompletionKey) {
					IOContext* ct = reinterpret_cast<IOContext*>(lpOverLapped);
					pCompletionKey->OnCompleteOperation(true, reinterpret_cast<IOContext*>(lpOverLapped), dwNumberOfBytes, 0);
				}
				else
				{
					HANDLE hEvent = mThreadExitHandles[threadIndex];
					if (hEvent) {
						Log("system.log", LOG_LEVEL_DEBUG, "set IOCP worker,index:%d", threadIndex);
						SetEvent(hEvent);
					}
					else {
						Log("system.log", LOG_LEVEL_DEBUG, "IOCP worker,index:%d no event", threadIndex);
					}
					Log("system.log", LOG_LEVEL_DEBUG, "exit IOCP worker:%d", threadIndex);
					break;
				}
			}
			
		}
		catch (...)
		{
			Log("system.log", LOG_LEVEL_DEBUG, "IOCP worker has exception");
		}
	}
}

unsigned int NetCoreIOCP::GetNextLogicSocketID()
{
	unsigned int ret = (unsigned int)InterlockedIncrement(&mNextLogicSocketID);
	if (ret == 0) {
		ret = (unsigned int)InterlockedIncrement(&mNextLogicSocketID);
	}

	return ret;
}

void NetCoreIOCP::MakeLog()
{
	int ret = 0;
	do
	{
		//
		mLogger = new(std::nothrow) LogManager();
		if (!mLogger) {
			printf("new log manager failed\n");
			break;
		}

		// 获取当前路径，创建时间戳文件夹
		std::string strLogPath;
		char szDirBuffer[512] = { 0 };

		time_t ulNow = { 0 };
		time(&ulNow);
		struct tm tNow;
		localtime_s(&tNow, &ulNow);

		char  szTimeBuffer[256] = { 0 };
		_snprintf_s(szTimeBuffer,
			255,
			_TRUNCATE,
			"%04d_%02d_%02d_%02d_%02d_%02d",
			tNow.tm_year + 1900,
			tNow.tm_mon + 1,
			tNow.tm_mday,
			tNow.tm_hour,
			tNow.tm_min,
			tNow.tm_sec);

		// 设置日志路径
#ifdef _WIN32
		if (GetModuleFileName(NULL, szDirBuffer, 511) == 0)
#else
		if (getcwd(szDirBuffer, sizeof(szDirBuffer) - 1) == NULL)
#endif
		{
			ret = -1;
			break;
		}

		//
#ifdef  _WIN32
		std::string strDir = szDirBuffer;
		size_t nPos1 = strDir.find_last_of('\\');

		strDir = strDir.substr(0, nPos1);
		memset(szDirBuffer, 0, sizeof(szDirBuffer));
		memcpy(szDirBuffer, strDir.c_str(), min(sizeof(szDirBuffer), strDir.length()));
#endif
		strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), "/");
		strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), "netliblog/");
		strLogPath = szDirBuffer;

		strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), szTimeBuffer);
		strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), "/");

		mLogger->Init(szDirBuffer);
		mLogger->SetLogLevel("system.log", LOG_LEVEL_DEBUG);

	} while (false);

	if (ret != 0) {
		UnInitialize();
	}
}

bool NetCoreIOCP::CreateLog(const char* pszLogName, unsigned int nLogLevel)
{
	if (!mLogger)
		return false;

	return mLogger->SetLogLevel(pszLogName, nLogLevel);
}

void NetCoreIOCP::Log(const char* pszLogName, unsigned int nLogLevel, const char* pszFormat, ...)
{
	va_list argp;
	va_start(argp, pszFormat);
	mLogger->WriteV(pszLogName, nLogLevel, pszFormat, argp);
	va_end(argp);
}

void NetCoreIOCP::LogV(const char* pszLogName, unsigned int nLogLevel, const char* pszFormat, va_list args)
{
	if (!mLogger)
		return;

	mLogger->WriteV(pszLogName, nLogLevel, pszFormat, args);
}

WT_END