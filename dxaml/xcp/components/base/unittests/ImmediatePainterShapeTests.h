// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // Backend-neutral contract for the shapes/brushes painter primitives (F1 shapes and strokes, F2 brushes): styled
    // strokes (dash arrays and offsets, start/end/dash caps including Triangle, line joins, miter limit) and solid,
    // linear and radial gradient brushes (spread modes, sRGB / linear-light interpolation, brush transforms) for
    // fills, strokes and glyph runs. Run once per immediate mode; D2D-HW is Skipped without a hardware adapter.
    class ShapeBrushPainterConformanceTests
    {
        BEGIN_TEST_CLASS(ShapeBrushPainterConformanceTests)
            TEST_CLASS_PROPERTY(L"Data:Mode", L"{gdiplus,d2d-hw,d2d-sw}")
        END_TEST_CLASS()

        // Stroke styles
        TEST_METHOD(DashArrayAlternatesOnAndOff);
        TEST_METHOD(DashOffsetShiftsPatternTowardStart);
        TEST_METHOD(OddDashArrayRepeats);
        TEST_METHOD(DashesScaleWithTransform);
        TEST_METHOD(StartAndEndCapsExtendBeyondEndPoints);
        TEST_METHOD(DashCapsApplyBetweenDashes);
        TEST_METHOD(ZeroLengthDashWithRoundCapDrawsDot);
        TEST_METHOD(LineJoinsShapeSharpCorner);
        TEST_METHOD(MiterLimitClipsSharpJoin);
        TEST_METHOD(DashedRectangleStartsAtTopLeftClockwise);
        TEST_METHOD(DashedRoundedRectangleStartsAfterTopLeftArc);
        TEST_METHOD(DashedEllipseStartsAtRightmostPointClockwise);

        // Brushes
        TEST_METHOD(SolidBrushMatchesColorPrimitives);
        TEST_METHOD(LinearGradientSpreadModes);
        TEST_METHOD(LinearGradientInterpolationModes);
        TEST_METHOD(LinearGradientBrushTransform);
        TEST_METHOD(RadialGradientCenterAndEdge);
        TEST_METHOD(RadialGradientSpreadModes);
        TEST_METHOD(RadialGradientEllipticRadii);
        TEST_METHOD(RadialGradientFocalOrigin);
        TEST_METHOD(GradientFillRespectsTransformClipAndDpi);
        TEST_METHOD(GradientStrokePaintsOnlyTheStroke);
        TEST_METHOD(GradientDashedStrokeKeepsGaps);
        TEST_METHOD(GradientGlyphRunInkFollowsGradient);
        TEST_METHOD(SolidBrushGlyphRunMatchesColorGlyphRun);
    };

    // GDI+ against D2D on WARP for whole scenes of styled strokes and gradients: the two painters must agree closely
    // (dash phase on closed figures, cap and join shapes, radial focal math, gradient text).
    class ShapeBrushCrossBackendTests
    {
        BEGIN_TEST_CLASS(ShapeBrushCrossBackendTests)
        END_TEST_CLASS()

        TEST_METHOD(StyledStrokeScenesMatch);
        TEST_METHOD(GradientScenesMatch);
    };
}
