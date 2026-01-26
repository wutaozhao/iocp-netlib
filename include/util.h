
#pragma once

#include "Config.h"

WT_BEGIN

template <typename T>
inline void SafeDelete(T*& p)
{
	if (p)
	{
		delete p;
		p = NULL;
	}
}

inline std::string ToStringIP(unsigned int ip)
{
	std::ostringstream oss;
	unsigned char* p = (unsigned char*)&ip;
	oss<<(unsigned int)p[0]<<"."<<(unsigned int)p[1]<<"."<<(unsigned int)p[2]<<"."<<(unsigned int)p[3];
	return oss.str();
}

inline std::string StringTrimLeft(const std::string& str)
{
	size_t nPos = str.find_first_not_of(' ');
	return str.substr(nPos);
}

inline bool SplitStr(std::string str, char splitCh, std::vector<std::string>& result) {
	size_t npos = str.find_first_of(splitCh);
	while (npos != std::string::npos) {
		std::string substr = str.substr(0, npos);
		if (substr.length() > 0) {
			result.push_back(substr);
		}

		str = str.substr(npos + 1);
		npos = str.find_first_of(splitCh);
	}

	if (str.length() > 0) {
		result.push_back(str.substr(0, npos));
	}

	return result.size() > 0;
}

// 没有以\结尾
inline bool GetModuleFilePath(std::string& strPath)
{
	char szFileName[257] = {0};

#ifdef _WIN32
	if (GetModuleFileName(NULL, szFileName, MAX_PATH) == 0)
		return false;

	std::string strFullPath = szFileName;
	size_t nPos = strFullPath.find_last_of('\\');
	if (nPos == std::string::npos)
	{
		nPos = strFullPath.find_last_of('/');
		if (nPos == std::string::npos)
			return false;
	}

	strPath.clear();
	strPath = strFullPath.substr(0, nPos);
#else
	if (getcwd(szFileName, sizeof(szFileName) - 1) == NULL)
	{
		return false;
	}

	strPath = szFileName;
#endif

	return true;
}

inline int GetCPUCount()
{
#ifdef _WIN32
	SYSTEM_INFO info = {0};
	GetSystemInfo(&info);

	return (int)info.dwNumberOfProcessors;
#else
	return 4;
#endif
}

inline std::string GetCurrentTimeStr()
{
	// 发送报警信息 
	std::string        strTimeFormat = "%Y_%m_%d_%H_%M_%S";
	time_t        ltime;
	struct tm     gmt;
	char          timebuf[256]={0};

	memset(timebuf, 0, sizeof( timebuf ) );
	time( &ltime );
	localtime_s(&gmt, &ltime );
	strftime(timebuf, sizeof( timebuf ), strTimeFormat.c_str(), &gmt);

	return timebuf;
}

#ifdef _WIN32
inline DWORD GetMsInterval(DWORD dwNow, DWORD dwLast) 
{
	if( dwNow < dwLast )
		return 0xFFFFFFFF - dwLast + dwNow;
	else
		return dwNow - dwLast;
}

inline DWORD ConvertUTF8ToUnicode(char* pszUTF8, int nUTF8Bytes, char* pszUnicodeBuffer, int nBufferLen)
{
	return MultiByteToWideChar(CP_UTF8, 0, pszUTF8, nUTF8Bytes, (wchar_t*)pszUnicodeBuffer, nBufferLen / sizeof(wchar_t));
}

#else
// 获取当前毫秒值
inline unsigned long long GetCurrMsTime()
{
	struct timeval nowTime;
	gettimeofday(&nowTime, NULL);
	unsigned long long nMillSecond = nowTime.tv_sec * 1000 + nowTime.tv_usec / 1000;

	return nMillSecond;
}

inline void Sleep(int a)
{
	usleep(a * 1000);
}
#endif

class CTimer
{
public:
	CTimer()
	{
		m_nMillTimeout = 0;
		m_nLastMillTime = 0;
	}
	~CTimer()
	{

	}

	bool IsTimed()
	{
#ifdef _WIN32
		unsigned long long now = GetTickCount64();
		if ((now - m_nLastMillTime) >= m_nMillTimeout)
		{
			m_nLastMillTime = now;
			return true;
		}
		return false;
#else
		unsigned long long nMillSecond = GetCurrMsTime();
		if (nMillSecond - m_nLastMillTime >= m_nMillTimeout)
		{
			m_nLastMillTime = nMillSecond;
			return true;
		}
		return false;
#endif
	}

	void SetTimer(unsigned int nMillTimeout)
	{
#ifdef _WIN32
		m_nLastMillTime = GetTickCount64();
#else
		m_nLastMillTime = GetCurrMsTime();
#endif
		m_nMillTimeout = nMillTimeout;
	}

private:
	unsigned long long m_nMillTimeout;

#ifdef _WIN32
	unsigned long long m_nLastMillTime;
#else
	unsigned long long m_nLastMillTime;
#endif

};

WT_END
