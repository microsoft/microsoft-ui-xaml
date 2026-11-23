// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // Backend-neutral contract for the clips3d primitives: antialiased geometry clips (any path, under any 2D transform)
    // and perspective layers (content drawn flat, then composited through a 4x4 matrix). Run once per immediate mode; a
    // D2D-HW row is Skipped (not passed) on machines without a hardware adapter.
    class Clips3DPainterConformanceTests
    {
        BEGIN_TEST_CLASS(Clips3DPainterConformanceTests)
            TEST_CLASS_PROPERTY(L"Data:Mode", L"{gdiplus,d2d-hw,d2d-sw}")
        END_TEST_CLASS()

        TEST_METHOD(GeometryClipKeepsInsideAndRemovesOutside);
        TEST_METHOD(GeometryClipEdgeIsAntialiased);
        TEST_METHOD(GeometryClipRespectsRotation);
        TEST_METHOD(NestedGeometryClipsIntersect);
        TEST_METHOD(GeometryClipPopRestoresDrawing);
        TEST_METHOD(EmptyGeometryClipClipsEverything);

        TEST_METHOD(PerspectiveLayerIdentityMatchesFlatDrawing);
        TEST_METHOD(PerspectiveLayerAffineMatrixMapsCorners);
        TEST_METHOD(PerspectiveLayerProjectsWithDepth);
        TEST_METHOD(PerspectiveLayerRespectsPushTransformAndClip);
        TEST_METHOD(PerspectiveLayerRespectsDamage);
        TEST_METHOD(PerspectiveLayerRespectsDpiScale);
        TEST_METHOD(PerspectiveLayerNestsOpacityAndClips);
    };
}
