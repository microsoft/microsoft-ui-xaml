// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "StringUnitTests.h"
#include <XamlLogging.h>

#include <xstring_ptr.h>
#include <StringConversions.h>
#include <EnumDefs.g.h>
#include <EnumValueTable.g.h>

#include <StringUtilities.h>
#include <XStringUtilities.h>
#include <xstringbuilder.h>

using namespace WEX::Common;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Strings {

    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_Test, L"Test");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_test, L"test");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_Testing, L"Testing");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_String, L"String");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_stRING, L"stRING");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_ring, L"ring");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_rINg, L"rINg");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_strWhitespace, L"    ");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_strNotWhitespace, L"  . ");

    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_empty, L"");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_empty2, L"");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_a, L"a");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_a2, L"a");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_ab, L"ab");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_aB, L"aB");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_abc, L"abc");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_abc_2, L"abc");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_b, L"b");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_bc, L"bc");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_str_Bc, L"Bc");

    void StringUnitTests::ConstXStringPtrDeclaration()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strTest, L"Testing constant string");
        DECLARE_STATIC_CONST_STRING_IN_TEST_CODE(s_strTest, L"Testing static constant string");

        VERIFY_ARE_STRINGS_EQUAL(L"Testing constant string", c_strTest);
        VERIFY_ARE_STRINGS_EQUAL(L"Testing static constant string", s_strTest);

        VERIFY_ARE_STRINGS_NOT_EQUAL(c_strTest, s_strTest);
    }

    void StringUnitTests::MalformedNullStringStorage()
    {
        xstring_ptr_storage malformed_xstring_ptr_storage_null = { nullptr, 7 /* Count */, FALSE /* IsEphemeral */, FALSE /* IsRuntimeStringHandle */ };
        xstring_ptr_storage malformed_xstring_ptr_storage_not_null = { L"abc", 0 /* Count */, FALSE /* IsEphemeral */, FALSE /* IsRuntimeStringHandle */ };

        VERIFY_IS_FALSE(!!XSTRING_PTR_FROM_STORAGE(malformed_xstring_ptr_storage_null).IsNull());
        VERIFY_IS_FALSE(!!XSTRING_PTR_FROM_STORAGE(malformed_xstring_ptr_storage_not_null).IsNull());

        VERIFY_IS_TRUE(!!XSTRING_PTR_FROM_STORAGE(malformed_xstring_ptr_storage_null).IsNullOrEmpty());
        VERIFY_IS_TRUE(!!XSTRING_PTR_FROM_STORAGE(malformed_xstring_ptr_storage_not_null).IsNullOrEmpty());
    }

    void StringUnitTests::XStringPtrBufferAndCount()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strTest, L"Testing string view functionality");

        xephemeral_string_ptr strEphemeral;
        xruntime_string_ptr strRuntime;

        c_strTest.Demote(&strEphemeral);
        VERIFY_SUCCEEDED(c_strTest.Promote(&strRuntime));

        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strEphemeral);
        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strRuntime);

        VERIFY_ARE_EQUAL(c_strTest.GetBuffer(), strEphemeral.GetBuffer());
        VERIFY_ARE_EQUAL(c_strTest.GetCount(), strEphemeral.GetCount());

        VERIFY_ARE_EQUAL(c_strTest.GetCount(), strRuntime.GetCount());

        UINT32 cConst, cEphemeral, cRuntime;

        VERIFY_ARE_EQUAL(c_strTest.GetBuffer(), c_strTest.GetBufferAndCount(&cConst));
        VERIFY_ARE_EQUAL(strEphemeral.GetBuffer(), strEphemeral.GetBufferAndCount(&cEphemeral));
        VERIFY_ARE_EQUAL(strRuntime.GetBuffer(), strRuntime.GetBufferAndCount(&cRuntime));

        VERIFY_ARE_EQUAL(cConst, cEphemeral);
        VERIFY_ARE_EQUAL(cConst, cRuntime);
    }

    void StringUnitTests::XStringPtrCreation()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strTest, L"Testing constant string");

        xstring_ptr strTest(c_strTest);
        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strTest);

        strTest.Reset();
        VERIFY_ARE_STRINGS_NOT_EQUAL(c_strTest, strTest);
        VERIFY_IS_TRUE(!!strTest.IsNull());
        VERIFY_IS_TRUE(!!strTest.IsNullOrEmpty());

        xstring_ptr str_42;
        VERIFY_SUCCEEDED(xstring_ptr::CreateFromUInt32(42, &str_42));
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42);

        xstring_ptr str_43;
        VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(STR_LEN_PAIR(L"43"), &str_43));
        VERIFY_ARE_STRINGS_EQUAL(L"43", str_43);

        xstring_ptr str_very_large;
        VERIFY_FAILED(xstring_ptr::CloneBuffer(L"43", (1 << 30), &str_very_large));
        VERIFY_IS_TRUE(!!str_very_large.IsNull());

        xstring_ptr str_42_from_null_terminated_buffer;
        VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(L"42", &str_42_from_null_terminated_buffer));
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42_from_null_terminated_buffer);

        xstring_ptr str_42_trimmed;
        VERIFY_SUCCEEDED(xstring_ptr::CloneBufferTrimWhitespace(STR_LEN_PAIR(L"  42    "), &str_42_trimmed));
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42_trimmed);

        xstring_ptr str_42_from_8bit_char_buffer;
        VERIFY_SUCCEEDED(xstring_ptr::CloneBufferCharToWideChar("42", 2, &str_42_from_8bit_char_buffer));
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42_from_8bit_char_buffer);

        xstring_ptr str_42_from_empty_8bit_char_buffer;
        VERIFY_SUCCEEDED(xstring_ptr::CloneBufferCharToWideChar(nullptr, 0, &str_42_from_empty_8bit_char_buffer));
        VERIFY_IS_TRUE(!!str_42_from_empty_8bit_char_buffer.IsNull());
    }

    void StringUnitTests::CopyConstructor()
    {
        xstring_ptr str_42;
        VERIFY_SUCCEEDED(xstring_ptr::CreateFromUInt32(42, &str_42));

        xstring_ptr str_42_from_assignment;
        str_42_from_assignment = str_42;
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42_from_assignment);

        xstring_ptr str_42_from_copy_construction(str_42);
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42_from_copy_construction);
    }

    void StringUnitTests::MoveConstructor()
    {
        xstring_ptr str_42;
        VERIFY_SUCCEEDED(xstring_ptr::CreateFromUInt32(42, &str_42));

        xstring_ptr str_42_from_move_assignment;
        str_42_from_move_assignment = std::move(str_42);
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42_from_move_assignment);
        VERIFY_IS_TRUE(!!str_42.IsNull());

        str_42 = str_42_from_move_assignment;
        VERIFY_IS_FALSE(!!str_42.IsNull());
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42);

        xstring_ptr str_42_from_move_construction(std::move(str_42));
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42_from_move_construction);
        VERIFY_IS_TRUE(!!str_42.IsNull());

        str_42 = str_42_from_move_construction;
        VERIFY_IS_FALSE(!!str_42.IsNull());
        VERIFY_ARE_STRINGS_EQUAL(L"42", str_42);
    }

    void StringUnitTests::XEphemeralStringPtrCreation()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strTest, L"Testing ephemeral string");

        xruntime_string_ptr strRuntime;
        VERIFY_SUCCEEDED(c_strTest.Promote(&strRuntime));

        xephemeral_string_ptr strEphemeral(c_strTest);
        VERIFY_ARE_STRINGS_EQUAL(L"Testing ephemeral string", strEphemeral);

        xephemeral_string_ptr strEphemeralDemotedFromConstant;
        c_strTest.Demote(&strEphemeralDemotedFromConstant);
        VERIFY_ARE_STRINGS_EQUAL(L"Testing ephemeral string", strEphemeralDemotedFromConstant);

        xephemeral_string_ptr strEphemeralDemotedFromRuntime;
        strRuntime.Demote(&strEphemeralDemotedFromRuntime);
        VERIFY_ARE_STRINGS_EQUAL(L"Testing ephemeral string", strEphemeralDemotedFromRuntime);

        xstring_ptr strPromotedFromEphemeralDemotedFromConstant;
        VERIFY_SUCCEEDED(strEphemeralDemotedFromConstant.Promote(&strPromotedFromEphemeralDemotedFromConstant));
        VERIFY_ARE_STRINGS_EQUAL(L"Testing ephemeral string", strPromotedFromEphemeralDemotedFromConstant);

        xstring_ptr strPromotedFromEphemeralDemotedFromRuntime;
        VERIFY_SUCCEEDED(strEphemeralDemotedFromRuntime.Promote(&strPromotedFromEphemeralDemotedFromRuntime));
        VERIFY_ARE_STRINGS_EQUAL(L"Testing ephemeral string", strPromotedFromEphemeralDemotedFromRuntime);

        xruntime_string_ptr strPromotedToRuntimeFromEphemeralDemotedFromConstant;
        VERIFY_SUCCEEDED(strEphemeralDemotedFromConstant.Promote(&strPromotedToRuntimeFromEphemeralDemotedFromConstant));
        VERIFY_ARE_STRINGS_EQUAL(L"Testing ephemeral string", strPromotedToRuntimeFromEphemeralDemotedFromConstant);

        xruntime_string_ptr strPromotedToRuntimeFromEphemeralDemotedFromRuntime;
        VERIFY_SUCCEEDED(strEphemeralDemotedFromRuntime.Promote(&strPromotedToRuntimeFromEphemeralDemotedFromRuntime));
        VERIFY_ARE_STRINGS_EQUAL(L"Testing ephemeral string", strPromotedToRuntimeFromEphemeralDemotedFromRuntime);
    }

    void StringUnitTests::XRuntimeStringPtrCreation()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strTest, L"Testing runtime string");

        xruntime_string_ptr strRuntime;
        VERIFY_SUCCEEDED(c_strTest.Promote(&strRuntime));
        VERIFY_ARE_STRINGS_EQUAL(L"Testing runtime string", strRuntime);

        HSTRING hstrRuntime = strRuntime.GetHSTRING();
        VERIFY_IS_NOT_NULL(hstrRuntime);
        VERIFY_ARE_STRINGS_EQUAL(L"Testing runtime string", hstrRuntime);

        HSTRING hstrRuntimeDetached = strRuntime.DetachHSTRING();
        VERIFY_ARE_EQUAL(hstrRuntime, hstrRuntimeDetached);
        VERIFY_IS_TRUE(!!strRuntime.IsNull());

        xstring_ptr strClonedFromHSTRING;
        VERIFY_SUCCEEDED(xstring_ptr::CloneRuntimeStringHandle(hstrRuntimeDetached, &strClonedFromHSTRING));
        VERIFY_ARE_STRINGS_EQUAL(L"Testing runtime string", strClonedFromHSTRING);
        VERIFY_IS_FALSE(!!strClonedFromHSTRING.IsNull());

        strClonedFromHSTRING.Reset();
        VERIFY_IS_TRUE(!!strClonedFromHSTRING.IsNull());
    }

    void StringUnitTests::XEncodedStringPtrCreation()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strTest, L"Testing encoded string");
        xstring_ptr strTest(c_strTest);

        xencoded_string_ptr strEncoded = xstring_ptr::Encode(strTest);
        xstring_ptr strDecoded = xstring_ptr::Decode(strEncoded);
        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strDecoded);

        xencoded_string_ptr strMoveEncoded = xstring_ptr::MoveEncode(std::move(strTest));
        xstring_ptr strMoveDecoded = xstring_ptr::Decode(strMoveEncoded);
        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strMoveDecoded);

        VERIFY_IS_TRUE(!!strTest.IsNull());

        xruntime_string_ptr strDecodedAndPromotedToRuntime;
        VERIFY_SUCCEEDED(xruntime_string_ptr::DecodeAndPromote(strEncoded, &strDecodedAndPromotedToRuntime));
        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strDecodedAndPromotedToRuntime);

        xstring_ptr strDecodedAndPromotedToRuntimeAsXString;
        VERIFY_SUCCEEDED(strDecodedAndPromotedToRuntime.Promote(&strDecodedAndPromotedToRuntimeAsXString));
        xencoded_string_ptr strMoveEncodedFromRuntime = xstring_ptr::MoveEncode(std::move(strDecodedAndPromotedToRuntimeAsXString));
        VERIFY_IS_TRUE(!!strDecodedAndPromotedToRuntimeAsXString.IsNull());
        xstring_ptr strMoveDecodedFromRuntime = xstring_ptr::Decode(strMoveEncodedFromRuntime);
        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strMoveDecodedFromRuntime);

        xruntime_string_ptr strMoveDecodedAndPromotedFromRuntime;
        VERIFY_SUCCEEDED(xruntime_string_ptr::DecodeAndPromote(strMoveEncodedFromRuntime, &strMoveDecodedAndPromotedFromRuntime));
        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strMoveDecodedAndPromotedFromRuntime);

        xephemeral_string_ptr strDecodedAsEphemeral;
        xephemeral_string_ptr::Decode(strEncoded, &strDecodedAsEphemeral);
        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strDecodedAsEphemeral);

        xephemeral_string_ptr strDecodedFromRuntimeAsEphemeral;
        xephemeral_string_ptr::Decode(strMoveEncodedFromRuntime, &strDecodedFromRuntimeAsEphemeral);
        VERIFY_ARE_STRINGS_EQUAL(c_strTest, strDecodedFromRuntimeAsEphemeral);
    }

    void StringUnitTests::XStringPtrConcatenate()
    {
        xstring_ptr str_TestingString;
        VERIFY_SUCCEEDED(xstring_ptr::Concatenate(c_str_Testing, 0, c_str_String, 0, &str_TestingString));
        VERIFY_ARE_STRINGS_EQUAL(L"TestingString", str_TestingString);

        xstring_ptr str_TestingRING;
        VERIFY_SUCCEEDED(xstring_ptr::Concatenate(c_str_Testing, 0, c_str_stRING, 2, &str_TestingRING));
        VERIFY_ARE_STRINGS_EQUAL(L"TestingRING", str_TestingRING);

        xstring_ptr str_stingRING;
        VERIFY_SUCCEEDED(xstring_ptr::Concatenate(c_str_Testing, 2, c_str_stRING, 2, &str_stingRING));
        VERIFY_ARE_STRINGS_EQUAL(L"stingRING", str_stingRING);
    }

    void StringUnitTests::XStringPtrFind()
    {
        UINT ichFound = 0;

        xstring_ptr str_TestingString;
        VERIFY_SUCCEEDED(xstring_ptr::Concatenate(c_str_Testing, 0, c_str_String, 0, &str_TestingString));

        VERIFY_SUCCEEDED(str_TestingString.Find(c_str_ring, 0, xstrCompareCaseSensitive, &ichFound));
        VERIFY_ARE_EQUAL(ichFound, (UINT)9);
        ichFound = 0;

        VERIFY_FAILED(str_TestingString.Find(c_str_rINg, 0, xstrCompareCaseSensitive, &ichFound));
        VERIFY_ARE_EQUAL(ichFound, (UINT)0);
        ichFound = 0;

        VERIFY_SUCCEEDED(str_TestingString.Find(c_str_rINg, 0, xstrCompareCaseInsensitive, &ichFound));
        VERIFY_ARE_EQUAL(ichFound, (UINT)9);
        ichFound = 0;

        VERIFY_FAILED(str_TestingString.Find(c_str_rINg, UINT32_MAX - 1, xstrCompareCaseInsensitive, &ichFound));
        VERIFY_ARE_EQUAL(ichFound, (UINT)0);
        ichFound = 0;

        VERIFY_FAILED(str_TestingString.Find(c_str_rINg, UINT32_MAX - c_str_rINg.GetCount() - 1, xstrCompareCaseInsensitive, &ichFound));
        VERIFY_ARE_EQUAL(ichFound, (UINT)0);
        ichFound = 0;

        VERIFY_FAILED(str_TestingString.Find(c_str_rINg, str_TestingString.GetCount() - 1, xstrCompareCaseInsensitive, &ichFound));
        VERIFY_ARE_EQUAL(ichFound, (UINT)0);
        ichFound = 0;

        VERIFY_ARE_EQUAL(str_TestingString.FindChar(L'n'), 5u);
        VERIFY_ARE_EQUAL(str_TestingString.FindLastChar(L'n'), 11u);

        VERIFY_ARE_EQUAL(str_TestingString.FindChar(L'n', 6), 11u);
        VERIFY_ARE_EQUAL(str_TestingString.FindChar(L'n', str_TestingString.GetCount() * 42), xstring_ptr_view::npos);

        VERIFY_ARE_EQUAL(str_TestingString.FindLastChar(L'n', str_TestingString.GetCount() * 42), 11u);
        VERIFY_ARE_EQUAL(str_TestingString.FindLastChar(L'n', 0), xstring_ptr_view::npos);
    }

    void StringUnitTests::XStringBuilderFind()
    {
        XStringBuilder str_TestingString;
        VERIFY_SUCCEEDED(str_TestingString.Append(c_str_Testing));
        VERIFY_SUCCEEDED(str_TestingString.Append(c_str_String));

        VERIFY_ARE_EQUAL(str_TestingString.FindChar(L'n'), 5u);
        VERIFY_ARE_EQUAL(str_TestingString.FindChar(L'n', 6), 11u);
        VERIFY_ARE_EQUAL(str_TestingString.FindChar(L'n', str_TestingString.GetCount() * 42), xstring_ptr_view::npos);
    }

    void StringUnitTests::XStringPtrStartsWithEndsWith()
    {
        xstring_ptr str_TestingString;
        VERIFY_SUCCEEDED(xstring_ptr::Concatenate(c_str_Testing, 0, c_str_String, 0, &str_TestingString));

        VERIFY_IS_TRUE(!!str_TestingString.StartsWith(c_str_Test, xstrCompareCaseSensitive));
        VERIFY_IS_TRUE(!!str_TestingString.StartsWith(c_str_Test, xstrCompareCaseInsensitive));
        VERIFY_IS_FALSE(!!str_TestingString.StartsWith(c_str_test, xstrCompareCaseSensitive));
        VERIFY_IS_TRUE(!!str_TestingString.StartsWith(c_str_test, xstrCompareCaseInsensitive));
        VERIFY_IS_FALSE(!!c_str_test.StartsWith(str_TestingString, xstrCompareCaseSensitive));

        VERIFY_IS_TRUE(!!c_str_String.EndsWith(c_str_ring, xstrCompareCaseSensitive));
        VERIFY_IS_TRUE(!!c_str_String.EndsWith(c_str_ring, xstrCompareCaseInsensitive));
        VERIFY_IS_FALSE(!!c_str_String.EndsWith(c_str_rINg, xstrCompareCaseSensitive));
        VERIFY_IS_TRUE(!!c_str_String.EndsWith(c_str_rINg, xstrCompareCaseInsensitive));
        VERIFY_IS_FALSE(!!c_str_test.EndsWith(str_TestingString, xstrCompareCaseSensitive));
    }

    void StringUnitTests::XStringPtrIsAllWhitespace()
    {
        VERIFY_IS_TRUE(!!c_strWhitespace.IsAllWhitespace());
        VERIFY_IS_FALSE(!!c_strNotWhitespace.IsAllWhitespace());
    }

    void StringUnitTests::XStringPtrCompare()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strTest, L"axrd");

        xephemeral_string_ptr strEphemeral;
        c_strTest.SubString(0, 1, &strEphemeral);

        VERIFY_ARE_EQUAL(-1, strEphemeral.Compare(c_str_ab));
        strEphemeral.Reset();
        c_strTest.SubString(0, 2, &strEphemeral);
        VERIFY_ARE_EQUAL(1, strEphemeral.Compare(c_str_a));

        xstring_ptr nullxstring;
        xstring_ptr nullxstring2;

        VERIFY_ARE_EQUAL(0, nullxstring.Compare(nullxstring));
        VERIFY_ARE_EQUAL(0, nullxstring.Compare(nullxstring2));
        VERIFY_ARE_EQUAL(2, c_str_ab.Compare(nullxstring));
        VERIFY_ARE_EQUAL(0, c_str_empty.Compare(c_str_empty2));
        VERIFY_ARE_EQUAL(1, c_str_a.Compare(c_str_empty));
        VERIFY_ARE_EQUAL(0, c_str_a.Compare(c_str_a2));
        VERIFY_ARE_EQUAL(-1, c_str_a.Compare(c_str_ab));
        VERIFY_ARE_EQUAL(1, c_str_ab.Compare(c_str_a));
        VERIFY_ARE_EQUAL(L'B' - L'b', c_str_ab.Compare(c_str_aB));
        VERIFY_ARE_EQUAL(L'b' - L'B', c_str_aB.Compare(c_str_ab));
        VERIFY_ARE_EQUAL(0, c_str_ab.Compare(c_str_aB, xstrCompareCaseInsensitive));
        VERIFY_ARE_EQUAL(0, c_str_abc.Compare(c_str_abc_2, xstrCompareCaseSensitive));

        VERIFY_ARE_EQUAL(0, c_str_empty.Compare(c_str_empty2, 2, c_str_empty2.GetCount()));
        VERIFY_ARE_EQUAL(-2, c_str_empty.Compare(c_str_ab, 2, c_str_ab.GetCount()));
        VERIFY_ARE_EQUAL(-1, c_str_ab.Compare(c_str_bc, 42, 1));
        VERIFY_ARE_EQUAL(0, c_str_abc.Compare(c_str_bc, 0, 0));
        VERIFY_ARE_EQUAL(0, c_str_ab.Compare(nullxstring, 2, 2));
        VERIFY_ARE_EQUAL(1, c_str_ab.Compare(nullxstring, 1, 2));
        VERIFY_ARE_EQUAL(0, nullxstring.Compare(nullxstring2,0,1));
        VERIFY_ARE_EQUAL(0, nullxstring.Compare(nullxstring2, 1, 1));

        VERIFY_ARE_EQUAL(0, c_str_a.Compare(c_str_a2, 0, c_str_a2.GetCount()));
        VERIFY_ARE_EQUAL(0, c_str_a.Compare(c_str_a2, 0, 2));

        strEphemeral.Reset();
        c_str_abc_2.SubString(0, 2, &strEphemeral);
        VERIFY_ARE_EQUAL(0, strEphemeral.Compare(c_str_ab, 0, 2));
        VERIFY_ARE_EQUAL(0, strEphemeral.Compare(c_str_ab, 0, 3));

        VERIFY_ARE_EQUAL(0, c_str_abc.Compare(c_str_abc_2, 0, 5));
        VERIFY_ARE_EQUAL(L'a'-'c', c_str_abc.Compare(c_str_abc_2, 2, 5));
        VERIFY_ARE_EQUAL(0, c_str_abc.Compare(c_str_bc, 1, 2));
        VERIFY_ARE_EQUAL(L'b' - L'B', c_str_aB.Compare(c_str_bc, 1, 2));

        VERIFY_ARE_EQUAL(-1, c_str_a.Compare(c_str_ab, 0, c_str_ab.GetCount()));
        VERIFY_ARE_EQUAL(-1, c_str_a.Compare(c_str_ab, 0, 3));
        VERIFY_ARE_EQUAL(1, c_str_ab.Compare(c_str_a, 0, c_str_ab.GetCount()));
        VERIFY_ARE_EQUAL(1, c_str_ab.Compare(c_str_a, 0, 3));
        VERIFY_ARE_EQUAL(-2, c_str_a.Compare(c_str_abc, 0, c_str_abc.GetCount()));
        VERIFY_ARE_EQUAL(-1, c_str_a.Compare(c_str_abc, 0, 2));

        VERIFY_ARE_EQUAL(L'B' - L'b', c_str_ab.Compare(c_str_aB, 0, c_str_aB.GetCount()));
        VERIFY_ARE_EQUAL(0, c_str_ab.Compare(c_str_aB, 0, c_str_aB.GetCount(), xstrCompareCaseInsensitive));
        VERIFY_ARE_EQUAL(L'a' - L'b', c_str_ab.Compare(c_str_aB, 1, c_str_aB.GetCount()));
        VERIFY_ARE_EQUAL(0, c_str_ab.Compare(c_str_aB, 0, 1));

        VERIFY_ARE_EQUAL(L'B' - L'b', c_str_abc.Compare(c_str_Bc, 1, 2, xstrCompareCaseSensitive));
        VERIFY_ARE_EQUAL(0, c_str_abc.Compare(c_str_Bc, 1, 2, xstrCompareCaseInsensitive));
    }

    void StringUnitTests::XStringPtrSubString()
    {
        xephemeral_string_ptr strEphemeralSubstring;
        c_str_abc.SubString(1, 2, &strEphemeralSubstring);
        VERIFY_ARE_STRINGS_NOT_EQUAL(c_str_b, strEphemeralSubstring);

        xstring_ptr strNonEphemeralSubstring;
        VERIFY_SUCCEEDED(c_str_abc.SubString(1, 2, &strNonEphemeralSubstring));
        VERIFY_ARE_STRINGS_EQUAL(c_str_b, strNonEphemeralSubstring);
    }

    void StringUnitTests::XStringPtrWideCharToChar()
    {
        UINT32 c8BitBuffer = 0;

        char* p8BitBuffer = c_str_a.WideCharToChar(&c8BitBuffer);
        VERIFY_IS_NULL(p8BitBuffer);
        VERIFY_ARE_EQUAL((UINT)1, c8BitBuffer);

        p8BitBuffer = c_str_a.WideCharToChar();
        VERIFY_ARE_EQUAL('a', p8BitBuffer[0]);
    }

    void StringUnitTests::XStringPtrMakeBufferCopy()
    {
        LPCWSTR pBufferCopy = c_str_abc.MakeBufferCopy();
        VERIFY_IS_NOT_NULL(pBufferCopy);
        VERIFY_ARE_EQUAL((UINT)3, wcslen(pBufferCopy));
        VERIFY_ARE_STRINGS_EQUAL(pBufferCopy, c_str_abc);
    }

    void StringUnitTests::EnumFromString()
    {
        INT32 result = 0;
        VERIFY_SUCCEEDED(EnumerateFromString(ARRAY_SIZE(satZoomMode), satZoomMode, LEN_STR_PAIR(L"Disabled"), &result));
        VERIFY_ARE_EQUAL(static_cast<DirectUI::ZoomMode>(result), DirectUI::ZoomMode::Disabled);

        VERIFY_SUCCEEDED(EnumerateFromString(ARRAY_SIZE(satZoomMode), satZoomMode, LEN_STR_PAIR(L"Enabled"), &result));
        VERIFY_ARE_EQUAL(static_cast<DirectUI::ZoomMode>(result), DirectUI::ZoomMode::Enabled);
    }

    void StringUnitTests::EnumFromStringWithLeadingWhitespace()
    {
        INT32 result = 0;
        VERIFY_SUCCEEDED(EnumerateFromString(ARRAY_SIZE(satZoomMode), satZoomMode, LEN_STR_PAIR(L" Disabled"), &result));
        VERIFY_ARE_EQUAL(static_cast<DirectUI::ZoomMode>(result), DirectUI::ZoomMode::Disabled);

        VERIFY_SUCCEEDED(EnumerateFromString(ARRAY_SIZE(satZoomMode), satZoomMode, LEN_STR_PAIR(L" Enabled"), &result));
        VERIFY_ARE_EQUAL(static_cast<DirectUI::ZoomMode>(result), DirectUI::ZoomMode::Enabled);
    }

    void StringUnitTests::FlagsEnumFromString()
    {
        INT32 result = 0;
        VERIFY_SUCCEEDED(FlagsEnumerateFromString(ARRAY_SIZE(satManipulationModes), satManipulationModes, LEN_STR_PAIR(L"TranslateX,Rotate"), &result));
        VERIFY_ARE_EQUAL(static_cast<DirectUI::ManipulationModes>(result), DirectUI::ManipulationModes::TranslateX | DirectUI::ManipulationModes::Rotate);

        VERIFY_SUCCEEDED(FlagsEnumerateFromString(ARRAY_SIZE(satHorizontalAlignment), satHorizontalAlignment, LEN_STR_PAIR(L"Stretch"), &result));
        VERIFY_ARE_EQUAL(static_cast<DirectUI::HorizontalAlignment>(result), DirectUI::HorizontalAlignment::Stretch);
    }

    void StringUnitTests::XStringPtrHashStability()
    {
        // Table-driven test: verify that GetHash() produces stable, known values
        // for a set of fixed strings. If this test breaks, the hash algorithm has
        // changed and all persisted or cached hash values (e.g. ResourceDictionary
		// keys stored in XBF) may be invalid.
        struct HashTestCase
        {
            const wchar_t* input;
            std::uint64_t expectedHash;
        };

        static const HashTestCase testCases[] =
        {
            { L"",                                      0x42BC986DC5EEC4D3ULL },  //  0: empty string
            { L"A",                                     0x83C8A1F77F3BE4D0ULL },  //  1: single char
            { L"ab",                                    0x3AA468A9F277A9AFULL },  //  2: 2 chars
            { L"foo",                                   0x84236FFA53C54A3FULL },  //  3: 3 chars
            { L"test",                                  0x297EACBEC0297683ULL },  //  4: 4 chars
            { L"Hello",                                 0x5571579D3FE1AA45ULL },  //  5: 5 chars
            { L"Widget",                                0xA41D130832BACBAFULL },  //  6: 6 chars
            { L"XAML",                                  0xC8020674B299B810ULL },  //  7: 4 chars uppercase
            { L"Controls",                              0x32E7BB34F6E9D457ULL },  //  8: 8 chars
            { L"Microsoft",                             0x7641E9E605C32839ULL },  //  9: 9 chars
            { L"StringHash",                            0x77CD22226184BE76ULL },  // 10: 10 chars
            { L"xstring_ptr_view",                      0x65273F7293519AC9ULL },  // 11: 16 chars with underscores
            { L"The quick brown fox",                   0x7F1629212EC9F18BULL },  // 12: 19 chars with spaces
            { L"Windows.UI.Xaml.Controls",              0x7259D8DC0E420FF4ULL },  // 13: 24 chars with dots
            { L"unordered_dense::detail",               0xC44D4B12BBFAC3B8ULL },  // 14: 23 chars with colons
            { L"Hash stability test string number 16",  0x7489E1D26B2D4D85ULL },  // 15: 36 chars
            { L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",  0x91C808CBFECC8545ULL },  // 16: 36 chars alphanumeric
            { L"\x00E9\x00F1\x00FC\x00E4\x00F6",        0xD9D55899F91B2A03ULL },  // 17: 5 Unicode chars (éñüäö)
            { L"Mix\x2603\x2764Unicode\x00AE",          0x246E5E25D5C73F9DULL },  // 18: mixed ASCII + Unicode
            { L"A slightly longer string that exercises more of the hash computation path for better coverage", 0x2DF67A57453DFF91ULL },  // 19: 93 chars
        };

        for (int i = 0; i < ARRAYSIZE(testCases); ++i)
        {
            xstring_ptr str;
            VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(testCases[i].input, static_cast<XUINT32>(wcslen(testCases[i].input)), &str));

            std::uint64_t actualHash = str.GetHash();

            WEX::Logging::Log::Comment(
                WEX::Common::String().Format(L"String[%d] = \"%s\", hash = 0x%016llX", i, testCases[i].input, actualHash));

            VERIFY_ARE_EQUAL(testCases[i].expectedHash, actualHash);
        }
    }

    // Jenkins one-at-a-time hash (original implementation from commit 7734731a).
    // This is the hash that xstring_ptr::GetHash() used before switching to wyhash.
    // Note: uses std::size_t (64-bit on x64) with shift constants tuned for 32-bit.
    static std::size_t JenkinsHash(const WCHAR* buffer, unsigned int length)
    {
        std::size_t hash = 0;
        for (unsigned int i = 0; i < length; ++i)
        {
            hash += buffer[i];
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
    }

    void StringUnitTests::XStringPtrHashBenchmark()
    {
        struct BenchmarkCase
        {
            const wchar_t* label;
            const wchar_t* input;
        };

        static const BenchmarkCase cases[] =
        {
            { L"Short  5ch", L"Width" },
            { L"Med   24ch", L"Windows.UI.Xaml.Controls" },
            { L"Long  93ch", L"A slightly longer string that exercises more of the hash computation path for better coverage" },
        };

        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);

        static const int warmupIterations = 10000;
        static const int measureIterations = 1000000;

        WEX::Logging::Log::Comment(L"=== xstring_ptr Hash Benchmark: wyhash (current) vs Jenkins (commit 7734731a) ===");
        WEX::Logging::Log::Comment(
            WEX::Common::String().Format(L"Iterations: %d (warmup: %d), QPC freq: %lld Hz",
                measureIterations, warmupIterations, freq.QuadPart));

        volatile std::uint64_t sink = 0;

        for (int c = 0; c < ARRAYSIZE(cases); ++c)
        {
            xstring_ptr str;
            VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(
                cases[c].input,
                static_cast<XUINT32>(wcslen(cases[c].input)),
                &str));

            unsigned int charLen;
            const WCHAR* buf = str.GetBufferAndCount(&charLen);

            // --- Benchmark wyhash (current GetHash) ---
            for (int i = 0; i < warmupIterations; ++i)
            {
                sink = str.GetHash();
            }

            LARGE_INTEGER startWy, endWy;
            QueryPerformanceCounter(&startWy);
            for (int i = 0; i < measureIterations; ++i)
            {
                sink = str.GetHash();
            }
            QueryPerformanceCounter(&endWy);

            double wySeconds = static_cast<double>(endWy.QuadPart - startWy.QuadPart) / freq.QuadPart;
            double wyNsPerCall = (wySeconds * 1.0e9) / measureIterations;
            double wyNsPerChar = wyNsPerCall / charLen;

            // --- Benchmark Jenkins (old hash) ---
            for (int i = 0; i < warmupIterations; ++i)
            {
                sink = JenkinsHash(buf, charLen);
            }

            LARGE_INTEGER startJk, endJk;
            QueryPerformanceCounter(&startJk);
            for (int i = 0; i < measureIterations; ++i)
            {
                sink = JenkinsHash(buf, charLen);
            }
            QueryPerformanceCounter(&endJk);

            double jkSeconds = static_cast<double>(endJk.QuadPart - startJk.QuadPart) / freq.QuadPart;
            double jkNsPerCall = (jkSeconds * 1.0e9) / measureIterations;
            double jkNsPerChar = jkNsPerCall / charLen;

            double ratio = jkNsPerCall / wyNsPerCall;

            WEX::Logging::Log::Comment(
                WEX::Common::String().Format(
                    L"[%s] wyhash: %6.1f ns/call %5.2f ns/char | Jenkins: %6.1f ns/call %5.2f ns/char | speedup: %.2fx",
                    cases[c].label,
                    wyNsPerCall, wyNsPerChar,
                    jkNsPerCall, jkNsPerChar,
                    ratio));
        }

        // Use sink to prevent optimization
        WEX::Logging::Log::Comment(
            WEX::Common::String().Format(L"(sink = 0x%016llX)", static_cast<std::uint64_t>(sink)));
    }

} } } } }
