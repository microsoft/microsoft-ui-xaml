// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TrackerTargetReference.h"
#include "WeakReferenceSourceNoThreadId.h"
#include "LifetimeUtils.h"
#include "DependencyObjectAbstractionHelpers.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <FeatureFlags.h>

using namespace DirectUI;

// PReferenceTrackerInternal
void TrackerTargetReference::ReferenceTrackerWalk(EReferenceTrackerWalkType walkType)
{

#if DBG
    if (walkType == RTW_TrackerPtrTest)
    {
        if (m_fHasBeenWalked)
            return;

        m_fHasBeenWalked = true;
    }
    else
    {        
        // This is a debug aid.  To break here every time it's called -- for one particular instance -- set a memory
        // breakpoint on m_pCore.
        
        auto save = m_pCore;
        m_pCore = nullptr;
        m_pCore = save;
    }
#endif

    // The TrackerTarget keeps a Peg on a DO until the tracker target gets walked for the 
    // first time. After a walk, we can rely on the ReferenceTrackerWalk to protect CCWs
    // reachable from the DO, instead of the temporary Peg that the TrackerTarget keeps.
    //
    // Example:
    // * ProgressBar DO
    //   - TemplateSettings: TrackerTarget<IDO>
    //                       - ProgressBarTemplateSettings DO
    //                         - <CCW>
    if (walkType == RTW_Unpeg)
    {
        ClearTrackerPeg();
    }

    // Forward the walk to the target
    if (m_isManagedReference)
    {
        ReferenceTrackerManager::ReferenceTrackerWalk(walkType, GetTrackerTarget());
    }

    else if (m_isTrackerInternal)
    {
        // Can be a DO even if GetTrackerTarget() = nullptr for C++/Hybrid scenarios.
        GetReferenceTracker()->ReferenceTrackerWalk(walkType);
    }
}

void TrackerTargetReference::ClearTrackerPeg()
{
    // Take off any tracker peg which was set up temporarily.
    if (m_trackerPeg && m_isTrackerInternal)
    {
        m_trackerPeg = false;
        GetReferenceTracker()->UpdatePeg(false);
    }
}

#if DBG
bool TrackerTargetReference::IsValueSafeToUse() const
{
    return IsValueSafeToUse(GetOwner());
}
#endif

bool TrackerTargetReference::IsValueSafeToUse(_In_opt_ ctl::WeakReferenceSourceNoThreadId* pOwner) const
{
    if (m_isTrackerInternal &&
        (GetReferenceTracker()->IsPegged(false /*isRefCountPegged*/) || GetReferenceTracker()->IsPegged(true /*isRefCountPegged*/)))
    {
        return true;
    }

    if (!m_isManagedReference && !m_isTrackerInternal)
    {
        return true;
    }

    if (pOwner)
    {
        return !pOwner->IsInFinalRelease();
    }

    return true;
}

bool TrackerTargetReference::IsRegistered() const
{
    return m_isRegistered;
}

#if DBG

void TrackerTargetReference::Register()
{
    if (m_isRegistered)
    {
        IFCFAILFAST(E_UNEXPECTED);
    }

    m_isRegistered = true;
    ReferenceTrackerManager::RegisterTrackerPtr(this, m_pCore);
}

void TrackerTargetReference::Unregister()
{
    if (!m_isRegistered)
    {
        IFCFAILFAST(E_UNEXPECTED);
    }

    m_isRegistered = false;

    ReferenceTrackerManager::UnregisterTrackerPtr(this, m_pCore);
    m_pOwner = nullptr;
}

#endif // DBG

#if DBG
void TrackerTargetReference::Register(ctl::WeakReferenceSourceNoThreadId* pOwner)
{
    Register();

    ASSERT(m_pOwner == nullptr);

    m_pOwner = pOwner;
}

ctl::WeakReferenceSourceNoThreadId* TrackerTargetReference::GetOwner() const
{
    return m_pOwner;
}
#endif

IReferenceTrackerTarget* TrackerTargetReference::GetTrackerTarget() const
{
    ASSERT(m_isManagedReference);
    if (m_isTrackerInternal)
    {
        return m_trackerData->TrackerTarget;
    }
    else
    {
        return m_trackerTarget;
    }
}

xaml_hosting::IReferenceTrackerInternal*
TrackerTargetReference::GetReferenceTracker() const
{
    ASSERT(m_isTrackerInternal);
    if (m_isManagedReference)
    {
        return m_trackerData->ReferenceTracker;
    }
    else
    {
        return m_referenceTracker;
    }
}

void
TrackerTargetReference::SetRawValue(
    _In_ IUnknown* value,
    _In_ IReferenceTrackerTarget* trackerTarget,
    _In_ xaml_hosting::IReferenceTrackerInternal* referenceTracker)
{
    // We don't call addref here, we just set the m_value and m_trackerData field depending
    // on the arguments we get. It's the responsibility of the caller to handle
    // the reference counts.

    ClearRawValue();

    m_isManagedReference = (trackerTarget != nullptr);
    m_isTrackerInternal = (referenceTracker != nullptr);

    if (m_isTrackerInternal && m_isManagedReference)
    {
        auto data = new ReferenceTrackerTargetData();

        data->TrackerTarget = trackerTarget;
        data->ReferenceTracker = referenceTracker;
        m_trackerData = data;
    }
    else if (m_isTrackerInternal)
    {
        m_referenceTracker = referenceTracker;
    }
    else if (m_isManagedReference)
    {
        m_trackerTarget = trackerTarget;
    }

    m_value = value;
}

void TrackerTargetReference::ClearRawValue()
{
    // We don't call release here, we just clear the m_value and m_trackerData field.
    // It's the responsibility of the caller to handle the reference counts.

    if (m_isTrackerInternal && m_isManagedReference)
    {
        ASSERT(m_trackerData);
        delete m_trackerData;
    }

    m_value = nullptr;
    m_trackerData = nullptr;
    m_isTrackerInternal = false;
    m_isManagedReference = false;
}

TrackerTargetReference::TrackerTargetReference()
{
    XCP_STRONG(&m_value);

    Initialize();
}

TrackerTargetReference::TrackerTargetReference(TrackerTargetReference&& other) noexcept
{
    XCP_STRONG(&m_value);
    Initialize();

    *this = std::move(other);
}

TrackerTargetReference::~TrackerTargetReference()
{
    ClearRawValue();

#if DBG
    if (IsRegistered())
    {
        Unregister();
    }
#endif

}

void TrackerTargetReference::Initialize()
{
#if DBG
    // Must be set from the template (before calling base Initialize) because it's a forward reference.
    m_pCore = DXamlServices::GetDXamlCore();
#endif

    m_isDO = false;
    m_trackerPeg = false;
    m_isRegistered = false;
    m_extraExpectedRef = false;
    m_isTrackerInternal = false;
    m_isManagedReference = false;

#if DBG
    m_fStatic = false;
    m_fHasBeenWalked = false;
    m_pOwner = nullptr;
#endif

#if DBG
    if(RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::CaptureTrackerPtrCallStack))
    {
        CaptureStackBackTrace(
            0, // skip 0 frames
            ARRAY_SIZE(m_frameAddresses),
            m_frameAddresses,
            nullptr);
    }
#endif

    m_value = nullptr;
    m_trackerData = nullptr;
}

// Note: This method is safe to call under the lock 
// if this object is empty
TrackerTargetReference& TrackerTargetReference::Assign(_In_ TrackerTargetReference&& other) noexcept
{
    // If this tracker is not empty then we can't call this 
    // method under the lock or we will stop responding
#if DBG
    if (GetNoMptStress() != nullptr)
    {
        AutoReentrantReferenceLock::AssertIfEntered();
    }
#endif

    if (*this != other)
    {
#if DBG
        ASSERT(IsValueSafeToUse());
#endif

        Clear();

        {
            AutoReentrantReferenceLock lock(DXamlServices::GetDXamlCore());

            m_value = other.m_value;
            m_trackerData = other.m_trackerData;
            m_trackerPeg = other.m_trackerPeg;
            m_isDO = other.m_isDO;
            m_extraExpectedRef = other.m_extraExpectedRef;
            m_isTrackerInternal = other.m_isTrackerInternal;
            m_isManagedReference = other.m_isManagedReference;

#if DBG
            m_fStatic = other.m_fStatic;
            m_fHasBeenWalked = other.m_fHasBeenWalked;
#endif

            // Cleanup the other object as we're moving the references here
            other.m_value = nullptr;
            other.m_trackerData = nullptr;
            other.m_trackerPeg = false;
            other.m_isDO = false;
            other.m_extraExpectedRef = false;
            other.m_isTrackerInternal = false;
            other.m_isManagedReference = false;

#if DBG
            other.m_fStatic = false;
            other.m_fHasBeenWalked = false;
#endif
        }
    }

    return *this;
}


bool TrackerTargetReference::operator==(const TrackerTargetReference& other) const
{
    return GetNoMptStress() == other.GetNoMptStress();
}

bool TrackerTargetReference::operator!=(const TrackerTargetReference& other) const
{
    return GetNoMptStress() != other.GetNoMptStress();
}

void TrackerTargetReference::Set(
    _In_ IUnknown* const pValue
#if DBG
    , bool fStatic
#endif
    )
{
    using namespace xaml_hosting;

    IInspectable                         *punk = nullptr;
    bool                                  fPegged = false;
    bool                                  fIsDO = false;
    IUnknown                             *pNewValue = pValue;
    xaml::IDependencyObject *pNewDO = nullptr;
    IReferenceTrackerTarget              *pNewTrackerTarget = nullptr;
    IReferenceTrackerInternal            *pNewReferenceTracker = nullptr;

#if DBG
    ASSERT(IsValueSafeToUse());
#endif

#if DBG
    m_fStatic = fStatic;
#endif

#if XCP_MONITOR
    if (ReferenceTrackerManager::IsPeerStressEnabled())
    {
        ReferenceTrackerManager::TriggerCollection();
    }
#endif

    Clear();

    // Save the direct reference to the IDependencyObject if available, because we might not
    // be able to QI for this later.  Don't keep a COM reference on it though, rely on m_value 
    // (or later, GetTrackerTarget()) to do that.
    pNewDO = ctl::query_interface<xaml::IDependencyObject>(pNewValue);
    fIsDO = (pNewDO != nullptr);
    ctl::release_interface_nonull(pNewDO);
    pNewReferenceTracker = ctl::query_interface<IReferenceTrackerInternal>(pNewValue);
    ctl::release_interface_nonull(pNewReferenceTracker);

    // We do, however, need to keep ref on the inner object (the actual DependencyObject), or it could
    // be gone when we need it during destruction in DependencyObject::LeaveImpl
    ctl::addref_interface_inner(DependencyObjectAbstractionHelpers::IDOtoWRSNTI(pNewDO));
    if (pNewDO)
    {
        ctl::addref_expected(pNewDO, ctl::ExpectedRef_Tree);
    }

    // See if the value is an IReferenceTrackerTarget
    pNewTrackerTarget = ctl::query_interface<IReferenceTrackerTarget>(pNewValue);

    //
    // keep a temporary peg to mark reachability until we add it as part of the tree walk
    // when we set GetTrackerTarget() = pNewTrackerTarget
    //
    if (pNewReferenceTracker && pNewReferenceTracker->IsReferenceTrackerPegSet())
    {
        pNewReferenceTracker->UpdatePeg(true);
        fPegged = true;
    }

    // If we didn't get a tracker target for the value, keep the value with a COM ref count
    if (pNewTrackerTarget == nullptr)
    {
        ctl::addref_interface(pNewValue);

        // If this is some kind if IReferenceTrackerInternal (e.g. a DO),
        // and it's not being composed, then the above addref_interface needs to be counted as an expected reference
        // (so as not to cause an implicit peg).
        if (pNewReferenceTracker != nullptr &&
            !ComposingTrackerExtensionWrapper::IsComposed(ctl::impl_cast<ctl::WeakReferenceSourceNoThreadId>(pNewReferenceTracker)))
        {
            ctl::addref_expected(pNewReferenceTracker, ctl::ExpectedRef_Tree);

            // Remember to remove this in Clear();
            m_extraExpectedRef = true;
        }
    }
    // Keep a special tracking ref count to the tracker target, rather than
    // a COM ref count.
    else
    {
        // Also peg the target to ensure protection until the next GC cycle.  This must not be done until 
        // after the AddRefFromReferenceTracker call (otherwise it causes an assert in the CLR).

        pNewTrackerTarget->AddRefFromReferenceTracker();
        pNewTrackerTarget->Peg();

        // Don't do pNewTrackerTarget->Release yet, do it in cleanup after it's in GetTrackerTarget()
    }

    // Swap the new values in under protection of the lock
    {
        AutoReentrantReferenceLock lock(DXamlServices::GetDXamlCore());

        SetRawValue(pNewValue, pNewTrackerTarget, pNewReferenceTracker);
        pNewValue = nullptr;

        m_isDO = fIsDO;
        pNewDO = nullptr;

        // Don't null pNewTrackerTarget, we still need to Release the ref it got from the QI.
        pNewReferenceTracker = nullptr;

        if (fPegged)
        {
            ASSERT(!m_trackerPeg);
            m_trackerPeg = fPegged;
        }
    }


    // clear the reference tracker peg that is set to temporarily set to protect the object from
    // clearing its weak reference. Since the object is now reachable through a tracker walk
    // we clear the reference tracker peg.
    if (m_isTrackerInternal)
    {
        GetReferenceTracker()->ClearReferenceTrackerPeg();
    }

    ctl::release_interface(punk);

    DependencyObject* const pDORelease = DependencyObjectAbstractionHelpers::IDOtoDO(pNewDO);
    ctl::release_interface_inner(DependencyObjectAbstractionHelpers::DOtoWRSNTI(pDORelease));

    ctl::release_interface(pNewReferenceTracker);
    ctl::release_interface(pNewTrackerTarget);
}

IUnknown* TrackerTargetReference::Get() const
{
#if DBG
    AutoReentrantReferenceLock::AssertIfEntered();
#endif

#if XCP_MONITOR
    if (ReferenceTrackerManager::IsPeerStressEnabled())
    {
        ReferenceTrackerManager::TriggerCollection();
    }
#endif

    return GetNoMptStress();
}

//
// Call this method to peg the target of this TrackerPtr, if the target is still reachable.
// If it's not reachable, the returned AutoPeg behaves as a nullptr.  When the returned
// AutoPeg falls out of scope it will release the peg.
//
// This is primarily useful during destruction of an object, where it wants to clean its state
// out of referenced objects, but needn't/shouldn't if those objects are not reachable
// (i.e. they have been GC'd).
//
ctl::AutoPeg<xaml_hosting::IReferenceTrackerInternal, true> TrackerTargetReference::TryMakeAutoPeg()
{
    return ctl::AutoPeg<xaml_hosting::IReferenceTrackerInternal, true>(
        GetAsReferenceTrackerUnsafe(),
        m_value != nullptr // If GetReferenceTracker is nullptr, and we have a value, assume the peg was successful
        );
}

xaml::IDependencyObject* TrackerTargetReference::GetAsDO() const
{
    if (m_isDO)
    {
#if DBG
        ASSERT(IsValueSafeToUse());
#endif
        return DependencyObjectAbstractionHelpers::IRTItoIDO(GetReferenceTracker());
    }

    return nullptr;
}

// Get an unsafe reference to the IReferenceTrackerInternal that we QI'd in Set.
// It may be in a GC'd (but not finalized) state, and no ref-count is taken on it.
xaml_hosting::IReferenceTrackerInternal* TrackerTargetReference::GetAsReferenceTrackerUnsafe() const
{
    return m_isTrackerInternal ? GetReferenceTracker() : nullptr;
}

CDependencyObject* TrackerTargetReference::GetAsCoreDO() const
{
    if (m_isDO)
    {
        // In certain cases during finalization, we need to retrieve the Core DO
        // for cleanup. Although it is not safe to use the TrackerPtr's reference,
        // if it was a DO, it is safe to use the CoreDO because there is no interaction
        // with outer objects.
        DependencyObject *pDO = DependencyObjectAbstractionHelpers::IRTItoDO(GetReferenceTracker());
        if (pDO)
        {
            return DependencyObjectAbstractionHelpers::GetHandle(pDO);
        }
    }

    return nullptr;
}

void TrackerTargetReference::Clear()
{
    IDXamlCore *pCore = nullptr;
    IReferenceTrackerTarget *pTrackerTarget = nullptr;
    xaml_hosting::IReferenceTrackerInternal *pTrackerInternal = nullptr;
    DependencyObject *pDO = nullptr;
    IUnknown* pValue = nullptr;

    // Early out if there's nothing to do            
    if (!m_isManagedReference && m_value == nullptr)
    {
        ASSERT(!m_isTrackerInternal);
        return;
    }

    // Clear temporary tracker peg on the DO.
    IGNOREHR(ClearTrackerPeg());

    // Get the core, but don't create it; if there isn't one, it means that we're on the 
    // finalizer thread.  This only happens during shutdown, when the original UI thread
    // and its core have already gone away.  This check is a workaround until we get that resolved (103169).

    pCore = DXamlServices::GetDXamlCore();
    if (pCore == nullptr)
    {
        ClearWithNoCallouts(&pTrackerTarget, &pTrackerInternal, &pDO, &pValue);
    }
    else
    {
        // We have a core, we can use its lock
        AutoReentrantReferenceLock lock(pCore);
        ClearWithNoCallouts(&pTrackerTarget, &pTrackerInternal, &pDO, &pValue);
    }

    // If we did an extra ctl::addref_expected in Set(), remove it.
    if (m_extraExpectedRef)
    {
        ASSERT(pTrackerInternal != nullptr);
        ctl::release_expected(pTrackerInternal);
        m_extraExpectedRef = false;
    }

    // Release the previous values

    if (pDO)
    {
        ctl::release_expected(DependencyObjectAbstractionHelpers::DOtoIRTI(pDO));
    }
    ctl::release_interface_inner(DependencyObjectAbstractionHelpers::DOtoWRSNTI(pDO));

    ASSERT(pValue == nullptr || pTrackerTarget == nullptr);

    if (pValue != nullptr)
    {
        ctl::release_interface_nonull(pValue);
    }
    else if (pTrackerTarget != nullptr)
    {
        pTrackerTarget->ReleaseFromReferenceTracker();
    }
}

// This version of Clear does  not make any calls out.  This enables the fields to be cleared while under the lock, without
// risking deadlock; the calls out could block on GC, which on another thread is in ReferenceTrackerManager trying to take the lock.
void TrackerTargetReference::ClearWithNoCallouts(
    _Out_ IReferenceTrackerTarget **ppPreviousTrackerTarget,
    _Out_ xaml_hosting::IReferenceTrackerInternal **ppPreviousTrackerInternal,
    _Out_ DependencyObject **ppPreviousDO,
    _Out_ IUnknown **ppPreviousValue)
{
    *ppPreviousTrackerInternal = GetAsReferenceTrackerUnsafe();
    *ppPreviousDO = (m_isDO ? DependencyObjectAbstractionHelpers::IRTItoDO(GetReferenceTracker()) : nullptr);
    *ppPreviousValue = nullptr;

    // Clear references, using special ref-counting if it's a tracker target.
    if (m_isManagedReference)
    {
        *ppPreviousTrackerTarget = GetTrackerTarget();
    }
    else if (IsSet())
    {
        *ppPreviousValue = m_value;
    }

    ClearRawValue();
}

IUnknown* TrackerTargetReference::GetNoMptStress() const
{
#if DBG
    ASSERT(IsValueSafeToUse());
#endif
    return m_value;
}

// Variation of TryGetSafeReference that returns *false* if the value is null.
bool TrackerTargetReference::TryGetSafeReference(_In_ REFIID riid, _Outptr_result_maybenull_ void** safeReference) const
{
    auto safe = TryGetSafeReferenceNullValueReturnsTrue (riid, safeReference);
    return safe && *safeReference;
}


// Variation of TryGetSafeReference that returns *true* if the value is null.
// This should become the only version, but keeping the other overload to avoid regressing callers that don't expect to get nullptr back.
bool TrackerTargetReference::TryGetSafeReferenceNullValueReturnsTrue(_In_ REFIID riid, _Outptr_result_maybenull_ void** ppSafeReference) const
{
    void* pResult = nullptr;
    bool fResult = false;
    auto nullValue = false;

    if (m_isTrackerInternal)
    {
        DirectUI::AutoReentrantReferenceLock lock(DXamlServices::GetDXamlCore());
        if (!DXamlServices::IsDXamlCoreShutdown())
        {
            if (GetReferenceTracker()->IsReachable())
            {
                // At this point the object is reachable so we can just use this interface
                // to QI and therefore get a safe reference out of it
                if (FAILED(GetReferenceTracker()->QueryInterface(riid, &pResult)))
                {
                    pResult = nullptr;
                }
            }
        }
    }
    
    else if (m_isManagedReference)
    {
        // A failure to QI on this interface, any failure will mean 
        // that either U is not supported or that the object is neutered
        // in both cases we can't get a safe reference to the value
        if (FAILED(GetTrackerTarget()->QueryInterface(riid, &pResult)))
        {
            pResult = nullptr;
        }
    }
    
    else if (m_value)
    {
        // The TrackerPtr is behaving just as a ComPtr at this point 
        // just do the QI for the requested interface
        if (FAILED(m_value->QueryInterface(riid, &pResult)))
        {
            pResult = nullptr;
        }
    }
    
    else
    {
        // The value is null, which is valid
        nullValue = true;
    }

    // The result is valid if we have a safe reference either
    // by just addrefing or by getting it from the tracker target
    fResult = (pResult != nullptr) || nullValue;

    // Return it
    *ppSafeReference = pResult;

    return fResult;
}
