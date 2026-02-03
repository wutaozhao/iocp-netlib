#pragma once

#include <vector>
#include <new>
#include <assert.h>
#include <stddef.h>

namespace wt
{
    /*
    * A dynamically growing memory pool.
    */
    class DynamicMemoryPool
    {
    public:
        DynamicMemoryPool(size_t elemSize, size_t maxCache)
            : m_elemSize(AlignUp(elemSize))
            , m_maxCache(maxCache)
            , m_usedCount(0)
        {
            assert(elemSize > 0);
            assert(maxCache > 0);
        }

        ~DynamicMemoryPool()
        {
            Clear();
        }

    public:
        void* Create()
        {
            void* p = 0;
            if (!m_cache.empty())
            {
                p = m_cache.back();
                m_cache.pop_back();
            }
            else
            {
                p = ::operator new(m_elemSize);
            }

            if (p)
                ++m_usedCount;

            return p;
        }

        template <typename T>
        T* Create()
        {
            assert(sizeof(T) <= m_elemSize);
            void* mem = Create();
            return mem ? new (mem) T() : 0;
        }

        template <typename T>
        void Free(T* obj)
        {
            if (!obj)
                return;

            obj->~T();
            Free((void*)obj);
        }

        void Free(void* obj)
        {
            if (!obj)
                return;

            assert(m_usedCount > 0);
            --m_usedCount;

            if (m_cache.size() < m_maxCache)
            {
                m_cache.push_back(obj);
            }
            else
            {
                ::operator delete(obj);
            }
        }

        size_t GetUsedCount() const
        {
            return m_usedCount;
        }

        size_t GetCachedCount() const
        {
            return m_cache.size();
        }

        size_t GetElementSize() const
        {
            return m_elemSize;
        }

    private:
        void Clear()
        {
            for (size_t i = 0; i < m_cache.size(); ++i)
                ::operator delete(m_cache[i]);
            m_cache.clear();
        }

        static size_t AlignUp(size_t size)
        {
            const size_t align = sizeof(void*);
            return (size + align - 1) & ~(align - 1);
        }

    private:
        DynamicMemoryPool(const DynamicMemoryPool&);
        DynamicMemoryPool& operator=(const DynamicMemoryPool&);

    private:
        std::vector<void*> m_cache;
        size_t             m_elemSize;
        size_t             m_maxCache;
        size_t             m_usedCount; 
    };
}
