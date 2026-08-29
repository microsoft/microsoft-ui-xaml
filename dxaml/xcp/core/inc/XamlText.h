// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XStringBuilder.h>
#include "IXamlSchemaObject.h"

// A representation of text that has been read by the XAML parser and
// correctly trims whitespace, escapes markup extensions, etc.
class XamlText
    final : public IXamlSchemaObject
{
public:
    XamlText(bool bPreserveSpace = false)
        : m_bPreserveSpace(bPreserveSpace)
        , m_bIsAllWhiteSpace(TRUE)
    {
    }

    // Note: this isn't on the original class, but since we use it as
    // a value type, we can reset the class, and reuse the internal buffer.
    HRESULT Reset(bool bPreserveSpace = false);
    HRESULT Initialize(XUINT32 uInitialSize);
    HRESULT Initialize(_In_reads_(cBuffer) const WCHAR* pBuffer, XUINT32 cBuffer);
    HRESULT Initialize(_In_ const xstring_ptr& rstrString);

    HRESULT Paste(_In_ const xstring_ptr& inText, bool bTrimLeadingWhitespace);
    ////HRESULT Paste(_In_ WCHAR* pBuffer, XUINT32 cBuffer, bool bTrimLeadingWhitespace);

    HRESULT get_IsEmpty(bool& bIsEmpty);
    _Check_return_ HRESULT get_Text(_Out_ xstring_ptr* pstrText);
    _Check_return_ HRESULT set_Text(_In_ const xstring_ptr& spText);
    HRESULT get_IsSpacePreserved(bool& bIsSpacePreserved);
    HRESULT get_IsWhiteSpaceOnly(bool& bIsOnlyWhitespace);
    HRESULT get_LooksLikeAMarkupExtension(bool& bLooksLikeMarkupExtension);

    HRESULT UnescapeMEEscapingIfRequired();

    // Remove the leading and trailing whitespace from the text.
    //
    // (Note that we've merged the WPF methods TrimLeadingWhitespace and
    // TrimTrailingWhitespace into a single method controlled by boolean
    // parameters because it allows us to have a cleaner implementation of
    // XamlPullParser::Logic_ApplyFinalTextTrimming).
    static _Check_return_ HRESULT TrimWhitespace(
        _In_ const xstring_ptr& spText,   // The text to trim
        _In_ bool bTrimStart,                      // Whether to trim the start
        _In_ bool bTrimEnd,                        // Whether to trim the end
        _Out_ xstring_ptr* pstrTrimmed);    // The trimmed text

    // Gets whether the entire string is comprised of whitespace.
    static bool IsWhitespace(
        _In_ const xstring_ptr& spText);

private:
    bool PrivateIsEmpty() { return m_Builder.GetCount() == 0; }
    bool PrivateLooksLikeMarkupExtension();
    bool PrivateIsMarkupEscaping();
    HRESULT AppendToBuffer(_In_ const xstring_ptr& inText);
    HRESULT AppendToBuffer(const WCHAR ch);

    // Gets whether the character is whitespace.
    static bool IsWhitespaceChar(WCHAR ch);

    // Remove all leading and trailing whitespace, and collapse any internal
    // runs of whitespace into a single space.
    static _Check_return_ HRESULT CollapseWhitespace(
        _In_ const xstring_ptr& spText,
        _Out_ xstring_ptr* pstrCollapsed);

    // TODO: Don't think we need these
    ////static HRESULT HasSurroundingEastAsianChars(XINT32 start, XINT32 end, xstring_ptr& strText, _Out_ bool* pOut);
    ////static HRESULT ComputeUnicodeScalarValue(XINT32 takeOneIdx, XINT32 takeTwoIdx, xstring_ptr& strText, _Out_ XINT32* pOut);
    ////static HRESULT IsEastAsianCodePoint(XINT32 unicodeScalarValue, _Out_ bool* pOut);

private:
    bool m_bPreserveSpace;
    bool m_bIsAllWhiteSpace;
    XSmallStringBuilder m_Builder;
};

