// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FontAndScriptServices.h"
#include <FrameworkTheming.h>

//------------------------------------------------------------------------
//
//  Method:   LanguageToLCID
//
//  Synopsis: Convert a language string containing an xml:lang tab into a LCID
//
//------------------------------------------------------------------------

void
LanguageToLCID(
    _In_ XUINT32 cLanguageString,
    _In_reads_(cLanguageString) const WCHAR *pLanguageString,
    _Out_ XUINT32 *pLanguage
    )
{
    XUINT32 language = 0;

    if (cLanguageString > 0)
    {
        language = LocaleNameToLCID(pLanguageString, LOCALE_ALLOW_NEUTRAL_NAMES);
    }
    *pLanguage = language;

    // for now we never fail, language get set to 0 for language string we cannot parse
}


//------------------------------------------------------------------------
//
//  Method:   CStringToLCID
//
//  Synopsis: Convert a CString containing an xml:lang tag into a LCID
//
//------------------------------------------------------------------------

_Check_return_ XUINT32 CStringToLcid(_In_ CString *pString)
{
    XUINT32 result = 0;  // Default result is neutral

    if (pString != NULL)
    {
        result = XStringPtrToLCID(pString->m_strString);
    }

    return result;
}


//------------------------------------------------------------------------
//
//  Method:   XStringPtrToLCID
//
//  Synopsis: Convert an xstring_ptr containing an xml:lang tag into a LCID
//
//------------------------------------------------------------------------

_Check_return_ XUINT32 XStringPtrToLCID(_In_ const xstring_ptr& strString)
{
    XUINT32 result = 0;  // Default result is neutral

    if (!strString.IsNullOrEmpty())
    {
        LanguageToLCID(
            strString.GetCount(),
            strString.GetBuffer(),
            &result);
    }

    return result;
}




//------------------------------------------------------------------------
//
//  Method:   ValidateXmlLanguage
//
//  Synopsis:
//     validation of the XmlLanguage string:
//         The language string may be empty, or else must conform to RFC 3066 rules:
//         The first subtag must consist of only ASCII letters.
//         Additional subtags must consist of ASCII letters or numerals.
//         Subtags are separated by a single hyphen character.
//         Every subtag must be 1 to 8 characters long.
//         No leading or trailing hyphens are permitted.
//
//------------------------------------------------------------------------


static const XUINT32 maxCharsPerSubtag = 8;

bool ValidateXmlLanguageSubTag(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _In_ XUINT32 bPrimary
    )
{
    XUINT32 bValid = FALSE;
    XUINT32 charsRead = 0;

    if (cString > 0)
    {
        // Read the first character
        bValid = xisalpha(*pString);
        if (!bValid && !bPrimary)
            bValid = xisdigit(*pString);

        if (!bValid)
        {
            return false;
        }
    }
    else
    {
        // it's valid to have an empty string for the full tag but not for a sub-tag
        return !!bPrimary;
    }

    cString--;
    pString++;
    charsRead = 1;

    while (cString)
    {
        charsRead++;

        bValid = xisalpha(*pString);
        if (!bValid && !bPrimary)
            bValid = xisdigit(*pString);

        if (!bValid)
        {
            if ((*pString == '-') && (cString > 1))
            {
                cString--;
                pString++;

                // validate the sub-tag
                return ValidateXmlLanguageSubTag(cString, pString, /* bPrimary */ false);
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (charsRead > maxCharsPerSubtag)
            {
                return false;
            }

        }

        cString--;
        pString++;
    }

    return true;
}


bool ValidateXmlLanguage(_In_ const CValue *pValue)
{
    bool bValid = false;

    if (pValue->GetType() == valueObject)
    {
        CString* pLanguage = do_pointer_cast<CString>(pValue->AsObject());
        if (pLanguage && pLanguage->m_strString.GetBuffer())
        {
            bValid = ValidateXmlLanguageSubTag(pLanguage->m_strString.GetCount(), pLanguage->m_strString.GetBuffer(), /* bPrimary */ TRUE);
        }
    }
    else if (pValue->GetType() == valueString)
    {
        XUINT32 cString = 0;
        const WCHAR* pString = pValue->AsEncodedString().GetBufferAndCount(&cString);
        if (cString)
        {
            bValid = ValidateXmlLanguageSubTag(cString, pString, /* bPrimary */ TRUE);
        }
    }


    return bValid;
}


//------------------------------------------------------------------------
//
//  Description:
//     Determines whether a run is empty and should be removed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ShouldRemoveEmptyRun(
    _In_opt_ CRun *pRun,
    _Out_ bool& bShouldRemoveEmptyRun)
{
    bShouldRemoveEmptyRun = FALSE;

    if (pRun && !(pRun->GetFlags() & RunFlagsRunElement))
    {
        CValue   value;

        IFC_RETURN(pRun->GetValue(pRun->GetPropertyByIndexInline(KnownPropertyIndex::Run_Text), &value));
        IFCEXPECT_ASSERT_RETURN(value.GetType() == valueString);

        // If the run is now empty, and came from content rather than an
        // explicit Run element, remove it altogether.
        bShouldRemoveEmptyRun = value.AsString().IsNullOrEmpty();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Description:
//     Walks through the inlines, applying whitespace compression according to run
//     settings. Applies leading and trailing whitespace removal to all content runs
//     except around nested inlines.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CompressInlinesWhitespace(_Inout_opt_ CInlineCollection *pInlines)
{
    HRESULT            hr              = S_OK;
    XUINT32            cInlines        = 0;
    CInline           *pInline         = NULL;
    CRun              *pRun            = NULL;

    if (pInlines == NULL)
    {
        goto Cleanup;
    }

    // The SL 4 parser now handles text trimming - NOTE: until SL4 RI of parser,
    // the CompressInlinesWhitespace in jolt_dev is the almost the SAME as
    // OLD_CompressInlinesWhitespace (with shared ShouldRemoveEmptyRun logic
    // pulled out)

    // Remove any empty runs
    cInlines = pInlines->GetCount();
    for (XUINT32 i=0; i<cInlines; i++)
    {
        bool bShouldRemoveEmptyRun = false;

        IFC(DoPointerCast(pInline, static_cast<CDependencyObject*>(pInlines->GetItemWithAddRef(i))));
        pRun = do_pointer_cast<CRun>(pInline);

        IFC(ShouldRemoveEmptyRun(pRun, bShouldRemoveEmptyRun));
        if (bShouldRemoveEmptyRun)
        {
            IFCEXPECT(pInlines->RemoveAt(i) == pInline);
            pInline->Release();
            IFCEXPECT(pInline->GetRefCount() >= 1); // No longer owned by m_pInline, this code still has a ref
            if (i > 0)
            {
                --i;
            }
            cInlines--;
        }

        ReleaseInterface(pInline);
    }

Cleanup:
    ReleaseInterface(pInline);
    RRETURN(hr);
}

_Check_return_ XINT32 IsXamlWhitespace(XUINT32 character)
{
    return character == UNICODE_SPACE
        || character == UNICODE_CHARACTER_TABULATION
        || character == UNICODE_FORM_FEED
        || character == UNICODE_CARRIAGE_RETURN;
}

_Check_return_ XINT32 IsXamlNewline(XUINT32 character)
{
    // This test is used in a number of tight character loops, so great
    // performance is vital.
    // Since most characters are not a form of newline, the test is optimized to
    // fail as quickly as possible.

    return  ((character & 0xffffdf50) == 0)    // Very fast rejection of most common character codes
        &&  (    ((character-0x0000000a) < 4)  // LF, VT, FF, CR
             ||  ((character-0x00002028) < 2)  // LS, PS
             ||  (character == 0x00000085));   // NL
}

_Check_return_ XINT32 IsXamlBlockSeparator(XUINT32 character)
{
    return character == UNICODE_LINE_SEPARATOR
        || character == UNICODE_PARAGRAPH_SEPARATOR; 
}

bool UseHighContrastSelection(
    _In_ CCoreServices *pCore
    )
{
    return pCore->GetFrameworkTheming()->HasHighContrastTheme();
}

bool IsForegroundPropertyIndex(KnownPropertyIndex propertyIndex)
{
    if (propertyIndex == KnownPropertyIndex::TextElement_Foreground ||
        propertyIndex == KnownPropertyIndex::TextBlock_Foreground ||
        propertyIndex == KnownPropertyIndex::RichTextBlock_Foreground ||
        propertyIndex == KnownPropertyIndex::Control_Foreground ||
        propertyIndex == KnownPropertyIndex::ContentPresenter_Foreground)
    {
        return true;
    }
    return false;
}
