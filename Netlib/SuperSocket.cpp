#include "net/SuperSocket.h"

WT_BEGIN

// ============================
// VS2005 安全 Tick 实现
// ============================
static unsigned long GetTickMs()
{
#ifdef _WIN32
	return GetTickCount(); // 32bit，约 49.7 天回绕
#else
	timeval tv;
	gettimeofday(&tv, 0);
	return (unsigned long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

// 处理回绕安全的时间差
static unsigned long TickDiff(unsigned long now, unsigned long old)
{
	return (now >= old) ? (now - old) : (0xFFFFFFFFUL - old + now + 1);
}

static bool ResolveIPv4(const char* host, unsigned long& outIp)
{
#if defined(_MSC_VER) && _MSC_VER < 1600
	unsigned long ip = inet_addr(host);
	if (ip != INADDR_NONE)
	{
		outIp = ip;
		return true;
	}

	hostent* he = gethostbyname(host);
	if (!he || he->h_addrtype != AF_INET || !he->h_addr_list[0])
		return false;

	outIp = *(unsigned long*)he->h_addr_list[0];
	return true;
#else
	in_addr addr;
	if (inet_pton(AF_INET, host, &addr) == 1)
	{
		outIp = addr.s_addr;
		return true;
	}

	addrinfo hints{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* res = nullptr;
	if (getaddrinfo(host, nullptr, &hints, &res) != 0)
		return false;

	sockaddr_in* sin = (sockaddr_in*)res->ai_addr;
	outIp = sin->sin_addr.s_addr;

	freeaddrinfo(res);
	return true;
#endif
}

// ============================
// 构造 / 析构
// ============================
SuperSocket::SuperSocket()
: m_fd(INVALID_SOCKET), m_type(TCP), m_blocking(true)
{
#ifdef _WIN32
	static bool inited = false;
	if (!inited)
	{
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
		inited = true;
	}
#endif
}

SuperSocket::~SuperSocket()
{
	Close();
}

// ============================
// 基础
// ============================
bool SuperSocket::Create(Type type)
{
	Close();
	m_type = type;

	int st = (type == TCP) ? SOCK_STREAM : SOCK_DGRAM;
	m_fd = socket(AF_INET, st, 0);

	return m_fd != INVALID_SOCKET;
}

void SuperSocket::Close()
{
	if (m_fd != INVALID_SOCKET)
	{
#ifdef _WIN32
		closesocket(m_fd);
#else
		close(m_fd);
#endif
		m_fd = INVALID_SOCKET;
	}
}

void SuperSocket::SetBlocking(bool block)
{
#ifdef _WIN32
	u_long mode = block ? 0 : 1;
	ioctlsocket(m_fd, FIONBIO, &mode);
#else
	int flags = fcntl(m_fd, F_GETFL, 0);
	fcntl(m_fd, F_SETFL, block ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK));
#endif
	m_blocking = block;
}

// ============================
// Connect（带超时）
// ============================
bool SuperSocket::Connect(const char* host, unsigned short port, int timeoutMs)
{
	if (!host)
		return false;

	Create(wt::SuperSocket::TCP);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	unsigned long ip;
	if (!ResolveIPv4(host, ip))
		return false;

	addr.sin_addr.s_addr = ip;

	SetBlocking(false);

	int ret = connect(m_fd, (sockaddr*)&addr, sizeof(addr));

#ifdef _WIN32
	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS)
			return false;
	}
#else
	if (ret < 0 && errno != EINPROGRESS)
		return false;
#endif

	int ev = WaitEvent(EVENT_WRITE | EVENT_ERROR, timeoutMs);
	if (!(ev & EVENT_WRITE))
		return false;

	int so_error = 0;
	socklen_t len = sizeof(so_error);
	getsockopt(m_fd, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);

	return so_error == 0;
}

// ============================
// Recv / Send
// ============================
int SuperSocket::Recv(void* buf, int len)
{
	return recv(m_fd, (char*)buf, len, 0);
}

int SuperSocket::Recv(void* buf, int len, int timeoutMs)
{
	int ev = WaitEvent(EVENT_READ | EVENT_ERROR, timeoutMs);
	if (!(ev & EVENT_READ))
		return -1;

	return recv(m_fd, (char*)buf, len, 0);
}

int SuperSocket::RecvAll(void* buf, int len, int timeoutMs)
{
	char* p = (char*)buf;
	int total = 0;
	unsigned long start = GetTickMs();

	while (total < len)
	{
		unsigned long used = TickDiff(GetTickMs(), start);
		if ((int)used >= timeoutMs)
			return -1;

		int remain = timeoutMs - (int)used;
		int ret = Recv(p + total, len - total, remain);

		if (ret > 0) total += ret;
		else return ret;
	}
	return total;
}

int SuperSocket::SendAll(const void* buf, int len, int timeoutMs)
{
	const char* p = (const char*)buf;
	int total = 0;
	unsigned long start = GetTickMs();

	while (total < len)
	{
		unsigned long used = TickDiff(GetTickMs(), start);
		if ((int)used >= timeoutMs)
			return -1;

		int remain = timeoutMs - (int)used;
		int ev = WaitEvent(EVENT_WRITE | EVENT_ERROR, remain);
		if (!(ev & EVENT_WRITE))
			return -1;

		int ret = send(m_fd, p + total, len - total, 0);
		if (ret <= 0)
			return -1;

		total += ret;
	}
	return total;
}

// ============================
// select
// ============================
int SuperSocket::WaitEvent(int events, int timeoutMs)
{
	fd_set rfds, wfds, efds;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);

	if (events & EVENT_READ)  FD_SET(m_fd, &rfds);
	if (events & EVENT_WRITE) FD_SET(m_fd, &wfds);
	FD_SET(m_fd, &efds);

	timeval tv;
	tv.tv_sec  = timeoutMs / 1000;
	tv.tv_usec = (timeoutMs % 1000) * 1000;

	int ret = select((int)m_fd + 1,
		&rfds, &wfds, &efds,
		timeoutMs < 0 ? 0 : &tv);

	int fired = EVENT_NONE;
	if (ret > 0)
	{
		if (FD_ISSET(m_fd, &rfds)) fired |= EVENT_READ;
		if (FD_ISSET(m_fd, &wfds)) fired |= EVENT_WRITE;
		if (FD_ISSET(m_fd, &efds)) fired |= EVENT_ERROR;
	}
	return fired;
}

// ============================
// shutdown
// ============================
void SuperSocket::ShutdownRead()  { shutdown(m_fd, SD_RECEIVE); }
void SuperSocket::ShutdownWrite() { shutdown(m_fd, SD_SEND); }
void SuperSocket::ShutdownBoth()  { shutdown(m_fd, SD_BOTH); }

// ============================
// KeepAlive（VS2005 安全）
// ============================
bool SuperSocket::EnableKeepAlive(int idleSec, int intervalSec, int count)
{
	int on = 1;
	setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on));

#ifdef _WIN32
	struct tcp_keepalive
	{
		unsigned long onoff;
		unsigned long keepalivetime;
		unsigned long keepaliveinterval;
	};

	tcp_keepalive ka;
	ka.onoff = 1;
	ka.keepalivetime = idleSec * 1000;
	ka.keepaliveinterval = intervalSec * 1000;

	DWORD retBytes = 0;
	return WSAIoctl(m_fd, SIO_KEEPALIVE_VALS,
		&ka, sizeof(ka),
		0, 0, &retBytes, 0, 0) == 0;
#else
	setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPIDLE, &idleSec, sizeof(idleSec));
	setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPINTVL, &intervalSec, sizeof(intervalSec));
	setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
	return true;
#endif
}

WT_END
