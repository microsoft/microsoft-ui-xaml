// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyObjectPropertyAccess.h"
#include <xstrutil.h>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

DependencyObjectPropertyAccess::DependencyObjectPropertyAccess():
    m_pOwner(NULL),
    m_pSourceType(NULL)
{ }

DependencyObjectPropertyAccess::~DependencyObjectPropertyAccess()
{
    IGNOREHR(SafeRemovePropertyChangedHandler());
}

void
DependencyObjectPropertyAccess::Initialize(
    _In_ IPropertyAccessHost* const pOwner,
    _In_ IInspectable* const pSource,
    _In_ const CClassInfo* const pSourceType,
    _In_ const CDependencyProperty* const pProperty)
{
    m_pProperty = pProperty;
    SetPtrValue(m_tpSource, pSource);
    m_pSourceType = pSourceType;
    m_pOwner = pOwner;
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::CreateInstance(
    _In_ IPropertyAccessHost *pOwner, 
    _In_ IInspectable *pInspSource, 
    _In_ const CClassInfo *pSourceType,
    _In_ bool fListenToChanges,
    _Outptr_ PropertyAccess **ppPropertyAccess)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spSource;
    const CDependencyProperty* pProperty = nullptr;

    // By default there's no property access
    *ppPropertyAccess = NULL;

    // First resolve the property
    IFC(GetSource(pInspSource, spSource.ReleaseAndGetAddressOf()));
    if (spSource != NULL)
    {
        IFC(ResolvePropertyName(spSource.Get(), pSourceType, pOwner->GetPropertyName(), &pProperty));
    }

    // If we didn't find the property then we're done
    if (!pProperty)
    {
        goto Cleanup;
    }

    IFC(CreateInstance(pOwner, pInspSource, pSourceType, pProperty, fListenToChanges, ppPropertyAccess));

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::CreateInstance(
    _In_ IPropertyAccessHost *pOwner, 
    _In_ IInspectable *pInspSource, 
    _In_ const CClassInfo *pSourceType,
    _In_ const CDependencyProperty* pDP,
    _In_ bool fListenToChanges,
    _Outptr_ PropertyAccess **ppPropertyAccess)
{
    UNREFERENCED_PARAMETER(pSourceType);

    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObjectPropertyAccess> spResult;
    
    // By default there's no property access
    *ppPropertyAccess = NULL;
    
    IFC(ctl::make<DependencyObjectPropertyAccess>(&spResult));
    spResult->Initialize(pOwner, pInspSource, pSourceType, pDP);

    if (fListenToChanges)
    {
       IFC(spResult->AddPropertyChangedHandler());
    }

    *ppPropertyAccess = spResult.Detach();

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::AddPropertyChangedHandler()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spSource;

    IFC(GetSource(spSource.ReleaseAndGetAddressOf()));

    if (!spSource)
    {
        goto Cleanup;
    }

    IFC(m_epSyncHandler.AttachEventHandler(spSource.Get(),
        [this](_In_ xaml::IDependencyObject *sender, _In_ const CDependencyProperty* pDP)
        {
            return this->PropertyAccessPathStepDPChanged(pDP);
        }));

    // Only FrameworkElement(s) have the ability of raising
    // core property events

Cleanup:

    RRETURN(hr);
}



_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::PropertyAccessPathStepDPChanged(_In_ const CDependencyProperty* pDP)
{
    HRESULT hr = S_OK;

    if (pDP->GetIndex() == m_pProperty->GetIndex())
    {
        IFC(SourceDPChanged());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::SafeRemovePropertyChangedHandler()
{
    HRESULT hr = S_OK;

    if (m_epSyncHandler)
    {
        ctl::ComPtr<DependencyObject> spSource;
        IFC(SafeGetSource(&spSource));

        if (spSource)
        {
            IFC(m_epSyncHandler.DetachEventHandler(ctl::iinspectable_cast(spSource.Get())));
        }
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue)
{
    ctl::ComPtr<DependencyObject> spSource;

    *ppValue = nullptr;
    IFC_RETURN(GetSource(spSource.ReleaseAndGetAddressOf()));

    if (spSource && IsConnected())
    {
        if (   !m_pProperty->ShouldBindingGetValueUseCheckOnDemandProperty()
            || !spSource->GetHandle()->CheckOnDemandProperty(m_pProperty).IsNull())
        {
            IFC_RETURN(spSource->GetValue(m_pProperty, ppValue));
        }
    }

    return S_OK;
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::SetValue(_In_  IInspectable *pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spSource;

    IFC(GetSource(spSource.ReleaseAndGetAddressOf()));

    if (!spSource)
    {
        goto Cleanup;
    }

    IFCEXPECT(IsConnected());

    IFC(spSource->SetValue(m_pProperty, pValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::GetType(_Outptr_ const CClassInfo **ppType)
{
    HRESULT hr = S_OK;

    IFCEXPECT(IsConnected());
    *ppType = m_pProperty->GetPropertyType();

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyObjectPropertyAccess::SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges)
{
    HRESULT hr = S_OK;

    IFC(DisconnectEventHandlers());
    SetPtrValue(m_tpSource, pSource);

    if (fListenToChanges)
    {
        IFC(AddPropertyChangedHandler());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyObjectPropertyAccess::TryReconnect(_In_ IInspectable* pSource, _In_ BOOLEAN fListenToChanges, _Inout_ BOOLEAN& bConnected, _Inout_ const CClassInfo*& pResolvedType)
{
    HRESULT hr = S_OK;

    bConnected = FALSE;

    IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(pSource, &pResolvedType));

    if (m_pSourceType == pResolvedType)
    {
        IFC(SetSource(pSource, fListenToChanges));
        bConnected = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::GetSource(_Outptr_ IInspectable **ppSource)
{
    HRESULT hr = S_OK;

    IFCEXPECT(IsConnected());
    
    *ppSource = m_tpSource.Get();
    AddRefInterface(*ppSource);

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::DisconnectEventHandlers()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spSource;

    if (m_tpSource)
    {
        IFC(GetSource(spSource.ReleaseAndGetAddressOf()));

        if (m_epSyncHandler)
        {
            IFC(m_epSyncHandler.DetachEventHandler(ctl::iinspectable_cast(spSource.Get())));
        }
    }

Cleanup:

    RRETURN(hr);
}



_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::SourceDPChanged()
{
    HRESULT hr = S_OK;

    IFC(m_pOwner->SourceChanged());

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::GetSource(
    _In_ IInspectable *pSource, 
    _Outptr_ DependencyObject **ppSource)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spObj;

    spObj.Attach(ValueWeakReference::get_value_as<xaml::IDependencyObject>(pSource));
    *ppSource = static_cast<DependencyObject*>(spObj.Detach());

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::GetSource(_Outptr_ DependencyObject **ppSource)
{
    return GetSource(m_tpSource.Get(), ppSource);
}

_Check_return_ 
HRESULT 
DependencyObjectPropertyAccess::SafeGetSource(_Outptr_ DependencyObject **ppSource)
{
    HRESULT hr = S_OK;
    auto spSource = m_tpSource.GetSafeReference();

    *ppSource = NULL;

    if (spSource)
    {
        IFC(GetSource(spSource.Get(), ppSource));
    }
    

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT 
DependencyObjectPropertyAccess::ResolvePropertyName(
    _In_ DependencyObject *pSource, 
    _In_ const CClassInfo* pSourceType,
    _In_z_ WCHAR *szPropertyName, 
    _Outptr_ const CDependencyProperty** ppDP)
{
    HRESULT hr = S_OK;

    IFC(MetadataAPI::TryGetDependencyPropertyByName(
        pSourceType,
        XSTRING_PTR_EPHEMERAL2(
            szPropertyName,
            xstrlen(szPropertyName)),
        ppDP));

Cleanup:
    RRETURN(hr);
}

