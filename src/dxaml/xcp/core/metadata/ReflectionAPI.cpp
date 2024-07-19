// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <TypeTable.g.h>
#include <MetadataAPI.h>
#include <DynamicMetadataStorage.h>
#include <CustomClassInfo.h>
#include <CustomDependencyProperty.h>
#include <xcptypes.h>
#include <Rometadataresolution.h>
#include <TypeNamePtr.h>

using namespace DirectUI;

// Resolves a type from an IPropertyValue of type=OtherType.
_Check_return_ HRESULT MetadataAPI::GetClassInfoFromOtherTypeBox(_In_ wf::IPropertyValue* pValue, _Outptr_ const CClassInfo** ppType)
{
    HRESULT hr = S_OK;
    HSTRING hRuntimeClassName = nullptr;
    DWORD nPartsCount = 0;
    HSTRING* phTypeNameParts = nullptr;

    *ppType = nullptr;

    IFC(pValue->GetRuntimeClassName(&hRuntimeClassName));
    IFC(RoParseTypeName(hRuntimeClassName, &nPartsCount, &phTypeNameParts));

    // We only support IReference<T>, where T is a non-generic type.
    if (nPartsCount == 2)
    {
        IFC(GetClassInfoByFullName(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(phTypeNameParts[1]), ppType));
    }
    else
    {
        // Not a type we recognize, so fall back to Object.
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Object);
    }

Cleanup:
    // Free the strings that were returned.
    if (phTypeNameParts != nullptr)
    {
        for (DWORD i = 0; i < nPartsCount; i++)
        {
            WindowsDeleteString(phTypeNameParts[i]);
        }
    }
    CoTaskMemFree(phTypeNameParts);

    WindowsDeleteString(hRuntimeClassName);
    RRETURN(hr);
}

// Resolves a WinRT property type (e.g. PropertyType_Int32) to a type (e.g. KnownTypeIndex::Int32).
_Check_return_ HRESULT MetadataAPI::GetClassInfoFromWinRTPropertyType(_In_ wf::IPropertyValue* pValue, _In_ wf::PropertyType ePropertyType, _Outptr_ const CClassInfo** ppType)
{
    *ppType = nullptr;

    // NOTE: UInt8/UInt32 are treated as Int32.  Since we don't have entries
    // for either UInt8 or UInt32 in the type table, we'll never have DPs of
    // those types and can just return the type for Int32.  We'll need to
    // change this if have DPs of those types (or allow custom DP
    // registration with those types).
    switch (ePropertyType)
    {
    case wf::PropertyType_Boolean:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Boolean);
        break;
    case wf::PropertyType_Char16:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Char16);
        break;
    case wf::PropertyType_UInt8:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Int32);
        break;
    case wf::PropertyType_Int16:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Int16);
        break;
    case wf::PropertyType_UInt16:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::UInt16);
        break;
    case wf::PropertyType_Int32:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Int32);
        break;
    case wf::PropertyType_UInt32:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Int32);
        break;
    case wf::PropertyType_Int64:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Int64);
        break;
    case wf::PropertyType_UInt64:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Int64);
        break;
    case wf::PropertyType_Single:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Float);
        break;
    case wf::PropertyType_Double:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Double);
        break;
    case wf::PropertyType_String:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::String);
        break;
    case wf::PropertyType_Point:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Point);
        break;
    case wf::PropertyType_Size:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Size);
        break;
    case wf::PropertyType_Rect:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Rect);
        break;
    case wf::PropertyType_TimeSpan:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::TimeSpan);
        break;
    case wf::PropertyType_Guid:
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Guid);
        break;
    case wf::PropertyType_OtherType:
        // We used to call this before, so we may have to re-enable this. It's currently disabled because it
        // doesn't appear anything actually uses this.
        //IFC(GetTypeFromIids(pValue);
        IFC_RETURN(GetClassInfoFromOtherTypeBox(pValue, ppType));
        break;
    default:
        // *ppType will be NULL if there's no value
        break;
    }

    return S_OK;
}

// Gets the friendly runtime class name of a type. This may not necessarily be the same as calling GetRuntimeClassName on an IInspectable,
// because it takes into account the available type information. Should generally only be used for debug/tracing output.
_Check_return_ HRESULT MetadataAPI::GetFriendlyRuntimeClassName(_In_ IInspectable* instance, _Out_ xstring_ptr* friendlyName)
{
    // Try to get the name from the object via its TypeName entry in our type table.
    auto customPropertyProvider = ctl::query_interface_cast<xaml_data::ICustomPropertyProvider>(instance);
    if (customPropertyProvider != nullptr)
    {
        TypeNamePtr typeName;
        if (SUCCEEDED(customPropertyProvider->get_Type(typeName.ReleaseAndGetAddressOf())))
        {
            const CClassInfo* type = nullptr;
            if (SUCCEEDED(GetClassInfoByTypeName(typeName.Get(), &type)) && type && type->IsPublic())
            {
                *friendlyName = type->GetFullName();
                return S_OK;
            }
        }

        // If we don't get the string from our TypeTable, then we can get really ugly looking strings like:
        //      MyApp.MyType, MyApp, Version=abc.easyas.123, Culture=neutral, etc. Getting the string representation
        // from the CustomPropertyProvder will strip the stuff after MyApp.MyType from the class name and deliver a
        // nice string for debug/tracing.
        wrl_wrappers::HString stringRep;
        if (SUCCEEDED(customPropertyProvider->GetStringRepresentation(stringRep.GetAddressOf())))
        {
            IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(stringRep.Get(), friendlyName));
            return S_OK;
        }
    }

    // Fall back on the runtime class name.
    IFC_RETURN(GetRuntimeClassName(instance, friendlyName));

    return S_OK;
}

// Gets a null enter property, which can be used to identify the last enter property of a type.
const CEnterDependencyProperty* MetadataAPI::GetNullEnterProperty()
{
    return reinterpret_cast<const CEnterDependencyProperty*>(&c_aEnterProperties[static_cast<XUINT32>(KnownPropertyIndex::UnknownType_UnknownProperty)]);
}

// Gets a null object property, which can be used to identify the last object property of a type.
const CObjectDependencyProperty* MetadataAPI::GetNullObjectProperty()
{
    return reinterpret_cast<const CObjectDependencyProperty*>(&c_aObjectProperties[static_cast<XUINT32>(KnownPropertyIndex::UnknownType_UnknownProperty)]);
}

// Gets a null render property, which can be used to identify the last render property of a type.
const CRenderDependencyProperty* MetadataAPI::GetNullRenderProperty()
{
    return reinterpret_cast<const CRenderDependencyProperty*>(&c_aRenderProperties[static_cast<XUINT32>(KnownPropertyIndex::UnknownType_UnknownProperty)]);
}

// Gets the runtime class name of a type.
_Check_return_ HRESULT MetadataAPI::GetRuntimeClassName(_In_ IInspectable* instance, _Out_ xstring_ptr* runtimeClassName)
{
    wrl_wrappers::HString str;
    IFC_RETURN(instance->GetRuntimeClassName(str.GetAddressOf()));
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(str, runtimeClassName));
    return S_OK;
}


// Determines whether the specified object is an instance of the specified type.
_Check_return_ HRESULT MetadataAPI::IsInstanceOfType(_In_ IInspectable* pInstance, _In_ const CClassInfo* pType, _Out_ bool* pbIsInstanceOfType)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    IUnknown* pIgnored = nullptr;
    REFIID riid = pType->GetGuid();

    *pbIsInstanceOfType = FALSE;

    // Everything is an instance of Object.
    if (pType->GetIndex() == KnownTypeIndex::Object)
    {
        *pbIsInstanceOfType = TRUE;
        goto Cleanup;
    }

    if (riid != GUID_NULL)
    {
        if (SUCCEEDED(pInstance->QueryInterface(riid, reinterpret_cast<void**>(&pIgnored))))
        {
            ReleaseInterface(pIgnored);
            *pbIsInstanceOfType = TRUE;
            goto Cleanup;
        }
    }

    if (pType->IsEnum())
    {
        wf::PropertyType propertyType = wf::PropertyType_Empty;
        if (pInstance)
        {
            IFC(ctl::do_get_property_type(pInstance, &propertyType));
        }
        *pbIsInstanceOfType = (propertyType == wf::PropertyType_Int32 || propertyType == wf::PropertyType_UInt32);
    }
    else
    {
        const CClassInfo* pActualType = nullptr;

        // Try to resolve the type of the instance.
        IFC(GetClassInfoFromObject_SkipWinRTPropertyOtherType(pInstance, &pActualType));

        if (pActualType)
        {
            // We don't currently have a way to know if a type is a custom interface, so we always
            // assume that a custom type object can be set on a custom type target if the target
            // doesn't derive from Object.
            if (!pType->IsBuiltinType() && pType->m_nBaseTypeIndex == KnownTypeIndex::UnknownType)
            {
                *pbIsInstanceOfType = !pActualType->IsBuiltinType();
            }
            else
            {
                // Check if the incoming value is a valid value for the target.
                *pbIsInstanceOfType = IsAssignableFrom(pType, pActualType);
            }
        }
    }

Cleanup:
    RRETURN(S_OK);
}

// Look for a DP given its full name, it might use the ObjectWriter context if allowed to.
_Check_return_ HRESULT MetadataAPI::TryGetDependencyPropertyByFullyQualifiedName(_In_ const xstring_ptr_view& strName, _In_opt_ XamlServiceProviderContext* context, _Outptr_result_maybenull_ const CDependencyProperty** ppDP)
{
    // Assume it is not found.
    *ppDP = nullptr;

    if (context)
    {
        IFC_RETURN(TryGetDependencyPropertyUsingOWContext(strName, context, ppDP));
    }
    if (*ppDP == nullptr)
    {
        IFC_RETURN(TryGetAttachedPropertyByName(strName, ppDP));
    }

    return S_OK;
}

METHODPFN CDependencyProperty::GetPropertyMethod() const
{
    return c_aDependencyPropertyRuntimeData[static_cast<UINT>(m_nIndex)].m_pfn;
}

UINT16 CDependencyProperty::GetOffset() const
{
    if (IsPropMethodCall())
    {
        return 0;
    }
    return c_aDependencyPropertyRuntimeData[static_cast<UINT>(m_nIndex)].m_nOffset;
}

UINT16 CDependencyProperty::GetGroupOffset() const
{
    return c_aDependencyPropertyRuntimeData[static_cast<UINT>(m_nIndex)].m_nGroupOffset;
}

CREATEGROUPPFN CDependencyProperty::GetGroupCreator() const
{
    return c_aDependencyPropertyRuntimeData[static_cast<UINT>(m_nIndex)].m_cgpfn;
}

RENDERCHANGEDPFN CDependencyProperty::GetRenderChangedHandler() const
{
    return c_aDependencyPropertyRuntimeData[static_cast<UINT>(m_nIndex)].m_pfnNWRenderChangedHandler;
}

bool CDependencyProperty::IsAssignable(_In_ const CValue& value) const
{
    switch (GetStorageType())
    {
    case valueBool:
        switch (value.GetType())
        {
        case valueBool:
        case valueEnum:
        case valueEnum8:
        case valueString:
        case valueObject:
        case valueThemeResource:
            return true;
        }
        return false;
    case valueSigned:
    case valueFloat:
    case valueDouble:
        switch (value.GetType())
        {
        case valueSigned:
        case valueFloat:
        case valueDouble:
        case valueString:
        case valueObject:
        case valueThemeResource:
            return true;
        }
        return false;
    case valueCornerRadius:
        switch (value.GetType())
        {
        case valueCornerRadius:
        case valueString:
        case valueObject:
        case valueThemeResource:
            return true;
        }
        return false;
    case valueThickness:
        switch (value.GetType())
        {
        case valueThickness:
        case valueString:
        case valueObject:
        case valueThemeResource:
            return true;
        }
        return false;
    case valueColor:
        switch (value.GetType())
        {
        case valueColor:
        case valueString:
        case valueObject:
        case valueThemeResource:
            return true;
        }
        return false;
    case valueGridLength:
        switch (value.GetType())
        {
        case valueGridLength:
        case valueString:
        case valueObject:
        case valueThemeResource:
        case valueDouble:
        case valueFloat:
            return true;
        }
        return false;
    case valueAny:
    default:
        return true;
    }
}

const CREATEPFN CClassInfo::GetCoreConstructor() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return c_aTypeActivations[static_cast<UINT>(m_nIndex)].m_pfnCreate;
    }
    else
    {
        return nullptr;
    }
}

const CREATEPFNFX CClassInfo::GetFrameworkConstructor() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return c_aTypeActivations[static_cast<UINT>(m_nIndex)].m_pfnCreateFramework;
    }
    else
    {
        return nullptr;
    }
}

const CEnterDependencyProperty* CClassInfo::GetFirstEnterProperty() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        UINT nFirstEnterPropertyIndex = c_aTypeProperties[static_cast<UINT>(m_nIndex)].m_nFirstEnterPropertyIndex;
        return reinterpret_cast<const CEnterDependencyProperty*>(&c_aEnterProperties[nFirstEnterPropertyIndex]);
    }
    else
    {
        // We only return built-in properties.
        return GetBaseType()->GetFirstEnterProperty();
    }
}

const CObjectDependencyProperty* CClassInfo::GetFirstObjectProperty() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        UINT nFirstObjectPropertyIndex = c_aTypeProperties[static_cast<UINT>(m_nIndex)].m_nFirstObjectPropertyIndex;
        return reinterpret_cast<const CObjectDependencyProperty*>(&c_aObjectProperties[nFirstObjectPropertyIndex]);
    }
    else
    {
        // We only return built-in properties.
        return GetBaseType()->GetFirstObjectProperty();
    }
}

const CRenderDependencyProperty* CClassInfo::GetFirstRenderProperty() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        UINT nFirstRenderPropertyIndex = c_aTypeProperties[static_cast<UINT>(m_nIndex)].m_nFirstRenderPropertyIndex;
        return reinterpret_cast<const CRenderDependencyProperty*>(&c_aRenderProperties[nFirstRenderPropertyIndex]);
    }
    else
    {
        // We only return built-in properties.
        return GetBaseType()->GetFirstRenderProperty();
    }
}

_Check_return_ HRESULT CClassInfo::RunClassConstructorIfNecessary()
{
    HRESULT hr = S_OK;
    wxaml_interop::TypeName typeName = {};

    if ((m_flags & MetaDataTypeInfoFlags::ExecutedClassConstructor) != MetaDataTypeInfoFlags::ExecutedClassConstructor)
    {
        ctl::ComPtr<IManagedActivationFactory> spManagedActivationFactory;
        xaml_markup::IXamlType* pXamlTypeNoRef = AsCustomType()->GetXamlTypeNoRef();

        // Unknown types don't have an IXamlType reference.
        if (pXamlTypeNoRef != nullptr)
        {
            IFC(pXamlTypeNoRef->get_UnderlyingType(&typeName));
            if (typeName.Kind == wxaml_interop::TypeKind::TypeKind_Metadata && SUCCEEDED(ctl::GetActivationFactory(typeName.Name, &spManagedActivationFactory)))
            {
                // For WinMD types, invoke the class constructor ourselves.
                IGNOREHR(spManagedActivationFactory->RunClassConstructor());
            }
            else
            {
                // If we cannot find the DLL activation factory, or if this is a non-WinMD type, then
                // invoke IXamlType.RunInitializer and let it try handling class construction.
                IGNOREHR(pXamlTypeNoRef->RunInitializer());
            }
        }

        m_flags |= MetaDataTypeInfoFlags::ExecutedClassConstructor;
    }

Cleanup:
    WindowsDeleteString(typeName.Name);
    RRETURN(hr);
}

const CEnterDependencyProperty* CEnterDependencyProperty::GetNextProperty() const
{
    return reinterpret_cast<const CEnterDependencyProperty*>(&c_aEnterProperties[m_nNextEnterPropertyIndex]);
}

const CObjectDependencyProperty* CObjectDependencyProperty::GetNextProperty() const
{
    return reinterpret_cast<const CObjectDependencyProperty*>(&c_aObjectProperties[m_nNextObjectPropertyIndex]);
}

const CRenderDependencyProperty* CRenderDependencyProperty::GetNextProperty() const
{
    return reinterpret_cast<const CRenderDependencyProperty*>(&c_aRenderProperties[m_nNextRenderPropertyIndex]);
}
