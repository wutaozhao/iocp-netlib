#pragma once

#include "tool/LogManager.h"
#include "tool/PackageQueue.h"
#include "tool/Thread.h"
#include "tool/util.h"
#include "tool/Buffer.h"
#include "net/NetService.h"
#include "net/SuperSocket.h"
#include "tool/Singleton.h"
#include "db/AdoDatabase.h"

#include "TestProtocol.h"


struct PacketHeader {
	unsigned int nSocketID;
	unsigned int nRemoteIP;
	unsigned short nRemotePort;
	unsigned long long createTime;
};

#define DECLARE_NETIOCP_INTERFACE() \
	protected: \
	void OnConnected(unsigned int nSocket, unsigned int nIP, unsigned short usPort); \
	int  OnReceived(unsigned int nSocket, unsigned int nIP, unsigned short usPort, void* pData, int nDataLen); \
	void OnClosed(unsigned int nSocket, unsigned int nIP, unsigned short usPort, unsigned int nErrorCode); \
	public:


#define LOG(name, level, format, ...)    g_service.Log(name".log", level, format, __VA_ARGS__)

using namespace wt;