
#pragma once

#include "Config.h"

WT_BEGIN

class CServiceInstance
{
public:
	CServiceInstance();

	~CServiceInstance();

	static CServiceInstance* GetInstance() { return ms_pInstance; }

	//
	virtual bool InitInstance() = 0;

	virtual int RunInstance() = 0;

	virtual void StopInstance() = 0;

	const char* GetServiceName() { return m_szServiceName; }

	const char* GetDisplayName() { return m_szDisplayName; }

	const char* GetDescription() { return m_szDescription; }

	void SetServiceName(const char* pszServiceName, const char* pszDisplayName, const char* pszDescription = "");

	// end with '/'
	const char* GetWorkPath();

	// workpath\log\2015_03_05_15_00_00
	const char* GetLogPath();

private:
	static CServiceInstance* ms_pInstance;
	char         m_szServiceName[256];
	char         m_szDisplayName[256];
	char         m_szDescription[256];
	char         m_szWorkPath[256];
	char         m_szLogPath[256];
};

WT_END