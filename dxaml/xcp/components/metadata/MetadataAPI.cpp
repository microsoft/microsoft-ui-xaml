// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Metadata API. This is a process-wide API. Custom types may be resolved
// through IXamlMetadataProvider.
// TODO: Make thread-safe.

#include "precomp.h"

#include <Rometadataresolution.h>

#include <XStringBuilder.h>
#include <TypeTable.g.h>
#include <MetadataAPI.h>
#include <DynamicMetadataStorage.h>
#include <CustomClassInfo.h>
#include <CustomDependencyProperty.h>
#include <xcptypes.h>
#include "TypeNameHelper.h"
#include "stack_vector.h"

using namespace DirectUI;

static const CDependencyProperty* TryFindCustomPropertyHelper(
    _In_ const DynamicMetadataStorage::PropertiesByTypeTable* table,
    _In_ const CClassInfo* pType,
    _In_ const xstring_ptr_view& strName)
{
    if (table)
    {
        auto typeIterator = table->find(pType->GetIndex());

        if (typeIterator != table->end())
        {
            auto mapDPs = typeIterator->second.get();
            auto itMap = mapDPs->find(strName);

            if (itMap != mapDPs->end() &&
                itMap->second != nullptr)
            {
                return itMap->second;
            }
        }
    }

    return nullptr;
}

// Associates a dependency property with a type in a runtime cache.
_Check_return_ HRESULT MetadataAPI::AssociateDependencyProperty(_In_ const CClassInfo* pType, _In_ const CDependencyProperty* pDP)
{
    DynamicMetadataStorage::PropertiesTable* propertiesMap;
    DynamicMetadataStorage::PropertiesByTypeTable::iterator typeIterator;
    DynamicMetadataStorageInstanceWithLock storage;

    if (storage->m_customDPsByTypeAndNameCache == nullptr)
    {
        storage->m_customDPsByTypeAndNameCache.reset(new DynamicMetadataStorage::PropertiesByTypeTable());
    }

    typeIterator = storage->m_customDPsByTypeAndNameCache->find(pType->GetIndex());
    if (typeIterator == storage->m_customDPsByTypeAndNameCache->end())
    {
        propertiesMap = new containers::vector_map<xstring_ptr, const CDependencyProperty*>();
        (*storage->m_customDPsByTypeAndNameCache)[pType->GetIndex()].reset(propertiesMap);
    }
    else
    {
        propertiesMap = typeIterator->second.get();
    }

    {
        auto iter = propertiesMap->find(pDP->GetName());
        bool entryIsDuplicate = (iter!=propertiesMap->end());
        (*propertiesMap)[pDP->GetName()] = pDP;

        if (entryIsDuplicate)
        {
            // In the case of registration of the same DP multiple times, this code needs to update
            // the wrapper DP to point to the new underlying DP.
            const CDependencyProperty* pWrappingProperty = nullptr;

            IFC_RETURN(MetadataAPI::TryGetPropertyByName(
                pType,
                pDP->GetName(),
                &pWrappingProperty));

            if (pWrappingProperty != nullptr)
            {
                // MSFT:1065892: Photo Editor app re-registers a built-in DP. Built-in DPs don't have a separate
                //               wrapper property, so there's nothing to update.
                if (auto customProperty = pWrappingProperty->AsOrNull<CCustomProperty>())
                {
                    const_cast<CCustomProperty*>(customProperty)->UpdateUnderlyingDP(pDP);
                }
            }
        }
    }

    return S_OK;
}

// Associates a property with a type in a runtime cache.
_Check_return_ HRESULT MetadataAPI::AssociateProperty(_In_ const CClassInfo* pType, _In_ const CDependencyProperty* pProperty)
{
    HRESULT hr = S_OK;
    DynamicMetadataStorage::PropertiesTable* propertiesMap;
    DynamicMetadataStorage::PropertiesByTypeTable::iterator typeIterator;
    DynamicMetadataStorageInstanceWithLock storage;

    if (storage->m_customPropertiesByTypeAndNameCache == nullptr)
    {
        storage->m_customPropertiesByTypeAndNameCache.reset(new DynamicMetadataStorage::PropertiesByTypeTable());
    }

    typeIterator = storage->m_customPropertiesByTypeAndNameCache->find(pType->GetIndex());
    if (typeIterator == storage->m_customPropertiesByTypeAndNameCache->end())
    {
        propertiesMap = new containers::vector_map<xstring_ptr, const CDependencyProperty*>();
        (*storage->m_customPropertiesByTypeAndNameCache)[pType->GetIndex()].reset(propertiesMap);
    }
    else
    {
        propertiesMap = typeIterator->second.get();
    }

    {
        (*propertiesMap)[pProperty->GetName()] = pProperty;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

// Extracts the namespace name and short name from the specified full name.
_Check_return_ HRESULT MetadataAPI::ExtractNamespaceNameAndShortName(_In_ const xstring_ptr_view& strTypeFullName, _Out_ xephemeral_string_ptr* pstrNamespaceName, _Out_ xephemeral_string_ptr* pstrTypeName)
{
    auto nIndex = strTypeFullName.FindLastChar('.', strTypeFullName.FindChar('<'));
    if (nIndex != xstring_ptr_view::npos)
    {
        strTypeFullName.SubString(0, nIndex, pstrNamespaceName);
        strTypeFullName.SubString(nIndex + 1, strTypeFullName.GetCount(), pstrTypeName);
    }
    else
    {
        // There's no namespace.
        strTypeFullName.Demote(pstrTypeName);
    }

    RRETURN(S_OK);//RRETURN_REMOVAL
}

// Extracts the short name from the specified full name.
void MetadataAPI::ExtractShortName(_In_ const xstring_ptr_view& strTypeFullName, _Out_ xephemeral_string_ptr* pstrTypeName)
{
    auto nIndex = strTypeFullName.FindLastChar('.');
    if (nIndex != xstring_ptr_view::npos)
    {
        strTypeFullName.SubString(nIndex + 1, strTypeFullName.GetCount(), pstrTypeName);
    }
    else
    {
        // This appears to be a primitive type ("Object", "String", etc.) - they don't have a namespace.
        strTypeFullName.Demote(pstrTypeName);
    }
}

// Resolves a built-in type by its short name.
const CClassInfo* MetadataAPI::GetBuiltinClassInfoByName(_In_ const xstring_ptr_view& strTypeName)
{
    xephemeral_string_ptr strNormalizedTypeName;
    UINT nTypeNameLength = strTypeName.GetCount();

    // Normalize name by removing legacy type metadata.
    if (strTypeName.GetChar(0) == '!')
    {
        strTypeName.SubString(1, nTypeNameLength, &strNormalizedTypeName);
        nTypeNameLength--;
    }
    else
    {
        strTypeName.Demote(&strNormalizedTypeName);
    }

    UINT nEndIndex;
    UINT nStartIndex = MapTypeNameLengthToSearchRange(nTypeNameLength, &nEndIndex);

    for (UINT i = nStartIndex; i < nEndIndex; i++)
    {
        UINT nTypeIndex = static_cast<UINT>(c_aTypeNames[i].m_nTypeIndex);
        xstring_ptr_storage strName = c_aTypeNameInfos[nTypeIndex].m_strNameStorage;
        if (strNormalizedTypeName.Equals(strName.Buffer, strName.Count))
        {
            return reinterpret_cast<const CClassInfo*>(&c_aTypes[nTypeIndex]);
        }
    }

    return nullptr;
}

// Gets a built-in namespace by its name.
const CNamespaceInfo* MetadataAPI::GetBuiltinNamespaceByName(_In_ const xstring_ptr_view& strNamespaceName)
{
    UINT nExpectedLength = strNamespaceName.GetCount();

    // if the value of avoiding the loop ever exceeds the cost of creating
    // a valuestore, then we'll do that.
    // Start at 1 to skip KnownNamespaceIndex::UnknownNamespace.
    for (XUINT32 i = 1; i < ARRAY_SIZE(c_aNamespaces); i++)
    {
        UINT nCurrentLength = c_aNamespaces[i].m_strNameStorage.Count;
        if (nCurrentLength == nExpectedLength)
        {
            if (strNamespaceName.Equals(c_aNamespaces[i].m_strNameStorage.Buffer, nCurrentLength))
            {
                return reinterpret_cast<const CNamespaceInfo*>(&c_aNamespaces[i]);
            }
        }
        else if (nCurrentLength > nExpectedLength)
        {
            // c_aNamespaces is sorted by namespace name length, so if we haven't found anything by now, it doesn't exist in this array.
            return nullptr;
        }
    }

    return nullptr;
}

// Gets a type by its index.
const CClassInfo* MetadataAPI::GetClassInfoByIndex(_In_ KnownTypeIndex eTypeIndex)
{
    if (IsKnownIndex(eTypeIndex))
    {
        return reinterpret_cast<const CClassInfo*>(&c_aTypes[static_cast<UINT16>(eTypeIndex)]);
    }
    else
    {
        DynamicMetadataStorageInstanceWithLock storage;
        ASSERT((static_cast<UINT>(eTypeIndex) - KnownTypeCount) < storage->m_customTypesCache.size());
        return storage->m_customTypesCache[static_cast<UINT>(eTypeIndex) - KnownTypeCount].get();
    }
}

// Resolves a type by its full name.
_Check_return_ HRESULT MetadataAPI::GetClassInfoByFullName(_In_ const xstring_ptr_view& strTypeFullName, _Outptr_ const CClassInfo** ppType)
{
    IFC_RETURN(TryGetClassInfoByFullName(strTypeFullName, /* bSearchCustomTypesOnly */ FALSE, ppType));
    IFCCHECK_RETURN(*ppType != nullptr);

    return S_OK;
}

// Resolves a type by its type name. This function may call out to user code (IXamlMetadataProvider) to resolve custom types.
_Check_return_ HRESULT MetadataAPI::GetClassInfoByTypeName(_In_ wxaml_interop::TypeName typeName, _Outptr_ const CClassInfo** ppType)
{
    xstring_ptr strFullName;

    *ppType = nullptr;

    if (typeName.Name != nullptr)
    {
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(typeName.Name, &strFullName));
        if (typeName.Kind == wxaml_interop::TypeKind::TypeKind_Primitive)
        {
            // This is a primitive WinRT type.
            *ppType = GetBuiltinClassInfoByName(strFullName);

            // If we didn't find the type, it's either a primitive we don't know about, or the app is lying
            // to us and this type isn't really a primitive. (At the time of writing this, Numbers.exe tells us
            // that "Microsoft.UI.Xaml.Controls.ListView" is a primitive.)
            if (*ppType == nullptr)
            {
                // Try looking it up as a metadata type instead.
                IFC_RETURN(GetClassInfoByFullName(strFullName, ppType));
            }
        }
        else if (typeName.Kind == wxaml_interop::TypeKind::TypeKind_Metadata)
        {
            // This is a regular WinRT type.
            *ppType = TryGetBuiltinClassInfoByFullName(strFullName);

            if (*ppType == nullptr)
            {
                DynamicMetadataStorageInstanceWithLock storage;
                // Type doesn't exist in our type table. See if it exists in our runtime type cache.
                auto itCustomTypes = storage->m_customTypesByNameCache.find(strFullName);
                if (itCustomTypes == storage->m_customTypesByNameCache.end())
                {
                    // TODO: [TFS 344624] Add a quirk. We should call ImportClassInfoFromMetadataProvider(TypeName) for new applications.
                    // We don't know about this type. Resolve it now.
                    {
                        const CNamespaceInfo* pNamespace = nullptr;
                        xephemeral_string_ptr strNamespaceName, strTypeName;
                        IFC_RETURN(ExtractNamespaceNameAndShortName(strFullName, &strNamespaceName, &strTypeName));
                        IFC_RETURN(GetNamespaceByName(strNamespaceName, &pNamespace));
                        if (pNamespace->IsBuiltinNamespace())
                        {
                            IFC_RETURN(ImportClassInfoFromMetadataProvider(typeName, ppType));
                        }
                        else
                        {
                            // For Windows 8.1 compatibility, we resolve by fullname instead of TypeName.
                            IFC_RETURN(ImportClassInfoFromMetadataProvider(strFullName, ppType));
                        }
                    }
                }
                else{
                    *ppType = itCustomTypes->second;
                }
            }
        }
        else
        {
            DynamicMetadataStorageInstanceWithLock storage;
            // This is a non-WinRT type. See if it exists in our runtime type cache.
            auto itCustomTypes = storage->m_customTypesByCustomNameCache.find(strFullName);
            if (itCustomTypes == storage->m_customTypesByCustomNameCache.end())
            {
                // We don't know about this type. Resolve it now.
                IFC_RETURN(ImportClassInfoFromMetadataProvider(typeName, ppType));
            }
            else{
                *ppType = itCustomTypes->second;
            }
        }
    }

    return S_OK;
}

// Resolves a type to represent the specified IXamlType.
_Check_return_ HRESULT MetadataAPI::GetClassInfoByXamlType(_In_ xaml_markup::IXamlType* pXamlType, _Outptr_ const CClassInfo** ppType)
{
    HRESULT hr = S_OK;
    wxaml_interop::TypeName typeName = {};

    IFC(pXamlType->get_UnderlyingType(&typeName));

    // While .NET 5 projects System.Type to Windows.UI.Xaml.Interop.TypeName, it does not project the fields as expected.
    // When the underlying type is System.Type itself, the TypeName.Name is still given as System.Type with TypeName.Kind as TypeKind_Custom.
    // However, those fields themselves should also be projected, with TypeName.Name as Windows.UI.Xaml.Interop.TypeName with TypeName.Kind as TypeKind_Metadata.
    // Fix up the discrepancy here until it's fixed - without it, properties of type System.Type won't work when set in markup.
    // CSWinRT tracking issue: https://github.com/microsoft/CsWinRT/issues/344
    if (wrl_wrappers::HStringReference(L"System.Type") == typeName.Name)
    {
        WindowsDeleteString(typeName.Name);
        IFC(WindowsCreateString(L"Windows.UI.Xaml.Interop.TypeName", 32, &typeName.Name));
        typeName.Kind = wxaml_interop::TypeKind::TypeKind_Metadata;
    }

    IFC(GetClassInfoByTypeName(typeName, ppType));

Cleanup:
    WindowsDeleteString(typeName.Name);
    RRETURN(hr);
}

// Gets a dependency property by its index.
const CPropertyBase* MetadataAPI::GetPropertyBaseByIndex(_In_ KnownPropertyIndex ePropertyIndex)
{
    if (IsKnownIndex(ePropertyIndex))
    {
        return reinterpret_cast<const CPropertyBase*>(&c_aProperties[static_cast<UINT16>(ePropertyIndex)]);
    }
    else
    {
        DynamicMetadataStorageInstanceWithLock storage;
        ASSERT(GetRelativeCustomIndex(ePropertyIndex) < storage->m_customPropertiesCache.size());
        return storage->m_customPropertiesCache[GetRelativeCustomIndex(ePropertyIndex)];
    }
}

// Gets a dependency property by its index.
const CDependencyProperty* MetadataAPI::GetDependencyPropertyByIndex(_In_ KnownPropertyIndex ePropertyIndex)
{
    const CDependencyProperty* result = GetPropertyBaseByIndex(ePropertyIndex)->AsOrNull<CDependencyProperty>();
    // This assert is no longer valid, as we have built-in simple properties as well.  Those are represented by
    // CSimpleProperty, which is a CPropertyBase but not a CDependencyProperty.  If this is a simple property,
    // we just want to return null (to actually get a simple property, use TryGetBuiltinPropertyBaseByName directly)
    //ASSERT(result);
    return result;
}

bool MetadataAPI::IsCustomIndex(_In_ KnownPropertyIndex ePropertyIndex)
{
    if (IsKnownIndex(ePropertyIndex))
    {
        return false;
    }
    else
    {
        DynamicMetadataStorageInstanceWithLock storage;
        return GetRelativeCustomIndex(ePropertyIndex) < storage->m_customPropertiesCache.size();
    }
}

// Gets a namespace by its index.
const CNamespaceInfo* MetadataAPI::GetNamespaceByIndex(_In_ KnownNamespaceIndex eNamespaceIndex)
{
    if (IsKnownIndex(eNamespaceIndex))
    {
        return reinterpret_cast<const CNamespaceInfo*>(&c_aNamespaces[static_cast<UINT>(eNamespaceIndex)]);
    }
    else
    {
        DynamicMetadataStorageInstanceWithLock storage;
        ASSERT((static_cast<UINT>(eNamespaceIndex) - KnownNamespaceCount) < storage->m_customNamespacesCache.size());
        return storage->m_customNamespacesCache[static_cast<UINT>(eNamespaceIndex) - KnownNamespaceCount].get();
    }
}

// Resolves a namespace by the specified name.
_Check_return_ HRESULT MetadataAPI::GetNamespaceByName(_In_ const xstring_ptr_view& strNamespaceName, _Outptr_ const CNamespaceInfo** ppNamespace)
{
    // If the name looks familiar (e.g. starts with "Windows."), then loop through our built-in list of namespaces.
    if (ShouldLookupInBuiltinNamespaceTable(strNamespaceName))
    {
        *ppNamespace = GetBuiltinNamespaceByName(strNamespaceName);
        if (*ppNamespace != nullptr)
        {
            return S_OK;
        }
    }

    {
        DynamicMetadataStorageInstanceWithLock storage;
        // Namespace doesn't exist in our type table. See if it exists in our runtime namespace cache.
        auto itNamespace = storage->m_customNamespacesByNameCache.find(strNamespaceName);
        if (itNamespace == storage->m_customNamespacesByNameCache.end())
        {
            // We don't know about this type. Resolve it now.
            IFC_RETURN(ImportNamespaceInfo(strNamespaceName, ppNamespace));
        }
        else{
            *ppNamespace = itNamespace->second;
        }
    }

    return S_OK;
}

// Returns the primitive WinRT type for a type. Complex types will be returned as Object.
_Check_return_ HRESULT MetadataAPI::GetPrimitiveClassInfo(_In_opt_ const CClassInfo* pType, _Outptr_ const CClassInfo** ppType)
{
    if (pType != nullptr)
    {
        switch (pType->m_nIndex)
        {
        case KnownTypeIndex::Int32:
        case KnownTypeIndex::Char16:
        case KnownTypeIndex::UInt32:
        case KnownTypeIndex::Int16:
        case KnownTypeIndex::UInt16:
        case KnownTypeIndex::Int64:
        case KnownTypeIndex::UInt64:
        case KnownTypeIndex::String:
        case KnownTypeIndex::Float:
        case KnownTypeIndex::Double:
        case KnownTypeIndex::Boolean:
        case KnownTypeIndex::Guid:
            *ppType = pType;
            break;
        default:
            *ppType = GetClassInfoByIndex(KnownTypeIndex::Object);
            break;
        }
    }
    else
    {
        *ppType = GetClassInfoByIndex(KnownTypeIndex::Object);
    }

    RRETURN(S_OK);
}

// Gets a dependency property by its index.
const CDependencyProperty* MetadataAPI::GetPropertyByIndex(_In_ KnownPropertyIndex ePropertyIndex)
{
    // DPs and properties share the same index system.
    return GetDependencyPropertyByIndex(ePropertyIndex);
}

// Gets the property slot for a DP.
UINT8 MetadataAPI::GetPropertySlot(_In_ KnownPropertyIndex ePropertyIndex)
{
    ASSERT(IsKnownIndex(ePropertyIndex));
    return c_aPropertySlot[static_cast<UINT>(ePropertyIndex)];
}

// Gets the property slot count for a type.
UINT8 MetadataAPI::GetPropertySlotCount(_In_ KnownTypeIndex eTypeIndex)
{
    ASSERT(IsKnownIndex(eTypeIndex));
    return c_aTypeProperties[static_cast<UINT>(eTypeIndex)].m_nPropertySlotCount;
}

// Resolves a TypeName by the specified type.
_Check_return_ HRESULT MetadataAPI::GetTypeNameByClassInfo(_In_ const CClassInfo* pType, _Out_ wxaml_interop::TypeName* pTypeName)
{
    xruntime_string_ptr strRuntimeName;

    // Internally we don't consider Object to be a primitive, but the TypeName spec
    // expects Object to be of Kind=Primitive. The CLR goes by this expectation.
    if (pType->IsPrimitive() || pType->m_nIndex == KnownTypeIndex::Object)
    {
        IFC_RETURN(pType->GetName().Promote(&strRuntimeName));
        pTypeName->Name = strRuntimeName.DetachHSTRING();
        pTypeName->Kind = wxaml_interop::TypeKind_Primitive;
    }
    else if (pType->IsBuiltinType())
    {
        IFC_RETURN(pType->GetFullName().Promote(&strRuntimeName));
        pTypeName->Name = strRuntimeName.DetachHSTRING();
        pTypeName->Kind = wxaml_interop::TypeKind_Metadata;
    }
    else
    {
        const CCustomClassInfo* customType = pType->AsCustomType();
        bool found = false;

        xaml_markup::IXamlType* pXamlType = customType->GetXamlTypeNoRef();

        if (pXamlType)
        {
            IFC_RETURN(pXamlType->get_UnderlyingType(pTypeName));
            found = true;
        }

        if (!found)
        {
            pTypeName->Name = nullptr;
            pTypeName->Kind = wxaml_interop::TypeKind_Primitive;
        }
    }

    return S_OK;
}

// Gets the underlying dependency property from a property. Use this if pProperty may
// refer to a regular property, and you want the underlying DP for it. If it refers to
// a DP already, this function will return a reference to that DP.
_Check_return_ HRESULT MetadataAPI::GetUnderlyingDependencyProperty(_In_ const CDependencyProperty* pProperty, _Outptr_ const CDependencyProperty** ppDP)
{
    IFC_RETURN(TryGetUnderlyingDependencyProperty(pProperty, ppDP));
    IFCCHECK_RETURN(*ppDP != nullptr);

    return S_OK;
}

// Resolves a TypeName by the specified full name.
_Check_return_ HRESULT MetadataAPI::GetTypeNameByFullName(_In_ const xstring_ptr_view& strTypeFullName, _Out_ wxaml_interop::TypeName* pTypeName)
{
    const CClassInfo* pType = nullptr;

    IFC_RETURN(GetClassInfoByFullName(strTypeFullName, &pType));
    IFC_RETURN(GetTypeNameByClassInfo(pType, pTypeName));

    return S_OK;
}

// If the type represents a boxed type, e.g. IReference<Boolean>, returns true.
_Check_return_ bool MetadataAPI::RepresentsBoxedType(
    _In_ const CClassInfo* pType)
{
    return (!pType->IsBuiltinType() && pType->AsCustomType()->RepresentsBoxedType());
}

// If the type represents a boxed type, e.g. IReference<Boolean>, gets the inner type like Boolean.
// Otherwise, gets null.
_Check_return_ const CClassInfo* MetadataAPI::TryGetBoxedType(
    _In_ const CClassInfo* pType)
{
    if (!pType->IsBuiltinType())
    {
        if (pType->AsCustomType()->RepresentsBoxedType())
        {
            // For boxed values, try unboxing its boxed type instead.  E.g. for an IReference<Boolean>,
            // we can treat it as unboxing a Boolean since it returns the desired IReference<Boolean>.
            return pType->AsCustomType()->GetBoxedType();
        }
    }

    return nullptr;
}

// Imports an IXamlType.
_Check_return_ HRESULT MetadataAPI::ImportClassInfo(_In_ xaml_markup::IXamlType* pXamlType, _Outptr_ const CClassInfo** ppType)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_markup::IXamlType> spBaseXamlType;
    ctl::ComPtr<xaml_markup::IXamlMember> spXamlContentProperty;
    const CClassInfo* pBaseType = nullptr;
    const CClassInfo* boxedType = nullptr;
    KnownTypeIndex boxedTypeIndex = KnownTypeIndex::UnknownType;
    wxaml_interop::TypeName typeName = {};
    HSTRING hFullName = nullptr;
    xstring_ptr strKey;
    xstring_ptr strFullName;
    xephemeral_string_ptr strNamespaceName;
    xephemeral_string_ptr strTypeName;
    BOOLEAN bFlag = FALSE, isBindable = FALSE;
    MetaDataTypeInfoFlags eTypeFlags = (
        MetaDataTypeInfoFlags::IsCustomType |
        MetaDataTypeInfoFlags::IsPublic |
        MetaDataTypeInfoFlags::HasTypeConverter | // For every custom type, we want to call into IXamlMetadataProvider::CreateFromString.
        MetaDataTypeInfoFlags::RequiresPeerActivation);
    CCustomClassInfo* pType = nullptr;
    KnownNamespaceIndex eNamespaceIndex = KnownNamespaceIndex::UnknownNamespace;
    DynamicMetadataStorageInstanceWithLock storage;
    ctl::ComPtr<xaml_markup::IXamlType> spXamlType(pXamlType);
    ctl::ComPtr<xaml_markup::IXamlType> spBoxedType;

    // Resolve the base type.
    IFC(pXamlType->get_BaseType(&spBaseXamlType));
    if (spBaseXamlType != nullptr)
    {
        IFC(GetClassInfoByXamlType(spBaseXamlType.Get(), &pBaseType));
    }
    else
    {
        // Type has no base type.
        pBaseType = GetClassInfoByIndex(KnownTypeIndex::UnknownType);
    }

    // Retrieve the boxed type, if it exists
    IFC(spXamlType->get_BoxedType(&spBoxedType));
    if (spBoxedType != nullptr)
    {
        IFC(GetClassInfoByXamlType(spBoxedType.Get(), &boxedType));
        boxedTypeIndex = boxedType->GetIndex();
    }

    // Get the type name.
    IFC(pXamlType->get_UnderlyingType(&typeName));

    // Get the key we will use to store this type in the runtime type cache.
    IFC(xstring_ptr::CloneRuntimeStringHandle(typeName.Name, &strKey));

    // Get the namespace name and type name.
    if (typeName.Kind == wxaml_interop::TypeKind_Custom)
    {
        // For custom types, get a simple full name that we can extract a namespace and type name from.
        IFC(pXamlType->get_FullName(&hFullName));
        IFC(xstring_ptr::CloneRuntimeStringHandle(hFullName, &strFullName));
    }
    else
    {
        // For WinRT types, we shouldn't call get_FullName, to avoid getting projected type names rather
        // than the the WinRT type names.
        strFullName = strKey;
    }

    IFC(ExtractNamespaceNameAndShortName(strFullName, &strNamespaceName, &strTypeName));
    if (!strNamespaceName.IsNullOrEmpty())
    {
        const CNamespaceInfo* pNamespace = nullptr;
        IFC(GetNamespaceByName(strNamespaceName, &pNamespace));
        eNamespaceIndex = pNamespace->GetIndex();
    }

    // Resolve type flags.
    if (SUCCEEDED(pXamlType->get_IsConstructible(&bFlag)) && bFlag)     eTypeFlags |= MetaDataTypeInfoFlags::IsConstructible;
    if (SUCCEEDED(pXamlType->get_IsCollection(&bFlag)) && bFlag)        eTypeFlags |= MetaDataTypeInfoFlags::IsCollection;
    if (SUCCEEDED(pXamlType->get_IsDictionary(&bFlag)) && bFlag)        eTypeFlags |= MetaDataTypeInfoFlags::IsDictionary;
    if (SUCCEEDED(pXamlType->get_IsMarkupExtension(&bFlag)) && bFlag)   eTypeFlags |= MetaDataTypeInfoFlags::IsMarkupExtension;
    VERIFYHR(pXamlType->get_IsBindable(&isBindable));

    // Create the class info.
    IFC(CCustomClassInfo::Create(
        eNamespaceIndex,
        pBaseType,
        storage->GetNextAvailableTypeIndex(),   // Get unique type index.
        eTypeFlags,
        pXamlType,
        strTypeName,
        strFullName,
        !!isBindable,
        boxedTypeIndex,
        &pType));

    // Add the type to the runtime type caches.
    storage->m_customTypesByNameCache[strFullName] = pType;
    if (typeName.Kind == wxaml_interop::TypeKind_Custom)
    {
        // We store non-WinRT types using the original name (e.g. assembly qualified name) to avoid ambiguity issues.
        storage->m_customTypesByCustomNameCache[strKey] = pType;
    }
    storage->m_customTypesCache.emplace_back(pType);

    // Resolve the content property if there is one.
    IFC(pXamlType->get_ContentProperty(&spXamlContentProperty));
    if (spXamlContentProperty != nullptr)
    {
        const CDependencyProperty* pContentProperty = nullptr;
        IFC(ImportPropertyInfo(pType, spXamlContentProperty.Get(), &pContentProperty));
        pType->UpdateContentPropertyIndex(pContentProperty->GetIndex());
    }

    *ppType = pType;

Cleanup:
    WindowsDeleteString(typeName.Name);
    WindowsDeleteString(hFullName);
    RRETURN(hr);
}

// Resolves a type via the IXamlMetadataProvider and adds it to the runtime type cache.
_Check_return_ HRESULT MetadataAPI::ImportClassInfoFromMetadataProvider(_In_ const xstring_ptr_view& strTypeFullName, _Outptr_ const CClassInfo** ppType)
{
    ctl::ComPtr<xaml_markup::IXamlMetadataProvider> spMetadataProvider;
    ctl::ComPtr<xaml_markup::IXamlType> spXamlType;
    xruntime_string_ptr strTypeFullNamePromoted;

    IFC_RETURN(strTypeFullName.Promote(&strTypeFullNamePromoted));

    IFC_RETURN(TryGetMetadataProvider(&spMetadataProvider));
    if (spMetadataProvider != nullptr)
    {
        IFC_RETURN(spMetadataProvider->GetXamlTypeByFullName(strTypeFullNamePromoted.GetHSTRING(), &spXamlType));
    }

    if (spXamlType != nullptr)
    {
        IFC_RETURN(ImportClassInfo(spXamlType.Get(), ppType));
    }
    else
    {
        wxaml_interop::TypeName typeName;
        typeName.Kind = wxaml_interop::TypeKind_Metadata;
        typeName.Name = strTypeFullNamePromoted.GetHSTRING();
        IFC_RETURN(ImportUnknownClassInfo(typeName, ppType));
    }

    return S_OK;
}

// Resolves a type via the IXamlMetadataProvider and adds it to the runtime type cache.
_Check_return_ HRESULT MetadataAPI::ImportClassInfoFromMetadataProvider(_In_ wxaml_interop::TypeName typeName, _Outptr_ const CClassInfo** ppType)
{
    ctl::ComPtr<xaml_markup::IXamlMetadataProvider> spMetadataProvider;
    ctl::ComPtr<xaml_markup::IXamlType> spXamlType;

    IFC_RETURN(TryGetMetadataProvider(&spMetadataProvider));
    if (spMetadataProvider != nullptr)
    {
        IFC_RETURN(spMetadataProvider->GetXamlType(typeName, &spXamlType));
    }

    if (spXamlType != nullptr)
    {
        IFC_RETURN(ImportClassInfo(spXamlType.Get(), ppType));
    }
    else
    {
        IFC_RETURN(ImportUnknownClassInfo(typeName, ppType));
    }

    return S_OK;
}

// Imports a namespace into the runtime namespace cache.
_Check_return_ HRESULT MetadataAPI::ImportNamespaceInfo(_In_ const xstring_ptr_view& strNamespaceName, _Outptr_ const CNamespaceInfo** ppNamespace)
{
    CNamespaceInfo* pNamespace = nullptr;
    DynamicMetadataStorageInstanceWithLock storage;

    pNamespace = new CNamespaceInfo();
    pNamespace->m_nIndex = storage->GetNextAvailableNamespaceIndex();
    IFC_RETURN(strNamespaceName.Promote(&pNamespace->m_strNameStorage));

    // Add the namespace to the runtime caches.
    storage->m_customNamespacesByNameCache[xstring_ptr(pNamespace->m_strNameStorage)] = pNamespace;

    storage->m_customNamespacesCache.emplace_back(pNamespace);

    *ppNamespace = pNamespace;

    return S_OK;
}

// Imports an IXamlMember.
_Check_return_ HRESULT MetadataAPI::ImportPropertyInfo(_In_ const CClassInfo* declaringType, _In_ xaml_markup::IXamlMember* pXamlProperty, _Outptr_ const CDependencyProperty** ppProperty)
{
    CCustomProperty* pProperty = nullptr;
    ctl::ComPtr<xaml_markup::IXamlType> spXamlPropertyType;
    const CClassInfo* pPropertyType = nullptr;
    wrl_wrappers::HString strPropertyName;
    xstring_ptr strLocalPropertyName;
    BOOLEAN bFlag = FALSE;
    MetaDataPropertyInfoFlags ePropertyFlags = (MetaDataPropertyInfoFlags::IsPublic | MetaDataPropertyInfoFlags::IsSparse | MetaDataPropertyInfoFlags::IsNullable);
    DynamicMetadataStorageInstanceWithLock storage;

    // Get the property's type.
    IFC_RETURN(pXamlProperty->get_Type(&spXamlPropertyType));

    if (spXamlPropertyType != nullptr)
    {
        IFC_RETURN(GetClassInfoByXamlType(spXamlPropertyType.Get(), &pPropertyType));
    }
    else
    {
        pPropertyType = GetClassInfoByIndex(KnownTypeIndex::Object);
    }

    // Get the property name.
    IFC_RETURN(pXamlProperty->get_Name(strPropertyName.GetAddressOf()));
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(strPropertyName.Get(), &strLocalPropertyName));

    // Resolve property flags.
    if (SUCCEEDED(pXamlProperty->get_IsReadOnly(&bFlag)) && bFlag)   ePropertyFlags |= MetaDataPropertyInfoFlags::IsReadOnlyProperty;
    if (SUCCEEDED(pXamlProperty->get_IsAttachable(&bFlag)) && bFlag) ePropertyFlags |= MetaDataPropertyInfoFlags::IsAttached;

    // Create the property info.
    IFC_RETURN(CCustomProperty::Create(
        storage->GetNextAvailablePropertyIndex(),   // Get unique property index.
        pPropertyType->GetIndex(),
        declaringType->GetIndex(),
        ePropertyFlags,
        pXamlProperty,
        strLocalPropertyName,
        &pProperty));

    // Add the property to the runtime type caches.
    IFC_RETURN(AssociateProperty(declaringType, pProperty));
    storage->m_customPropertiesCache.push_back(pProperty);

    if (!declaringType->IsBuiltinType())
    {
        IFC_RETURN(const_cast<CClassInfo*>(declaringType)->RunClassConstructorIfNecessary());
    }

    *ppProperty = pProperty;

    return S_OK;
}

// Imports an unknown class. Unknown classes are used for type references from DependencyProperty::Register calls to types
// that are not described by anything.
_Check_return_ HRESULT MetadataAPI::ImportUnknownClassInfo(_In_ const wxaml_interop::TypeName typeName, _Outptr_ const CClassInfo** ppType)
{
    CCustomClassInfo* pType = nullptr;
    DynamicMetadataStorageInstanceWithLock storage;
    KnownTypeIndex eTypeIndex = storage->GetNextAvailableTypeIndex();
    MetaDataTypeInfoFlags eTypeFlags = MetaDataTypeInfoFlags::IsCustomType;

    xstring_ptr strTypeFullName;
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(typeName.Name, &strTypeFullName));

    if (typeName.Kind == wxaml_interop::TypeKind::TypeKind_Custom)
    {
        xstring_ptr spNoName;

        // Create the class info.
        IFC_RETURN(CCustomClassInfo::Create(
            KnownNamespaceIndex::UnknownNamespace,
            GetClassInfoByIndex(KnownTypeIndex::UnknownType), // We don't know the base type.
            eTypeIndex,
            eTypeFlags,
            nullptr, // There's no backing IXamlType.
            spNoName, // No need for a short name.
            strTypeFullName,
            false, // isBindable
            KnownTypeIndex::UnknownType, // We don't know the boxed type, if it exists
            &pType));

        // Add the type to the runtime type caches.
        VERIFY_COND(storage->m_customTypesByCustomNameCache.insert({ strTypeFullName, pType }), .second);

        storage->m_customTypesCache.emplace_back(pType);

        // We never store a dummy type in m_mapTypesByName. That's ok though, because it should only fail lookups from XAML. When someone
        // refers to custom types in XAML, there should always be an entry in IXamlMetadataProvider, and as such we should go through
        // AddCustomType instead of AddDummyType. AddCustomType will store a mapping in m_mapTypesByName for non-WinRT types.
    }
    else
    {
        xephemeral_string_ptr strNamespaceName;
        xephemeral_string_ptr strTypeName;
        KnownNamespaceIndex eNamespaceIndex = KnownNamespaceIndex::UnknownNamespace;

        IFC_RETURN(ExtractNamespaceNameAndShortName(strTypeFullName, &strNamespaceName, &strTypeName));
        if (!strNamespaceName.IsNullOrEmpty())
        {
            const CNamespaceInfo* pNamespace = nullptr;
            IFC_RETURN(GetNamespaceByName(strNamespaceName, &pNamespace));
            eNamespaceIndex = pNamespace->GetIndex();
        }

        // Create the class info.
        IFC_RETURN(CCustomClassInfo::Create(
            eNamespaceIndex,
            GetClassInfoByIndex(KnownTypeIndex::UnknownType), // We don't know the base type.
            eTypeIndex,
            eTypeFlags,
            nullptr, // There's no backing IXamlType.
            strTypeName,
            strTypeFullName,
            false, // isBindable
            KnownTypeIndex::UnknownType, // We don't know the boxed type, if it exists
            &pType));

        // Add the type to the runtime type caches.
        VERIFY_COND(storage->m_customTypesByNameCache.insert({ strTypeFullName, pType }), .second);

        storage->m_customTypesCache.emplace_back(pType);
    }

    *ppType = pType;

    return S_OK;
}

// Checks whether the specified name used to be (<=Blue) an event name that we silently ignored at runtime.
bool MetadataAPI::IsLegacyEventName(_In_ const CClassInfo* pType, _In_ const xstring_ptr_view& strName)
{
    for (int i = 0; i < ARRAY_SIZE(c_strLegacyEventNames); i++)
    {
        if (strName.Equals(c_strLegacyEventNames[i].Buffer, c_strLegacyEventNames[i].Count))
        {
            return true;
        }
    }

    return false;
}

bool FastCompareKnownTypesHelper(KnownTypeIndex eTargetTypeIndex, KnownTypeIndex eSourceTypeIndex)
{
    ASSERT(MetadataAPI::IsKnownIndex(eTargetTypeIndex));
    ASSERT(MetadataAPI::IsKnownIndex(eSourceTypeIndex));
    ASSERT(eSourceTypeIndex != eTargetTypeIndex);

    const TypeCheckData& targetData = c_aTypeCheckData[static_cast<UINT>(eTargetTypeIndex)];
    const UINT64 targetHandle = targetData.GetHandle();

    // A leaf type can only be assigned to with a value of that type. We only verified that we're not comparing the same types
    // though, so the target type can not be a leaf type at this point.
    if (targetHandle != 0 && targetData.IsNotLeaf())
    {
        const UINT64 sourceHandle = c_aTypeCheckData[static_cast<UINT>(eSourceTypeIndex)].GetHandle();
        return (sourceHandle & targetData.GetHandleMask()) == targetHandle;
    }
    else
    {
        // For types not having type handle data, don't allow assignment
        return false;
    }
}

bool MetadataAPI::CompareCustomTypesHelper(const CClassInfo* typeClass, KnownTypeIndex targetType)
{
    for (const CClassInfo* type = typeClass; type->m_nIndex != KnownTypeIndex::UnknownType; type = type->GetBaseType())
    {
        if (type->m_nIndex == targetType)
        {
            return true;
        }
    }

    return false;
}

// Determines whether the specified target type is assignable from the specified source type.
bool MetadataAPI::IsAssignableFrom(_In_ const CClassInfo* pTargetType, _In_ const CClassInfo* pSourceType)
{
    KnownTypeIndex eSourceTypeIndex = pSourceType->GetIndex();
    KnownTypeIndex eTargetTypeIndex = pTargetType->GetIndex();

    ASSERT(eSourceTypeIndex != KnownTypeIndex::UnknownType);
    ASSERT(eTargetTypeIndex != KnownTypeIndex::UnknownType);

    if (pTargetType == pSourceType || eTargetTypeIndex == KnownTypeIndex::Object)
    {
        return true;
    }
    else if (pTargetType->IsBuiltinType() && pSourceType->IsBuiltinType())
    {
        return FastCompareKnownTypesHelper(eTargetTypeIndex, eSourceTypeIndex);
    }
    else
    {
        // No fast type check available. Loop through the base types.
        return CompareCustomTypesHelper(pSourceType, eTargetTypeIndex);
    }
}

// Determines whether the specified target type is assignable from the specified source type.
bool MetadataAPI::IsAssignableFrom(_In_ KnownTypeIndex eTargetTypeIndex, _In_ KnownTypeIndex eSourceTypeIndex)
{
    ASSERT(eSourceTypeIndex != KnownTypeIndex::UnknownType);
    ASSERT(eTargetTypeIndex != KnownTypeIndex::UnknownType);

    if (eTargetTypeIndex == eSourceTypeIndex || eTargetTypeIndex == KnownTypeIndex::Object)
    {
        return true;
    }
    else if (IsKnownIndex(eTargetTypeIndex) && IsKnownIndex(eSourceTypeIndex))
    {
        return FastCompareKnownTypesHelper(eTargetTypeIndex, eSourceTypeIndex);
    }
    else
    {
        // No fast type check available. Loop through the base types.
        return CompareCustomTypesHelper(GetClassInfoByIndex(eSourceTypeIndex), eTargetTypeIndex);
    }
}

// Overrides the default metadata provider.
void MetadataAPI::OverrideMetadataProvider(_In_opt_ xaml_markup::IXamlMetadataProvider* pMetadataProvider)
{
    DynamicMetadataStorageInstanceWithLock storage;
    storage->m_overriddenMetadataProvider = pMetadataProvider;
    storage->m_metadataProvider = nullptr;
}

// Processes the queued DP registrations.
_Check_return_ HRESULT MetadataAPI::ProcessRegistrations()
{
    // Collection of objects to be released after "storage" goes out of scope and releases CStaticLock
    Jupiter::stack_vector<ctl::ComPtr<xaml::IPropertyMetadata>, 32> propertyMetadataKeepAliveVector;

    // Note that DynamicMetadataStorageInstanceWithLock takes the CStaticLock while it's alive.
    DynamicMetadataStorageInstanceWithLock storage;
    if (storage->m_queuedDPRegistrations != nullptr)
    {
        while (!storage->m_queuedDPRegistrations->empty())
        {
            const auto& registration = storage->m_queuedDPRegistrations->front();

            CCustomDependencyProperty* pDP = registration.m_dp;
            const CClassInfo* pPropertyType = nullptr;
            const CClassInfo* pDeclaringType = nullptr;

            xruntime_string_ptr propertyTypeNamePromoted, ownerTypeNamePromoted;

            IFC_RETURN(registration.m_propertyTypeName.Promote(&propertyTypeNamePromoted));
            IFC_RETURN(registration.m_declaringTypeName.Promote(&ownerTypeNamePromoted));

            wxaml_interop::TypeName propertyType = { propertyTypeNamePromoted.GetHSTRING(), registration.m_propertyTypeKind };
            wxaml_interop::TypeName declaringType = { ownerTypeNamePromoted.GetHSTRING(), registration.m_declaringTypeKind };

            // Resolve the type references.
            IFC_RETURN(GetClassInfoByTypeName(propertyType, &pPropertyType));
            IFC_RETURN(GetClassInfoByTypeName(declaringType, &pDeclaringType));

            // Update the property info.
            pDP->UpdateTypeInfo(
                pPropertyType->GetIndex(),
                pDeclaringType->GetIndex(),
                (pDP->IsAttached()) ? KnownTypeIndex::Object : pDeclaringType->GetIndex());

            if (registration.m_defaultMetadata != nullptr)
            {
                IFC_RETURN(pDP->SetDefaultMetadata(registration.m_defaultMetadata.Get()));
            }

            // We're done with the registration, so remove it from the queue.
            // This is popped before AssociateDepenendcyProperty to prevent re-entrancy
            // in some cases such as when AssociateDependencyProperty needs to update the
            // underlying DP which could invoke another ProcessRegistrations when resolving
            // the name for the wrapper DP.

            // We don't want to let m_defaultMetadata do its final release while holding the lock in
            // DynamicMetadataStorageInstanceWithLock (CStaticLock). If the final release happens right here, while
            // we're holding the DynamicMetadataStorageInstanceWithLock lock, we can hit a deadlock because
            // WeakReferenceSourceNoThreadId::OnFinalReleaseOffThread will call
            // ReferenceTrackerManager::SearchCoresForObject, which aquires the ReferenceTrackerManager::s_lock.  That
            // sounds fine, except that in other code locations where we aquire ReferenceTrackerManager::s_lock _before_
            // CStaticLock.  Deadlocks can occur when we acquire the same two locks in different orders.
            const DynamicMetadataStorage::DPRegistrationInfo& front = storage->m_queuedDPRegistrations->front();
            propertyMetadataKeepAliveVector.m_vector.push_back(front.m_defaultMetadata);
            storage->m_queuedDPRegistrations->pop();

            // Add to the runtime type caches.
            IFC_RETURN(AssociateDependencyProperty(pDeclaringType, pDP));
        }
    }

    return S_OK;
}

// Resets the runtime type caches.
void MetadataAPI::Reset()
{
    DynamicMetadataStorageInstanceWithLock::Reset();
}

void MetadataAPI::Destroy()
{
    DynamicMetadataStorageInstanceWithLock::Destroy();
}

// Sets the metadata provider to use for resolving custom types.
_Check_return_ HRESULT MetadataAPI::SetMetadataProvider(_In_ xaml_markup::IXamlMetadataProvider* pMetadataProvider)
{
    DynamicMetadataStorageInstanceWithLock storage;
    storage->m_metadataProvider = pMetadataProvider;
    return S_OK;
}

// Tries to resolve an attached property by its name. strName should use the format ClassName.PropertyName. This should only
// be called for built-in attached properties.
_Check_return_ HRESULT MetadataAPI::TryGetAttachedPropertyByName(_In_ const xstring_ptr_view& strName, _Outptr_result_maybenull_ const CDependencyProperty** ppDP)
{
    // The property was not found or we can't use the parser context. Assume that the property is in the default namespace.
    // The property will be of the format: ClassName.PropertyName.
    // We will extract the class name first then the property name and do the lookup using the type tables.
    auto nDotIndex = strName.FindChar(L'.');
    if (nDotIndex != xstring_ptr_view::npos)
    {
        // Try to resolve the type.
        xephemeral_string_ptr strClassName;
        strName.SubString(0, nDotIndex, &strClassName);
        const CClassInfo* pType = GetBuiltinClassInfoByName(strClassName);

        if (pType != nullptr)
        {
            // Try to resolve the property.
            xephemeral_string_ptr strPropertyName;
            strName.SubString(nDotIndex + 1, strName.GetCount(), &strPropertyName);
            IFC_RETURN(TryGetDependencyPropertyByName(pType, strPropertyName, ppDP));
        }
    }
    return S_OK;
}

// Given a fully qualified property name (including namespace, e.g. "Windows.UI.Xaml.Shapes.Shape.Fill" instead of "Fill"),
// return the property.  Will search for a dependency property, simple property, or custom user property (in that order).
_Check_return_ HRESULT MetadataAPI::TryGetPropertyByFullName(_In_ const xstring_ptr_view& strName, _Outptr_result_maybenull_ const CPropertyBase** ppProp)
{
    const CClassInfo* pType = nullptr;
    xstring_ptr strClassName;
    xstring_ptr strPropertyName;

    *ppProp = nullptr;
    unsigned int nLastDotIndex = strName.FindLastChar(L'.', strName.GetCount());
    if (nLastDotIndex != xstring_ptr_view::npos)
    {
        //Get the property's owning class from its full name.
        IFC_RETURN(strName.SubString(0, nLastDotIndex, &strClassName));
        IFC_RETURN(GetClassInfoByFullName(strClassName, &pType));

        if (pType != nullptr)
        {
            //We resolved the base class, now use its class info to determine the property.
            IFC_RETURN(strName.SubString(nLastDotIndex + 1, strName.GetCount(), &strPropertyName));

            const CDependencyProperty* pDP = nullptr;

            // First, search for dependency properties
            IFC_RETURN(TryGetDependencyPropertyByName(pType, strPropertyName, &pDP, true /*allowDirectives*/));
            if (pDP != nullptr)
            {
                *ppProp = static_cast<const CPropertyBase*>(pDP);
            }

            // Next, search for simple properties
            if (*ppProp == nullptr)
            {
                *ppProp = TryGetBuiltInPropertyBaseByName(pType, strPropertyName, true /*allowDirectives*/);
            }

            // Last, try custom user properties
            if (*ppProp == nullptr)
            {
                IFC_RETURN(TryGetPropertyByName(pType, strPropertyName, &pDP, true /*allowDirectives*/));
            }

            if (pDP != nullptr && *ppProp == nullptr)
            {
                *ppProp = static_cast<const CPropertyBase*>(pDP);
            }
        }
        else
        {
            //If we could parse a class from the string, but couldn't resolve it, fail.
            return E_INVALIDARG;
        }
    }
    else
    {
        //If there was no '.' in the string, can't separate class/property, so fail.
        return E_INVALIDARG;
    }

    return S_OK;
}

// Try to find a built-in type by the specified full name.
const CClassInfo* MetadataAPI::TryGetBuiltinClassInfoByFullName(_In_ const xstring_ptr_view& strTypeFullName)
{
    bool fIsPrimitiveType = false;
    const CClassInfo* typeInfo = nullptr;
    // Only do a lookup in our type table if the name looks familiar (e.g. is in the Windows namespace).
    if (ShouldLookupInBuiltinTypeTable(strTypeFullName, fIsPrimitiveType))
    {
        xephemeral_string_ptr strTypeName;
        ExtractShortName(strTypeFullName, &strTypeName);
        typeInfo = GetBuiltinClassInfoByName(strTypeName);

        // Verify the type we resolved matches the full name, unless this is a primitive type. We skip this check
        // for primitive types because we allow lookups to happen for both "X" and "Windows.Foundation.X".
        if (typeInfo != nullptr && !fIsPrimitiveType && typeInfo->GetFullName().GetCount() != strTypeFullName.GetCount())
        {
            // We the wrong type. Reset the return value.
            return nullptr;
        }
    }

    return typeInfo;
}

// Try to find a type by the specified full name.
_Check_return_ HRESULT MetadataAPI::TryGetClassInfoByFullName(_In_ const xstring_ptr_view& strTypeFullName, _In_ bool bSearchCustomTypesOnly, _Outptr_ const CClassInfo** ppType)
{
    if (!bSearchCustomTypesOnly)
    {
        *ppType = TryGetBuiltinClassInfoByFullName(strTypeFullName);
    }

    // Type doesn't exist in our type table. See if it exists in our runtime type cache.
    if (*ppType == nullptr)
    {
        DynamicMetadataStorageInstanceWithLock storage;
        auto itCustomTypes = storage->m_customTypesByNameCache.find(strTypeFullName);
        if (itCustomTypes == storage->m_customTypesByNameCache.end())
        {
            // We don't know about this type. Resolve it now.
            IFC_RETURN(ImportClassInfoFromMetadataProvider(strTypeFullName, ppType));
        }
        else
        {
            *ppType = itCustomTypes->second;
        }
    }

    return S_OK;
}

// Try to find a type by the specified namespace index and the type's short name.
_Check_return_ HRESULT MetadataAPI::TryGetClassInfoByName(_In_ KnownNamespaceIndex eNamespaceIndex, _In_ const xstring_ptr_view& strTypeName, _Outptr_ const CClassInfo** ppType)
{
    // Check if this is a built-in namespace (such as Windows.UI.Xaml) and, if so,
    // do a quick lookup through our list of built-in type names.
    if (IsKnownIndex(eNamespaceIndex))
    {
        *ppType = GetBuiltinClassInfoByName(strTypeName);

        if (*ppType != nullptr)
        {
            // Resolved the type, stop searching.
            return S_OK;
        }
    }

    // Search the runtime type cache
    DynamicMetadataStorageInstanceWithLock storage;

    XStringBuilder strFullNameBuilder;
    xstring_ptr strNamespaceName = GetNamespaceByIndex(eNamespaceIndex)->GetName();
    xstring_ptr strFullName;

    // Create a full name (e.g. "MyApp.MyControl").
    IFC_RETURN(strFullNameBuilder.Initialize(strNamespaceName.GetCount() + /* "." */ 1 + strTypeName.GetCount()));
    IFC_RETURN(strFullNameBuilder.Append(strNamespaceName));
    IFC_RETURN(strFullNameBuilder.AppendChar(L'.'));
    IFC_RETURN(strFullNameBuilder.Append(strTypeName));
    IFC_RETURN(strFullNameBuilder.DetachString(&strFullName));

    // Unable to resolve so far. Try the IXamlMetadataProvider for custom types (this includes types from our extension DLLs)
    IFC_RETURN(TryGetClassInfoByFullName(strFullName, /* bSearchCustomTypesOnly */ TRUE, ppType));

    return S_OK;
}

const CPropertyBase* MetadataAPI::TryGetBuiltInPropertyBaseByName(
    _In_ const CClassInfo* pType,
    _In_ const xstring_ptr_view& strName,
    _In_ bool allowDirectives)
{
    if (IsKnownIndex(pType->GetIndex()))
    {
        // Loop through the type's built-in DPs.

        for (const CPropertyBase* pb = pType->GetFirstProperty();
             pb->GetIndex() != KnownPropertyIndex::UnknownType_UnknownProperty;
             pb = pb->GetNextProperty())
        {
            if (strName.Equals(pb->GetName()))
            {
                // Handle directive properties (such as x:Name).

                if (allowDirectives || !pb->IsDirective())
                {
                    return pb;
                }
            }
        }
    }

    return nullptr;
}

// Try to find a dependency property on the specified type with the specified name.
_Check_return_ HRESULT MetadataAPI::TryGetDependencyPropertyByName(
     _In_ const CClassInfo* pType,
     _In_ const xstring_ptr_view& strName,
     _Outptr_result_maybenull_ const CDependencyProperty** prop,
     _In_ bool allowDirectives)
{
    *prop = nullptr;

    if (IsKnownIndex(pType->GetIndex()))
    {
        const CPropertyBase* builtIn = TryGetBuiltInPropertyBaseByName(
            pType,
            strName,
            allowDirectives);

        if (builtIn)
        {
            // This assert is no longer valid, as we have built-in simple properties as well.  Those are represented by
            // CSimpleProperty, which is a CPropertyBase but not a CDependencyProperty.  If this is a simple property,
            // we just want to return null (to actually get a simple property, use TryGetBuiltinPropertyBaseByName directly)
            //ASSERT(builtIn->Is<CDependencyProperty>());
            *prop = builtIn->AsOrNull<CDependencyProperty>();
            return S_OK;
        }
    }
    else
    {
        IFC_RETURN(const_cast<CClassInfo*>(pType)->RunClassConstructorIfNecessary());
    }

    // TODO: [TFS 394095] Add a quirk. We shouldn't be executing the following code for built-in types, but we
    //       have to for compat reasons. Some apps register custom (attached) DPs for built-in types, and
    //       we resolved them in TemplateBindings/etc. just fine.

    // Make sure we processed all DP registrations, so we can resolve properties by name.
    IFC_RETURN(ProcessRegistrations());

    {
        DynamicMetadataStorageInstanceWithLock storage;

        *prop = TryFindCustomPropertyHelper(
             storage->m_customDPsByTypeAndNameCache.get(),
             pType,
             strName);

        if (*prop)
        {
            return S_OK;
        }
    }

    // We weren't able to resolve the property. Try looking on the base class.
    const CClassInfo* pBaseType = pType->GetBaseType();

    // TODO: [TFS 394068] Add a quirk. Starting with Threshold, we should consider looking for DPs registered on "Object" as well.
    // if (pBaseType->GetIndex() != KnownTypeIndex::UnknownType)
    const bool isDO = (pType->GetIndex() == KnownTypeIndex::DependencyObject);

    if (pBaseType->GetIndex() != KnownTypeIndex::UnknownType &&
       // For app compat with Windows 8.1, we look at properties registered with an 'Object' owner
       // except if the current type is (or inherits from) DependencyObject.
       !(pBaseType->GetIndex() == KnownTypeIndex::Object && isDO))
    {
        IFC_RETURN(TryGetDependencyPropertyByName(pBaseType, strName, prop, allowDirectives));

        if (*prop != nullptr)
        {
            // Store a reference to the inherited property, so we don't need to traverse our base types to find it in the future.
            IFC_RETURN(AssociateDependencyProperty(pType, *prop));
        }
    }

    return S_OK;
}

// Tries to resolve a property via the IXamlMetadataProvider and adds it to the runtime type cache.
_Check_return_ HRESULT MetadataAPI::TryImportPropertyInfoFromMetadataProvider(_In_ const CClassInfo* pType, _In_ const xstring_ptr_view& strName, _Outptr_ const CDependencyProperty** ppProperty)
{
    *ppProperty = nullptr;

    IFCEXPECT_RETURN(!pType->IsBuiltinType());

    // Make sure we were able to resolve this as a custom type.
    const CCustomClassInfo* customType = pType->AsCustomType();

    if (customType != nullptr)
    {
        xaml_markup::IXamlType* xamlTypeNoRef = customType->GetXamlTypeNoRef();
        if (xamlTypeNoRef != nullptr)
        {
            ctl::ComPtr<xaml_markup::IXamlMember> xamlProperty;
            xruntime_string_ptr namePromoted;

            IFC_RETURN(strName.Promote(&namePromoted));
            IFC_RETURN(xamlTypeNoRef->GetMember(namePromoted.GetHSTRING(), &xamlProperty));
            if (xamlProperty != nullptr)
            {
                IFC_RETURN(ImportPropertyInfo(pType, xamlProperty.Get(), ppProperty));
            }
        }
    }

    return S_OK;
}

// Try to find a property on the specified type with the specified name.
_Check_return_ HRESULT MetadataAPI::TryGetPropertyByName(
    _In_ const CClassInfo* pType,
    _In_ const xstring_ptr_view& strName,
    _Outptr_result_maybenull_ const CDependencyProperty** ppProperty,
    _In_ bool allowDirectives)
{
    *ppProperty = nullptr;

    if (IsKnownIndex(pType->GetIndex()))
    {
        // Built-in DPs also act as their own property wrappers.

        const CDependencyProperty* dp = nullptr;

        IFC_RETURN(TryGetDependencyPropertyByName(
            pType,
            strName,
            &dp,
            allowDirectives));

        if (dp != nullptr && !dp->Is<CCustomDependencyProperty>())
        {
            // Ignore custom registered DPs.
            *ppProperty = dp;
        }
    }
    else
    {
        {
            DynamicMetadataStorageInstanceWithLock storage;

            *ppProperty = TryFindCustomPropertyHelper(
                storage->m_customPropertiesByTypeAndNameCache.get(),
                pType,
                strName);

            if (*ppProperty)
            {
                return S_OK;
            }
        }

        // We don't know about this property. Try to resolve it now.
        IFC_RETURN(TryImportPropertyInfoFromMetadataProvider(pType, strName, ppProperty));

        if (*ppProperty)
        {
            return S_OK;
        }

        // We weren't able to resolve the property. Try looking on the base class.
        const CClassInfo* pBaseType = pType->GetBaseType();

        if (pBaseType->GetIndex() != KnownTypeIndex::UnknownType)
        {
            IFC_RETURN(TryGetPropertyByName(pBaseType, strName, ppProperty, allowDirectives));

            if (*ppProperty != nullptr)
            {
                // Store a reference to the inherited property, so we don't need to traverse our base types to find it in the future.
                IFC_RETURN(AssociateProperty(pType, *ppProperty));
            }
        }
    }

    return S_OK;
}

// Gets the underlying dependency property from a property. Use this if pProperty may
// refer to a regular property, and you want the underlying DP for it. If it refers to
// a DP already, this function will return a reference to that DP.
_Check_return_ HRESULT MetadataAPI::TryGetUnderlyingDependencyProperty(_In_ const CDependencyProperty* pProperty, _Outptr_result_maybenull_ const CDependencyProperty** ppDP)
{
    if (auto customProperty = pProperty->AsOrNull<CCustomProperty>())
    {
        IFC_RETURN(const_cast<CCustomProperty*>(customProperty)->TryGetUnderlyingDP(ppDP));
    }
    else
    {
        *ppDP = pProperty;
    }

    return S_OK;
}

_Check_return_ HRESULT MetadataAPI::EnsureDependencyPropertyInitialized(_In_ const CDependencyProperty* prop)
{
    if (prop->Is<CCustomProperty>() ||
       (prop->Is<CCustomDependencyProperty>() && !prop->IsInitialized()))
    {
        IFC_RETURN(MetadataAPI::ProcessRegistrations());
    }

    return S_OK;
}

MetaDataNamespaceNonAggregate::MetaDataNamespaceNonAggregate()
    : m_nIndex(KnownNamespaceIndex::UnknownNamespace)
    , m_strNameStorage()
{
}

MetaDataTypeNonAggregate::MetaDataTypeNonAggregate()
    : m_nIndex(KnownTypeIndex::UnknownType)
    , m_flags(MetaDataTypeInfoFlags::None)
    , m_nBaseTypeIndex(KnownTypeIndex::UnknownType)
{
}

MetaDataPropertyNonAggregate::MetaDataPropertyNonAggregate()
    : m_nIndex(KnownPropertyIndex::UnknownType_UnknownProperty)
    , m_flags(MetaDataPropertyInfoFlags::None)
    , m_nPropertyTypeIndex(KnownTypeIndex::UnknownType)
    , m_nDeclaringTypeIndex(KnownTypeIndex::UnknownType)
    , m_nTargetTypeIndex(KnownTypeIndex::UnknownType)
{
}

MetaDataEnterPropertyNonAggregate::MetaDataEnterPropertyNonAggregate()
    : m_nOffset(0)
    , m_nGroupOffset(0)
    , m_flags(MetaDataEnterPropertyInfoFlags::None)
    , m_nPropertyIndex(KnownPropertyIndex::UnknownType_UnknownProperty)
    , m_nNextEnterPropertyIndex(0)
{
}

MetaDataObjectPropertyNonAggregate::MetaDataObjectPropertyNonAggregate()
    : m_nOffset(0)
    , m_nGroupOffset(0)
    , m_nPropertyIndex(KnownPropertyIndex::UnknownType_UnknownProperty)
    , m_nNextObjectPropertyIndex(0)
{
}

MetaDataRenderPropertyNonAggregate::MetaDataRenderPropertyNonAggregate()
    : m_nOffset(0)
    , m_nGroupOffset(0)
    , m_nPropertyIndex(KnownPropertyIndex::UnknownType_UnknownProperty)
    , m_nNextRenderPropertyIndex(0)
{
}

const CClassInfo* CClassInfo::GetBaseType() const
{
    return MetadataAPI::GetClassInfoByIndex(m_nBaseTypeIndex);
}

const CDependencyProperty* CClassInfo::GetContentProperty() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return MetadataAPI::GetDependencyPropertyByIndex(c_aTypeProperties[static_cast<UINT>(m_nIndex)].m_nContentPropertyIndex);
    }
    else
    {
        return MetadataAPI::GetDependencyPropertyByIndex(AsCustomType()->m_nContentPropertyIndex);
    }
}

const CPropertyBase* CClassInfo::GetFirstProperty() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return MetadataAPI::GetPropertyBaseByIndex(c_aTypeProperties[static_cast<UINT>(m_nIndex)].m_nFirstPropertyIndex);
    }
    else
    {
        // We only return built-in properties.
        return GetBaseType()->GetFirstProperty();
    }
}

const CNamespaceInfo* CClassInfo::GetNamespace() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return MetadataAPI::GetNamespaceByIndex(c_aTypeNameInfos[static_cast<UINT>(m_nIndex)].m_nNamespaceIndex);
    }
    else
    {
        return MetadataAPI::GetNamespaceByIndex(AsCustomType()->m_nNamespaceIndex);
    }
}

REFIID CClassInfo::GetGuid() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return c_aTypeUUIDs[static_cast<UINT>(m_nIndex)];
    }
    else
    {
        // We currently don't store UUIDs for custom types.
        return GUID_NULL;
    }
}

xstring_ptr CClassInfo::GetFullName() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return xstring_ptr(c_aTypeNameInfos[static_cast<UINT>(m_nIndex)].m_strFullNameStorage);
    }
    else
    {
        return AsCustomType()->m_strFullName;
    }
}

xstring_ptr CClassInfo::GetName() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return xstring_ptr(c_aTypeNameInfos[static_cast<UINT>(m_nIndex)].m_strNameStorage);
    }
    else
    {
        return AsCustomType()->m_strName;
    }
}

bool CClassInfo::IsBindable() const
{
    return !IsBuiltinType() && AsCustomType()->IsBindable();
}

xstring_ptr CPropertyBase::GetName() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return xstring_ptr(c_aPropertyNames[static_cast<UINT>(m_nIndex)]);
    }
    else if (auto customDependencyProperty = AsOrNull<CCustomDependencyProperty>())
    {
        return customDependencyProperty->m_strName;
    }
    else if (auto customProperty = AsOrNull<CCustomProperty>())
    {
        return customProperty->m_strName;
    }
    else
    {
        ASSERT(FALSE);
        return xstring_ptr::NullString();
    }
}

const CClassInfo* CPropertyBase::GetDeclaringType() const
{
    return MetadataAPI::GetClassInfoByIndex(GetDeclaringTypeIndex());
}

const CClassInfo* CPropertyBase::GetPropertyType() const
{
    return MetadataAPI::GetClassInfoByIndex(GetPropertyTypeIndex());
}

const CClassInfo* CPropertyBase::GetTargetType() const
{
    return MetadataAPI::GetClassInfoByIndex(GetTargetTypeIndex());
}

const CPropertyBase* CPropertyBase::GetNextProperty() const
{
    if (MetadataAPI::IsKnownIndex(m_nIndex))
    {
        return MetadataAPI::GetPropertyBaseByIndex(c_aNextProperty[static_cast<UINT>(m_nIndex)]);
    }
    else
    {
        return MetadataAPI::GetPropertyBaseByIndex(KnownPropertyIndex::UnknownType_UnknownProperty);
    }
}

bool CPropertyBase::IsKnownDependencyPropertyIndexHelper() const
{
    return MetadataAPI::IsKnownDependencyPropertyIndex(m_nIndex);
}

bool CCustomClassInfo::RepresentsBoxedType() const
{
    return m_boxedTypeIndex != KnownTypeIndex::UnknownType;
}

const CClassInfo* CCustomClassInfo::GetBoxedType() const
{
    return MetadataAPI::GetClassInfoByIndex(m_boxedTypeIndex);
}

_Check_return_ HRESULT CCustomClassInfo::Create(
    _In_ KnownNamespaceIndex eNamespaceIndex,
    _In_ const CClassInfo* pBaseType,
    _In_ KnownTypeIndex eTypeIndex,
    _In_ MetaDataTypeInfoFlags eTypeFlags,
    _In_ xaml_markup::IXamlType* pXamlType,
    _In_ const xstring_ptr_view& strName,
    _In_ const xstring_ptr_view& strFullName,
    _In_ bool isBindable,
    _In_ KnownTypeIndex boxedTypeIndex,
    _Outptr_ CCustomClassInfo** ppType
    )
{
    CCustomClassInfo* pType = nullptr;

    pType = new CCustomClassInfo();
    pType->m_isBindable = isBindable;
    pType->m_nNamespaceIndex = eNamespaceIndex;
    pType->m_nBaseTypeIndex = pBaseType->GetIndex();
    pType->m_nIndex = eTypeIndex;
    pType->m_flags = eTypeFlags;
    pType->m_spXamlType = pXamlType;
    pType->m_nContentPropertyIndex = pBaseType->GetContentProperty()->GetIndex();
    pType->m_boxedTypeIndex = boxedTypeIndex;

    IFC_RETURN(strName.Promote(&pType->m_strName));
    IFC_RETURN(strFullName.Promote(&pType->m_strFullName));

    *ppType = pType;

    return S_OK;
}

_Check_return_ HRESULT CCustomDependencyProperty::Create(
    _In_ KnownPropertyIndex index,
    _In_ MetaDataPropertyInfoFlags flags,
    _In_ const xstring_ptr_view& strName,
    _Outptr_ CCustomDependencyProperty** ppDP)
{
    xstring_ptr name;

    IFC_RETURN(strName.Promote(&name));

    *ppDP = new CCustomDependencyProperty(
        index,
        flags,
        std::move(name));

    return S_OK;
}

_Check_return_ HRESULT CCustomDependencyProperty::EnsureTypesResolved()
{
    IFC_RETURN(MetadataAPI::EnsureDependencyPropertyInitialized(this));
    return S_OK;
}

_Check_return_ HRESULT CCustomDependencyProperty::GetDefaultValue(_Outptr_ IInspectable** ppDefaultValue) const
{
    if (m_spCreateDefaultValueCallback != nullptr)
    {
        IFC_RETURN(m_spCreateDefaultValueCallback->Invoke(ppDefaultValue));
    }
    else
    {
        ctl::ComPtr<IInspectable> spDefaultValue = m_spDefaultValue;
        *ppDefaultValue = spDefaultValue.Detach();
    }

    return S_OK;
}

_Check_return_ HRESULT CCustomProperty::Create(
    _In_ KnownPropertyIndex index,
    _In_ KnownTypeIndex propertyTypeIndex,
    _In_ KnownTypeIndex declaringTypeIndex,
    _In_ MetaDataPropertyInfoFlags flags,
    _In_ xaml_markup::IXamlMember* xamlProperty,
    _In_ const xstring_ptr_view& strName,
    _Outptr_ CCustomProperty** ppProperty)
{
    xstring_ptr name;

    IFC_RETURN(strName.Promote(&name));

    *ppProperty = new CCustomProperty(
        index,
        propertyTypeIndex,
        declaringTypeIndex,
        flags,
        xamlProperty,
        std::move(name));

    return S_OK;
}

_Check_return_ HRESULT CCustomProperty::TryGetUnderlyingDP(_Outptr_ const CDependencyProperty** ppUnderlyingDP)
{
    IFC_RETURN(MetadataAPI::EnsureDependencyPropertyInitialized(this));

    if (m_pUnderlyingDP == nullptr)
    {
        IFC_RETURN(MetadataAPI::TryGetDependencyPropertyByName(GetDeclaringType(), GetName(), &m_pUnderlyingDP));
    }

    *ppUnderlyingDP = m_pUnderlyingDP;

    return S_OK;
}
