
#include "ServiceConfig.h"
#include "MyService.h"

bool CServiceConfig::InitConfigInfo()
{
	std::string strPath;
	
	if (!GetModuleFilePath(strPath))
	{
		return false;
	}

	m_strXmlFilePath = strPath + "/config/config.xml";

	if (!m_xmlHandle.Load(m_strXmlFilePath))
	{
		printf("load xml failed!\n");
		return false;
	}

	m_strServiceName = ReadValueAsStr(m_xmlHandle, "/Server/ServiceName", "ZWT first lib test program");
	m_strServiceDisplayName = ReadValueAsStr(m_xmlHandle, "/Server/ServiceDisp", "ZWT first lib test program display name");
	m_strServiceDesc = ReadValueAsStr(m_xmlHandle, "/Server/ServiceDesc", "ZWT first lib test program description");
	m_strLocalIP = ReadValueAsStr(m_xmlHandle, "/Server/LocalIP", "10.1.8.159");
	m_usListenPort = (unsigned short)ReadValueAsInteger(m_xmlHandle, "/Server/ListenPort", 8750);
	m_nWorkThreadCount = ReadValueAsInteger(m_xmlHandle, "/Server/WorkThreadCount", 4);

	m_nNetLogLevel = ReadValueAsInteger(m_xmlHandle, "/Server/NetLogLevel", 3);
	m_nLogLevel = ReadValueAsInteger(m_xmlHandle, "/Server/LogLevel", 3);

	m_nConnectionTimeoutSec = ReadValueAsInteger(m_xmlHandle, "/Server/ConnectionTimeoutSec", 180);
	m_nMaxConnectionCount = ReadValueAsInteger(m_xmlHandle, "/Server/MaxConnectionCount", 3000);
	m_nMaxSendQueuePacketCount = ReadValueAsInteger(m_xmlHandle, "/Server/MaxPacketCount", 50); 
	m_nMaxRecvPacketSize = ReadValueAsInteger(m_xmlHandle, "/Server/MaxRecvPacketSize", 4096);
	m_nMaxSendPacketSize = ReadValueAsInteger(m_xmlHandle, "/Server/MaxSendPacketSize", 4096);

	m_strDBAddr = ReadValueAsStr(m_xmlHandle, "/DB/DBAddr");
	m_strDBName = ReadValueAsStr(m_xmlHandle, "/DB/DBName");
	m_strDBLoginName = ReadValueAsStr(m_xmlHandle, "/DB/DBLoginName");
	m_strDBLoginPwd = ReadValueAsStr(m_xmlHandle, "/DB/DBLoginPwd");

	return true;
}

std::string CServiceConfig::ReadValueAsStr(CMarkup& xml, std::string strPath, std::string strDefault)
{
	std::string strRet;
	if (strPath[0] != '/')
		return strRet;

	xml.ResetPos();

	size_t nIndex = strPath.find_first_of('/');
	strPath = strPath.substr(nIndex + 1, std::string::npos);
	nIndex = strPath.find_first_of('/');
	std::string strName = strPath.substr(0, nIndex);
	//
	while (xml.FindChildElem(strName))
	{
		if (strPath.find_first_of('/') != std::string::npos)
		{
			xml.IntoElem();
			nIndex = strPath.find_first_of('/');
			strPath = strPath.substr(nIndex + 1, std::string::npos);
			nIndex = strPath.find_first_of('/');
			strName = strPath.substr(0, nIndex);
		}
		else
		{
			strRet = xml.GetChildData();
		}
	}

	if(strRet.empty())
		strRet = strDefault;

	return strRet;
}

unsigned int CServiceConfig::ReadValueAsInteger(CMarkup& xml, std::string strPath, unsigned int nDefault)
{
	unsigned int nRet = 0;
	std::string strValue = ReadValueAsStr(xml, strPath, "");
	if (!strValue.empty())
	{
		nRet = atoi(strValue.c_str());
	}
	else
		nRet = nDefault;

	return nRet;
}

unsigned long long CServiceConfig::ReadValueAsBigInt(CMarkup& xml, std::string strPath, unsigned int nDefault)
{
	unsigned long long nRet = 0;
	std::string strValue = ReadValueAsStr(xml, strPath, "");
	if (!strValue.empty())
	{
#ifdef _WIN32
		nRet = _atoi64(strValue.c_str());
#else
		nRet = atoll(strValue.c_str());
#endif
	}
	else
		nRet = nDefault;

	return nRet;
}

void CServiceConfig::WriteXmlIntValue(std::string strOriPath, int nValue)
{
	CMarkup xml;
	bool bRet =xml.Load(m_strXmlFilePath);
	if (!bRet)
	{
		return;
	}

	std::string strPath;

	strPath = strOriPath;

	size_t nIndex = strPath.find_first_of('/');
	strPath = strPath.substr(nIndex + 1, std::string::npos);
	nIndex = strPath.find_first_of('/');
	std::string strName = strPath.substr(0, nIndex);
	//
	while (xml.FindChildElem(strName))
	{
		if (strPath.find_first_of('/') != std::string::npos)
		{
			xml.IntoElem();
			nIndex = strPath.find_first_of('/');
			strPath = strPath.substr(nIndex + 1, std::string::npos);
			nIndex = strPath.find_first_of('/');
			strName = strPath.substr(0, nIndex);
		}
		else
		{
			if (xml.SetChildData(nValue))
			{
				xml.Save(m_strXmlFilePath);
			}
		}
	}
}