#pragma once
#include "tool/LogManager.h"
#include "net/NetCore.h"

WT_BEGIN

struct HttpProtocolHeader {
	unsigned int socketID;
	unsigned short remotePort;
	unsigned int   remoteIP;
};

void LogS(NetCore* core, unsigned int logLeve, const char* format, ...);

WT_END