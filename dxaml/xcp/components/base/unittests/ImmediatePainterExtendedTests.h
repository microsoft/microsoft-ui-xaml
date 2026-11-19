// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // Backend-neutral contract for the extended painter primitives (scenes S3-S10): rounded clips, paths, DirectWrite
    // glyph runs, bitmaps, and ScrollPixels. Run once per immediate mode; red until each backend implements them, and
    // a D2D-HW row is Skipped (not passed) on machines without a hardware adapter.
    class ExtendedPainterConformanceTests
    {
        BEGIN_TEST_CLASS(ExtendedPainterConformanceTests)
            TEST_CLASS_PROPERTY(L"Data:Mode", L"{gdiplus,d2d-hw,d2d-sw}")
        END_TEST_CLASS()

        // Rounded-rectangle clips
        TEST_METHOD(RoundedClipRestrictsDrawingToRoundedRect);
        TEST_METHOD(RoundedClipNestsWithAxisAlignedClipAndOpacityLayer);

        // Paths
        TEST_METHOD(FillPathFillsTriangle);
        TEST_METHOD(FillPathHonorsFillRule);
        TEST_METHOD(FillPathFillsCubicCurve);
        TEST_METHOD(StrokePathIsCenteredOnPath);
        TEST_METHOD(FillPathCacheKeyReuseIsStable);
        TEST_METHOD(InvalidPathDataIsNoOp);

        // Glyph runs
        TEST_METHOD(GlyphRunInkStaysWithinRunBounds);
        TEST_METHOD(GlyphRunEmptyOrNullFontFaceIsNoOp);
        TEST_METHOD(GlyphRunRightToLeftExtendsLeftOfOrigin);

        // Bitmaps
        TEST_METHOD(BitmapDrawsOneToOne);
        TEST_METHOD(BitmapScalesWithLinearFiltering);
        TEST_METHOD(BitmapSourceRectSelectsSubRegion);
        TEST_METHOD(BitmapOpacityScalesPremultipliedValues);
        TEST_METHOD(BitmapUploadsOncePerGeneration);
        TEST_METHOD(BitmapReleaseFreesCacheAndDisablesDraw);
        TEST_METHOD(BitmapUnknownKeyIsNoOp);

        // Scrolling
        TEST_METHOD(ScrollPixelsMovesContentVertically);
        TEST_METHOD(ScrollPixelsMovesContentHorizontally);
        TEST_METHOD(ScrollPixelsFailsInsideFrame);
        TEST_METHOD(ScrollPixelsPresentedAreaCountsAsUpload);

        // Stats
        TEST_METHOD(ExtendedPrimitivesCountInStats);
    };
}
