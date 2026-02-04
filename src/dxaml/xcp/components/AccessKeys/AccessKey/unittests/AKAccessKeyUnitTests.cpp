// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>

#include "AKAccessKeyUnitTests.h"
#include "AccessKey.h"
#include <XamlLogging.h>
#include "CxxMock.h"
#include "WaitForDebugger.h"

using namespace AccessKeys;


namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {
    void AKAccessKeyUnitTests::VerifyEqualsMethodOnIdenticalAccessKeys()
    {
        WaitForDebugger();
        wchar_t myKey[maxAccessKeyLength] = { L'M', L'A', L'8', L'B', L'Z', L'2' };
        wchar_t emptyKey[maxAccessKeyLength];

        AKAccessKey aKey1(myKey);
        AKAccessKey aKey2(myKey);
        AKAccessKey aKeyEmpty1(emptyKey);
        AKAccessKey aKeyEmpty2(emptyKey);

        // Check access keys equal themselves
        VERIFY_ARE_EQUAL(aKey1, aKey1);
        VERIFY_ARE_EQUAL(aKey2, aKey2);

        // These access keys should also equal each other
        VERIFY_ARE_EQUAL(aKey1, aKey2);
        VERIFY_ARE_EQUAL(aKey2, aKey1);

        // Should also return true on empty access keys
        VERIFY_ARE_EQUAL(aKeyEmpty1, aKeyEmpty1);
        VERIFY_ARE_EQUAL(aKeyEmpty1, aKeyEmpty2);
    }

    void AKAccessKeyUnitTests::VerifyEqualsMethodOnDifferentAccessKeys()
    {
        WaitForDebugger();
        wchar_t myKey1[maxAccessKeyLength] = { L'M', L'A', L'C', L'B', L'Z', L'1' };
        wchar_t myKey2[maxAccessKeyLength] = { L'T', L'H', L'P', L'C', L'X', L'2' };
        wchar_t emptyKey[maxAccessKeyLength] = L"";

        AKAccessKey aKey1(myKey1);
        AKAccessKey aKey2(myKey2);
        AKAccessKey aKeyEmpty(emptyKey);

        // Two different keys should be not be equal
        VERIFY_ARE_NOT_EQUAL(aKey1, aKey2);
        VERIFY_ARE_NOT_EQUAL(aKey2, aKey1);

        // Testing against an empty key should also return false unless the key itself is empty
        VERIFY_ARE_NOT_EQUAL(aKey1, aKeyEmpty);
        VERIFY_ARE_NOT_EQUAL(aKey2, aKeyEmpty);

        VERIFY_ARE_NOT_EQUAL(aKeyEmpty, aKey2);
        VERIFY_ARE_NOT_EQUAL(aKeyEmpty, aKey1);
    }

    void AKAccessKeyUnitTests::VerifyAccessKeyAssignmentOperator()
    {
        WaitForDebugger();
        wchar_t myKey1[maxAccessKeyLength] = { L'M' };
        wchar_t myKey2[maxAccessKeyLength] = { L'T' };

        AKAccessKey aKey1(myKey1);
        AKAccessKey aKey2(myKey2);
        AKAccessKey aKeyOther;

        VERIFY_ARE_NOT_EQUAL(aKeyOther, aKey1);
        aKeyOther = L'M';
        VERIFY_ARE_EQUAL(aKeyOther, aKey1);

        VERIFY_ARE_NOT_EQUAL(aKeyOther, aKey2);
        aKeyOther = aKey2;
        VERIFY_ARE_EQUAL(aKeyOther, aKey2);
    }

    void AKAccessKeyUnitTests::CorrectlyMatchMixedCaseUnicodeKeys()
    {
        WaitForDebugger();
        AKAccessKey aKey1, aKey2;

        // Let's try a single character first.
        aKey1 = std::wstring(L"a");
        aKey2 = std::wstring(L"A");
        VERIFY_ARE_EQUAL(aKey1, aKey2);

        // Multiple character mixed case (but still english)
        aKey1 = std::wstring(L"Bam");
        aKey2 = std::wstring(L"BAm");
        VERIFY_ARE_EQUAL(aKey1, aKey2);

        // 0xf1 is lowercase n with tilde
        // Let's try a single character first.
        aKey1 = std::wstring(L"iA\xf1");
        aKey2 = std::wstring(L"Ia\xd1");
        VERIFY_ARE_EQUAL(aKey1, aKey2);
    }

    void AKAccessKeyUnitTests::CanCorrectlyDeterminPartialMatches()
    {
        WaitForDebugger();
        AKAccessKey aKey1, aKey2;

        // Identical keys should be partial matches of each other
        aKey1 = std::wstring(L"A");
        aKey2 = std::wstring(L"A");
        VERIFY_IS_TRUE(aKey1.IsPartialMatch(aKey2));
        VERIFY_IS_TRUE(aKey2.IsPartialMatch(aKey1));

        aKey1 = std::wstring(L"BAM");
        aKey2 = std::wstring(L"BAM");
        VERIFY_IS_TRUE(aKey1.IsPartialMatch(aKey2));
        VERIFY_IS_TRUE(aKey2.IsPartialMatch(aKey1));

        aKey1 = std::wstring(L"NICE-1");
        aKey2 = std::wstring(L"NICE-1");
        VERIFY_IS_TRUE(aKey1.IsPartialMatch(aKey2));
        VERIFY_IS_TRUE(aKey2.IsPartialMatch(aKey1));

        // If first n, non null characters of a key match the argument, that is a partial match
        aKey1 = std::wstring(L"A");
        aKey2 = std::wstring(L"AB");
        VERIFY_IS_TRUE(aKey1.IsPartialMatch(aKey2));
        VERIFY_IS_FALSE(aKey2.IsPartialMatch(aKey1));

        aKey1 = std::wstring(L"BA");
        aKey2 = std::wstring(L"BAM");
        VERIFY_IS_TRUE(aKey1.IsPartialMatch(aKey2));
        VERIFY_IS_FALSE(aKey2.IsPartialMatch(aKey1));

        aKey1 = std::wstring(L"NICE-");
        aKey2 = std::wstring(L"NICE-1");
        VERIFY_IS_TRUE(aKey1.IsPartialMatch(aKey2));
        VERIFY_IS_FALSE(aKey2.IsPartialMatch(aKey1));

        // Empty string should be a partial match for everything, but only an empty string should be a partial match of it
        aKey1 = std::wstring(L"");
        aKey2 = std::wstring(L"AB");
        VERIFY_IS_TRUE(aKey1.IsPartialMatch(aKey2));
        VERIFY_IS_FALSE(aKey2.IsPartialMatch(aKey1));

        aKey2 = std::wstring(L"");
        VERIFY_IS_TRUE(aKey2.IsPartialMatch(aKey1));
    }

} } } } }
