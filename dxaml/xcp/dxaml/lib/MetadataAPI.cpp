// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Metadata API. This is a process-wide API. Custom types may be resolved
//      through IXamlMetadataProvider.
//
//      Eventually this file should be merged with /core/dll/MetadataAPI.cpp. It's
//      currently a separate file to avoid requiring a dependency from /core/dll on
//      /dxaml/lib.

#include "precomp.h"
#include <MetadataAPI.h>
#include <DynamicMetadataStorage.h>
#include <CustomClassInfo.h>
#include <CustomDependencyProperty.h>
#include <DependencyObject.h>
#include <FrameworkApplication.g.h>
#include <PropertyMetadata.g.h>
#include <BackgroundTaskFrameworkContext.h>
#include <XamlLibMetadataProvider.h>
#include <TypeNamePtr.h>
#include <RuntimeProfiler.h>
#include "XamlProperty.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;

//TFS: 186956 : Get this list from a registry
//Note:  Added size specifier defined in MetadataAPI.h to reference in PLMHandler::ExtensionTelemetryProc
const wchar_t* s_pWUXExtensions[WUXExtensionListSize] = {  L"Microsoft.UI.Xaml.Phone.dll"  };

extern void STDAPICALLTYPE CacheExtensionSuspensionCallback(_In_opt_ const HMODULE hModule, _In_ const wchar_t * modulename);

_Check_return_ HRESULT MetadataAPI::GetClassInfoFromObject_ResolveWinRTPropertyOtherType(
    _In_opt_ IInspectable* pInstance,
    _Outptr_ const CClassInfo** ppType)
{
    return GetClassInfoFromObject_Helper(pInstance, ppType, true);
}

_Check_return_ HRESULT MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(
    _In_opt_ IInspectable* pInstance,
    _Outptr_ const CClassInfo** ppType)
{
    return GetClassInfoFromObject_Helper(pInstance, ppType, false);
}

// Resolves the type of an object.
_Check_return_ HRESULT MetadataAPI::GetClassInfoFromObject_Helper(
    _In_opt_ IInspectable* pInstance,
    _Outptr_ const CClassInfo** ppType,
    _In_ bool resolveWinRTPropertyOtherType)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IPropertyValue> spPropertyValue;
    ctl::ComPtr<DirectUI::DependencyObject> spDO;
    xstring_ptr strClassName;
    HSTRING hRuntimeClassName = nullptr;

    *ppType = nullptr;

    if (!pInstance)
    {
        // If there's no instance, default to "Object".
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Object);
        goto Cleanup;
    }

    // Check if this is a WinRT box.
    spPropertyValue.Attach(ctl::get_property_value(pInstance));
    if (spPropertyValue != nullptr)
    {
        wf::PropertyType ePropertyType;
        IFC(spPropertyValue->get_Type(&ePropertyType));
        if (   ePropertyType != wf::PropertyType_OtherType
            || resolveWinRTPropertyOtherType)
        {
            IFC(GetClassInfoFromWinRTPropertyType(spPropertyValue.Get(), ePropertyType, ppType));
            if (*ppType != nullptr)
            {
                goto Cleanup;
            }
        }
    }

    // Check if this is a DO.
    spDO.Attach(ctl::query_interface<DirectUI::DependencyObject>(pInstance));

    // Only try to resolve types by class-name for composed types or non-DO types. This
    // allows us to resolve internal types such as CollectionViewGroup.
    if (spDO == nullptr || spDO->IsComposed())
    {
        ctl::ComPtr<xaml_data::ICustomPropertyProvider> spCustomObject;

        // Try to get the name from the object via its TypeName entry in our type table.
        spCustomObject.Attach(ctl::query_interface<xaml_data::ICustomPropertyProvider>(pInstance));
        if (spCustomObject)
        {
            TypeNamePtr typeName;
            if (SUCCEEDED(spCustomObject->get_Type(typeName.ReleaseAndGetAddressOf())))
            {
                const CClassInfo* pType = nullptr;

                hr = GetClassInfoByTypeName(typeName.Get(), &pType);

                // We really want to fail, but unfortunately because we didn't in Windows 8.1, we can't do so now either.
                // TODO: [Add quirk.] TFS#677191: Quirk: GetClassInfoFromObject should not swallow errors from GetClassInfoByTypeName anymore in Threshold and beyond.
                ASSERTSUCCEEDED(hr);

                // Only allow unknown/non-public types when we're not dealing with DOs. When
                // we're dealing with DOs, prefer the known built-in DO type. E.g. if someone
                // has a type called MyButton deriving from Button, its built-in DO type is
                // Button.
                if (SUCCEEDED(hr) && pType != nullptr &&
                    (pType->IsPublic() || spDO == nullptr))
                {
                    *ppType = pType;
                    goto Cleanup;
                }
            }
        }

        // Try using GetRuntimeClassName to infer the type.
        IFC(pInstance->GetRuntimeClassName(&hRuntimeClassName));
        IFC(xstring_ptr::CloneRuntimeStringHandle(hRuntimeClassName, &strClassName));
        if (!strClassName.IsNullOrEmpty())
        {
            const CClassInfo* pType = nullptr;

            // We don't want to return internal types.
            if (SUCCEEDED(MetadataAPI::GetClassInfoByFullName(strClassName, &pType)) && pType->IsPublic())
            {
                *ppType = pType;
                goto Cleanup;
            }
        }
    }

    // Try to get the most derived built-in DO type. For example, if this is a custom type that
    // derives from UserControl, then try returning the TypeInfo for UserControl.
    if (spDO != nullptr)
    {
        *ppType = GetClassInfoByIndex(spDO->GetTypeIndex());
        if (*ppType != nullptr)
        {
            goto Cleanup;
        }
    }

    // If we weren't able to resolve the type, try to find it
    // based on the primary type ID. This is generally only required
    // to resolve value types such as IReference<MyEnum>.
    // TODO: Consider re-enabling this.
    //IFC(GetTypeFromIids(pInstance, ppType));

    // Default to object.
    *ppType = GetClassInfoByIndex(KnownTypeIndex::Object);

Cleanup:
    WindowsDeleteString(hRuntimeClassName);
    RRETURN(hr);
}

// Gets an IDependencyProperty object.
_Check_return_ HRESULT MetadataAPI::GetIDependencyProperty(_In_ KnownPropertyIndex ePropertyIndex, _Outptr_ xaml::IDependencyProperty** ppProperty)
{
    // Expect that most apps will register at least 100 custom DPs.
    constexpr size_t DPCacheSizeMargin = 100;
    constexpr double DPCacheSizeGrowthRate = 1.5;

    ctl::ComPtr<DependencyPropertyHandle> spProperty;
    UINT nIndex = static_cast<UINT>(ePropertyIndex);
    DynamicMetadataStorageInstanceWithLock storage;

    if (storage->m_dpHandleCache == nullptr)
    {
        size_t requiredSize = KnownPropertyCount + std::max(
            DPCacheSizeMargin,
            storage->m_customPropertiesCache.size());

        storage->m_dpHandleCache.reset(new std::vector<ctl::ComPtr<xaml::IDependencyProperty>>(requiredSize));
    }
    else
    {
        // Check if we already have an instance.
        if (nIndex < storage->m_dpHandleCache->size())
        {
            spProperty = ((*storage->m_dpHandleCache)[nIndex]).Cast<DependencyPropertyHandle>();

            if (spProperty != nullptr)
            {
                *ppProperty = spProperty.Detach();
                return S_OK;
            }
        }
        else
        {
            // Make sure the vector is big enough.
            // Whichever is larger - previous size * growth rate OR minimum required to fit currently requested DP index.

            size_t requiredSize = std::max(
                static_cast<size_t>(storage->m_dpHandleCache->size() * DPCacheSizeGrowthRate),
                storage->m_customPropertiesCache.size() + KnownPropertyCount);

            storage->m_dpHandleCache->resize(requiredSize);
        }
    }

    // This is the first time we create a DP handle.
    IFC_RETURN(ctl::make_ignoreleak(GetDependencyPropertyByIndex(ePropertyIndex), &spProperty));

    // Add it to our cache.
    (*storage->m_dpHandleCache)[nIndex] = spProperty;

    *ppProperty = spProperty.Detach();

    return S_OK;
}

// Registers a custom dependency property.
// TODO: Validate that people don't register properties for built-in types for Blue+1 and beyond.
// TODO: Validate that people don't register the same DP multiple times. This is something OneNote
//       current does.
_Check_return_ HRESULT MetadataAPI::RegisterDependencyProperty(
    _In_ bool bIsAttached,
    _In_ HSTRING hName,
    _In_ wxaml_interop::TypeName propertyType,
    _In_ wxaml_interop::TypeName ownerType,
    _In_opt_ xaml::IPropertyMetadata* pDefaultMetadata,
    _In_ bool bIsReadOnly,
    _Outptr_ xaml::IDependencyProperty** ppProperty)
{
    xstring_ptr strName, propertyTypeName, ownerTypeName;
    CCustomDependencyProperty* pDP = nullptr;
    MetaDataPropertyInfoFlags ePropertyFlags = (MetaDataPropertyInfoFlags::IsSparse | MetaDataPropertyInfoFlags::IsNullable);
    DynamicMetadataStorageInstanceWithLock storage;

    // Get the property name.
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(hName, &strName));

    // Copy the property and owner type names.
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(propertyType.Name, &propertyTypeName));
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(ownerType.Name, &ownerTypeName));

    // Get unique type index.
    KnownPropertyIndex ePropertyIndex = storage->GetNextAvailablePropertyIndex();

    // Define additional flags.
    if (bIsAttached) ePropertyFlags |= MetaDataPropertyInfoFlags::IsAttached;
    if (bIsReadOnly) ePropertyFlags |= MetaDataPropertyInfoFlags::IsReadOnlyProperty;

    // Create a DP with the information we have so far.
    IFC_RETURN(CCustomDependencyProperty::Create(ePropertyIndex, ePropertyFlags, strName, &pDP));

    // Queue the registration. We don't want to resolve types until we absolutely need to.
    if (storage->m_queuedDPRegistrations == nullptr)
    {
        storage->m_queuedDPRegistrations.reset(new std::queue<DynamicMetadataStorage::DPRegistrationInfo>());
    }

    storage->m_queuedDPRegistrations->emplace(DynamicMetadataStorage::DPRegistrationInfo(pDP, propertyType.Kind, propertyTypeName, ownerType.Kind, ownerTypeName, pDefaultMetadata));

    // Add the DP to the main runtime property cache. The property index we just acquired is based on
    // the assumption that we're immediately inserting into the property cache.
    storage->m_customPropertiesCache.push_back(pDP);

    IFC_RETURN(GetIDependencyProperty(ePropertyIndex, ppProperty));

    return S_OK;
}

_Check_return_ HRESULT CCustomDependencyProperty::SetDefaultMetadata(_In_ xaml::IPropertyMetadata* pDefaultMetadata)
{
    HRESULT hr = S_OK;

    // Store callback.
    IFC(static_cast<PropertyMetadata*>(pDefaultMetadata)->get_PropertyChangedCallback(&m_spPropertyChangedCallback));

    // Store default value.
    IFC(static_cast<PropertyMetadata*>(pDefaultMetadata)->get_CreateDefaultValueCallback(&m_spCreateDefaultValueCallback));
    if (m_spCreateDefaultValueCallback == nullptr)
    {
        BOOLEAN bIsUnsetValue = FALSE;
        ctl::ComPtr<IInspectable> spDefaultValue;

        IFC(static_cast<PropertyMetadata*>(pDefaultMetadata)->get_DefaultValue(&spDefaultValue));
        IFC(DependencyPropertyFactory::IsUnsetValue(spDefaultValue.Get(), bIsUnsetValue));
        if (!bIsUnsetValue)
        {
            m_spDefaultValue = spDefaultValue;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Tries to resolve a DP given its full name using the current parser context.
// NOTE: This method does not need to use the lock because it is not actually looking for the property on the tables directly,
// it will first use the supplied context then default to the tables using one of the existing methods.
_Check_return_ HRESULT MetadataAPI::TryGetDependencyPropertyUsingOWContext(_In_ const xstring_ptr_view& strName, _In_ DirectUI::XamlServiceProviderContext* context, _Outptr_result_maybenull_ const CDependencyProperty** ppDP)
{
    std::shared_ptr<XamlProperty> xamlProperty;

    // Assume it is not found.
    *ppDP = nullptr;

    IFC_RETURN(context->ResolveProperty(strName, xamlProperty));
    IFCCHECK_RETURN(xamlProperty);

    // If we got a valid token for our property resolve it back to the underlying DP.
    if (!xamlProperty->IsUnknown())
    {
        const CDependencyProperty* pProperty = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(xamlProperty->get_PropertyToken().GetHandle());
        if (pProperty != nullptr)
        {
            IFC_RETURN(GetUnderlyingDependencyProperty(pProperty, ppDP));
        }
    }

    return S_OK;
}

// Tries to get a IXamlMetadataProvider.
_Check_return_ HRESULT MetadataAPI::TryGetMetadataProvider(_Outptr_result_maybenull_ xaml_markup::IXamlMetadataProvider** ppMetadataProvider)
{
    DynamicMetadataStorageInstanceWithLock storage;

    *ppMetadataProvider = nullptr;

    // First check for a cached metadata provider.
    if (storage->m_metadataProvider != nullptr)
    {
        IFC_RETURN(storage->m_metadataProvider.CopyTo(ppMetadataProvider));

        return S_OK;
    }

    // Pick up the new override
    IInspectable* pMetadataProviderInspectableInnerNoRef = storage->m_overriddenMetadataProvider.Get();

    // If the override has just been cleared, let's fall back to the usual suspects
    if (!pMetadataProviderInspectableInnerNoRef)
    {
        // Determine an IInspectable to query for IXamlMetadataProvider.
        // We'll try the following in order, stopping when we find a non-null object:
        //     - Application instance
        //     - XamlRenderingBackgroundTask instance
        if (FrameworkApplication::GetCurrentNoRef())
        {
            pMetadataProviderInspectableInnerNoRef = ctl::as_iinspectable(FrameworkApplication::GetCurrentNoRef());
        }
        else if (BackgroundTaskFrameworkContext::GetMetadataProviderNoRef())
        {
            pMetadataProviderInspectableInnerNoRef = BackgroundTaskFrameworkContext::GetMetadataProviderNoRef();
        }
    }

    // Aggregate the provider obtained from the host with an extensions-aware metadata provider
    wrl::ComPtr<xaml_markup::IXamlMetadataProvider> spMetadataProvider;
    IFC_RETURN(wrl::MakeAndInitialize<xaml_phone_xti::XamlLibMetadataProvider>(
        &spMetadataProvider,
        s_pWUXExtensions,
        ARRAYSIZE(s_pWUXExtensions),
        CacheExtensionSuspensionCallback,
        pMetadataProviderInspectableInnerNoRef));
    storage->m_metadataProvider = spMetadataProvider.Get();

    // Cache the result so that next time we can immediately return this provider.
    IFC_RETURN(storage->m_metadataProvider.CopyTo(ppMetadataProvider));

    return S_OK;
}
