// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryMetadata.h"
#include "StableXbfIndexes.g.h"

class XamlBinaryMetadataReader final
{
private:
    _Check_return_ HRESULT LoadHeader();
    _Check_return_ HRESULT LoadStringTable();
    _Check_return_ HRESULT LoadAssemblyList();
    _Check_return_ HRESULT LoadTypeNamespaceList();
    _Check_return_ HRESULT LoadTypeList();
    _Check_return_ HRESULT LoadPropertyList();
    _Check_return_ HRESULT LoadXmlNamespaceList();
    _Check_return_ HRESULT LogError(
        _In_ XUINT32 iErrorCode,
        _In_ const xstring_ptr& strParam1,
        _In_ const xstring_ptr& strParam2
        );
    _Check_return_ HRESULT LogOffsetCheckError(_In_ const xstring_ptr& strTableDescription);
    _Check_return_ HRESULT LogDeserializationError(_In_ const xstring_ptr& strTableDescription);

public:
    XamlBinaryMetadataReader(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ IPALStream *pMetadataStream,
        _In_ const Parser::XamlBinaryFileVersion& version
        )
            : m_spXamlSchemaContext(spXamlSchemaContext)
            , m_spMetadataStream(pMetadataStream)
            , m_version(version)
    {
    }

    _Check_return_ HRESULT LoadMetadata();

    virtual ~XamlBinaryMetadataReader() { }

    _Check_return_ HRESULT GetString(XUINT32 index, _Out_ xstring_ptr& result) const;
    _Check_return_ HRESULT GetAssembly(XUINT32 index, _Out_ std::shared_ptr<XamlAssembly>& result) const;
    _Check_return_ HRESULT GetTypeNamespace(XUINT32 index, _Out_ std::shared_ptr<XamlTypeNamespace>& result) const;
    _Check_return_ HRESULT GetType(XUINT32 index, _Out_ std::shared_ptr<XamlType>& result) const;
    _Check_return_ HRESULT GetProperty(XUINT32 index, _Out_ std::shared_ptr<XamlProperty>& result) const;
    _Check_return_ HRESULT GetXmlNamespace(XUINT32 index, _Out_ std::shared_ptr<XamlNamespace>& result) const;

private:
    XamlBinaryFileHeader m_header;
    Parser::XamlBinaryFileVersion m_version;

    xvector< xstring_ptr > m_vecStringTable;
    xvector < std::shared_ptr<XamlAssembly> > m_vecMasterAssemblyList;
    xvector < std::shared_ptr<XamlTypeNamespace> > m_vecMasterTypeNamespaceList;
    xvector < std::shared_ptr<XamlType> > m_vecMasterTypeList;
    xvector < std::shared_ptr<XamlProperty> > m_vecMasterPropertyList;
    xvector < std::shared_ptr<XamlNamespace> > m_vecMasterXmlNamespaceList;

    xref_ptr<IPALStream> m_spMetadataStream;
    std::shared_ptr<XamlSchemaContext> m_spXamlSchemaContext;
};

