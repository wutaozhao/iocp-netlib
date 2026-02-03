#pragma once

#include "sql/AdoDatabase.h"
#include "tool/LogManager.h"
#include "tool/PackageQueue.h"
#include "tool/ServiceInstance.h"
#include "tool/ThreadLock.h"
#include "tool/Thread.h"
#include "tool/util.h"
#include "tool/Singleton.h"
#include "tool/Buffer.h"
#include "net/NetService.h"
#include "net/HttpService.h"
#include "tool/Markup.h"
#include "tool/FixedObjectPool.h"
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