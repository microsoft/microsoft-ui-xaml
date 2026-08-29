// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

class DoubleAnimationUsingKeyFramesUnitTests : public WEX::TestClass<DoubleAnimationUsingKeyFramesUnitTests>
{
public:
    BEGIN_TEST_CLASS(DoubleAnimationUsingKeyFramesUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(ValidateLinearDoubleKeyFramesWUC)

    TEST_METHOD(ValidateSplineDoubleKeyFramesWUC)

    TEST_METHOD(ValidateInferDurationWUC)

private:
    void ValidateLinearDoubleKeyFrames();
    void ValidateSplineDoubleKeyFrames();
};

} } } } } }
