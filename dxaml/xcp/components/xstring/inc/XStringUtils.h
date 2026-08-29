// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

_Check_return_ HRESULT TrimStringEndWithEllipsis(
    _In_ const xstring_ptr_view& strInput, _In_ XUINT32 maxLength, _Out_ xstring_ptr* pstrResult);

_Check_return_ HRESULT TrimStringStartWithEllipsis(
    _In_ const xstring_ptr_view& strInput, _In_ XUINT32 maxLength, _Out_ xstring_ptr* pstrResult);

char* WideCharToChar(
    _In_ XUINT32 cstr,
    _In_reads_(cstr) const WCHAR* pstr,
    _Inout_opt_ XUINT32 *pcstr = NULL,
    _In_reads_opt_(*pcstr) char* pstrDest = NULL);

WCHAR* CharToWideChar(_Inout_ XUINT32* cstr, _In_reads_(*cstr) const char* pstr);

xstring_ptr StringCchPrintfWWrapper(_In_z_ const wchar_t* format, ...);