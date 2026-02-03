
#include "BaseDefine.h"
#include "NetCoreIOCP.h"

WT_BEGIN

void LogS(NetCore* core, unsigned int logLeve, const char* format, ...) {
	if (!core)
		return;
	NetCoreIOCP* iocpCore = reinterpret_cast<NetCoreIOCP*>(core->GetCore());
	if (!iocpCore)
		return;

	va_list argp;
	va_start(argp, format);
	iocpCore->LogV("system.log", logLeve, format, argp);
	va_end(argp);
}

WT_END