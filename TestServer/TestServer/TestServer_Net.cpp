
#include "MyService.h"

void TestServer::OnConnected(unsigned int nSocket, unsigned int nIP, unsigned short usPort)
{
	printf("[listenPort:%d], client:%u connected, ip:%s, port:%d\n", mPort, nSocket, ToStringIP(nIP).c_str(), usPort);
}

int TestServer::OnReceived(unsigned int nSocket, unsigned int nIP, unsigned short usPort, void* pData, int nDataLen)
{
	//printf("receive data, socket:%u, len:%d\n", nSocket, nDataLen);
	PacketHeader header;
	header.createTime = GetTickCount64();
	header.nRemoteIP = nIP;
	header.nRemotePort = usPort;
	header.nSocketID = nSocket;

	PacketWrite pw;
	if (!mPackageQueue.AllocWritePacket(pw)) {
		printf("Alloc write packet failed\n");
		return -1;
	}

	pw.Write(&header, sizeof(header));
	pw.Write(pData, nDataLen);

	if (pw.GetBufLen() == 274) {
		printf("get invalid packet\n");
	}

	mPackageQueue.Push(pw);

	return 0;
}

void TestServer::OnClosed(unsigned int nSocket, unsigned int nIP, unsigned short usPort, unsigned int nErrorCode)
{
	printf("[listenPort:%d], client:%u closed, ip:%s, port:%d, code:%d\n", mPort, nSocket, ToStringIP(nIP).c_str(), usPort, nErrorCode);
}