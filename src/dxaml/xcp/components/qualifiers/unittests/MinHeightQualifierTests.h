// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        class MinHeightQualifierTests : public WEX::TestClass<MinHeightQualifierTests>
        {
        public:
            BEGIN_TEST_CLASS(MinHeightQualifierTests)
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(IsQualified)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Tests IsQualified function.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScoreQualifierFlags)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Tests Score(QualifierFlags) function.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Evaluate)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Tests Evaluate function.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Flags)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Tests Flags function")
            END_TEST_METHOD()

        };
    }
} } } }


