// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryMetadataStore.h"
#include "ObjectWriterNode.h"
#include <palfileuri.h>

class ObjectWriterNodeList;
class CCoreServices;
struct IPALStream;

class XamlBinaryFormatSubWriter2 final
{
public:
    XamlBinaryFormatSubWriter2(
        _In_ CCoreServices *pCore,
        _In_ const std::shared_ptr<XamlBinaryMetadataStore>& spBinaryMetadataStore,
        _In_ const bool fGenerateLineInfo
        )
        : m_spMetadataStore(spBinaryMetadataStore)
        , m_pCore(pCore)
        , m_fGenerateLineInfo(fGenerateLineInfo)
        , m_lastNodeStreamOffset(0)
        , m_lastColumnOffset(0)
        , m_lastLineOffset(0)
    {
    }

    const Parser::XamlBinaryFileVersion& GetVersion() const
    {
        return m_spMetadataStore->GetVersion();
    }

    const TargetOSVersion& GetTargetOSVersion() const
    {
        return m_spMetadataStore->GetTargetOSVersion();
    }


    _Check_return_ HRESULT Initialize();

    _Check_return_ HRESULT PersistNode(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT PersistNodeType(
        _In_ const ObjectWriterNodeType nodeType);

    _Check_return_ HRESULT PersistConstantNodeType(
        _In_ const PersistedConstantType nodeType);

    _Check_return_ HRESULT PersistNamespace(
        _In_ const std::shared_ptr<XamlNamespace>& spNamespace,
        _In_ const xstring_ptr& spPrefix);

    _Check_return_ HRESULT PersistType(
        _In_ const std::shared_ptr<XamlType>& spType);

    _Check_return_ HRESULT PersistProperty(
        _In_ const std::shared_ptr<XamlProperty>& spType);

    _Check_return_ HRESULT PersistConstant(
        _In_ const CValue& value);

    _Check_return_ HRESULT PersistConstant(
        _In_ const unsigned int value);

    _Check_return_ HRESULT PersistLineInfo(
        _In_ const ObjectWriterNode& objectNode);

    _Check_return_ HRESULT Persist7BitEncodedInt(
        _In_ const unsigned int value,
        _In_ const xref_ptr<IPALStream>& spStream);

    _Check_return_ HRESULT PersistSharedString(
        _In_ const xstring_ptr strValue);

    xref_ptr<IPALStream> GetNodeStream() const;

    xref_ptr<IPALStream> GetLineStream() const;

    _Check_return_ HRESULT GetNodeStreamOffset(_Out_ XUINT64 *pOffset);

    ~XamlBinaryFormatSubWriter2();

private:
    _Check_return_ HRESULT PersistStringConstant(
        _In_ const xstring_ptr strValue);


private:
    std::shared_ptr<XamlBinaryMetadataStore> m_spMetadataStore;
    xref_ptr<IPALStream> m_spSubNodeStream;
    xref_ptr<IPALStream> m_spSubLineStream;
    unsigned int m_lastNodeStreamOffset;
    unsigned int m_lastLineOffset;
    unsigned int m_lastColumnOffset;
    bool m_fGenerateLineInfo;
    CCoreServices *m_pCore;
};

