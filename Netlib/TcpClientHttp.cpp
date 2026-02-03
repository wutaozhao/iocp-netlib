#include "TcpClient.h"
#include "TcpService.h"

WT_BEGIN

static bool HeaderKeyEqual(const HttpHeader& h, const char* key)
{
	size_t klen = strlen(key);
	if (h.keyLen != klen)
		return false;
	return _strnicmp(h.key, key, klen) == 0;
}


static int GetContentLength(const HttpHeaders& hs)
{
	for (size_t i = 0; i < hs.headers.size(); ++i)
	{
		if (HeaderKeyEqual(hs.headers[i], "Content-Length"))
		{
			return atoi(hs.headers[i].value);
		}
	}
	return 0;
}

static bool IsConnectionClose(const HttpHeaders& hs)
{
	for (size_t i = 0; i < hs.headers.size(); ++i)
	{
		if (HeaderKeyEqual(hs.headers[i], "Connection"))
		{
			if (_strnicmp(hs.headers[i].value, "close", 5) == 0)
				return true;
		}
	}
	return false;
}


bool ParseHttpHeaders(const char* data,
	size_t len,
	HttpHeaders& hs,
	size_t& headerSize)
{
	const char* end = strstr(data, "\r\n\r\n");
	if (!end)
		return false;

	headerSize = (end - data) + 4;

	hs.raw = data;
	hs.rawLen = headerSize;
	hs.headers.clear();

	const char* p = strstr(data, "\r\n") + 2;

	while (p < end)
	{
		const char* lineEnd = strstr(p, "\r\n");
		if (!lineEnd)
			break;

		const char* colon = (const char*)memchr(p, ':', lineEnd - p);
		if (colon)
		{
			const char* v = colon + 1;
			while (v < lineEnd && *v == ' ')
				++v;

			HttpHeader h;
			h.key = p;
			h.keyLen = colon - p;
			h.value = v;
			h.valueLen = lineEnd - v;

			hs.headers.push_back(h);
		}
		p = lineEnd + 2;
	}
	return true;
}

int TcpClient::SplitHttpPacket(char* pData, int dataLen)
{
	int    leftSize = dataLen;
	char* pTemp = pData;

	while (leftSize > 0)
	{
		if (!mRecvPacketBuffer)
		{
			mRecvPacketBuffer = (char*)mTcpService->AllocRecvPacket();
			if (!mRecvPacketBuffer)
			{
				mTcpService->Log(LOG_LEVEL_ERROR, "http recv queue full");
				return NSE_RECV_QUEUE_FULL;
			}
			mRecvBytes = 0;
		}

		int copySize = min(leftSize,
			mTcpService->mMaxPacketSize - mRecvBytes);

		memcpy(mRecvPacketBuffer + mRecvBytes, pTemp, copySize);

		mRecvBytes += copySize;
		leftSize -= copySize;
		pTemp += copySize;

		HttpHeaders hs;
		size_t headerSize = 0;

		if (!ParseHttpHeaders(mRecvPacketBuffer,
			mRecvBytes,
			hs,
			headerSize))
		{
			return 0;
		}

		int contentLength = GetContentLength(hs);
		if (contentLength < 0)
		{
			mTcpService->Log(LOG_LEVEL_ERROR, "illegal http Content-Length");
			return NSE_ILLEGAL_RECV_PACKET;
		}

		int packetSize = (int)(headerSize + contentLength);
		if (packetSize > mTcpService->mMaxPacketSize)
		{
			return NSE_ILLEGAL_RECV_PACKET;
		}

		if (mRecvBytes < packetSize)
		{
			return 0;
		}

		int ret = mTcpService->mIOCallbackPtr->OnReceived(
			mLogicSocketID,
			mRemoteIP,
			mRemotePort,
			mRecvPacketBuffer,
			packetSize
		);
		if (ret != 0)
		{
			mTcpService->Log(LOG_LEVEL_ERROR, "http recv callback ret:%d", ret);
			SetDisconnect(NSE_BE_CLOSED);
			return NSE_BE_CLOSED;
		}

		mCloseAfterSend = IsConnectionClose(hs);
		
		mRecvBytes -= packetSize;
		if (mRecvBytes > 0)
		{
			memmove(mRecvPacketBuffer, mRecvPacketBuffer + packetSize, mRecvBytes);
		}
	}

	return 0;
}


WT_END