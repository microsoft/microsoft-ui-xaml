// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#ifdef DBG
// Count all instances of work items for debugging purposes
// Used to track that cleanup and shutdown works
#define INCWORKITEMCOUNTER ::InterlockedIncrement(&(m_pFactory->m_dwWorkItemCounter));
#define DECWORKITEMCOUNTER ::InterlockedDecrement(&(m_pFactory->m_dwWorkItemCounter));
#define ASSERTNOWORKLEFT ASSERT(m_dwWorkItemCounter == 0)
#else
#define INCWORKITEMCOUNTER
#define DECWORKITEMCOUNTER
#define ASSERTNOWORKLEFT
#endif


// NOTE:  According to Win32 docs, most of the threadpool functions return no values or errors
// As a result, most calls are not checked (there is nothing to check)

// ----------------------------------------------------------------------
//
// Represents an item of work in a windows threadpool
// Class implementation entirely hidden as exposed only through the factory
//
//----------------------------------------------------------------------
class CWinWorkItem : public CXcpObjectBase<IPALWorkItem>
{
protected:
    CWinWorkItem(_In_ CWinWorkItemFactory *pFactory)
    {
        m_pfn = NULL;
        m_pData = NULL;
        m_pThreadpoolWork = NULL;
        #ifdef DBG
        m_pFactory = pFactory;
        #endif
        INCWORKITEMCOUNTER;
    }
    ~CWinWorkItem() override
    {
        IFCFAILFAST(Close());
        ReleaseInterface(m_pData);
        DECWORKITEMCOUNTER;
    }
public:
    static _Check_return_ HRESULT Create(_Outptr_ CWinWorkItem **pNewItem,
    _In_ PALWORKITEMCALLBACK pfn,
    _In_opt_ IObject *pData,
    _In_ PTP_CALLBACK_ENVIRON threadPoolEnvironment,
    _In_ CWinWorkItemFactory* pFactory);


    // Submit the work to the threadpool.
    _Check_return_ HRESULT Submit() override
    {
        HRESULT hr = S_OK;

        IFCPTR(m_pThreadpoolWork);
        IFCPTR(m_pfn);

        // We are submitting ourselves to a queue in the thread pool
        // this AddRef represents that queue holding on to us
        // The corresponding ReleaseInteface will occur after the work is
        // executed or cancelled
        AddRefInterface(this);
        ::SubmitThreadpoolWork(m_pThreadpoolWork);
    Cleanup:
        RRETURN(hr);
    }

    // Close the work item.  May be called multiple times.
    _Check_return_ HRESULT Close()
    {
        HRESULT hr = S_OK;


        ReleaseInterface(m_pData);

        // The work may have already been closed by the system during
        // cancel
        if (m_pThreadpoolWork != NULL)
        {
            ASSERT(m_pfn);

            ::CloseThreadpoolWork(m_pThreadpoolWork);

            m_pThreadpoolWork = NULL;
            m_pfn = NULL;
        }

        RRETURN(hr);
    }

    // Wait for the item to complete or be cancelled.  If cancelingPending is true
    // will cancel items in the queue before they are executed.
    _Check_return_ HRESULT Wait(bool cancelPending) override
    {
        HRESULT hr = S_OK;

        IFCPTR(m_pThreadpoolWork);
        IFCPTR(m_pfn);
        ::WaitForThreadpoolWorkCallbacks(m_pThreadpoolWork, cancelPending);
    Cleanup:
        RRETURN(hr);
    }

    // Get the data held by the work
    _Check_return_ IObject *GetDataNoRef() override
    {
        return m_pData;
    }

private:
    PALWORKITEMCALLBACK m_pfn;
    IObject *m_pData;
    TP_WORK *m_pThreadpoolWork;
    #ifdef DBG
    CWinWorkItemFactory *m_pFactory;
    #endif

private:
    // Execute the work.  Called on the threadpool thread
    void Execute()
    {
        (*m_pfn)(m_pData);
    }

    // Callback that matches the signature required by the Windows threadpool
    // Just a stub to convert the threadpool callback signature in to a call to our item class
    static VOID CALLBACK
    WorkCallback(_Inout_ PTP_CALLBACK_INSTANCE Instance,
                    _Inout_opt_ PVOID pContext,
                    _Inout_     PTP_WORK Work)
    {
        if (pContext == NULL)
        {
            ASSERT(FALSE);
            return;
        }
        CWinWorkItem* pItem = static_cast<CWinWorkItem*>(pContext);

        pItem->Execute();

        // Matches the AddRefInteface done in Submit
        ReleaseInterface(pItem);
    }

public:
    // Stub that matches the callback signature required by the Windows threadpool
    // Will be called back during CancelAndCleanupAllWork
    static VOID CALLBACK
        CleanupGroupCancelCallback(_Inout_opt_ PVOID pObjectContext, _Inout_opt_ PVOID pCleanupContext)
    {
        if (pObjectContext != NULL)
        {
            CWinWorkItem *pWorkItem = reinterpret_cast<CWinWorkItem *>(pObjectContext);

            // When cancel happens, the system cleans up the work, so we shouldn't call ::CloseThreadpoolWork
            // but we still need to delete our objects.  Setting the work to NULL prevents us Close it in the dtor
            pWorkItem->m_pThreadpoolWork = NULL;
            ReleaseInterface(pWorkItem);
        }
    }

};

//---------------------------------------------------------------------------
//
// Creates a work item
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CWinWorkItem::Create(_Outptr_ CWinWorkItem **pNewItem,
    _In_ PALWORKITEMCALLBACK pfn,
    _In_opt_ IObject *pData,
    _In_ PTP_CALLBACK_ENVIRON threadPoolEnvironment,
    _In_ CWinWorkItemFactory *pFactory)

{
    HRESULT hr = S_OK;
    CWinWorkItem *pItem = NULL;

    // The return value and the callback function are not optional
    // data is optional
    if ((pNewItem == NULL) || (pfn == NULL))
        IFC(E_INVALIDARG);

    pItem = new CWinWorkItem(pFactory);
    pItem->m_pfn = pfn;
    pItem->m_pData = pData;
    AddRefInterface(pItem->m_pData);

    pItem->m_pThreadpoolWork = ::CreateThreadpoolWork( &CWinWorkItem::WorkCallback, pItem, threadPoolEnvironment);
    IFCW32(pItem->m_pThreadpoolWork); // CreateThreadpoolWork should set LastError on failure

    *pNewItem = pItem;
    pItem = NULL;

Cleanup:
    ReleaseInterface(pItem);
    RRETURN(hr);
}


//--------------------------------------------------------
//
// Class: CWinWorkItemFactory
//
//--------------------------------------------------------
CWinWorkItemFactory::CWinWorkItemFactory()
{
    m_pCleanupGroup = NULL;
    #ifdef DBG
    m_dwWorkItemCounter = 0;
    #endif
}

CWinWorkItemFactory::~CWinWorkItemFactory()
{
    ASSERTNOWORKLEFT;
    if (m_pCleanupGroup != NULL)
    {
        ::CloseThreadpoolCleanupGroup(m_pCleanupGroup);
        m_pCleanupGroup = NULL;
    }
}



// ---------------------------------------------------
//
// Cleans up all the work items associated with this
// factory, waiting for those already running and cancelling
// those that are in the queue but haven't run.  Does not destroy
// the factory, which can have new work queued to it
//
// ----------------------------------------------------
HRESULT
CWinWorkItemFactory::CancelAndCleanupAllWork()
{
    if (m_pCleanupGroup != NULL)
    {
        ::CloseThreadpoolCleanupGroupMembers(m_pCleanupGroup, TRUE, NULL);
    }
    // After cleanup there should be no works left
    ASSERTNOWORKLEFT;

    RRETURN(S_OK);
}

//--------------------------------------------------------
//
// Sets up the threadpool environment and cleanup group
//
//--------------------------------------------------------
HRESULT
CWinWorkItemFactory::Initialize()
{
    HRESULT hr = S_OK;

    // According to the docs, InitializeThreadpoolEnvironment cannot fail
    ::InitializeThreadpoolEnvironment(&m_threadPoolEnvironment);
    m_pCleanupGroup = ::CreateThreadpoolCleanupGroup();
    IFCW32(m_pCleanupGroup); // CreateThreadpoolCleanupGroup should set LastError on failure

    // Associate our thread pool environment with the default global thread pool
    // According to docs, cannot fail
    ::SetThreadpoolCallbackPool(&m_threadPoolEnvironment, NULL);
    ::SetThreadpoolCallbackCleanupGroup(&m_threadPoolEnvironment, m_pCleanupGroup, &CWinWorkItem::CleanupGroupCancelCallback);


Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------
//
// Creates actual work
//
//---------------------------------------------------------
_Check_return_ HRESULT
CWinWorkItemFactory::CreateWorkItem(
    _Outptr_ IPALWorkItem **pNewItem,
    _In_ PALWORKITEMCALLBACK pfn,
    _In_opt_ IObject *pData)

{
    HRESULT hr = S_OK;

    CWinWorkItem *pWinWorkItem = NULL;
    IFC(CWinWorkItem::Create(&pWinWorkItem, pfn, pData, &m_threadPoolEnvironment, this));
    IFCPTR(pWinWorkItem);

    *pNewItem = pWinWorkItem;

Cleanup:
    RRETURN(hr);
}
