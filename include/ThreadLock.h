
#ifndef __THREAD_LOCK_H__
#define __THREAD_LOCK_H__

#include "Config.h"

WT_BEGIN

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <pthread.h>
#endif

class CThreadLock
{
public:
	CThreadLock()
	{
#ifdef _WIN32
		::InitializeCriticalSection(&m_csLock);
#else
		pthread_mutex_init(&m_mutex, 0);
#endif
	}

	~CThreadLock()
	{
#ifdef _WIN32
		::DeleteCriticalSection(&m_csLock);
#else
		pthread_mutex_destroy(&m_mutex);
#endif
	}

	inline void Lock()
	{
#ifdef _WIN32
		::EnterCriticalSection(&m_csLock);
#else
		pthread_mutex_lock(&m_mutex);
#endif
	}

	inline void Unlock()
	{
#ifdef _WIN32
		::LeaveCriticalSection(&m_csLock);
#else
		pthread_mutex_unlock(&m_mutex);
#endif
	}

private:
#ifdef _WIN32
	CRITICAL_SECTION m_csLock;
#else
	pthread_mutex_t m_mutex;
#endif
};

template <class T>
class CLockGuard
{
	T* m_lock;
public:
	CLockGuard(T& lock) : m_lock(&lock) 
	{
		m_lock->Lock();
	}

	~CLockGuard()
	{
		m_lock->Unlock();
	}
};

#define LOCK_GUARD(lock) CLockGuard<CThreadLock> guard(lock);

WT_END

#endif
