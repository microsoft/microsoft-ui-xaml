// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include <FeatureFlags.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Elements {

class UIElementUnitTests : public WEX::TestClass<UIElementUnitTests>
{
public:
    BEGIN_TEST_CLASS(UIElementUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(ValidateGetLocalTransform_Xaml)

    TEST_METHOD(ValidateGetLocalTransform_Hybrid)

    BEGIN_TEST_METHOD(ValidateInvalidateViewport)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateEffectiveViewportWalk)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateComputeUnidimensionalEffectiveViewport)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateComputeUnidimensionalMaxViewport)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateComputeUnidimensionalBringIntoViewDistance)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHideElement)
        TEST_METHOD_PROPERTY(L"FeatureUnderTest",
            L"@ClassName = 'CUIElement' AND ("
            L"@FunctionName = 'ValidateHideElement')")
    END_TEST_METHOD()
};

} } } } } }
