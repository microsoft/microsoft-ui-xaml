// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryMetadata.h"
#include "StableXbfIndexes.g.h"
#include "palfileuri.h"
#include "xvector.h"
#include <ParserAPI.h>

class XamlSchemaContext;
class XamlTypeNamespace;
class XamlAssembly;
class XamlTypeNamespace;
class XamlType;
class XamlProperty;
class XamlNamespace;
struct IPALStream;

class XamlBinaryMetadataReader2 final
{
public:
    XamlBinaryMetadataReader2(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ IPALStream* pMetadataStream,
        _In_ IPALMemory* pMetadataMemory,
        _In_ Parser::XamlBufferType bufferType,
        _In_ const Parser::XamlBinaryFileVersion& version
        )
            : m_spXamlSchemaContext(spXamlSchemaContext)
            , m_spMetadataStream(pMetadataStream)
            , m_spMetadataMemory(pMetadataMemory)
            , m_bufferType(bufferType)
            , m_version(version)
    {
    }

    const Parser::XamlBinaryFileVersion& GetVersion() const
    {
        return m_version;
    }

private:
    _Check_return_ HRESULT LoadHeader();
    _Check_return_ HRESULT LoadAssemblyList();
    _Check_return_ HRESULT LoadStringTable();
    _Check_return_ HRESULT LoadTypeNamespace(_In_ uint32_t index);
    _Check_return_ HRESULT LoadTypeNamespaceList();
    _Check_return_ HRESULT LoadType(_In_ uint32_t index);
    _Check_return_ HRESULT LoadTypeList();
    _Check_return_ HRESULT LoadProperty(_In_ uint32_t index);
    _Check_return_ HRESULT LoadPropertyList();
    _Check_return_ HRESULT LoadXmlNamespace(_In_ uint32_t index);
    _Check_return_ HRESULT LoadXmlNamespaceList();

    _Check_return_ HRESULT LogError(
        _In_ const uint32_t iErrorCode,
        _In_ const xstring_ptr& strParam1,
        _In_ const xstring_ptr& strParam2
        );
    _Check_return_ HRESULT LogOffsetCheckError(_In_ const xstring_ptr& strTableDescription);
    _Check_return_ HRESULT LogDeserializationError(_In_ const xstring_ptr& strTableDescription);

public:
    _Check_return_ HRESULT LoadMetadata();

    _Check_return_ HRESULT GetString(_In_ const PersistedXamlNode2& node, _Out_ xstring_ptr& result) const;
    _Check_return_ HRESULT GetAssembly(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlAssembly>& result) const;
    _Check_return_ HRESULT GetTypeNamespace(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlTypeNamespace>& result);
    _Check_return_ HRESULT GetType(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlType>& result);
    _Check_return_ HRESULT GetProperty(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlProperty>& result);
    _Check_return_ HRESULT GetXmlNamespace(_In_ const PersistedXamlNode2& node, _Out_ std::shared_ptr<XamlNamespace>& result);

    unsigned int GetStringTableCount() const
    {
        if (m_vecStringStorageList)
            return (int)m_vecStringStorageList->size();
        else
            return (int)m_vecStringList->size();
    }

    unsigned int GetAssemblyTableCount() const  { return (int)m_vecMasterAssemblyList.size(); }
    unsigned int GetTypeNamespaceTableCount() const { return (int)m_vecMasterTypeNamespaceList.size(); }
    unsigned int GetTypeTableCount() const  { return (int)m_vecMasterTypeList.size(); }
    unsigned int GetPropertyTableCount() const  { return (int)m_vecMasterPropertyList.size(); }
    unsigned int GetXmlNamespaceTableCount() const  { return (int)m_vecMasterXmlNamespaceList.size(); }

    xstring_ptr GetXbfHash() const;

    const std::shared_ptr<std::vector<xstring_ptr_storage>>& GetStringStorage() const
    {
        return m_vecStringStorageList;
    }

private:
    XamlBinaryFileHeader2 m_header;
    Parser::XamlBinaryFileVersion m_version;

    // Indexed lookups should only happen for untrusted types
    _Check_return_ HRESULT GetString(_In_ const uint32_t index, _Out_ xstring_ptr& result) const;
    _Check_return_ HRESULT GetAssembly(_In_ const uint32_t index, _Out_ std::shared_ptr<XamlAssembly>& result) const;
    _Check_return_ HRESULT GetTypeNamespace(_In_ const uint32_t index, _Out_ std::shared_ptr<XamlTypeNamespace>& result);
    _Check_return_ HRESULT GetType(_In_ const uint32_t index, _Out_ std::shared_ptr<XamlType>& result);
    _Check_return_ HRESULT GetProperty(_In_ const uint32_t index, _Out_ std::shared_ptr<XamlProperty>& result);
    _Check_return_ HRESULT GetXmlNamespace(_In_ const uint32_t index, _Out_ std::shared_ptr<XamlNamespace>& result);

    // xstring_ptr_storage wrappers for memory mapped XBF string buffers.
    std::shared_ptr<std::vector<xstring_ptr_storage>> m_vecStringStorageList;

    // xstring_ptr instances for copies of non-mapped buffers.
    std::shared_ptr<std::vector<xstring_ptr>> m_vecStringList;

    std::vector < std::shared_ptr<XamlAssembly> > m_vecMasterAssemblyList;
    std::vector < std::pair < PersistedXamlTypeNamespace, std::shared_ptr<XamlTypeNamespace> >>  m_vecMasterTypeNamespaceList;
    std::vector < std::pair < PersistedXamlType, std::shared_ptr<XamlType> > > m_vecMasterTypeList;
    std::vector < std::pair < PersistedXamlProperty, std::shared_ptr<XamlProperty> > > m_vecMasterPropertyList;
    std::vector < std::pair < PersistedXamlXmlNamespace, std::shared_ptr<XamlNamespace> > > m_vecMasterXmlNamespaceList;

    xref_ptr<IPALStream> m_spMetadataStream;
    xref_ptr<IPALMemory> m_spMetadataMemory;
    Parser::XamlBufferType m_bufferType;

    std::shared_ptr<XamlSchemaContext> m_spXamlSchemaContext;
};

