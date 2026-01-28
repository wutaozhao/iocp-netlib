
#pragma once

#include "BaseInclude.h"

class CServiceConfig
{
public:
	CServiceConfig(){}

	~CServiceConfig(){}

	bool InitConfigInfo();

private:
	std::string ReadValueAsStr(CMarkup& xml, std::string strPath, std::string strDefault = "");

	unsigned int ReadValueAsInteger(CMarkup& xml, std::string strPath, unsigned int nDefault = 0);

	unsigned long long ReadValueAsBigInt(CMarkup& xml, std::string strPath, unsigned int nDefault = 0);

	void WriteXmlIntValue(std::string strOriPath, int nValue);

public:
	std::string      m_strServiceName;
	std::string      m_strServiceDisplayName;
	std::string      m_strServiceDesc;
	std::string      m_strLocalIP;
	unsigned short   m_usListenPort;
	unsigned int     m_nWorkThreadCount;
	unsigned int     m_nNetLogLevel;
	unsigned int     m_nLogLevel;

	unsigned int     m_nConnectionTimeoutSec;
	unsigned int     m_nMaxConnectionCount;
	unsigned int     m_nMaxSendQueuePacketCount; 
	unsigned int     m_nMaxRecvPacketSize;
	unsigned int     m_nMaxSendPacketSize;

	// db connection
	std::string      m_strDBAddr;
	std::string      m_strDBName;
	std::string      m_strDBLoginName;
	std::string      m_strDBLoginPwd;

private:
	std::string m_strXmlFilePath;
	CMarkup     m_xmlHandle;
};