// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DependencyLocator.h>
#include <GCInstrumentationAggregator.h>
#include "UIAffinityReleaseQueue.h"
#include "MUX-ETWEvents.h"
#include "BuildTreeService.g.h"
#include "BudgetManager.g.h"

using namespace DirectUI;
using namespace Instrumentation;

UIAffinityReleaseQueue::~UIAffinityReleaseQueue()
{
    Cleanup( /* bSync */ TRUE );
}

HRESULT UIAffinityReleaseQueue::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ITreeBuilder)))
    {
        *ppObject = static_cast<ITreeBuilder*>(this);
    }
    else
    {
        RRETURN(ctl::WeakReferenceSourceNoThreadId::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// ITreeBuilder
// This is called by the BudgetManager when we've requested time.
IFACEMETHODIMP UIAffinityReleaseQueue::BuildTree(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN bCompletedCleanup = FALSE;

    // Clear out all of m_pQueuedObjectsForUnreachableCleanup and at least some
    // of m_pQueuedObjectsForFinalRelease
    IFC( DoCleanup( /*bSync*/ FALSE, &bCompletedCleanup ) );

    if( bCompletedCleanup )
    {
        // We cleaned up everything, we don't need more time.
        *returnValue = FALSE;
    }
    else
    {
        // We need more time to clean
        *returnValue = TRUE;
    }

Cleanup:
    RRETURN(hr);
}




HRESULT UIAffinityReleaseQueue::Cleanup( _In_ BOOLEAN bSync )
{
    HRESULT hr = S_OK;

    if( bSync || m_cDoCleanupAttempts >= BUDGET_MANAGER_DEFAULT_LIMIT )
    {
        // Call DoCleanup right now, we can't wait for future ticks.

        BOOLEAN bCompleted = FALSE;
        IFC( DoCleanup( /*bSync*/ TRUE, &bCompleted ));
        ASSERT( bCompleted );
    }

    else
    {
        ctl::ComPtr<BuildTreeService> spBuildTree;

        // Synchronize the UI Thread and the Finalizer thread while we check the queue
        auto lock = m_CriticalSection.lock();

        // If we haven't already registered for cleanup time, and we have something to clean up, register.
        if( !m_bIsRegisteredForCallbacks
            && ( !m_queuedObjectsForUnreachableCleanup.empty()
                 || !m_queuedObjectsForFinalRelease.empty() ))
        {
            IFC(DXamlCore::GetCurrent()->GetBuildTreeService(spBuildTree));
            IFC(spBuildTree->RegisterWork(this));
        }
    }

Cleanup:

    return hr;

}


HRESULT UIAffinityReleaseQueue::DoCleanup( _In_ BOOLEAN bSync, _Out_ BOOLEAN *completed )
{
    HRESULT hr = S_OK;
    TraceReleaseQueueCleanupBegin();
    //
    // This lock synchronizes the UI Thread and the Finalizer Thread
    //
    auto lock = m_CriticalSection.lock();

    // Don't re-enter
    if( m_bInCleanup )
    {
        *completed = TRUE;
        return S_OK;
    }

    //Telemetry Notify ,UIFinalizer started
    const auto telemetryAggregator = Instrumentation::GetTelemetryAggregator();
    telemetryAggregator->SignalEvent(GCEventSignalType::AcquireUIFinalizerLock);

    *completed = FALSE;
    m_bInCleanup = TRUE;

    m_cDoCleanupAttempts++;

    std::vector<ObjectReleaseInfo>::const_iterator iter;
    std::vector<ObjectReleaseInfo>::const_iterator firstRelease;
    INT timeElapsedInMS = 0;
    ctl::ComPtr<BudgetManager> spBudget;

    //
    // UI+Finalizer Lock inplace.
    //

    // Only take a core-lock if there are objects to cleanup. This way we can block a GC from queueing
    // more objects.
    if (!m_queuedObjectsForUnreachableCleanup.empty())
    {
        //
        // UI+GC+Finalizer Lock inplace.
        //

        //
        // Get a core lock
        // This synchronizes the UI Thread and the GC Thread
        //
        // Transfer out all the unreachable objects into the final release queue,
        // free up the GC lock at the end of this transfer to let GC proceed.
        //
        AutoReentrantReferenceLock coreLock( DXamlCore::GetCurrent() );

        if (!m_queuedObjectsForUnreachableCleanup.empty())
        {
            for(iter = m_queuedObjectsForUnreachableCleanup.begin(); iter != m_queuedObjectsForUnreachableCleanup.end(); iter++)
            {
                // Sometimes a GC will add an item to the unreachable queue even though the item was already
                // in the middle of a final release on the UI thread. If the item has already been deleted,
                // we're going to AV when we call the final release coming up. Here we check IsInFinalRelease,
                // because if it's true, we assume the item was either already released or already added to the
                // final release queue, in which case we should just silently ignore it here, rather than copy it
                // and let it crash. Note that if the memory has already been freed, we'll still get an AV here.
                // See bugs 20395531, 13108159
                if (!iter->m_ptr->IsInFinalRelease())
                {
                    m_queuedObjectsForFinalRelease.push_back(*iter);
                }
            }
            m_queuedObjectsForUnreachableCleanup.clear();
        }
    }


    // Process objects for final release.

    *completed = TRUE;
    firstRelease = m_queuedObjectsForFinalRelease.begin();
    for(iter = firstRelease; iter != m_queuedObjectsForFinalRelease.end(); iter++)
    {
        m_cleanupCount++;

        ctl::WeakReferenceSourceNoThreadId *pItem = iter->m_ptr;

        // Allow cycles to be broken
        pItem->Deinitialize();

        // Make sure that this upcoming final release is allowed
        pItem->DisableIgnoreReleases();

        // An implicit AddRef is done inside the tracker extension or, if none exists,
        // inside ComBase when the last reference is released.
        ctl::release_interface_inner(pItem);

        // If we've spent too much time during this tick cleaning out this queue, and we haven't deferred this cleanup
        // too many times, defer until the next tick.
        if( !bSync )
        {
            // Don't create the budgent manager outside of the async case, or it leaks during shutdown.
            if( !spBudget )
            {
                IFC(DXamlCore::GetCurrent()->GetBudgetManager(spBudget));
            }

            hr = spBudget->GetElapsedMilliSecondsSinceLastUITick(&timeElapsedInMS);
            if( FAILED(hr) )
            {
                // Erase the entries that were processed
                 m_queuedObjectsForFinalRelease.erase( firstRelease, ++iter );
                *completed = FALSE;
                IFC( hr );
                ASSERT( FALSE );
            }

            if( timeElapsedInMS > BUDGET_MANAGER_DEFAULT_LIMIT // Don't go more than 40 ms
                && m_cDoCleanupAttempts < BUDGET_MANAGER_DEFAULT_LIMIT  ) // Don't defer more than 40 times
            {
                // Advance the iterator, because we're going to erase(), and erase doesn't
                // erase the end point
                iter++;

                *completed = FALSE;
                break;
            }
        }
    }

    // If we iterated all the way to the end, this will actually call clear()
     m_queuedObjectsForFinalRelease.erase( firstRelease, iter );

    TraceReferenceTrackingCleanupInfo (m_cleanupCount, m_cDoCleanupAttempts, *completed);

    if( *completed )
    {
        m_cDoCleanupAttempts = 0;
        m_cleanupCount = 0;

#if DBG
        LOG(L"Reference tracking release queue emptied");
#if XCP_MONITOR
        if (m_queuedObjectsForFinalRelease.empty())
        {
            m_queuedObjectsForFinalRelease.shrink_to_fit();
        }
#endif
#endif

    }

    m_bInCleanup = FALSE;

Cleanup:
    TraceReleaseQueueCleanupEnd();

    //Telemetry Notify ,UIFinalizer ended
    telemetryAggregator->SignalEvent(GCEventSignalType::ReleaseUIFinalizerLock);

#if XCP_MONITOR
    // Release the dependency after it has been used so it isn't considered leaked
    DependencyLocator::Internal::ReleaseInstance<Instrumentation::CTelemetryAggregatorGC>();
#endif
    return hr;
}


// ITreeBuilder
IFACEMETHODIMP UIAffinityReleaseQueue::ShutDownDeferredWork()
{
    // We don't need to do anything here; DXamlCore already cleans up the queues.
    return S_OK;
}

// ITreeBuilder
_Check_return_ HRESULT DirectUI::UIAffinityReleaseQueue::get_IsRegisteredForCallbacks(_Out_ BOOLEAN* pValue)
{
    *pValue = m_bIsRegisteredForCallbacks;
    return S_OK;
}


// ITreeBuilder
_Check_return_ HRESULT DirectUI::UIAffinityReleaseQueue::put_IsRegisteredForCallbacks(_In_ BOOLEAN value)
{
    m_bIsRegisteredForCallbacks = value;
    return S_OK;
}

BOOLEAN DirectUI::UIAffinityReleaseQueue::IsEmpty(){
    return m_queuedObjectsForFinalRelease.empty() && m_queuedObjectsForUnreachableCleanup.empty();
}

HRESULT DirectUI::UIAffinityReleaseQueue::GetQueueObjects(wfc::IVector<IInspectable*>* queue)
{
    auto lock = m_CriticalSection.lock();

    for(auto& obj : m_queuedObjectsForFinalRelease)
    {
        wrl::ComPtr<IInspectable> inspectable;
        obj.m_ptr->QueryInterface(__uuidof(IInspectable), &inspectable);
        IFC_RETURN(queue->InsertAt(0, inspectable.Get()));
    }
    for(auto& obj : m_queuedObjectsForUnreachableCleanup)
    {
        wrl::ComPtr<IInspectable> inspectable;
        obj.m_ptr->QueryInterface(__uuidof(IInspectable), &inspectable);
        IFC_RETURN(queue->InsertAt(0, inspectable.Get()));
    }

    return S_OK;
}

