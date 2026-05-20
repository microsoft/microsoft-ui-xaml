// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {

class AKAccessKeyUnitTests : public WEX::TestClass<AKAccessKeyUnitTests>
{
public:
    BEGIN_TEST_CLASS(AKAccessKeyUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(VerifyEqualsMethodOnIdenticalAccessKeys)
        TEST_METHOD_PROPERTY(L"Description", L"Verify the Equals method returns true when two access keys have identical access key characters specified")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyEqualsMethodOnDifferentAccessKeys)
        TEST_METHOD_PROPERTY(L"Description", L"Verify the Equals method returns false when two access keys have different access key characters specified")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyAccessKeyAssignmentOperator)
        TEST_METHOD_PROPERTY(L"Description", L"Verify the assignment operator (=)")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CorrectlyMatchMixedCaseUnicodeKeys)
        TEST_METHOD_PROPERTY(L"Description", L"Validates AKAccessKeys defined with different case, but otherwise identical, input match.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanCorrectlyDeterminPartialMatches)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that IsPartialMatch returns true when the first non-null characters match the argument's first characters.")
    END_TEST_METHOD()
};

} } } } }
