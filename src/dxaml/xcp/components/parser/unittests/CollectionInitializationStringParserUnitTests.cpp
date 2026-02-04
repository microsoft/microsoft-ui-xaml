// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CollectionInitializationStringParserUnitTests.h>
#include <CollectionInitializationStringParser.h>
#include "XStringBuilder.h"
#include "xstring_ptr.h"

using namespace Parser;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework {  

    void CollectionInitializationStringParserUnitTests::VerifyValidInitializationStringParsing()
    {
        // create xstring_ptr from WCHAR, call ParseInitializationString
        Jupiter::stack_vector<xstring_ptr, 8> parsedStrings1;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input1, L"1,'3, 4', a");
        VERIFY_SUCCEEDED(CollectionInitializationStringParser::ParseInitializationString(input1, parsedStrings1));
        // create comparison vector
        std::vector<xstring_ptr> parsedStringsToCompare1;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg11, L"1");
        parsedStringsToCompare1.push_back(arg11);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg12, L"3, 4");
        parsedStringsToCompare1.push_back(arg12);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg13, L"a");
        parsedStringsToCompare1.push_back(arg13);
        // verify output vector and do cleanup
        VerifyOutput(parsedStrings1, parsedStringsToCompare1);

        // escaped special characters in regular and literal text
        Jupiter::stack_vector<xstring_ptr, 8> parsedStrings2;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input2, L"'[hello], world\\' ',hello\\, \\[world\\]");
        VERIFY_SUCCEEDED(CollectionInitializationStringParser::ParseInitializationString(input2, parsedStrings2));
        // create comparison vector
        std::vector<xstring_ptr> parsedStringsToCompare2;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg21, L"[hello], world' ");
        parsedStringsToCompare2.push_back(arg21);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg22, L"hello, [world]");
        parsedStringsToCompare2.push_back(arg22);
        // verify output vector
        VerifyOutput(parsedStrings2, parsedStringsToCompare2);

        // whitespace
        Jupiter::stack_vector<xstring_ptr, 8> parsedStrings3;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input3, L" ");
        VERIFY_SUCCEEDED(CollectionInitializationStringParser::ParseInitializationString(input3, parsedStrings3));
        // create comparison vector
        std::vector<xstring_ptr> parsedStringsToCompare3;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg31, L"");
        parsedStringsToCompare3.push_back(arg31);
        // verify output vector
        VerifyOutput(parsedStrings3, parsedStringsToCompare3);
        
        // multiple whitespaces around commas
        Jupiter::stack_vector<xstring_ptr, 8> parsedStrings4;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input4, L" 1  ,   a");
        VERIFY_SUCCEEDED(CollectionInitializationStringParser::ParseInitializationString(input4, parsedStrings4));
        // create comparison vector
        std::vector<xstring_ptr> parsedStringsToCompare4;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg41, L"1");
        parsedStringsToCompare4.push_back(arg41);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg42, L"a");
        parsedStringsToCompare4.push_back(arg42);
        // verify output vector and do cleanup
        VerifyOutput(parsedStrings4, parsedStringsToCompare4);

        // whitespaces around literal text
        Jupiter::stack_vector<xstring_ptr, 8> parsedStrings5;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input5, L"   'a'  ,'   b ','   '");
        VERIFY_SUCCEEDED(CollectionInitializationStringParser::ParseInitializationString(input5, parsedStrings5));
        // create comparison vector
        std::vector<xstring_ptr> parsedStringsToCompare5;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg51, L"a");
        parsedStringsToCompare5.push_back(arg51);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg52, L"   b ");
        parsedStringsToCompare5.push_back(arg52);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg53, L"   ");
        parsedStringsToCompare5.push_back(arg53);
        // verify output vector and do cleanup
        VerifyOutput(parsedStrings5, parsedStringsToCompare5);

        // escaped comma and non-special characters
        Jupiter::stack_vector<xstring_ptr, 8> parsedStrings6;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input6, L"a\\, ,\\b");
        VERIFY_SUCCEEDED(CollectionInitializationStringParser::ParseInitializationString(input6, parsedStrings6));
        // create comparison vector
        std::vector<xstring_ptr> parsedStringsToCompare6;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg61, L"a,");
        parsedStringsToCompare6.push_back(arg61);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg62, L"b");
        parsedStringsToCompare6.push_back(arg62);
        // verify output vector and do cleanup
        VerifyOutput(parsedStrings6, parsedStringsToCompare6);

        // other whitespace types
        Jupiter::stack_vector<xstring_ptr, 8> parsedStrings7;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input7, L"\n\f\r1 \v, \ta,'a, \tb'");
        VERIFY_SUCCEEDED(CollectionInitializationStringParser::ParseInitializationString(input7, parsedStrings7));
        // create comparison vector
        std::vector<xstring_ptr> parsedStringsToCompare7;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg71, L"1");
        parsedStringsToCompare7.push_back(arg71);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg72, L"a");
        parsedStringsToCompare7.push_back(arg72);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(arg73, L"a, \tb");
        parsedStringsToCompare7.push_back(arg73);
        // verify output vector and do cleanup
        VerifyOutput(parsedStrings7, parsedStringsToCompare7);
    }

    void CollectionInitializationStringParserUnitTests::ThrowsInvalidArgumentForInvalidInitializationString()
    {
        // unescaped reserved symbol
        Jupiter::stack_vector<xstring_ptr, 8> parsedStrings;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input1, L"1, [3, 4]");
        VERIFY_FAILED(CollectionInitializationStringParser::ParseInitializationString(input1, parsedStrings));

        // unmatched quote, missing quote
        parsedStrings.m_vector.clear();
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input2, L"1,'2");
        VERIFY_FAILED(CollectionInitializationStringParser::ParseInitializationString(input2, parsedStrings));

        // unmatched quote, extra quote
        parsedStrings.m_vector.clear();
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input3, L"1, '3''");
        VERIFY_FAILED(CollectionInitializationStringParser::ParseInitializationString(input3, parsedStrings));

        // no comma between values
        parsedStrings.m_vector.clear();
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input4, L"'1' '2'");
        VERIFY_FAILED(CollectionInitializationStringParser::ParseInitializationString(input4, parsedStrings));

        // comma at end
        parsedStrings.m_vector.clear();
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input5, L"'2',");
        VERIFY_FAILED(CollectionInitializationStringParser::ParseInitializationString(input5, parsedStrings));
        
        // unmatched literal text quotes
        parsedStrings.m_vector.clear();
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(input6, L"'foo bar\"");
        VERIFY_FAILED(CollectionInitializationStringParser::ParseInitializationString(input6, parsedStrings));
    }

    void CollectionInitializationStringParserUnitTests::VerifyOutput(Jupiter::stack_vector<xstring_ptr, 8>& output, std::vector<xstring_ptr>& correctOutput)
    {
        // verify vector sizes are equal
        VERIFY_ARE_EQUAL(output.m_vector.size(), correctOutput.size());

        // verify vector contents are the same
        for (size_t i = 0; i < correctOutput.size(); i++)
        {
            VERIFY_IS_TRUE(output.m_vector[i].Equals(correctOutput[i]));
        }
    }
 
}}}}} 
