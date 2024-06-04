// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CMediaQueueManagerItem;

class CMediaQueue;
class CMediaQueueItem;

//------------------------------------------------------------------------
//
//  Class:  CMediaQueueManager
//
//  Synopsis:
//      The media queue manager is used to create new queues for event
//      processing that needs to be run on the UI thread.
//
//------------------------------------------------------------------------

class CMediaQueueManager final
{
    friend class CMediaQueue;

public:
    static _Check_return_ HRESULT Create(_Outptr_ CMediaQueueManager** ppQueueManager);

    virtual ~CMediaQueueManager() = default;

    _Check_return_ HRESULT CreateMediaQueue(
        _In_ IMediaQueueClient* pClient,
        _In_ CCoreServices *pCore,
        _Outptr_ CMediaQueue** ppQueue
        );

    _Check_return_ HRESULT ProcessQueues(_In_ bool bIsShuttingDown = false);

protected:
    CMediaQueueManager();

    _Check_return_ HRESULT Shutdown();

    _Check_return_ HRESULT AddMediaQueue(_In_ CMediaQueue* pQueue);
    _Check_return_ HRESULT RemoveMediaQueue(_In_ CMediaQueue* pQueue);

    wil::critical_section     m_Lock;
    CMediaQueueManagerItem* m_pHead;
    CMediaQueueManagerItem* m_pTail;
    bool                   m_fShutdown;
};

//------------------------------------------------------------------------
//
//  Class:  CMediaQueue
//
//  Synopsis:
//      Use to perform media processing on the UI thread.  Certain actions
//  in media are driven by actions of the driver or platform worker threads.
//  To guarantee thread safety we need to post a message to this queue and
//  perform those actions on the next Tick/Draw request to the core.
//
//------------------------------------------------------------------------

class CMediaQueue final : public CXcpObjectBase< IMediaQueue >
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CMediaQueueManager* pManager,
        _In_ IMediaQueueClient* pClient,
        _In_ CCoreServices *pScheduler,
        _Outptr_ CMediaQueue** ppQueue
        );

    ~CMediaQueue() override;

    _Check_return_ HRESULT ProcessQueue() override;
    _Check_return_ HRESULT Shutdown() override;
    _Check_return_ HRESULT AddMediaEvent(_In_opt_ IObject* pState, _In_opt_ IMediaQueueTest* pTest) override;

protected:
    CMediaQueue();

    _Check_return_ HRESULT Initialize(
        _In_ CMediaQueueManager* pManager,
        _In_ IMediaQueueClient* pClient,
        _In_ CCoreServices *pCore
        );

private:
    CMediaQueueManager*         m_pManager;
    IMediaQueueClient*          m_pClient;
    CCoreServices*              m_pCoreNoRef{};
    wil::critical_section       m_Lock;
    CMediaQueueItem*            m_pHead;
    CMediaQueueItem*            m_pTail;
    bool                       m_fShutdown;
};

//------------------------------------------------------------------------
//
//  Class:  CMediaListItemBase
//
//  Synopsis:
//      Base class used for implementing the doubly linked lists used by
//  the media queue classes.
//
//------------------------------------------------------------------------
template< typename T >
class CMediaListItemBase
{
public:
    CMediaListItemBase() : Next(NULL),
                           Previous(NULL)
    {
    }

    virtual ~CMediaListItemBase()
    {
    }

    void InsertAfter(_In_ T* pInsertionPoint)
    {
        if(pInsertionPoint)
        {
            Next = pInsertionPoint->Next;
            Previous = pInsertionPoint;

            if(Next)
            {
                Next->Previous = (T*)this;
            }

            if(Previous)
            {
                Previous->Next = (T*)this;
            }
        }
    }

    void RemoveFromList()
    {
        if(Next)
        {
            Next->Previous = Previous;
        }

        if(Previous)
        {
            Previous->Next = Next;
        }

        Next = NULL;
        Previous = NULL;
    }

public:
    T*  Next;
    T*  Previous;
};

//------------------------------------------------------------------------
//
//  Class:  CMediaQueueManagerItem
//
//  Synopsis:
//      An item in the queue manager list. This holds a pointer to a
//  created queue.
//
//------------------------------------------------------------------------
class CMediaQueueManagerItem final : public CMediaListItemBase< CMediaQueueManagerItem >
{
public:
    CMediaQueueManagerItem(CMediaQueue* pQueue);
    ~CMediaQueueManagerItem() override;

    CMediaQueue* GetQueue();
    void MarkForRemoval();
    bool GetMarkedForRemoval();

private:
    CMediaQueue* m_Queue;
    bool m_MarkedForRemoval;
};

//------------------------------------------------------------------------
//
//  Class:  CMediaQueueItem
//
//  Synopsis:
//      An item in the queue list. This holds the state for a specific
//  event.
//
//------------------------------------------------------------------------
class CMediaQueueItem final : public CMediaListItemBase< CMediaQueueItem >, public IMediaQueueItem
{
public:
    CMediaQueueItem(IObject* pObject);
    ~CMediaQueueItem() override;

    IMediaQueueItem* GetPrevious() override;
    IMediaQueueItem* GetNext() override;

    IObject* GetState() override;

private:
    IObject* m_State;
};

//------------------------------------------------------------------------
//
//  Class:  CMQResetableTest
//
//  Synopsis:
//      A generic class that tests if an event should be added to the
//  event queue. Once the event is added the test fails until Reset
//  is called.
//
//------------------------------------------------------------------------
class CMQResetableTest : public IMediaQueueTest
{
public:
    CMQResetableTest();
    virtual ~CMQResetableTest() = default;

    void Reset();

    bool Test(IMediaQueueItem*& pHead, IMediaQueueItem*& pTail) override;

protected:
    wil::critical_section   m_Lock;
    bool                    m_fState;
};
