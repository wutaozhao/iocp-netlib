#pragma once

#include "Config.h"
#include "NetCore.h"

WT_BEGIN

class IIOCallback
{
public:
	virtual void OnConnected(unsigned int nSocket, unsigned int nIP, unsigned short usPort) = 0;

	virtual int OnReceived(unsigned int nSocket, unsigned int nIP, unsigned short usPort, void* pData, int nDataLen) = 0;

	virtual void OnClosed(unsigned int nSocket, unsigned int nIP, unsigned short usPort, unsigned int nErrorCode) = 0;
};

class NetService {
public:
	NetService();

	~NetService();

	/**
	* @brief start net service
	*
	* @param ip                     listen ip
	* @param listenPort             listen port
	* @param maxConnection          max connection
	* @param maxSendPacketSize      max send packet size
	* @param sendQueueSize          send queue size
	* @param maxRecvPacketSize      max recv packet size
	* @param recvQueueSize          recv queue size
	* @param packetSizeOffset       packet size of offset
	* @param clientTimeoutSec       client time out second
	* @param logLevel               log level
	* @param callback               net core
	* @param callback               callbacker
	*/
	int StartNetService(
		const char* ip,
		unsigned short listenPort,
		int listenBacklog,
		int maxConnection,
		int maxSendPacketSize,
		int sendQueueSize,
		int maxRecvPacketSize,
		int recvQueueSize,
		int packetSizeOffset,
		int clientTimeoutSec,
		int logLevel,
		NetCore* netCore,
		IIOCallback* callback
		);

	int Send(unsigned int socketID, const void* pData, int nDataLen);

	unsigned int ConnectServer(const char* ip, unsigned short port, int waitMS = 1000);

	int CloseSocket(unsigned int socketID);

	void StopNetService();

	int GetConnectionCount();

private:
	NetCore*    mNetCore;
	void*       mService;
};

WT_END