// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"


//-------------------------------------------------------------------------
//
//  Function:   xstralloc
//
//  Synopsis:   
//              Allocate a string from an existing string.
//              The sourceString can be NULL, in which case just the space is allocated.
//              sourceStringLen is increased by one to append a \0 character at the end.
//                  (This is not required by the definition of WCHAR strings, but can
//                  be quite useful for other routines which expect a null at the end, or
//                  otherwise don't know or use the length parameter (think system API calls...)
// 
//-------------------------------------------------------------------------
_Check_return_ __ecount_opt(sourceStringLen + 1) WCHAR* CommonPlatformUtilities::Xstralloc(
       const _In_reads_opt_(sourceStringLen) WCHAR*  sourceString,
       const _In_ XUINT32   sourceStringLen)

{
    WCHAR* theNewString = NULL;

    if (XUINT32_MAX / sizeof(WCHAR) - 1 < sourceStringLen)
        return NULL;
    
#pragma prefast( suppress: 26451 "PREfast does not know we already checked the size of sourceStringLen above. Abstain from adding casting to complicate the code only to make PREfast happy.")
    theNewString = new WCHAR[sourceStringLen + 1];
    if (theNewString != NULL)
    {
        theNewString[sourceStringLen] = 0;

        if (sourceString != NULL)
            Xstrncpy(theNewString, sourceString, sourceStringLen);

    }
    return(theNewString);
}

//-------------------------------------------------------------------------
//
//  Function:   Xstrfree
//
//  Synopsis:   
//              Deallocate a string created with xstralloc() (or malloc() for that matter)
// 
//-------------------------------------------------------------------------
void CommonPlatformUtilities::Xstrfree(
        _In_z_ WCHAR*      theString)

{
    delete [] theString;
}

//-------------------------------------------------------------------------
//
//  Function:   xstrncpy
//
//  Synopsis:
//      Copies the source string to the target string.
// 
//  Implementation details:
//      This function assumes the strings are 16 bit aligned.  This will fault
//  on certain architectures if that is not the case.
//
//-------------------------------------------------------------------------

void *
    CommonPlatformUtilities::Xstrncpy(
        _Out_writes_(cChar) WCHAR *pTrg,
        _In_reads_(cChar) const WCHAR *pSrc,
        _In_ XUINT32 cChar
    )
{
    void *pStart = pTrg;
// we are not copying after null
    while (cChar)  /* copy string */
    {
        WCHAR ch = *pSrc++;
        *pTrg++ = ch;
        if (ch == NULL)
        {
            break;
        }
        
        cChar--;
    }

    return pStart;
}

//-------------------------------------------------------------------------
//
//  Function:   Xstrlen
//
//  Synopsis:   
//              
// 
//-------------------------------------------------------------------------
XUINT32 
    CommonPlatformUtilities::Xstrlen(
        _Null_terminated_ const WCHAR *pString )
{
    // sanity check
    if (pString == NULL)
        return(0);

    const WCHAR *pEos = pString;

    while (*pEos++);

    return XUINT32(pEos - pString - 1);
}

//-------------------------------------------------------------------------
//
//  Function:   Xstrncmpi
//
//  Synopsis:
//      Compares two strings ignoring case differences. 
//
//-------------------------------------------------------------------------

XINT32
    CommonPlatformUtilities::Xstrncmpi(
        const _In_reads_(cChar) WCHAR *pTrg,
        const _In_reads_(cChar) WCHAR *pSrc,
        _In_ XUINT32 cChar)
{
    while (cChar)
    {
        // This is a crude check to make sure these are at least ASCII comparable
        //  Better would be an implementation of XisAlpha()
        if ( (*pTrg < 128) && (*pSrc < 128) )
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

//-------------------------------------------------------------------------
//
//  Function:   Xstrncmp
//
//  Synopsis:
//      Compares two strings observing case differences. 
//
//-------------------------------------------------------------------------

XINT32
    CommonPlatformUtilities::Xstrncmp(
        const _In_reads_(cChar) WCHAR *pTrg,
        const _In_reads_(cChar) WCHAR *pSrc,
        _In_ XUINT32 cChar)
{
    while (cChar)
    {
        if (*pTrg != *pSrc)
            break;

        pTrg++;
        pSrc++;
        cChar--;
    }

    return cChar ? XINT32(*pTrg - *pSrc) : 0;
}
