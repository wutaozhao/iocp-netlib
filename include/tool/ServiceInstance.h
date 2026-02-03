
#pragma once

#include "Config.h"

WT_BEGIN

class CServiceInstance
{
public:
	CServiceInstance();

	~CServiceInstance();

	static CServiceInstance* GetInstance() { return ms_pInstance; }

	// 入口,做一些初始化，如设置服务名，注册服务
	virtual bool InitInstance() = 0;

	virtual int RunInstance() = 0;

	virtual void StopInstance() = 0;

	const char* GetServiceName() { return m_szServiceName; }

	const char* GetDisplayName() { return m_szDisplayName; }

	const char* GetDescription() { return m_szDescription; }

	void SetServiceName(const char* pszServiceName, const char* pszDisplayName, const char* pszDescription = "");

	// 获取当前工作目录，以'/'结尾
	const char* GetWorkPath();

	// 获取当前日志目录，工作目录\log\当前时间命名的目录，如工作目录\log\2015_03_05_15_00_00
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