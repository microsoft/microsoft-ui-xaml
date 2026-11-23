// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ImmediatePainter.h>
#include <cstdint>
#include <vector>

class CBrush;
class CShape;

// F1 shapes and strokes, F2 brushes for the immediate render walk (ImmediateRenderer.Shapes.cpp): reads XAML brushes
// (solid, LinearGradientBrush, MUXC RadialGradientBrush) and Shape stroke properties into painter brush / stroke style
// descriptions. See docs/design-notes/immediate-rendering-backends-proposal.md, "Full XAML coverage: shapes".
namespace ImmediateRenderingShapes
{
    // A brush and optional stroke style owned by a display-list entry. The descriptor pointers (stops, dashes) are
    // bound when the command is replayed, so a Paint can be moved and copied freely.
    struct Paint
    {
        RenderingBackend::BrushDesc brush;
        std::vector<RenderingBackend::GradientStop> stops;
        RenderingBackend::StrokeStyleDesc style;
        std::vector<float> dashes;
        bool hasStyle = false;

        bool IsGradient() const noexcept { return brush.kind != RenderingBackend::BrushKind::Solid; }
        RenderingBackend::BrushDesc Bind(float opacity) const noexcept;
        const RenderingBackend::StrokeStyleDesc* BindStyle(_Out_ RenderingBackend::StrokeStyleDesc* storage) const noexcept;
        uint64_t Hash() const noexcept;
    };

    enum class PaintKind
    {
        NotHandled, // not a brush handled here
        None,       // draws nothing (transparent, no stops)
        Paint,
    };

    // SolidColorBrush, LinearGradientBrush (spread, interpolation, Transform/RelativeTransform) and MUXC
    // RadialGradientBrush, mapped to `bounds` (the drawing's local bounds, used by RelativeToBoundingBox) in the drawing's
    // DIP space. `detail` receives an unsupported-content detail to report, or nullptr.
    PaintKind ResolvePaint(_In_opt_ CBrush* brush, const RenderingBackend::RectF& bounds, float opacity, _Out_ Paint* paint,
        _Outptr_result_maybenull_ const wchar_t** detail);

    // True for brushes ResolvePaint resolves to a gradient (callers that used the average stop color switch to Paint).
    bool IsGradientBrush(_In_opt_ CBrush* brush) noexcept;

    // Shape stroke caps, join, miter limit and dashes (StrokeDashArray scaled by `thickness`, like Composition) into
    // paint->style. Leaves hasStyle false when the stroke uses the defaults (flat caps, miter 10 joins, no dashes).
    void ResolveStrokeStyle(_In_ CShape* shape, float thickness, _Inout_ Paint* paint);

    // How far (DIPs) a stroke of `thickness` with `paint`'s style can reach past its geometry: joins and caps.
    float StrokeReach(const Paint& paint, float thickness) noexcept;
}
