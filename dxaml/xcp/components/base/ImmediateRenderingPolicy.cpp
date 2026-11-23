// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediateRenderingPolicy.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace ImmediateRenderingPolicy
{
    using namespace RenderingBackend;

    const wchar_t* GetFallbackReason(const ElementInterop& interop) noexcept
    {
        if (interop.swapChainPanel)
        {
            return L"SwapChainPanel";
        }
        if (interop.media)
        {
            return L"MediaPlayerElement";
        }
        if (interop.appAnimatedVisualPlayer)
        {
            return L"AnimatedVisualPlayer";
        }
        if (interop.handInVisual && !interop.handInIsBuiltInProgressRing)
        {
            return L"Hand-in Composition visual";
        }
        if (interop.connectedAnimation)
        {
            return L"ConnectedAnimation";
        }
        if (interop.implicitShowHideAnimation)
        {
            return L"Implicit show/hide animation";
        }
        if (interop.handOffVisualTouched)
        {
            return L"Hand-off Composition visual";
        }
        return nullptr;
    }

    namespace
    {
        bool Near(float value, float expected) noexcept
        {
            return std::fabs(value - expected) <= 1e-4f;
        }
    }

    bool IsHandOffVisualTouched(const HandOffVisualState& state) noexcept
    {
        return state.animating || state.unsupportedClip;
    }

    bool ComputeHandOffVisualTransform(const HandOffVisualState& state, _Out_writes_(16) float matrix[16]) noexcept
    {
        using M4 = std::array<double, 16>; // row-major, row vectors
        auto identity = [] { M4 m{}; m[0] = m[5] = m[10] = m[15] = 1; return m; };
        auto multiply = [](const M4& a, const M4& b) // a, then b
        {
            M4 r{};
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    for (int k = 0; k < 4; ++k)
                        r[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            return r;
        };
        auto translation = [&](double x, double y, double z) { M4 m = identity(); m[12] = x; m[13] = y; m[14] = z; return m; };

        const double pivot[3] = {
            static_cast<double>(state.centerPoint[0]) + static_cast<double>(state.anchorPoint[0]) * state.size[0],
            static_cast<double>(state.centerPoint[1]) + static_cast<double>(state.anchorPoint[1]) * state.size[1],
            state.centerPoint[2] };
        M4 m = translation(-pivot[0], -pivot[1], -pivot[2]);

        M4 scale = identity();
        scale[0] = state.scale[0]; scale[5] = state.scale[1]; scale[10] = state.scale[2];
        m = multiply(m, scale);

        // System.Numerics Matrix4x4.CreateFromQuaternion / CreateFromAxisAngle (Composition's conventions).
        {
            const double x = state.orientation[0], y = state.orientation[1], z = state.orientation[2], w = state.orientation[3];
            M4 q = identity();
            q[0] = 1 - 2 * (y * y + z * z); q[1] = 2 * (x * y + z * w); q[2] = 2 * (x * z - y * w);
            q[4] = 2 * (x * y - z * w); q[5] = 1 - 2 * (z * z + x * x); q[6] = 2 * (y * z + x * w);
            q[8] = 2 * (x * z + y * w); q[9] = 2 * (y * z - x * w); q[10] = 1 - 2 * (y * y + x * x);
            m = multiply(m, q);
        }
        if (state.rotationAngle != 0)
        {
            double x = state.rotationAxis[0], y = state.rotationAxis[1], z = state.rotationAxis[2];
            const double length = std::sqrt(x * x + y * y + z * z);
            if (length > 0)
            {
                x /= length; y /= length; z /= length;
                const double sa = std::sin(state.rotationAngle), ca = std::cos(state.rotationAngle);
                const double xx = x * x, yy = y * y, zz = z * z, xy = x * y, xz = x * z, yz = y * z;
                M4 r = identity();
                r[0] = xx + ca * (1 - xx); r[1] = xy - ca * xy + sa * z; r[2] = xz - ca * xz - sa * y;
                r[4] = xy - ca * xy - sa * z; r[5] = yy + ca * (1 - yy); r[6] = yz - ca * yz + sa * x;
                r[8] = xz - ca * xz + sa * y; r[9] = yz - ca * yz - sa * x; r[10] = zz + ca * (1 - zz);
                m = multiply(m, r);
            }
        }
        m = multiply(m, translation(state.centerPoint[0], state.centerPoint[1], state.centerPoint[2]));
        M4 tm{};
        for (int i = 0; i < 16; ++i)
        {
            tm[i] = state.transformMatrix[i];
        }
        m = multiply(m, tm);
        m = multiply(m, translation(static_cast<double>(state.offset[0]) + state.translation[0],
            static_cast<double>(state.offset[1]) + state.translation[1], static_cast<double>(state.offset[2]) + state.translation[2]));

        bool isIdentity = true;
        for (int i = 0; i < 16; ++i)
        {
            matrix[i] = static_cast<float>(m[i]);
            isIdentity = isIdentity && Near(matrix[i], (i % 5 == 0) ? 1.0f : 0.0f);
        }
        return !isIdentity;
    }

    namespace
    {
        // Row-vector convention (same as CMILMatrix / D2D): p' = (x*m11 + y*m21 + dx, x*m12 + y*m22 + dy).
        // Then(a, b) applies a first, then b.
        Matrix3x2F Then(const Matrix3x2F& a, const Matrix3x2F& b) noexcept
        {
            Matrix3x2F r;
            r.m11 = a.m11 * b.m11 + a.m12 * b.m21;
            r.m12 = a.m11 * b.m12 + a.m12 * b.m22;
            r.m21 = a.m21 * b.m11 + a.m22 * b.m21;
            r.m22 = a.m21 * b.m12 + a.m22 * b.m22;
            r.dx = a.dx * b.m11 + a.dy * b.m21 + b.dx;
            r.dy = a.dx * b.m12 + a.dy * b.m22 + b.dy;
            return r;
        }

        bool Invert(const Matrix3x2F& m, _Out_ Matrix3x2F* inverse) noexcept
        {
            const double det = static_cast<double>(m.m11) * m.m22 - static_cast<double>(m.m12) * m.m21;
            if (!std::isfinite(det) || std::fabs(det) < 1e-12)
            {
                return false;
            }
            Matrix3x2F r;
            r.m11 = static_cast<float>(m.m22 / det);
            r.m12 = static_cast<float>(-m.m12 / det);
            r.m21 = static_cast<float>(-m.m21 / det);
            r.m22 = static_cast<float>(m.m11 / det);
            r.dx = -(m.dx * r.m11 + m.dy * r.m21);
            r.dy = -(m.dx * r.m12 + m.dy * r.m22);
            *inverse = r;
            return true;
        }
    }

    bool MapLinearGradient(const LinearGradientInput& input, _Out_ PointF* start, _Out_ PointF* end) noexcept
    {
        *start = input.start;
        *end = input.end;

        // Brush space -> shape local space. RelativeTransform applies in the unit (bounding box) space, before the
        // bounding box mapping; Transform applies after it.
        Matrix3x2F toLocal = input.relativeTransform;
        if (input.relativeToBoundingBox)
        {
            Matrix3x2F boxMap;
            boxMap.m11 = input.bounds.right - input.bounds.left;
            boxMap.m22 = input.bounds.bottom - input.bounds.top;
            boxMap.dx = input.bounds.left;
            boxMap.dy = input.bounds.top;
            toLocal = Then(toLocal, boxMap);
        }
        toLocal = Then(toLocal, input.transform);

        const double dx = static_cast<double>(input.end.x) - input.start.x;
        const double dy = static_cast<double>(input.end.y) - input.start.y;
        const double lengthSquared = dx * dx + dy * dy;
        Matrix3x2F inverse;
        if (!(lengthSquared > 1e-12) || !Invert(toLocal, &inverse))
        {
            return false;
        }

        // The gradient parameter in brush space is t(p) = dot(p - s, d) / |d|^2 with p = A q + b (q local). That is
        // t(q) = dot(q, g) + c, an affine function of q, so the mapped gradient is still linear: its local axis is g.
        const double gx = (inverse.m11 * dx + inverse.m12 * dy) / lengthSquared;
        const double gy = (inverse.m21 * dx + inverse.m22 * dy) / lengthSquared;
        const double gSquared = gx * gx + gy * gy;
        if (!(gSquared > 1e-18) || !std::isfinite(gSquared))
        {
            return false;
        }

        // The mapped start point has t = 0; the end lies one gradient length along the (local) axis g.
        const double sx = static_cast<double>(input.start.x) * toLocal.m11 + static_cast<double>(input.start.y) * toLocal.m21 + toLocal.dx;
        const double sy = static_cast<double>(input.start.x) * toLocal.m12 + static_cast<double>(input.start.y) * toLocal.m22 + toLocal.dy;
        start->x = static_cast<float>(sx);
        start->y = static_cast<float>(sy);
        end->x = static_cast<float>(sx + gx / gSquared);
        end->y = static_cast<float>(sy + gy / gSquared);
        return true;
    }

    uint32_t GetThemeShadowLayers(float translationZ, bool darkTheme, _Out_writes_(2) ShadowLayer* layers) noexcept
    {
        layers[0] = {};
        layers[1] = {};

        // DropShadowRecipe.h (GetDropShadowRecipe).
        const float elevation = std::min(64.0f, translationZ / 2);
        if (!(elevation >= 2))
        {
            return 0;
        }

        float ambientOpacity = 0;
        float ambientBlur = 2;
        float ambientYOffset = 0;
        float directionalOpacity = 0;
        if (elevation <= 16)
        {
            directionalOpacity = darkTheme ? 0.26f : std::min(elevation / 100.0f + 0.06f, 0.14f);
        }
        else
        {
            ambientBlur = elevation / 3;
            ambientYOffset = 2;
            ambientOpacity = darkTheme ? 0.37f : 0.15f;
            directionalOpacity = darkTheme ? 0.37f : 0.19f;
        }

        // Composition quantizes the shadow colors to 8 bits.
        auto quantize = [](float opacity) { return static_cast<float>(static_cast<uint8_t>(opacity * 255)) / 255.0f; };

        uint32_t count = 0;
        if (ambientOpacity > 0)
        {
            layers[count].blurRadius = ambientBlur;
            layers[count].offset = { 0, ambientYOffset };
            layers[count].color = { 0, 0, 0, quantize(ambientOpacity) };
            ++count;
        }
        layers[count].blurRadius = elevation;
        layers[count].offset = { 0, elevation * 0.5f };
        layers[count].color = { 0, 0, 0, quantize(directionalOpacity) };
        ++count;
        return count;
    }

    RectF InflateForBlur(const RectF& rect, float blurRadius, PointF offset) noexcept
    {
        const float extent = std::ceil(std::max(blurRadius, 0.0f) * 1.5f); // 3 sigma, sigma = blurRadius / 2
        RectF result = rect;
        result.left = std::min(rect.left, rect.left + offset.x - extent);
        result.top = std::min(rect.top, rect.top + offset.y - extent);
        result.right = std::max(rect.right, rect.right + offset.x + extent);
        result.bottom = std::max(rect.bottom, rect.bottom + offset.y + extent);
        return result;
    }

    namespace
    {
        struct Hsv
        {
            double h = 0, s = 0, v = 0;
        };

        Hsv ToHsv(double r, double g, double b) noexcept
        {
            const double maxValue = std::max({ r, g, b });
            const double minValue = std::min({ r, g, b });
            Hsv hsv;
            hsv.v = maxValue;
            hsv.s = maxValue > 0 ? (maxValue - minValue) / maxValue : 0;
            return hsv; // hue is not needed: only V is changed below, which preserves hue
        }

        double Quantize8(double alpha) noexcept
        {
            return std::round(std::clamp(alpha, 0.0, 1.0) * 255.0) / 255.0;
        }

        double GetTintOpacityModifier(const ColorF& tint) noexcept
        {
            // MUXC AcrylicBrush.cpp GetTintOpacityModifier.
            constexpr double midPoint = 0.50;
            constexpr double whiteMaxOpacity = 0.45;
            constexpr double midPointMaxOpacity = 0.90;
            constexpr double blackMaxOpacity = 0.85;

            const Hsv hsv = ToHsv(tint.r, tint.g, tint.b);
            if (hsv.v == midPoint)
            {
                return midPointMaxOpacity;
            }
            double lowestMaxOpacity = midPointMaxOpacity;
            double maxDeviation = midPoint;
            if (hsv.v > midPoint)
            {
                lowestMaxOpacity = whiteMaxOpacity;
                maxDeviation = 1 - maxDeviation;
            }
            else
            {
                lowestMaxOpacity = blackMaxOpacity;
            }
            double maxOpacitySuppression = midPointMaxOpacity - lowestMaxOpacity;
            const double normalizedDeviation = std::fabs(hsv.v - midPoint) / maxDeviation;
            if (hsv.s > 0)
            {
                maxOpacitySuppression *= std::max(1 - (hsv.s * 2), 0.0);
            }
            return midPointMaxOpacity - maxOpacitySuppression * normalizedDeviation;
        }
    }

    AcrylicLayers GetAcrylicLayers(ColorF tintColor, float tintOpacity, bool hasLuminosityOpacity, float luminosityOpacity) noexcept
    {
        AcrylicLayers layers;

        const double modifier = hasLuminosityOpacity ? 1.0 : GetTintOpacityModifier(tintColor);
        layers.tint = tintColor;
        layers.tint.a = static_cast<float>(Quantize8(tintColor.a * tintOpacity * modifier));

        const double originalTintAlpha = Quantize8(tintColor.a * tintOpacity);
        if (hasLuminosityOpacity)
        {
            layers.luminosity = tintColor;
            layers.luminosity.a = static_cast<float>(std::clamp(static_cast<double>(luminosityOpacity), 0.0, 1.0));
            return layers;
        }

        // Clamp HSV value to [0.125, 0.965]; scaling RGB keeps hue and saturation.
        const Hsv hsv = ToHsv(tintColor.r, tintColor.g, tintColor.b);
        const double clampedV = std::clamp(hsv.v, 0.125, 0.965);
        if (hsv.v > 0)
        {
            const double factor = clampedV / hsv.v;
            layers.luminosity.r = static_cast<float>(tintColor.r * factor);
            layers.luminosity.g = static_cast<float>(tintColor.g * factor);
            layers.luminosity.b = static_cast<float>(tintColor.b * factor);
        }
        else
        {
            layers.luminosity.r = layers.luminosity.g = layers.luminosity.b = static_cast<float>(clampedV);
        }
        layers.luminosity.a = static_cast<float>(std::min(originalTintAlpha * (1.03 - 0.15) + 0.15, 1.0));
        return layers;
    }

    bool IsCoveredByOpaque(const RectF& rect, _In_reads_(count) const RectF* opaque, uint32_t count) noexcept
    {
        if (!(rect.right > rect.left) || !(rect.bottom > rect.top))
        {
            return false;
        }
        for (uint32_t i = 0; i < count; ++i)
        {
            const RectF& o = opaque[i];
            if (o.left <= rect.left && o.top <= rect.top && o.right >= rect.right && o.bottom >= rect.bottom)
            {
                return true;
            }
        }
        return false;
    }

    bool ExpandDamageForBackdrops(std::vector<RectF>& damage, _In_reads_(count) const BackdropArea* backdrops, uint32_t count) noexcept
    {
        auto intersects = [](const RectF& a, const RectF& b)
        {
            return a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;
        };
        bool any = false;
        std::vector<bool> added(count, false);
        for (bool changed = true; changed;)
        {
            changed = false;
            for (uint32_t i = 0; i < count; ++i)
            {
                const BackdropArea& b = backdrops[i];
                if (added[i] || !(b.rect.right > b.rect.left) || !(b.rect.bottom > b.rect.top))
                {
                    continue;
                }
                const RectF sampled{ b.rect.left - b.margin, b.rect.top - b.margin, b.rect.right + b.margin, b.rect.bottom + b.margin };
                const size_t n = damage.size();
                for (size_t d = 0; d < n; ++d)
                {
                    if (intersects(damage[d], sampled))
                    {
                        damage.push_back(b.rect);
                        added[i] = true;
                        changed = any = true;
                        break;
                    }
                }
            }
        }
        return any;
    }

    void AppendRoundedRectangle(const RectF& rect, float radiusX, float radiusY, std::vector<PathVerb>& verbs, std::vector<PointF>& points)
    {
        const float rx = std::clamp(radiusX, 0.0f, std::max(0.0f, (rect.right - rect.left) / 2));
        const float ry = std::clamp(radiusY, 0.0f, std::max(0.0f, (rect.bottom - rect.top) / 2));
        constexpr float k = 0.5522847f;
        const float kx = rx * k;
        const float ky = ry * k;
        const float l = rect.left, t = rect.top, r = rect.right, b = rect.bottom;
        auto move = [&](float x, float y) { verbs.push_back(PathVerb::MoveTo); points.push_back({ x, y }); };
        auto line = [&](float x, float y) { verbs.push_back(PathVerb::LineTo); points.push_back({ x, y }); };
        auto cubic = [&](float x1, float y1, float x2, float y2, float x, float y)
        {
            verbs.push_back(PathVerb::CubicTo);
            points.push_back({ x1, y1 });
            points.push_back({ x2, y2 });
            points.push_back({ x, y });
        };
        const bool round = rx > 0 && ry > 0;
        move(l + (round ? rx : 0), t);
        line(r - (round ? rx : 0), t);
        if (round) cubic(r - rx + kx, t, r, t + ry - ky, r, t + ry);
        line(r, b - (round ? ry : 0));
        if (round) cubic(r, b - ry + ky, r - rx + kx, b, r - rx, b);
        line(l + (round ? rx : 0), b);
        if (round) cubic(l + rx - kx, b, l, b - ry + ky, l, b - ry);
        line(l, t + (round ? ry : 0));
        if (round) cubic(l, t + ry - ky, l + rx - kx, t, l + rx, t);
        verbs.push_back(PathVerb::Close);
    }

    void AppendArc(PointF center, float radius, float startDegrees, float sweepDegrees, std::vector<PathVerb>& verbs, std::vector<PointF>& points)
    {
        if (!(radius > 0) || !(sweepDegrees > 0))
        {
            return;
        }
        constexpr double toRadians = 3.14159265358979323846 / 180.0;
        auto at = [&](double degrees, double r)
        {
            const double a = degrees * toRadians;
            return PointF{ static_cast<float>(center.x + r * std::sin(a)), static_cast<float>(center.y - r * std::cos(a)) };
        };
        const double sweep = std::min<double>(sweepDegrees, 360.0);
        const int segments = std::max(1, static_cast<int>(std::ceil(sweep / 90.0)));
        const double step = sweep / segments;
        const double handle = 4.0 / 3.0 * std::tan(step * toRadians / 4) * radius;
        verbs.push_back(PathVerb::MoveTo);
        points.push_back(at(startDegrees, radius));
        for (int i = 0; i < segments; ++i)
        {
            const double a0 = startDegrees + step * i;
            const double a1 = a0 + step;
            const PointF p0 = at(a0, radius);
            const PointF p1 = at(a1, radius);
            // Tangent (clockwise) at angle a: (cos a, sin a).
            const double r0 = a0 * toRadians, r1 = a1 * toRadians;
            verbs.push_back(PathVerb::CubicTo);
            points.push_back({ static_cast<float>(p0.x + handle * std::cos(r0)), static_cast<float>(p0.y + handle * std::sin(r0)) });
            points.push_back({ static_cast<float>(p1.x - handle * std::cos(r1)), static_cast<float>(p1.y - handle * std::sin(r1)) });
            points.push_back(p1);
        }
    }

    ArcSpan GetProgressRingArc(bool indeterminate, double value, double minimum, double maximum, double timeSeconds) noexcept
    {
        ArcSpan arc;
        if (!indeterminate)
        {
            if (!(maximum > minimum))
            {
                return arc;
            }
            const double fraction = std::clamp((value - minimum) / (maximum - minimum), 0.0, 1.0);
            arc.sweepDegrees = static_cast<float>(360.0 * fraction);
            return arc;
        }

        // Approximates the built-in indeterminate Lottie: a 2 s cycle in which the arc's head runs ahead (grow) and
        // then its tail catches up (shrink), while the whole arc rotates.
        constexpr double period = 2.0;
        constexpr double minSweep = 10.0;
        constexpr double maxSweep = 270.0;
        const double t = std::isfinite(timeSeconds) ? std::fmod(std::max(timeSeconds, 0.0), period) / period : 0.0;
        const double grow = t < 0.5 ? t * 2 : (1 - t) * 2;
        const double sweep = minSweep + (maxSweep - minSweep) * grow;
        const double tailAdvance = t < 0.5 ? 0.0 : (maxSweep - minSweep) * (t - 0.5) * 2;
        const double start = std::fmod(360.0 * t + tailAdvance, 360.0);
        arc.startDegrees = static_cast<float>(start);
        arc.sweepDegrees = static_cast<float>(sweep);
        return arc;
    }

    // ---- scrolling ----

    bool TryGetScrollBlitDelta(const ScrollerFrame& previous, const ScrollerFrame& current, float viewportWidth,
        float viewportHeight, _Out_ int32_t* dx, _Out_ int32_t* dy) noexcept
    {
        *dx = 0;
        *dy = 0;
        if (!(previous.zoom > 0) || std::fabs(current.zoom - previous.zoom) > 1e-4f * previous.zoom)
        {
            return false;
        }
        const float fx = current.offsetX - previous.offsetX;
        const float fy = current.offsetY - previous.offsetY;
        if (!std::isfinite(fx) || !std::isfinite(fy))
        {
            return false;
        }
        const float rx = std::round(fx);
        const float ry = std::round(fy);
        if ((rx == 0 && ry == 0) || std::fabs(fx - rx) > 0.01f || std::fabs(fy - ry) > 0.01f
            || std::fabs(rx) >= viewportWidth || std::fabs(ry) >= viewportHeight)
        {
            return false;
        }
        *dx = static_cast<int32_t>(rx);
        *dy = static_cast<int32_t>(ry);
        return true;
    }
}