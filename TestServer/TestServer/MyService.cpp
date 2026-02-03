
#include "MyService.h"
#include "DBConnection.h"

CMyService  g_service;

CMyService::CMyService()
{
	mInitQueueSucc = false;
	mStartNetServiceSucc = false;
	mStartThreadSucc = false;
	m_ctTimer.SetTimer(15 * 1000);
	m_hStopEvent = NULL;
}

CMyService::~CMyService()
{

}

bool CMyService::InitInstance()
{
	// can read config here
	m_serviceConfig.InitConfigInfo();

	SetServiceName(m_serviceConfig.m_strServiceName.c_str(), 
		m_serviceConfig.m_strServiceDisplayName.c_str(),
		m_serviceConfig.m_strServiceDesc.c_str());

	return true;
}

int CMyService::RunInstance()
{
	// here can do some initialization
	MakeLog();

	m_hStopEvent = ::CreateEvent(NULL, true, false, NULL);
	if (NULL == m_hStopEvent)
	{
		StopInstance();
		return 0;
	}

	// the main loop
	bool bInitResult = Init();

	CTimer initTimer;
	initTimer.SetTimer(3000);
	while (::WaitForSingleObject(m_hStopEvent, 10) != WAIT_OBJECT_0)
	{
		while (!bInitResult) {
			if (initTimer.IsTimed()) {
				bInitResult = Init();
			}
			else {
				Sleep(10);
			}
			continue;
		}

		if (m_ctTimer.IsTimed())
		{
			
		}
	}

	return 0;
}

void CMyService::MakeLog()
{
	m_logMgr.Init(GetLogPath());
	m_logMgr.SetLogLevel("error.log", m_serviceConfig.m_nLogLevel);

	m_logMgr.Write("error.log", LOG_LEVEL_DEBUG, "%s", "Service has already start");
}

void CMyService::StopInstance()
{
	LOG("error", LOG_LEVEL_DEBUG, "%s", "Service will stop");
	SetEvent(m_hStopEvent);

	mHttpServer1.Stop();

	LOG("error", LOG_LEVEL_DEBUG, "%s", "http server stop");

	m_logMgr.UnInit();

	/*
	mServer1.StopServer();

	mServer2.StopServer();

	mServer3.StopServer();
	*/
}

bool CMyService::Init()
{
	do 
	{
		if (!mStartNetServiceSucc) {
			if (!StartNetService())
			{
				LOG("error", LOG_LEVEL_DEBUG, "Start net service failed");
				break;
			}
			else {
				mStartNetServiceSucc = true;
			}
		}

		printf("init success!\n");

		LOG("error", LOG_LEVEL_DEBUG, "Service init success");
		return true;
	} while (false);
	
	return false;
}

bool CMyService::StartNetService()
{
	if (mNetCore.Initialize(0) != 0) {
		return false;
	}

	mHttpServer1.StartHttpServer(&mNetCore);

	/*
	bool ret = mServer1.StartServer(&mNetCore, 8, 3200);
	if (!ret) {
		printf("start server 1 failed\n");
	}
	*/
	
	/*
	ret = mServer2.StartServer(&mNetCore, 8, 3600);
	if (!ret) {
		printf("start server 2 failed\n");
	}

	ret = mServer3.StartServer(&mNetCore, 8, 3900);
	if (!ret) {
		printf("start server 2 failed\n");
	}
	*/
	

	return true;
}

void CMyService::Log(const char* logName, int level, const char* format, ...)
{
	va_list argp;
	va_start(argp, format);
	m_logMgr.WriteV(logName, level, format, argp);
	va_end(argp);
}