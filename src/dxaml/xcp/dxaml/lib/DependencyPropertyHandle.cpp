// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyPropertyHandle.h"
#include "PropertyMetadata.g.h"
#include "CustomDependencyProperty.h"
#include "StaticStore.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT DependencyPropertyHandle::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(DependencyPropertyHandle)))
    {
        *ppObject = static_cast<DependencyPropertyHandle*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(IDependencyProperty)))
    {
        *ppObject = static_cast<IDependencyProperty*>(this);
    }
    else
    {
        return ctl::SupportErrorInfo::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Returns a fully initialized DP.
const CDependencyProperty* DependencyPropertyHandle::GetDP()
{
    if (!m_pDP->IsInitialized())
    {
        if (auto customDependencyProperty = m_pDP->AsOrNull<CCustomDependencyProperty>())
        {
            // If initialization fails, the app is in an inconsistent state. Terminate the process.
            IFCFAILFAST(const_cast<CCustomDependencyProperty*>(customDependencyProperty)->EnsureTypesResolved());
        }
    }

    return m_pDP;
}

IFACEMETHODIMP DependencyPropertyHandle::GetMetadata(
    _In_ wxaml_interop::TypeName type,
    _Outptr_ xaml::IPropertyMetadata** ppMetadata)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spDefaultValue;

    const CDependencyProperty* pDP = GetDP();

    if (auto customDependencyProperty = pDP->AsOrNull<CCustomDependencyProperty>())
    {
        IFC(customDependencyProperty->GetDefaultValue(&spDefaultValue));
    }
    else
    {
        CValue defaultValue;
        const CClassInfo* pType = nullptr;

        IFC(MetadataAPI::GetClassInfoByTypeName(type, &pType));
        IFC(pDP->GetDefaultValue(DXamlCore::GetCurrent()->GetHandle(), /* pReferenceObject */ nullptr, pType, &defaultValue));

        // Unbox default value.
        IFC(CValueBoxer::UnboxObjectValue(&defaultValue, pDP->GetPropertyType(), &spDefaultValue));

        if (spDefaultValue == nullptr)
        {
            IFC(DependencyPropertyFactory::GetUnsetValue(&spDefaultValue));
        }
    }

    // Create property metadata.
    IFC(PropertyMetadata::Create(spDefaultValue.Get(), ppMetadata));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyPropertyFactory::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(IDependencyPropertyStatics)))
    {
        *ppObject = static_cast<IDependencyPropertyStatics*>(this);
    }
    else
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

IFACEMETHODIMP DependencyPropertyFactory::get_UnsetValue(_Out_ IInspectable** ppValue)
{
    RRETURN(DependencyPropertyFactory::GetUnsetValue(ppValue));
}

_Check_return_ HRESULT DependencyPropertyFactory::RegisterStatic(
    _In_ HSTRING hName,
    _In_ wxaml_interop::TypeName propertyType,
    _In_ wxaml_interop::TypeName ownerType,
    _In_opt_ IPropertyMetadata* pDefaultMetadata,
    _Outptr_ IDependencyProperty** ppProperty)
{
    RRETURN(Register(/* bIsAttached */ FALSE, hName, propertyType, ownerType, pDefaultMetadata, /* bIsReadOnly */ FALSE, ppProperty));
}

_Check_return_ HRESULT DependencyPropertyFactory::RegisterAttachedStatic(
    _In_ HSTRING hName,
    _In_ wxaml_interop::TypeName propertyType,
    _In_ wxaml_interop::TypeName ownerType,
    _In_opt_ IPropertyMetadata* pDefaultMetadata,
    _Outptr_ IDependencyProperty** ppProperty)
{
    RRETURN(Register(/* bIsAttached */ TRUE, hName, propertyType, ownerType, pDefaultMetadata, /* bIsReadOnly */ FALSE, ppProperty));
}

_Check_return_ HRESULT DependencyPropertyFactory::Register(
    _In_ BOOLEAN bIsAttached,
    _In_ HSTRING hName,
    _In_ wxaml_interop::TypeName propertyType,
    _In_ wxaml_interop::TypeName ownerType,
    _In_opt_ IPropertyMetadata* pDefaultMetadata,
    _In_ BOOLEAN bIsReadOnly,
    _Outptr_ IDependencyProperty** ppProperty)
{
    RRETURN(MetadataAPI::RegisterDependencyProperty(!!bIsAttached, hName, propertyType, ownerType, pDefaultMetadata, !!bIsReadOnly, ppProperty));
}

_Check_return_ HRESULT DependencyPropertyFactory::GetUnsetValue(_Out_ IInspectable** ppValue)
{
    RRETURN(StaticStore::GetUnsetValue(ppValue));
}

_Check_return_ HRESULT DependencyPropertyFactory::IsUnsetValue(_In_ IInspectable* pValue, _Out_ BOOLEAN& isUnsetValue)
{
    HRESULT hr = S_OK;
    IInspectable* pUnsetValue = NULL;
    isUnsetValue = FALSE;

    IFC(DependencyPropertyFactory::GetUnsetValue(&pUnsetValue));
    if (pValue == pUnsetValue)
    {
        isUnsetValue = TRUE;
    }

Cleanup:
    ReleaseInterface(pUnsetValue);
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyPropertyFactory::CheckActivationAllowed()
{
    return S_OK;
}

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_DependencyProperty()
    {
        RRETURN(ctl::ActivationFactoryCreator<DirectUI::DependencyPropertyFactory>::CreateActivationFactory());
    }
}
