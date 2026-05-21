// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace ValueBoxer {
        class BasicValueBoxerUnitTests : public WEX::TestClass<BasicValueBoxerUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicValueBoxerUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)

            TEST_METHOD(Validate_CValueBoxer_CopyValue)
            TEST_METHOD(Validate_CValueBoxer_BoxUnboxSimple)
            TEST_METHOD(Validate_CValueBoxer_BoxUnboxTextRange)
            TEST_METHOD(Validate_CValueBoxer_BoxUnboxEnum)
            TEST_METHOD(Validate_CValueBoxer_BoxUnboxDuration)
            TEST_METHOD(Validate_CValueBoxer_BoxIFontFamily)
            TEST_METHOD(Validate_CValueBoxer_BoxUnboxViaIReference)
            TEST_METHOD(Validate_CValueBoxer_ConvertToFramework)

            TEST_METHOD(Validate_IValueBoxer_BoxUnboxSimple)
        };
    }
} } } }
