// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Animation {

        class AnimationUnitTests : public WEX::TestClass<AnimationUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(AnimationUnitTests)
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(ValidateGetNaturalDurationFromKeyFrames)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Tests inferring the natural duration from an animation's key frames.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateGetNaturalDuration_DA)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Tests inferring the natural duration from a DoubleAnimation.")
            END_TEST_METHOD()

            // missing: ValidateGetNaturalDuration_DAUKF (DoubleAnimationUsingKeyFrames)
        };
    }}
} } } }
