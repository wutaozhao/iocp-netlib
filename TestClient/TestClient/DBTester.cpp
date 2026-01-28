
#include "DBTester.h"
#include "DBConnection.h"

struct ServerInfo {
	int server_id;
	int game_mode_id;
	int server_type_id;
	std::string ip;
	int server_port;
	int client_port;
	bool  disabled;
	int flag;
	int weight;
};

typedef std::vector<ServerInfo> ServerInfoArray;

DBTester::DBTester()
{

}

DBTester::~DBTester()
{

}

void DBTester::Test()
{
	DBConnectionParam param;
	param.m_strDBAddress = "192.168.7.209,1433";
	param.m_strDBSchema = "QFPlatformDBV2025";
	param.m_strDBUserName = "un_wtzhao";
	param.m_strDBUserPwd = "zhaosql123.com";

	CDBConnection* pConnectoin = new CDBConnection(&param);
	if (pConnectoin->IsValid()) {
		printf("db valid!\n");
	}
	else {
		printf("db not valid!");
	}

	ServerInfoArray arr;

	//
	do
	{
		try {
			if (!pConnectoin->IsValid()) {
				break;
			}

			CAdoStoreProc proc(pConnectoin, "P_GetServers");

			CADORecordset recSet(pConnectoin);
			if (!recSet.Execute(&proc))
			{
				std::string err = recSet.GetLastErrorString();
				pConnectoin->Close();
				printf("err:%s\n", err.c_str());
				break;
			}

			int nRecCount = recSet.GetRecordCount();
			if (nRecCount == 0)
			{
				break;
			}

			recSet.MoveFirst();

			bool bError = false;
			while (!recSet.IsEof())
			{
			    ServerInfo srvInfo;

				if (!recSet.GetFieldValue("ID", srvInfo.server_id)) {
					bError = true;
					break;
				}

				if (!recSet.GetFieldValue("GameModeID", srvInfo.game_mode_id)) {
					bError = true;
					break;
				}

				if (!recSet.GetFieldValue("ServerTypeID", srvInfo.server_type_id)) {
					bError = true;
					break;
				}

				if (!recSet.GetFieldValue("IP", srvInfo.ip)) {
					bError = true;
					break;
				}

				if (!recSet.GetFieldValue("ServerPort", srvInfo.server_port)) {
					bError = true;
					break;
				}

				if (!recSet.GetFieldValue("ClientPort", srvInfo.client_port)) {
					bError = true;
					break;
				}

				int disabled = 0;
				if (!recSet.GetFieldValue("Disabled", disabled)) {
					bError = true;
					break;
				}
				srvInfo.disabled = (disabled > 0);

				if (!recSet.GetFieldValue("Flag", srvInfo.flag)) {
					bError = true;
					break;
				}

				if (!recSet.GetFieldValue("Weight", srvInfo.weight)) {
					bError = true;
					break;
				}

				arr.push_back(srvInfo);

				recSet.MoveNext();
			}

			if (bError) {
				break;
			}
		}
		catch (...) {
			pConnectoin->Close();
		}
	} while (false);

	printf("size:%d\n", arr.size());
}