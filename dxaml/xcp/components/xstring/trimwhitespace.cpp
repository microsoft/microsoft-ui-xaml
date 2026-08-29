// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <TrimWhitespace.h>

void TrimWhitespace(
    _In_ uint32_t cString,
    _In_reads_(cString) const WCHAR* pString,
    _Out_ uint32_t* pcResult,
    _Outptr_result_buffer_(*pcResult) const WCHAR **ppResult
    )
{
    while (cString && isspace(*pString))
    {
        pString = pString + 1;
        cString = cString - 1;
    }

    *pcResult = cString;
    *ppResult = pString;
}

void
TrimTrailingWhitespace(
    _In_ uint32_t cString,
    _In_reads_(cString) const WCHAR* pString,
    _Out_ uint32_t* pcResult,
    _Outptr_result_buffer_(*pcResult) const WCHAR** ppResult
    )
{
    while (cString && isspace(pString[cString - 1]))
    {
        cString = cString - 1;
    }

    *pcResult = cString;
    *ppResult = pString;
}

void TrimWhitespace(
    _In_ uint32_t cString,
    _In_reads_(cString) WCHAR* pString,
    _Out_ uint32_t* pcResult,
    _Outptr_result_buffer_(*pcResult) WCHAR **ppResult
    )
{
    while (cString && isspace(*pString))
    {
        pString = pString + 1;
        cString = cString - 1;
    }

    *pcResult = cString;
    *ppResult = pString;
}

void
TrimTrailingWhitespace(
    _In_ uint32_t cString,
    _In_reads_(cString) WCHAR* pString,
    _Out_ uint32_t* pcResult,
    _Outptr_result_buffer_(*pcResult) WCHAR** ppResult
    )
{
    while (cString && isspace(pString[cString - 1]))
    {
        cString = cString - 1;
    }

    *pcResult = cString;
    *ppResult = pString;
}
