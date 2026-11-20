// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImmediateIndependentTests.h"
#include <ImmediateIndependent.h>
#include <algorithm>
#include <cmath>
#include <iterator>

namespace RenderingBackendTests
{
    using namespace ImmediateIndependent;

    namespace
    {
        bool Near(float a, float b, float epsilon = 1e-3f)
        {
            return std::fabs(a - b) <= epsilon;
        }

        bool Near(PointF a, PointF b, float epsilon = 1e-3f)
        {
            return Near(a.x, b.x, epsilon) && Near(a.y, b.y, epsilon);
        }

        TimingSpec Timing(float duration, double begin = 0)
        {
            TimingSpec spec;
            spec.naturalDuration = duration;
            spec.progressDuration = duration;
            spec.beginTime = begin;
            return spec;
        }
    }

    void ImmediateIndependentTests::MatrixInverseRoundTrips()
    {
        const Matrix3x2F m{ 2, 1, -1, 3, 5, 7 };
        Matrix3x2F inverse;
        VERIFY_IS_TRUE(Invert(m, &inverse));
        const Matrix3x2F product = Multiply(m, inverse);
        VERIFY_IS_TRUE(Near(product.m11, 1) && Near(product.m12, 0) && Near(product.m21, 0) && Near(product.m22, 1));
        VERIFY_IS_TRUE(Near(product.dx, 0) && Near(product.dy, 0));

        Matrix3x2F singular;
        VERIFY_IS_FALSE(Invert(Matrix3x2F{ 0, 0, 0, 0, 1, 1 }, &singular));
    }

    void ImmediateIndependentTests::RotationMatchesQuarterTurnsExactly()
    {
        const Matrix3x2F quarter = RotationMatrix(90, 0, 0);
        VERIFY_ARE_EQUAL(0.0f, quarter.m11);
        VERIFY_ARE_EQUAL(1.0f, quarter.m12);
        VERIFY_ARE_EQUAL(-1.0f, quarter.m21);

        const Matrix3x2F negative = RotationMatrix(-90, 0, 0);
        VERIFY_ARE_EQUAL(-1.0f, negative.m12);

        // Rotating about a center keeps the center fixed.
        VERIFY_IS_TRUE(Near(TransformPoint(RotationMatrix(37, 12, 8), { 12, 8 }), { 12, 8 }));
    }

    void ImmediateIndependentTests::CompositeTransformMatchesComponentOrder()
    {
        TransformModel model;
        TransformOp op;
        op.type = TransformOpType::Composite;
        const float params[] = { 10, 20, 2, 3, 0, 0, 90, 5, 6 };
        std::copy(std::begin(params), std::end(params), op.params);
        model.ops.push_back(op);

        // (11,20) - center = (1,0) -> scale (2,0) -> rotate 90 (0,2) -> + center + translate (15,28).
        const Matrix3x2F m = EvaluateTransform(model, nullptr, 0);
        VERIFY_IS_TRUE(Near(TransformPoint(m, { 11, 20 }), { 15, 28 }));

        // A track overrides the bound parameter: rotation 0 gives (2,0) + (15,26) = (17,26).
        model.ops[0].track[6] = 0;
        const float rotation = 0;
        VERIFY_IS_TRUE(Near(TransformPoint(EvaluateTransform(model, &rotation, 1), { 11, 20 }), { 17, 26 }));
    }

    void ImmediateIndependentTests::ExpirationHonorsRepeatAutoReverseAndSpeed()
    {
        TimingSpec spec = Timing(1, 0.5);
        spec.repeatCount = 3;
        spec.autoReverse = true;
        spec.speedRatio = 2;
        OptionalTime expiration = ComputeExpiration(spec);
        VERIFY_IS_TRUE(expiration.hasValue);
        VERIFY_IS_TRUE(Near(static_cast<float>(expiration.value), 3.5f));

        spec = Timing(1);
        spec.repeat = RepeatKind::Forever;
        VERIFY_IS_FALSE(ComputeExpiration(spec).hasValue);

        spec = Timing(1);
        spec.repeat = RepeatKind::Duration;
        spec.repeatDuration = 2.5;
        VERIFY_IS_TRUE(Near(static_cast<float>(ComputeExpiration(spec).value), 2.5f));

        spec = Timing(0, 1);
        expiration = ComputeExpiration(spec);
        VERIFY_IS_TRUE(expiration.hasValue && Near(static_cast<float>(expiration.value), 1));
    }

    void ImmediateIndependentTests::ClockHoldsOrStopsAtEnd()
    {
        TimingSpec spec = Timing(1, 0.5);
        VERIFY_IS_TRUE(EvaluateClock(spec, true, 0.25).state == ClockState::NotStarted);

        ClockSample sample = EvaluateClock(spec, true, 0.75);
        VERIFY_IS_TRUE(sample.state == ClockState::Active);
        VERIFY_IS_TRUE(Near(sample.progress, 0.25f));

        sample = EvaluateClock(spec, true, 3);
        VERIFY_IS_TRUE(sample.state == ClockState::Filling);
        VERIFY_ARE_EQUAL(1.0f, sample.progress);

        spec.holdEnd = false;
        VERIFY_IS_TRUE(EvaluateClock(spec, true, 3).state == ClockState::Stopped);
        VERIFY_IS_TRUE(EvaluateClock(spec, false, 3).state == ClockState::NotStarted);
    }

    void ImmediateIndependentTests::AutoReverseInvertsOddIterations()
    {
        TimingSpec spec = Timing(1);
        spec.autoReverse = true;
        ClockSample sample = EvaluateClock(spec, true, 1.25);
        VERIFY_IS_TRUE(sample.state == ClockState::Active);
        VERIFY_IS_TRUE(Near(sample.progress, 0.75f));

        // Expired after the reverse leg: holds at the beginning.
        sample = EvaluateClock(spec, true, 3);
        VERIFY_IS_TRUE(sample.state == ClockState::Filling);
        VERIFY_ARE_EQUAL(0.0f, sample.progress);
    }

    void ImmediateIndependentTests::ChainGivesLeafNoTimeWhenParentStopped()
    {
        TimingSpec chain[2] = { Timing(1, 0.5), Timing(1) };
        ClockSample leaf = EvaluateChain(chain, 2, 0.75);
        VERIFY_IS_TRUE(leaf.state == ClockState::Active);
        VERIFY_IS_TRUE(Near(leaf.progress, 0.25f));

        chain[0].holdEnd = false;
        VERIFY_IS_TRUE(EvaluateChain(chain, 2, 5).state == ClockState::Stopped);
    }

    void ImmediateIndependentTests::CurveSamplingInterpolatesAndClamps()
    {
        ValueCurve curve;
        curve.baseValue = 42;
        VERIFY_ARE_EQUAL(42.0f, SampleCurve(curve, 0.5f));
        curve.samples = { 0, 10, 20 };
        VERIFY_IS_TRUE(Near(SampleCurve(curve, 0.25f), 5));
        VERIFY_ARE_EQUAL(0.0f, SampleCurve(curve, -1));
        VERIFY_ARE_EQUAL(20.0f, SampleCurve(curve, 2));
    }

    void ImmediateIndependentTests::TrackRunsUntilExpirationThenHolds()
    {
        AnimationTrack track;
        track.chain = { Timing(2) };
        track.rootParentTime0 = 0.5;
        track.curve.samples = { 0, 100 };
        bool running = false;
        VERIFY_IS_TRUE(Near(EvaluateTrack(track, 0.5, &running), 50));
        VERIFY_IS_TRUE(running);
        VERIFY_ARE_EQUAL(100.0f, EvaluateTrack(track, 5, &running));
        VERIFY_IS_FALSE(running);

        AnimationTrack spin;
        spin.kind = AnimationTrack::Kind::Linear;
        spin.value0 = 10;
        spin.rate = 360;
        VERIFY_IS_TRUE(Near(EvaluateTrack(spin, 0.5, &running), 190));
        VERIFY_IS_TRUE(running);
    }

    void ImmediateIndependentTests::LayerDeltaRotatesAboutOrigin()
    {
        const Matrix3x2F world0 = TranslationMatrix(100, 50);
        const Matrix3x2F delta = ComputeLayerDelta(world0, { 10, 10 }, Matrix3x2F{}, RotationMatrix(90, 0, 0));
        // (20,10) - origin = (10,0) -> rotate 90 (0,10) -> + origin (10,20) -> + (100,50).
        VERIFY_IS_TRUE(Near(TransformPoint(Multiply(world0, delta), { 20, 10 }), { 110, 70 }));
        VERIFY_IS_TRUE(Near(TransformPoint(Multiply(world0, delta), { 10, 10 }), { 110, 60 }));
    }

    void ImmediateIndependentTests::LayerDeltaIsIdentityWhenUnchanged()
    {
        const Matrix3x2F render = ScaleMatrix(2, 2, 0, 0);
        Matrix3x2F world0 = TranslationMatrix(-5, -5);
        world0 = Multiply(world0, render);
        world0 = Multiply(world0, TranslationMatrix(30, 40));
        const Matrix3x2F delta = ComputeLayerDelta(world0, { 5, 5 }, render, render);
        VERIFY_IS_TRUE(Near(delta.m11, 1) && Near(delta.m12, 0) && Near(delta.m21, 0) && Near(delta.m22, 1));
        VERIFY_IS_TRUE(Near(delta.dx, 0) && Near(delta.dy, 0));
    }

    void ImmediateIndependentTests::DeviceBoundsFollowDelta()
    {
        const RectF moved = TransformDeviceBounds(TranslationMatrix(10, 0), { 0, 0, 20, 20 }, 2);
        VERIFY_ARE_EQUAL(19.0f, moved.left);
        VERIFY_ARE_EQUAL(-1.0f, moved.top);
        VERIFY_ARE_EQUAL(41.0f, moved.right);
        VERIFY_ARE_EQUAL(21.0f, moved.bottom);
    }

    void ImmediateIndependentTests::ArcEndsOnCircle()
    {
        std::vector<RenderingBackend::PathVerb> verbs;
        std::vector<PointF> points;
        BuildArc(50, 50, 10, 0, 270, verbs, points);
        VERIFY_ARE_EQUAL(4u, static_cast<uint32_t>(verbs.size()));
        VERIFY_ARE_EQUAL(10u, static_cast<uint32_t>(points.size()));
        VERIFY_IS_TRUE(Near(points.front(), { 60, 50 }));
        VERIFY_IS_TRUE(Near(points.back(), { 50, 40 }));
    }

    void ImmediateIndependentTests::SimulationParsesEveryListAndLimit()
    {
        DeviceLostSimulation every = DeviceLostSimulation::Parse(L"Every:30, limit:5");
        VERIFY_IS_TRUE(every.IsEnabled());
        VERIFY_IS_TRUE(every.ShouldLose(30, 0));
        VERIFY_IS_FALSE(every.ShouldLose(31, 0));
        VERIFY_IS_FALSE(every.ShouldLose(60, 5));

        DeviceLostSimulation list = DeviceLostSimulation::Parse(L"3, 10,25");
        VERIFY_ARE_EQUAL(3u, static_cast<uint32_t>(list.frames.size()));
        VERIFY_IS_TRUE(list.ShouldLose(10, 0));
        VERIFY_IS_FALSE(list.ShouldLose(11, 0));
    }

    void ImmediateIndependentTests::SimulationIgnoresMalformedTokens()
    {
        VERIFY_IS_FALSE(DeviceLostSimulation::Parse(nullptr).IsEnabled());
        VERIFY_IS_FALSE(DeviceLostSimulation::Parse(L"").IsEnabled());
        const DeviceLostSimulation parsed = DeviceLostSimulation::Parse(L"every:x,abc,0,-4,7");
        VERIFY_ARE_EQUAL(0u, parsed.every);
        VERIFY_ARE_EQUAL(1u, static_cast<uint32_t>(parsed.frames.size()));
        VERIFY_ARE_EQUAL(7u, parsed.frames[0]);
    }

    void ImmediateIndependentTests::ScaleChangeDetection()
    {
        VERIFY_IS_FALSE(ScaleChangeInvalidatesCaches(1.5f, 1.5f));
        VERIFY_IS_TRUE(ScaleChangeInvalidatesCaches(1.0f, 1.25f));
    }
}
