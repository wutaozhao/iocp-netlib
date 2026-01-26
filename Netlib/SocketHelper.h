
#pragma once

#include "Config.h"

WT_BEGIN

class SocketHelper {
public:
	SocketHelper();
	virtual ~SocketHelper();

	static bool InitSocket();
	static void UnInitSocket();

	bool Create(int nSocketType = SOCK_STREAM, unsigned int flags = 0);

	void Shutdown();

	void Close();

	inline bool IsInvalid() const { return (mSocket == INVALID_SOCKET); };

	bool Bind(unsigned short port, const char* ip = "0.0.0.0");

	bool Listen(int backlog = 5);

	SOCKET GetHandle();

	bool GetLocalAddr(unsigned long& ulNetAddr, unsigned short& ulPort);

	bool GetPeerAddr(unsigned long& ulNetAddr, unsigned short& ulPort);

	int SetSockOpt(int level, int optname, const char FAR* optval, int optlen);

private:
	SOCKET mSocket;
};

WT_END