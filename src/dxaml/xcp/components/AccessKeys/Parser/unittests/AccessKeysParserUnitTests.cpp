// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AccessKeysParserUnitTests.h"
#include "AccessKeysParser.h"
#include "AccessKey.h"
#include <winstring.h>
#include <XamlLogging.h>

using namespace AccessKeys;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {
    void AKParserUnitTests::CanParseZeroLengthString()
    {
        AKAccessKey groundTruthAK, parsedOutputAK;
        groundTruthAK = 0;
        parsedOutputAK = 0;

        std::wstring stringIn;

        stringIn = L"";

        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);
    }

    void AKParserUnitTests::HandlesMixedCaseUnicodeInput()
    {
        AKAccessKey groundTruthAK, parsedOutputAK;
        std::wstring stringIn;

        stringIn = L"\xd1";
        groundTruthAK = stringIn;
        VERIFY_IS_TRUE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        // Appending next output to the end of groundTruth and parsedOutput.  Vectors should match if parsed output is correct
        stringIn = L"\xf1";
        groundTruthAK = stringIn;
        VERIFY_IS_TRUE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);
    }

    void AKParserUnitTests::TestFailToParseDegenerateStrings()
    {
        // Tests that invalid AccessKey strings cause TryParseAccessKey to return false
        // and no change to parsedOutputAK
        AKAccessKey groundTruthAK, parsedOutputAK;
        groundTruthAK = 0;
        parsedOutputAK = 0;

        std::wstring stringIn;

        stringIn=L"S+CtRL a";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L"ALT + A";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L"CONTROL T";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L"S\x200b"; //S and a 0-width space
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L"a\t";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L"1234567";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);
    }

    void AKParserUnitTests::TestFailToParseDegenerateCharacters()
    {
        // Tests that invalid AccessKey characters cause TryParseAccessKey to return false
        // and no change to parsedOutputAK
        AKAccessKey groundTruthAK, parsedOutputAK;
        groundTruthAK = 0;
        parsedOutputAK = 0;

        std::wstring stringIn;

        stringIn=L"\t";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L"\r";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L"\n";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L"\0";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L" ";
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn=L"\x200b"; //S and a 0-width space
        VERIFY_IS_FALSE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);
    }

    void AKParserUnitTests::CanParseSingleCharacterKey()
    {
        AKAccessKey groundTruthAK, parsedOutputAK;

        std::wstring stringIn;

        groundTruthAK = L'S';
        stringIn=L"S";

        VERIFY_IS_TRUE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);
    }

    void AKParserUnitTests::CanParseMultipleCharacterKey()
    {
        AKAccessKey groundTruthAK, parsedOutputAK;

        std::wstring stringIn;

        stringIn = L"SA";
        groundTruthAK = stringIn;

        VERIFY_IS_TRUE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn = L"BAM";
        groundTruthAK = stringIn;

        VERIFY_IS_TRUE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        stringIn = L"D0";
        groundTruthAK = stringIn;

        VERIFY_IS_TRUE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);
    }

    void AKParserUnitTests::CanParseEastAsianCharacters()
    {
        AKAccessKey groundTruthAK, parsedOutputAK;

        std::wstring stringIn;

        // Thai characters
        // For some reason, when compiled with the actual character these were not
        // parsing correctly (defaulting to ?)
        stringIn = L"\xe1f";
        groundTruthAK = stringIn;
        VERIFY_IS_TRUE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);

        // Japanese character
        stringIn = L"\x30c1";
        groundTruthAK = stringIn;
        VERIFY_IS_TRUE(AKParser::TryParseAccessKey(stringIn, parsedOutputAK));
        VERIFY_ARE_EQUAL(parsedOutputAK, groundTruthAK);
    }

} } } } }
