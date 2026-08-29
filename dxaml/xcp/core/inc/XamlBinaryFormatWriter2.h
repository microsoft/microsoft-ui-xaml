// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryMetadataStore.h"
#include "ObjectWriterNode.h"
#include "XbfVersioning.h"

class ObjectWriterNodeList;
class XamlBinaryFormatSubWriter2;

class XamlBinaryFormatWriter2 final
{
public:
    XamlBinaryFormatWriter2(
        _In_ CCoreServices *pCore,
        _In_ const std::shared_ptr<XamlBinaryMetadataStore>& spBinaryMetadataStore,
        _In_ IPALStream *pNodeStreamStream,
        _In_ const bool fGenerateLineInfo
        )
        : m_spMetadataStore(spBinaryMetadataStore)
        , m_spMasterNodeStream(pNodeStreamStream)
        , m_pCore(pCore)
        , m_fGenerateLineInfo(fGenerateLineInfo)
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

    _Check_return_ HRESULT WriteAllNodes(
        _In_ const std::shared_ptr<ObjectWriterNodeList>& spObjectNodeList);

private:
    _Check_return_ HRESULT PersistReferencedResourceList(
        _In_ const std::vector<std::pair<bool, xstring_ptr>>& listOfResources,
        _In_ const std::shared_ptr<XamlBinaryFormatSubWriter2>& spSubWriter);

    _Check_return_ HRESULT ProcessSubNodes(
        _In_ const std::shared_ptr<ObjectWriterNodeList>& spObjectNodeList,
        _Out_ std::vector<unsigned int>& streamOffsetList);

    _Check_return_ HRESULT WriteToMasterStream(
        _In_ xref_ptr<IPALStream> spMasterStream,
        _In_ xref_ptr<IPALStream> spStream);

    _Check_return_ HRESULT GetMasterStreamOffset(
        _In_ xref_ptr<IPALStream> spMasterStream,
        _Out_ XUINT32 *pOffset);

private:
    std::shared_ptr<XamlBinaryMetadataStore> m_spMetadataStore;
    xref_ptr<IPALStream> m_spMasterNodeStream;
    xvector< xref_ptr<IPALStream> > m_spSubNodeStreamList;
    xvector< xref_ptr<IPALStream> > m_spSubLineStreamList;
    xvector< PersistedXamlSubStream > m_vecMasterSubStreamList;
    CCoreServices *m_pCore;
    bool m_fGenerateLineInfo;
};

