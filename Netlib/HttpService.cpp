
#include "Config.h"
#include "net/HttpService.h"
#include "HttpServiceImplementer.h"
#include "BaseDefine.h"

WT_BEGIN

HttpService::HttpService()
{
	mImpl = new(std::nothrow) HttpServiceImplementer();
}

HttpService::~HttpService()
{
	delete mImpl;
}

int HttpService::Start(const char* ip,
    unsigned short listenPort,
    int maxPacketSize,
    int logLevel,
    NetCore* netCore)
{
    if (!mImpl) {
        return NSE_SYSTEM_ERROR;
    }

    return mImpl->Start(ip, listenPort, maxPacketSize, logLevel, netCore);
}

int HttpService::SendResponse(HttpResponse& resp)
{
    if (!mImpl) {
        return NSE_SYSTEM_ERROR;
    }

    return mImpl->SendResponse(resp);
}

void HttpService::Stop()
{
    if (!mImpl) {
        return;
    }

    return mImpl->Stop();
}

WT_END