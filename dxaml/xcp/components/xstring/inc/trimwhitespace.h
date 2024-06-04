// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

void
TrimWhitespace(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR* pString,
    _Out_ XUINT32* pcResult,
    _Outptr_result_buffer_(*pcResult) const WCHAR **ppResult
    );

void
TrimTrailingWhitespace(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR* pString,
    _Out_ XUINT32* pcResult,
    _Outptr_result_buffer_(*pcResult) const WCHAR** ppResult
    );

void
TrimWhitespace(
    _In_ XUINT32 cString,
    _In_reads_(cString) WCHAR* pString,
    _Out_ XUINT32* pcResult,
    _Outptr_result_buffer_(*pcResult) WCHAR **ppResult
    );

void
TrimTrailingWhitespace(
    _In_ XUINT32 cString,
    _In_reads_(cString) WCHAR* pString,
    _Out_ XUINT32* pcResult,
    _Outptr_result_buffer_(*pcResult) WCHAR** ppResult
    );
