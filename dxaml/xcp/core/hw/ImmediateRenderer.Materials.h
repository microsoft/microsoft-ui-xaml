// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ImmediatePainter.h>
#include <ImmediateRenderingPolicy.h>
#include <vector>

class CBrush;
class CUIElement;
class CDependencyObject;
class CValue;

// S13 materials for the immediate render walk (ImmediateRenderer.Materials.cpp): reads XAML brushes and shadow casters
// into painter primitives. The walk (ImmediateRenderer.cpp) owns the display list and emits what these return.
namespace ImmediateRenderingMaterials
{
    enum class FillKind
    {
        NotHandled, // not a brush handled here
        None,       // draws nothing (transparent)
        Solid,      // degenerate gradient: one color
        Gradient,
    };

    struct GradientFill
    {
        RenderingBackend::PointF start, end; // in the filled shape's local DIP space
        std::vector<RenderingBackend::GradientStop> stops;
        float opacity = 1;
        const wchar_t* detail = nullptr;      // unsupported-content detail to report, or nullptr
    };

    // LinearGradientBrush mapped to `bounds` (the shape's local bounds). RadialGradientBrush is NotHandled.
    FillKind ResolveLinearGradient(_In_opt_ CBrush* brush, const RenderingBackend::RectF& bounds, float opacity,
        _Out_ GradientFill* gradient, _Out_ RenderingBackend::ColorF* solid);

    struct AcrylicFill
    {
        bool blur = false;                  // false: paint `fallback` only (AlwaysUseFallback / high contrast)
        RenderingBackend::ColorF luminosity;
        RenderingBackend::ColorF tint;
        RenderingBackend::ColorF fallback;
        // FallbackColor made opaque (times the brush opacity): MUXC blends the backdrop onto it before blurring, which
        // is all that shows where nothing is painted beneath (a windowed popup's own surface).
        RenderingBackend::ColorF opaqueFallback;
        const wchar_t* detail = nullptr;      // unsupported-content detail to report, or nullptr
    };

    // MUXC AcrylicBrush (in-app acrylic). False when the brush is not an AcrylicBrush.
    bool ResolveAcrylic(_In_opt_ CBrush* brush, float opacity, _Out_ AcrylicFill* acrylic);

    struct ThemeShadowFill
    {
        uint32_t count = 0;
        ImmediateRenderingPolicy::ShadowLayer layers[2];
        float radiusX = 0;
        float radiusY = 0;
    };

    // ThemeShadow drop shadow for `element` (a caster, or the child of a Popup caster), like
    // HWCompTreeNodeWinRT::UpdateDropShadowVisual. False when the element casts no visible shadow.
    bool ResolveThemeShadow(_In_ CUIElement* element, _Out_ ThemeShadowFill* shadow);

    // S12: the built-in ProgressRing Lottie drawn natively, as stroked arcs in the player's local DIP space.
    struct PathData
    {
        std::vector<RenderingBackend::PathVerb> verbs;
        std::vector<RenderingBackend::PointF> points;
    };
    struct ProgressRingFill
    {
        float centerX = 0;
        float centerY = 0;
        float radius = 0;
        float thickness = 0;
        RenderingBackend::ColorF foreground;
        RenderingBackend::ColorF background;
        PathData track;          // full circle in `background`
        PathData arc;            // progress / spinning arc in `foreground`
        bool animating = false;  // indeterminate and active: needs another frame
    };

    // `player` is the AnimatedVisualPlayer in ProgressRing's template (IsBuiltInProgressRingPlayer). False when the
    // ring draws nothing (inactive, empty).
    bool ResolveProgressRing(_In_ CUIElement* player, double timeSeconds, _Out_ ProgressRingFill* ring);

    // Reads a custom (non-built-in) dependency property, e.g. MUXC AcrylicBrush.TintColor. Fails safe.
    bool TryGetCustomValue(_In_ CDependencyObject* object, _In_ const xstring_ptr& typeName, _In_ const xstring_ptr& propertyName, _Out_ CValue* value) noexcept;

    // ---- imaging ----
    // F11: an app XamlCompositionBrushBase subclass (its CompositionBrush is app Composition content). Built-in
    // subclasses (AcrylicBrush, RevealBrush, RadialGradientBrush, ColorPicker's SpectrumBrush, the brush owned by
    // MonochromaticOverlayPresenter owner) are not: they are painted natively or with their fallback color.
    bool IsAppCompositionBrush(_In_opt_ CBrush* brush, _In_opt_ CUIElement* owner) noexcept;
}
