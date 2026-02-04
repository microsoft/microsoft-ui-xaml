// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {

class AKParserUnitTests : public WEX::TestClass<AKParserUnitTests>
{
public:
    BEGIN_TEST_CLASS(AKParserUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(CanParseZeroLengthString)
        TEST_METHOD_PROPERTY(L"Description", L"Validates AcessKeyParser can handle 0-length string inputs.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HandlesMixedCaseUnicodeInput)
        TEST_METHOD_PROPERTY(L"Description", L"Validates AcessKeyParser can parse mixed-case unicode characters.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanParseSingleCharacterKey)
        TEST_METHOD_PROPERTY(L"Description", L"Validates AcessKeyParser can parse a single character.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanParseMultipleCharacterKey)
        TEST_METHOD_PROPERTY(L"Description", L"Validates AcessKeyParser can parse a multiple character strings.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TestFailToParseDegenerateCharacters)
        TEST_METHOD_PROPERTY(L"Description", L"Validates AcessKeyParser correctly fails to parse degenerate and disallowed input characters.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TestFailToParseDegenerateStrings)
        TEST_METHOD_PROPERTY(L"Description", L"Validates AcessKeyParser correctly fails to parse degenerate and disallowed input strings.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanParseEastAsianCharacters)
        TEST_METHOD_PROPERTY(L"Description", L"Validates AcessKeyParser can parse characters from some East Asian languages.")
    END_TEST_METHOD()

};

} } } } }
