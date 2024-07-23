// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ReferenceTrackerManager.h"
#include <DependencyLocator.h>
#include <GCInstrumentationAggregator.h>
#include <AutoReentrantReferenceLock.h>
#include <CStaticLock.h>
#include "LifetimeExterns.h"
#include "TrackerTargetReference.h"
#include "DependencyObjectAbstractionHelpers.h"
#include "MUX-ETWEvents.h"

using namespace DirectUI;
using namespace Instrumentation;
using namespace xaml_hosting;

ReferenceTrackerManager* ReferenceTrackerManager::This = NULL;
SRWLOCK ReferenceTrackerManager::s_lock {SRWLOCK_INIT};

#if XCP_MONITOR
int ReferenceTrackerManager::s_cPeerStressIteration = 0;
int ReferenceTrackerManager::s_cMaxPeerStressIterations = 0;

bool ReferenceTrackerManager::s_enableBackgroundGC = false;
#endif




//+----------------------------------------------------------------------------------
//
//  Helpers for Custom Iterator of Peer Table
//
//+----------------------------------------------------------------------------------

PeerMapEntriesHelper::Iterator::Iterator(_In_ IDXamlCore *pCore, _In_ bool fEnd)
{
    // The IDXamlCore PeerMapEntry starts with the Window (if present) and
    // then iterates over the PeerMapTable
    XCP_WEAK(&m_pCore);
    m_pCore = pCore;
    if (!fEnd)
    {
        m_PeersIterator = m_pCore->GetPeers().begin();
        m_ReferenceTrackersIterator = m_pCore->GetReferenceTrackers().begin();
    }
    else
    {
        m_PeersIterator = m_pCore->GetPeers().end();
        m_ReferenceTrackersIterator = m_pCore->GetReferenceTrackers().end();
    }
}

PeerMapEntriesHelper::Iterator& PeerMapEntriesHelper::Iterator::operator++()
{
    if (m_PeersIterator != m_pCore->GetPeers().end())
    {
        ++m_PeersIterator;
    }
    else if (m_ReferenceTrackersIterator != m_pCore->GetReferenceTrackers().end())
    {
        ++m_ReferenceTrackersIterator;
    }
    return *this;
}


xaml_hosting::IReferenceTrackerInternal* PeerMapEntriesHelper::Iterator::operator*()
{
    if (m_PeersIterator != m_pCore->GetPeers().end())
    {
        return DependencyObjectAbstractionHelpers::DOtoIRTI(*m_PeersIterator);
    }
    else
    {
        return *m_ReferenceTrackersIterator;
    }
}

//static
IDXamlCore* ReferenceTrackerManager::SearchCoresForObject(_In_ xaml_hosting::IReferenceTrackerInternal *pObject)
{
    if (This)
    {
        auto lock = wil::AcquireSRWLockShared(&s_lock);

        for(auto core_iterator = This->m_allCores.begin(); core_iterator != This->m_allCores.end(); ++core_iterator)
        {
            IDXamlCore* pCore = *core_iterator;
            auto coreLock = wil::AcquireSRWLockShared(&pCore->GetReferenceSrwLock());

            auto iterator = pCore->GetReferenceTrackers().find(pObject);
            if (iterator != pCore->GetReferenceTrackers().end())
            {
                return pCore;
            }
        }
    }

    return NULL;
}

//+----------------------------------------------------------------------------------
//
//  EnsureInitialized
//
//  Create the ReferenceTrackerManager
//
//+----------------------------------------------------------------------------------

//static
_Check_return_
HRESULT
ReferenceTrackerManager::EnsureInitialized(_Outptr_opt_ ReferenceTrackerManager** referenceTrackerManager)
{
    wil::assign_null_to_opt_param(referenceTrackerManager);

    ReferenceTrackerManager *newInstance {nullptr};

    {
        auto lock = wil::AcquireSRWLockExclusive(&s_lock);

        if (This == nullptr)
        {
            newInstance = new ReferenceTrackerManager();

            // Initialize the event used to synchronize when GC starts on a UI thread.  When we start a GC, we'll reset it.
            auto  eventOptions = wil::EventOptions::ManualReset | wil::EventOptions::Signaled;
            IFC_RETURN(newInstance->m_runningOnUIThreadEvent.create( eventOptions ));

            This = newInstance;
        }

        if (referenceTrackerManager)
        {
            SetInterface(*referenceTrackerManager, This);
        }
    }

    if (newInstance)
    {
        #if DBG
        // Initialize logging
        {
            LPCWSTR wszXamlDebugKeyName = XAML_ROOT_KEY L"\\Debug";
            DWORD keyValue = 0;
            DWORD size = sizeof(DWORD);
            HRESULT hrRegGetValue = HRESULT_FROM_WIN32(::RegGetValue(HKEY_LOCAL_MACHINE, wszXamlDebugKeyName, L"ReferenceTrackerLog", RRF_RT_REG_DWORD, NULL, &keyValue, &size));

            if (SUCCEEDED(hrRegGetValue))
            {
                if(keyValue == 1)
                {
                    TRACE(TraceAlways, L"ReferenceTracker logging enabled");

                    _logItems = new ReferenceTrackerLogItem[_maxLogItemsEntries];
                }
            }
        }
        #endif

        #if XCP_MONITOR
        GetPALDebuggingServices()->GetTrackerStressFromEnvironment( &s_cMaxPeerStressIterations, &s_cPeerStressIteration, &s_enableBackgroundGC );

        newInstance->m_bIsReferenceTrackingActive = FALSE;

        if (s_enableBackgroundGC)
        {
            IFC_RETURN(StartBackgroundGC());
        }
        #endif
    }

    return S_OK;

}



//+----------------------------------------------------------------------------------
//
//  RegisterCore/UnregisterCore
//
//  Register a IDXamlCore with the ReferenceTrackerManager
//
//+----------------------------------------------------------------------------------


//static
_Check_return_
HRESULT
ReferenceTrackerManager::RegisterCore( _In_ IDXamlCore *pCore )
{
    auto lock {wil::AcquireSRWLockExclusive(&s_lock)};

    This->m_allCores.push_back( pCore );
    This->m_activeCores.push_back( pCore );
    This->AddRef();

    return S_OK;
}

//static
_Check_return_
HRESULT
ReferenceTrackerManager::DisableCore( _In_ IDXamlCore *pCore )
{
    // Create a nested scope for the lock, so that the lock wrapper can destruct before the release.
    {
        auto lock = wil::AcquireSRWLockExclusive(&s_lock);
        ASSERT( !This->m_activeCores.empty() );
        AssertActive(FALSE);

        This->m_activeCores.remove(pCore);
    }

    return S_OK;    //RRETURN_REMOVAL
}


//static
_Check_return_
HRESULT
ReferenceTrackerManager::UnregisterCore( _In_ IDXamlCore *pCore )
{
    // Create a nested scope for the lock, so that the lock wrapper can destruct before the release.
    {
        auto lock = wil::AcquireSRWLockExclusive(&s_lock);
        ASSERT( !This->m_allCores.empty() );
        AssertActive(FALSE);

        This->m_activeCores.remove(pCore);
        This->m_allCores.remove(pCore);
    }

    This->Release();    //RRETURN_REMOVAL

    return S_OK;
}


//static
_Check_return_
bool
ReferenceTrackerManager::IsCoreRegistered(_In_ IDXamlCore* pCore )
{
    auto lock = wil::AcquireSRWLockExclusive(&s_lock);    

    ASSERT(This != NULL);
    return (std::find(This->m_allCores.begin(), This->m_allCores.end(), pCore) != This->m_allCores.end());
}




//+--------------------------------------------------------------------
//
//  OnShutdownAllPeers
//
//  This is called during shutdown, in IDXamlCore::ShutdownAllPeers.  We call to the
//  reference tracking sources and tell them to disconnect.  Otherwise, they may not
//  release their reference trackers until this thread is gone (so they'll do the release
//  on the finalizer thread).
//
//+--------------------------------------------------------------------

//static
void
ReferenceTrackerManager::OnShutdownAllPeers( )
{
#if XCP_MONITOR
    This->m_bIsReferenceTrackingActive = TRUE;
#endif

    if( This->_pReferenceTrackerHost != NULL )
    {
        VERIFYHR( This->_pReferenceTrackerHost->NotifyEndOfReferenceTrackingOnThread() );
    }
}


//+--------------------------------------------------------------------
//
//  TriggerCollection
//
//  This is called by the core when a GC collection is recommended.
//
//+--------------------------------------------------------------------

//static
HRESULT
ReferenceTrackerManager::TriggerCollection( )
{
    // Forward the recommendation
    if( This && This->_pReferenceTrackerHost != NULL )
    {
#if XCP_MONITOR
        if (This->m_bIsReferenceTrackingActive)
        {
            return S_OK;
        }
#endif

        IFC_RETURN( This->_pReferenceTrackerHost->DisconnectUnusedReferenceSources( XAML_REFERENCETRACKER_DISCONNECT_DEFAULT ));
    }



    return S_OK;

}

//+--------------------------------------------------------------------
//
//  TriggerFinalization
//
//  This is called by the core when a GC finalization is recommended.
//
//+--------------------------------------------------------------------
//static
HRESULT
ReferenceTrackerManager::TriggerFinalization( )
{
    // Forward the recommendation
    if( This && This->_pReferenceTrackerHost != NULL )
    {
#if XCP_MONITOR
        if (This->m_bIsReferenceTrackingActive)
        {
            return S_OK;
        }
#endif

        IFC_RETURN( This->_pReferenceTrackerHost->ReleaseDisconnectedReferenceSources() );
    }


    return S_OK;

}


//+--------------------------------------------------------------------
//
//  TriggerCollectionForSuspend
//
//  Called during Suspend to trigger a GC.
//
//+--------------------------------------------------------------------
// static
HRESULT
ReferenceTrackerManager::TriggerCollectionForSuspend( )
{
    if( This->_pReferenceTrackerHost != NULL )
    {
#if XCP_MONITOR
        if (This->m_bIsReferenceTrackingActive)
        return S_OK;
#endif

        IFC_RETURN( This->_pReferenceTrackerHost->DisconnectUnusedReferenceSources( XAML_REFERENCETRACKER_DISCONNECT_SUSPEND ) );
    }


    return S_OK;

}

//+-------------------------------------- ------------------------------
//
//  ReferenceTrackingStarted
//
//  This is called before the calls to IReferenceTracker::FindTrackerTarget
//
//+--------------------------------------------------------------------

// Trace counters
int ReferenceTrackerManager::_peerCount = 0;
int ReferenceTrackerManager::_targetCount = 0;
int ReferenceTrackerManager::_unreachableCount = 0;

_Check_return_
IFACEMETHODIMP
ReferenceTrackerManager::ReferenceTrackingStarted()
{
    TraceReferenceTrackingStartedBegin();
    const auto telemetryAggregator = GetTelemetryAggregator();
    //Telemetry Notify ,GC tree walk started
    telemetryAggregator->SignalEvent(GCEventSignalType::AcquireTreeWalkLock);

    // Lock all of the reference-tracking structures across all cores.

    AcquireSRWLockExclusive( &s_lock );

    // If the current thread is a UI thread, the above m_lock won't help, because you can't block on an SWR
    // lock that was acquired on the same thread.  For such cases, wait on this event.  We save the current
    // thread ID so that we can recognize this case (in AutoReentrantReferenceLock).

    m_runningOnUIThreadEvent.ResetEvent();
    ASSERT( m_startThreadId == 0 );
    m_startThreadId = ::GetCurrentThreadId();


    _peerCount = 0;
    _targetCount = 0;
    _unreachableCount = 0;


    #if XCP_MONITOR
    m_bIsReferenceTrackingActive = TRUE;
    #endif


    #if DBG
    ReferenceTrackerLogTracking::Log( ReferenceTrackerLogTrackingPhase::Start );

    #if XCP_MONITOR
    if( s_cMaxPeerStressIterations == 0 && !m_backgroundGCEnabled)
    #else
    if( !m_backgroundGCEnabled)
    #endif
    {
        LOG(L"Reference tracking starting");
    }
    #endif

    // Walk the cores

    for(auto core_iterator = m_activeCores.begin(); core_iterator != m_activeCores.end(); ++core_iterator)
    {
        IDXamlCore* pCore = *core_iterator;

        // Lock this core and its peer table, so that it's not changing any of
        // the references we're about to look at.
        // Use an exclusive lock so that no app threads are modifying references.

        AcquireSRWLockExclusive(&pCore->GetReferenceSrwLock());

        ASSERT(!pCore->IsShutdown());

        PeerMapEntriesHelper peerMap(pCore);

        // Unpeg all tracker targets

        for (auto it = peerMap.begin(); it != peerMap.end(); ++it)
        {
            xaml_hosting::IReferenceTrackerInternal* pObject = *it;
            ASSERT( pObject != NULL );

            pObject->ReferenceTrackerWalk( RTW_Unpeg, /* fIsRoot */ TRUE );

            // Clear the 'reachable' flag.  We'll set it again, as appropriate, in
            // OnReferenceTrackingProcessed.  We can't clear it earlier than here,
            // because the previous value was being used during the previous RTW_Unpeg walk.
            // This also clears the 'bPegWalked' flag that's used later in the RTW_Peg walks.
            pObject->PrepareForReferenceWalking();

            // For an ETW trace, calculate how many peers we have.
            _peerCount++;
        }

        // Peg tracker targets that are reachable from a pegged peer
        // We do this now so that during the tracker walks (calls to
        // DependencyObject::FindTrackerTargets), we can prune the tree walks
        // on an object if this pegging already walked through it.

        for (auto it = peerMap.begin(); it != peerMap.end(); ++it)
        {
            xaml_hosting::IReferenceTrackerInternal *pObject = *it;
            ASSERT( pObject != NULL );

            pObject->ReferenceTrackerWalk( RTW_Peg, /* fIsRoot */ TRUE );
        }

        // Walk the core roots too.  We didn't need to walk them for the Unpeg walk, because
        // they don't do anything special in that case.  But we need it in the Peg walk, because
        // the Peg walk only continues if the root of the walk is pegged.
        pCore->ReferenceTrackerWalkOnCoreGCRoots(RTW_Peg);
    }

    // The counter starts at 0, but it'll be incremented to 1 when we start our first
    // find walk, in SetRootOfTrackerWalk with walkType==RTW_Find
    m_currentFindWalkID = 0;
    TraceReferenceTrackingStartedEnd();

    RRETURN(S_OK);

}

//
// When the find walk counter m_currentFindWalkID wraps back to zero, we need
// to reset the last find walk ID on every peer so we can re-use find walk IDs
// we've already used.
//
void
ReferenceTrackerManager::ResetLastFindWalkIdForAllPeers()
{
#if DBG
    LOG(L"Reference tracking called ResetLastFindWalkIdForAllPeers");
#endif

    for (auto core_iterator = m_activeCores.begin(); core_iterator != m_activeCores.end(); ++core_iterator)
    {
        IDXamlCore* pCore = *core_iterator;
        PeerMapEntriesHelper peerMap(pCore);

#if DBG
        // Make sure we already have the SRW lock (acquired in ReferenceTrackingStarted)
        // SRW locks are not recursive, TryAcquireSRWLockExclusive will fail if we
        // already have access.
        ASSERT(TryAcquireSRWLockExclusive(&pCore->GetReferenceSrwLock()) == FALSE);
#endif
        for (auto it = peerMap.begin(); it != peerMap.end(); ++it)
        {
            (*it)->ResetLastFindWalkId();
        }
    }
}

//+--------------------------------------------------------------------
//
//  FindTrackerTargetsCompleted
//
//  This is called after the calls to IReferenceTracker::FindTrackerTarget.
//
//  Note:  This marks the end of the foreground GC.  At this point background
//  GC will run on a separate thread.
//
//+--------------------------------------------------------------------

_Check_return_
IFACEMETHODIMP
ReferenceTrackerManager::FindTrackerTargetsCompleted( _In_ BOOLEAN findFailed )
{
    //
    // Walk the cores and peg CCWs of DO's which are reachable from an RCW,
    // but have not yet been RCW walked.
    //
    // This happens in a few cases in stress scenarios where we don't see
    // the RCW walk even though we see a ConnectFromTrackerSource.
    // In order to protect the CCWs between the period that a Connect
    // and a FindTrackerTarget call happens, we need to temporarily peg
    // these CCWs.
    //

    for(auto core_iterator = m_activeCores.begin(); core_iterator != m_activeCores.end(); ++core_iterator)
    {
        IDXamlCore* pCore = *core_iterator;

        ASSERT(!pCore->IsShutdown());

        PeerMapEntriesHelper peerMap(pCore);

        // Walk the tree from any object that is either pegged or directly referenced
        // by a tracker source.

        for (auto it = peerMap.begin(); it != peerMap.end(); ++it)
        {
            xaml_hosting::IReferenceTrackerInternal *pObject = *it;
            ASSERT( pObject != NULL );

            //
            // RCW references this DO, but no RCW walk has occurred.
            //
            if (pObject->IsReferencedByTrackerSource() &&
                !pObject->HasBeenWalked(DirectUI::EReferenceTrackerWalkType::RTW_Find))
            {
                if (!pObject->HasBeenWalked(DirectUI::EReferenceTrackerWalkType::RTW_Peg))
                {
                    pObject->ReferenceTrackerWalk( RTW_Peg, /* fIsRoot */ TRUE );
                }
            }
        }

    }

    RRETURN(S_OK);
}



//+--------------------------------------------------------------------
//
//  ReferenceTrackingCompleted
//
//  This is called after the calls to IReferenceTracker::FindTrackerTarget, and after
//  some post-processing.  At this point if a tracker source still exists, it won't
//  go away.  We use this notification to clear weak references for any non-reachable
//  object.
//
//  Note:  This method can be called on a different thread than that of the
//  OnReferenceTrackingProcessed call.
//
//+--------------------------------------------------------------------

_Check_return_
IFACEMETHODIMP
ReferenceTrackerManager::ReferenceTrackingCompleted()
{
    TraceReferenceTrackingCompletedBegin();

    if( This->_pReferenceTrackerHost != NULL )
    {
        // Walk the cores

        for(auto core_iterator = m_activeCores.begin(); core_iterator != m_activeCores.end(); ++core_iterator)
        {
            IDXamlCore* pCore = *core_iterator;

            ASSERT(!pCore->IsShutdown());

            PeerMapEntriesHelper peerMap(pCore);

            // Walk the tree from any object that is either pegged or directly referenced
            // by a tracker source.

            for (auto it = peerMap.begin(); it != peerMap.end(); ++it)
            {
                xaml_hosting::IReferenceTrackerInternal *pObject = *it;
                ASSERT( pObject != NULL );

                // If this is a pegged object, or referenced by a tracker source,
                // walk it to flag objects that are reachable from it.    This is optimized so that
                // it's linear and only visits each object once.
                // To do: collapsed all these IsPegged into one call
                if( pObject->IsPegged(false /*isRefCountPegged*/) || pObject->IsReferencedByTrackerSource() || pObject->IsReferenceTrackerPegSet() || pObject->IsPegged(true /*isRefCountPegged*/))
                {
                    if (!pObject->IsReachable())
                    {
                        pObject->ReferenceTrackerWalk( RTW_Reachable, /* fIsRoot */ TRUE );
                    }
                }
           }

            // Walk the core roots too.  We didn't need to walk them for the Unpeg walk, because
            // they don't do anything special in that case.  But we need it in the Reachable walk, because
            // the above doesn't take into account that an object might be reachable from the core roots.
            pCore->ReferenceTrackerWalkOnCoreGCRoots(RTW_Reachable);

            // For any object that isn't reachable by a pegged object or by a tracker source,
            // clear the object's weak references.

            for (auto it = peerMap.begin(); it != peerMap.end(); ++it)
            {
                xaml_hosting::IReferenceTrackerInternal *pObject = *it;
                ASSERT(pObject != NULL);
                pObject->OnReferenceTrackingProcessed(pCore);
            }
        }
    }

    // Release the core locks

    for(auto core_iterator = m_activeCores.begin(); core_iterator != m_activeCores.end(); ++core_iterator)
    {
        IDXamlCore* pCore = *core_iterator;

        #if DBG
        pCore->SetTrackerPtrsNeedValidation(TRUE);
        #endif

        ReleaseSRWLockExclusive(&pCore->GetReferenceSrwLock());
    }


    #if DBG
    ReferenceTrackerLogTracking::Log( ReferenceTrackerLogTrackingPhase::Completed);

    #if XCP_MONITOR
    if( s_cMaxPeerStressIterations == 0 && !m_backgroundGCEnabled)
    #else
    if( !m_backgroundGCEnabled)
    #endif
    {
        LOG(L"Reference tracking completed.  Objects=%d, Sources=%d, Targets=%d, Unreachable=%d",
            _peerCount, m_currentFindWalkID, _targetCount, _unreachableCount );
    }
    #endif

    TraceReferenceTrackingCompletedEnd (_peerCount, m_currentFindWalkID, _targetCount, _unreachableCount );


    // Put the event back into the signaled state to unblock the UI thread on which
    // this tracking started (if the thread is waiting to update references).
    ASSERT( m_startThreadId != 0 );
    m_runningOnUIThreadEvent.SetEvent();
    m_startThreadId = 0;


    // Release the global reference tracking lock
    ReleaseSRWLockExclusive( &s_lock );

    //Telemetry Notify ,GC tree walk ended
    const auto telemetryAggregator = GetTelemetryAggregator();
    telemetryAggregator->SignalEvent(GCEventSignalType::ReleaseTreeWalkLock);


#if XCP_MONITOR
    m_bIsReferenceTrackingActive = FALSE;
#endif

    RRETURN(S_OK);

}

#if DBG
HRESULT
ReferenceTrackerManager::RunValidation()
{
    IDXamlCore* pCore = DXamlServices::GetDXamlCore();

    if( _pReferenceTrackerHost != NULL && pCore->IsFinalReleaseQueueEmpty() )
    {
        AutoReentrantReferenceLock peerTableLock(pCore);
        PeerMapEntriesHelper peerMap(pCore);

        for (auto it = peerMap.begin(); it != peerMap.end(); ++it)
        {
            xaml_hosting::IReferenceTrackerInternal *pObject = *it;
            ASSERT(pObject != NULL);

            if (!pObject->IsReferenceTrackerPegSet() && !pObject->IsPegged(false /*isRefCountPegged*/) && pObject->IsAlive() && pObject->ImplicitPegAllowed())
            {
                ULONG expectedRef = pObject->GetRefCount(RefCountType::Expected);
                ULONG actualRef = pObject->GetRefCount(RefCountType::Actual);

                // synchronization issue with CLR, if the CLR started releasing
                // its references to the object at this time.
                // check a state flag on the object to make sure we are not in
                // the middle of a BeforeRelease()
                if (!pObject->ShouldSkipTrackerLeakCheck())
                {
                    if (expectedRef < actualRef)
                    {
                        #if DBG_LIFETIME
                        WCHAR szValue2[250];
                        swprintf_s(szValue2, 250, L"Object: %p (%S) - ActualRef:%d ExpectedRef:%d", pObject, typeid(*pObject).name(), actualRef, expectedRef);
                        Trace(szValue2);
                        #endif

                        GetPALDebuggingServices()->TESTSignalReferenceTrackerLeak();
                        //ASSERT(FALSE);
                    }
                }

                // clear the BeforeRelease() state, because we when we revisit this
                // object, we'll have refreshed the reference counts we are examining
                pObject->ClearSkipTrackerLeakCheck();
            }
        }
    }

    ValidateTrackerPtrs( pCore );

    RRETURN(S_OK);
}
#endif


#if DBG
void
ReferenceTrackerManager::ValidateTrackerPtrs( IDXamlCore *pCore)
{
    ASSERT( This != nullptr );

    // we can't let a GC occur during this time because we are updating state on the static ReferenceTrackerManager
    // the static member on ReferenceTrackerManager is updated when calling SetRootOfTrackerWalk
    auto gcLock = wil::AcquireSRWLockExclusive(&s_lock);

    AutoReentrantReferenceLock peerTableLock(pCore);

    if (This->_pReferenceTrackerHost == nullptr || !pCore->GetTrackerPtrsNeedValidation())
        return;

    pCore->SetTrackerPtrsNeedValidation(FALSE);

    PeerMapEntriesHelper peerMap(pCore);
    CStaticLock lock;

    // Reset the flag in all the TrackerPtrs
    for (auto& pTrackerBase : This->m_allTrackerPtrs)
    {
        if( pTrackerBase->m_pCore != pCore )
            continue;

        pTrackerBase->m_fHasBeenWalked = FALSE;
    }

    // Walk all the peer objects, setting the above flag
    for (auto pObject : peerMap)
    {
        pObject->ReferenceTrackerWalk( RTW_TrackerPtrTest, TRUE);
    }

    // Verify that the flag got set in all TrackerPtrs
    for (const auto& pTrackerBase : This->m_allTrackerPtrs)
    {
        if( pTrackerBase->m_pCore != pCore )
            continue;

        // If this assert fires, it usually means that there is a TrackerPtr<> somewhere that isn't walked in an
        // OnReferenceTrackerWalk override.  To help identify the exact TrackerPtr<>, enable
        // RuntimeEnabledFeature.CaptureTrackerPtrCallStack in a DBG build.

        ASSERT(
            pTrackerBase->m_fHasBeenWalked  // Flag got set by the above walk
            || !pTrackerBase->IsSet()       // TrackerPtr isn't in use
            || pTrackerBase->m_fStatic      // TrackerPtr is a static event
            );
    }

}
#endif



//+--------------------------------------------------------------------
//
//  ReferenceTrackerWalk
//
//  This is called as part of the walk inside an IReferenceTracker::FindTrackerTargets
//  call.  If we reach this point, we're either going to peg the tracker target, unpeg
//  it, or notify the tracker source of it.
//
//+--------------------------------------------------------------------

//static
void
ReferenceTrackerManager::ReferenceTrackerWalk(EReferenceTrackerWalkType walkType, ::IReferenceTrackerTarget *pTrackerTarget )
{
    AssertActive( TRUE );

    switch(walkType)
    {
        case RTW_Unpeg:
            #if DBG
            ReferenceTrackerLogTarget::Log( pTrackerTarget, walkType);
            #endif

            VERIFYHR( pTrackerTarget->Unpeg() );

            _targetCount++;
            break;

        case RTW_Find:
            #if DBG
            ReferenceTrackerLogTarget::Log( pTrackerTarget, walkType);
            #endif

            VERIFYHR( This->_pFindReferenceTargetsCallback->FoundTrackerTarget( pTrackerTarget ));
            break;

        case RTW_Peg:
            #if DBG
            ReferenceTrackerLogTarget::Log( pTrackerTarget, walkType);
            #endif

            VERIFYHR( pTrackerTarget->Peg() );
            break;

        case RTW_Reachable:

            #if DBG
            ReferenceTrackerLogTarget::Log( pTrackerTarget, walkType);
            #endif
            break;

        case RTW_GetElementCount:
        case RTW_TotalCompressedImageSize:
            break;

        #if DBG
        case RTW_TrackerPtrTest:
            break;
        #endif

        default:
            ASSERT(FALSE);
    }

}



//+--------------------------------------------------------------------
//
//  SetReferenceTrackerHost
//
//  Receive callbacks that we can use to disconnect reference tracker sources.
//
//+--------------------------------------------------------------------


_Check_return_
IFACEMETHODIMP
ReferenceTrackerManager::SetReferenceTrackerHost( _In_ IReferenceTrackerHost *value )
{
    // Keep the callbacks.
    ReplaceInterface( _pReferenceTrackerHost, value );

    return S_OK;
}


//+--------------------------------------------------------------------
//
//  AddRef/ReleaseFromReferenceTracker
//
//  This is called by an IReferenceTracker on an IReferenceTrackerTarget.
//  This is used in place of normal interface ref-counting.
//
//+--------------------------------------------------------------------


HRESULT
ReferenceTrackerManager::AddRefFromReferenceTracker( IReferenceTrackerTarget *pTrackerTarget )
{
    AssertActive( FALSE );
    return pTrackerTarget->AddRefFromReferenceTracker();
}


HRESULT
ReferenceTrackerManager::ReleaseFromReferenceTracker( IReferenceTrackerTarget *pTrackerTarget )
{
    AssertActive( FALSE );
    return pTrackerTarget->ReleaseFromReferenceTracker();
}




//+--------------------------------------------------------------------
//
//  GetTrackerTarget
//
//  Get the input as an IReferenceTrackerTarget.  If it already is one, it is simply
//  returned (with a new ref).  If it isn't one, but we have reference tracking
//  callbacks available, then wrap it in one.
//
//+--------------------------------------------------------------------


IInspectable*
ReferenceTrackerManager::GetTrackerTarget(IUnknown* const punk)
{
    IInspectable *pReturnValue = nullptr;

    if (DXamlServices::IsDXamlCoreShutdown())
    {
        // If the DXamlCore is shutdown, then we shouldn't really be creating loopback RCWs
        // after we had requested CLR to disconnect all RCWs.
        ASSERT(FALSE);
    }

    // Do nothing if we don't have callbacks available (return null)
    if(This->_pReferenceTrackerHost != nullptr )
    {
        // See if the value is already an IReferenceTrackerTarget.
        ::IReferenceTrackerTarget* pTrackerTarget = ctl::query_interface<::IReferenceTrackerTarget>(punk);

        // If not, then wrap it in one.
        if(pTrackerTarget == nullptr)
        {
            IFCFAILFAST(This->_pReferenceTrackerHost->GetTrackerTarget(punk, &pTrackerTarget));
        }

        // Get the tracker target as an Inspectable
        IFCFAILFAST(ctl::do_query_interface(pReturnValue, pTrackerTarget));
        ReleaseInterface(pTrackerTarget);
    }

    return pReturnValue;

}




//+--------------------------------------------------------------------
//
//  CalculateIsPeerStressEnabled
//
//  This is optionally used in stress to cause a GC every n-th iteration, rather than
//  every iteration.
//
//+--------------------------------------------------------------------

#if XCP_MONITOR
//static
bool
ReferenceTrackerManager::CalculateIsPeerStressEnabled()
{
    ASSERT( s_cMaxPeerStressIterations != 0 );

    if( ++s_cPeerStressIteration >= s_cMaxPeerStressIterations )
    {
        s_cPeerStressIteration = 0;
        return true;
    }

    return false;
}
#endif



//+--------------------------------------------------------------------
//
//  ReferenceTrackerManager
//
//+--------------------------------------------------------------------

ReferenceTrackerManager::ReferenceTrackerManager(  )
    : m_pTrackerWalkRoot(nullptr)
    , m_currentFindWalkID(0)
    , _pFindReferenceTargetsCallback(nullptr)
    , _pReferenceTrackerHost(nullptr)
    , _refs(0)
    , m_startThreadId(0)
{ }

//+--------------------------------------------------------------------
//
//  ~ReferenceTrackerManager
//
//+--------------------------------------------------------------------

ReferenceTrackerManager::~ReferenceTrackerManager()
{
    // There's a race condition in EnsureInitialized where we create an object and immediately throw it away.
    // That throwaway copy shouldn't clear the global instance pointer.
    if (This == this)
    {
        auto lock {wil::AcquireSRWLockExclusive(&s_lock)};
        if (This == this)
        {
            This = nullptr;
        }
    }

    ReleaseInterface( _pReferenceTrackerHost );
}


#if DBG
void
ReferenceTrackerManager::UnregisterTrackerPtr( _In_ TrackerTargetReference *pTrackerPtr, IDXamlCore *pCore )
{
    if( This == nullptr)
        return;

    if( pCore )
    {
        CStaticLock lock;

        This->m_allTrackerPtrs.erase(pTrackerPtr);
        This->m_allTrackerPtrs.shrink_to_fit();
    }
}
#endif



#if DBG
void
ReferenceTrackerManager::RegisterTrackerPtr( _In_ TrackerTargetReference *pTrackerPtr, IDXamlCore *pCore )
{
    if( pCore )
    {
        CStaticLock lock;
        ASSERT(This->m_allTrackerPtrs.insert(pTrackerPtr).second);
    }
}
#endif

// Loop forever, calling to CLR to trigger a GC.Collect(2).
#if XCP_MONITOR
bool ReferenceTrackerManager::m_backgroundGCEnabled = FALSE;
DWORD WINAPI ReferenceTrackerManager::BackgroundGCThread(_In_  LPVOID lpParameter)
{
    HRESULT hr = S_OK;
    int count = 0;
    m_backgroundGCEnabled = TRUE;

    while(true)
    {
        IFC(ReferenceTrackerManager::TriggerCollection());

        if(count++ % 5 == 0)
        {
            IFC(ReferenceTrackerManager::TriggerFinalization());
        }
    }

Cleanup:
    return SUCCEEDED(hr) ? 0 : 1;
}
#endif // XCP_MONITOR


// Fork  a thread and run BackgroundGCThread() on it.
#if XCP_MONITOR
_Check_return_ HRESULT ReferenceTrackerManager::StartBackgroundGC()
{
    HANDLE thread = CreateThread(NULL, 0, ReferenceTrackerManager::BackgroundGCThread, NULL, 0, NULL);
    if (!thread)
    {
        return E_FAIL;
    }

    CloseHandle(thread);
    return S_OK;
}
#endif // XCP_MONITOR


// Tracking debug logging
#if DBG
ReferenceTrackerLogItem* ReferenceTrackerManager::_logItems = nullptr;
int ReferenceTrackerManager::_logPosition = 0;
#endif

#if DBG
void ReferenceTrackerManager::Log( const ReferenceTrackerLogItem &item )
{
    _logItems[ _logPosition ] = item;
    if( ++_logPosition == _maxLogItemsEntries )
    {
        _logPosition = 0;
    }
}
#endif
