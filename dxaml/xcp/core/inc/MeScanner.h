// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlParserContext;
class XamlLineInfo;
class XamlProperty;
class XamlNamespace;
class XamlType;

class MeScanner 
{
public:

    enum eMeTokenKind
    {
        mtkNone,
        // NB: Code depends on these symbols having their ascii values.
        mtkOpen         = L'{',
        mtkClose        = L'}',
        mtkEqualSign    = L'=',
        mtkComma        = L',',
        mtkTypeName,      // String - Follows a '{' space delimited
        mtkPropertyName,  // String - Preceeds a '='.  {},= delimited, can (but shouldn't) contain spaces.
        mtkString         // String - all other strings, {},= delimited can contain spaces.
    };

    MeScanner(
                const std::shared_ptr<XamlParserContext>& inContext, 
                _In_ const xstring_ptr& inText, 
                const XamlLineInfo& lineInfo);
    XUINT32 LineNumber();
    XUINT32 LinePosition();

    eMeTokenKind TokenKind();
    HRESULT get_TokenType(std::shared_ptr<XamlType>& outXamlType);
    HRESULT get_TokenProperty(std::shared_ptr<XamlProperty>& outXamlProperty);
    ////HRESULT get_TokenPrefix(_Out_ xstring_ptr* pstrOutPrefix);
    HRESULT get_TokenText(_Out_ xstring_ptr* pstrOutText);
    bool IsAtEndOfInput()
    {
        return (m_mtkTokenKind == MeScanner::mtkNone) && InternalIsAtEndOfInput();
    }
    bool HasTrailingWhitespace() { return m_bHasTrailingWhitespace; }
    HRESULT Read();

    static bool HasMarkupExtensionEscaping(_In_ XUINT32 cBuffer, _In_reads_(cBuffer) const WCHAR* pBuffer)
    {
        return (cBuffer >= 2) && (pBuffer[0] == L'{') && (pBuffer[1] == L'}');
    }

private:
    enum eTokenStringState
    {
        tssNone,
        tssValue,
        tssType,
        tssProperty
    };

    XUINT32 m_uIndex;
    eMeTokenKind m_mtkTokenKind;
    eTokenStringState m_TokenStringState; 
    bool m_bHasTrailingWhitespace;
    XamlLineInfo m_LineInfo;

    // TODO: Should this be a weak ptr?
    std::shared_ptr<XamlParserContext> m_Context;
    xstring_ptr m_ssInputText;
    std::shared_ptr<XamlType> m_TokenXamlType;
    std::shared_ptr<XamlNamespace> m_XamlNamespace;
    std::shared_ptr<XamlProperty> m_TokenProperty;
    xstring_ptr m_ssTokenText;

    // TODO: used?
    ////string _tokenNamespace;

private:
    bool InternalIsAtEndOfInput();
    HRESULT ResolveTypeName(_In_ const xstring_ptr_view& inTypeName);
    HRESULT ResolvePropertyName(_In_ const xstring_ptr_view& inPropertyName);
    HRESULT ReadString(_Inout_ XStringBuilder& sb);
    WCHAR CurrentChar();
    bool Advance();
    static bool IsWhitespaceChar(const WCHAR& ch);
    static HRESULT RemoveEscapesInPlace(_Inout_ XStringBuilder& stringBuilder);
    void AdvanceOverWhitespace();
    void PushBack();

    HRESULT GetSchemaContext(_Out_ std::shared_ptr<XamlSchemaContext>& rspOut)
    {
        RRETURN(m_Context->get_SchemaContext(rspOut));
    }
    // Error handling code.
    HRESULT GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService);
    HRESULT ReportError(XUINT32 errorCode, const XamlLineInfo& inLineInfo);
    HRESULT ReportError(XUINT32 errorCode, const XamlLineInfo& inLineInfo, _In_ const xstring_ptr& inssParam1);
    HRESULT ReportError(XUINT32 errorCode, const XamlLineInfo& inLineInfo, _In_ const xstring_ptr& inssParam1, _In_ const xstring_ptr& inssParam2);

    // TODO: Not needed??
    ////HRESULT CreateErrorXamlType(XamlName* name, _In_ const xstring_ptr& strXmlns, _Out_ XamlType** ppOut);
};

