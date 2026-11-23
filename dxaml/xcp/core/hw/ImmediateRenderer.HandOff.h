// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ImmediatePainter.h>
#include <ImmediateRenderingPolicy.h>
#include <vector>

class CUIElement;

// clips3d: static properties of an element's hand-off visual (ElementCompositionPreview.GetElementVisual), read by the
// immediate walk (painted natively) and by the per-window fallback policy (ImmediateRenderer.Fallback.cpp). In
// immediate modes no comp node writes the hand-off visual, so every value was set by the app or a control.
namespace ImmediateRenderingHandOff
{
    struct VisualInfo
    {
        ImmediateRenderingPolicy::HandOffVisualState state;
        bool isVisible = true;
        const wchar_t* animatedProperty = nullptr; // first property found with a running animation

        // Clip (InsetClip, RectangleClip, GeometricClip with a rectangle, rounded rectangle or ellipse geometry), as a
        // closed path in clip space; clipTransform maps clip space to the element's local space (before the visual's
        // own transform, like Composition).
        bool hasClip = false;
        bool clipIsRect = false;                  // clipRect alone describes the clip (no corner radii)
        RenderingBackend::RectF clipRect;
        std::vector<RenderingBackend::PathVerb> clipVerbs;
        std::vector<RenderingBackend::PointF> clipPoints;
        RenderingBackend::Matrix3x2F clipTransform;
    };

    // Reads the element's hand-off visual; false when it has none. width/height: the element's render size (InsetClip
    // insets apply to it: no comp node keeps the visual's Size in sync). checkAnimations: probe the visual and its
    // clip for running animations (ICompositionObject4::TryGetAnimationController; expression animations are not
    // reported by Composition).
    bool Read(_In_ CUIElement* element, float width, float height, bool checkAnimations, _Out_ VisualInfo* info) noexcept;

    // Appends a closed rounded rectangle (per-corner radii: top-left, top-right, bottom-right, bottom-left; x then y).
    void AppendRoundedRect(const RenderingBackend::RectF& rect, const float radii[8],
        _Inout_ std::vector<RenderingBackend::PathVerb>& verbs, _Inout_ std::vector<RenderingBackend::PointF>& points);
}
