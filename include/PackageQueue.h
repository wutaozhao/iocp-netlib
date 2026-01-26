#pragma once

#include "Config.h"
#include "ThreadLock.h"
#include "ObjectPool.h"
#include "Semaphore.h"

WT_BEGIN

enum PopResult
{
	POP_OK = 0,
	POP_TIMEOUT,
	POP_EMPTY
};

struct WTPacketHeader
{
	size_t writePos;
	size_t capacity;
};

class PacketWrite
{
public:
	PacketWrite() : m_buf(0) {}

	void Attach(void* buf, size_t cap);

	bool Write(const void* data, size_t len);

	size_t GetBufLen();

	void* GetRawBuffer() const { return m_buf; }

private:
	unsigned char* m_buf;
};

class PacketRead
{
public:
	PacketRead();

	~PacketRead();

	void Attach(void* owner, void* buf);

	void* GetBuffer();

	size_t GetDataLength();

	bool IsValid() { return m_buf != 0; }

private:
	PacketRead& operator=(const PacketRead&);

private:
	unsigned char* m_buf;
	void*          m_owner;
};

class PackageQueue
{
public:
	friend class PacketRead;

	PackageQueue();
	~PackageQueue();

	int Init(size_t maxPacketPayload, size_t maxPackets);

	bool AllocWritePacket(PacketWrite& pw);

	bool Push(PacketWrite& pw);

	int Pop(PacketRead& pr, unsigned int timeoutMs);

	size_t GetQueueSize();

private:
	void Release(void* buf);

private:
	std::queue<void*> m_queue;

	CSemaphore m_sem;
	CThreadLock m_poolLock;

	CObjectPool* m_pool;
};

WT_END