#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <new>
#include <assert.h>

namespace wt
{
    class CObjectPool
    {
    private:
        struct FreeNode
        {
            FreeNode* next;
        };

        unsigned char* m_pool;      // 原始内存
        FreeNode* m_freeList;  // 空闲链表
        size_t         m_elemSize;  // 对齐后的元素大小
        size_t         m_capacity;
        size_t         m_freeCount;

    private:
        // 向上对齐到 void* 大小（C++03 可用的最大安全对齐）
        static size_t AlignUp(size_t size)
        {
            const size_t align = sizeof(void*);
            return (size + align - 1) & ~(align - 1);
        }

    public:
        CObjectPool(size_t elemSize, size_t capacity)
            : m_pool(0)
            , m_freeList(0)
            , m_elemSize(0)
            , m_capacity(capacity)
            , m_freeCount(0)
        {
            assert(elemSize > 0);
            assert(capacity > 0);

            // 保证至少能存放 FreeNode
            if (elemSize < sizeof(FreeNode))
                elemSize = sizeof(FreeNode);

            // 强制元素大小对齐
            m_elemSize = AlignUp(elemSize);

            // malloc 在 C++03 中保证返回地址满足 void* 对齐
            m_pool = (unsigned char*)::malloc(m_elemSize * capacity);
            if (!m_pool)
            {
                m_capacity = 0;
                return;
            }

            // 初始化空闲链表
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

        ~CObjectPool()
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
        CObjectPool(const CObjectPool&);
        CObjectPool& operator=(const CObjectPool&);
    };

} // namespace wt
