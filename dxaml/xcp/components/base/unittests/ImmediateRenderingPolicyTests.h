// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace ImmediateRenderingPolicyTests
{
    class PolicyTests
    {
        BEGIN_TEST_CLASS(PolicyTests)
        END_TEST_CLASS()

        TEST_METHOD(PlainElementDoesNotFallBack);
        TEST_METHOD(InteropTriggersFallBack);
        TEST_METHOD(BuiltInProgressRingHandInDoesNotFallBack);
        TEST_METHOD(UntouchedHandOffVisualIsPaintable);
        TEST_METHOD(TouchedHandOffVisualIsDetected);
        TEST_METHOD(HandOffVisualTransformFollowsCompositionOrder);
        TEST_METHOD(RelativeGradientMapsToBounds);
        TEST_METHOD(AbsoluteGradientIsUnchanged);
        TEST_METHOD(GradientTransformsAreApplied);
        TEST_METHOD(DegenerateGradientIsRejected);
        TEST_METHOD(ThemeShadowFollowsRecipe);
        TEST_METHOD(BlurInflationCoversOffset);
        TEST_METHOD(AcrylicLayersMatchMuxc);
        TEST_METHOD(OpaqueCoverRequiresFullContainment);
        TEST_METHOD(ProgressRingArc);
        TEST_METHOD(BackdropDamageExpands);
        TEST_METHOD(RoundedRectanglePathIsClosed);
        TEST_METHOD(ArcPathEndsAtSweep);
        // ---- scrolling ----
        TEST_METHOD(ScrollBlitNeedsWholePixelTranslation);
        TEST_METHOD(ScrollBlitRefusesZoomChange);
        TEST_METHOD(ScrollerChangeClassification);
        // ---- DManip ticking (scroll physics) ----
        TEST_METHOD(DManipTickSettingParsing);
        TEST_METHOD(DManipTickStartsWhenViewportMoves);
        TEST_METHOD(DManipTickStopsWhenIdle);
        TEST_METHOD(DManipTickingReplacesInertiaJumpAndTween);
        // ---- scrollgaps ----
        TEST_METHOD(DManipUpdatesRightAfterHandledInput);
        TEST_METHOD(BlitDamageSkipsUniformInterior);
        TEST_METHOD(DamageCoalescingKeepsThinStripsThin);
        TEST_METHOD(UnboundedClipEdgeIgnoresScrollOffset);
        TEST_METHOD(OffThreadScrollIsOptIn);
        TEST_METHOD(OffThreadOverscanGrowsViewport);
        TEST_METHOD(OffThreadInertiaExtrapolates);
        // ---- imaging ----
        TEST_METHOD(AppCompositionBrushFallsBack);
        TEST_METHOD(NineGridCellsMapInsets);
        TEST_METHOD(NineGridOverlappingInsetsShrink);
    };
}
