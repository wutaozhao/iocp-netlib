// FixedObjectPool.h
#pragma once

#include <vector>
#include "tool/Semaphore.h"
#include "tool/ThreadLock.h"

WT_BEGIN

template <class T>
class FixedObjectPool
{
public:
    FixedObjectPool(int maxCount)
    {
        m_sem.Create(maxCount);
        m_freeList.reserve(maxCount);
        for (int i = 0; i < maxCount; ++i)
        {
            T* obj = new T();
            m_freeList.push_back(obj);
        }
    }

    ~FixedObjectPool()
    {
        for (size_t i = 0; i < m_freeList.size(); ++i)
            delete m_freeList[i];
    }

    T* Acquire(int timeoutMs = -1)
    {
        if (!m_sem.Wait(timeoutMs))
            return NULL;

        LOCK_GUARD(m_lock);
        T* obj = m_freeList.back();
        m_freeList.pop_back();
        return obj;
    }

    void Release(T* obj)
    {
        if (!obj)
            return;

        LOCK_GUARD(m_lock);
        m_freeList.push_back(obj);
        m_sem.Post();
    }

    int FreeCount()
    {
        LOCK_GUARD(m_lock);
        return (int)m_freeList.size();
    }

    std::vector<T*> GetAllObjects() {
        return m_freeList;
    }

private:
    std::vector<T*> m_freeList;
    CSemaphore      m_sem;
    CThreadLock     m_lock;
};

template <class T>
class FixedPoolObject
{
public:
    FixedPoolObject(FixedObjectPool<T>* pool, int timeoutMs = -1)
        : m_pool(pool)
    {
        m_obj = pool->Acquire(timeoutMs);
    }

    ~FixedPoolObject()
    {
        if (m_obj)
            m_pool->Release(m_obj);
    }

    T* Get() const { return m_obj; }
    T* operator->() { return m_obj; }
    operator bool() const { return m_obj != NULL; }

private:
    FixedObjectPool<T>* m_pool;
    T* m_obj;
};

WT_END
