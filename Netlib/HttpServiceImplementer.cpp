
#include "Config.h"
#include "HttpServiceImplementer.h"
#include "BaseDefine.h"

WT_BEGIN

HttpServiceImplementer::HttpServiceImplementer()
{
	mStop = false;
}

HttpServiceImplementer::~HttpServiceImplementer()
{
	Stop();
}

void HttpServiceImplementer::SetRoute(const std::string& path, IHttpRouteHandler* handler)
{
	mRoutes[path] = handler;
}


int HttpServiceImplementer::Start(
	const char* ip,
	unsigned short listenPort,
	int maxPacketSize,
	int logLevel,
	NetCore* netCore
)
{
	int ret = 0;

	do
	{
		int listenBackLog = 4;
		int maxConnection = 10;
		int packetSizeOffset = 0;
		int clientTimeoutSec = 10;

		ret = mPackageQueue.Init(maxPacketSize, 100);
		if (ret != 0) {
			LogS(netCore, LOG_LEVEL_ERROR, "StartHttpService init queue failed, ret£º%d", ret);
			break;
		}

		int threadCount = 4;
		for (int i = 0; i < threadCount; i++) {
			wt::Thread* th = new(std::nothrow) wt::Thread(this, &HttpServiceImplementer::ProcessMsg);
			if (th) {
				if (th->Start()) {
					mThreads.push_back(th);
				}
				else {
					delete th;
				}
			}
		}
		if ((int)mThreads.size() != threadCount) {
			LogS(netCore, LOG_LEVEL_ERROR, "StartHttpService start threads failed");
			break;
		}

		ret = StartNetService(ip, listenPort, listenBackLog, maxConnection, maxPacketSize, packetSizeOffset,
			clientTimeoutSec, logLevel, netCore, this, NET_PROTOCOL_HTTP);
		if (ret != 0) {
			LogS(netCore, LOG_LEVEL_ERROR, "StartHttpService failed, ret£º%d", ret);
			break;
		}

		LogS(netCore, LOG_LEVEL_ERROR, "StartHttpService start successful!");

	} while (false);

	if (ret != 0) {
		Stop();
	}

	return ret;
}

void HttpServiceImplementer::Stop()
{
	if (mStop)
		return;

	mStop = true;

	StopNetService();

	for (size_t i = 0; i < mThreads.size(); i++) {
		mThreads[i]->Join(200);
		delete mThreads[i];
	}
	mThreads.clear();

	// clear routes
	for (RouteMap::iterator it = mRoutes.begin(); it != mRoutes.end(); ++it)
		delete it->second;
	mRoutes.clear();
}

void HttpServiceImplementer::OnConnected(unsigned int nSocket, unsigned int nIP, unsigned short usPort)
{

}

int HttpServiceImplementer::OnReceived(unsigned int nSocket, unsigned int nIP, unsigned short usPort, void* pData, int nDataLen)
{
	HttpProtocolHeader header;
	header.remoteIP = nIP;
	header.remotePort = usPort;
	header.socketID = nSocket;

	PacketWrite pw;
	if (!mPackageQueue.AllocWritePacket(pw)) {
		return -1;
	}

	pw.Write(&header, sizeof(header));
	pw.Write(pData, nDataLen);
	mPackageQueue.Push(pw);

	return 0;
}

void HttpServiceImplementer::OnClosed(unsigned int nSocket, unsigned int nIP, unsigned short usPort, unsigned int nErrorCode)
{

}

void HttpServiceImplementer::ProcessMsg()
{
	while (!mStop) {
		PacketRead pr;
		if (mPackageQueue.Pop(pr, 100) != 0)
			continue;

		HttpProtocolHeader* header = (HttpProtocolHeader*)pr.GetBuffer();
		char*  data = (char*)(header + 1);
		size_t dataLen = pr.GetDataLength() - sizeof(HttpProtocolHeader);

		OnHttpPacket(header, data, dataLen);
	}
}

void HttpServiceImplementer::OnHttpPacket(HttpProtocolHeader* protocolHeader, const char* data, size_t len)
{
	HttpRequest req;
	req.socketId = protocolHeader->socketID;
	req.remoteIP = protocolHeader->remoteIP;
	req.remotePort = protocolHeader->remotePort;
	
	const char* p = data;
	const char* sp1 = strchr(p, ' ');
	const char* sp2 = strchr(sp1 + 1, ' ');

	req.method = p;
	req.methodLen = sp1 - p;

	req.path = sp1 + 1;
	req.pathLen = sp2 - (sp1 + 1);

	size_t headerSize = 0;
	ParseHttpHeaders(data, len, req.headers, headerSize);

	const HttpHeader* cl = req.headers.Find("Content-Length");
	req.body = data + headerSize;
	req.bodyLen = cl ? atoi(cl->value) : 0;

	const HttpHeader* conn = req.headers.Find("Connection");
	req.keepAlive = (conn && _strnicmp(conn->value, "keep-alive", 10) == 0);

	std::string path(req.path, req.pathLen);
	RouteMap::iterator it = mRoutes.find(path);
	if (it != mRoutes.end())
	{
		it->second->Handle(req);
	}
}

bool HttpServiceImplementer::ParseHttpHeaders(const char* data,
	size_t len,
	HttpHeaders& hs,
	size_t& headerSize)
{
	const char* end = strstr(data, "\r\n\r\n");
	if (!end)
		return false;

	headerSize = (end - data) + 4;

	hs.raw = data;
	hs.rawLen = headerSize;
	hs.headers.clear();

	const char* p = strstr(data, "\r\n") + 2;

	while (p < end)
	{
		const char* lineEnd = strstr(p, "\r\n");
		if (!lineEnd)
			break;

		const char* colon = (const char*)memchr(p, ':', lineEnd - p);
		if (colon)
		{
			const char* v = colon + 1;
			while (v < lineEnd && *v == ' ')
				++v;

			HttpHeader h;
			h.key = p;
			h.keyLen = colon - p;
			h.value = v;
			h.valueLen = lineEnd - v;

			hs.headers.push_back(h);
		}
		p = lineEnd + 2;
	}
	return true;
}

int HttpServiceImplementer::SendResponse(HttpResponse& resp)
{
	if (resp.body.size() > (size_t)mMaxPacketSize) {
		return NSE_ILLEGAL_SEND_PACKET;
	}

	return SendHttpResponse(resp);
}

WT_END