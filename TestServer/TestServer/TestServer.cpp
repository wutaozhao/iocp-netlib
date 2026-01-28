
#include "MyService.h"

TestServer::TestServer()
{
	mStop = false;
	mPort = 0;
	mCheckThread = NULL;

	ResetStats();
}

TestServer::~TestServer()
{

}

bool TestServer::StartServer(NetCore* core, int threadCount, unsigned short port)
{
	do
	{
		if (mPackageQueue.Init(4096, 200) != 0) {
			printf("[listenPort:%d] init queue failed\n", port);
			break;
		}

		for (int i = 0; i < threadCount; i++) {
			wt::Thread* thread = new(std::nothrow) wt::Thread(this, &TestServer::ProcessMessage);
			if (thread)
			{
				if (thread->Start()) {
					mThreads.push_back(thread);
				}
				else {
					delete thread;
				}
			}
		}

		if (mThreads.size() != threadCount) {
			LOG("error", LOG_LEVEL_ERROR, "start threads failed, size:%d -- %d", mThreads.size(), threadCount);
			break;
		}

		mStatsTimer.SetTimer(30 * 1000);
		mCheckThread = new(std::nothrow) wt::Thread(this, &TestServer::CheckProc);
		if (mCheckThread) {
			if (!mCheckThread->Start()) {
				SafeDelete(mCheckThread);
				break;
			}
		}

		mPort = port;

		std::string ip = "0.0.0.0";
		unsigned short listenPort = port;
		int listenBacklog = 16;
		int maxConnection = g_service.m_serviceConfig.m_nMaxConnectionCount;
		int maxPacketSize = g_service.m_serviceConfig.m_nMaxSendPacketSize;
		int packetSizeOffset = 0;
		int clientTimeoutSec = 180;
		int nLogLevel = LOG_LEVEL_INFO;
		IIOCallback* callback = this;
		int nRet = mNetService.StartNetService(ip.c_str(), listenPort, listenBacklog,
			maxConnection, maxPacketSize,
			packetSizeOffset, clientTimeoutSec, nLogLevel, core, callback);
		if (nRet != 0)
		{
			LOG("error", LOG_LEVEL_ERROR, "Net service start failed, error: %d", nRet);
		}
		else
		{
			printf("[listenPort:%d], server is running!\n", mPort);
		}
	} while (false);
	

	return true;
}

void TestServer::StopServer()
{
	if (mStop) {
		return;
	}
	mStop = true;

	mNetService.StopNetService();

	for (size_t i = 0; i < mThreads.size(); i++) {
		mThreads[i]->Join(200);
		delete mThreads[i];
	}
	mThreads.clear();

	if (mCheckThread) {
		mCheckThread->Join(200);
		SafeDelete(mCheckThread);
	}
}

void TestServer::CheckProc()
{
	while (!mStop) {
		if (mStatsTimer.IsTimed()) {
			printf("[listenPort:%d], recv:%d send:%d sendSucc:%d\n",
				mPort,
				mReceiveCount,
				mSendCount,
				mSendSuccCount);
			ResetStats();
		}
		else {
			Sleep(1000);
		}
	}
}

void TestServer::ProcessMessage()
{
	while (!mStop) {
		PacketRead readPacket;
		if (mPackageQueue.Pop(readPacket, 100) != 0) {
			continue;
		}

		mReceiveCount++;

		//
		int dataLen = readPacket.GetDataLength();

		PacketHeader* header = (PacketHeader*)readPacket.GetBuffer();

		//113
		BaseMsgHead* msgHead = (BaseMsgHead*)((char*)readPacket.GetBuffer() + sizeof(PacketHeader));
		LoginRequest* request = (LoginRequest*)(msgHead + 1);

		if (msgHead->cmd == CMD_LOGIN_REQUEST) {
			mSendCount++;
			Buffer sendBuffer;
			char* buffer = sendBuffer.getBuffer(sizeof(BaseMsgHead) + sizeof(LoginResponse));
			if (!buffer) {
				printf("get buffer failed");
				continue;
			}

			BaseMsgHead* respHead = (BaseMsgHead*)buffer;
			
			LoginResponse* response = (LoginResponse*)(respHead + 1);
			response->errorCode = 100;
			response->userId = request->userId;

			respHead->packetSize = sizeof(BaseMsgHead) + sizeof(LoginResponse);
			respHead->cmd = CMD_LOGIN_RESPONSE;

			int ret = mNetService.Send(header->nSocketID, sendBuffer.getData(), sendBuffer.getSize());
			if (ret != 0) {
				LOG("error", LOG_LEVEL_ERROR, "process send data failed:%d", ret);
			}
			else {
				mSendSuccCount++;
			}
		}
	}
}