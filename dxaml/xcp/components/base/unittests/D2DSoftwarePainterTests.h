// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // D2D-SW (Direct2D on WARP) behavior that the backend-neutral PainterConformanceTests do not cover. The
    // "Infrastructure" tests check the test seams themselves.
    class D2DSoftwarePainterTests
    {
        BEGIN_TEST_CLASS(D2DSoftwarePainterTests)
        END_TEST_CLASS()

        // Infrastructure.
        TEST_METHOD(RejectsInvalidArguments);
        TEST_METHOD(WarpSupportsCompositionSwapChain);
        TEST_METHOD(DeviceRemovalCanBeSimulatedOnWarp);

        // WARP output is a reproducible golden reference for C1.
        TEST_METHOD(TwoWarpDevicesProduceIdenticalPixels);
        // Steady-state frames reuse brushes, text formats, and geometries.
        TEST_METHOD(RepeatedFrameReusesResources);
        // The painter draws with the caller's WARP device and reports its identity.
        TEST_METHOD(UsesCallerDeviceAndReportsItsIdentity);
        // W5 copy accounting (contract in D2DPainter.cpp).
        TEST_METHOD(OffscreenFrameReportsNoUpload);
        TEST_METHOD(PresentedFrameUploadsDamagedPixels);
        TEST_METHOD(AttachedSwapChainBackBufferIsTheTarget);
        TEST_METHOD(PresentedUploadIsClippedToTarget);
        // C4: removal is reported as failure, not a crash.
        TEST_METHOD(ReportsFailureAfterDeviceRemoval);
        // Multi-rect damage: Clear replaces (even with transparent colors) only inside the rects; the mask is reused.
        TEST_METHOD(MultiRectDamageReplacesOnlyDamagedPixels);
        // The presentation swap chain must be on the painter's device.
        TEST_METHOD(RejectsSwapChainOnAnotherDevice);
        // Resize resizes the attached swap chain's buffers and fully damages the next frame.
        TEST_METHOD(ResizeResizesAttachedSwapChain);

        // Extended primitives (S3-S10).
        // Rounded clips are layerless layers with a cached rounded-rectangle mask; they nest with the other entries.
        TEST_METHOD(RoundedClipUsesCachedLayerMask);
        // Steady-state frames with paths, glyph runs, rounded clips, and bitmaps create nothing.
        TEST_METHOD(ExtendedFrameReusesResources);
        // EnsureBitmap copies count once, in the right frame; refreshes reuse the bitmap when the size is unchanged.
        TEST_METHOD(BitmapUploadAccounting);
        // ScrollPixels moves pixels in the target and presents the moved area with the next frame.
        TEST_METHOD(ScrollPixelsPresentsMovedArea);
    };
}
