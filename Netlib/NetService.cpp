
#include "net/NetService.h"
#include "Config.h"
#include "TcpService.h"
#include "BaseDefine.h"

WT_BEGIN

NetService::NetService()
{
	mService = new(std::nothrow) TcpService();
}

NetService::~NetService()
{
	StopNetService();
}

int NetService::StartNetService(
	const char* ip,
	unsigned short listenPort,
	int listenBacklog,
	int maxConnection,
	int maxPacketSize,
	int packetSizeOffset,
	int clientTimeoutSec,
	int logLevel,
	NetCore* netCore,
	IIOCallback* callback,
	int protocolType)
{
	int ret = 0;

	do
	{
		if (!netCore || !netCore->GetCore()) {
			ret = NSE_INVALID_PARAM;
			break;
		}

		mNetCore = netCore;

		if (!mService) {
			LogS(mNetCore, LOG_LEVEL_ERROR, "NetService::StartService no TcpService");
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		TcpService* service = reinterpret_cast<TcpService*>(mService);
		if (!service) {
			LogS(mNetCore, LOG_LEVEL_ERROR, "NetService::StartNetService convert service failed");
			ret = NSE_SYSTEM_ERROR;
			break;
		}
		ret = service->StartNetService(ip, listenPort, listenBacklog, maxConnection,
			maxPacketSize, packetSizeOffset, clientTimeoutSec, logLevel, netCore, callback, protocolType);
		if (ret != 0) {
			LogS(mNetCore, LOG_LEVEL_ERROR, "NetService::StartNetService start failed, ret:%d", ret);
			break;
		}

	} while (false);

	if (ret != 0) {
		StopNetService();
	}

	return ret;
}

int NetService::Send(unsigned int socketID, const void* pData, int nDataLen)
{
	if (!mService) {
		LogS(mNetCore, LOG_LEVEL_ERROR, "Send Data no service");
		return NSE_SYSTEM_ERROR;
	}
	
	TcpService* service = reinterpret_cast<TcpService*>(mService);
	if (!service) {
		LogS(mNetCore, LOG_LEVEL_ERROR, "Send Data failed convert service");
		return NSE_SYSTEM_ERROR;
	}
	
	return service->Send(socketID, pData, nDataLen);
}

int NetService::CloseSocket(unsigned int socketID)
{
	if (!mService) {
		LogS(mNetCore, LOG_LEVEL_ERROR, "CloseSocket no service");
		return NSE_SYSTEM_ERROR;
	}

	TcpService* service = reinterpret_cast<TcpService*>(mService);
	if (!service) {
		LogS(mNetCore, LOG_LEVEL_ERROR, "CloseSocket failed convert service");
		return NSE_SYSTEM_ERROR;
	}

	return service->CloseSocket(socketID);
}

void NetService::StopNetService()
{
	//then we can release memory pool of tcp service safely
	if (mService) {
		TcpService* service = reinterpret_cast<TcpService*>(mService);
		if (!service) {
			LogS(mNetCore, LOG_LEVEL_ERROR, "StopNetService failed convert service");
			return;
		}
		service->StopNetService();
		delete service;
		mService = NULL;
	}
}

unsigned int NetService::ConnectServer(const char* ip, unsigned short port, int waitMS)
{
	if (!mService) {
		LogS(mNetCore, LOG_LEVEL_ERROR, "ConnectServer no service");
		return NSE_SYSTEM_ERROR;
	}

	TcpService* service = reinterpret_cast<TcpService*>(mService);
	if (!service) {
		LogS(mNetCore, LOG_LEVEL_ERROR, "ConnectServer failed convert service");
		return NSE_SYSTEM_ERROR;
	}
	return service->ConnectServer(ip, port, waitMS);
}

int NetService::GetConnectionCount()
{
	if (!mService) {
		LogS(mNetCore, LOG_LEVEL_ERROR, "GetConnectionCount no service");
		return NSE_SYSTEM_ERROR;
	}

	TcpService* service = reinterpret_cast<TcpService*>(mService);
	if (!service) {
		LogS(mNetCore, LOG_LEVEL_ERROR, "GetConnectionCount failed convert service");
		return NSE_SYSTEM_ERROR;
	}
	return service->GetConnectionCount();
}

WT_END