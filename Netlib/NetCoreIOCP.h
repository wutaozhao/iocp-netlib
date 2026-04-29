#pragma once

#include "Config.h"
#include "tool/Singleton.h"
#include "tool/Thread.h"
#include "tool/ThreadLock.h"
#include "tool/LogManager.h"

WT_BEGIN

class TcpService;
class NetCoreIOCP
{
public:
	typedef std::map<unsigned int, TcpService*> ServiceMap;
	NetCoreIOCP();
	~NetCoreIOCP();

	int Initialize(int threadCount);

	void UnInitialize();

	HANDLE GetIOCPHandle();

	int AttachSocketToIOCP(SOCKET sock, ULONG_PTR completionKey);

	unsigned int GetNextLogicSocketID();

	unsigned int GetNextServiceID();

	bool CreateLog(const char* pszLogName, unsigned int nLogLevel);

	void Log(const char* pszLogName, unsigned int nLogLevel, const char* pszFormat, ...);

	void LogV(const char* pszLogName, unsigned int nLogLevel, const char* pszFormat, va_list args);

	void AddService(int serviceID, TcpService* service);
	void RemoveService(int serviceID);

protected:
	void IOCPWorkProc(int threadIndex);

	void MakeLog();

	int InitAssistThread();
	void AssistThreadProc();

private:
	HANDLE                   mIOCPHandle;

	int                      mThreadCount;

	bool                     mInitOnce;

	std::vector<wt::Thread*> mThreads;

	HANDLE*                  mThreadExitHandles;

	volatile long            mNextLogicSocketID;

	volatile long            mNextServiceID;
	CThreadLock				 mServiceMapLock;
	ServiceMap      		 mServiceMap;
	wt::Thread*              mAssistThread;
	bool                     mStopAssistThread;

public:
	LogManager*              mLogger;
};

WT_END