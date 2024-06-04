// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlBinaryMetadataReader2.h"
#include "XamlTypeNamespace.h"
#include "XamlUnknownXmlNamespace.h"
#include "XamlSerializationHelper.h"
#include "XbfMetadataApi.h"
#include "MetadataAPI.h"
#include "XamlTypeTokens.h"
#include <XcpAllocation.h>

using namespace Parser;

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strPropertyTableDescriptionStorage, L"Property Table");

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadMetadata()
{
    IFC_RETURN(LoadHeader());
    IFC_RETURN(LoadStringTable());
    IFC_RETURN(LoadAssemblyList());
    IFC_RETURN(LoadTypeNamespaceList());
    IFC_RETURN(LoadTypeList());
    IFC_RETURN(LoadPropertyList());
    IFC_RETURN(LoadXmlNamespaceList());
    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadHeader()
{
    IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeItemFromMetadataStream(&m_header, m_version, m_spMetadataStream));
    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadStringTable()
{
    DECLARE_CONST_XSTRING_PTR_STORAGE(c_strStringTableDescription, L"String Table");

    auto offsetGuard = wil::scope_exit([this]()
    {
        IGNOREHR(LogOffsetCheckError(xstring_ptr(c_strStringTableDescription)));
    });

    XUINT64 uPos = 0;
    IFC_RETURN(m_spMetadataStream->GetPosition(&uPos));
    IFCCHECK_RETURN(uPos == m_header.m_uStringTableOffset);

    offsetGuard.release();

    auto deserializationGuard = wil::scope_exit([this]()
    {
        IGNOREHR(LogDeserializationError(xstring_ptr(c_strStringTableDescription)));
    });

    if (m_version.ShouldNullTerminateStrings() && m_bufferType == Parser::XamlBufferType::MemoryMappedResource)
    {
        // Create xstring_ptr_storage wrappers for memory mapped XBF string buffers.

        XamlBinaryFormatSerializationHelper::VectorData vectorData;
        IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeItemFromMetadataStream(&vectorData, m_version, m_spMetadataStream));

        m_vecStringStorageList = std::make_shared<std::vector<xstring_ptr_storage>>(vectorData.uiCount);

        for (size_t stringIndex = 0; stringIndex < vectorData.uiCount; ++stringIndex)
        {
            uint32_t count = 0;
            IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeItemFromMetadataStream(&count, m_version, m_spMetadataStream));
            IFC_RETURN(m_spMetadataStream->GetPosition(&uPos));
            const WCHAR* data = reinterpret_cast<WCHAR*>(static_cast<BYTE*>(m_spMetadataMemory->GetAddress()) + uPos);

            xstring_ptr_storage& storage = m_vecStringStorageList->at(stringIndex);
            storage.Buffer = data;
            storage.Count = count;
            storage.IsEphemeral = FALSE;
            storage.IsRuntimeStringHandle = FALSE;

            // Increment the stream without reading the data, reading an extra char for the null terminator
            IFC_RETURN(m_spMetadataStream->Seek(uPos + (count + 1)*sizeof(WCHAR), PALSeekOrigin::SeekOriginStart, nullptr));
        }
    }
    else
    {
        // Create xstring_ptr copies of non-mapped buffers.

        m_vecStringList = std::make_shared<std::vector<xstring_ptr>>();
        IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(*m_vecStringList, m_version, m_spMetadataStream));
    }

    deserializationGuard.release();

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadAssemblyList()
{
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strReferenceTableDescription, L"Reference Table");

    auto offsetGuard = wil::scope_exit([this, &c_strReferenceTableDescription]()
    {
        IGNOREHR(LogOffsetCheckError(c_strReferenceTableDescription));
    });

    XUINT64 uPos = 0;
    IFC_RETURN(m_spMetadataStream->GetPosition(&uPos));
    IFCCHECK_RETURN(uPos == m_header.m_uAssemblyListOffset);
    offsetGuard.release();

    auto deserializationGuard = wil::scope_exit([this, &c_strReferenceTableDescription]()
    {
        IGNOREHR(LogDeserializationError(c_strReferenceTableDescription));
    });

    std::vector < PersistedXamlAssembly > vecAssemblyList;
    IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecAssemblyList, m_version, m_spMetadataStream));
    m_vecMasterAssemblyList.reserve(vecAssemblyList.size());
    deserializationGuard.release();

    auto readGuard = wil::scope_exit([this]()
    {
        IGNOREHR(LogError(AG_E_PARSER2_XBF_METADATA_ASSEMBLY_LIST, xstring_ptr::NullString(), xstring_ptr::NullString()));
    });

    for (const auto& persistedAssembly : vecAssemblyList)
    {
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlAssembly> spAssembly;
        XamlAssemblyToken assemblyToken;
        xstring_ptr spAssemblyName;

        IFC_RETURN(GetString(persistedAssembly.m_uiAssemblyName, spAssemblyName));

        IFC_RETURN(m_spXamlSchemaContext->GetTypeInfoProvider(persistedAssembly.m_eTypeInfoProviderKind, spTypeInfoProvider));
        IFC_RETURN(spTypeInfoProvider->ResolveAssembly(spAssemblyName, assemblyToken));
        IFC_RETURN(m_spXamlSchemaContext->GetXamlAssembly(assemblyToken, spAssemblyName, spAssembly));

        IFCCHECK_RETURN(spAssembly);
        m_vecMasterAssemblyList.push_back(spAssembly);
    }
    readGuard.release();

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadTypeNamespace(_In_ uint32_t index)
{
    xstring_ptr spNamespaceName;
    auto& persistedTypeNamespace = m_vecMasterTypeNamespaceList[index].first;

    auto readGuard = wil::scope_exit([this, &spNamespaceName]()
    {
        IGNOREHR(LogError(AG_E_PARSER2_XBF_METADATA_TYPE_NAMESPACE_LIST, spNamespaceName, xstring_ptr::NullString()));
    });

    std::shared_ptr<XamlTypeNamespace> spTypeNamespace;
    std::shared_ptr<XamlAssembly> spAssembly;

    IFC_RETURN(GetString(persistedTypeNamespace.m_uiNamespaceName, spNamespaceName));
    IFC_RETURN(GetAssembly(persistedTypeNamespace.m_uiAssembly, spAssembly));
    IFC_RETURN(spAssembly->GetTypeNamespace_ForBinaryXaml(spAssembly, spNamespaceName, spTypeNamespace));

    IFCCHECK_RETURN(spTypeNamespace);

    m_vecMasterTypeNamespaceList[index].second = spTypeNamespace;
    readGuard.release();

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadTypeNamespaceList()
{
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strTypeNamespaceTableDescription, L"Type Namespace Table");

    auto offsetGuard = wil::scope_exit([this, &c_strTypeNamespaceTableDescription]()
    {
        IGNOREHR(LogOffsetCheckError(c_strTypeNamespaceTableDescription));
    });

    XUINT64 uPos = 0;
    IFC_RETURN(m_spMetadataStream->GetPosition(&uPos));
    IFCCHECK_RETURN(uPos == m_header.m_uTypeNamespaceListOffset);
    offsetGuard.release();

    auto deserializationGuard = wil::scope_exit([this, &c_strTypeNamespaceTableDescription]()
    {
        IGNOREHR(LogDeserializationError(c_strTypeNamespaceTableDescription));
    });

    std::vector < PersistedXamlTypeNamespace > vecTypeNamespaceList;
    IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecTypeNamespaceList, m_version, m_spMetadataStream));
    m_vecMasterTypeNamespaceList.reserve(vecTypeNamespaceList.size());

    std::transform(
        vecTypeNamespaceList.begin(), vecTypeNamespaceList.end(), std::back_inserter(m_vecMasterTypeNamespaceList),
        [](PersistedXamlTypeNamespace &element)
        {
            return std::make_pair(element, std::shared_ptr<XamlTypeNamespace>());
        });

    deserializationGuard.release();

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadType(_In_ uint32_t index)
{
    xstring_ptr spTypeName;
    std::shared_ptr<XamlTypeNamespace> spTypeNamespace;
    std::shared_ptr<XamlType> spType;

    auto& xamlType = m_vecMasterTypeList[index].first;
    auto readGuard = wil::scope_exit([this, &spTypeNamespace, &spTypeName]()
    {
        xstring_ptr spTypeNamespaceName;

        if (spTypeNamespace)
        {
            spTypeNamespaceName = spTypeNamespace->get_TargetNamespace();
        }

        IGNOREHR(LogError(AG_E_PARSER2_XBF_TYPE_LIST, spTypeNamespaceName, spTypeName));
    });

    IFC_RETURN(GetString(xamlType.m_uiTypeName, spTypeName));
    if (xamlType.m_TypeFlags.IsBitSet(PersistedXamlType::IsMarkupDirective))
    {
        std::shared_ptr<XamlNamespace> spDirectiveNamespace;

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_directiveNamespace, L"http://schemas.microsoft.com/winfx/2006/xaml");
        IFC_RETURN(m_spXamlSchemaContext->GetXamlXmlNamespace(XSTRING_PTR_FROM_STORAGE(c_directiveNamespace), spDirectiveNamespace));
        IFC_RETURN(spDirectiveNamespace->GetDirectiveType(spTypeName, spType));
    }
    else if (xamlType.m_TypeFlags.IsBitSet(PersistedXamlType::IsUnknown))
    {
        // If an unknown type was persisted, then that means it was declared conditionally and couldn't be
        // resolved at XBF generation time. Try again to resolve it; if it's still unknown, we'll preserve
        // that information so that the ObjectWriter can emit an error if the conditional predicate
        // evaluates to true (i.e. this unknown type can't be skipped).

        // NOTE: It shouldn't be necessary to do the fallback to the managed type info provider
        // like in the usual case below, since this case is going through the same code path taken
        // by XamlScanner when resolving XamlTypes, so if it doesn't work, we're probably in trouble.
        std::shared_ptr<XamlNamespace> xamlXmlNamespace;
        IFC_RETURN(GetXmlNamespace(xamlType.m_uiTypeNamespace, xamlXmlNamespace));
        IFC_RETURN(xamlXmlNamespace->GetXamlType(spTypeName, spType));

        if (!spType)
        {
            IFC_RETURN(UnknownType::Create(m_spXamlSchemaContext, xamlXmlNamespace, spTypeName, spType));
        }
    }
    else
    {
        IFC_RETURN(GetTypeNamespace(xamlType.m_uiTypeNamespace, spTypeNamespace));
        IFC_RETURN(spTypeNamespace->GetXamlType(spTypeName, spType));

        //
        // If we failed to find the type, it might be in IXamlMetadataProvider which is not being
        // queried because we reset the Namespace Token if we lookup through the ManagedProvider
        // in XamlNativeTypeInfoProvider::ResolveTypeName()
        //
        // Try to resolve with the Managed Provider explicitly passing the Namespace Index.
        //

        // TODO: If PS WINBLUE:121474 is fixed, we can remove this code path.
        if (!spType &&
            spTypeNamespace->get_TypeNamespaceToken().GetProviderKind() == tpkNative)
        {
            std::shared_ptr<XamlTypeInfoProvider> spXamlTypeInfoProvider;
            XamlTypeNamespaceToken managedTypeNamespaceToken = spTypeNamespace->get_TypeNamespaceToken();
            XamlTypeToken retTypeToken;

            managedTypeNamespaceToken.SetProviderKind(tpkManaged);
            IFC_RETURN(m_spXamlSchemaContext->GetTypeInfoProvider(tpkManaged, spXamlTypeInfoProvider));
            IFC_RETURN(spXamlTypeInfoProvider->ResolveTypeName(managedTypeNamespaceToken, spTypeName, retTypeToken));
            IFC_RETURN(m_spXamlSchemaContext->GetXamlType(retTypeToken, spTypeNamespace, spTypeName, spType));
        }
    }

    IFCCHECK_RETURN(spType);
    m_vecMasterTypeList[index].second = std::move(spType);
    readGuard.release();

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadTypeList()
{
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strTypeTableDescription, L"Type Table");

    auto offsetGuard = wil::scope_exit([this, &c_strTypeTableDescription]()
    {
        IGNOREHR(LogOffsetCheckError(c_strTypeTableDescription));
    });

    XUINT64 uPos = 0;
    IFC_RETURN(m_spMetadataStream->GetPosition(&uPos));
    IFCCHECK_RETURN(uPos == m_header.m_uTypeListOffset);
    offsetGuard.release();

    auto deserializationGuard = wil::scope_exit([this, &c_strTypeTableDescription]()
    {
        IGNOREHR(LogDeserializationError(c_strTypeTableDescription));
    });

    std::vector< PersistedXamlType > vecMasterTypeList;
    IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecMasterTypeList, m_version, m_spMetadataStream));
    m_vecMasterTypeList.reserve(vecMasterTypeList.size());

    std::transform(
        vecMasterTypeList.begin(), vecMasterTypeList.end(), std::back_inserter(m_vecMasterTypeList),
        [](PersistedXamlType &element)
    {
        return std::make_pair(element, std::shared_ptr<XamlType>());
    });

    deserializationGuard.release();

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadProperty(_In_ uint32_t index)
{
    xstring_ptr spPropertyName;
    std::shared_ptr<XamlType> spType;
    std::shared_ptr<XamlProperty> spProperty;

    auto& xamlProperty = m_vecMasterPropertyList[index].first;
    auto readGuard = wil::scope_exit([this, &spType, &spPropertyName]()
    {
        xstring_ptr spTypeName;

        if (spType)
        {
            IGNOREHR(spType->get_FullName(&spTypeName));
        }

        IGNOREHR(LogError(AG_E_PARSER2_XBF_METADATA_PROPERTY_LIST, spTypeName, spPropertyName));
    });

    IFC_RETURN(GetString(xamlProperty.m_uiPropertyName, spPropertyName));
    if (xamlProperty.m_PropertyFlags.IsBitSet(PersistedXamlProperty::IsImplicitProperty))
    {
        if (spPropertyName.Equals(STR_LEN_PAIR(L"__implicit_items")))
        {
            // The implicit items property is generically applicable
            std::shared_ptr<ImplicitProperty> implicitProperty;
            IFC_RETURN(m_spXamlSchemaContext->get_ItemsProperty(spType /* unused */, implicitProperty));
            spProperty = implicitProperty;
        }
        else if (spPropertyName.Equals(STR_LEN_PAIR(L"__implicit_initialization")))
        {
            std::shared_ptr<ImplicitProperty> implicitProperty;
            IFC_RETURN(m_spXamlSchemaContext->get_InitializationProperty(implicitProperty));
            spProperty = implicitProperty;
        }
        else
        {
            // This is a bad/unrecoverable situation here with little actionable error info
            // unlike something like a misspelled property name that could be more informative
            // Failfast so we can get more actionable dumps that tell us how we arrived at this bizarre state
            IFCFAILFAST(E_FAIL);
        }

    }
    else if (xamlProperty.m_PropertyFlags.IsBitSet(PersistedXamlProperty::IsMarkupDirective))
    {
        // First look in Xaml markup namespace
        std::shared_ptr<XamlNamespace> spNamespace;
        std::shared_ptr<DirectiveProperty> directiveProperty;

        IFC_RETURN(m_spXamlSchemaContext->GetXamlXmlNamespace(c_strMarkupUri, spNamespace));
        IFC_RETURN(m_spXamlSchemaContext->GetXamlDirective(spNamespace, spPropertyName, directiveProperty));
        spProperty = directiveProperty;

        // if we can't find in xaml markup namespace, look in xml namespace
        if (!spProperty)
        {
            IFC_RETURN(m_spXamlSchemaContext->GetXamlXmlNamespace(c_strXmlUri, spNamespace));
            IFC_RETURN(m_spXamlSchemaContext->GetXamlDirective(spNamespace, spPropertyName, directiveProperty));
            spProperty = directiveProperty;
        }
    }
    else if (xamlProperty.m_PropertyFlags.IsBitSet(PersistedXamlProperty::IsCustomDependencyProperty))
    {
        IFC_RETURN(GetType(xamlProperty.m_uiType, spType));
        IFC_RETURN(spType->GetDependencyProperty(spPropertyName, spProperty));
    }
    else
    {
        IFC_RETURN(GetType(xamlProperty.m_uiType, spType));
        IFC_RETURN(spType->GetProperty(spPropertyName, spProperty));
    }

    if (!spProperty)
    {
        if (xamlProperty.m_PropertyFlags.IsBitSet(PersistedXamlProperty::IsUnknown))
        {
            std::shared_ptr<UnknownProperty> spUnknownProperty;
            IFC_RETURN(UnknownProperty::Create(m_spXamlSchemaContext, spPropertyName, spType, FALSE, spUnknownProperty));
            spProperty = spUnknownProperty;
        }
        else
        {
            // Being unable to resolve a property that was encoded in the XBF stream is a fatal failure.
            // Instead of propagating an error message, a FAILFAST is being introduced to be able to
            // capture more information when this occurs.
            IFCFAILFAST(E_FAIL);
        }
    }
    m_vecMasterPropertyList[index].second = spProperty;
    readGuard.release();

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadPropertyList()
{
    auto offsetGuard = wil::scope_exit([this]()
    {
        IGNOREHR(LogOffsetCheckError(xstring_ptr(c_strPropertyTableDescriptionStorage)));
    });

    XUINT64 uPos = 0;
    IFC_RETURN(m_spMetadataStream->GetPosition(&uPos));
    IFCCHECK_RETURN(uPos == m_header.m_uPropertyListOffset);
    offsetGuard.release();

    auto deserializationGuard = wil::scope_exit([this]()
    {
        IGNOREHR(LogDeserializationError(xstring_ptr(c_strPropertyTableDescriptionStorage)));
    });

    std::vector < PersistedXamlProperty > vecMasterPropertyList;
    IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecMasterPropertyList, m_version, m_spMetadataStream));
    m_vecMasterPropertyList.reserve(vecMasterPropertyList.size());

    std::transform(
        vecMasterPropertyList.begin(), vecMasterPropertyList.end(), std::back_inserter(m_vecMasterPropertyList),
        [](PersistedXamlProperty &element)
    {
        return std::make_pair(element, std::shared_ptr<XamlProperty>());
    });

    deserializationGuard.release();

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadXmlNamespace(_In_ uint32_t index)
{
    xstring_ptr spNamespaceUri;
    auto& xamlNamespace = m_vecMasterXmlNamespaceList[index].first;
    auto readGuard = wil::scope_exit([this, &spNamespaceUri]()
    {
        IGNOREHR(LogError(AG_E_PARSER2_XBF_METADATA_XML_NAMESPACE_LIST, spNamespaceUri, xstring_ptr::NullString()));
    });

    std::shared_ptr<XamlNamespace> spXamlNamespace;

    IFC_RETURN(GetString(xamlNamespace.m_uiNamespaceUri, spNamespaceUri));
    IFC_RETURN(m_spXamlSchemaContext->GetXamlXmlNamespace(spNamespaceUri, spXamlNamespace));

    if (!spXamlNamespace)
    {
        auto spUnknownNamespace = std::make_shared<XamlUnknownXmlNamespace>(m_spXamlSchemaContext, spNamespaceUri);
        spXamlNamespace = spUnknownNamespace;
    }

    IFCCHECK_RETURN(spXamlNamespace);
    m_vecMasterXmlNamespaceList[index].second = spXamlNamespace;

    readGuard.release();

    return S_OK;

}

_Check_return_ HRESULT
XamlBinaryMetadataReader2::LoadXmlNamespaceList()
{
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strXmlNamespaceTableDescription, L"Xml Namespace Table");

    auto offsetGuard = wil::scope_exit([this, &c_strXmlNamespaceTableDescription]()
    {
        IGNOREHR(LogOffsetCheckError(c_strXmlNamespaceTableDescription));
    });

    XUINT64 uPos = 0;
    IFC_RETURN(m_spMetadataStream->GetPosition(&uPos));
    IFCCHECK_RETURN(uPos == m_header.m_uXmlNamespaceListOffset);
    offsetGuard.release();

    auto deserializationGuard = wil::scope_exit([this, &c_strXmlNamespaceTableDescription]()
    {
        IGNOREHR(LogDeserializationError(c_strXmlNamespaceTableDescription));
    });

    std::vector<PersistedXamlXmlNamespace> vecXmlNamespaceList;
    IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecXmlNamespaceList, m_version, m_spMetadataStream));
    m_vecMasterXmlNamespaceList.reserve(vecXmlNamespaceList.size());

    std::transform(
        vecXmlNamespaceList.begin(), vecXmlNamespaceList.end(), std::back_inserter(m_vecMasterXmlNamespaceList),
        [](PersistedXamlXmlNamespace &element)
    {
        return std::make_pair(element, std::shared_ptr<XamlNamespace>());
    });

    deserializationGuard.release();

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::LogOffsetCheckError(_In_ const xstring_ptr& strTableDescription)
{
    IFC_RETURN(LogError(AG_E_PARSER2_XBF_METADATA_OFFSET, strTableDescription, xstring_ptr::NullString()));
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::LogDeserializationError(_In_ const xstring_ptr& strTableDescription)
{
    IFC_RETURN(LogError(AG_E_PARSER2_XBF_METADATA_DESERIALIZE, strTableDescription, xstring_ptr::NullString()));
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::LogError(
    _In_ uint32_t iErrorCode,
    _In_ const xstring_ptr& strParam1,
    _In_ const xstring_ptr& strParam2
    )
{
    // Log the failure to the error service
    std::shared_ptr<ParserErrorReporter> errorReporter;
    if (SUCCEEDED(m_spXamlSchemaContext->GetErrorService(errorReporter)))
    {
        // The error service can't handle NULL parameters
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strNullLiteral, L"null");

        IFC_RETURN(errorReporter->SetError(iErrorCode,
            0,
            0,
            strParam1.IsNull() ? c_strNullLiteral : strParam1,
            strParam2.IsNull() ? c_strNullLiteral : strParam2));
    }
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetString(_In_ uint32_t index, _Out_ xstring_ptr& result) const
{
    if (m_vecStringStorageList)
    {
        IFCCHECK_RETURN(index < m_vecStringStorageList->size());
        result = XSTRING_PTR_FROM_STORAGE(m_vecStringStorageList->at(index));
    }
    else
    {
        IFCCHECK_RETURN(index < m_vecStringList->size());
        result = m_vecStringList->at(index);
    }
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetAssembly(_In_ uint32_t index, _Out_ std::shared_ptr<XamlAssembly>& result) const
{
    IFCCHECK_RETURN(index < m_vecMasterAssemblyList.size());
    result = m_vecMasterAssemblyList[index];
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetTypeNamespace(_In_ uint32_t index, _Out_ std::shared_ptr<XamlTypeNamespace>& result)
{
    IFCCHECK_RETURN(index < m_vecMasterTypeNamespaceList.size());
    result = m_vecMasterTypeNamespaceList[index].second;
    if (!result)
    {
        IFC_RETURN(LoadTypeNamespace(index));
        result = m_vecMasterTypeNamespaceList[index].second;
    }
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetType(_In_ uint32_t index, _Out_ std::shared_ptr<XamlType>& result)
{
    IFCCHECK_RETURN(index < m_vecMasterTypeList.size());
    result = m_vecMasterTypeList[index].second;
    if (!result)
    {
        IFC_RETURN(LoadType(index));
        result = m_vecMasterTypeList[index].second;
    }
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetProperty(_In_ uint32_t index, _Out_ std::shared_ptr<XamlProperty>& result)
{
    IFCCHECK_RETURN(index < m_vecMasterPropertyList.size());
    result = m_vecMasterPropertyList[index].second;
    if (!result)
    {
        IFC_RETURN(LoadProperty(index));
        result = m_vecMasterPropertyList[index].second;
    }
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetXmlNamespace(_In_ uint32_t index, _Out_ std::shared_ptr<XamlNamespace>& result)
{
    IFCCHECK_RETURN(index < m_vecMasterXmlNamespaceList.size());
    result = m_vecMasterXmlNamespaceList[index].second;
    if (!result)
    {
        IFC_RETURN(LoadXmlNamespace(index));
        result = m_vecMasterXmlNamespaceList[index].second;
    }
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetString(_In_ const PersistedXamlNode2& node, _Out_ xstring_ptr& result) const
{
    // No support for trusted string indices yet
    ASSERT(node.m_bIsTrusted == FALSE);
    return GetString(node.m_uiObjectId, result);
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetAssembly(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlAssembly>& result) const
{
    // No support for trusted assembly indices yet
    ASSERT(node.m_bIsTrusted == FALSE);
    return GetAssembly(node.m_uiObjectId, result);
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetTypeNamespace(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlTypeNamespace>& result)
{
    // No support for trusted type namespace indices yet
    ASSERT(node.m_bIsTrusted == FALSE);
    return GetTypeNamespace(node.m_uiObjectId, result);
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetType(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlType>& result)
{
    if (node.m_bIsTrusted)
    {
        auto stableIndex = static_cast<StableXbfTypeIndex>(node.m_uiObjectId);
        IFC_RETURN(m_spXamlSchemaContext->GetXamlTypeFromStableXbfIndex(stableIndex, result));
    }
    else
    {
        IFC_RETURN(GetType(node.m_uiObjectId, result));
    }
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetProperty(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlProperty>& result)
{
    if (node.m_bIsTrusted)
    {
        auto stableIndex = static_cast<StableXbfPropertyIndex>(node.m_uiObjectId);
        IFC_RETURN(m_spXamlSchemaContext->GetXamlPropertyFromStableXbfIndex(stableIndex, result));
    }
    else
    {
        IFC_RETURN(GetProperty(node.m_uiObjectId, result));
    }
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader2::GetXmlNamespace(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlNamespace>& result)
{
    // No support for trusted xml namespace indices yet
    ASSERT(node.m_bIsTrusted == FALSE);
    return GetXmlNamespace(node.m_uiObjectId, result);
}

xstring_ptr XamlBinaryMetadataReader2::GetXbfHash() const
{
    // This work will be behind a flag in the future, and it will no-op.
    const size_t stringBufferSize = 2 * Parser::c_xbfHashSize + 1;
    std::array<WCHAR, stringBufferSize> temp;
    WCHAR* pTemp = temp.data();
    const WCHAR nibbleToHex[] = { L"0123456789ABCDEF" };
    for (auto val : m_header.m_byteHashBuffer) {
        *pTemp++ = nibbleToHex[val >> 4];   // Take the leftmost four bits and grab the hex value that corresponds to it.
        *pTemp++ = nibbleToHex[val & 0xF];  // Take the rightmost four bits and  grab the hax value.
    }
    ASSERT(pTemp - temp.data() == stringBufferSize - 1);
    *pTemp = 0; // Null terminate the string.

    xstring_ptr result;
    VERIFYHR(xstring_ptr::CloneBuffer(temp.data(), &result));

    return result;
}
