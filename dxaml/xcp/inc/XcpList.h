// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Vector-backed list container (replaces legacy linked-list implementation).
//      CXcpList<C> stores C* pointers in a std::vector.
//      Use OldestBegin/End or NewestBegin/End for explicit iteration direction.

#pragma once

#include <vector>
#include <algorithm>

template <class C>
class CXcpList
{
public:

    CXcpList() = default;

    explicit CXcpList(size_t reserveCount)
    {
        Reserve(reserveCount);
    }

    ~CXcpList()
    {
        Clean();
    }

    // Non-copyable but movable.
    CXcpList(const CXcpList&) = delete;
    CXcpList& operator=(const CXcpList&) = delete;
    CXcpList(CXcpList&&) = default;
    CXcpList& operator=(CXcpList&&) = default;

    HRESULT Add(C *pData)
    {
#if XCP_MONITOR
        const bool realloc = (m_data.size() == m_data.capacity());
#endif
        m_data.push_back(pData);
#if XCP_MONITOR
        if (m_isContainingWeakPointers)
        {
            if (!realloc)
            {
                ::XcpMarkWeakPointer(m_data.data(), &m_data.back());
            }
            else
            {
                // Buffer moved — re-mark all existing pointers at their new addresses
                for (size_t i = 0; i < m_data.size(); ++i)
                {
                    ::XcpMarkWeakPointer(m_data.data(), &m_data[i]);
                }
            }
        }
#endif
        return S_OK;
    }

    HRESULT Remove(C *pData, XUINT8 bDoDelete = TRUE)
    {
        HRESULT hr = S_FALSE;
        auto it = std::find(m_data.begin(), m_data.end(), pData);
        if (it != m_data.end())
        {
            if (bDoDelete)
            {
                DeleteItem(*it);
            }
            m_data.erase(it);
            hr = S_OK;

#if XCP_MONITOR
            if (m_isContainingWeakPointers)
            {
                // Re-mark all existing pointers at their new addresses
                for (size_t i = 0; i < m_data.size(); ++i)
                {
                    ::XcpMarkWeakPointer(m_data.data(), &m_data[i]);
                }
            }

            // Free the buffer when empty so the leak checker doesn't
            // flag the high-water-mark allocation that persists after
            // all items have been individually removed.
            if (m_data.empty())
            {
                m_data.shrink_to_fit();
            }
#endif
        }

        return hr;
    }

    void Clean(XUINT8 bDoDelete = TRUE)
    {
        if (bDoDelete)
        {
            for (C* item : m_data)
            {
                DeleteItem(item);
            }
        }

        // Shrink-to-fit so the vector's internal buffer is freed.  Under XCP_MONITOR
        // this discards stale weak-pointer tags the leak checker attached to the buffer.
        m_data.clear();
#if XCP_MONITOR
        m_data.shrink_to_fit();
#endif
    }

    void Reserve(size_t n) 
    {
        #if XCP_MONITOR
        const bool realloc = (n > m_data.capacity());
        #endif
        m_data.reserve(n);
        #if XCP_MONITOR
        if (m_isContainingWeakPointers && realloc)
        {
            // Buffer moved — re-mark all existing pointers at their new addresses
            for (size_t i = 0; i < m_data.size(); ++i)
            {
                ::XcpMarkWeakPointer(m_data.data(), &m_data[i]);
            }
        }
        #endif
    }
    size_t Size() const { return m_data.size(); }
    bool Empty() const { return m_data.empty(); }
    C* At(size_t i) const 
    {
        ASSERT(i < m_data.size());
        return m_data[i];
    }

    // Front() = oldest element (front of vector = first added)
    C* Front() const { return m_data.empty() ? nullptr : m_data.front(); }
    // Back() = most recently added element (back of vector = last added)
    C* Back() const { return m_data.empty() ? nullptr : m_data.back(); }

    // Oldest-to-newest iteration (forward through the vector)
    auto OldestBegin()       { return m_data.begin(); }
    auto OldestEnd()         { return m_data.end(); }
    auto OldestBegin() const { return m_data.cbegin(); }
    auto OldestEnd()   const { return m_data.cend(); }

    // Newest-to-oldest iteration (reverse through the vector)
    auto NewestBegin()       { return m_data.rbegin(); }
    auto NewestEnd()         { return m_data.rend(); }
    auto NewestBegin() const { return m_data.crbegin(); }
    auto NewestEnd()   const { return m_data.crend(); }

    void XcpMarkWeakPointers()
    {
#if XCP_MONITOR
        ASSERT(m_data.empty());
        m_isContainingWeakPointers = true;
#endif
    }

    void DeleteItem(C* pData)
    {
        delete pData;
    }

private:
    std::vector<C*> m_data;

#if XCP_MONITOR
    bool m_isContainingWeakPointers = false;
#endif
};

// A list whose links are embedded in the contained items
template<typename T>
class IntrusiveList
{
public:
    struct ListEntry
    {
        ListEntry *pPrev;
        ListEntry *pNext;

        ListEntry()
        {
            // The object starts out not on a list
            pPrev = NULL;
            pNext = NULL;
        }

        ~ListEntry()
        {
        }

        // Returns TRUE if the link is on a list
        bool IsOnList() const
        {
            // pPrev and pNext must both be NULL or non-NULL together
            ASSERT(
                ((pPrev == NULL) && (pNext == NULL)) ||
                ((pPrev != NULL) && (pNext != NULL))
                );

            return pNext != NULL;
        }
    };

    struct Iterator
    {
        ListEntry *pCurr;
        XUINT32 offsetToLink;

        T* operator*() const
        {
            ASSERT(offsetToLink != 0xffffffff); // ensure that End() iterator is not dereferenced
            return reinterpret_cast<T*>(reinterpret_cast<XBYTE*>(pCurr) - offsetToLink);
        }

        void operator++()
        {
            pCurr = pCurr->pNext;
        }

        bool operator==(const Iterator& rhs)
        {
            return (pCurr == rhs.pCurr);
        }

        bool operator!=(const Iterator& rhs)
        {
            return !(*this == rhs);
        }
    };

    ListEntry head;
    ListEntry tail;

    IntrusiveList()
    {
        head.pNext = &tail;
        head.pPrev = NULL;

        tail.pNext = NULL;
        tail.pPrev = &head;
    }

    Iterator Begin(
        XUINT32 offsetToLink
        )
    {
        Iterator it = { head.pNext, offsetToLink };
        return it;
    }

    Iterator End()
    {
        Iterator it = { &tail, 0xffffffff };
        return it;
    }

    void PushBack(
        _In_ ListEntry *pEntry
        )
    {
        // The list should be consistent
        ASSERT(tail.pPrev->pNext == &tail);

        // The new item must not already be on a list
        ASSERT(!pEntry->IsOnList());

        pEntry->pPrev = tail.pPrev;
        pEntry->pNext = &tail;

        tail.pPrev->pNext = pEntry;
        tail.pPrev = pEntry;
    }

    void Remove(
        _In_ ListEntry *pEntry
        )
    {
        // The object should be on the list
        ASSERT(pEntry->IsOnList());
        ASSERT(pEntry->pPrev->pNext == pEntry);
        ASSERT(pEntry->pNext->pPrev == pEntry);

        pEntry->pPrev->pNext = pEntry->pNext;
        pEntry->pNext->pPrev = pEntry->pPrev;

        pEntry->pPrev = NULL;
        pEntry->pNext = NULL;
    }

    void Clear()
    {
        while (!IsEmpty())
        {
            Remove(head.pNext);
        }
    }

    bool IsEmpty() const
    {
        return head.pNext == &tail;
    }

    T* GetHead(XUINT32 offsetToLink)
    {
        ASSERT(!IsEmpty());
        
        return *(Begin(offsetToLink));
    }
};
