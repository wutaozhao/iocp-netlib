
#include "Client.h"

Client::Client()
{
	mStop = false;
	mCheckThread = NULL;

	mSocketID = 0;

	mRemoteIP = "";
	mRemotePort = 0;

	ResetStats();
}

Client::~Client()
{
	StopClient();
}

int Client::StartClient(NetCore* core, int port)
{
	int ret = 0;

	do
	{
		MakeLog();

		ret = mPackageQueue.Init(4096, 500);
		if (ret != 0) {
			Log("error.log", LOG_LEVEL_ERROR, "init queue failed");
			break;
		}

		for (int i = 0; i < 4; i++) {
			wt::Thread* thread = new(std::nothrow) wt::Thread(this, &Client::ProcessMsg);
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
		if (mThreads.size() != 4) {
			Log("error.log", LOG_LEVEL_ERROR, "start threads failed, size:%d -- %d", mThreads.size(), 4);
			break;
		}

		mCheckTimer.SetTimer(6000);
		mCheckThread = new(std::nothrow) wt::Thread(this, &Client::CheckThreadProc);
		if (mCheckThread)
		{
			if (!mCheckThread->Start()) {
				delete mCheckThread;
				mCheckThread = NULL;
				break;
			}
		}
		else {
			break;
		}

		// start net service
		std::string ip = "0.0.0.0";
		unsigned short listenPort = 0;
		int maxConnection = 1;
		int maxPacketSize = 4096;
		int packetSizeOffset = 0;
		int clientTimeoutSec = 180;
		IIOCallback* callback = this;
		ret = mNetService.StartNetService(ip.c_str(), listenPort,
			10,
			maxConnection, maxPacketSize, 
			packetSizeOffset, clientTimeoutSec, 3, core, callback);
		if (ret != 0) {
			break;
		}

		//
		mRemoteIP = "10.1.28.91";
		mRemotePort = port;
		printf("begin connect server!\n");
		mSocketID = mNetService.ConnectServer(mRemoteIP.c_str(), mRemotePort, 1500);
		printf("socketID:%u\n", mSocketID);
		Log("error.log", LOG_LEVEL_ERROR, "connect get socketID:%u", mSocketID);

	} while (false);

	if (ret != 0) {
		StopClient();
	}

	return ret;
}

void Client::StopClient()
{
	if (mStop) {
		return;
	}
	mStop = true;

	//
	mNetService.StopNetService();

	//
	if (mCheckThread) {
		mCheckThread->Join(200);
	}

	for (size_t i = 0; i < mThreads.size(); i++) {
		mThreads[i]->Join(200);
		delete mThreads[i];
	}
	mThreads.clear();

	mLogManager.UnInit();
}

void Client::ProcessMsg()
{
	while (!mStop) {
		PacketRead readPacket;
		if (mPackageQueue.Pop(readPacket, 100) != 0) {
			continue;
		}

		mReceiveCount++;

		//
		size_t dataLen = readPacket.GetDataLength();

		PacketHeader* header = (PacketHeader*)readPacket.GetBuffer();
		BaseMsgHead* msgHead = (BaseMsgHead*)((char*)readPacket.GetBuffer() + sizeof(PacketHeader));

		LoginResponse* response = (LoginResponse*)(msgHead + 1);

		if (msgHead->cmd == CMD_LOGIN_RESPONSE && response->userId == 102) {
			TestSend();
		}
	}
}

int Client::TestSend()
{
	if (mSocketID == 0)
		return 0;

	//printf("begin send...\n");
	Buffer sendBuffer;
	char* buffer = sendBuffer.getBuffer(sizeof(BaseMsgHead) + sizeof(LoginRequest));
	if (!buffer) {
		printf("get buffer failed");
		return 0;
	}

	BaseMsgHead* respHead = (BaseMsgHead*)buffer;

	LoginRequest* request = (LoginRequest*)(respHead + 1);
	request->userId = 102;

	respHead->packetSize = sizeof(BaseMsgHead) + sizeof(LoginRequest);
	respHead->cmd = CMD_LOGIN_REQUEST;

	mSendCount++;
	int ret = SendData(sendBuffer.getData(), (int)sendBuffer.getSize());
	if (ret != 0) {
		Log("error.log", LOG_LEVEL_ERROR, "process send data failed:%d", ret);
	}
	else {
		mSendSuccCount++;
	}

	return ret;
}

void Client::CheckThreadProc()
{
	CTimer statsTimer;
	statsTimer.SetTimer(30 * 1000);
	while (!mStop) {
		if (mCheckTimer.IsTimed()) {
			if (mSocketID == 0) {
				mSocketID = mNetService.ConnectServer(mRemoteIP.c_str(), mRemotePort, 4000);
				if (mSocketID == 0) {
					printf("CheckThreadProc find socket id zero,thread:%u\n", GetCurrentThreadId());
				}
				else {
					TestSend();
				}
			}
		}
		else if (statsTimer.IsTimed()) {
			printf("recv:%d send:%d sendSucc:%d\n",
				mReceiveCount,
				mSendCount,
				mSendSuccCount);
			ResetStats();
		}
		else {
			Sleep(3000);
		}
	}
}

void Client::MakeLog()
{
	// 获取当前路径，创建时间戳文件夹
	char    szDirBuffer[256] = { 0 };
	time_t  ulNow = { 0 };
	time(&ulNow);
	struct tm pNow;
	localtime_s(&pNow, &ulNow);

	char  szTimeBuffer[64] = { 0 };
	snprintf(szTimeBuffer, 63,
		"%04d_%02d_%02d_%02d_%02d_%02d",
		pNow.tm_year + 1900,
		pNow.tm_mon + 1,
		pNow.tm_mday,
		pNow.tm_hour,
		pNow.tm_min,
		pNow.tm_sec);

	// 设置日志路径
#ifdef _WIN32
	if (GetModuleFileName(NULL, szDirBuffer, 255) == 0)
#else
	if (getcwd(szDirBuffer, sizeof(szDirBuffer) - 1) == NULL)
#endif
	{
		return;
	}

	if (strlen(szDirBuffer) + strlen(szTimeBuffer) > 240)
		return;

	//
#ifdef  _WIN32
	std::string strDir = szDirBuffer;
	size_t nPos = strDir.find_last_of('\\');

	strDir = strDir.substr(0, nPos);
	memset(szDirBuffer, 0, sizeof(szDirBuffer));
	memcpy(szDirBuffer, strDir.c_str(), min(sizeof(szDirBuffer), strDir.length()));
#endif
	strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), "/log/");
	strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), szTimeBuffer);
	strcat_s(szDirBuffer, sizeof(szDirBuffer) - strlen(szDirBuffer), "/");

	mLogManager.Init(szDirBuffer);
	mLogManager.SetLogLevel("error.log", 5);
}

void Client::Log(const char* logName, int level, const char* format, ...)
{
	va_list argp;
	va_start(argp, format);
	mLogManager.WriteV(logName, level, format, argp);
	va_end(argp);
}

int Client::SendData(const void* data, int len)
{
	if (mSocketID == -1) {
		return -1;
	}

	return mNetService.Send(mSocketID, data, len);
}