// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.hpp"
#include "XStringUtils.h"

// strings treated as character arrays
#define HEX_DIGIT_CHARACTERS L"0123456789ABCDEF"

// ContainsCharactersToEncode
//  Return true if there string contains characters that are outside of the [33,126] range.
//  Note that this will not check for reserved or discouraged characters in URIs such as #"<> etc.
bool ContainsCharactersToEncode(_In_ XUINT32 cOriginal, _In_reads_(cOriginal) WCHAR* pszOriginal)
{
    bool bNeedsEncoding = false;

    for (XUINT32 i = 0; i < cOriginal; i++)
    {
        if (pszOriginal[i] > 126 || pszOriginal[i] < 33 /* CTL or outside ASCII range */)
        {
            bNeedsEncoding = true;
            break;
        }
    }

    return bNeedsEncoding;
}

// PercentEncodeString
//  Encodes characters that are outside of the [33,126] range as a UTF8 percent-encoded octet.
//  Note that this will not check for reserved or discouraged characters in URIs such as #"<> etc.
HRESULT PercentEncodeString(_In_ XUINT32 cOriginal, _In_reads_(cOriginal) WCHAR* pszOriginal, _Out_ XUINT32* pcEncoded, _Outptr_result_z_ WCHAR** ppszEncoded)
{
    HRESULT hr = S_OK;
    
    char* pszUtf8Copy = NULL;
    XUINT32 cUtf8Copy = 0;

    WCHAR* pszEncoded = NULL;
    XUINT32 cEncoded = 0;

    IFCPTR(pszOriginal);
    IFCPTR(pcEncoded);
    IFCPTR(ppszEncoded);

    pszUtf8Copy = WideCharToChar(cOriginal, pszOriginal);

    cUtf8Copy = 0;
    while(pszUtf8Copy[cUtf8Copy] != '\0')
    {
        cUtf8Copy++;
    }

    pszEncoded = new WCHAR[cUtf8Copy * 3 + 1];

    for (XUINT32 i = 0; i < cUtf8Copy && cEncoded < (cUtf8Copy * 3 + 1); i++)
    {
        if (pszUtf8Copy[i] > 126 || pszUtf8Copy[i] < 33 /* CTL or outside ASCII range */)
        {
            pszEncoded[cEncoded++] = L'%';
            pszEncoded[cEncoded++] = HEX_DIGIT_CHARACTERS[(pszUtf8Copy[i] & 0xf0) >> 4];
            pszEncoded[cEncoded++] = HEX_DIGIT_CHARACTERS[(pszUtf8Copy[i] & 0x0f)];
        }

        else
        {
            pszEncoded[cEncoded++] = pszUtf8Copy[i];
        }
    }
    pszEncoded[cEncoded] = L'\0';

    *pcEncoded = cEncoded;
    *ppszEncoded = pszEncoded;
    pszEncoded = NULL;

Cleanup:
    delete [] pszEncoded;
    delete [] pszUtf8Copy;

    RRETURN(hr);
}
