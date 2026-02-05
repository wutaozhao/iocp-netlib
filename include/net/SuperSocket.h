#pragma once

#include "Config.h"

WT_BEGIN

class SuperSocket
{
public:
	enum Type
	{
		TCP,
		UDP
	};

	enum Event
	{
		EVENT_NONE  = 0,
		EVENT_READ  = 1 << 0,
		EVENT_WRITE = 1 << 1,
		EVENT_ERROR = 1 << 2
	};

public:
	SuperSocket();
	~SuperSocket();

	bool Create(Type type);
	void Close();

	bool Bind(const char* ip, unsigned short port);

	// TCP
	bool Connect(const char* host, unsigned short port, int timeoutMs);

	int  Send(const void* buf, int len);
	int  Recv(void* buf, int len);
	int  Recv(void* buf, int len, int timeoutMs);

	int  RecvAll(void* buf, int len, int timeoutMs);
	int  SendAll(const void* buf, int len, int timeoutMs);

	// UDP
	int  SendTo(const char* ip, unsigned short port, const void* buf, int len);
	int  RecvFrom(char* ip, unsigned short& port, void* buf, int len);

	// select
	int  WaitEvent(int events, int timeoutMs);

	// shutdown
	void ShutdownRead();
	void ShutdownWrite();
	void ShutdownBoth();

	// keepalive
	bool EnableKeepAlive(int idleSec, int intervalSec, int count);

	void   SetBlocking(bool block);
	SOCKET GetFD() const { return m_fd; }

private:
	SOCKET m_fd;
	Type   m_type;
	bool   m_blocking;
};

WT_END
