// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WeakReferenceSource.h"
#include "LifetimeExterns.h"

using namespace DirectUI;
using namespace ctl;
using namespace xaml_hosting;

WeakReferenceSource::WeakReferenceSource()
{
    m_uThreadId = ::GetCurrentThreadId();
}

_Check_return_ HRESULT WeakReferenceSource::Initialize()
{
    IFC_RETURN(WeakReferenceSourceNoThreadId::Initialize());

    // Add to the reference tracking list.  (It's OK to add while DXamlServices is still initializing; DXamlCore::Initialize creates objects.)
    if (DirectUI::DXamlServices::IsDXamlCoreInitialized() || DirectUI::DXamlServices::IsDXamlCoreInitializing())
    {
        IFC_RETURN(CheckThread());

        // Set the create-time peg on this object, so that it doesn't get garbage collected before
        // it's fully initialized.
        SetReferenceTrackerPeg();

        // Add to the reference tracking management
        AddToReferenceTrackingList();
    }

    return S_OK;

}

void WeakReferenceSource::OnFinalRelease()
{
    if (OnFinalReleaseOffThread())
    {
        // the object will have posted for
        // a release on the UI thread
        return;
    }

    // if DXamlCore has already been destroyed, don't re-create it just to remove our peer mapping
    // (the peer mapping will have already been removed when DXamlCore was deinitialized)
    IDXamlCore* pCore = DXamlServices::GetDXamlCore();
    if (pCore)
    {
        // Take this object out of the ReferenceTrackingTable using by the ReferenceTrackerManager
        pCore->RemoveFromReferenceTrackingList(ctl::interface_cast<IReferenceTrackerInternal>(this));
    }

    ctl::WeakReferenceSourceNoThreadId::OnFinalRelease();
}

_Check_return_ HRESULT WeakReferenceSource::CheckThread() const
{
    if (m_uThreadId != ::GetCurrentThreadId())
    {
        RRETURN(RPC_E_WRONG_THREAD);
    }
    // TODO: (WIN8#719042) Enable this once we cleaned up our destructors..
    /*else if (!DXamlServices::IsDXamlCoreInitialized())
    {
        RRETURN(RPC_E_WRONG_THREAD);
    }*/
    RRETURN(S_OK);
}

IDXamlCore* WeakReferenceSource::GetCoreForObject()
{
    return ReferenceTrackerManager::SearchCoresForObject(ctl::interface_cast<IReferenceTrackerInternal>(this));
}


HRESULT WeakReferenceSource::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ctl::WeakReferenceSource)))
    {
        *ppObject = static_cast<ctl::WeakReferenceSource*>(this);
    }
    else
    {
        RRETURN(__super::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}
