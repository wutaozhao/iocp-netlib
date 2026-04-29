
#include  "tool/MessageQueue.h"

namespace
{
	struct MQPacketHeader
	{
		size_t writePos;
		size_t capacity;
	};
}

WT_BEGIN

MessageQueue::MessageQueue():m_pool(0)
{
}

MessageQueue::~MessageQueue()
{
	Uninit();
}

int MessageQueue::Init(size_t maxPacketPayload,
	size_t maxPackets)
{
	if (m_pool) {
		return 0;
	}

	int ret = 1;
	do
	{
		m_pool = new(std::nothrow) FixedMemoryPool(sizeof(MQPacketHeader) + maxPacketPayload, maxPackets);
		if (!m_pool) {
			break;
		}

		ret = 0;
	} while (false);
	
	if (ret != 0) {
		Uninit();
	}

	return ret;
}

void MessageQueue::Uninit()
{
	LOCK_GUARD(m_poolLock);
	if (m_pool != NULL) {
		delete m_pool;
		m_pool = NULL;
	}
}

void MessageQueue::Clear()
{
	LOCK_GUARD(m_poolLock);
	while (!m_queue.empty()) {
		void* buf = m_queue.front();
		m_queue.pop();
		Release(buf);
	}
}

void* MessageQueue::Front(size_t& len)
{
	LOCK_GUARD(m_poolLock);
	if (m_queue.empty()) {
		len = 0;
		return NULL;
	}
	void* raw = m_queue.front();
	MQPacketHeader* header = (MQPacketHeader*)raw;
	len = header->writePos;
	return (char*)raw + sizeof(MQPacketHeader);
}

bool MessageQueue::IsEmpty()
{
	LOCK_GUARD(m_poolLock);
	return m_queue.empty();
}

bool MessageQueue::Push(const void* data, size_t len)
{
	LOCK_GUARD(m_poolLock);

	if (len > (m_pool->GetElementSize() - sizeof(MQPacketHeader)))
	    return false;

    void* buf = m_pool->Create();
	if (!buf)
		return false;

	memcpy((char*)buf + sizeof(MQPacketHeader), data, len);
    ((MQPacketHeader*)buf)->writePos = len;
    ((MQPacketHeader*)buf)->capacity = m_pool->GetElementSize() - sizeof(MQPacketHeader);
	m_queue.push(buf);
	
	return true;
}

int MessageQueue::Pop()
{
    LOCK_GUARD(m_poolLock);
    if (m_queue.empty())
        return MPOP_EMPTY;

    void* buf = m_queue.front();
    m_queue.pop();
    Release(buf);

    return MPOP_OK;
}

size_t MessageQueue::GetQueueSize()
{
	LOCK_GUARD(m_poolLock);
	return m_queue.size();
}

void MessageQueue::Release(void* buf)
{
	if (m_pool) {
		m_pool->Free(buf);
	}
}

WT_END
