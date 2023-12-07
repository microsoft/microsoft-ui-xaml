// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

extern "C"
_Check_return_ HRESULT ExtractFirstHeaderValueFromPackedString(
    _In_ const xstring_ptr_view& strPackedHeaders,
    _In_ const xstring_ptr_view& strHeaderName,
    _Out_ xstring_ptr* pstrHeaderValue);


_Check_return_ HRESULT FindHeaderValueInPackedString(
    _In_ const xstring_ptr_view& strPackedHeaders,
    _In_ const xstring_ptr_view& strHeaderName,
    XUINT32 ichStart,
    _Out_ XUINT32* pichEnd,
    _Outptr_result_buffer_(*pcchHeaderValue + 1) const WCHAR** pwszHeaderValue,
    _Out_ XUINT32* pcchHeaderValue);
   

