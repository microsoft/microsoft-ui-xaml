// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ctypes.h>

// This function assumes the strings are 16 bit aligned.  This will fault
// on certain architectures if that is not the case.
XINT32
__cdecl
xstrncmp(
    const _In_reads_(cChar) WCHAR *pTrg,
    const _In_reads_(cChar) WCHAR *pSrc,
    _In_ XUINT32 cChar
)
{
    while (cChar && (*pTrg == *pSrc))
    {
        pTrg++;
        pSrc++;
        cChar--;
    }

    return cChar ? XINT32(*pTrg - *pSrc) : 0;
}

// Compares two strings ignoring case differences.  So for example the
// strings "Match" and "matCH" will, in fact, match.  It only works if the
// characters are in the range 0x0041 to 0x007a.
//
// Implementation details:
//      This function assumes the strings are 16 bit aligned.  This will fault
//  on certain architectures if that is not the case.
//
//-------------------------------------------------------------------------
XINT32
__cdecl
xstrncmpi(
    const _In_reads_(cChar) WCHAR *pTrg,
    const _In_reads_(cChar) WCHAR *pSrc,
    _In_ XUINT32 cChar
)
{
    while (cChar)
    {
        if (isalpha(*pTrg) && isalpha(*pSrc))
        {
            if ((*pTrg & 0x00df) != (*pSrc & 0x00df))
                break;
        }
        else
        {
            if (*pTrg != *pSrc)
                break;
        }

        pTrg++;
        pSrc++;
        cChar--;
    }

    return cChar ? XINT32(*pTrg - *pSrc) : 0;
}

// Will convert all the Ascii chars to upper case
void
__cdecl
    xstrntoUpper(
    _Inout_updates_z_(cChar) WCHAR *pInBuffer,
    _In_ XUINT32 cChar
)
{
    while (cChar)
    {
        if (isalpha(*pInBuffer))
        {
            *pInBuffer = (*pInBuffer & 0x00df);
        }

        pInBuffer++;
        cChar--;
    }

    return ;
}

// Will convert all the Ascii chars to lower case
void
__cdecl
    xstrntoLower(
    _Inout_updates_z_(cChar) WCHAR *pInBuffer,
    _In_ XUINT32 cChar
)
{
    while (cChar)
    {
        if (isalpha(*pInBuffer))
        {
            *pInBuffer = (*pInBuffer & 0x00df) + 0x0020;
        }

        pInBuffer++;
        cChar--;
    }

    return ;
}

// Copies the source string to the target string.
//
// Implementation details:
//  This function assumes the strings are 16 bit aligned.  This will fault
//  on certain architectures if that is not the case.
void *
__cdecl
xstrncpy(
    _Out_writes_(cChar) WCHAR *pTrg,
    _In_reads_(cChar) const WCHAR *pSrc,
    _In_ XUINT32 cChar
)
{
    void *pStart = pTrg;

    while (cChar--)
    {
       *pTrg++ = *pSrc++;
    }

    return pStart;
}

// case sensitive strstr bound by character count
_Check_return_ WCHAR *
__cdecl
xstrnstr(
    const _In_z_ WCHAR *pSrc,
    const _In_z_ WCHAR *pSub,
    _In_  XUINT32 cChar)
{
    const WCHAR *a, *b;
    
    for (;;)
    {
        XUINT32 cchRemaining = cChar;
        a = pSrc;
        b = pSub;
        do
        {
            if (!cchRemaining || !*b)
               return ((WCHAR *)pSrc);
            if (!*a)
               return (NULL);
            
            cchRemaining--;
            
        } while (*a++ == *b++);
        pSrc++;
    }
}

// case sensitive srtstr bound by character count and NULL termination
// This vesrion of xstrnstr is safer, it does size and NULL checks on
// all args.
_Check_return_ WCHAR* __cdecl xstrnstr_s(
    _In_reads_( f_cxchSrc )       WCHAR   f_rgxchSrc[ ],
    _In_                     const XUINT32 f_cxchSrc,
    _In_reads_( f_cxchSub )       WCHAR   f_rgxchSub[ ],
                             const XUINT32 f_cxchSub )
{
    const WCHAR *a, *b;
    XUINT32 cxchRemainingSrc = f_cxchSrc;
    
    for (;;)
    {
        XUINT32 cxchRemainingSub     = f_cxchSub;
        XUINT32 cxchRemainingSrcTemp = cxchRemainingSrc;

        a = f_rgxchSrc;
        b = f_rgxchSub;
        
        do
        {
            if ( 0 == cxchRemainingSub || 0 == *b )
               return ( f_rgxchSrc );
            if ( 0 == cxchRemainingSrcTemp || 0 == *a  )
               return ( NULL );
            
            cxchRemainingSub--;
            cxchRemainingSrcTemp--;
        } while (*a++ == *b++);

        f_rgxchSrc++;
        cxchRemainingSrc--;
    }
}


// case insensitive strstr bound by character count
// Copied from the xstrnstr function in this file, and changed the
// matching logic to be case-insensitive, following the pattern
// we're using elsewhere in this file.
_Check_return_ WCHAR *
__cdecl
xstrnistr(
    const _In_z_ WCHAR *pSrc,
    const _In_z_ WCHAR *pSub,
    _In_  XUINT32 cChar)
{
    const WCHAR *a, *b;

    for (;;)
    {
        XUINT32 cchRemaining = cChar;
        bool fMatch;
        a = pSrc;
        b = pSub;
        do
        {
            if (!cchRemaining || !*b)
               return ((WCHAR *)pSrc);
            if (!*a)
               return (NULL);
            
            cchRemaining--;

            if (isalpha(*a) && isalpha(*b))
            {
                fMatch = ((*a & 0x00df) == (*b & 0x00df));
            }
            else
            {
                fMatch = (*a == *b);
            }
            ++a;
            ++b;
            
        } while (fMatch);
        pSrc++;
    }
}

// convert an integer into a string
_Check_return_ WCHAR* 
__cdecl 
xstritoa(const _In_ XUINT32 value, _Out_ XUINT32* pcchString)
{
    const int MAX_CCH_XUINT32_BASE10 = 10; // 32-bit ints max out at 10 chars in base 10

    WCHAR buffer[MAX_CCH_XUINT32_BASE10 + 1];
    XINT32 i = MAX_CCH_XUINT32_BASE10 - 1;
    XUINT32 number = value;
    XUINT32 iFirstChar = 0;
    XUINT32 cchString = 0;
    WCHAR* pszString = NULL;

    buffer[MAX_CCH_XUINT32_BASE10] = L'\0';

    while (i >= 0)
    {
        XUINT32 digit = number % 10;
        buffer[i] = static_cast<WCHAR>(L'0' + digit);
        i--;

        number = number / 10;
        if (number == 0)
        {
            break;
        }
    }
    
    iFirstChar = i + 1;
    cchString = MAX_CCH_XUINT32_BASE10 - iFirstChar;
    
    pszString = new WCHAR[cchString + 1];
    if (pszString)
    {
        xstrncpy(pszString, &buffer[iFirstChar], cchString);
        pszString[cchString] = L'\0';

        if (pcchString)
        {
            *pcchString = cchString;
        }
    }

    return pszString;
}

//    convert a string into an integer
_Check_return_ HRESULT
__cdecl
xstratoi(_In_z_ WCHAR* pszInteger, _Out_ XINT32 *pValue)
{
    HRESULT hr = E_FAIL;
    XINT32 n = 0;
    bool fNeg = false;
    bool fHex = false;

    // Check to see if its a negative number
    if (*pszInteger == L'-')
    {
        fNeg = TRUE;
        ++pszInteger;
    }

    // Check to see if its a hex number
    if (pszInteger[0] == L'0' && pszInteger[1] == L'x') // leading '0x'
    {
        fHex = TRUE;
        pszInteger += 2;
    }
    else if (pszInteger[0] == L'x')     // leading 'x' same as leading '0x'
    {
        fHex = TRUE;
        ++pszInteger;
    }

    WCHAR* pszDigit = pszInteger;

    if (fHex)
    {
        for (; *pszInteger; pszInteger++)
        {
            if (*pszInteger - L'0' <= L'9' - L'0')
            {
                n = n * 0x10 + (*pszInteger - L'0');
            }
            //  x | 0x20 maps uppercase to lowercase if x is a letter
            else if ((*pszInteger | 0x20) - L'a' <= L'f' - L'a')
            {
                n = n * 0x10 + ((*pszInteger | 0x20) - L'a') + 10;
            }
            else
            {
                break;  // illegal hex digit
            }
        }
    }
    else
    {
        for (; *pszInteger; pszInteger++)
        {
            if (*pszInteger - L'0' <= L'9' - L'0')
            {
                n = n * 10 + (*pszInteger - L'0');
            }
            else
            {
                break;  // illegal decimal digit
            }
        }
    }

    //  We must have parsed at least one digit
    if (pszDigit != pszInteger)
    {
        *pValue = fNeg ? (-1 * n) : n;

        //  No trailing characters allowed
        if (*pszInteger == L'\0')
        {
            hr = S_OK;
        }

    }

    return hr;
}

// case sensitive srtstr.
_Check_return_ WCHAR *
__cdecl
xstrstr(
    const _In_z_ WCHAR *pSrc,
    const _In_z_ WCHAR *pSub)
{
    const WCHAR *a, *b;

    for (;;)
    {
        a = pSrc;
        b = pSub;
        do
        {
            if (!*b)
               return ((WCHAR *)pSrc);
            if (!*a)
               return (NULL);
        } while (*a++ == *b++);
        pSrc++;
    }
}

// Create a new string from the given string, starting at a given offset and with a given length.
// Checks to make sure that resulting string does not overrun source string at end.
WCHAR*
__cdecl
xsubstr(
    const __in_ecount_z(cchStringLength + 1) WCHAR* pszString,
    const _In_ XUINT32  cchStringLength,
    const _In_ XUINT32  uiOffset,
    const _In_ XUINT32  cchSubstring)

{
    WCHAR* pchSubString = NULL;

    if (uiOffset < cchStringLength)
    {
        XUINT32 theTrueLength = cchSubstring;

        if (((uiOffset + cchSubstring) < cchSubstring) || ((uiOffset + cchSubstring ) > cchStringLength ))
        {
            theTrueLength = cchStringLength - uiOffset;
        }

        // avoid overflow
        if (theTrueLength > ((XUINT32_MAX / sizeof(WCHAR) - 1))) XAML_FAIL_FAST();

        pchSubString = new WCHAR[theTrueLength + 1];

        if (pchSubString != NULL)
        {
            xstrncpy(pchSubString, pszString + uiOffset, theTrueLength);
            pchSubString[theTrueLength] = 0;
        }
    }

    return(pchSubString);
}

// Allocate a string from an existing string.
// The sourceString can be NULL, in which case just the space is allocated.
// sourceStringLen is increased by one to append a \0 character at the end.
// (This is not required by the definition of WCHAR strings, but can
//  be quite useful for other routines which expect a null at the end, or
//  otherwise don't know or use the length parameter (think system API calls...)
_Check_return_ __ecount_opt(sourceStringLen + 1) _Null_terminated_ WCHAR*
__cdecl
    xstralloc(
       const _In_reads_opt_(sourceStringLen) WCHAR*  sourceString,
       const _In_ XUINT32   sourceStringLen)

{
    WCHAR* theNewString = NULL;

    if (XUINT32_MAX / sizeof(WCHAR) - 1 < sourceStringLen)
        return NULL;

    theNewString = new WCHAR[sourceStringLen + 1];
    if (theNewString != NULL)
    {
        theNewString[sourceStringLen] = 0;

        if (sourceString != NULL)
            xstrncpy(theNewString, sourceString, sourceStringLen);
    }

    return(theNewString);
}

// Deallocate a string created with xstralloc() (or malloc() for that matter)
void
__cdecl
    xstrfree(
        _Inout_ WCHAR*      theString)

{
    delete [] theString;
}

// Find the offset to the first occurrence of a given character.
// Return [-1 | XUINT32(~0) | all bits on] if not found
XUINT32
__cdecl
xfindchar(
    const __in_ecount_z(theStringLen) WCHAR* theString,
    const _In_ XUINT32  theStringLen,
    const _In_ WCHAR    theChar)

{
    XUINT32     theOffset = 0;
    XINT32      found = FALSE;

    while ( (! found) && (theOffset < theStringLen) )
    {
        if (theString[theOffset] == theChar)
        {
            found = TRUE;
        }
        else
        {
            ++ theOffset;
        }
    }

    if(! found)
    {
        theOffset = XUINT32(~0);
    }

    return(theOffset);
}

// Find the offset to the last occurrence of a given character.
// Return [-1 | XUINT32(~0) | all bits on] if not found
XUINT32
__cdecl
xfindcharreverse(
    const __in_ecount_z(theStringLen) WCHAR* theString,
    const _In_ XUINT32  theStringLen,
    const _In_ WCHAR    theChar)

{
    XINT32 i;
    
    for (i = theStringLen - 1; i >= 0; --i)
    {
        if (theString[i] == theChar)
        {
            break;
        }
    }

    return i;
}

// Take two strings and create and return a single string with
// their combined contents.
void
__cdecl xstrconcat(
    _Outptr_result_buffer_maybenull_(*theCombinedStrLen+1) WCHAR**                        theCombinedStr,
    _Out_ XUINT32*                       theCombinedStrLen,
    const _In_reads_(string1Len) WCHAR* string1,
    const _In_ XUINT32                   string1Len,
    const _In_reads_(string2Len) WCHAR* string2,
    const _In_ XUINT32                   string2Len)

{
    // avoid overflow
    if (string2Len >= (XUINT32_MAX - string1Len)) XAML_FAIL_FAST();

    *theCombinedStrLen = string1Len + string2Len;
    *theCombinedStr = xstralloc(NULL, *theCombinedStrLen);
    if (*theCombinedStr != NULL)
    {
        xstrncpy(&(*theCombinedStr)[0], string1, string1Len);           // Move in the first
        xstrncpy(&(*theCombinedStr)[string1Len], string2, string2Len);  // Tack on the second
        (*theCombinedStr)[*theCombinedStrLen] = NULL;                   // Null terminate the result
    }
}

// The xstrnlen function returns the number of characters in the string pointed to by theString,
// not including the terminating '\0' character, but at most maxlen.
// It never looks beyond theString + maxLen.
HRESULT
__cdecl
xstrnlen(
    const __in_ecount_z_opt(maxLen) WCHAR* theString,
    const _In_ XUINT32               maxLen,
    _Out_range_(<=,maxLen) XUINT32*  len)

{
    HRESULT         hr = E_FAIL;
    XUINT32         theCount = 0;
    const WCHAR*    theChar = theString;

    // Make sure there's a valid len pointer
    if (!len)
       goto Cleanup;

    // Check for null string pointer
    if (theString == NULL)
    {
        *len = 0;
        goto Cleanup;
    }

    while ( (theCount < maxLen) && (*theChar != NULL) )
    {   ++theCount;
        ++theChar;
    }

    if (theCount < maxLen)
    {
        *len = theCount;
         hr = S_OK;
    }

Cleanup:
    return hr;
}

// Returns an array of substrings created by splitting the source string
// wherever there were separators.
//
// cSubstrings provides the size of the substring buffer, and will be
//  set on return to the number of substrings found.
//
//  WARNING: THIS OPERATAION IS DESTRUCTIVE. IT WRITES NULLS INTO THE
//  PROVIDED STRING
HRESULT
__cdecl
xstrsplit(
    _Inout_z_ WCHAR* pSource,
    WCHAR separator,
    _Inout_ XUINT32& cSubstrings,
    _Out_writes_(cSubstrings) WCHAR** aSubstrings)
{
    XUINT32 cSubstringCount = cSubstrings;
    cSubstrings = 0;

    WCHAR* pChar = pSource;

    // If we allow this we return a pointer that is 
    // past the end of the input buffer, setting the caller
    // up to read or write garbage memory.
    IFCEXPECT_RETURN(separator != NULL);

    while (cSubstrings < cSubstringCount)
    {
        aSubstrings[cSubstrings++] = pChar;
        if (!*pChar)
        {
            break;
        }

        while ( (*pChar != separator) && *pChar)
            ++pChar;

        if (*pChar == separator)
        {
            *pChar = 0;
            ++pChar;
        } 
        else if (!*pChar)
        {
            break;
        }
    }

    if (*pChar)
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

// strchr for WCHAR strings
WCHAR* xstrchr(_In_reads_(cchString) WCHAR* pszString, XUINT32 cchString, WCHAR ch)
{
    if (pszString == NULL)
        return NULL;

    WCHAR* pch = pszString;

    while (cchString && *pch)
    {
        if (*pch == ch)
            return pch;
        pch++;
        cchString--;
    }

    return NULL;
}
