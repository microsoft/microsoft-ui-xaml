// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // GDI+-specific behavior that the backend-neutral PainterConformanceTests do not cover.
    class GdiPlusPainterTests
    {
        BEGIN_TEST_CLASS(GdiPlusPainterTests)
        END_TEST_CLASS()

        // GDI+ runtime lifetime.
        TEST_METHOD(RuntimeReferencesAreRefcounted);
        TEST_METHOD(PaintersShareOneRuntime);
        TEST_METHOD(LastPainterShutsDownRuntime);
        TEST_METHOD(PainterCanBeRecreatedAfterShutdown);

        // PARGB target.
        TEST_METHOD(HalfAlphaFillReadsBackPremultiplied);

        // Quality settings, asserted through pixels.
        TEST_METHOD(GeometryIsAntialiased);
        TEST_METHOD(GeometryUsesHalfPixelOffset);
        TEST_METHOD(TextUsesGrayscaleAntialiasing);

        // Leaks and churn.
        TEST_METHOD(SteadyFramesDoNotLeakGdiObjectsOrCache);
        TEST_METHOD(IdenticalFrameCreatesNoResources);

        // Upload/copy accounting and presentation.
        TEST_METHOD(DamageCopyBytesCountsClippedDamage);
        TEST_METHOD(PartialDamageUploadsOnlyDamagedPixels);
        TEST_METHOD(PresentationCopiesDamageIntoBackBuffer);
        TEST_METHOD(ResizeResizesAttachedSwapChain);
        TEST_METHOD(RejectsNonBgraSwapChain);

        // Extended primitives (GDI+ implementation details).
        TEST_METHOD(RoundedClipIsAntialiasedInsideOpacityLayer);
        TEST_METHOD(RoundedClipSharesStackWithClipsAndLayers);
        TEST_METHOD(SquarePixelAlignedRoundedClipNeedsNoLayer);
        TEST_METHOD(PathCacheIsSteadyAndBounded);
        TEST_METHOD(GlyphRunFontCacheIsSteadyAndBounded);
        TEST_METHOD(GlyphRunRightToLeftAdvancesLeftwards);
        TEST_METHOD(BitmapCacheCountsBytesAndUploads);
        TEST_METHOD(BitmapDrawsOneToOneWithClampedEdges);
        TEST_METHOD(ScrollPixelsMovesRowsWithinRect);
        TEST_METHOD(ScrollPixelsPresentsMovedArea);
    };

    class ResourceChurnEventTests
    {
        BEGIN_TEST_CLASS(ResourceChurnEventTests)
        END_TEST_CLASS()

        TEST_METHOD(TraceResourceChurnIsCallableWithoutListener);
    };
}
