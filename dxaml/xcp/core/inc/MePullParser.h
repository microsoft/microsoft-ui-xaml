// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MeScanner.h"

class XamlParserContext;
class XamlNode;
class XamlSchemaContext;
class ParserErrorReporter;
class XamlLineInfo;

class IPullParserNodeCallback
{
public:
    // This is provided so that the ME parser can put nodes
    // directly onto the main parser queue.
    virtual HRESULT AcceptNode(const XamlNode& node) = 0;
};

class MePullParser
{
private:
    // TODO: These are temporary.
    static const XUINT32 NO_PARSER_ERROR = 0;
    static const XUINT32 DEFAULT_ERROR = 1;
    std::shared_ptr<XamlParserContext> m_ParserContext;
    xstring_ptr m_OriginalText;
    IPullParserNodeCallback* m_pEnqueueNodeDelegate;
    MeScanner m_Scanner;
    XamlNode m_WorkingNode;

public:

    // TODO: Maybe want re-usable with Init instead
    MePullParser(
                _In_ IPullParserNodeCallback* pNodeCallback,
                _In_ const std::shared_ptr<XamlParserContext>& inParserContext,
                _In_ const xstring_ptr& inOriginalText,
                _In_ const XamlLineInfo& lineInfo);

    HRESULT Parse();

private:
    HRESULT CheckToken(const MeScanner::eMeTokenKind& inTokenKind);
    HRESULT ExpectToken(const MeScanner::eMeTokenKind& inTokenKind);
    HRESULT ExpectTypeName();
    HRESULT ExpectPropertyName();
    HRESULT ExpectValue();

    HRESULT P_MarkupExtension();
    HRESULT P_Arguments();

    HRESULT Logic_StartObject();
    HRESULT Logic_EndObject();
    HRESULT Logic_StartMember();
    HRESULT Logic_EndMember();
    HRESULT Logic_Text();
    HRESULT Logic_StartPositionalParameters();
    HRESULT Logic_EndPositionalParameters();
    HRESULT GetPropertyForSilverlightPositionalParameter(std::shared_ptr<XamlProperty>& outXamlProperty);

    HRESULT GetSchemaContext(_Out_ std::shared_ptr<XamlSchemaContext>& rspOut)
    {
        RRETURN(m_ParserContext->get_SchemaContext(rspOut));
    }
    HRESULT GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService);
    HRESULT ReportError(XUINT32 errorCode);
    HRESULT ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1);
    HRESULT ReportError(XUINT32 errorCode, _In_ const xstring_ptr& inssParam1, _In_ const xstring_ptr& inssParam2);
    HRESULT NextToken();
    XUINT32 LineNumber();
    XUINT32 LinePosition();
    HRESULT EnqueueWorkingNode();
    void SetLineInfoOnWorkingNode();
};


