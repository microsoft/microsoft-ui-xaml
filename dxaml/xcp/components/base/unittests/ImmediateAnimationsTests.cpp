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

    void ImmediateAnimationsTests::ConnectedFrameInterpolatesOffsetScaleAndInsets()
    {
        ConnectedParams params;
        params.fromX = 10; params.fromY = 20; params.toX = 210; params.toY = 120;
        params.toScaleX = 3; params.toScaleY = 2;
        params.fromInsets[0] = 4; params.toInsets[0] = 0;
        params.fromInsets[3] = 0; params.toInsets[3] = 8;

        ConnectedFrame start = EvaluateConnected(params, Curve::Linear(), 0);
        VERIFY_IS_TRUE(Near(10, start.offsetX) && Near(20, start.offsetY));
        VERIFY_IS_TRUE(Near(1, start.scaleX) && Near(1, start.scaleY) && Near(0, start.crossFade));
        VERIFY_IS_TRUE(Near(4, start.insets[0]) && Near(0, start.insets[3]));

        ConnectedFrame mid = EvaluateConnected(params, Curve::Linear(), 0.5f);
        VERIFY_IS_TRUE(Near(110, mid.offsetX) && Near(70, mid.offsetY));
        VERIFY_IS_TRUE(Near(2, mid.scaleX) && Near(1.5f, mid.scaleY) && Near(0.5f, mid.crossFade));
        VERIFY_IS_TRUE(Near(2, mid.insets[0]) && Near(4, mid.insets[3]));
        VERIFY_IS_TRUE(Near(0, mid.liftZ));

        // The configuration curve drives every channel: Direct's (0.1,0.9)-(0.2,1) is far ahead of linear at 25%.
        const Curve direct = Curve::Bezier(0.1f, 0.9f, 0.2f, 1.0f);
        ConnectedFrame early = EvaluateConnected(params, direct, 0.25f);
        const float eased = direct.Evaluate(0.25f);
        VERIFY_IS_TRUE(eased > 0.75f);
        VERIFY_IS_TRUE(Near(10 + 200 * eased, early.offsetX) && Near(eased, early.crossFade));

        ConnectedFrame end = EvaluateConnected(params, direct, 1);
        VERIFY_IS_TRUE(Near(210, end.offsetX) && Near(120, end.offsetY));
        VERIFY_IS_TRUE(Near(3, end.scaleX) && Near(2, end.scaleY) && Near(1, end.crossFade));
    }

    void ImmediateAnimationsTests::GravityDipsAndReturns()
    {
        VERIFY_IS_TRUE(Near(0, EvaluateGravityDip(0)));
        VERIFY_IS_TRUE(Near(1, EvaluateGravityDip(0.66f)));
        VERIFY_IS_TRUE(Near(0, EvaluateGravityDip(1)));
        VERIFY_IS_TRUE(EvaluateGravityDip(0.33f) > 0 && EvaluateGravityDip(0.33f) < 1);

        // Same-size flight: the effect adds the dip to Offset.Y/Z and scales up by 10% at the peak.
        ConnectedParams params;
        params.toX = 100;
        params.gravity = true;
        ConnectedFrame peak = EvaluateConnected(params, Curve::Linear(), 0.66f);
        VERIFY_IS_TRUE(Near(66, peak.offsetX) && Near(80, peak.offsetY) && Near(32, peak.liftZ));
        VERIFY_IS_TRUE(Near(1.1f, peak.scaleX) && Near(1.1f, peak.scaleY));
        ConnectedFrame end = EvaluateConnected(params, Curve::Linear(), 1);
        VERIFY_IS_TRUE(Near(0, end.offsetY) && Near(0, end.liftZ) && Near(1, end.scaleX));

        // When the primary motion scales, the effect leaves Scale alone (GetEffectPropertySet only adds it at 1x1).
        params.toScaleX = 2;
        peak = EvaluateConnected(params, Curve::Linear(), 0.66f);
        VERIFY_IS_TRUE(Near(1.66f, peak.scaleX) && Near(1, peak.scaleY) && Near(80, peak.offsetY));
    }
}