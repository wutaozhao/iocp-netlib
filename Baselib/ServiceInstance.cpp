
#include "tool/ServiceInstance.h"
#include <WinSvc.h>

WT_BEGIN

CServiceInstance* CServiceInstance::ms_pInstance = NULL;

CServiceInstance::CServiceInstance()
{
	ms_pInstance = this;
	memset(m_szServiceName, 0, sizeof(m_szServiceName));
	memset(m_szDisplayName, 0, sizeof(m_szDisplayName));
	memset(m_szDescription, 0, sizeof(m_szDescription));

	memset(m_szWorkPath, 0, sizeof(m_szWorkPath));
	memset(m_szLogPath, 0, sizeof(m_szLogPath));
}

CServiceInstance::~CServiceInstance()
{

}

void CServiceInstance::SetServiceName(const char* pszServiceName, const char* pszDisplayName, const char* pszDescription)
{
	memcpy(m_szServiceName, pszServiceName, min(sizeof(m_szServiceName) - 1, strlen(pszServiceName)));
	memcpy(m_szDisplayName, pszDisplayName, min(sizeof(m_szDisplayName) - 1, strlen(pszDisplayName)));
	memcpy(m_szDescription, pszDescription, min(sizeof(m_szDescription) - 1, strlen(pszDescription)));
}

const char* CServiceInstance::GetWorkPath()
{
	m_szWorkPath[sizeof(m_szWorkPath) - 1] = '\0';
	if (strlen(m_szWorkPath) > 0)
		return m_szWorkPath;

	char    szDirBuffer[256] = {0};
#ifdef _WIN32
	if (GetModuleFileName(NULL, szDirBuffer, 255) == 0)
#else
	if (getcwd(szDirBuffer, sizeof(szDirBuffer) - 1) == NULL)
#endif
	{
		return NULL;
	}

	if (strlen(szDirBuffer) > 250)
		return NULL;

	//
#ifdef  _WIN32
	std::string strDir = szDirBuffer;
	size_t nPos = strDir.find_last_of('\\');

	strDir = strDir.substr(0, nPos);
	memset(szDirBuffer, 0, sizeof(szDirBuffer));
	memcpy(szDirBuffer, strDir.c_str(), min(sizeof(szDirBuffer), strDir.length()));
#endif
	strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), "/");
	memcpy(m_szWorkPath, szDirBuffer, min(sizeof(m_szWorkPath) - 1, sizeof(szDirBuffer)));

	return m_szWorkPath;
}

const char* CServiceInstance::GetLogPath()
{
	memset(m_szLogPath, 0, sizeof(m_szLogPath));

	// 获取当前路径，创建时间戳文件夹
	char    szDirBuffer[256] = {0};
	time_t  ulNow = {0};
	time(&ulNow);
	struct tm tNow;
	localtime_s(&tNow, &ulNow);

	char  szTimeBuffer[64] = {0};
	_snprintf_s(szTimeBuffer, 63, _TRUNCATE,
		"%04d_%02d_%02d_%02d_%02d_%02d",
		tNow.tm_year + 1900,
		tNow.tm_mon + 1,
		tNow.tm_mday,
		tNow.tm_hour,
		tNow.tm_min,
		tNow.tm_sec);

	// 设置日志路径
#ifdef _WIN32
	if (GetModuleFileName(NULL, szDirBuffer, 255) == 0)
#else
	if (getcwd(szDirBuffer, sizeof(szDirBuffer) - 1) == NULL)
#endif
	{
		return NULL;
	}

	if (strlen(szDirBuffer) + strlen(szTimeBuffer) > 240)
		return NULL;

	//
#ifdef  _WIN32
	std::string strDir = szDirBuffer;
	size_t nPos = strDir.find_last_of('\\');

	strDir = strDir.substr(0, nPos);
	memset(szDirBuffer, 0, sizeof(szDirBuffer));
	memcpy(szDirBuffer, strDir.c_str(), min(sizeof(szDirBuffer), strDir.length()));
#endif
	strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), "/log/");
	strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), szTimeBuffer);
	strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), "/");

	memcpy(m_szLogPath, szDirBuffer, min(sizeof(m_szLogPath) - 1, sizeof(szDirBuffer)));

	return m_szLogPath;
}

WT_END