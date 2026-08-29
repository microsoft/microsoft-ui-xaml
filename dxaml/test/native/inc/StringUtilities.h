// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

inline void VERIFY_ARE_STRINGS_EQUAL(_In_ LPCWSTR expected, _In_ LPCWSTR actual)
{
    VERIFY_ARE_EQUAL(expected, actual);
}

inline void VERIFY_ARE_STRINGS_EQUAL(_In_ LPCWSTR expected, _In_ HSTRING actual)
{
    VERIFY_ARE_EQUAL(expected, (LPCWSTR)WindowsGetStringRawBuffer(actual, nullptr));
}

inline void VERIFY_ARE_STRINGS_NOT_EQUAL(_In_ LPCWSTR expected, _In_ HSTRING actual)
{
    VERIFY_ARE_NOT_EQUAL(expected, (LPCWSTR)WindowsGetStringRawBuffer(actual, nullptr));
}

// Enable VERIFY_ARE_EQUAL() to handle LPCWSTR
namespace WEX {
    namespace TestExecution {
        template <>
        class VerifyCompareTraits<LPCWSTR, LPCWSTR>
        {
        public:
            static bool AreEqual(LPCWSTR expected, LPCWSTR actual)
            {
                if (expected == nullptr && actual == nullptr)
                {
                    return true;
                }
                else if (expected == nullptr || actual == nullptr)
                {
                    return false;
                }
                return (wcscmp(expected, actual) == 0);
            }

            static bool AreSame(LPCWSTR expected, LPCWSTR actual)
            {
                return (expected == actual);
            }

            static bool IsLessThan(LPCWSTR expectedLess, LPCWSTR expectedGreater)
            {
                return (wcscmp(expectedLess, expectedGreater) < 0);
            }

            static bool IsGreaterThan(LPCWSTR expectedGreater, LPCWSTR expectedLess)
            {
                return (wcscmp(expectedLess, expectedGreater) > 0);
            }

            static bool IsNull(LPCWSTR object)
            {
                return object == nullptr;
            }
        };
    }
}
