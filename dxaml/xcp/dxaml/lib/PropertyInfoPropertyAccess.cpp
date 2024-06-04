// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyInfoPropertyAccess.h"
#include "CustomDependencyProperty.h"
#include <xstrutil.h>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

_Check_return_ HRESULT
PropertyInfoPropertyAccess::Initialize(
    _In_ IPropertyAccessHost* pOwner,
    _In_ IInspectable* pSource,
    _In_ const CClassInfo* pSourceType,
    _In_ const CCustomProperty* pProperty)
{
    m_pOwner = pOwner;

    SetPtrValue(m_tpSource, pSource);
    m_pProperty = pProperty;
    m_pSourceType = pSourceType;
    return S_OK;
}

PropertyInfoPropertyAccess::~PropertyInfoPropertyAccess()
{
    auto spSource = m_tpSource.GetSafeReference();
    if (spSource)
    {
        IGNOREHR(DisconnectPropertyChangedHandler(spSource.Get()));
    }
}

_Check_return_
HRESULT
PropertyInfoPropertyAccess::GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue)
{
    if (IsConnected())
    {
        IFC_RETURN(m_pProperty->AsOrNull<CCustomProperty>()->GetXamlPropertyNoRef()->GetValue(m_tpSource.Get(), ppValue));
    }
    else
    {
        *ppValue = nullptr;
    }

    return S_OK;
}

_Check_return_
HRESULT
PropertyInfoPropertyAccess::SetValue(_In_ IInspectable *pValue)
{
    HRESULT hr = S_OK;

    IFCEXPECT(IsConnected());

    IFC(m_pProperty->AsOrNull<CCustomProperty>()->GetXamlPropertyNoRef()->SetValue(m_tpSource.Get(), pValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
PropertyInfoPropertyAccess::GetType(_Outptr_ const CClassInfo **ppType)
{
    HRESULT hr = S_OK;

    IFCEXPECT(IsConnected());
    *ppType = m_pProperty->GetPropertyType();

Cleanup:
    RRETURN(hr);
}

bool
PropertyInfoPropertyAccess::IsConnected()
{
    return m_pProperty && m_tpSource && m_pProperty->IsValid();
}

_Check_return_ HRESULT
PropertyInfoPropertyAccess::SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges)
{
    HRESULT hr = S_OK;

    if (fListenToChanges)
    {
        // Disconnect the handler on the previous source, and attach to the new source. Both
        // old and new source are allowed to be NULL.
        IFC(UpdatePropertyChangedHandler(m_tpSource.Get(), pSource));
    }
    else
    {
        IFC(DisconnectEventHandlers());
    }
    SetPtrValue(m_tpSource, pSource);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PropertyInfoPropertyAccess::TryReconnect(_In_ IInspectable* pSource, _In_ BOOLEAN fListenToChanges, _Inout_ BOOLEAN& bConnected, _Inout_ const CClassInfo*& pResolvedType)
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
PropertyInfoPropertyAccess::GetSource(_Outptr_ IInspectable** ppSource)
{
    *ppSource = m_tpSource.Get();
    AddRefInterface(*ppSource);

    RRETURN(S_OK);
}

_Check_return_
HRESULT
PropertyInfoPropertyAccess::DisconnectEventHandlers()
{
    HRESULT hr = S_OK;

    if (m_pProperty && m_tpSource)
    {
        IFC(DisconnectPropertyChangedHandler(m_tpSource.Get()));
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
PropertyInfoPropertyAccess::OnPropertyChanged()
{
    IFC_RETURN(m_pOwner->SourceChanged());
    return S_OK;
}

_Ret_notnull_
const wchar_t*
PropertyInfoPropertyAccess::GetPropertyName()
{
    return m_pOwner->GetPropertyName();
}

_Check_return_ HRESULT PropertyInfoPropertyAccess::CreateInstance(
    _In_ IPropertyAccessHost* pOwner,
    _In_ IInspectable* pSource,
    _In_ const CClassInfo* pSourceType,
    _In_ bool fListenToChanges,
    _Outptr_ PropertyAccess** ppPropertyAccess)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PropertyInfoPropertyAccess> spResult;
    const CDependencyProperty* pProperty = nullptr;
    const WCHAR* pszPropertyName = pOwner->GetPropertyName();

    IFCPTR(pSource);

    *ppPropertyAccess = nullptr; // By default no property is generated

    // We only care about types that are in the metadata because they are explicitly bindable
    // this will remove things like DPs as we need those to be resolved in a specialized form.
    if (!pSourceType->IsBindable())
    {
        goto Cleanup;
    }

    // Now try to resolve the property by name.
    IFC(MetadataAPI::TryGetPropertyByName(pSourceType, XSTRING_PTR_EPHEMERAL2(pszPropertyName, xstrlen(pszPropertyName)), &pProperty));
    if (pProperty == nullptr)
    {
        // We failed to find the property, bail out.
        goto Cleanup;
    }

    // Create a property access object.
    IFC(ctl::make(pOwner, pSource, pSourceType, pProperty->AsOrNull<CCustomProperty>(), &spResult));

    if (fListenToChanges)
    {
        // If we're listenting for changes need to keep the name of the property
        // to compare it against the name of the properties that are changing
        IFC(spResult->AddPropertyChangedHandler(pSource));
    }

    *ppPropertyAccess = spResult.Detach();

Cleanup:
    RRETURN(hr);
}
