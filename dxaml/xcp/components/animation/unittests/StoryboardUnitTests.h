// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include "CDependencyObject.h"
#include "MinPal.h"
#include "TypeTableStructs.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

class StoryboardUnitTests : public WEX::TestClass<StoryboardUnitTests>
{
public:
    BEGIN_TEST_CLASS(StoryboardUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(SimpleStoryboards)
    TEST_METHOD(SimpleStoryboardsWithProperties)
    TEST_METHOD(StoryboardsWithDuration)
    TEST_METHOD(NestedStoryboards)
    TEST_METHOD(ProgressRingStoryboard)
    TEST_METHOD(ProgressRingStoryboardKeyFrames)
};

} } } } } }
