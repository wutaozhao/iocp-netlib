#pragma once

#include "Config.h"
#include "Singleton.h"
#include "Thread.h"
#include "ThreadLock.h"
#include "LogManager.h"

WT_BEGIN

class NetCoreIOCP
{
public:
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

protected:
	void IOCPWorkProc(int threadIndex);

	void MakeLog();

private:
	HANDLE                   mIOCPHandle;

	int                      mThreadCount;

	bool                     mInitOnce;

	std::vector<wt::Thread*> mThreads;

	HANDLE*                  mThreadExitHandles;

	volatile long            mNextLogicSocketID;

	volatile long            mNextServiceID;

public:
	LogManager*              mLogger;
};

WT_END