// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComposingTrackerTargetWrapper.h"
#include "WeakReferenceSourceNoThreadId.h"
#include "AutoReentrantReferenceLock.h"
#include "LifetimeExterns.h"

//+---------------------------------------------------------------------------
//
//  ComposingTrackerTargetWrapper::Ensure
//
//  Deterimine if a DO's outer object is an IReferenceTrackerTarget or IReferenceTrackerExtension.
//  We can't do this during object creation, because we can't call QI at that time.
//
//+---------------------------------------------------------------------------

/* static */
_Check_return_ HRESULT 
DirectUI::ComposingTrackerTargetWrapper::Ensure(
    _In_ ctl::WeakReferenceSourceNoThreadId* tracker)
{
    if (tracker->m_referenceTrackerBitFields.bEnsuredTrackerTarget == false)
    {
        // We haven't already checked

        // See if the outer is an IReferenceTrackerTarget
        IReferenceTrackerTarget *composingTrackerTarget = ctl::query_interface<IReferenceTrackerTarget>(tracker);

        // See if the outer is an IReferenceTrackerExtension
        ctl::ComPtr<IReferenceTrackerExtension> referenceTrackerExtension;
        referenceTrackerExtension.Attach (ctl::query_interface<IReferenceTrackerExtension>(tracker));
        
        // Don't update tracked references while the ReferenceTrackerManager is running
        {
            AutoReentrantReferenceLock lock(DXamlServices::GetDXamlCore());
            tracker->m_referenceTrackerBitFields.bEnsuredTrackerTarget = true;
            if (composingTrackerTarget != nullptr || referenceTrackerExtension)
            {
                IFC_RETURN(tracker->EnsureCompositionWrapper());
                tracker->m_compositionWrapper->m_composingTrackerTarget = composingTrackerTarget;
                tracker->m_compositionWrapper->m_composingTrackerExtensionNoRef = referenceTrackerExtension.Get();
            }
        }
        
        // If it is, we can keep the pointer, but can't keep a COM ref-count on it
        if (composingTrackerTarget != nullptr)
        {
            // A jupiter ref on the Outer CCW ensures that we will not
            // losing the CCW if the DO is finalized and moved into 
            // FinalReleaseQueue. A subsequent GC walk that attempts to 
            // unpeg this CCW will not fail as the CCW is still being protected
            // until the Release Queue is flushed.
            //
            // Reference Win8 Bug: 643960
#if DBG
            tracker->m_compositionWrapper->m_composingTrackerTargetRefCount++;
#endif
            composingTrackerTarget->AddRefFromReferenceTracker();

            // To work around a race condition, peg the target now.  That will protect it until the next round of
            // reference tracking, at which point we'll correct it if necessary.
            IFC_RETURN(composingTrackerTarget->Peg());

            composingTrackerTarget->Release();
        }
    }
    return S_OK;
}

/* static */
bool DirectUI::ComposingTrackerTargetWrapper::IsTrackerTarget(_In_ ctl::WeakReferenceSourceNoThreadId* tracker)
{
    return tracker->m_compositionWrapper ?
           tracker->m_compositionWrapper->m_composingTrackerTarget != nullptr :
           false;
}

/* static */
::IReferenceTrackerTarget *DirectUI::ComposingTrackerTargetWrapper::GetTrackerTarget(
    _In_ const ctl::WeakReferenceSourceNoThreadId* tracker)
{
    #if DBG
    // Don't call out under the lock
    AutoReentrantReferenceLock::AssertIfEntered();
    #endif

    return GetTrackerTargetNoAssertIfEntered(tracker);
}

/* static */
::IReferenceTrackerTarget *DirectUI::ComposingTrackerTargetWrapper::GetTrackerTargetNoAssertIfEntered(
    _In_ const ctl::WeakReferenceSourceNoThreadId* tracker)
{
    return tracker->m_compositionWrapper ?
           tracker->m_compositionWrapper->m_composingTrackerTarget :
           nullptr;
}

// Special IReferenceTrackerTarget form of AddRef
// This special count is necessary for the target to understand what to expect during tracking.
ULONG DirectUI::ComposingTrackerTargetWrapper::AddRefTarget(_In_ const ctl::WeakReferenceSourceNoThreadId* tracker)
{
    auto trackerTarget = GetTrackerTarget(tracker);
    if (trackerTarget)
    {
#if DBG
        tracker->m_compositionWrapper->m_composingTrackerTargetRefCount++;
#endif
        return trackerTarget->AddRefFromReferenceTracker();
    }
    else
    {
        return 0;
    }
}

// Special IReferenceTrackerTarget form of Release
// This special count is necessary for the target to understand what to expect during tracking.
ULONG DirectUI::ComposingTrackerTargetWrapper::ReleaseTarget(_In_ const ctl::WeakReferenceSourceNoThreadId* tracker)
{
    auto trackerTarget = GetTrackerTarget(tracker);
    if (trackerTarget)
    {
#if DBG
        tracker->m_compositionWrapper->m_composingTrackerTargetRefCount--;
#endif
        return trackerTarget->ReleaseFromReferenceTracker();
    }
    else
    {
        return 0;
    }
}
