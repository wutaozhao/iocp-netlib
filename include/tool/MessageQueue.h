#pragma once

#include "Config.h"
#include "tool/ThreadLock.h"
#include "tool/FixedMemoryPool.h"
#include "tool/Semaphore.h"

WT_BEGIN

enum MPopResult
{
	MPOP_OK = 0,
	MPOP_EMPTY
};

class MessageQueue
{
public:
	MessageQueue();
	~MessageQueue();

	int Init(size_t maxPacketPayload, size_t maxPackets);

	void Uninit();

	bool Push(const void* data, size_t len);

	int Pop();

	void* Front(size_t& len);

	size_t GetQueueSize();

	bool IsEmpty();

	void Clear();

private:
	void Release(void* buf);

private:
	std::queue<void*> m_queue;
	CThreadLock m_poolLock;
	FixedMemoryPool* m_pool;
};

WT_END