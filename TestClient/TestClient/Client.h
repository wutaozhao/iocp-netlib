#pragma once

#include "BaseInclude.h"

class Client: public IIOCallback, public Singleton<Client>
{
public:
	DECLARE_NETIOCP_INTERFACE()

	Client();

	~Client();

	int StartClient(NetCore* core, int port);

	void StopClient();

	int TestSend();

	int SendData(const void* data, int len);

protected:
	void ProcessMsg();

	void CheckThreadProc();

	void MakeLog();

	void Log(const char* logName, int level, const char* format, ...);

	void ResetStats() {
		mReceiveCount = 0;
		mSendCount = 0;
		mSendSuccCount = 0;
	}

public:
	int mReceiveCount;
	int mSendCount;
	int mSendSuccCount;

private:
	bool mStop;

	NetService mNetService;

	PackageQueue mPackageQueue;

	LogManager   mLogManager;

	std::vector<wt::Thread*> mThreads;
	Thread* mCheckThread;
	CTimer  mCheckTimer;

	std::string mRemoteIP;
	unsigned short mRemotePort;

	unsigned int mSocketID;
};