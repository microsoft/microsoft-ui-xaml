// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlName
{
protected:
    XamlName()
    {
    }

public:
    XamlName(_In_ const xstring_ptr& inName)
        : m_ssName(inName)
        , m_ssPrefix(xstring_ptr::EmptyString())
    {
        // TODO: should verify name is not null?
    }

    XamlName(_In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName)
        : m_ssPrefix(inPrefix)
        , m_ssName(inName)
    {
    }

    virtual ~XamlName()
    {
    }

    const xstring_ptr& get_Name()
    {
        return m_ssName;
    }

    const xstring_ptr& get_Prefix()
    {
        return m_ssPrefix;
    }

    virtual HRESULT get_ScopedName(_Out_ xstring_ptr* pstrOutPrefix) = 0;

    // ZEN::: Some protected methods were not added. Will add them as required from
    // the derived classes.

protected:
    xstring_ptr m_ssPrefix;
    xstring_ptr m_ssName;
};

class XamlNameParser
{
public:
    XamlNameParser(_In_ const xstring_ptr_view& input)
        : m_uIndex(0)
    {
        ASSERT(!input.IsNullOrEmpty());

        m_pszInputNoRef = input.GetBufferAndCount(&m_cInput);
    }

protected:
    typedef XINT32 (*fParsePredicate)(WCHAR);

    bool ParsePrefix(_Out_ xephemeral_string_ptr* pstrOutPrefix);
    bool ParseName(_Out_ xephemeral_string_ptr* pstrOutName);
    bool ParseSingleDottedName(_Out_ xephemeral_string_ptr* pstrOutssBeforeDot, _Out_ xephemeral_string_ptr* pstrOutssAfterDot);
    bool ParseUndottedName(_Out_ xephemeral_string_ptr* pstrOutssBeforeDot, _Out_ xephemeral_string_ptr* pstrOutssAfterDot);
    bool ParseUndottedName(_Out_ xephemeral_string_ptr* pstrOutssName);
    bool ParsePredicate(fParsePredicate fnPredicate)
    {
        if ((m_uIndex < m_cInput) && fnPredicate(m_pszInputNoRef[m_uIndex]))
        {
            m_uIndex++;
            return true;
        }

        return false;
    }

    bool ParseChar(WCHAR ch)
    {
        if ((m_uIndex < m_cInput) && (m_pszInputNoRef[m_uIndex] == ch))
        {
            m_uIndex++;
            return true;
        }

        return false;
    }

    bool ParseXmlNameFirstChar()
    {
        return ParsePredicate(&XamlNameParser::IsValidXmlNameFirstChar);
    }

    bool ParseXmlNameChar()
    {
        return ParsePredicate(&XamlNameParser::IsValidXmlNameChar);
    }


    bool Seq(XUINT32 uStartIndex, bool bWasSuccess)
    {
        // Be careful passing m_uIndex to uStartIndex, because the
        // order of evaluation isn't defined in C++, it won't do
        // what you expect it does in C#.
        if (!bWasSuccess)
        {
            m_uIndex = uStartIndex;
        }

        return bWasSuccess;
    }

    bool ParseRestOfName()
    {
        while (ParsePredicate(&XamlNameParser::IsValidXmlNameChar));
        return true;
    }

    bool ParseRestOfPrefix()
    {
        while (ParsePredicate(&XamlNameParser::IsXmlPrefixChar));
        return true;
    }
    
    static bool IsColon(WCHAR ch)
    {
        return ch == L':';
    }

    bool ParseEmpty()
    {
        return m_cInput == m_uIndex;
    }

    static XINT32 IsXmlPrefixChar(WCHAR ch)
    {
        return !IsColon(ch);
    }

    
    static XINT32 IsValidXmlNameFirstChar(WCHAR ch)
    {
        return (ch != L'.') && !IsColon(ch);
    }


    static XINT32 IsValidXmlNameChar(WCHAR ch)
    {
        return (ch != L'.') && !IsColon(ch);
    }

protected:
    XUINT32 m_uIndex;

private:
    // We want to avoid storing a copy of the input string -- but need to be careful
    // not to leave a dangling reference to the input string's buffer stored below.
    void *operator new(size_t);

    // In order to reduce the amount of memory allocations during parsing of XAML
    // names, we allow the XAML name parser to operate on non-reference counted
    // buffers -- appropriate care needs to be taken to ensure that the input
    // buffer does not outlive this name parser instance.
    XUINT32 m_cInput;
    const WCHAR* m_pszInputNoRef;
};
