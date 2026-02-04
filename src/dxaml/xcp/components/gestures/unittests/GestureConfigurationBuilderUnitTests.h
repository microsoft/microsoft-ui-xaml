// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Input { namespace Gestures {

class GestureConfigurationBuilderUnitTests : public WEX::TestClass < GestureConfigurationBuilderUnitTests >
{
public:
    BEGIN_TEST_CLASS(GestureConfigurationBuilderUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(VerifyGestureConfigurationBuilder)
    TEST_METHOD(VerifyGestureConfigurationBuilderForInteractionConfig)
    TEST_METHOD(VerifyGestureConfigurationBuilderForInteractionConfigPartial)

    TEST_METHOD(VerifyGestureSettingsToInteractionConfigManipulation)
    TEST_METHOD(VerifyGestureSettingsToInteractionConfigTap)
    TEST_METHOD(VerifyGestureSettingsToInteractionConfigRightTap)
    TEST_METHOD(VerifyGestureSettingsToInteractionConfigHold)
};


} } } } } }
