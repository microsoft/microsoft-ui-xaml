// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

bool XamlNameParser::ParsePrefix(_Out_ xephemeral_string_ptr* pstrOutPrefix)
{
    XUINT32 uStartIndex = m_uIndex;

    if (ParseXmlNameFirstChar()
        && ParseRestOfPrefix()
        && ParseChar(L':'))
    {
        // The -1 is the not include the ':'. To be in this block
        // at least 2 characters must have been consumed.
        ASSERT(m_uIndex >= uStartIndex + 1);
        XSTRING_PTR_EPHEMERAL2(m_pszInputNoRef, m_cInput).SubString(uStartIndex, m_uIndex - 1, pstrOutPrefix);
        return true;
    }
    else
    {
        // TODO: Member hr?
        pstrOutPrefix->Reset();
        m_uIndex = uStartIndex;
        return false;
    }
}

bool XamlNameParser::ParseName(_Out_ xephemeral_string_ptr* pstrOutName)
{
    XUINT32 uStartIndex = m_uIndex;

    if (ParseXmlNameFirstChar()
        && ParseRestOfName())
    {
        XSTRING_PTR_EPHEMERAL2(m_pszInputNoRef, m_cInput).SubString(uStartIndex, m_uIndex, pstrOutName);
        return true;
    }
    else
    {
        // TODO: Should we have a m_hr to store failure results?
        pstrOutName->Reset();
        m_uIndex = uStartIndex;
        return false;
    }
}




// Parser..............
bool XamlNameParser::ParseSingleDottedName(_Out_ xephemeral_string_ptr* pstrOutssBeforeDot, _Out_ xephemeral_string_ptr* pstrOutssAfterDot)
{
    XUINT32 startIndex = m_uIndex;
    return Seq(startIndex, ParseName(pstrOutssBeforeDot) 
                && ParseChar(L'.')
                && ParseName(pstrOutssAfterDot) 
                && ParseEmpty());
}

bool XamlNameParser::ParseUndottedName(_Out_ xephemeral_string_ptr* pstrOutssBeforeDot, _Out_ xephemeral_string_ptr* pstrOutssAfterDot)
{
    if (ParseUndottedName(pstrOutssAfterDot))
    {
        // On success initalizes before dot to empty string
        xstring_ptr::EmptyString().Demote(pstrOutssBeforeDot);
        return true;
    }

    pstrOutssBeforeDot->Reset();
    pstrOutssAfterDot->Reset();

    return false;
}

bool XamlNameParser::ParseUndottedName(_Out_ xephemeral_string_ptr* pstrOutssName)
{
    XUINT32 startIndex = m_uIndex;
    return Seq(startIndex, ParseName(pstrOutssName) && ParseEmpty());
}
