
#include "Client.h"


void Client::OnConnected(unsigned int nSocket, unsigned int nIP, unsigned short usPort)
{
	printf("client:%u connected, ip:%s, port:%d\n", nSocket, ToStringIP(nIP).c_str(), usPort);
}

int Client::OnReceived(unsigned int nSocket, unsigned int nIP, unsigned short usPort, void* pData, int nDataLen)
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
	mPackageQueue.Push(pw);

	return 0;
}

void Client::OnClosed(unsigned int nSocket, unsigned int nIP, unsigned short usPort, unsigned int nErrorCode)
{
	mSocketID = 0;
	printf("client:%u closed, ip:%s, port:%d, code:%d\n", nSocket, ToStringIP(nIP).c_str(), usPort, nErrorCode);
}