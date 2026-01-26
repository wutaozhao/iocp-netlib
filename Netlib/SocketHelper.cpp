
#include "SocketHelper.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

WT_BEGIN



SocketHelper::SocketHelper()
{
	mSocket = INVALID_SOCKET;
}

SocketHelper::~SocketHelper()
{

}

bool SocketHelper::InitSocket()
{
	WSADATA wsadata;
	int nRet = ::WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (0 != nRet)
	{
		return false;
	}

	return true;
}

void SocketHelper::UnInitSocket()
{
	::WSACleanup();
}

bool SocketHelper::Create(int nSocketType, unsigned int flags)
{
	Close();

	if (SOCK_STREAM == nSocketType)
		mSocket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, flags);
	else
		mSocket = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, flags);

	if (IsInvalid())
		return false;

	return true;
}

void SocketHelper::Shutdown()
{
	if (!IsInvalid())
	{
		::shutdown(mSocket, SD_BOTH);
	}
}

void SocketHelper::Close()
{
	if (!IsInvalid())
	{
		::closesocket(mSocket);
		mSocket = INVALID_SOCKET;
	}
}

bool SocketHelper::Bind(unsigned short port, const char* ip)
{
	if (IsInvalid())
		return false;

	SOCKADDR_IN sAddr;
	sAddr.sin_family = AF_INET;
	if (inet_pton(AF_INET, ip, &sAddr.sin_addr) != 1)
	{
		return false;
	}
	sAddr.sin_port = ::htons(port);
	if (::bind(mSocket, (PSOCKADDR)&sAddr, sizeof(sAddr)) == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}

bool SocketHelper::Listen(int backlog)
{
	if (IsInvalid())
		return false;

	if (::listen(mSocket, backlog) == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}

SOCKET SocketHelper::GetHandle()
{
	return mSocket;
}

bool SocketHelper::GetLocalAddr(unsigned long& ulNetAddr, unsigned short& ulPort)
{
	if (IsInvalid()) {
		return false;
	}

	sockaddr_in  addr;
	int nLen = sizeof(sockaddr_in);
	if (SOCKET_ERROR == ::getsockname(mSocket, (sockaddr*)&addr, &nLen))
		return false;

	ulNetAddr = addr.sin_addr.S_un.S_addr;
	ulPort = addr.sin_port;
	return true;
}

bool SocketHelper::GetPeerAddr(unsigned long& ulNetAddr, unsigned short& ulPort)
{
	if (IsInvalid()) {
		return false;
	}

	sockaddr_in  addr;
	int nLen = sizeof(sockaddr_in);
	if (SOCKET_ERROR == ::getpeername(mSocket, (sockaddr*)&addr, &nLen))
		return false;
	ulNetAddr = addr.sin_addr.S_un.S_addr;
	ulPort = addr.sin_port;
	return true;
}

int SocketHelper::SetSockOpt(int level, int optname, const char FAR* optval, int optlen)
{
	return ::setsockopt(mSocket, level, optname, optval, optlen);
}

WT_END