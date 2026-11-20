// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    class ImmediateIndependentTests
    {
        BEGIN_TEST_CLASS(ImmediateIndependentTests)
        END_TEST_CLASS()

        TEST_METHOD(MatrixInverseRoundTrips);
        TEST_METHOD(RotationMatchesQuarterTurnsExactly);
        TEST_METHOD(CompositeTransformMatchesComponentOrder);
        TEST_METHOD(ExpirationHonorsRepeatAutoReverseAndSpeed);
        TEST_METHOD(ClockHoldsOrStopsAtEnd);
        TEST_METHOD(AutoReverseInvertsOddIterations);
        TEST_METHOD(ChainGivesLeafNoTimeWhenParentStopped);
        TEST_METHOD(CurveSamplingInterpolatesAndClamps);
        TEST_METHOD(TrackRunsUntilExpirationThenHolds);
        TEST_METHOD(LayerDeltaRotatesAboutOrigin);
        TEST_METHOD(LayerDeltaIsIdentityWhenUnchanged);
        TEST_METHOD(DeviceBoundsFollowDelta);
        TEST_METHOD(ArcEndsOnCircle);
        TEST_METHOD(SimulationParsesEveryListAndLimit);
        TEST_METHOD(SimulationIgnoresMalformedTokens);
        TEST_METHOD(ScaleChangeDetection);
    };
}
