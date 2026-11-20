// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediateRenderingPolicy.h>
#include <algorithm>
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
        if (state.hasNonInsetClip || !Near(state.opacity, 1) || !Near(state.rotationAngle, 0))
        {
            return true;
        }
        for (int i = 0; i < 3; ++i)
        {
            if (!Near(state.offset[i], 0) || !Near(state.scale[i], 1) || !Near(state.translation[i], 0))
            {
                return true;
            }
        }
        for (int i = 0; i < 16; ++i)
        {
            if (!Near(state.transformMatrix[i], (i % 5 == 0) ? 1.0f : 0.0f))
            {
                return true;
            }
        }
        return false;
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
}
