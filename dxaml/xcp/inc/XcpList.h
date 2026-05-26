// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Templates for singly and doubly linked lists

#pragma once

// Singly linked lists

template <class C> class CXcpList
{
public:
    struct XCPListNode
    {
        C *m_pData;
        XCPListNode *m_pNext;    
    };

public:

    CXcpList()
    {
        m_pHead = NULL;
        m_pTail = NULL;
#if XCP_MONITOR
        m_isContainingWeakPointers = false;
#endif
    }
    
    ~CXcpList()
    {
        Clean();
    }
    
    HRESULT Add(C *pData)
    {
        XCPListNode *pTemp = m_pHead;
        HRESULT hr = S_OK;

        XCPListNode *pTempNode; 

        pTempNode = new XCPListNode;

#if XCP_MONITOR
        if (m_isContainingWeakPointers)
        {
            ::XcpMarkWeakPointer(pTempNode, &pTempNode->m_pData);
        }
#endif


        m_pHead = pTempNode;    
        if (!m_pTail)
        {
            m_pTail = m_pHead;
        }
        
        m_pHead->m_pNext = pTemp;
        m_pHead->m_pData = pData;      

        RRETURN(hr);//RRETURN_REMOVAL
    }

    HRESULT AddTail(C *pData)
    {
        XCPListNode *pTemp = m_pTail;
        HRESULT hr = S_OK;

        XCPListNode *pTempNode; 

        pTempNode = new XCPListNode;

#if XCP_MONITOR
        if (m_isContainingWeakPointers)
        {
            ::XcpMarkWeakPointer(pTempNode, &pTempNode->m_pData);
        }
#endif


        m_pTail = pTempNode;    
        if (!m_pHead)
        {
            m_pHead = m_pTail;
        }
        
        if (pTemp)
        {
            pTemp->m_pNext = m_pTail;
        }
        m_pTail->m_pNext = NULL;
        m_pTail->m_pData = pData;      

        RRETURN(hr);//RRETURN_REMOVAL
    }
    
    HRESULT Remove(C *pData,  XUINT8 bDoDelete = TRUE)
    {
        XCPListNode *pTemp;
        XCPListNode *pTempPrev = NULL; 
        XUINT8 bFound = FALSE;
        HRESULT hr = S_OK;

        if (m_pHead == NULL)
        {
            goto Cleanup;
        }

        if (m_pHead->m_pData == pData)
        {
            pTemp = m_pHead;
            m_pHead = pTemp->m_pNext;

            if (bDoDelete)
                DeleteItem(pTemp->m_pData);

            pTemp->m_pData = NULL;
            delete pTemp;
            pTemp = NULL;
            bFound = TRUE;

            goto Cleanup;

        }

        pTemp = m_pHead->m_pNext;
        pTempPrev = m_pHead;

        while (pTemp)
        {
            if (pTemp->m_pData == pData)
            {
                pTempPrev->m_pNext = pTemp->m_pNext;

                if (pTemp == m_pTail)
                    m_pTail = pTempPrev;
                    
                if (bDoDelete)
                    DeleteItem(pTemp->m_pData);

                pTemp->m_pData = NULL;
                delete pTemp;
                bFound = TRUE;                    
                pTemp = NULL;
                break;                

            }

            pTempPrev = pTemp;
            pTemp = pTemp->m_pNext;

        }

        Cleanup:    
            if (!m_pHead || !m_pTail)
            {
                m_pHead = m_pTail = NULL;
            }
            
            if (!bFound)
                hr = S_FALSE;

            RRETURN(hr);
    }
    
    void Clean( XUINT8 bDoDelete = TRUE)
    {
        XCPListNode *pTemp;
        
        while (m_pHead)
        {
            pTemp = m_pHead;
            m_pHead = m_pHead->m_pNext;
            
            if (bDoDelete)
                DeleteItem(pTemp->m_pData);
            pTemp->m_pData = NULL;

            delete pTemp;    
        }
        
        m_pHead = NULL;
        m_pTail = NULL;
    }

    XCPListNode *GetHead() const { return m_pHead; }

    XCPListNode *GetTail() const
    {
        return m_pTail;
    }

    void GetReverse(CXcpList<C>* const pReversedList)
    {
        XCPListNode *pTemp = m_pHead;

        while (pTemp)
        {
            pReversedList->Add(pTemp->m_pData);
            pTemp = pTemp->m_pNext;
        }
    }

    void XcpMarkWeakPointers()
    {
#if XCP_MONITOR
        // Can only call this when list is empty.
        ASSERT(m_pHead == NULL);
        ASSERT(m_pTail == NULL);
        m_isContainingWeakPointers = true;
#endif
    }

private:

    void DeleteItem(C* pData)
    {
        delete pData;
    }

    XCPListNode *m_pHead;
    XCPListNode *m_pTail;

#if XCP_MONITOR
    bool m_isContainingWeakPointers;
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
