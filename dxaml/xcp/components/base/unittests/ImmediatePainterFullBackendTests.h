// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // Backend-neutral contract for the full-backend painter primitives (scenes S11-S23): linear gradient fills, rounded
    // rectangle drop shadows, in-place backdrop blur, and the glyph-run hints (color fonts, ClearType) plus GDI+
    // fallback fonts (CFF, variable). Run once per immediate mode; a D2D-HW row is Skipped (not passed) on machines
    // without a hardware adapter.
    class FullBackendPainterConformanceTests
    {
        BEGIN_TEST_CLASS(FullBackendPainterConformanceTests)
            TEST_CLASS_PROPERTY(L"Data:Mode", L"{gdiplus,d2d-hw,d2d-sw}")
        END_TEST_CLASS()

        // Gradient fills
        TEST_METHOD(GradientFillHorizontalTwoStops);
        TEST_METHOD(GradientFillVerticalTwoStops);
        TEST_METHOD(GradientFillClampsBeyondEndPoints);
        TEST_METHOD(GradientFillThreeStops);
        TEST_METHOD(GradientFillOpacityScalesColor);
        TEST_METHOD(GradientFillRoundedCornersAreAntialiased);
        TEST_METHOD(GradientFillRespectsTransform);
        TEST_METHOD(GradientFillRespectsClipAndDamage);
        TEST_METHOD(GradientPathFillsTriangle);
        TEST_METHOD(GradientFillsCountInStats);

        // Drop shadows
        TEST_METHOD(ShadowIsDarkerOnOffsetSide);
        TEST_METHOD(ShadowFalloffIsMonotonicAndVanishesBeyondThreeSigma);
        TEST_METHOD(ShadowKnockoutLeavesShapeAreaUntouched);
        TEST_METHOD(ShadowColorAndAlphaAreRespected);
        TEST_METHOD(ShadowZeroBlurIsHardEdgeOffsetShape);
        TEST_METHOD(ShadowRespectsTransformAndDamage);

        // Backdrop blur
        TEST_METHOD(BlurBackdropSoftensEdgeInsideRect);
        TEST_METHOD(BlurBackdropLeavesPixelsOutsideRoundedRectUnchanged);
        TEST_METHOD(BlurBackdropZeroRadiusIsNoOp);
        TEST_METHOD(BlurBackdropKeepsUniformRegionUniform);
        TEST_METHOD(BlurBackdropRespectsTranslation);
        TEST_METHOD(BlurBackdropRespectsDamage);

        // Glyph-run hints and fallback fonts
        TEST_METHOD(ColorGlyphRunDrawsColorLayers);
        TEST_METHOD(ColorGlyphRunIsMonochromeWhenColorFontDisabled);
        TEST_METHOD(ClearTypeGlyphRunOnOpaqueBackgroundDrawsInk);
        TEST_METHOD(ClearTypeGlyphRunOnTransparentTargetFallsBackToGrayscale);
        TEST_METHOD(GlyphRunWithCffFontDrawsInk);
        TEST_METHOD(GlyphRunWithVariableFontDrawsInk);

        // ---- text ----
        // InvertRectangle (CompositeMode DestInvert caret)
        TEST_METHOD(InvertRectangleInvertsOpaquePixels);
        TEST_METHOD(InvertRectangleTwiceRestoresPixels);
        TEST_METHOD(InvertRectangleWithTranslucentSourceBlends);
        TEST_METHOD(InvertRectangleAntialiasesFractionalEdges);
        TEST_METHOD(InvertRectangleRespectsTransformClipAndDamage);
        TEST_METHOD(InvertRectangleCountsInStats);
    };
}
