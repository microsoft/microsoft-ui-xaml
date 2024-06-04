// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ExternalObjectReference_partial.h"

using namespace DirectUI;

_Check_return_ 
HRESULT 
ExternalObjectReference::ShouldBeWrapped(_In_ IInspectable *pInValue, _Out_ BOOLEAN& returnValue)
{
    HRESULT hr = S_OK;
    IDependencyObject *pObj = NULL;

    if (pInValue)
    {
        returnValue = TRUE;

        if (SUCCEEDED(ctl::do_get_value(pObj, pInValue)))
        {
            returnValue = (static_cast<DependencyObject*>(pObj)->GetHandle() == NULL);
        }
    }
    else
    {
        returnValue = FALSE;
    }

    ReleaseInterface(pObj);
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
ExternalObjectReference::ConditionalWrap(_In_ IInspectable *pInValue, _Outptr_ DependencyObject **ppWrapped, _Out_opt_ BOOLEAN *pWasWrapped)
{
    HRESULT hr = S_OK;
    BOOLEAN wrappingNeeded = FALSE;
    IDependencyObject *pObj = NULL;

    IFC(ShouldBeWrapped(pInValue, wrappingNeeded));

    if (wrappingNeeded)
    {
        IFC(Wrap(pInValue, ppWrapped));
    }
    else
    {
        IFC(ctl::do_get_value(pObj, pInValue));
        *ppWrapped = static_cast<DependencyObject *>(pObj);
        pObj = NULL;
    }  

    if (pWasWrapped != NULL)
    {
        *pWasWrapped = wrappingNeeded;
    }

Cleanup:
    ReleaseInterface(pObj);
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
ExternalObjectReference::Wrap(_In_ IInspectable *pInspectable, _Outptr_ DependencyObject **ppWrapped)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ExternalObjectReference> spMOR;

    IFCPTR(pInspectable);
    IFCPTR(ppWrapped);

    IFC(ctl::make(&spMOR));
    IFC(spMOR->put_Target(pInspectable));

    *ppWrapped = spMOR.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ 
HRESULT
ExternalObjectReference::GetTarget(_In_ CDependencyObject* pNativeDO, _Outptr_ IInspectable** ppTarget)
{
    ctl::ComPtr<DependencyObject> peer;
    ctl::ComPtr<IInspectable> target;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pNativeDO, &peer));
    peer.Cast<ExternalObjectReference>()->get_Target(&target);

    *ppTarget = target.Detach();

    return S_OK;
}