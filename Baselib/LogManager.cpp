
#include "tool/LogManager.h"

WT_BEGIN

#ifdef _WIN32
#include <Windows.h>
#include <time.h>
#else
#include <unistd.h>
#include <sys/syscall.h>
#include <stdarg.h>
#define zwtgettid()   syscall(__NR_gettid)
#endif

#include <string.h>

static bool DirectoryExists(const char* path)
{
	return (_access(path, 0) == 0);
}

static bool CreateDirectoryRecursive(const char* path)
{
	if (!path || !*path)
		return false;

	std::string fullPath(path);

	for (size_t i = 0; i < fullPath.size(); ++i)
	{
		if (fullPath[i] == '\\')
			fullPath[i] = '/';
	}

	std::string current;
	size_t pos = 0;

	if (fullPath.size() >= 2 && fullPath[1] == ':')
	{
		current = fullPath.substr(0, 2);
		pos = 2;
	}

	while (pos < fullPath.size())
	{
		if (fullPath[pos] == '/')
		{
			current += '/';
			if (!DirectoryExists(current.c_str()))
			{
				if (_mkdir(current.c_str()) != 0 && errno != EEXIST)
					return false;
			}
		}
		else
		{
			current += fullPath[pos];
		}
		++pos;
	}

	if (!DirectoryExists(current.c_str()))
	{
		if (_mkdir(current.c_str()) != 0 && errno != EEXIST)
			return false;
	}

	return true;
}

LogManager::LogManager()
{
	m_ThreadForWrite = NULL;
	m_Stop = false;
}

LogManager::~LogManager()
{
	UnInit();
}

bool LogManager::Init(const char* pszLogPath, int queueSize)
{
	m_strLogDir = pszLogPath;

#ifdef _WIN32
	if (m_strLogDir[m_strLogDir.length() - 1] != '\\' && m_strLogDir[m_strLogDir.length() - 1] != '/')
	{
		m_strLogDir += "\\";
	}
#else
	if (m_strLogDir[m_strLogDir.length() - 1] != '/')
	{
		m_strLogDir += "/";
	}
#endif

	bool ret = false;
	do
	{
		ret = CreateDirectoryRecursive(m_strLogDir.c_str());
		if (!ret) {
			printf("create log directory failed\n");
			break;
		}
			
		//
		if (m_logQueue.Init(sizeof(LogCell), queueSize) != 0) {
			printf("int log queue failed\n");
			break;
		}

		//
		m_ThreadForWrite = new(std::nothrow) wt::Thread(this, &LogManager::ThreadProc);

		if (!m_ThreadForWrite)
			break;

		if (!m_ThreadForWrite->Start()) {
			SafeDelete(m_ThreadForWrite);
			break;
		}

		ret = true;
	} while (false);

	if (!ret) {
		UnInit();
	}

	return ret;
}

void LogManager::UnInit()
{
	if (m_Stop) {
		return;
	}
	m_Stop = true;

	if (m_ThreadForWrite != NULL) {
		m_ThreadForWrite->Join(100);
		SafeDelete(m_ThreadForWrite);
	}

	do
	{
		LOCK_GUARD(m_threadLock);
		MapNameToFileObject::iterator itor = m_mapNameToFileObject.begin();
		for (; itor != m_mapNameToFileObject.end(); itor++)
		{
			fclose(itor->second.pFileHandle);
		}
		m_mapNameToFileObject.clear();
	} while (false);
	
}

// 设置日志等级
bool LogManager::SetLogLevel(const char* pszLogName, unsigned int nLogLevel)
{
	std::string strLogFullDir = m_strLogDir + pszLogName;
	FILE* pFile = _fsopen(strLogFullDir.c_str(), "w+", _SH_DENYNO);
	if (pFile != NULL)
	{
		LogFileObject logFileObject;
		logFileObject.pFileHandle = pFile;
		logFileObject.nLogLevel = nLogLevel;
#if (_WIN32_WINNT >= 0x0600)
		logFileObject.lastCheckTime = GetTickCount64();
#else
		logFileObject.lastCheckTime = GetTickCount();
#endif

		//
		LOCK_GUARD(m_threadLock);
		m_mapNameToFileObject.insert(MapNameToFileObject::value_type(pszLogName, logFileObject));

		return true;
	}

	return false;
}

std::string LogManager::GetLevelName(unsigned int nLogLevel)
{
	std::string strLevelName;
	if (nLogLevel == LOG_LEVEL_FATAL)
	{
		strLevelName = "FATAL";
	}
	else if (nLogLevel == LOG_LEVEL_ERROR)
	{
		strLevelName = "ERROR";
	}
	else if (nLogLevel == LOG_LEVEL_INFO)
	{
		strLevelName = " INFO";
	}
	else if (nLogLevel == LOG_LEVEL_DEBUG)
	{
		strLevelName = "DEBUG";
	}

	return strLevelName;
}

void LogManager::ThreadProc()
{
	while (!m_Stop){
		PacketRead rd;
		if (m_logQueue.Pop(rd, 50) != 0)
			continue;

		if (rd.GetDataLength() < sizeof(LogCell))
			continue;

		LogCell* cell = (LogCell*)rd.GetBuffer();
		_write(cell);
	}
}

bool LogManager::Write(const char* pszLogName, unsigned int nLogLevel, const char* pszFormat, ...)
{
	va_list argp;
	va_start(argp, pszFormat);
	bool ret = WriteV(pszLogName, LOG_LEVEL_INFO, pszFormat, argp);
	va_end(argp);

	return ret;
}

bool LogManager::WriteV(const char* pszLogName, unsigned int nLogLevel, const char* pszFormat, va_list args)
{
	LogCell logCell;
	memset(&logCell, 0, sizeof(logCell));

#ifdef _WIN32
	logCell.threadID = GetCurrentThreadId(),
#else
	logCell.threadID = zwtgettid(),      // thread id
#endif
	logCell.tickTime = GetCPUTickTime();
	

	logCell.logLevel = nLogLevel;
	memcpy(logCell.name, pszLogName, min(sizeof(logCell.name) - 1, strlen(pszLogName)));
	vsprintf_s(logCell.content, sizeof(logCell.content) - 1, pszFormat, args);

	PacketWrite w;
	if (!m_logQueue.AllocWritePacket(w)) {
		return false;
	}
	w.Write(&logCell, sizeof(logCell));

	return m_logQueue.Push(w);
}

void LogManager::_write(LogCell* cell)
{
	LOCK_GUARD(m_threadLock);
	bool       bRet = true;
	do
	{
		MapNameToFileObject::iterator itor = m_mapNameToFileObject.find(cell->name);
		if (itor == m_mapNameToFileObject.end())
		{
			bRet = false;
			break;
		}
		else
		{
			if (cell->logLevel > itor->second.nLogLevel)
			{
				break;
			}

			time_t ulNow = { 0 };
			time(&ulNow);
			struct tm tNow;
			localtime_s(&tNow, &ulNow);

			// each 10 seconds to check rename file
#if (_WIN32_WINNT >= 0x0600)
			unsigned long long now = GetTickCount64();
			if (now > itor->second.lastCheckTime && (now - itor->second.lastCheckTime) > (10 * 1000))
#else
			unsigned long now = GetTickCount();
			if (GetMsInterval(now, itor->second.lastCheckTime) > 10 * 1000)
#endif
			{
				int nSize = ftell(itor->second.pFileHandle);
				if (nSize > 10 * 1024 * 1024) {
					fclose(itor->second.pFileHandle);

					itor->second.lastCheckTime = now;

					char tmpTimeBuffer[256] = { 0 };
					_snprintf_s(tmpTimeBuffer, 255, _TRUNCATE, "%04d_%02d_%02d_%02d_%02d_%02d", tNow.tm_year + 1900,
						tNow.tm_mon + 1,
						tNow.tm_mday,
						tNow.tm_hour,
						tNow.tm_min,
						tNow.tm_sec);

					char oldFileName[MAX_PATH] = { 0 };
					_snprintf_s(oldFileName, MAX_PATH - 1, _TRUNCATE, "%s%s", m_strLogDir.c_str(), cell->name);

					char newPath[MAX_PATH] = { 0 };
					_snprintf_s(newPath, MAX_PATH - 1, _TRUNCATE, "%s%s_%s", m_strLogDir.c_str(), tmpTimeBuffer, cell->name);

					if (rename(oldFileName, newPath) == 0) {
						itor->second.pFileHandle = _fsopen(oldFileName, "wb", _SH_DENYNO);
					}
				}
			}

			if (!itor->second.pFileHandle) {
				break;
			}

			char szBuffer[10240] = { 0 };
			// [DateTime] [ProcessID] [ThreadID][LEVELINFO]: CONTENT
			_snprintf_s(szBuffer,
				sizeof(szBuffer) - 1, _TRUNCATE,
				"[%04d-%02d-%02d %02d:%02d:%02d][%d][%llu_%llu][%d:%s] %s\r\n",
				tNow.tm_year + 1900,
				tNow.tm_mon + 1,
				tNow.tm_mday,
				tNow.tm_hour,
				tNow.tm_min,
				tNow.tm_sec,
				cell->threadID,
				cell->tickTime,
				GetCPUTickTime(),
				cell->logLevel,       // log level
				GetLevelName(cell->logLevel).c_str(),
				cell->content);

			szBuffer[10240 - 1] = '\0';

			fwrite(szBuffer, 1, strlen(szBuffer), itor->second.pFileHandle);
			fflush(itor->second.pFileHandle);
		}
	} while (false);
}

WT_END