// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <UIThreadScheduler.h>

// REVIEW: still needed?
//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new media queue manager and initializes it.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaQueueManager::Create(_Outptr_ CMediaQueueManager** ppQueueManager)
{
    HRESULT hr = S_OK;
    CMediaQueueManager* pQueueManager = NULL;

    pQueueManager = new CMediaQueueManager();

    *ppQueueManager = pQueueManager;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the media queue manager with default values.
//
//------------------------------------------------------------------------
CMediaQueueManager::CMediaQueueManager()
    : m_pHead(NULL)
    , m_pTail(NULL)
    , m_fShutdown(FALSE)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Created a new media queue that is owned by the manager.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaQueueManager::CreateMediaQueue(
    _In_ IMediaQueueClient* pClient,
    _In_ CCoreServices *pCore,
    _Outptr_ CMediaQueue** ppQueue
    )
{

    IFCPTR_RETURN(pClient);
    IFCPTR_RETURN(ppQueue);

    xref_ptr<CMediaQueue> pQueue;
    IFC_RETURN(CMediaQueue::Create(this, pClient, pCore, pQueue.ReleaseAndGetAddressOf()));

    IFC_RETURN(AddMediaQueue(pQueue));

    *ppQueue = pQueue.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Processes all current events in all owned media queus. Optionally
//      shuts down the queues before flushing events.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaQueueManager::ProcessQueues(_In_ bool bIsShuttingDown)
{
    CMediaQueueManagerItem* pCurrent = NULL;
    CMediaQueueManagerItem* pNext = NULL;
    CMediaQueue* pQueue = NULL;

    if(bIsShuttingDown)
    {
        IFC_RETURN(Shutdown());
    }

    // Scope the lock
    {
        auto CritSec = m_Lock.lock();

        pCurrent = m_pHead;

        while(pCurrent != NULL)
        {
            pNext = pCurrent->Next;

            if(pCurrent->GetMarkedForRemoval())
            {
                if(pCurrent == m_pHead)
                {
                    m_pHead = pCurrent->Next;
                }

                if(pCurrent == m_pTail)
                {
                    m_pTail = pCurrent->Previous;
                }

                pCurrent->RemoveFromList();

                delete pCurrent;
            }
            else
            {
                pQueue = pCurrent->GetQueue();

                IFCPTR_RETURN(pQueue);

                IGNOREHR(pQueue->ProcessQueue());
            }

            pCurrent = pNext;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Shuts down all owned media queues.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaQueueManager::Shutdown()
{
    CMediaQueueManagerItem* pCurrent = NULL;

    // Scope the lock
    {
        auto CritSec = m_Lock.lock();

        m_fShutdown = TRUE;

        pCurrent = m_pHead;

        while(pCurrent != NULL)
        {
            CMediaQueue* pQueue = pCurrent->GetQueue();

            IFCPTR_RETURN(pQueue);

            IFC_RETURN(pQueue->Shutdown());

            pCurrent = pCurrent->Next;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a media queue to the owned media queue list.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaQueueManager::AddMediaQueue(_In_ CMediaQueue* pQueue)
{
    HRESULT hr = S_OK;
    CMediaQueueManagerItem* pItem = NULL;

    // Protect against bogus calls
    IFCPTR(pQueue);

    pItem = new CMediaQueueManagerItem(pQueue);

    // Scope the lock
    {
        auto CritSec = m_Lock.lock();

        IFCEXPECT(!m_fShutdown);

        pItem->InsertAfter(m_pTail);
        m_pTail = pItem;

        if(m_pHead == NULL)
        {
            m_pHead = pItem;
        }
    }

    pItem = NULL;

Cleanup:
    delete pItem;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes a media queue from the owned media queue list.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaQueueManager::RemoveMediaQueue(_In_ CMediaQueue* pQueue)
{
    HRESULT hr = S_OK;
    CMediaQueueManagerItem* pCurrent = NULL;

    // Scope the lock
    {
        auto CritSec = m_Lock.lock();

        pCurrent = m_pHead;

        while(pCurrent != NULL)
        {
            if(pCurrent->GetQueue() == pQueue)
            {
                pCurrent->MarkForRemoval();

                break;
            }

            pCurrent = pCurrent->Next;
        }
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates and initializes a new CMediaQueue object.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMediaQueue::Create(
    _In_ CMediaQueueManager* pManager,
    _In_ IMediaQueueClient* pClient,
    _In_ CCoreServices *pCore,
    _Outptr_ CMediaQueue** ppQueue
    )
{
    xref_ptr<CMediaQueue> pQueue;

    pQueue.attach(new CMediaQueue());

    IFC_RETURN(pQueue->Initialize(pManager, pClient, pCore));

    *ppQueue = pQueue.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the media queue with default values.
//
//------------------------------------------------------------------------
CMediaQueue::CMediaQueue()
    : m_pManager(NULL)
    , m_pClient(NULL)
    , m_pHead(NULL)
    , m_pTail(NULL)
    , m_fShutdown(FALSE)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Cleans up the media queue.
//
//------------------------------------------------------------------------
CMediaQueue::~CMediaQueue()
{
    ASSERT(m_pHead == NULL);
    ASSERT(m_pTail == NULL);

    ASSERT(m_fShutdown);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a MediaQueue object.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaQueue::Initialize(
    _In_ CMediaQueueManager* pManager,
    _In_ IMediaQueueClient* pClient,
    _In_ CCoreServices *pCore
    )
{
    IFCPTR_RETURN(pManager);
    IFCPTR_RETURN(pClient);

    m_pManager = pManager;
    m_pClient = pClient;
    XCP_WEAK(&m_pCoreNoRef);
    m_pCoreNoRef = pCore;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a MediaElement and event to the queue for processing on the
//      next Tick() call.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaQueue::AddMediaEvent(_In_opt_ IObject* pState, _In_opt_ IMediaQueueTest* pTest)
{
    HRESULT hr = S_OK;
    CMediaQueueItem* pItem = NULL;
    bool bInsert = true;

    pItem = new CMediaQueueItem(pState);

    // Scope the lock
    {
        auto CritSec = m_Lock.lock();

        IFCEXPECT(!m_fShutdown);

        if (pTest)
        {
            IMediaQueueItem* pHead = m_pHead;
            IMediaQueueItem* pTail = m_pTail;

            bInsert = pTest->Test(pHead, pTail);

            m_pHead = (CMediaQueueItem*)pHead;
            m_pTail = (CMediaQueueItem*)pTail;
        }

        if (bInsert)
        {
            pItem->InsertAfter(m_pTail);
            m_pTail = pItem;

            if(m_pHead == NULL)
            {
                m_pHead = pItem;
            }

            pItem = NULL;

            // Since the media queue is not empty, request a UI thread tick to process the queue.
            // The browser host and/or frame scheduler can be NULL during shutdown.
            IXcpBrowserHost *pBH = m_pCoreNoRef->GetBrowserHost();
            if (pBH != NULL)
            {
                ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
                if (pFrameScheduler != NULL)
                {
                    IFC(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::MediaQueue));
                }
            }
        }
    }

Cleanup:
    delete pItem;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Process all the queued media end requests and either rollover to the
//  next element or fire the MediaEnded event as appropriate.  Will also free
//  the queue.  Calls methods on the individual media elements to do the real
//  work.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMediaQueue::ProcessQueue()
{
    CMediaQueueItem* pCurrent = NULL;
    CMediaQueueItem* pNext = NULL;
    bool bShuttingDown = false;

    // Remove the chain from the queue.  This allows HandleCreateEvent to post a
    // request that can be processed on the next Tick.
    {
        auto CritSec = m_Lock.lock();

        pCurrent = m_pHead;
        m_pHead = NULL;
        m_pTail = NULL;
        bShuttingDown = m_fShutdown;
    }

    // Now walk the chain
    while(pCurrent != NULL)
    {
        pNext = pCurrent->Next;

        IGNOREHR(m_pClient->HandleMediaEvent(pCurrent->GetState(), bShuttingDown));

        delete pCurrent;

        pCurrent = pNext;
    }

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Shutdown the media queue and flush all pending events.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMediaQueue::Shutdown()
{
    bool bShouldShutdown = false;

    // Scope the lock.
    {
        auto CritSec = m_Lock.lock();

        if(!m_fShutdown)
        {
            m_fShutdown = TRUE;
            bShouldShutdown = TRUE;
        }
    }

    if(bShouldShutdown)
    {
        IFC_RETURN(ProcessQueue());

        if(m_pManager)
        {
            IFC_RETURN(m_pManager->RemoveMediaQueue(this));
            m_pManager = NULL;
        }

        // All events are now processed so clear the client
        // pointer.
        m_pClient = NULL;
    }

    return S_OK;
}

CMQResetableTest::CMQResetableTest() : m_fState(FALSE)
{
}

void CMQResetableTest::Reset()
{
    auto CritSec = m_Lock.lock();

    m_fState = FALSE;
}

bool CMQResetableTest::Test(IMediaQueueItem*& pHead, IMediaQueueItem*& pTail)
{
    auto CritSec = m_Lock.lock();

    if(!m_fState)
    {
        m_fState = TRUE;

        return true;
    }

    return false;
}

CMediaQueueManagerItem::CMediaQueueManagerItem(CMediaQueue* pQueue) : m_Queue(pQueue),
                                                                      m_MarkedForRemoval(FALSE)
{
    AddRefInterface(m_Queue);
}

CMediaQueueManagerItem::~CMediaQueueManagerItem()
{
    ReleaseInterface(m_Queue);
}

CMediaQueue* CMediaQueueManagerItem::GetQueue()
{
    return m_Queue;
}

VOID CMediaQueueManagerItem::MarkForRemoval()
{
    m_MarkedForRemoval = TRUE;
}

bool CMediaQueueManagerItem::GetMarkedForRemoval()
{
    return m_MarkedForRemoval;
}

CMediaQueueItem::CMediaQueueItem(IObject* pObject) : m_State(pObject)
{
    AddRefInterface(m_State);
}

CMediaQueueItem::~CMediaQueueItem()
{
    ReleaseInterface(m_State);
}

IObject* CMediaQueueItem::GetState()
{
    return m_State;
}

IMediaQueueItem* CMediaQueueItem::GetPrevious()
{
    return Previous;
}

IMediaQueueItem* CMediaQueueItem::GetNext()
{
    return Next;
}
