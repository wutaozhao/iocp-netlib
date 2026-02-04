#pragma once

#include "BaseInclude.h"
#include "DBConnection.h"

class TestHttpServer
{
public:
	typedef std::map<unsigned int, CDBConnection*> ThreadDBConnectionMap;
	TestHttpServer();
	~TestHttpServer();

	int StartHttpServer(NetCore* core);

	void Stop();

	CDBConnection* GetThreadConnection(unsigned int threadID);

public:
	void OnTestHttp(HttpRequest& request);

private:
	HttpService mHttpService;
	FixedObjectPool<CDBConnection>* mDBPool;

	CThreadLock           mThreadConnectionMapLock;
	ThreadDBConnectionMap mThreadConnectionMap;
};