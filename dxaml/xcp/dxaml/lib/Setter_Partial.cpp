// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Setter.g.h"
#include "DependencyPropertyProxy.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
SetterFactory::CreateInstanceImpl(
    _In_ xaml::IDependencyProperty* pProperty,
    _In_ IInspectable* pValue,
    _Outptr_ xaml::ISetter** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Setter> spSetter;

    IFCPTR(pProperty);
    IFCPTR(ppInstance);

    IFC(ctl::make(&spSetter));
    IFC(spSetter->put_Property(pProperty));
    IFC(spSetter->put_Value(pValue));

    *ppInstance = spSetter.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Setter::get_PropertyImpl(
    _Outptr_result_maybenull_ IDependencyProperty** pValue)
{
    HRESULT hr = S_OK;
    IInspectable* pProxyAsInsp = NULL;
    CValue boxedValue;

    IFC(DependencyObject::GetValueCore(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Setter_Property),
        boxedValue));

    IFC(CValueBoxer::UnboxObjectValue(&boxedValue, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyPropertyProxy), __uuidof(IInspectable), reinterpret_cast<void**>(&pProxyAsInsp)));

    if (pProxyAsInsp == NULL)
    {
        *pValue = NULL;
    }
    else
    {
        const CDependencyProperty* pUnderlyingDP = nullptr;
        DependencyPropertyProxy* pProxy = static_cast<DependencyPropertyProxy*>(static_cast<ComBase*>(pProxyAsInsp));
        IFC(pProxy->GetWrappedDependencyProperty(&pUnderlyingDP));
        IFC(MetadataAPI::GetIDependencyProperty(pUnderlyingDP->GetIndex(), pValue));
    }

Cleanup:
    ReleaseInterface(pProxyAsInsp);
    RRETURN(hr);
}

_Check_return_ HRESULT Setter::put_PropertyImpl(
    _In_opt_ IDependencyProperty* value)
{
    HRESULT hr = S_OK;
    DependencyPropertyProxy* pProxy = NULL;

    IFC(DependencyPropertyProxy::CreateObject(value, &pProxy));
    IFC(DependencyObject::SetValueCore(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Setter_Property), ctl::as_iinspectable(pProxy)));

Cleanup:
    ctl::release_interface(pProxy);
    RRETURN(hr);
}
