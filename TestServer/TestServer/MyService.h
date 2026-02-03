
#pragma once

#include "BaseInclude.h"
#include "ServiceConfig.h"
#include "TestServer.h"
#include "TestHttpServer.h"

class CMyService: public CServiceInstance
{
public:
	typedef std::vector<Thread*> ThreadArray;
	CMyService();

	~CMyService();

	bool Init();

	// 
	bool InitInstance();
    
	int RunInstance();

	void StopInstance();

	void Log(const char* logName, int level, const char* format, ...);

private:
	bool StartNetService();

	void MakeLog();

public:
	CTimer            m_ctTimer;
	CServiceConfig    m_serviceConfig;

private:
	LogManager        m_logMgr;
	ThreadArray       m_threadArray;
	HANDLE            m_hStopEvent;

	NetCore           mNetCore;

	TestServer        mServer1;
	TestServer        mServer2;
	TestServer        mServer3;

	TestHttpServer    mHttpServer1;

private:
	bool              mInitQueueSucc;
	bool              mStartNetServiceSucc;
	bool              mStartThreadSucc;
};

extern CMyService  g_service;