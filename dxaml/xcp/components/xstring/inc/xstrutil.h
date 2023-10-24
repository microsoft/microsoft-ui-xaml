// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Wide string manipulation functions assume 16 bit aligned addresses and ANSI code page

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
XINT32 __cdecl xstrncmp(
    const _In_reads_(cChar) WCHAR *pTrg,
    const _In_reads_(cChar) WCHAR *pSrc,
    _In_ XUINT32 cChar);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
XINT32 __cdecl xstrncmpi(
    const _In_reads_(cChar) WCHAR *pTrg,
    const _In_reads_(cChar) WCHAR *pSrc,
    _In_ XUINT32 cChar);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
void * __cdecl xstrncpy(
    _Out_writes_(cChar) WCHAR *pTrg,
    _In_reads_(cChar) const WCHAR *pSrc,
    _In_ XUINT32 cChar);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
_Check_return_ WCHAR* __cdecl xstrstr(
    const _In_z_ WCHAR *pSrc,
    const _In_z_ WCHAR *pSub);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
_Check_return_ WCHAR* __cdecl xstrnstr(
    const _In_z_ WCHAR *pSrc,
    const _In_z_ WCHAR *pSub,
    _In_ XUINT32 cChar);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
_Check_return_ WCHAR* __cdecl xstrnistr(
    const _In_z_ WCHAR *pSrc,
    const _In_z_ WCHAR *pSub,
    _In_ XUINT32 cChar);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
_Check_return_ WCHAR* __cdecl xstrnstr_s(
    _In_reads_( f_cxchSrc )       WCHAR   f_rgxchSrc[ ],
    _In_                     const XUINT32 f_cxchSrc,
    _In_reads_( f_cxchSub )       WCHAR   f_rgxchSub[ ],
                             const XUINT32 f_cxchSub );

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
_Check_return_ WCHAR* __cdecl xstritoa(const _In_ XUINT32 value, _Out_ XUINT32* pcchString);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
_Check_return_ HRESULT __cdecl xstratoi(_In_z_ WCHAR* pszInteger, _Out_ XINT32 *pValue);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
void   __cdecl xstrfree(
    _Inout_ WCHAR* theString);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
_Check_return_ __ecount_opt((sourceStringLen + 1)) _Null_terminated_ WCHAR* __cdecl xstralloc(
    const  _In_reads_opt_(sourceStringLen) WCHAR* sourceString,
    const _In_ XUINT32  sourceStringLen);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
WCHAR* __cdecl xsubstr(
    const __in_ecount_z((cchStringLength + 1)) WCHAR* pszString,
    const _In_ XUINT32  cchStringLength,
    const _In_ XUINT32  uiOffset,
    const _In_ XUINT32  cchSubstring);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
XUINT32 __cdecl xfindchar(
    const __in_ecount_z(theStringLen) WCHAR* theString,
    const _In_ XUINT32  theStringLen,
    const _In_ WCHAR    theChar);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
XUINT32
__cdecl
xfindcharreverse(
    const __in_ecount_z(theStringLen) WCHAR* theString,
    const _In_ XUINT32  theStringLen,
    const _In_ WCHAR    theChar);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
void __cdecl xstrconcat(
    _Outptr_result_buffer_maybenull_((*theCombinedStrLen+1)) WCHAR** theCombinedStr,
    _Out_ XUINT32*                       theCombinedStrLen,
    const _In_reads_(string1Len) WCHAR* string1,
    const _In_ XUINT32                   string1Len,
    const _In_reads_(string2Len) WCHAR* string2,
    const _In_ XUINT32                   string2Len);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
HRESULT __cdecl xstrnlen(
    const __in_ecount_z_opt(maxLen) WCHAR* theString,
    const _In_ XUINT32               maxLen,
    _Out_range_(<=,maxLen) XUINT32*  len);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
inline XUINT32 __cdecl xstrlen(
    _In_z_ const WCHAR *pString
    )
{
    // sanity check
    if (pString == NULL)
        return(0);

    const WCHAR *pEos = pString;

    while (*pEos++);

    return XUINT32(pEos - pString - 1);
}

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
void
__cdecl
    xstrntoUpper(
    _Inout_updates_z_(cChar) WCHAR *pInBuffer,
    _In_ XUINT32 cChar
);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
void
__cdecl
    xstrntoLower(
    _Inout_updates_z_(cChar) WCHAR *pInBuffer,
    _In_ XUINT32 cChar
);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
HRESULT
__cdecl
xstrsplit(
    _Inout_z_ WCHAR* pSource,
    WCHAR separator,
    _Inout_ XUINT32& cSubstrings,
    _Out_writes_(cSubstrings) WCHAR** aSubstrings);

// DEPRECATED: Do not use in new code. Prefer built-in string utilities.
WCHAR* xstrchr(_In_reads_(cchString) WCHAR* pszString, XUINT32 cchString, WCHAR ch);
