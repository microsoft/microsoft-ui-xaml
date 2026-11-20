// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <vector>
#include <ImmediatePainter.h>

// Pure decision and mapping logic used by the immediate render walk (core\hw\ImmediateRenderer*.cpp). Kept free of
// XAML types so it can be unit tested in isolation. See docs/design-notes/immediate-rendering-backends-proposal.md,
// "Full-backend decisions (S11-S23)".
namespace ImmediateRenderingPolicy
{
    // ---- S11/S16 per-window fallback ----

    // What the walk observed on one element. Any trigger makes the whole island fall back to Composition permanently.
    struct ElementInterop
    {
        bool handInVisual = false;              // ElementCompositionPreview.SetElementChildVisual
        bool handInIsBuiltInProgressRing = false; // the AnimatedVisualPlayer inside ProgressRing; drawn natively
        bool handOffVisualTouched = false;      // GetElementVisual() modified or animated (see IsHandOffVisualTouched)
        bool implicitShowHideAnimation = false; // ElementCompositionPreview.SetImplicitShow/HideAnimation
        bool connectedAnimation = false;        // ConnectedAnimation in flight
        bool swapChainPanel = false;
        bool media = false;                     // MediaPlayerElement / MediaPlayerPresenter / full-window media
        bool appAnimatedVisualPlayer = false;   // AnimatedVisualPlayer with an app source (not ProgressRing)
    };

    // Reason string for the WindowFallback event, or nullptr when the element can be painted immediately.
    const wchar_t* GetFallbackReason(const ElementInterop& interop) noexcept;

    // Snapshot of a hand-off visual (ElementCompositionPreview.GetElementVisual) as polled by the walk.
    struct HandOffVisualState
    {
        float opacity = 1;
        float offset[3] = {};
        float scale[3] = { 1, 1, 1 };
        float rotationAngle = 0;
        float transformMatrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        float translation[3] = {};   // the "Translation" property set value, when present
        bool hasNonInsetClip = false; // InsetClip alone is drawn (Expander); other clips are not
    };

    // True when the app changed or animated the hand-off visual in a way the immediate walk cannot reproduce.
    // A running StartAnimation that has not yet moved a value cannot be observed; see the design notes.
    bool IsHandOffVisualTouched(const HandOffVisualState& state) noexcept;

    // ---- S13 materials ----

    // Maps a LinearGradientBrush (StartPoint/EndPoint in its mapping space, RelativeTransform, Transform) to an
    // equivalent clamp-mode gradient in the filled shape's local DIP space.
    struct LinearGradientInput
    {
        RenderingBackend::PointF start;
        RenderingBackend::PointF end{ 1, 1 };
        bool relativeToBoundingBox = true;
        RenderingBackend::RectF bounds;              // the shape's local bounds
        RenderingBackend::Matrix3x2F relativeTransform; // Brush.RelativeTransform (unit space)
        RenderingBackend::Matrix3x2F transform;         // Brush.Transform
    };

    // Returns false when the gradient is degenerate (start == end, or a singular transform); callers then fill with
    // the last stop color, which is what Composition shows.
    bool MapLinearGradient(const LinearGradientInput& input, _Out_ RenderingBackend::PointF* start, _Out_ RenderingBackend::PointF* end) noexcept;

    // One painter shadow (DrawRoundedRectangleShadow arguments besides the rect).
    struct ShadowLayer
    {
        float blurRadius = 0;
        RenderingBackend::PointF offset;
        RenderingBackend::ColorF color{ 0, 0, 0, 0 };
    };

    // ThemeShadow approximation: the ambient + directional drop shadow pair WinUI builds for a caster at Translation.Z
    // (components\graphics\inc\DropShadowRecipe.h). Returns the number of visible layers written (0..2).
    uint32_t GetThemeShadowLayers(float translationZ, bool darkTheme, _Out_writes_(2) ShadowLayer* layers) noexcept;

    // Damage (DIPs) a shadow or blur extends past its shape: 3 sigma (sigma = blurRadius / 2) plus the offset.
    RenderingBackend::RectF InflateForBlur(const RenderingBackend::RectF& rect, float blurRadius, RenderingBackend::PointF offset = {}) noexcept;

    // In-app AcrylicBrush layers, computed exactly like MUXC AcrylicBrush::GetEffectiveTintColor and
    // GetEffectiveLuminosityColor. Painted over the blurred backdrop: luminosity first, then tint.
    struct AcrylicLayers
    {
        RenderingBackend::ColorF luminosity{ 0, 0, 0, 0 };
        RenderingBackend::ColorF tint{ 0, 0, 0, 0 };
    };

    AcrylicLayers GetAcrylicLayers(RenderingBackend::ColorF tintColor, float tintOpacity, bool hasLuminosityOpacity, float luminosityOpacity) noexcept;

    // AcrylicBrush backdrop blur radius (MUXC AcrylicBrush::sc_blurRadius), DIPs.
    constexpr float AcrylicBlurRadius = 30.0f;

    // A backdrop blur reads the pixels under it (and up to `margin` around it): when damage touches that area, the
    // whole blurred rect must be repainted. Grows `damage` (appending the affected backdrop rects) until stable and
    // returns true when anything was added. All rects in the same (device) space.
    struct BackdropArea
    {
        RenderingBackend::RectF rect;
        float margin = 0;
    };

    bool ExpandDamageForBackdrops(std::vector<RenderingBackend::RectF>& damage, _In_reads_(count) const BackdropArea* backdrops, uint32_t count) noexcept;

    // Appends a rounded rectangle (clockwise; cubic corners) as one closed figure.
    void AppendRoundedRectangle(const RenderingBackend::RectF& rect, float radiusX, float radiusY,
        std::vector<RenderingBackend::PathVerb>& verbs, std::vector<RenderingBackend::PointF>& points);

    // Appends an open circular arc (center, radius; degrees clockwise from 12 o'clock) as cubic segments.
    void AppendArc(RenderingBackend::PointF center, float radius, float startDegrees, float sweepDegrees,
        std::vector<RenderingBackend::PathVerb>& verbs, std::vector<RenderingBackend::PointF>& points);

    // ---- S19 text ----

    // True when rect is fully inside one of the opaque rects (all in the same coordinate space).
    bool IsCoveredByOpaque(const RenderingBackend::RectF& rect, _In_reads_(count) const RenderingBackend::RectF* opaque, uint32_t count) noexcept;

    // ---- S12 ProgressRing ----

    // Arc (start angle, sweep angle; degrees clockwise from 12 o'clock) drawn by the native ProgressRing.
    // Determinate: sweep = 360 * (value - min) / (max - min). Indeterminate: a growing/shrinking arc rotating once
    // per cycle, driven by timeSeconds.
    struct ArcSpan
    {
        float startDegrees = 0;
        float sweepDegrees = 0;
    };

    ArcSpan GetProgressRingArc(bool indeterminate, double value, double minimum, double maximum, double timeSeconds) noexcept;
}
