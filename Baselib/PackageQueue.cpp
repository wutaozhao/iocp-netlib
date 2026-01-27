
#include  "PackageQueue.h"

WT_BEGIN

PacketRead::PacketRead() : m_owner(0), m_buf(0) {
}

PacketRead::~PacketRead()
{
	if (m_owner && m_buf)
	{
		((PackageQueue*)m_owner)->Release(m_buf);
	}
}

void PacketRead::Attach(void* owner, void* buf)
{
	m_buf = (unsigned char*)buf;
	m_owner = owner;
}

void* PacketRead::GetBuffer()
{
	return m_buf ? m_buf + sizeof(WTPacketHeader) : 0;
}

size_t PacketRead::GetDataLength()
{
	if (!m_buf) return 0;
	return ((WTPacketHeader*)m_buf)->writePos;
}

void PacketWrite::Attach(void* buf, size_t cap)
{
	m_buf = (unsigned char*)buf;
	WTPacketHeader* h = (WTPacketHeader*)m_buf;
	h->writePos = 0;
	h->capacity = cap;
}

bool PacketWrite::Write(const void* data, size_t len)
{
	WTPacketHeader* h = (WTPacketHeader*)m_buf;
	if (h->writePos + len > h->capacity)
		return false;

	memcpy(m_buf + sizeof(WTPacketHeader) + h->writePos,
		data, len);
	h->writePos += len;
	return true;
}

size_t PacketWrite::GetBufLen() {
	return ((WTPacketHeader*)m_buf)->writePos;
}

PackageQueue::PackageQueue():m_pool(0)
{
}

PackageQueue::~PackageQueue()
{
	Uninit();
}

int PackageQueue::Init(size_t maxPacketPayload,
	size_t maxPackets)
{
	if (m_pool) {
		return 0;
	}
	m_pool = new(std::nothrow) CObjectPool(sizeof(WTPacketHeader) + maxPacketPayload, maxPackets);

	if (m_pool) {
		return 0;
	}

	return 1;
}

void PackageQueue::Uninit()
{
	LOCK_GUARD(m_poolLock);
	if (m_pool != NULL) {
		delete m_pool;
		m_pool = NULL;
	}
}

bool PackageQueue::AllocWritePacket(PacketWrite& pw)
{
	LOCK_GUARD(m_poolLock);
	if (!m_pool) {
		return false;
	}

	void* buf = m_pool->Create();
	if (!buf)
		return false;

	pw.Attach(buf,
		m_pool->GetElementSize() - sizeof(WTPacketHeader));
	return true;
}

bool PackageQueue::Push(PacketWrite& pw)
{
	if (!pw.GetRawBuffer())
		return false;

	LOCK_GUARD(m_poolLock);
	m_queue.push(pw.GetRawBuffer());
	m_sem.Post();
	
	return true;
}

int PackageQueue::Pop(PacketRead& pr, unsigned int timeoutMs)
{
	if (!m_sem.Wait(timeoutMs))
		return POP_TIMEOUT;

	void* buf = 0;

	LOCK_GUARD(m_poolLock);
	if (m_queue.empty())
		return POP_EMPTY;

	buf = m_queue.front();
	m_queue.pop();
	pr.Attach(this, buf);

	return POP_OK;
}

size_t PackageQueue::GetQueueSize()
{
	LOCK_GUARD(m_poolLock);
	return m_queue.size();
}

void PackageQueue::Release(void* buf)
{
	LOCK_GUARD(m_poolLock);
	if (m_pool) {
		m_pool->Free(buf);
	}
}

WT_END