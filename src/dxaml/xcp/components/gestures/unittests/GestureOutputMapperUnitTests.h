// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include <Versioning.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Gestures {

class GestureOutputMapperUnitTests : public WEX::TestClass < GestureOutputMapperUnitTests >
{
public:
    BEGIN_TEST_CLASS(GestureOutputMapperUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(VerifyTap)
    TEST_METHOD(VerifySecondaryTap)

    TEST_METHOD(VerifyPointerInputTypeReflectedInMessage)

    TEST_METHOD(VerifyHolding)
    TEST_METHOD(VerifyHoldingStateStarted)
    TEST_METHOD(VerifyHoldingStateCompleted)
    TEST_METHOD(VerifyHoldingStateCanceled)

    TEST_METHOD(VerifySendManipulationStartingEventAfterGesture)
    TEST_METHOD(VerifyCallbackNotCalled)

    TEST_METHOD(VerifyManipulationStarted)

    TEST_METHOD(VerifyManipulationUpdated)
    TEST_METHOD(VerifyOnManipulationInertiaStarting)
    TEST_METHOD(VerifyManipulationInertiaStarting)
    TEST_METHOD(VerifyManipulationInertiaStartingFiresOnce)
    TEST_METHOD(VerifyManipulationUpdateIsInertiaUntilManipulationComplete)

    TEST_METHOD(VerifyManipulationCompleted)
};


} } } } }
