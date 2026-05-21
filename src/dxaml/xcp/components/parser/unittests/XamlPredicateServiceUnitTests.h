// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class XamlPredicateServiceUnitTests : public WEX::TestClass<XamlPredicateServiceUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(XamlPredicateServiceUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(VerifyCrackConditionalXmlns)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that XamlPredicateService correctly cracks a "
                L"conditional xmlns into its constituent substrings.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCrackConditionalPredicate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that XamlPredicateService correctly cracks a "
                L"conditional predicate string into the XamlType representing the predicate, and its argument string.")
        END_TEST_METHOD()
    };

} } } } }
