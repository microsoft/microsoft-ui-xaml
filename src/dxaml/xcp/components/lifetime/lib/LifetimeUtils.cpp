// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WeakReferenceSourceNoThreadId.h"
#include "LifetimeUtils.h"

using namespace DirectUI;
using namespace ctl;
using namespace xaml_hosting;

_Check_return_ HRESULT  ctl::as_weakref(_Outptr_ IWeakReference *&pWeakRef, _In_ IInspectable *pInput)
{
    HRESULT hr = S_OK;
    IWeakReferenceSource *pWS = NULL;

    if (pInput)
    {
        IFC(ctl::do_query_interface(pWS, pInput));
        IFC(pWS->GetWeakReference(&pWeakRef));
    }
    else
    {
        pWeakRef = NULL;
    }

Cleanup:

    ReleaseInterface(pWS);

    RRETURN(hr);
}

AutoPeg<xaml_hosting::IReferenceTrackerInternal, TRUE> ctl::try_make_autopeg(_In_opt_ xaml_hosting::IReferenceTrackerInternal* pTracker)
{
    return AutoPeg<xaml_hosting::IReferenceTrackerInternal, TRUE>(pTracker);
}

AutoPeg<xaml_hosting::IReferenceTrackerInternal, TRUE> ctl::try_make_autopeg(_In_opt_ ctl::WeakReferenceSourceNoThreadId* pTracker)
{
    return ctl::try_make_autopeg(ctl::interface_cast<IReferenceTrackerInternal>(pTracker));
}

void  ctl::addref_expected(_In_opt_ xaml_hosting::IReferenceTrackerInternal* pInterface, _In_ ExpectedRefType refType)
{
    if (pInterface)
    {
        pInterface->UpdateExpectedRefCount(RefCountUpdateType::Add);
    }
}

void  ctl::release_expected(_In_opt_ xaml_hosting::IReferenceTrackerInternal* pInterface)
{
    if (pInterface)
    {
        pInterface->UpdateExpectedRefCount(RefCountUpdateType::Remove);
    }
}

void  ctl::addref_expected(_In_opt_ IInspectable* pInterface, _In_ ExpectedRefType refType)
{
    xaml_hosting::IReferenceTrackerInternal *pTrackerInternal = ctl::query_interface<xaml_hosting::IReferenceTrackerInternal>(pInterface);
    ctl::addref_expected(pTrackerInternal, refType);
    ReleaseInterface(pTrackerInternal);
}

void  ctl::release_expected(_In_opt_ IInspectable* pInterface)
{
    xaml_hosting::IReferenceTrackerInternal *pTrackerInternal = ctl::query_interface<xaml_hosting::IReferenceTrackerInternal>(pInterface);
    ctl::release_expected(pTrackerInternal);
    ReleaseInterface(pTrackerInternal);
}

void  ctl::addref_expected(_In_opt_ WeakReferenceSourceNoThreadId* pObj, _In_ ExpectedRefType refType)
{
    addref_expected(ctl::interface_cast<IReferenceTrackerInternal>(pObj), refType);
}
void  ctl::release_expected(_In_opt_ WeakReferenceSourceNoThreadId* pObj)
{
    release_expected(ctl::interface_cast<IReferenceTrackerInternal>(pObj));
}
