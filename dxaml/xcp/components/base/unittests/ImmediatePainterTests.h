// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // Backend-neutral painter contract, run once per immediate mode. Red until each backend implements its painter;
    // a D2D-HW row is Skipped (not passed) on machines without a hardware adapter.
    class PainterConformanceTests
    {
        BEGIN_TEST_CLASS(PainterConformanceTests)
            TEST_CLASS_PROPERTY(L"Data:Mode", L"{gdiplus,d2d-hw,d2d-sw}")
        END_TEST_CLASS()

        TEST_METHOD(CreatesPainterWithExpectedDeviceIdentity);
        TEST_METHOD(ClearFillsTarget);
        TEST_METHOD(FillRectangleCoversExpectedPixels);
        TEST_METHOD(RoundedCornersAreAntialiased);
        TEST_METHOD(StrokeDrawsOnlyTheEdge);
        TEST_METHOD(TextProducesInkInsideLayout);
        TEST_METHOD(AxisAlignedClipRestrictsDrawing);
        TEST_METHOD(TransformTranslatesDrawing);
        TEST_METHOD(OpacityLayerUsesGroupSemantics);
        TEST_METHOD(DpiScaleMapsDipsToPixels);
        TEST_METHOD(FullFrameReportsStats);
        TEST_METHOD(PartialDamagePreservesOutsidePixels);
        TEST_METHOD(ResizeReallocatesTarget);
        TEST_METHOD(InventoryAccountsForTarget);
        TEST_METHOD(PaintTracedFrameReturnsStats);
    };

    class PainterFactoryTests
    {
        BEGIN_TEST_CLASS(PainterFactoryTests)
        END_TEST_CLASS()

        TEST_METHOD(CompositionIsNotAnImmediatePainter);
        TEST_METHOD(RejectsEmptyTarget);
        TEST_METHOD(WarpDeviceReportsWarpIdentity);
    };
}
