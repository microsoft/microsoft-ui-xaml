// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Class:  CommonPlatformUtilities
//  Synopsis:
//      Singleton class to implement platform utilities

#pragma once

class CommonPlatformUtilities: public IPlatformUtilities
{
protected:
    CommonPlatformUtilities() {}
public:
    
    _Check_return_ __ecount_opt(sourceStringLen + 1) WCHAR* Xstralloc(
        const _In_reads_opt_(sourceStringLen) WCHAR* sourceString,
        const _In_ XUINT32  sourceStringLen) override;

    void Xstrfree(
        _In_z_ WCHAR* theString) override;

    void * Xstrncpy(
        _Out_writes_(cChar) WCHAR *pTrg,
        _In_reads_(cChar) const WCHAR *pSrc,
        _In_ XUINT32 cChar) override;

    XUINT32 Xstrlen(
        _Null_terminated_ const WCHAR *pString ) override;

    XINT32 Xstrncmpi(
        const _In_reads_(cChar) WCHAR *pTrg,
        const _In_reads_(cChar) WCHAR *pSrc,
        _In_ XUINT32 cChar) override;

    XINT32 Xstrncmp(
        const _In_reads_(cChar) WCHAR *pTrg,
        const _In_reads_(cChar) WCHAR *pSrc,
        _In_ XUINT32 cChar) override;
};
