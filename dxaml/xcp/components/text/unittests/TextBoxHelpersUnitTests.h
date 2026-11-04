// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Text {

        class TextBoxHelpersUnitTests : public WEX::TestClass<TextBoxHelpersUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(TextBoxHelpersUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Unit")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(NearZeroSelectionDimensionsRemainEmpty)
            END_TEST_METHOD()
        };
    }
} } } }
