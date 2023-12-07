// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WeakReferenceSourceNoThreadId.h"
#include "AutoReentrantReferenceLock.h"
#include "LifetimeExterns.h"
#include "LifetimeUtils.h"
#include "CStaticLock.h"
#include "TrackerTargetReference.h"
#include "TrackerPtr.h"
#include "TrackerPtrWrapper.h"
#include "MUX-ETWEvents.h"

using namespace DirectUI;
using namespace ctl;
using namespace xaml_hosting;

// Lock object exposed to tracker extensions (in MUXP).
/* static */ WeakReferenceSourceNoThreadId::ReferenceTrackerGCLock WeakReferenceSourceNoThreadId::s_refTrackerGCLock;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WeakReferenceSourceNoThreadId
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WeakReferenceSourceNoThreadId::WeakReferenceSourceNoThreadId():
    m_bIsPeggedNoRef(FALSE),
    m_ulPegRefCount(0),
    m_bReferenceTrackerPeg(FALSE),
    m_ulRefCountFromTrackerSource(0),
    m_ulExpectedRefCount(0),
    m_lastFindWalkID(0),
    m_referenceTrackerBitFields(),
    m_bIsDisconnected(FALSE),
    m_bIsDisconnectedFromCore(TRUE),
    m_bHasState(false),
    m_bCastedAsControl(FALSE)
{
    // ReferenceTracker will need to know in the next GC if this object was reachable in the last GC.
    // Assume it was (otherwise ReferenceTracker will think this is pending finalization).
    m_referenceTrackerBitFields.bReachable  = true;

#if DBG
    m_bShouldSkipTrackerLeakCheck = FALSE;
    m_bQueuedForUnreachableCleanup = FALSE;

    m_breakOnWalk = false;
#endif

    #if DBG
    // This is a debug aid, to enable location-based breakpoints
    m_pMemoryCheck = this;
    #endif
}



#if DBG
WeakReferenceSourceNoThreadId::~WeakReferenceSourceNoThreadId()
{
    // We should have been removed from the reference tracking list during OnFinalRelease
    if( m_referenceTrackerBitFields.AddedToReferenceTrackingList )
    {
        auto pCore = DXamlServices::GetDXamlCore();
        if( pCore != nullptr )
        {
            ASSERT( !pCore->IsInReferenceTrackingList(ctl::interface_cast<IReferenceTrackerInternal>(this)) );
        }
    }
}
#endif



HRESULT
WeakReferenceSourceNoThreadId::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void **ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IWeakReferenceSource)))
    {
        *ppObject = ctl::interface_cast<IWeakReferenceSource>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(IReferenceTracker)))
    {
        IFC_RETURN(ComposingTrackerTargetWrapper::Ensure(this));
        *ppObject = static_cast<IReferenceTracker *>(ctl::interface_cast<IReferenceTrackerInternal>(this));
    }
    else if (InlineIsEqualGUID(iid, __uuidof(xaml_hosting::IReferenceTrackerInternal)))
    {
        *ppObject = ctl::interface_cast<IReferenceTrackerInternal>(this);
    }
    else
    {
        return SupportErrorInfo::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();

    return S_OK;
}




//
// WeakReferenceSource Support
//
_Check_return_ HRESULT
WeakReferenceSourceNoThreadId::GetWeakReference(_Outptr_ IWeakReference **weakReference)
{
    *weakReference = GetWeakReferenceImpl().Detach();

    // Note that this weak reference might already be disconnected.  When an object is abandoned but not
    // yet fully destructed, it might still attempt to get a weak reference.  (E.g. this happens in
    // ScrollContentPresenter::HookupScrollingComponents).
    if (m_referenceTrackerBitFields.bWeakReferenceDisconnected)
    {
        static_cast<Details::WeakReferenceImpl*>(*weakReference)->Clear();
    }

    return S_OK;
}

void
WeakReferenceSourceNoThreadId::Resurrect()
{
    ResurrectWeakReference();
    m_referenceTrackerBitFields.bReachable = true;
    m_referenceTrackerBitFields.bRefCountPeg = false;
    m_referenceTrackerBitFields.bWeakReferenceDisconnected = false;
}

_Check_return_ HRESULT WeakReferenceSourceNoThreadId::EndShutdown()
{
    // No-op. Derived types will add behavior that they need.
    return S_OK;
}

//
// Core Lifetime Support
//
//+---------------------------------------------------------------------------
//
//  GetReferenceTrackerManager
//
//  This is the current mechanism for getting the global ReferenceTrackerManager.
//
//+---------------------------------------------------------------------------


_Check_return_
HRESULT
WeakReferenceSourceNoThreadId::GetReferenceTrackerManager( _Out_ ::IReferenceTrackerManager **value )
{
    *value = ReferenceTrackerManager::GetNoRef();
    (*value)->AddRef();

    return S_OK;
}


//+---------------------------------------------------------------
//
// This is called when a ReferenceTrackerSource begins to point to this
// ReferenceTracker.
//
//+---------------------------------------------------------------
_Check_return_ HRESULT
WeakReferenceSourceNoThreadId::ConnectFromTrackerSource()
{
    InterlockedIncrement( &m_ulRefCountFromTrackerSource );

    // Once a tracker source is referencing (and protecting) this object, it no longer need protection
    // from Reference Tracking.
    ClearReferenceTrackerPeg();

    RRETURN(S_OK);
}

//+---------------------------------------------------------------
//
// This is called when a ReferenceTrackerSource releases this
// ReferenceTracker.  In terms of the ReferenceTrackerManager logic,
// this happens after OnReferenceTrackingCompleted and
//  OnReferenceTrackingProcessed, i.e. during GC.
//
//+---------------------------------------------------------------
_Check_return_ HRESULT
WeakReferenceSourceNoThreadId::DisconnectFromTrackerSource()
{
    LONG refCount = InterlockedDecrement( &m_ulRefCountFromTrackerSource );

    if (refCount == 0)
    {
        // Once the tracker source has disconnected, reset the find walked state.
        m_referenceTrackerBitFields.bFindWalked = false;
    }

    RRETURN(S_OK);
}

//
// For RCW References to C++ Aggregated Objects:
//
//     +-----------------------+
//     |  MyStackPanel(Native) <--- RCW References
//     |          |            |
//     |      StackPanel       |
//     +-----------------------+
//
// MyStackPanel will get the RCW references from the CLR.
// NOTE: There is no CCW.
//

//
// For RCW References to C# Aggregated Objects:
//
//     +-----------------------+
//     |  MyStackPanel(CCW)    |
//     |          |            |
//     |      RCW Reference    |
//     |          |            |
//     |      StackPanel       |
//     +-----------------------+
//
// MyStackPanel will keep an RCW reference to the StackPanel.
// NOTE: MyStackPanel has a CCW.
//

_Check_return_ HRESULT
WeakReferenceSourceNoThreadId::AddRefFromTrackerSource()
{
    // For C++ Aggregated Objects, ignore the notification, as the addref/release will go to the outer.
    if (!IsNativeAndComposed())
    {
        ctl::addref_expected(this, ExpectedRef_CLR);
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT
WeakReferenceSourceNoThreadId::ReleaseFromTrackerSource()
{
    #if DBG
    //
    // Skip Tracker Leak check on this object, because we may be reading
    // unstable reference counts on this object.
    //
    // We are not using locks here because this cannot happen while the GC thread is running.
    // The lock would be undesirable for product scenarios, and hence the workaround for the
    // tracker leak detection logic (for testing) is to use this state flag.
    //
    m_bShouldSkipTrackerLeakCheck = TRUE;
    #endif

    // For C++ Aggregated Objects, ignore the notification, as the addref/release will go to the outer.
    if (!IsNativeAndComposed())
    {
        ctl::release_expected(this);
    }

    RRETURN(S_OK);
}

bool
WeakReferenceSourceNoThreadId::IsAlive()
{
    return !m_referenceTrackerBitFields.bWeakReferenceDisconnected;
}

//+---------------------------------------------------------------
//
//  FindTrackerTargets
//
//  Find all IReferenceTrackerTarget instances that are reachable from
//  this Object and notify the source by calling the callback.
//
//+---------------------------------------------------------------

_Check_return_
HRESULT
WeakReferenceSourceNoThreadId::FindTrackerTargets( _In_ IFindReferenceTargetsCallback *callback )
{
    TraceFindTrackerTargetsBegin();
    // set the find walked state
    m_referenceTrackerBitFields.bFindWalked = true;

    // Identify the root of the walk
    ReferenceTrackerManager::SetRootOfTrackerWalk(ctl::interface_cast<IReferenceTrackerInternal>(this), callback, RTW_Find );

    // Walk the tree
    ReferenceTrackerWalk( RTW_Find );

    ReferenceTrackerManager::SetRootOfTrackerWalk( nullptr, nullptr, RTW_None );
    TraceFindTrackerTargetsEnd();
    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
//
//  ReferenceTrackerWalk
//
//  This method is called during reference tracking (during a garbage collection) to find all
//  objects that are referenced by this object.
//
//+--------------------------------------------------------------------------

bool
WeakReferenceSourceNoThreadId::ReferenceTrackerWalkCore(EReferenceTrackerWalkType walkType, _In_ bool fIsRoot)
{
    bool walked = false;

    // Check for cases where we can skip out early
    switch (walkType)
    {
        case RTW_Peg:
        {
            // If this object is already pegged, no need to go further
            if(HasBeenWalked(DirectUI::EReferenceTrackerWalkType::RTW_Peg))
            {
                ASSERT(IsReachable());
                goto Cleanup;
            }

            // If the root isn't already pegged, see if it should get a ref-count peg
            // To do: consolidate all three of these IsPegged into a single method
            if (ReferenceTrackerManager::IsRootOfTrackerWalk(ctl::interface_cast<IReferenceTrackerInternal>(this)) && !IsPegged(false /*isRefCountPegged*/) && !m_bReferenceTrackerPeg && !IsPegged(true))
            {
                // If the DO has an expected reference count less than the actual reference count, then it is
                // an object that a method is currently operating on and its tree should be protected.
                if (ImplicitPegAllowed()
                    && IsAlive()
                    && (GetRefCount(RefCountType::Expected) < GetRefCount(RefCountType::Actual)))
                {
                    SetRefCountPeg();
                }
            }

            // If still not pegged, we don't need to do the walk at all.
            if (ReferenceTrackerManager::IsRootOfTrackerWalk(ctl::interface_cast<IReferenceTrackerInternal>(this)) && !IsPegged(false /*isRefCountPegged*/) && !m_bReferenceTrackerPeg && !IsPegged(true /*isRefCountPegged*/))
            {
                goto Cleanup;
            }

            // We're going to peg all the reachable tracker targets from this object.
            // Mark it so that we don't do it again.
            m_referenceTrackerBitFields.bPegWalked = true;

            // Since we're going to peg this object and everything it can reach, we also know it's going to be reachable at the end of the GC
            m_referenceTrackerBitFields.bReachable = true;
        }
        break;

        case RTW_Unpeg:
        {
            // During an unpeg walk, we don't need to walk at all; just let each DependencyObject
            // get unpegged directly.
            // Also, if this DO wasn't reachable in the last GC, don't try to unpeg its references, as the CCW
            // it references is already disconnected.  (This can happen if a second GC happens before
            // the previous GC is fully finalized.)

            if( !fIsRoot || !IsReachable() )
            {
                goto Cleanup;
            }

            ClearRefCountPeg();

            // Clear the flag that indicates we've been pegged because of being in the core's
            // m_PegNoRefCoreObjectsWithoutPeers table.
            m_referenceTrackerBitFields.peggedByCoreTable = false;
        }
        break;

        case RTW_Find:
        {
            // If we've been here before, we can stop

            if (m_lastFindWalkID == ReferenceTrackerManager::GetCurrentFindWalkID())
            {
                goto Cleanup;
            }
            else
            {
                // Otherwise, remember that we've been here, to make sure we won't go into an infinite loop
                m_lastFindWalkID = ReferenceTrackerManager::GetCurrentFindWalkID();
            }

            // We don't need to inform the tracker source of tracker targets if the target is
            // already pegged anyway.  So if we're on a Find walk, and this object was already
            // part of a pegging walk, then there's nothing we need to do.

            if(HasBeenWalked(DirectUI::EReferenceTrackerWalkType::RTW_Peg))
            {
                goto Cleanup;
            }
        }
        break;

        case RTW_Reachable:
        {
            // We're identifying objects that can be reached from a root; either a pegged DO or a
            // rooted (not GC'd) tracker source.

            if( m_referenceTrackerBitFields.bReachable )
            {
                // We've already walked this tree, early out.
                goto Cleanup;
            }

            m_referenceTrackerBitFields.bReachable = true;
        }
        break;

        case RTW_GetElementCount:
        case RTW_TotalCompressedImageSize:
        {
            if (m_referenceTrackerBitFields.MemoryDiagWalked)
            {
                // We've already walked this tree, early out.
                goto Cleanup;
            }

            m_referenceTrackerBitFields.MemoryDiagWalked = true;
        }
        break;

        #if DBG
        case RTW_TrackerPtrTest:
        {
            // Only perform the TrackerPtr test on the root DO of the walk

            if( !fIsRoot )
            {
                goto Cleanup;
            }
        }
        break;
        #endif

        default:
        {
            ASSERT(false);
        }
    }

    walked = true;

Cleanup:
    return walked;
}

bool
WeakReferenceSourceNoThreadId::ReferenceTrackerWalk(EReferenceTrackerWalkType walkType, _In_ bool fIsRoot)
{
    bool walked = true;

    #if DBG
    // This is a debug aid, to enable location-based breakpoints
    m_pMemoryCheck = this;

    // Trace debug objects to the debugger.  (Set a breakpoint in here to inspect each walk.)
    if( m_breakOnWalk )
    {
        WCHAR message[250];
        swprintf_s(message, L"ReferenceTrackerWalk on %p (%d)", this, walkType);
        Trace(message);
    }

    #endif


    if (fIsRoot)
    {
        ReferenceTrackerManager::SetRootOfTrackerWalk(ctl::interface_cast<IReferenceTrackerInternal>(this), nullptr /*callback*/, walkType );
    }

    walked = ReferenceTrackerWalkCore(walkType, fIsRoot);

    if (walked)
    {
        #if DBG
        // Clear the flag that verifies that the base virtual was called
        m_bOnReferenceTrackerWalked = FALSE;
        #endif

        // Delegate ReferenceTrackerWalk to any subclasses.
        ComposingTrackerExtensionWrapper::OnReferenceTrackerWalk(this, walkType);

        // Verify that the base virtual was called
        #if DBG
        ASSERT(m_bOnReferenceTrackerWalked);
        #endif
    }

    if (fIsRoot)
    {
        ReferenceTrackerManager::SetRootOfTrackerWalk( nullptr, nullptr, RTW_None );
    }

    return walked;
}


//+---------------------------------------------------------------------------
//
// Lifetime Tree Walk Helpers
//
//+---------------------------------------------------------------------------

void
WeakReferenceSourceNoThreadId::OnReferenceTrackingProcessed(_In_ DirectUI::IDXamlCore* pCore)
{
    UNREFERENCED_PARAMETER(pCore);

    if( !IsReachable() )
    {
        // This object can't be reached by the application.  Disconnect
        // any weak references to it so that they don't cause any kind
        // of resurrection.

        m_referenceTrackerBitFields.bWeakReferenceDisconnected = true;
        ClearWeakReference();

        ReferenceTrackerManager::_unreachableCount++;
        #if DBG
        ReferenceTrackerLogUnreachable::Log( (IReferenceTracker*) this);
        #endif

        TraceReferenceTrackerCollectedInfo (static_cast<int>(GetTypeIndex()));
    }

    // The first find walk ID will be 1.  m_lastFindWalkID == 0 means it hasn't been visited.
    m_lastFindWalkID = 0;
}

void
WeakReferenceSourceNoThreadId::PrepareForReferenceWalking()
{
    m_referenceTrackerBitFields.bReachable = false;
    m_referenceTrackerBitFields.bPegWalked = false;
    m_referenceTrackerBitFields.MemoryDiagWalked = false;

    // The first find walk ID will be 1.  m_lastFindWalkID == 0 means it hasn't been visited.
    m_lastFindWalkID = 0;
}

//+---------------------------------------------------------------------------
//
// Pegs
//
// The ReferenceTracker peg flag is set when an object is created, and cleared when
// this object is referenced either internally (in UnPegNoRef) or when connected to a reference
// tracker source.  Unlike Peg and PegNoRef, this flag does not keep an AddRef on the object, it
// is only used by ReferenceTrackerManager.
//
//+---------------------------------------------------------------------------

void
WeakReferenceSourceNoThreadId::SetReferenceTrackerPeg()
{
    m_bReferenceTrackerPeg = TRUE;
}


// Clear the m_bReferenceTrackerPeg that gets set at create time, to protect ths object
// from GC during initialization.  This is called when someone references the object, for example
// adding it to the tree, or referencing it from an RCW.
// *Note that this can't be used during a GC because it takes the GC lock.*
void
WeakReferenceSourceNoThreadId::ClearReferenceTrackerPeg()
{
    // We have to be lazy about QI'ing the outer for IReferenceTracker and IReferenceTrackerExtension.
    // We'll need that before the next GC, since m_bReferenceTrackerPeg won't be protecting us from GC
    // any longer.  But when we get to the point of removing this peg, we're done with initialization, yet it's
    // design to happen very shortly after initialization completes.
    // Note that this can't be used during a GC because it takes the GC lock.
    VERIFYHR(ComposingTrackerTargetWrapper::Ensure(this));

    m_bReferenceTrackerPeg = FALSE;
}

void
WeakReferenceSourceNoThreadId::SetRefCountPeg()
{
    m_referenceTrackerBitFields.bRefCountPeg = true;

    #if DBG_LIFETIME
    if (m_referenceTrackerBitFields.bRefCountPeg)
    {
        WCHAR szValue2[250];
        swprintf_s(szValue2, 250, L"Implicit Pegging: Object: %p - ActualRef:%d ExpectedRef:%d RCWRef:%d", this, GetRefCount(RefCountType::Actual), GetActualOrExpectedRefCount(RefCountType::Expected), m_ulRefCountFromTrackerSource);
        Trace(szValue2);
    }
    #endif
}

void
WeakReferenceSourceNoThreadId::ClearRefCountPeg()
{
    #if DBG_LIFETIME
    if (m_referenceTrackerBitFields.bRefCountPeg)
    {
       WCHAR szValue2[250];
       swprintf_s(szValue2, 250, L"Cleared Pegging: Object: %p (%S) - ActualRef:%d ExpectedRef:%d", this, typeid(*this).name(), GetRefCount(RefCountType::Actual), GetRefCount(RefCountType::Expected));
       Trace(szValue2);
    }
    #endif

    m_referenceTrackerBitFields.bRefCountPeg = false;
}

void WeakReferenceSourceNoThreadId::UpdatePeg(bool peg)
{
    if(peg)
    {
        if (0 == m_ulPegRefCount)
        {
            ctl::addref_interface(this);
        }

        ULONG ulCount = m_ulPegRefCount + 1;
        if (ulCount < m_ulPegRefCount)
        {
            // counter overload
            ASSERT(FALSE);
            XAML_FAIL_FAST();
        }
        m_ulPegRefCount = ulCount;
    }
    else
    {
        if (m_ulPegRefCount > 0)
        {
            if (--m_ulPegRefCount == 0)
            {
                ctl::release_interface_nonull(this);

                #if XCP_MONITOR
                if (ReferenceTrackerManager::IsPeerStressEnabled())
                {
                    ReferenceTrackerManager::TriggerCollection();
                }
                #endif
            }
        }
       else
       {
           ASSERT(FALSE, L"Over unpeg: %p", this );
        }
    }
}

void WeakReferenceSourceNoThreadId::PegNoRef()
{
    if (!m_bIsPeggedNoRef)
    {
        m_bIsPeggedNoRef = TRUE;
        ctl::addref_interface(this);
    }
}

void WeakReferenceSourceNoThreadId::UnpegNoRef(bool suppressClearReferenceTrackerPeg)
{
    if (m_bIsPeggedNoRef)
    {
        m_bIsPeggedNoRef = FALSE;
        ctl::release_interface_nonull(this);
    }

    // Also clear the create-time peg that is used only by the ReferenceTrackerManager.  When this flag
    // is set, it doesn't mean that there is an AddRef on the object like PegNoRef does, but it does mean
    // that the object will be protected by the RTM.
    // The caller can suppress this (useful during destruction, where we don't need to bother worry about
    // GC anymore, and don't want to risk the GC lock.)
    if (!suppressClearReferenceTrackerPeg)
    {
        ClearReferenceTrackerPeg();
    }

    #if XCP_MONITOR
    if (ReferenceTrackerManager::IsPeerStressEnabled())
    {
        ReferenceTrackerManager::TriggerCollection();
    }
    #endif
}

//+---------------------------------------------------------------------------
//
// Object Lifetime States for Tree Walk
//
//+---------------------------------------------------------------------------

bool
WeakReferenceSourceNoThreadId::IsReferencedByTrackerSource()
{
    return m_ulRefCountFromTrackerSource > 0;
}

// The ReferenceTracker peg flag is set when an object is created, and cleared when
// this object is referenced either internally (in UnPegNoRef) or when connected to a reference
// tracker source.  Unlike Peg and PegNoRef, this flag does not keep an AddRef on the object, it
// is only used by ReferenceTrackerManager.
bool
WeakReferenceSourceNoThreadId::IsReferenceTrackerPegSet()
{
    return m_bReferenceTrackerPeg;
}

bool
WeakReferenceSourceNoThreadId::IsReachable()
{
    // Can this object be reached by either a tracker source or a pegged object?
    return m_referenceTrackerBitFields.bReachable;
}

bool
WeakReferenceSourceNoThreadId::HasBeenWalked(_In_ DirectUI::EReferenceTrackerWalkType walkType)
{
    if(walkType == DirectUI::EReferenceTrackerWalkType::RTW_Find)
    {
        return m_referenceTrackerBitFields.bFindWalked;
    }
    else if(walkType == DirectUI::EReferenceTrackerWalkType::RTW_Peg)
    {
        return m_referenceTrackerBitFields.bPegWalked;
    }
    else
    {
        XAML_FAIL_FAST(); // Never expecting to be called with another walk type at this point.
        return false;
    }
}

bool
WeakReferenceSourceNoThreadId::IsPegged(bool isRefCountPegged)
{
    if(isRefCountPegged)
    {
        return m_referenceTrackerBitFields.bRefCountPeg
            || m_referenceTrackerBitFields.peggedByCoreTable; // Pegged because it's in core's m_PegNoRefCoreObjectsWithoutPeers
    }
    else
    {
        // True if this is quick-pegged or count-pegged
        return ( m_bIsPeggedNoRef || m_ulPegRefCount > 0 );
    }
}

bool
WeakReferenceSourceNoThreadId::IsPeggedNoRef()
{
    return m_bIsPeggedNoRef ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// Final Release
//
//+---------------------------------------------------------------------------

bool
WeakReferenceSourceNoThreadId::OnFinalReleaseOffThread(_In_ bool allowOffThreadDelete /* = TRUE */)
{
    bool result = false;

    // If we're not on the thread we have affinity to, post to that thread and
    // do the FinalRelease there
    if (FAILED(CheckThread()))
    {
        // If we have an m_compositionWrapper, it's now a dangling pointer.  Clear it
        // so that we don't accidentally try to use it (such as in the AddRef that happens
        // during RTW_Peg walks).
        m_compositionWrapper = nullptr;

        // Reset the debug flag since we're going back to the UI thread where we will
        // call FinalRelease again.
        m_inFinalRelease = false;
        IDXamlCore* pCore = GetCoreForObject();
        if (pCore && ReferenceTrackerManager::IsCoreRegistered(pCore))
        {
            m_ignoreReleases = true;
            pCore->QueueObjectForFinalRelease(this);
            result = true;
        }
        else if (allowOffThreadDelete)
        {
            CStaticLock lock;
            // attempt an off thread release
            delete this;
            result = true;
        }
    }

    return result;
}

ULONG
WeakReferenceSourceNoThreadId::GetRefCount(RefCountType refCountType)
{
    if(refCountType == RefCountType::Actual)
    {
        // If the tracker extension is available, use it to return the actual ref. count
        // Otherwise, use this object ref. count.
        return ComposingTrackerExtensionWrapper::GetActualRefCount(this);
    }
    else
    {
        return m_ulExpectedRefCount;
    }
}

void
WeakReferenceSourceNoThreadId::UpdateExpectedRefCount(RefCountUpdateType updateType)
{
    if(updateType == RefCountUpdateType::Add)
    {
        InterlockedIncrement(&m_ulExpectedRefCount);
    }
    else
    {
        InterlockedDecrement(&m_ulExpectedRefCount);
    }
}

void
WeakReferenceSourceNoThreadId::OnReferenceTrackerWalk( INT walkType )
{
    DirectUI::EReferenceTrackerWalkType walkTypeAsEnum = static_cast<DirectUI::EReferenceTrackerWalkType>(walkType);
    #if DBG
    // Set the flag that verifies that the base virtual was called
    m_bOnReferenceTrackerWalked = TRUE;
    #endif

    if (m_trackers)
    {
        // Walk all of the registered trackers first
        std::for_each(m_trackers->begin(), m_trackers->end(),
           [walkTypeAsEnum](DirectUI::TrackerTargetReference* tracker){
               tracker->ReferenceTrackerWalk(walkTypeAsEnum);
           });
    }
}

void
WeakReferenceSourceNoThreadId::RegisterPtr(_In_ DirectUI::TrackerTargetReference* const pTrackerPtr)
{
    if (pTrackerPtr->IsRegistered())
    {
        #if DBG
        ASSERT(pTrackerPtr->GetOwner() == this);
        #endif
        return;
    }

    {
        // Initializing or modifying the vector of tracker pointers should be done under the lock
        AutoReentrantReferenceLock lock(DXamlServices::GetDXamlCore());

        if (!m_trackers)
        {
            m_trackers = std::make_unique<std::vector<DirectUI::TrackerTargetReference*>>();
            // Generally types seem to use 4-8 or more of these, so starting from 0 means a bunch
            // of re-allocations and copies. Starting with an initial capacity of 8 avoids this
            // overhead. For types that use more than 8 we still get one reallocation, but that
            // seems like a reasonable compromise with types that use fewer wasting more space.
            m_trackers->reserve(8);
        }

        #if DBG
        {
            auto itr = std::find(m_trackers->begin(), m_trackers->end(), pTrackerPtr);
            ASSERT(itr == m_trackers->end());
        }
        #endif

        m_trackers->push_back(pTrackerPtr);

        // This operations only fail if there's a violation of the invariants
        // no cleanup is necessary
        #if DBG
        pTrackerPtr->Register(this);
        #else
        pTrackerPtr->Register();
        #endif
    }
}

void
WeakReferenceSourceNoThreadId::UnregisterPtr(DirectUI::TrackerTargetReference* pTrackerPtr)
{
    if (pTrackerPtr->IsRegistered() && m_trackers)
    {
        // Modifying the vector of tracker pointers should be done under the lock
        AutoReentrantReferenceLock lock(DXamlServices::GetDXamlCore());

#if DBG
        {
            auto itr = std::find(m_trackers->begin(), m_trackers->end(), pTrackerPtr);
            ASSERT(itr != m_trackers->end());
        }
#endif

        m_trackers->erase(std::find(m_trackers->begin(), m_trackers->end(), pTrackerPtr));

#if XCP_MONITOR
        auto itr = std::find_if(m_trackers->begin(), m_trackers->end(), [](DirectUI::TrackerTargetReference* ptr) {
            return ptr && ptr->IsSet();
        });

        if (itr == m_trackers->end())
        {
            m_trackers.reset();
        }
#endif

        // This operations only fail if there's a violation of the invariants
        // no cleanup is necessary
        pTrackerPtr->Unregister();
    }
}

_Check_return_ HRESULT
WeakReferenceSourceNoThreadId::EnsureCompositionWrapper()
{
    AutoReentrantReferenceLock lock(DXamlServices::GetDXamlCore());
    if (!m_compositionWrapper)
    {
        auto wrapper = new DirectUI::ComposingTrackerWrapper();
        m_compositionWrapper = std::unique_ptr<DirectUI::ComposingTrackerWrapper>(wrapper);
    }
    return S_OK;
}

//
// Portion of IReferenceTrackerInternal that MUXP calls.
//

_Check_return_ HRESULT
WeakReferenceSourceNoThreadId::NewTrackerPtrWrapper(_Outptr_ xaml_hosting::ITrackerPtrWrapper** ppValue) /* override */
{
    HRESULT hr = S_OK;
    *ppValue = new TrackerPtrWrapper();
    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT
WeakReferenceSourceNoThreadId::DeleteTrackerPtrWrapper(_In_ xaml_hosting::ITrackerPtrWrapper* pValue) /* override */
{
    HRESULT hr = S_OK;
    delete pValue;
    RRETURN(hr);//RRETURN_REMOVAL
}

xaml_hosting::IReferenceTrackerGCLock*
WeakReferenceSourceNoThreadId::GetGCLock() /* override */
{
    return &s_refTrackerGCLock;
}

_Check_return_ HRESULT
WeakReferenceSourceNoThreadId::SetExtensionInterfaces(
    _In_ ::IReferenceTrackerExtension* extension,
    _In_ xaml_hosting::IReferenceTrackerInternal* overrides) /* override */
{
    return ComposingTrackerExtensionWrapper::SetExtensionInterfaces(this, extension, overrides);
}

//
// WeakReferenceSourceNoThreadId::ReferenceTrackerGCLock implementation.
//

INT64
WeakReferenceSourceNoThreadId::ReferenceTrackerGCLock::AcquireLock() /* override */
{
    // Count the number of times we've entered the lock.  The first time we
    // enter, perform the actual exclusive acquisition to the SRW lock.

    IDXamlCore* pCore = DXamlServices::GetDXamlCore();
    if (pCore != nullptr && pCore->IncrementReferenceLockEnterCount() == 1)
    {
        ASSERT(pCore->GetReferenceLockEnterCount() >= 0);
        AcquireSRWLockExclusive(&pCore->GetReferenceSrwLock());
    }

    return reinterpret_cast<INT64>(pCore);
}

void
WeakReferenceSourceNoThreadId::ReferenceTrackerGCLock::ReleaseLock(INT64 token) /* override */
{
    IDXamlCore* pCore = reinterpret_cast<IDXamlCore*>(token);

    if (pCore)
    {
        ASSERT(pCore->GetReferenceLockEnterCount() > 0);

        // On the last reentrant exit, release the SRW lock.
        if (pCore->DecrementReferenceLockEnterCount() == 0)
        {
            ReleaseSRWLockExclusive(&pCore->GetReferenceSrwLock());
        }
    }
}

void WeakReferenceSourceNoThreadId::AddToReferenceTrackingList()
{
    // We shouldn't be getting initialized more than once.  But just in case, ensure we don't
    // get added to the reference tracker list more than once, in order to avoid a dangling
    // reference after destruction.
    if (m_referenceTrackerBitFields.AddedToReferenceTrackingList)
    {
        ASSERT(false);
    }
    else
    {
        IDXamlCore *pCore = DXamlServices::GetDXamlCore();
        ASSERT(pCore != NULL);

        m_referenceTrackerBitFields.AddedToReferenceTrackingList = true;
        pCore->AddToReferenceTrackingList(ctl::interface_cast<IReferenceTrackerInternal>(this));
    }
}

void WeakReferenceSourceNoThreadId::ClearWeakReference()
{
    auto currentValue = Microsoft::WRL::Details::ReadValueFromPointerNoFence<Details::ReferenceCountOrWeakReferencePointer>(&refCount_);

    if (Details::IsValueAPointerToWeakReference(currentValue.rawValue))
    {
        Details::WeakReferenceImpl* weakRef = Details::DecodeWeakReferencePointer(currentValue.rawValue);
        weakRef->Clear();
    }

    // The weak reference target pointer gets cleared when we become unreachable, so we should
    // clear the "reachable" flag.  Otherwise we're in an inconsistent state, specifically the resurrection logic
    // in GetPeerPrivate gets confused thinking that the peer is reachable and therefore the weak reference
    // target pointer doesn't need to be reset.

    m_referenceTrackerBitFields.bReachable = false;
}

TrackerPtr<IUnknown>* FromTrackerHandle(_In_ ::TrackerHandle& arg)
{
    static_assert(std::is_trivially_copyable<::TrackerHandle>::value, "TrackerHandle must be trivially copyable");
    static_assert(std::is_standard_layout<::TrackerHandle>::value, "TrackerHandle must be standard layout");
    return reinterpret_cast<TrackerPtr<IUnknown>*>(arg);
}

TrackerPtr<IUnknown>** FromTrackerHandle(_In_ ::TrackerHandle* arg)
{
    static_assert(std::is_trivially_copyable<::TrackerHandle>::value, "TrackerHandle must be trivially copyable");
    static_assert(std::is_standard_layout<::TrackerHandle>::value, "TrackerHandle must be standard layout");
    return reinterpret_cast<TrackerPtr<IUnknown>**>(arg);
}

// Ensure that the requested handle is registered here
_Check_return_ HRESULT ValidateTrackerOwnership(_In_opt_ std::vector<TrackerTargetReference*>* trackers, _In_ TrackerTargetReference* target)
{
    // TODO: Should I have a better error code here?
    IFCCHECK_RETURN(trackers
        && (std::find(trackers->begin(), trackers->end(), target) != trackers->end()));
    return S_OK;
}

_Check_return_ HRESULT WeakReferenceSourceNoThreadId::CreateTrackerHandle(_Out_ ::TrackerHandle *returnValue)
{
    IFC_RETURN(CheckThread());
    IFCPTR_RETURN(returnValue);

    auto ppResult = FromTrackerHandle(returnValue);
    *ppResult = nullptr;

    // Create a new TrackerTargetReference, and register this as its owner
    std::unique_ptr<TrackerPtr<IUnknown>> temp{ new TrackerPtr<IUnknown> };
    SetPtrValue(*temp, static_cast<IUnknown*>(nullptr));

    *ppResult = temp.release();
    return S_OK;
}

_Check_return_ HRESULT WeakReferenceSourceNoThreadId::DeleteTrackerHandle(_In_ ::TrackerHandle handle)
{
    IFC_RETURN(CheckThread());
    auto pTarget = FromTrackerHandle(handle);
    IFCEXPECTRC_RETURN(pTarget, E_INVALIDARG);

    IFC_RETURN(ValidateTrackerOwnership(m_trackers.get(), pTarget->GetTrackerReference()));

    RemovePtrValue(*pTarget);
    delete pTarget;
    return S_OK;
}

_Check_return_ HRESULT WeakReferenceSourceNoThreadId::SetTrackerValue(
    _In_ ::TrackerHandle handle,
    _In_opt_ IUnknown *value)
{
    IFC_RETURN(CheckThread());
    auto pTarget = FromTrackerHandle(handle);
    IFCEXPECTRC_RETURN(pTarget, E_INVALIDARG);

    IFC_RETURN(ValidateTrackerOwnership(m_trackers.get(), pTarget->GetTrackerReference()));

    // If the tracker isn't safe to use, fail.
    IFCCHECK_RETURN(pTarget->GetTrackerReference()->IsValueSafeToUse(this));

    // Finally, set the value
    pTarget->Set(value);
    return S_OK;
}

_Success_(!!return) _Check_return_ BOOLEAN WeakReferenceSourceNoThreadId::TryGetSafeTrackerValue(
    _In_ ::TrackerHandle handle,
    _COM_Outptr_result_maybenull_ IUnknown **returnValue)
{
    if (!returnValue) {
        return FALSE;
    }
    *returnValue = nullptr;
    if (FAILED(CheckThread())) {
        return FALSE;
    }

    auto pTarget = FromTrackerHandle(handle);
    if (!pTarget) {
        return FALSE;
    }

    if (FAILED(ValidateTrackerOwnership(m_trackers.get(), pTarget->GetTrackerReference()))) {
        return FALSE;
    }

    if (!pTarget->GetTrackerReference()->IsValueSafeToUse(this)) {
        return FALSE;
    }

    // Try to get the value.  If it's there, or it's null, return true.
    return !!pTarget->GetTrackerReference()->TryGetSafeReferenceNullValueReturnsTrue(IID_PPV_ARGS(returnValue));
}