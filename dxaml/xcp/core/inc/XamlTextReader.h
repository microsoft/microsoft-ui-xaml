// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlReader.h"

class XamlSchemaContext;
class XamlTextReaderSettings;
class XamlPullParser;

// TextReader allows a consumer of XamlNodes treat the XamlPullParser
// as a XamlReader.
class XamlTextReader final
    : public XamlReader
{
public:
    XamlTextReader(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const std::shared_ptr<XamlPullParser> spXamlPullParser
        )
            : m_spXamlSchemaContext(spXamlSchemaContext)
            , m_spXamlPullParser(spXamlPullParser)
    {
    }

    _Check_return_ static HRESULT Create(
        _In_ std::shared_ptr<XamlSchemaContext> spXamlSchemaContext,
        _In_ const XamlTextReaderSettings& textReaderSettings,
        _In_ XUINT32 cSource,
        _In_reads_bytes_(cSource) const XUINT8 *pSource,
        _Out_ std::shared_ptr<XamlTextReader>& spXamlTextReader
        );

    _Check_return_ static HRESULT StripLeadingWhitespaceForXmlLiteParser(
        _In_ XUINT32 cSource,
        _In_reads_(cSource) const XUINT8* pSource,
        _Out_ XUINT32* pcResult,
        _Outptr_result_buffer_(*pcResult) const XUINT8** ppResult
        );

    ~XamlTextReader() override;
    _Check_return_ HRESULT Read() override;
    const XamlNode& CurrentNode() override;
    _Check_return_ HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext) override;
    HRESULT set_NextIndex(XUINT32 uiIndex) override   { UNREFERENCED_PARAMETER(uiIndex); ASSERT(FALSE); RRETURN(E_FAIL); }
    HRESULT get_NextIndex(XUINT32 *puiIndex) override { UNREFERENCED_PARAMETER(puiIndex); ASSERT(FALSE); RRETURN(E_FAIL); }

private:
    std::shared_ptr<XamlPullParser> m_spXamlPullParser;
    XamlNode m_currentXamlNode;
    std::weak_ptr<XamlSchemaContext> m_spXamlSchemaContext;
};


