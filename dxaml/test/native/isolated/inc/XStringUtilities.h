// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

inline void VERIFY_ARE_STRINGS_EQUAL(_In_ LPCWSTR expected, _In_ const xstring_ptr_view& actual)
{
    VERIFY_ARE_EQUAL(expected, (LPCWSTR)actual.GetBuffer());
}

inline void VERIFY_ARE_STRINGS_NOT_EQUAL(_In_ LPCWSTR expected, _In_ const xstring_ptr_view& actual)
{
    VERIFY_ARE_NOT_EQUAL(expected, (LPCWSTR)actual.GetBuffer());
}

inline void VERIFY_ARE_STRINGS_EQUAL(_In_ const xstring_ptr_view& expected, _In_ const xstring_ptr_view& actual)
{
    VERIFY_ARE_EQUAL((LPCWSTR)expected.GetBuffer(), (LPCWSTR)actual.GetBuffer());
}

inline void VERIFY_ARE_STRINGS_NOT_EQUAL(_In_ const xstring_ptr_view& expected, _In_ const xstring_ptr_view& actual)
{
    VERIFY_ARE_NOT_EQUAL((LPCWSTR)expected.GetBuffer(), (LPCWSTR)actual.GetBuffer());
}
