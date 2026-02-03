
#include "TestHttpServer.h"
#include "MyService.h"

TestHttpServer::TestHttpServer()
{

}

TestHttpServer::~TestHttpServer()
{

}

int TestHttpServer::StartHttpServer(NetCore* core)
{
	/*
	CDBConnection* pConnection = new CDBConnection();
	pConnection->SetConnectionParam(g_service.m_serviceConfig.m_strDBAddr.c_str(), g_service.m_serviceConfig.m_strDBName.c_str(),
		g_service.m_serviceConfig.m_strDBLoginName.c_str(), g_service.m_serviceConfig.m_strDBLoginPwd.c_str());
	if (!pConnection->IsValid()) {
		printf("db connection invalid\n");
		return 0;
	}
	*/

	mDBPool = new FixedObjectPool<CDBConnection>(10);
	if (!mDBPool)
		return -1;

	std::vector<CDBConnection*> conns = mDBPool->GetAllObjects();
	for (size_t i = 0; i < conns.size(); i++) {
		conns[i]->SetConnectionParam(g_service.m_serviceConfig.m_strDBAddr.c_str(), g_service.m_serviceConfig.m_strDBName.c_str(),
			g_service.m_serviceConfig.m_strDBLoginName.c_str(), g_service.m_serviceConfig.m_strDBLoginPwd.c_str());
		//conns[i]->IsValid();
	}

	/*
	FixedPoolObject<CDBConnection> conn(mDBPool, 1000);
	if (!conn.Get()) {
		printf("no connection\n");
		return 0;
	}
	if (!conn->IsValid()) {
		printf("db connection invalid\n");
		return 0;
	}
	else {
		printf("db valid\n");
	}
	*/

	unsigned short port = 6870;
	std::string path1 = "/srv/test";
	mHttpService.SetRoute(path1, this, &TestHttpServer::OnTestHttp);

	int ret = mHttpService.Start("0.0.0.0", port, 10240, 3, core);
	if (ret != 0) {
		printf("start http service failed, ret:%d\n", ret);
	}

	printf("http server listening on:%d\n", (int)port);

	return ret;
}

void TestHttpServer::Stop()
{
	mHttpService.Stop();
}

void TestHttpServer::OnTestHttp(HttpRequest& request)
{
	// 通过threadid 创建并获取连接
	/*
	FixedPoolObject<CDBConnection> conn(mDBPool, 1000);
	if (!conn.Get()) {
		printf("no connection\n");
		return;
	}
	if (!conn->IsValid()) {
		printf("db connection invalid\n");
		return;
	}
	else {
		printf("db valid\n");
	*/
	printf("current threadId:%u\n", GetCurrentThreadId());
	char buffer[128] = { 0 };
	memcpy(buffer, request.body, min(sizeof(buffer) - 1, request.bodyLen));
	printf("body:%s\n", buffer);

	//
	HttpResponse response(request.socketId);
	response.body = "{\"Code\":0, \"Message\":\"it is ok\"}";

	int sendRet = mHttpService.SendResponse(response);
	printf("send ret:%d\n", sendRet);
}