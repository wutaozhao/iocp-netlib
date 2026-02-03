#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <new>
#include <assert.h>

namespace wt
{
    /*
    * actually, this is a fixed memory pool.
    */
    class FixedMemoryPool
    {
    private:
        struct FreeNode
        {
            FreeNode* next;
        };

        unsigned char* m_pool;      
        FreeNode* m_freeList;  
        size_t         m_elemSize;  
        size_t         m_capacity;
        size_t         m_freeCount;

    private:
        // 
        static size_t AlignUp(size_t size)
        {
            const size_t align = sizeof(void*);
            return (size + align - 1) & ~(align - 1);
        }

    public:
        FixedMemoryPool(size_t elemSize, size_t capacity)
            : m_pool(0)
            , m_freeList(0)
            , m_elemSize(0)
            , m_capacity(capacity)
            , m_freeCount(0)
        {
            assert(elemSize > 0);
            assert(capacity > 0);

            // 
            if (elemSize < sizeof(FreeNode))
                elemSize = sizeof(FreeNode);

            // 
            m_elemSize = AlignUp(elemSize);

            // 
            m_pool = (unsigned char*)::malloc(m_elemSize * capacity);
            if (!m_pool)
            {
                m_capacity = 0;
                return;
            }

            // 
            unsigned char* p = m_pool;
            for (size_t i = 0; i < capacity - 1; ++i)
            {
                FreeNode* node = (FreeNode*)p;
                node->next = (FreeNode*)(p + m_elemSize);
                p += m_elemSize;
            }

            ((FreeNode*)p)->next = 0;

            m_freeList = (FreeNode*)m_pool;
            m_freeCount = capacity;
        }

        ~FixedMemoryPool()
        {
            ::free(m_pool);
            m_pool = 0;
        }

    public:
        bool IsValid() const
        {
            return m_pool != 0;
        }

        size_t GetElementSize() const
        {
            return m_elemSize;
        }

        size_t GetFreeCount() const
        {
            return m_freeCount;
        }

        size_t GetUsedCount() const
        {
            return m_capacity - m_freeCount;
        }

    public:
        template <typename T>
        T* Create()
        {
            if (sizeof(T) > m_elemSize)
                return 0;

            void* mem = AllocRaw();
            if (!mem)
                return 0;

            return new (mem) T();
        }

        void* Create()
        {
            return AllocRaw();
        }

        template <typename T>
        void Free(T* obj)
        {
            if (!obj)
                return;

            obj->~T();
            FreeRaw(obj);
        }

        void Free(void* obj)
        {
            if (!obj)
                return;

            FreeRaw(obj);
        }

    private:
        void* AllocRaw()
        {
            if (!m_freeList)
                return 0;

            FreeNode* node = m_freeList;
            m_freeList = node->next;
            --m_freeCount;
            return node;
        }

        void FreeRaw(void* ptr)
        {
            if (!ptr) {
                return;
            }

            FreeNode* node = (FreeNode*)ptr;
            node->next = m_freeList;
            m_freeList = node;
            ++m_freeCount;
        }

    private:
        FixedMemoryPool(const FixedMemoryPool&);
        FixedMemoryPool& operator=(const FixedMemoryPool&);
    };

}
