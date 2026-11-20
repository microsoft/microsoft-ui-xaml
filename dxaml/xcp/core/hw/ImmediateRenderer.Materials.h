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

    // Reads a custom (non-built-in) dependency property, e.g. MUXC AcrylicBrush.TintColor. Fails safe.
    bool TryGetCustomValue(_In_ CDependencyObject* object, _In_ const xstring_ptr& typeName, _In_ const xstring_ptr& propertyName, _Out_ CValue* value) noexcept;
}
