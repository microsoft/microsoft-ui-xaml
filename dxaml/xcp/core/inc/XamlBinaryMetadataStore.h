// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryMetadata.h"
#include "XamlTypeTokens.h"
#include "XbfVersioning.h"

class WinBluePropertyTypeCompatHelper;
struct IPALStream;
class XamlAssembly;
class XamlNamespace;
class XamlType;
class XamlProperty;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {
    class XamlBinaryMetadataStoreUnitTests;
} } } } }

class XamlBinaryMetadataStore final
{
    friend class Microsoft::UI::Xaml::Tests::Parser::XamlBinaryMetadataStoreUnitTests;

public:
    explicit XamlBinaryMetadataStore(
        _In_ const TargetOSVersion& version)
        : m_version(version)
    {
    }

    _Check_return_ static HRESULT Create(
        _In_ const TargetOSVersion& version,
        _Out_ std::shared_ptr<XamlBinaryMetadataStore>& spXamlBinaryMetadataStore)
    {
        spXamlBinaryMetadataStore = std::make_shared<XamlBinaryMetadataStore>(version);
        return S_OK;
    }

    const Parser::XamlBinaryFileVersion& GetVersion() const
    {
        return Parser::Versioning::GetXBFVersion(m_version);
    }

    const TargetOSVersion& GetTargetOSVersion() const
    {
        return m_version;
    }

    _Check_return_ HRESULT WriteMetadata(_In_ IPALStream *pStream, _In_ const std::array<byte, Parser::c_xbfHashSize>& byteHashForBinaryXaml);
    _Check_return_ HRESULT WriteMetadataBody(_In_ IPALStream *pStream);

    _Check_return_ HRESULT GetXamlAssemblyId(const std::shared_ptr<XamlAssembly>& inXamlType, _Out_ XUINT32& ruiXamlTypeId);
    _Check_return_ HRESULT GetXamlTypeNamespaceId(const std::shared_ptr<XamlNamespace>& inXamlNamespace, _Out_ XUINT32& ruiXamlNamespace);
    _Check_return_ HRESULT GetXamlTypeNode(const std::shared_ptr<XamlType>& inXamlType, _Out_ PersistedXamlNode& xamlNode);
    _Check_return_ HRESULT GetXamlTypeNode(const std::shared_ptr<XamlType>& inXamlType, _Out_ PersistedXamlNode2& xamlNode);
    _Check_return_ HRESULT GetXamlPropertyNode(const std::shared_ptr<XamlProperty>& inXamlProperty, _Out_ PersistedXamlNode2& xamlNode);
    _Check_return_ HRESULT GetXamlPropertyNode(const std::shared_ptr<XamlProperty>& inXamlProperty, _In_ const WinBluePropertyTypeCompatHelper& winBluePropertyTypeCompatHelper, _Out_ PersistedXamlNode& xamlNode);
    _Check_return_ HRESULT GetXamlXmlNamespaceId(const std::shared_ptr<XamlNamespace>& inXamXmlNamespace, _Out_ XUINT32& ruiXamlXmlNamespace);
    _Check_return_ HRESULT GetXamlTextValueId(_In_ const xstring_ptr& inString, _Out_ XUINT32& ruiStringId);
    _Check_return_ HRESULT GetStringId(_In_ const xstring_ptr& inString, _Out_ XUINT32& ruiStringId);

private:
    _Check_return_ HRESULT GetWinBlueTypeName(_In_ const XamlTypeToken& inTypeToken, _Out_ xstring_ptr* pstrRetVal);
    _Check_return_ HRESULT GetXamlTypeId(const std::shared_ptr<XamlType>& inXamlType, _Out_ XUINT32& ruiXamlTypeId);
    _Check_return_ HRESULT GetXamlPropertyId(const std::shared_ptr<XamlProperty>& inXamlProperty, _In_ const WinBluePropertyTypeCompatHelper& winBluePropertyTypeCompatHelper, _Out_ XUINT32& ruiXamlPropertyId);

    struct XstringPtrPairHash
    {
        std::size_t operator()(const std::pair<xstring_ptr, xstring_ptr> &x) const
        {
            return std::hash<xstring_ptr>()(x.first) ^ std::hash<xstring_ptr>()(x.second);
        }
    };

    XamlBinaryFileHeader m_header;
    XamlBinaryFileHeader2 m_header2;
    TargetOSVersion m_version;

    // Assuption:  Assembly > TypeNamespace > Type > Property
    xchainedmap< XamlAssemblyToken, XUINT32 > m_mapMasterAssemblyList;
    xvector < PersistedXamlAssembly > m_vecMasterAssemblyList;

    xchainedmap< XamlTypeNamespaceToken, XUINT32 > m_mapMasterTypeNamespaceList;
    xvector < PersistedXamlTypeNamespace > m_vecMasterTypeNamespaceList;

    std::unordered_map< XamlTypeToken, XUINT32 > m_mapMasterKnownTypeList;
    // Key is pair<namespace URI, type name>
    std::unordered_map< std::pair< xstring_ptr, xstring_ptr >, XUINT32, XstringPtrPairHash > m_mapMasterUnknownTypeList;
    xvector < PersistedXamlType > m_vecMasterTypeList;

    std::unordered_map< XamlPropertyToken, XUINT32 > m_mapMasterKnownPropertyList;
    // Key is pair<declaring type name, type name>
    std::unordered_map< std::pair< xstring_ptr, xstring_ptr >, XUINT32, XstringPtrPairHash > m_mapMasterUnknownPropertyList;
    xvector < PersistedXamlProperty > m_vecMasterPropertyList;

    xchainedmap< xstring_ptr, XUINT32 >  m_mapMasterXmlNamespaceList;
    xvector < PersistedXamlXmlNamespace > m_vecMasterXmlNamespaceList;

    xchainedmap< xstring_ptr, XUINT32 > m_mapStringTable;
    xvector< xstring_ptr > m_vecStringTable;
};

