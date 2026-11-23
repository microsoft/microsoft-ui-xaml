// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImmediateRenderingPolicyTests.h"
#include <ImmediateRenderingPolicy.h>
#include <cmath>

namespace ImmediateRenderingPolicyTests
{
    using namespace ImmediateRenderingPolicy;
    using namespace RenderingBackend;

    namespace
    {
        bool Near(float a, float b, float tolerance = 1e-3f)
        {
            return std::fabs(a - b) <= tolerance;
        }

        // Gradient parameter t of a local point for a mapped gradient.
        float ParameterAt(PointF start, PointF end, PointF q)
        {
            const float dx = end.x - start.x;
            const float dy = end.y - start.y;
            return ((q.x - start.x) * dx + (q.y - start.y) * dy) / (dx * dx + dy * dy);
        }
    }

    void PolicyTests::PlainElementDoesNotFallBack()
    {
        VERIFY_IS_NULL(GetFallbackReason(ElementInterop{}));
    }

    void PolicyTests::InteropTriggersFallBack()
    {
        ElementInterop interop;
        interop.swapChainPanel = true;
        VERIFY_ARE_EQUAL(std::wstring(L"SwapChainPanel"), std::wstring(GetFallbackReason(interop)));

        interop = {};
        interop.media = true;
        VERIFY_IS_NOT_NULL(GetFallbackReason(interop));

        interop = {};
        interop.appAnimatedVisualPlayer = true;
        VERIFY_ARE_EQUAL(std::wstring(L"AnimatedVisualPlayer"), std::wstring(GetFallbackReason(interop)));

        interop = {};
        interop.handInVisual = true;
        VERIFY_IS_NOT_NULL(GetFallbackReason(interop));

        interop = {};
        interop.connectedAnimation = true;
        VERIFY_IS_NOT_NULL(GetFallbackReason(interop));

        interop = {};
        interop.implicitShowHideAnimation = true;
        VERIFY_IS_NOT_NULL(GetFallbackReason(interop));

        interop = {};
        interop.handOffVisualTouched = true;
        VERIFY_IS_NOT_NULL(GetFallbackReason(interop));
    }

    void PolicyTests::BuiltInProgressRingHandInDoesNotFallBack()
    {
        ElementInterop interop;
        interop.handInVisual = true;
        interop.handInIsBuiltInProgressRing = true;
        VERIFY_IS_NULL(GetFallbackReason(interop));
    }

    void PolicyTests::UntouchedHandOffVisualIsPaintable()
    {
        VERIFY_IS_FALSE(IsHandOffVisualTouched(HandOffVisualState{}));
    }

    void PolicyTests::TouchedHandOffVisualIsDetected()
    {
        // Static values are painted natively (clips3d); only animations and unreadable clips need Composition.
        HandOffVisualState state;
        state.opacity = 0.5f;
        state.offset[1] = 12;
        state.scale[0] = 0.9f;
        state.rotationAngle = 0.1f;
        state.transformMatrix[12] = 4;
        state.translation[2] = 32;
        VERIFY_IS_FALSE(IsHandOffVisualTouched(state));

        state = {};
        state.animating = true;
        VERIFY_IS_TRUE(IsHandOffVisualTouched(state));

        state = {};
        state.unsupportedClip = true;
        VERIFY_IS_TRUE(IsHandOffVisualTouched(state));
    }

    void PolicyTests::HandOffVisualTransformFollowsCompositionOrder()
    {
        auto map = [](const float m[16], float x, float y, float* outX, float* outY)
        {
            const float w = x * m[3] + y * m[7] + m[15];
            *outX = (x * m[0] + y * m[4] + m[12]) / w;
            *outY = (x * m[1] + y * m[5] + m[13]) / w;
        };
        float m[16];
        VERIFY_IS_FALSE(ComputeHandOffVisualTransform(HandOffVisualState{}, m));

        // Scale 2 about (10, 10), then rotate 90 degrees clockwise about it, then offset (100, 0): (20, 10) is 10 DIPs
        // right of the center, scaled to 20 and rotated to 20 below it.
        HandOffVisualState state;
        state.centerPoint[0] = 10;
        state.centerPoint[1] = 10;
        state.scale[0] = 2;
        state.scale[1] = 2;
        state.rotationAngle = 3.14159265f / 2;
        state.offset[0] = 100;
        VERIFY_IS_TRUE(ComputeHandOffVisualTransform(state, m));
        float x, y;
        map(m, 20, 10, &x, &y);
        VERIFY_IS_TRUE(Near(x, 110) && Near(y, 30));

        // TransformMatrix applies after the center-point transforms and before Offset; Translation adds to Offset.
        state = {};
        state.scale[0] = 2;
        state.transformMatrix[12] = 5;
        state.translation[1] = 7;
        VERIFY_IS_TRUE(ComputeHandOffVisualTransform(state, m));
        map(m, 3, 0, &x, &y);
        VERIFY_IS_TRUE(Near(x, 11) && Near(y, 7));
    }

    void PolicyTests::RelativeGradientMapsToBounds()
    {
        LinearGradientInput input;
        input.start = { 0, 0 };
        input.end = { 1, 0 };
        input.bounds = { 10, 20, 110, 70 };
        PointF start, end;
        VERIFY_IS_TRUE(MapLinearGradient(input, &start, &end));
        VERIFY_IS_TRUE(Near(start.x, 10) && Near(start.y, 20));
        VERIFY_IS_TRUE(Near(end.x, 110) && Near(end.y, 20));

        // Diagonal on a non-square box: isolines stay parallel to the box's other diagonal, so the mapped axis is
        // perpendicular to it rather than along (0,0)-(1,1).
        input.end = { 1, 1 };
        input.bounds = { 0, 0, 200, 100 };
        VERIFY_IS_TRUE(MapLinearGradient(input, &start, &end));
        VERIFY_IS_TRUE(Near(ParameterAt(start, end, { 0, 0 }), 0));
        VERIFY_IS_TRUE(Near(ParameterAt(start, end, { 200, 100 }), 1));
        VERIFY_IS_TRUE(Near(ParameterAt(start, end, { 200, 0 }), 0.5f));
        VERIFY_IS_TRUE(Near(ParameterAt(start, end, { 0, 100 }), 0.5f));
    }

    void PolicyTests::AbsoluteGradientIsUnchanged()
    {
        LinearGradientInput input;
        input.relativeToBoundingBox = false;
        input.start = { 5, 6 };
        input.end = { 50, 60 };
        input.bounds = { 0, 0, 300, 300 };
        PointF start, end;
        VERIFY_IS_TRUE(MapLinearGradient(input, &start, &end));
        VERIFY_IS_TRUE(Near(start.x, 5) && Near(start.y, 6));
        VERIFY_IS_TRUE(Near(end.x, 50) && Near(end.y, 60));
    }

    void PolicyTests::GradientTransformsAreApplied()
    {
        LinearGradientInput input;
        input.relativeToBoundingBox = false;
        input.start = { 0, 0 };
        input.end = { 10, 0 };
        input.transform.dx = 100; // Brush.Transform translate
        PointF start, end;
        VERIFY_IS_TRUE(MapLinearGradient(input, &start, &end));
        VERIFY_IS_TRUE(Near(ParameterAt(start, end, { 100, 0 }), 0));
        VERIFY_IS_TRUE(Near(ParameterAt(start, end, { 110, 0 }), 1));

        // RelativeTransform rotates 90 degrees in unit space (about the origin): horizontal becomes vertical.
        input = {};
        input.start = { 0, 0 };
        input.end = { 1, 0 };
        input.bounds = { 0, 0, 100, 100 };
        input.relativeTransform.m11 = 0;
        input.relativeTransform.m12 = 1;
        input.relativeTransform.m21 = -1;
        input.relativeTransform.m22 = 0;
        VERIFY_IS_TRUE(MapLinearGradient(input, &start, &end));
        VERIFY_IS_TRUE(Near(ParameterAt(start, end, { 0, 0 }), 0));
        VERIFY_IS_TRUE(Near(ParameterAt(start, end, { 0, 100 }), 1));
        VERIFY_IS_TRUE(Near(ParameterAt(start, end, { 100, 0 }), 0));
    }

    void PolicyTests::DegenerateGradientIsRejected()
    {
        LinearGradientInput input;
        input.start = { 0.5f, 0.5f };
        input.end = { 0.5f, 0.5f };
        input.bounds = { 0, 0, 10, 10 };
        PointF start, end;
        VERIFY_IS_FALSE(MapLinearGradient(input, &start, &end));

        input.end = { 1, 1 };
        input.transform.m11 = 0;
        input.transform.m22 = 0;
        VERIFY_IS_FALSE(MapLinearGradient(input, &start, &end));
    }

    void PolicyTests::ThemeShadowFollowsRecipe()
    {
        ShadowLayer layers[2];
        VERIFY_ARE_EQUAL(0u, GetThemeShadowLayers(2, false, layers));

        // Elevation 16 (Z = 32): directional only.
        VERIFY_ARE_EQUAL(1u, GetThemeShadowLayers(32, false, layers));
        VERIFY_IS_TRUE(Near(layers[0].blurRadius, 16));
        VERIFY_IS_TRUE(Near(layers[0].offset.y, 8));
        VERIFY_IS_TRUE(Near(layers[0].color.a, static_cast<uint8_t>(0.14f * 255) / 255.0f));
        VERIFY_ARE_EQUAL(1u, GetThemeShadowLayers(32, true, layers));
        VERIFY_IS_TRUE(Near(layers[0].color.a, static_cast<uint8_t>(0.26f * 255) / 255.0f));

        // Elevation 32 (Z = 64): ambient + directional.
        VERIFY_ARE_EQUAL(2u, GetThemeShadowLayers(64, false, layers));
        VERIFY_IS_TRUE(Near(layers[0].blurRadius, 32.0f / 3));
        VERIFY_IS_TRUE(Near(layers[0].offset.y, 2));
        VERIFY_IS_TRUE(Near(layers[0].color.a, static_cast<uint8_t>(0.15f * 255) / 255.0f));
        VERIFY_IS_TRUE(Near(layers[1].blurRadius, 32));
        VERIFY_IS_TRUE(Near(layers[1].offset.y, 16));

        // Clamped at elevation 64.
        VERIFY_ARE_EQUAL(2u, GetThemeShadowLayers(1000, true, layers));
        VERIFY_IS_TRUE(Near(layers[1].blurRadius, 64));
    }

    void PolicyTests::BlurInflationCoversOffset()
    {
        const RectF inflated = InflateForBlur({ 10, 10, 20, 20 }, 4, { 0, 10 });
        VERIFY_IS_TRUE(Near(inflated.left, 4));
        VERIFY_IS_TRUE(Near(inflated.top, 10)); // the shadow moved down, the element itself is still covered
        VERIFY_IS_TRUE(Near(inflated.right, 26));
        VERIFY_IS_TRUE(Near(inflated.bottom, 36));
    }

    void PolicyTests::AcrylicLayersMatchMuxc()
    {
        // White tint at 100%: modifier 0.45 (pure white, no saturation).
        AcrylicLayers layers = GetAcrylicLayers({ 1, 1, 1, 1 }, 1.0f, false, 0);
        VERIFY_IS_TRUE(Near(layers.tint.a, std::round(255 * 0.45f) / 255));
        VERIFY_IS_TRUE(Near(layers.luminosity.r, 0.965f));
        VERIFY_IS_TRUE(Near(layers.luminosity.a, 1.0f));

        // Mid gray: modifier 0.9.
        layers = GetAcrylicLayers({ 0.5f, 0.5f, 0.5f, 1 }, 0.5f, false, 0);
        VERIFY_IS_TRUE(Near(layers.tint.a, std::round(255 * 0.45f) / 255));
        VERIFY_IS_TRUE(Near(layers.luminosity.a, static_cast<float>(std::round(127.5) / 255 * 0.88 + 0.15)));

        // Black: V clamps up to 0.125.
        layers = GetAcrylicLayers({ 0, 0, 0, 1 }, 1.0f, false, 0);
        VERIFY_IS_TRUE(Near(layers.tint.a, std::round(255 * 0.85f) / 255));
        VERIFY_IS_TRUE(Near(layers.luminosity.g, 0.125f));

        // Explicit luminosity opacity: no modifier, luminosity uses the tint RGB as is.
        layers = GetAcrylicLayers({ 0.2f, 0.4f, 0.6f, 1 }, 0.8f, true, 0.7f);
        VERIFY_IS_TRUE(Near(layers.tint.a, std::round(255 * 0.8f) / 255));
        VERIFY_IS_TRUE(Near(layers.luminosity.b, 0.6f));
        VERIFY_IS_TRUE(Near(layers.luminosity.a, 0.7f));
    }

    void PolicyTests::OpaqueCoverRequiresFullContainment()
    {
        const RectF opaque[] = { { 0, 0, 50, 50 }, { 100, 0, 200, 40 } };
        VERIFY_IS_TRUE(IsCoveredByOpaque({ 10, 10, 40, 30 }, opaque, 2));
        VERIFY_IS_TRUE(IsCoveredByOpaque({ 120, 5, 180, 35 }, opaque, 2));
        VERIFY_IS_FALSE(IsCoveredByOpaque({ 40, 10, 60, 30 }, opaque, 2));
        VERIFY_IS_FALSE(IsCoveredByOpaque({ 10, 10, 40, 30 }, opaque, 0));
        VERIFY_IS_FALSE(IsCoveredByOpaque({ 10, 10, 10, 30 }, opaque, 2));
    }

    void PolicyTests::ProgressRingArc()
    {
        ArcSpan arc = GetProgressRingArc(false, 25, 0, 100, 0);
        VERIFY_IS_TRUE(Near(arc.sweepDegrees, 90));
        VERIFY_IS_TRUE(Near(GetProgressRingArc(false, 500, 0, 100, 0).sweepDegrees, 360));
        VERIFY_IS_TRUE(Near(GetProgressRingArc(false, 5, 10, 10, 0).sweepDegrees, 0));

        const ArcSpan a = GetProgressRingArc(true, 0, 0, 100, 0.0);
        const ArcSpan b = GetProgressRingArc(true, 0, 0, 100, 0.5);
        const ArcSpan c = GetProgressRingArc(true, 0, 0, 100, 2.0);
        VERIFY_IS_TRUE(a.sweepDegrees > 0 && b.sweepDegrees > a.sweepDegrees);
        VERIFY_IS_TRUE(Near(a.startDegrees, c.startDegrees) && Near(a.sweepDegrees, c.sweepDegrees)); // periodic
    }

    void PolicyTests::BackdropDamageExpands()
    {
        // Damage near (within the margin of) the first backdrop repaints it; the second overlaps the first and chains.
        const BackdropArea backdrops[] = { { { 100, 100, 200, 200 }, 10 }, { { 190, 100, 300, 200 }, 10 }, { { 500, 500, 600, 600 }, 10 } };
        std::vector<RectF> damage{ { 80, 80, 95, 95 } };
        VERIFY_IS_TRUE(ExpandDamageForBackdrops(damage, backdrops, 3));
        VERIFY_ARE_EQUAL(3u, static_cast<uint32_t>(damage.size()));
        VERIFY_IS_TRUE(Near(damage[1].left, 100) && Near(damage[2].right, 300));

        std::vector<RectF> distant{ { 0, 0, 50, 50 } };
        VERIFY_IS_FALSE(ExpandDamageForBackdrops(distant, backdrops, 3));
        VERIFY_ARE_EQUAL(1u, static_cast<uint32_t>(distant.size()));
    }

    void PolicyTests::RoundedRectanglePathIsClosed()
    {
        std::vector<PathVerb> verbs;
        std::vector<PointF> points;
        AppendRoundedRectangle({ 0, 0, 100, 40 }, 50, 50, verbs, points); // radii clamp to 50 x 20
        VERIFY_ARE_EQUAL(static_cast<int>(PathVerb::MoveTo), static_cast<int>(verbs.front()));
        VERIFY_ARE_EQUAL(static_cast<int>(PathVerb::Close), static_cast<int>(verbs.back()));
        VERIFY_ARE_EQUAL(1u + 4u + 4u * 3u, static_cast<uint32_t>(points.size()));
        for (const PointF& p : points)
        {
            VERIFY_IS_TRUE(p.x >= 0 && p.x <= 100 && p.y >= 0 && p.y <= 40);
        }
        VERIFY_IS_TRUE(Near(points.back().x, points.front().x) && Near(points.back().y, points.front().y));

        verbs.clear();
        points.clear();
        AppendRoundedRectangle({ 0, 0, 10, 10 }, 0, 0, verbs, points);
        VERIFY_ARE_EQUAL(5u, static_cast<uint32_t>(points.size())); // square corners: lines only
    }

    void PolicyTests::ArcPathEndsAtSweep()
    {
        std::vector<PathVerb> verbs;
        std::vector<PointF> points;
        AppendArc({ 50, 50 }, 10, 0, 90, verbs, points);
        VERIFY_ARE_EQUAL(2u, static_cast<uint32_t>(verbs.size()));
        VERIFY_IS_TRUE(Near(points.front().x, 50) && Near(points.front().y, 40)); // 12 o'clock
        VERIFY_IS_TRUE(Near(points.back().x, 60) && Near(points.back().y, 50));   // 3 o'clock (clockwise)

        verbs.clear();
        points.clear();
        AppendArc({ 0, 0 }, 10, 45, 270, verbs, points);
        VERIFY_ARE_EQUAL(4u, static_cast<uint32_t>(verbs.size()));
        VERIFY_IS_TRUE(Near(points.back().x, -7.0711f) && Near(points.back().y, -7.0711f));
    }
}
