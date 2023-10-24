// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryFormatWriter.h"
#include "XamlBinaryMetadataStore.h"
#include "WinBluePropertyTypeCompatHelper.h"

// Xaml Binary Format Writer v1
// TODO: Note that this is not deriving from XamlWriter, as renames need to happen in that case for the Persist* to implement the abstract methods expected for a writer.
class XamlBinaryFormatWriter final  // : public XamlWriter
{
public:
    XamlBinaryFormatWriter(
        _In_ CCoreServices *pCore,
        _In_ const std::shared_ptr<XamlBinaryMetadataStore>& spBinaryMetadataStore,
        _In_ IPALStream *pMetadataStream,
        _In_ IPALStream *pNodeStreamStream,
        _In_ bool fGenerateLineInfo
        )
        : m_spMetadataStore(spBinaryMetadataStore)
        , m_spMetadataStream(pMetadataStream)
        , m_spNodeStreamStream(pNodeStreamStream)
        , m_pCore(pCore)
        , m_fGenerateLineInfo(fGenerateLineInfo)
    {
    }

    virtual ~XamlBinaryFormatWriter();

    const Parser::XamlBinaryFileVersion& GetVersion() const
    {
        return m_spMetadataStore->GetVersion();
    }

    _Check_return_ HRESULT WriteAllNodes(
        _In_ const std::shared_ptr<XamlReader>& spReader);

private:
    _Check_return_ HRESULT SkipEventProperty(_In_ const std::shared_ptr<XamlReader>& spReader);
    _Check_return_ HRESULT SaveNodeType(XamlNodeType nodeType);
    _Check_return_ HRESULT SaveValueNodeType(PersistedXamlValueNode::PersistedXamlValueNodeType nodeValueType);
    _Check_return_ HRESULT PersistNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistLineInfoIfNeeded(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistAddNamespaceNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistStartObjectNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistEndObjectNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistStartMemberNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistEndMemberNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistEndOfAttributesNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistEndOfStreamNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistTextValueNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT PersistValueNode(const XamlNode& xamlNode);
    _Check_return_ HRESULT TryOptimizePropertyValue(std::shared_ptr<XamlProperty>& spXamlProperty, _In_ const xstring_ptr& spTextString, bool *pfPropertyOptimized);

private:
    std::shared_ptr<XamlBinaryMetadataStore> m_spMetadataStore;
    std::shared_ptr<XamlProperty> m_spLastXamlProperty;
    WinBluePropertyTypeCompatHelper m_winbluePropertyTypeCompatHelper;

    xref_ptr<IPALStream> m_spMetadataStream;
    xref_ptr<IPALStream> m_spNodeStreamStream;

    bool m_fGenerateLineInfo;
    XamlLineInfo m_CurrentLineInfo;
    CCoreServices *m_pCore;
};
