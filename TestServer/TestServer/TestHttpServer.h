#pragma once

#include "BaseInclude.h"
#include "DBConnection.h"

class TestHttpServer
{
public:
	TestHttpServer();
	~TestHttpServer();

	int StartHttpServer(NetCore* core);

	void Stop();

public:
	void OnTestHttp(HttpRequest& request);

private:
	HttpService mHttpService;
	FixedObjectPool<CDBConnection>* mDBPool;
};