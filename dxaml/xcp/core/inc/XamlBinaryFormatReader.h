// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef _XAMLBINARYFORMATREADER_H_
#define _XAMLBINARYFORMATREADER_H_
//------------------------------------------------------------------------
//
//  Abstract:
//
//      A XAML reader implementation for reading binary node streams.
//
//------------------------------------------------------------------------

#include "XamlBinaryMetadata.h"
#include "XamlBinaryMetadataReader.h"

class XamlBinaryFormatReader final : public XamlReader
{
public:
    XamlBinaryFormatReader(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const std::shared_ptr<XamlBinaryMetadataReader>& spXamlBinaryMetadataReader,
        _In_ IPALStream *pNodeStreamStream
        )
        : m_spXamlSchemaContext(spXamlSchemaContext)
        , m_spXamlBinaryMetadataReader(spXamlBinaryMetadataReader)
        , m_spNodeStreamStream(pNodeStreamStream)
        , m_pbNodeStream(NULL)
        , m_cbNodeStream(0)
        , m_cbNodeStreamRead(0)
    {
    }

    _Check_return_ HRESULT Initialize(
        _In_ IPALStream *pNodeStreamStream
        );

    _Check_return_ static HRESULT Create(
        _In_ std::shared_ptr<XamlSchemaContext> spXamlSchemaContext,
        _In_ std::shared_ptr<XamlBinaryMetadataReader> spXamlBinaryMetadataReader,
        _In_ IPALStream *pNodeStreamStream,
        _Out_ std::shared_ptr<XamlBinaryFormatReader>& spXamlBinaryFormatReader
        );

    ~XamlBinaryFormatReader() override;
    _Check_return_ HRESULT Read() override;
    const XamlNode& CurrentNode() override;
    HRESULT set_NextIndex(XUINT32 uiIndex) override   { ASSERT(FALSE); RRETURN(E_FAIL); }
    HRESULT get_NextIndex(XUINT32 *puiIndex) override { ASSERT(FALSE); RRETURN(E_FAIL); }

    _Check_return_ HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext) override
    {
        outSchemaContext = m_spXamlSchemaContext;
        RRETURN(S_OK);
    }

private:

    _Check_return_ HRESULT ReadNodeType(_Out_ XamlNodeType& nodeType);
    _Check_return_ HRESULT ReadValueNodeType(_Out_ PersistedXamlValueNode::PersistedXamlValueNodeType& nodeValueType);
    _Check_return_ HRESULT ReadLineInfoDelta();
    _Check_return_ HRESULT ReadLineInfoAbsolute();
    _Check_return_ HRESULT ReadAddNamespaceNode();
    _Check_return_ HRESULT ReadStartObjectNode();
    _Check_return_ HRESULT ReadEndObjectNode();
    _Check_return_ HRESULT ReadStartMemberNode();
    _Check_return_ HRESULT ReadEndMemberNode();
    _Check_return_ HRESULT ReadEndOfAttributesNode();
    _Check_return_ HRESULT ReadEndOfStreamNode();
    _Check_return_ HRESULT ReadTextValueNode();
    _Check_return_ HRESULT ReadValueNode();
    _Check_return_ HRESULT ReadFromNodeStreamBuffer(_Out_writes_bytes_(cbRead) XUINT8* pbTarget, _In_ XUINT32 cbRead)
    {
        if (m_cbNodeStreamRead + cbRead > m_cbNodeStream)
        {
            RRETURN(E_FAIL);
        }

        // these are going to be small reads, so memcpy would be overkill.
        const XUINT8 *pbRead = m_pbNodeStream + m_cbNodeStreamRead;
        XUINT32 cbRemaining = cbRead;
        while (cbRemaining--)
        {
            *pbTarget++ = *pbRead++;
        }

        m_cbNodeStreamRead += cbRead;
        return S_OK;
    }
    _Check_return_ HRESULT ReadPersistedXamlNode(PersistedXamlNode& targetNode)
    {
        RRETURN(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&targetNode), sizeof(PersistedXamlNode)));
    }
    _Check_return_ HRESULT ReadPersistedString(_Out_ xstring_ptr* pstrTargetString);

private:
    std::shared_ptr<XamlPullParser> m_spXamlPullParser;
    XamlNode m_currentXamlNode;
    XamlLineInfo m_currentLineInfo;
    XamlTypeToken m_stringXamlTypeToken;

    std::shared_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    std::shared_ptr<XamlBinaryMetadataReader> m_spXamlBinaryMetadataReader;

    xref_ptr<IPALStream> m_spNodeStreamStream;

    XUINT8 *m_pbNodeStream;
    XUINT32 m_cbNodeStream;
    XUINT32 m_cbNodeStreamRead;
};

#endif

