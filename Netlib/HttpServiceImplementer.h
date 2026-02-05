#pragma once

#include "net/NetService.h"
#include "net/HttpService.h"
#include "TcpService.h"
#include "tool/PackageQueue.h"
#include "BaseDefine.h"

WT_BEGIN

class HttpServiceImplementer :public IIOCallback, public TcpService, public IHttpServiceImpl
{
public:
	HttpServiceImplementer();
	~HttpServiceImplementer();

	void SetRoute(const std::string& method, const std::string& path, IHttpRouteHandler* handler);

	int Start(
		const char* ip,
		unsigned short listenPort,
		int maxPacketSize,
		int logLevel,
		NetCore* netCore
	);

	int SendResponse(HttpResponse& resp);

	void Stop();

protected:
	void OnConnected(unsigned int nSocket, unsigned int nIP, unsigned short usPort);

	int OnReceived(unsigned int nSocket, unsigned int nIP, unsigned short usPort, void* pData, int nDataLen);

	void OnClosed(unsigned int nSocket, unsigned int nIP, unsigned short usPort, unsigned int nErrorCode);

	void ProcessMsg();

	void OnHttpPacket(HttpProtocolHeader* protocolHeader, const char* data, size_t len);

	bool ParseHttpHeaders(const char* data, size_t len, HttpHeaders& hs, size_t& headerSize);

	void SendNotFoundResponse(unsigned int nSocket);

private:
	typedef std::map<std::string, IHttpRouteHandler*> RouteMap;

	RouteMap                  mRoutes;
	bool                      mStop;
	std::vector<wt::Thread*>  mThreads;
	PackageQueue              mPackageQueue;
};

WT_END
