#pragma once

#include "Config.h"
#include "ThreadLock.h"
#include "Thread.h"
#include "PackageQueue.h"
#include "util.h"

WT_BEGIN

enum LogLevel
{
	LOG_LEVEL_FATAL = 1,
	LOG_LEVEL_ERROR = 2,
	LOG_LEVEL_INFO  = 3,
	LOG_LEVEL_DEBUG = 4,
};

struct LogFileObject
{
	LogFileObject()
	{
		pFileHandle = NULL;
		nLogLevel = 1;
		lastCheckTime = 0;
	}
	LogFileObject(const LogFileObject& rh)
	{
		pFileHandle = rh.pFileHandle;
		nLogLevel = rh.nLogLevel;
		lastCheckTime = rh.lastCheckTime;
	}

	FILE*               pFileHandle;
	unsigned int        nLogLevel;
	unsigned long long  lastCheckTime;
};

#pragma pack(1)
struct LogCell {
	char            name[32];
	unsigned int    logLevel;
	char            content[10240];
};
#pragma pack()

class LogManager
{
public:
	typedef std::map<std::string, LogFileObject> MapNameToFileObject;

	LogManager();
	~LogManager();

	//
	bool Init(const char* pszLogPath);

	void UnInit();

	//
	bool SetLogLevel(const char* pszLogName, unsigned int nLogLevel);

	// 
	bool Write(const char* pszLogName, unsigned int nLogLevel, const char* pszFormat, ...);

	bool WriteV(const char* pszLogName, unsigned int nLogLevel, const char* pszFormat, va_list args);

protected:
	void ThreadProc();

	void _write(LogCell* cell);

private:
	std::string GetLevelName(unsigned int nLogLevel);
private:
	std::string            m_strLogDir;
	MapNameToFileObject    m_mapNameToFileObject;
	CThreadLock            m_threadLock;
	wt::Thread*            m_ThreadForWrite;
	bool                   m_Stop;
	PackageQueue           m_logQueue;
};

WT_END
