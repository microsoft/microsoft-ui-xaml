// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyPropertyProxy.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
DependencyPropertyProxy::CreateObject(
    _In_ IDependencyProperty* pDP,
    _Outptr_ DependencyPropertyProxy** ppProxy)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyPropertyProxy> spProxy;
    const CDependencyProperty* pUnderlyingDP = nullptr;

    IFCPTR(pDP);
    IFCPTR(ppProxy);

    // Create a proxy object.
    IFC(ctl::make(&spProxy));

    pUnderlyingDP = static_cast<DependencyPropertyHandle*>(pDP)->GetDP();
    IFC(spProxy->put_PropertyId(static_cast<INT>(pUnderlyingDP->GetIndex())));

    *ppProxy = spProxy.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyPropertyProxy::GetWrappedDependencyProperty(
    _Outptr_ const CDependencyProperty** ppDP)
{
    HRESULT hr = S_OK;
    XINT32 nID = 0;
    
    IFC(get_PropertyId(&nID));
    *ppDP = MetadataAPI::GetDependencyPropertyByIndex(static_cast<KnownPropertyIndex>(nID));

Cleanup:
    RRETURN(hr);
}
