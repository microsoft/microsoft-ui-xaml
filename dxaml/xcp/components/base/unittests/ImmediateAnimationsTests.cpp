// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImmediateAnimationsTests.h"
#include <ImmediateAnimations.h>
#include <cmath>

namespace RenderingBackendTests
{
    using namespace ImmediateAnimations;

    namespace
    {
        bool Near(float a, float b, float epsilon = 1e-3f)
        {
            return std::fabs(a - b) <= epsilon;
        }

        int dummyOwnerA;
        int dummyOwnerB;
    }

    void ImmediateAnimationsTests::PropertyTransitionCurveMatchesCompositionBezier()
    {
        const Curve curve = Curve::PropertyTransition();
        VERIFY_ARE_EQUAL(0.0f, curve.Evaluate(0));
        VERIFY_ARE_EQUAL(1.0f, curve.Evaluate(1));

        // Points on the parametric curve with P1 = (0.8, 0), P2 = (0.2, 1).
        for (float t : { 0.1f, 0.3f, 0.5f, 0.7f, 0.9f })
        {
            const float u = 1 - t;
            const float x = 3 * u * u * t * 0.8f + 3 * u * t * t * 0.2f + t * t * t;
            const float y = 3 * u * t * t + t * t * t;
            VERIFY_IS_TRUE(Near(y, curve.Evaluate(x)));
        }

        // Ease-in-out: slow start, point symmetric about the midpoint, monotonic.
        VERIFY_IS_TRUE(curve.Evaluate(0.2f) < 0.05f);
        VERIFY_IS_TRUE(Near(0.5f, curve.Evaluate(0.5f)));
        VERIFY_IS_TRUE(Near(1 - curve.Evaluate(0.3f), curve.Evaluate(0.7f)));
        float previous = 0;
        for (int i = 1; i <= 100; ++i)
        {
            const float v = curve.Evaluate(i / 100.0f);
            VERIFY_IS_TRUE(v >= previous - 1e-5f);
            previous = v;
        }
        VERIFY_IS_TRUE(Near(0.37f, Curve::Linear().Evaluate(0.37f), 1e-6f));
    }

    void ImmediateAnimationsTests::DurationIsClampedLikeComposition()
    {
        VERIFY_ARE_EQUAL(1000LL, ClampDurationMicroseconds(0));
        VERIFY_ARE_EQUAL(300000LL, ClampDurationMicroseconds(3000000));
        VERIFY_ARE_EQUAL(24LL * 24 * 60 * 60 * 1000000, ClampDurationMicroseconds(INT64_MAX));
    }

    void ImmediateAnimationsTests::TransitionRunsFromStaticValueAndEnds()
    {
        TransitionStore store;
        const float from[4] = { 0.2f };
        const float to[4] = { 1.0f };
        store.Start(&dummyOwnerA, Channel::Opacity, from, to, 1, 1000, 5000, Curve::Linear());

        float value[4];
        VERIFY_IS_TRUE(store.TryGetValue(&dummyOwnerA, Channel::Opacity, 5000, value));
        VERIFY_IS_TRUE(Near(0.2f, value[0]));
        VERIFY_IS_TRUE(store.TryGetValue(&dummyOwnerA, Channel::Opacity, 5500, value));
        VERIFY_IS_TRUE(Near(0.6f, value[0]));
        VERIFY_IS_FALSE(store.TryGetValue(&dummyOwnerA, Channel::Rotation, 5500, value));
        VERIFY_IS_TRUE(store.Prune(5999));

        // Finished: the static value applies again and the entry is pruned.
        VERIFY_IS_FALSE(store.TryGetValue(&dummyOwnerA, Channel::Opacity, 6000, value));
        VERIFY_IS_FALSE(store.Prune(6000));
        VERIFY_IS_TRUE(store.Empty());
    }

    void ImmediateAnimationsTests::RetargetHandsOffFromAnimatedValue()
    {
        TransitionStore store;
        const float zero[4] = { 0, 0, 0 };
        const float hundred[4] = { 100, 0, 0 };
        store.Start(&dummyOwnerA, Channel::Translation, zero, hundred, 0x7, 1000, 0, Curve::Linear());

        // Retarget mid flight to 0: starts at the current animated value (40), not at the stale static value.
        const float staticValue[4] = { 100, 0, 0 };
        store.Start(&dummyOwnerA, Channel::Translation, staticValue, zero, 0x7, 1000, 400, Curve::Linear());
        VERIFY_ARE_EQUAL(1u, static_cast<uint32_t>(store.Size()));
        float value[4];
        VERIFY_IS_TRUE(store.TryGetValue(&dummyOwnerA, Channel::Translation, 400, value));
        VERIFY_IS_TRUE(Near(40, value[0]));
        VERIFY_IS_TRUE(store.TryGetValue(&dummyOwnerA, Channel::Translation, 900, value));
        VERIFY_IS_TRUE(Near(20, value[0]));
    }

    void ImmediateAnimationsTests::DisabledComponentsSnapToFinalValue()
    {
        TransitionStore store;
        const float from[4] = { 1, 1, 1 };
        const float to[4] = { 2, 3, 4 };
        store.Start(&dummyOwnerA, Channel::Scale, from, to, 0x1 /* X only */, 1000, 0, Curve::Linear());
        float value[4];
        VERIFY_IS_TRUE(store.TryGetValue(&dummyOwnerA, Channel::Scale, 500, value));
        VERIFY_IS_TRUE(Near(1.5f, value[0]));
        VERIFY_IS_TRUE(Near(3, value[1]));
        VERIFY_IS_TRUE(Near(4, value[2]));
    }

    void ImmediateAnimationsTests::NoOpTransitionIsNotStored()
    {
        TransitionStore store;
        const float v[4] = { 0.5f };
        store.Start(&dummyOwnerA, Channel::Opacity, v, v, 1, 1000, 0, Curve::Linear());
        VERIFY_IS_TRUE(store.Empty());

        // No enabled component: nothing animates (Vector3Transition.Components == 0).
        const float a[4] = { 0, 0, 0 };
        const float b[4] = { 5, 5, 5 };
        store.Start(&dummyOwnerA, Channel::Translation, a, b, 0, 1000, 0, Curve::Linear());
        VERIFY_IS_TRUE(store.Empty());
    }

    void ImmediateAnimationsTests::BrushTransitionInterpolatesArgbLinearly()
    {
        TransitionStore store;
        // Null -> red fades in from transparent red (newRgb & 0x00ffffff).
        const float from[4] = { 0, 255, 0, 0 };
        const float to[4] = { 255, 255, 0, 0 };
        store.Start(&dummyOwnerA, Channel::BackgroundColor, from, to, 0xF, 1000, 0, Curve::Linear());
        float value[4];
        VERIFY_IS_TRUE(store.TryGetValue(&dummyOwnerA, Channel::BackgroundColor, 250, value));
        VERIFY_IS_TRUE(Near(63.75f, value[0]));
        VERIFY_IS_TRUE(Near(255, value[1]));
        VERIFY_IS_TRUE(Near(0, value[2]));
    }

    void ImmediateAnimationsTests::CancelAndRemoveOwnerDropEntries()
    {
        TransitionStore store;
        const float from[4] = { 0, 0, 0, 0 };
        const float to[4] = { 1, 1, 1, 1 };
        store.Start(&dummyOwnerA, Channel::Opacity, from, to, 0xF, 1000, 0, Curve::Linear());
        store.Start(&dummyOwnerA, Channel::Rotation, from, to, 0xF, 1000, 0, Curve::Linear());
        store.Start(&dummyOwnerB, Channel::Opacity, from, to, 0xF, 1000, 0, Curve::Linear());
        VERIFY_ARE_EQUAL(3u, static_cast<uint32_t>(store.Size()));

        store.Cancel(&dummyOwnerA, Channel::Rotation);
        VERIFY_ARE_EQUAL(2u, static_cast<uint32_t>(store.Size()));
        store.RemoveOwner(&dummyOwnerA);
        VERIFY_IS_FALSE(store.HasOwner(&dummyOwnerA));
        VERIFY_IS_TRUE(store.HasOwner(&dummyOwnerB));
    }

    void ImmediateAnimationsTests::ConnectedFrameInterpolatesRectsAndScale()
    {
        ConnectedRects rects;
        rects.sourceX = 10; rects.sourceY = 20; rects.sourceWidth = 100; rects.sourceHeight = 50;
        rects.destX = 210; rects.destY = 120; rects.destWidth = 300; rects.destHeight = 100;

        ConnectedFrame start = EvaluateConnected(rects, 0);
        VERIFY_IS_TRUE(Near(10, start.offsetX) && Near(20, start.offsetY));
        VERIFY_IS_TRUE(Near(1, start.scaleX) && Near(1, start.scaleY) && Near(0, start.crossFade));

        ConnectedFrame mid = EvaluateConnected(rects, 0.5f);
        VERIFY_IS_TRUE(Near(110, mid.offsetX) && Near(70, mid.offsetY));
        VERIFY_IS_TRUE(Near(2, mid.scaleX) && Near(1.5f, mid.scaleY) && Near(0.5f, mid.crossFade));

        // At the end the scaled snapshot covers the destination rect exactly.
        ConnectedFrame end = EvaluateConnected(rects, 1);
        VERIFY_IS_TRUE(Near(rects.destWidth, rects.sourceWidth * end.scaleX));
        VERIFY_IS_TRUE(Near(rects.destHeight, rects.sourceHeight * end.scaleY));
        VERIFY_IS_TRUE(Near(210, end.offsetX) && Near(120, end.offsetY));
    }

    void ImmediateAnimationsTests::ConnectedFrameWithoutScaleCentersUnion()
    {
        ConnectedRects rects;
        rects.scaleEnabled = false;
        rects.sourceX = 0; rects.sourceY = 0; rects.sourceWidth = 100; rects.sourceHeight = 100;
        rects.destX = 500; rects.destY = 0; rects.destWidth = 200; rects.destHeight = 50;

        ConnectedFrame start = EvaluateConnected(rects, 0);
        VERIFY_IS_TRUE(Near(-50, start.offsetX) && Near(0, start.offsetY));
        VERIFY_IS_TRUE(Near(1, start.scaleX) && Near(1, start.scaleY));
        ConnectedFrame end = EvaluateConnected(rects, 1);
        VERIFY_IS_TRUE(Near(500, end.offsetX) && Near(-25, end.offsetY));
    }
}
