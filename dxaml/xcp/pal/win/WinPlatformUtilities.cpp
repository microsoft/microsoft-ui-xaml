// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <strsafe.h>

XUINT32 
CWinPlatformUtilities::vXswprintf_s(
    _Out_writes_z_(cchBuf) WCHAR *pBuf,
    XUINT32 cchBuf,
    _In_z_ const WCHAR* pFormat,
    _In_ va_list args
    )
{
    return vswprintf_s(pBuf, cchBuf, pFormat, args);
}


XUINT32 
CWinPlatformUtilities::Xswprintf_s(
    _Out_writes_z_(cchBuf) WCHAR *pBuf,
    XUINT32 cchBuf,
    _In_z_ const WCHAR* pFormat,
    ...
    )
{    
    va_list args;
    XUINT32 uRet;
    
    va_start(args, pFormat);
    uRet = vXswprintf_s(pBuf, cchBuf, pFormat, args);
    va_end(args);

    return uRet;
}

bool CWinPlatformUtilities::IsUILanguageRTL()
{
    WCHAR wchLCIDFontSig[16];
    LANGID langId = GetUserDefaultUILanguage();

    // Use bit 123 of the Unicode subset bitfields, for info see http://blogs.msdn.com/michkap/archive/2006/03/03/542963.aspx 
    if (GetLocaleInfoW(langId, LOCALE_FONTSIGNATURE, &wchLCIDFontSig[0], (sizeof(wchLCIDFontSig)/sizeof(WCHAR))) &&
        (wchLCIDFontSig[7] & (WCHAR)0x0800))
    {
        return true;
    }
    else
    {
        return false;
    }
}
