// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A representation of text that has been read by the XAML parser and
//      correctly trims whitespace, escapes markup extensions, etc.

#include "precomp.h"

HRESULT XamlText::Reset(bool bPreserveSpace)
{
    m_bIsAllWhiteSpace = TRUE;
    m_bPreserveSpace = bPreserveSpace;

    return m_Builder.Reset();
}

HRESULT XamlText::Initialize(XUINT32 uInitialSize)
{
    return m_Builder.Initialize(uInitialSize);
}

HRESULT XamlText::Initialize(_In_reads_(cBuffer) const WCHAR* pBuffer, XUINT32 cBuffer)
{
    return m_Builder.Initialize(pBuffer, cBuffer);
}

HRESULT XamlText::Initialize(_In_ const xstring_ptr& rstrString)
{
    return m_Builder.Initialize(rstrString);
}

HRESULT XamlText::Paste(_In_ const xstring_ptr& inText, bool bTrimLeadingWhitespace)
{
    bool bNewTextIsAllWhitespace = IsWhitespace(inText);

    if (m_bPreserveSpace)
    {
        IFC_RETURN(AppendToBuffer(inText));
    }
    else if (bNewTextIsAllWhitespace)
    {
        if (PrivateIsEmpty() && !bTrimLeadingWhitespace)
        {
            IFC_RETURN(AppendToBuffer(L' '));
        }
    }
    else
    {
        bool bNewTextHasLeadingWhitespace = false;
        bool bNewTextHasTrailingWhitespace = false;
        bool bExistingTextHasTrailingWhitespace = false;
        xstring_ptr ssTrimmed;

        ASSERT(inText.GetCount() > 0);
        bNewTextHasLeadingWhitespace = IsWhitespaceChar(inText.GetBuffer()[0]);
        bNewTextHasTrailingWhitespace = IsWhitespaceChar(inText.GetChar(inText.GetCount() - 1));
        IFC_RETURN(CollapseWhitespace(inText, &ssTrimmed));

        if (!PrivateIsEmpty())
        {
            if (m_bIsAllWhiteSpace)
            {
                IFC_RETURN(m_Builder.Reset());
            }
            else
            {
                if (IsWhitespaceChar(m_Builder.GetChar(m_Builder.GetCount() - 1)))
                {
                    bExistingTextHasTrailingWhitespace = TRUE;
                }
            }
        }

        if (bNewTextHasLeadingWhitespace && !bTrimLeadingWhitespace && !bExistingTextHasTrailingWhitespace)
        {
            IFC_RETURN(AppendToBuffer(L' '));
        }

        ASSERT(!ssTrimmed.IsNullOrEmpty());
        IFC_RETURN(AppendToBuffer(ssTrimmed));

        if (bNewTextHasTrailingWhitespace)
        {
            IFC_RETURN(AppendToBuffer(L' '));
        }
    }
    
    m_bIsAllWhiteSpace = m_bIsAllWhiteSpace && bNewTextIsAllWhitespace;
    
    return S_OK;
}

HRESULT XamlText::get_IsEmpty(bool& bIsEmpty)
{
    bIsEmpty = PrivateIsEmpty();
    return S_OK;
}

_Check_return_ HRESULT XamlText::get_Text(_Out_ xstring_ptr* pstrText)
{
    //
    // Note that the get_Text method can be called multiple times,
    // so we cannot just detach the string from the builder. Instead,
    // we clone it.
    //
    IFC_RETURN(xstring_ptr::CloneBuffer(
        m_Builder.GetBuffer(),
        m_Builder.GetCount(),
        pstrText));

    return S_OK;
}

// Explicitly set the text.  This will bypass the regular whitespace trimming
// and collapsing done by the Paste method.
_Check_return_ HRESULT XamlText::set_Text(_In_ const xstring_ptr& spText)
{
    m_bIsAllWhiteSpace = IsWhitespace(spText);
    IFC_RETURN(m_Builder.Reset());
    IFC_RETURN(AppendToBuffer(spText));
    
    return S_OK;
}

HRESULT XamlText::UnescapeMEEscapingIfRequired()
{
    if (PrivateIsMarkupEscaping())
    {
        IFC_RETURN(m_Builder.ShiftLeft(2));
    }

    return S_OK;
}

HRESULT XamlText::get_IsSpacePreserved(bool& bIsSpacePreserved)
{
    bIsSpacePreserved = m_bPreserveSpace;
    RRETURN(S_OK);
}

HRESULT XamlText::get_IsWhiteSpaceOnly(bool& bIsWhiteSpaceOnly)
{
    bIsWhiteSpaceOnly = m_bIsAllWhiteSpace;
    RRETURN(S_OK);
}

HRESULT XamlText::AppendToBuffer(_In_ const xstring_ptr& inText)
{
    return m_Builder.Append(inText);
}

HRESULT XamlText::AppendToBuffer(const WCHAR ch)
{
    return m_Builder.AppendChar(ch);
}

bool XamlText::PrivateLooksLikeMarkupExtension()
{
    return ((m_Builder.GetCount() > 0) && (m_Builder.GetBuffer()[0] == L'{'))
        && ((m_Builder.GetCount() == 1) || (m_Builder.GetBuffer()[1] != L'}'));
}

bool XamlText::PrivateIsMarkupEscaping()
{
    return MeScanner::HasMarkupExtensionEscaping(m_Builder.GetCount(), m_Builder.GetBuffer());
}

HRESULT XamlText::get_LooksLikeAMarkupExtension(bool& bLooksLikeMarkupExtension)
{
    bLooksLikeMarkupExtension = PrivateLooksLikeMarkupExtension();
    RRETURN(S_OK);
}

// Gets whether the character is whitespace.
bool XamlText::IsWhitespaceChar(WCHAR ch)
{
    // TODO: This could be faster using the xiswhite function.
    // TODO: Dupe in ME
    return (ch == L' ') || (ch == L'\n') || (ch == L'\r') || (ch == L'\t');
}

// Gets whether the entire string is comprised of whitespace.
bool XamlText::IsWhitespace(
    _In_ const xstring_ptr& spText)
{
    XUINT32 count;
    const WCHAR* buffer = spText.GetBufferAndCount(&count);
    
    for (XUINT32 i = 0; i < count; i++)
    {
        if (!IsWhitespaceChar(buffer[i]))
        {
            return false;
        }
    }

    return true;
}

// Remove all leading and trailing whitespace, and collapse any internal runs of
// whitespace into a single space.
_Check_return_ HRESULT XamlText::CollapseWhitespace(
    _In_ const xstring_ptr& spText,
    _Out_ xstring_ptr* pstrCollapsed)
{
    XUINT32 uFirstIndex = 0;
    XUINT32 uAdvancingIndex = 0;
    XStringBuilder builder;
    
    XUINT32 count;
    const WCHAR* buffer = spText.GetBufferAndCount(&count);

    IFC_RETURN(builder.Initialize(count));
    
    while (uFirstIndex < count)
    {
        // If it's not whitespace, copy it to the destination
        WCHAR ch = buffer[uFirstIndex];
        if (!IsWhitespaceChar(ch))
        {
            IFC_RETURN(builder.AppendChar(ch));
            ++uFirstIndex;
            continue;
        }
        
        // Skip any runs of whitespace
        uAdvancingIndex = uFirstIndex;
        while (++uAdvancingIndex < count)
        {
            if (!IsWhitespaceChar(buffer[uAdvancingIndex]))
            {
                break;
            }
        }
        
        // If the run of skipped spaces was in the middle of the text, replace
        // the entire run with a single space (otherwise we effectively trimmed
        // it off the end)
        if (uFirstIndex && uAdvancingIndex != count)
        {
            // TODO: Compat: System.XAML uses HasSurroundingEastAsianChars to determine whether to skip adding this space
            IFC_RETURN(builder.AppendChar(L' '));
        }
        uFirstIndex = uAdvancingIndex;
    }
    
    IFC_RETURN(builder.DetachString(pstrCollapsed));
    
    return S_OK;
}

// Remove the leading and trailing whitespace from the text.
// 
// (Note that we've merged the WPF methods TrimLeadingWhitespace and
// TrimTrailingWhitespace into a single method controlled by boolean parameters
// because it allows us to have a cleaner implementation of
// XamlPullParser::Logic_ApplyFinalTextTrimming).
_Check_return_ HRESULT XamlText::TrimWhitespace(
    _In_ const xstring_ptr& spText,   // The text to trim
    _In_ bool bTrimStart,                      // Whether to trim the start
    _In_ bool bTrimEnd,                        // Whether to trim the end
    _Out_ xstring_ptr* pstrTrimmed)     // The trimmed text
{
    XINT32 ichStart = 0;
    XINT32 ichEnd = 0;
    XINT32 cch = 0;
    
    XUINT32 count;
    const WCHAR* buffer = spText.GetBufferAndCount(&count);

    IFCEXPECT_RETURN(count < XINT32_MAX);
    cch = static_cast<XINT32>(count);
    ichEnd = cch - 1;

    // Determine how much whitespace to remove from the start of the text
    if (bTrimStart)
    {
        for (; ichStart < cch; ichStart++)
        {
           WCHAR ch = buffer[ichStart];
           if (!IsWhitespaceChar(ch))
           {
               break;
           }
        }
    }

    // Determine how much whitespace to remove from the end of the text
    if (bTrimEnd)
    {
        for (; ichEnd >= ichStart; ichEnd--)
        {
           WCHAR ch = buffer[ichEnd];
           if (!IsWhitespaceChar(ch))
           {
               break;
           }
        }
    }

    // Create the trimmed string
    if (ichStart == 0 && ichEnd == cch - 1)
    {
       *pstrTrimmed = spText;
    }
    else if (ichEnd < ichStart)
    {
       *pstrTrimmed = xstring_ptr::EmptyString();
    }
    else
    {
       IFC_RETURN(spText.SubString(ichStart, ichEnd + 1, pstrTrimmed));
    }
    
    return S_OK;
}

