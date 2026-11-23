// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // F10 (transitions): XAML-owned animations evaluated by XAML in immediate modes.
    class ImmediateAnimationsTests
    {
        BEGIN_TEST_CLASS(ImmediateAnimationsTests)
        END_TEST_CLASS()

        TEST_METHOD(PropertyTransitionCurveMatchesCompositionBezier);
        TEST_METHOD(DurationIsClampedLikeComposition);
        TEST_METHOD(TransitionRunsFromStaticValueAndEnds);
        TEST_METHOD(RetargetHandsOffFromAnimatedValue);
        TEST_METHOD(DisabledComponentsSnapToFinalValue);
        TEST_METHOD(NoOpTransitionIsNotStored);
        TEST_METHOD(BrushTransitionInterpolatesArgbLinearly);
        TEST_METHOD(CancelAndRemoveOwnerDropEntries);
        TEST_METHOD(ConnectedFrameInterpolatesOffsetScaleAndInsets);
        TEST_METHOD(GravityDipsAndReturns);
    };
}
