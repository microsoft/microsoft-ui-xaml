// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComposingTrackerExtensionWrapper.h"
#include "WeakReferenceSourceNoThreadId.h"

/* static */
void DirectUI::ComposingTrackerExtensionWrapper::OnReferenceTrackerWalk(
    _In_ ctl::WeakReferenceSourceNoThreadId* tracker,
    _In_ EReferenceTrackerWalkType walkType)
{
    auto overrides = GetComposingTrackerOverrides(tracker);
    if (overrides)
    {
        overrides->OnReferenceTrackerWalk(static_cast<INT>(walkType));
    }
    else
    {
        tracker->OnReferenceTrackerWalk(walkType);
    }
}


/* static */
bool DirectUI::ComposingTrackerExtensionWrapper::IsComposed(
    _In_ ctl::WeakReferenceSourceNoThreadId* tracker)
{
    auto extension = GetComposingTrackerExtension(tracker);
    if (extension)
    {
        // If we can QI for IReferenceTrackerExtension, then the extension isn't composed
        return false;
    }
    else
    {
        return !!tracker->IsComposed();
    }
}

/* static */
ULONG DirectUI::ComposingTrackerExtensionWrapper::GetActualRefCount(
    _In_ ctl::WeakReferenceSourceNoThreadId* tracker)
{
    // Ensure we've checked the outer for IReferenceTrackerExtension support
    VERIFYHR(ComposingTrackerTargetWrapper::Ensure(tracker));
    
    auto extension = GetComposingTrackerExtension(tracker);    
    if (extension)
    {
        // If a class implements IReferenceTrackerExtension, it's required to implement Release
        // such that the return value is the number of COM refs on the aggregate object.
        // Note: The purpose of this function is to get the object's current refcount without
        // modifying it. If the Release below is bringing the refcount to zero and triggering
        // object destruction, this is not expected and could be a problem.  Some providers will
        // return a "1" from AddRef even if it's not in the process of being destroyed, so we
        // can't just skip the Release in that case.
        tracker->AddRef();
        return tracker->Release();
    }
    else
    {
        return static_cast<ctl::ComBase*>(tracker)->GetRefCount();
    }
}

/* static */
_Check_return_ HRESULT
DirectUI::ComposingTrackerExtensionWrapper::SetExtensionInterfaces(
    _In_ ctl::WeakReferenceSourceNoThreadId* tracker,
    _In_ ::IReferenceTrackerExtension* extension,
    _In_opt_ xaml_hosting::IReferenceTrackerInternal* overrides)
{
    // This method may be called multiple during the tracker construction (and only
    // during its construction).
    // For example, in case of a MUXP object that inherits from another MUXP object
    // which, in turns, inherit from DO, this method will get called twice.
    // We care about the last time it's called because that will give us a reference
    // to the outer most MUXP object.
    
    ASSERT(extension);
    
    IFC_RETURN(tracker->EnsureCompositionWrapper());
    ASSERT(tracker->m_compositionWrapper);

    // It's not necessary to take a GC lock here because this object is being
    // constructed (second phase of the construction) and cannot be walked.

    tracker->m_compositionWrapper->m_composingTrackerOverridesNoRef = overrides;

    return S_OK;
}

/* static */
::IReferenceTrackerExtension*
DirectUI::ComposingTrackerExtensionWrapper::GetComposingTrackerExtension(
    _In_ ctl::WeakReferenceSourceNoThreadId* tracker)
{
    // Ensure we've checked the outer for IReferenceTrackerExtension support
    VERIFYHR(ComposingTrackerTargetWrapper::Ensure(tracker));
    
    return tracker->m_compositionWrapper ?
           tracker->m_compositionWrapper->m_composingTrackerExtensionNoRef: 
           nullptr;
}

/* static */
xaml_hosting::IReferenceTrackerInternal*
DirectUI::ComposingTrackerExtensionWrapper::GetComposingTrackerOverrides(
_In_ ctl::WeakReferenceSourceNoThreadId* tracker)
{
    return tracker->m_compositionWrapper ?
        tracker->m_compositionWrapper->m_composingTrackerOverridesNoRef :
        nullptr;
}
