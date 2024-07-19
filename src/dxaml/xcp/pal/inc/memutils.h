// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Portable replacements for memcpy, memmove, and memset functions

#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

// These functions don't optimize for 32 bit load/stores but are safe to call
// with any address

#if !defined(_INC_STRING)
extern "C"
{
#ifndef _ERRCODE_DEFINED  
#define _ERRCODE_DEFINED  
typedef int errcode;  
typedef int errno_t;  
#endif  

#if  defined(_WIN64) || defined(__LP64__)
void * __cdecl memcpy(_Out_writes_bytes_(cBytes) void *pTrg, _In_reads_bytes_(cBytes) const void *pSrc, _In_ size_t cBytes);
void * __cdecl memmove(__out_bcount_full_opt(cBytes) void *pTrg, _In_reads_bytes_opt_(cBytes) const void *pSrc, _In_ size_t cBytes);
void * __cdecl memset(_Out_writes_bytes_(cBytes) void *pTrg, XINT32 nValue, _In_ size_t cBytes);
XINT32 __cdecl memcmp(const _In_reads_bytes_(cBytes) void *pSrc1, _In_reads_bytes_(cBytes) const void *pSrc2, _In_ size_t cBytes);
errno_t __cdecl memcpy_s(_Out_writes_bytes_(cTrgBytes) void *pTrg, _In_ size_t cTrgBytes, _In_reads_bytes_(cBytes) const void *pSrc, _In_ size_t cBytes);
#else
void * __cdecl memcpy(_Out_writes_bytes_(cBytes) void *pTrg, _In_reads_bytes_(cBytes) const void *pSrc, _In_ XUINT32 cBytes);
void * __cdecl memmove(__out_bcount_full_opt(cBytes) void *pTrg, _In_reads_bytes_opt_(cBytes) const void *pSrc, _In_ XUINT32 cBytes);
void * __cdecl memset(_Out_writes_bytes_(cBytes) void *pTrg, XINT32 nValue, _In_ XUINT32 cBytes);
XINT32 __cdecl memcmp(const _In_reads_bytes_(cBytes) void *pSrc1, _In_reads_bytes_(cBytes) const void *pSrc2, _In_ XUINT32 cBytes);
errno_t __cdecl memcpy_s(_Out_writes_bytes_(cTrgBytes) void *pTrg, _In_ XUINT32 cTrgBytes, _In_reads_bytes_(cBytes) const void *pSrc, _In_ XUINT32 cBytes);
#endif
}

#endif

#endif //#ifndef __MEMUTILS_H__
