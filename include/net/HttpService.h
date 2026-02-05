#pragma once

#include "Config.h"
#include "net/NetService.h"

WT_BEGIN

struct HttpHeader
{
    const char* key;
    size_t      keyLen;
    const char* value;
    size_t      valueLen;
};

struct HttpHeaders
{
    const char* raw;
    size_t      rawLen;
    std::vector<HttpHeader> headers;

    const HttpHeader* Find(const char* name) const
    {
        for (size_t i = 0; i < headers.size(); ++i)
        {
            if (_strnicmp(headers[i].key, name, headers[i].keyLen) == 0)
                return &headers[i];
        }
        return NULL;
    }
};

struct HttpRequest
{
    unsigned int   socketId;
    unsigned int   remoteIP;
    unsigned short remotePort;

    const char*    method;     // "GET"
    size_t         methodLen;

    const char*    path;       // "/srv/test1"
    size_t         pathLen;

    HttpHeaders    headers;

    const char*    body;
    size_t         bodyLen;

    bool           keepAlive;

    // 
    bool IsMethod(const char* m) const
    {
        return _strnicmp(method, m, methodLen) == 0;
    }
};

class HttpResponse
{
public:
    unsigned int   socketId;
    int            status;
    std::string    body;
    std::string    headers;

    HttpResponse(int sid)
        : socketId(sid), status(200) {
    }

    void SetStatus(int s) { status = s; }
    void SetBody(const std::string& b) { body = b; }
    void AddHeader(const char* h)
    {
        headers.append(h).append("\r\n");
    }
};

class IHttpRouteHandler
{
public:
    virtual ~IHttpRouteHandler() {}
    virtual void Handle(HttpRequest& req) = 0;
};

template <class T>
class RouteHandlerImpl : public IHttpRouteHandler
{
public:
    typedef void (T::* HttpCallback)(HttpRequest&);

    RouteHandlerImpl(T* obj, HttpCallback cb)
        : mObj(obj)
        , mCallback(cb)
    {
    }

    virtual void Handle(HttpRequest& req)
    {
        (mObj->*mCallback)(req);
    }

private:
    T* mObj;
    HttpCallback mCallback;
};

class IHttpServiceImpl
{
public:
    virtual ~IHttpServiceImpl() {}

    virtual void SetRoute(
        const std::string& method,
        const std::string& path,
        IHttpRouteHandler* handler
    ) = 0;

    virtual int Start(
        const char* ip,
        unsigned short listenPort,
        int maxPacketSize,
        int logLevel,
        NetCore* netCore
    ) = 0;

    virtual int SendResponse(HttpResponse& resp) = 0;

    virtual void Stop() = 0;
};

class HttpService
{
public:
    HttpService();

    ~HttpService();

    template <class T>
    void SetRoute(
        const std::string& method,
        const std::string& path,
        T* obj,
        void (T::* callback)(HttpRequest&)
    )
    {
        IHttpRouteHandler* handler =
            CreateRouteHandler(obj, callback);

        mImpl->SetRoute(method, path, handler);
    }

    int Start(const char* ip,
        unsigned short listenPort,
        int maxPacketSize,
        int logLevel,
        NetCore* netCore);

    int SendResponse(HttpResponse& resp);

    void Stop();

private:
    IHttpServiceImpl* mImpl;

private:
    // 
    template <class T>
    static IHttpRouteHandler* CreateRouteHandler(
        T* obj,
        void (T::* callback)(HttpRequest&)
    );
};

template <class T>
IHttpRouteHandler* HttpService::CreateRouteHandler(
    T* obj,
    void (T::* callback)(HttpRequest&)
)
{
    return new RouteHandlerImpl<T>(obj, callback);
}


WT_END