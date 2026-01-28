#pragma once

#include "BaseInclude.h"

class TestServer: public IIOCallback
{
public:
	TestServer();
	~TestServer();

	DECLARE_NETIOCP_INTERFACE()

	bool StartServer(NetCore* core, int threadCount, unsigned short port);

	void StopServer();

	void ResetStats() {
		mReceiveCount = 0;
		mSendCount = 0;
		mSendSuccCount = 0;
	}

public:
	int mReceiveCount;
	int mSendCount;
	int mSendSuccCount;

protected:
	void ProcessMessage();

	void CheckProc();

private:
	bool mStop;
	std::vector<wt::Thread*> mThreads;

	NetService        mNetService;
	PackageQueue      mPackageQueue;
	int               mPort;
	CTimer            mStatsTimer;

	wt::Thread*       mCheckThread;
};