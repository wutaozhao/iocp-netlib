
#include "IOContext.h"

IOContext::IOContext(void)
{
	Reset();
}

IOContext::~IOContext(void)
{
	Reset();
}

void IOContext::Reset()
{
	ZeroMemory(&mOverlapped, sizeof(OVERLAPPED));
	mBuffer.buf = NULL;                   //»º³åÇø½á¹¹
	mBuffer.len = 0;
	mPointer = NULL;
	mTcpClient = NULL;
	mTcpService = NULL;
	mOperationType = IOCP_OP_IDLE;
}