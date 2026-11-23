// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <memory>
#include <RenderingBackend.h>

struct ID3D11Device;
struct IDXGISwapChain1;
struct IDWriteFontFace;

// Shared painter contract for the immediate-rendering prototype backends (GDI+, D2D hardware, D2D WARP).
// See docs/design-notes/immediate-rendering-backends-proposal.md. The primitive set is deliberately limited to what the
// S1 (settings) and S2 (dashboard) bench scenes need: solid and rounded fills, rounded strokes, text, rectangular clips,
// 2D transforms, and group opacity. Painters draw to an offscreen premultiplied BGRA target; presentation is separate.
namespace RenderingBackend
{
    struct ColorF
    {
        float r = 0, g = 0, b = 0, a = 1; // straight (non-premultiplied) alpha
    };

    struct RectF
    {
        float left = 0, top = 0, right = 0, bottom = 0;
    };

    // Row-vector affine matrix, same layout as D2D1_MATRIX_3X2_F and Gdiplus::Matrix(m11, m12, m21, m22, dx, dy).
    struct Matrix3x2F
    {
        float m11 = 1, m12 = 0, m21 = 0, m22 = 1, dx = 0, dy = 0;
    };

    struct TextStyle
    {
        const wchar_t* fontFamily = L"Segoe UI";
        float fontSize = 14;      // DIPs
        uint32_t fontWeight = 400; // DWRITE_FONT_WEIGHT / CSS weight
    };

    struct PainterOptions
    {
        uint32_t width = 0;   // physical pixels
        uint32_t height = 0;  // physical pixels
        float dpiScale = 1.0f;
    };

    struct PointF
    {
        float x = 0, y = 0;
    };

    enum class PathVerb : uint8_t
    {
        MoveTo,  // 1 point
        LineTo,  // 1 point
        CubicTo, // 3 points: control1, control2, end
        Close,   // 0 points
    };

    struct PathData
    {
        const PathVerb* verbs = nullptr;
        uint32_t verbCount = 0;
        const PointF* points = nullptr; // DIPs, in verb order
        uint32_t pointCount = 0;
        bool nonZeroFill = false;       // false: even-odd (alternate)
        // Nonzero: the caller promises identical verbs/points for the same key, so painters may cache the realized
        // geometry. 0: no caching.
        uint64_t cacheKey = 0;
    };

    // Same layout as DWRITE_GLYPH_OFFSET.
    struct GlyphOffset
    {
        float advanceOffset = 0, ascenderOffset = 0;
    };

    struct GlyphRunDesc
    {
        IDWriteFontFace* fontFace = nullptr;
        float fontEmSize = 0;                  // DIPs
        const uint16_t* glyphIndices = nullptr;
        const float* glyphAdvances = nullptr;  // DIPs; required
        const GlyphOffset* glyphOffsets = nullptr; // optional
        uint32_t glyphCount = 0;
        PointF baselineOrigin;                 // DIPs
        uint32_t bidiLevel = 0;                // odd = right-to-left
        // S19: color fonts (COLR/CPAL emoji) draw their color layers; the run color is used for the foreground layer
        // (palette index 0xFFFF). Painters detect color fonts themselves (IDWriteFontFace2::IsColorFont).
        bool useColorFont = true;
        // S19: ClearType (subpixel) antialiasing when the target is opaque under the run; grayscale otherwise.
        bool clearType = false;
    };

    struct GradientStop
    {
        float offset = 0; // 0..1
        ColorF color;
    };

    // S13/S23: LinearGradientBrush. Points are in the same DIP space as the filled shape (callers map
    // MappingMode=RelativeToBoundingBox to absolute). Clamp extend mode; sRGB interpolation, like Composition.
    struct LinearGradientDesc
    {
        PointF start, end;
        const GradientStop* stops = nullptr;
        uint32_t stopCount = 0;
        float opacity = 1;
    };

    // ---- shapes (F1 shapes and strokes, F2 brushes) ----

    // XAML PenLineCap. Triangle: a point of height thickness/2 beyond the end.
    enum class LineCap : uint8_t
    {
        Flat,
        Square,
        Round,
        Triangle,
    };

    // XAML PenLineJoin. Miter: clipped at miterLimit (D2D1_LINE_JOIN_MITER).
    enum class LineJoin : uint8_t
    {
        Miter,
        Bevel,
        Round,
    };

    // Shape stroke style, like Composition's sprite shape stroke. Lengths are in DIPs: the walk scales
    // Shape.StrokeDashArray and StrokeDashOffset by StrokeThickness, as XAML does for Composition.
    struct StrokeStyleDesc
    {
        LineCap startCap = LineCap::Flat;
        LineCap endCap = LineCap::Flat;
        LineCap dashCap = LineCap::Flat;
        LineJoin lineJoin = LineJoin::Miter;
        float miterLimit = 10;             // ratio of the miter length to half the thickness (D2D); clamped to >= 1
        const float* dashes = nullptr;     // alternating dash and gap lengths, DIPs; nullptr or 0 count: solid
        uint32_t dashCount = 0;
        float dashOffset = 0;              // DIPs into the dash pattern where the stroke starts
    };

    enum class GradientSpread : uint8_t
    {
        Pad,     // D2D1_EXTEND_MODE_CLAMP
        Reflect, // D2D1_EXTEND_MODE_MIRROR
        Repeat,  // D2D1_EXTEND_MODE_WRAP
    };

    enum class GradientInterpolation : uint8_t
    {
        SRgb,   // gamma 2.2 (sRGB) interpolation, like Composition's default
        Linear, // linear-light (scRGB) interpolation, D2D1_GAMMA_1_0
    };

    enum class BrushKind : uint8_t
    {
        Solid,
        Linear,
        Radial,
    };

    // A solid, linear or radial gradient brush. Gradient geometry is in brush space; `transform` maps brush space to
    // the drawing's DIP space (the space of the path / rect / glyph run it paints). Stops are sorted by offset.
    // Radial: the ellipse (center, radiusX, radiusY) is offset 1; offset 0 is the focal point `origin`, which may
    // differ from the center (D2D/Composition radial gradient semantics).
    struct BrushDesc
    {
        BrushKind kind = BrushKind::Solid;
        ColorF color;                      // Solid
        const GradientStop* stops = nullptr;
        uint32_t stopCount = 0;
        PointF start, end;                 // Linear
        PointF center, origin;             // Radial
        float radiusX = 0, radiusY = 0;    // Radial
        GradientSpread spread = GradientSpread::Pad;
        GradientInterpolation interpolation = GradientInterpolation::SRgb;
        Matrix3x2F transform;
        float opacity = 1;
    };

    struct BitmapSource
    {
        uint64_t key = 0;        // caller-chosen identity (for example the decoded image surface)
        uint64_t generation = 0; // bump when the pixels change
        uint32_t width = 0, height = 0, stride = 0;
        const uint8_t* pixels = nullptr; // premultiplied BGRA8
    };

    struct DeviceIdentity
    {
        Mode mode = Mode::Composition;
        uint64_t adapterLuid = 0; // (HighPart << 32) | LowPart; 0 for CPU-only painters
        bool isWarp = false;
        uint32_t featureLevel = 0; // D3D_FEATURE_LEVEL; 0 for CPU-only painters
    };

    struct FrameStats
    {
        uint64_t damagedPixels = 0;
        uint64_t paintedPixels = 0;
        uint32_t primitiveCount = 0;   // fills, strokes, and text runs issued this frame
        uint32_t damageRectCount = 0;
    };

    struct ResourceInventory
    {
        uint64_t surfaceBytes = 0;          // render targets / backbuffers owned by the painter
        uint64_t cacheBytes = 0;            // glyph, geometry, brush caches (best estimate)
        uint32_t cachedObjectCount = 0;
        uint64_t uploadBytesThisFrame = 0;  // CPU->GPU or CPU->surface copies attributable to the last frame
    };

    class IImmediatePainter
    {
    public:
        virtual ~IImmediatePainter() = default;

        virtual DeviceIdentity GetDeviceIdentity() const noexcept = 0;
        virtual HRESULT Resize(uint32_t width, uint32_t height, float dpiScale) noexcept = 0;

        // damageRects (physical pixels) == nullptr / damageRectCount == 0 means the whole target is damaged and is
        // reported as one damage rect. All drawing, including Clear, is clipped to the damage; pixels outside it keep
        // the previous frame's content. Clear does not count as a primitive. Drawing coordinates are DIPs.
        virtual HRESULT BeginFrame(_In_reads_opt_(damageRectCount) const RectF* damageRects, uint32_t damageRectCount) noexcept = 0;
        virtual void Clear(const ColorF& color) noexcept = 0;
        virtual void FillRectangle(const RectF& rect, const ColorF& color) noexcept = 0;
        virtual void FillRoundedRectangle(const RectF& rect, float radiusX, float radiusY, const ColorF& color) noexcept = 0;
        // The stroke is centered on rect's edge, matching D2D; Border layout insets rect by thickness/2.
        virtual void StrokeRoundedRectangle(const RectF& rect, float radiusX, float radiusY, float thickness, const ColorF& color) noexcept = 0;
        virtual void DrawText(_In_reads_(length) const wchar_t* text, uint32_t length, const TextStyle& style, const RectF& layoutRect, const ColorF& color) noexcept = 0;
        virtual void PushAxisAlignedClip(const RectF& clip) noexcept = 0;
        virtual void PopAxisAlignedClip() noexcept = 0;
        // Replaces the current transform (callers compose); identity is the default at BeginFrame.
        virtual void SetTransform(const Matrix3x2F& transform) noexcept = 0;
        // Group opacity: children are composited together, then blended once.
        virtual void PushOpacityLayer(float opacity) noexcept = 0;
        virtual void PopOpacityLayer() noexcept = 0;

        // ---- Extended primitives for scenes S3-S10 (see test plan "Extended scenes") ----

        // Antialiased geometric clip (avatars, CornerRadius clipping). Shares one stack with axis-aligned clips and
        // opacity layers; pushes and pops must nest.
        virtual void PushRoundedRectangleClip(const RectF& clip, float radiusX, float radiusY) noexcept = 0;
        virtual void PopRoundedRectangleClip() noexcept = 0;

        // Arbitrary paths (Path elements, glyph-like icons, ProgressRing arcs). The render loop flattens arcs and
        // quadratic segments to cubics, so painters only see these verbs.
        virtual void FillPath(const PathData& path, const ColorF& color) noexcept = 0;
        // Stroke centered on the path, flat caps, miter joins.
        virtual void StrokePath(const PathData& path, float thickness, const ColorF& color) noexcept = 0;

        // Text drawn from XAML's own DirectWrite layout (exact glyph positions), instead of re-laying out a string.
        // GDI+ draws the same glyph indices with GDI+ (DrawDriverString), keeping the "GDI+ text" decision.
        virtual void DrawGlyphRun(const GlyphRunDesc& run, const ColorF& color) noexcept = 0;

        // Images (S5). EnsureBitmap creates or refreshes the painter-owned copy of an image the first time a
        // (key, generation) pair is seen, outside or inside a frame; later calls are no-ops. Pixels are premultiplied
        // BGRA8. Bytes copied into painter-owned storage count in ResourceInventory::uploadBytesThisFrame of the frame
        // they happen in (or the next frame when called outside one); bitmap storage counts in cacheBytes.
        virtual HRESULT EnsureBitmap(const BitmapSource& source) noexcept = 0;
        // sourceRect is in bitmap pixels (nullptr = whole bitmap); linear filtering; no-op for unknown keys.
        virtual void DrawBitmap(uint64_t key, const RectF& destination, _In_opt_ const RectF* sourceRect, float opacity) noexcept = 0;
        virtual void ReleaseBitmap(uint64_t key) noexcept = 0;

        // Scrolling (S3, S8). Call before BeginFrame: moves the pixels of rect (physical pixels, integer-aligned) by
        // (dx, dy) inside the painter's target, clipped to rect. The caller must include the exposed strip in the
        // frame's damage. The moved area is presented with the next frame (it counts as presented area and in
        // uploadBytesThisFrame) even though it is not repainted. Returns E_ILLEGAL_METHOD_CALL inside a frame.
        virtual HRESULT ScrollPixels(const RectF& rect, int32_t dx, int32_t dy) noexcept = 0;

        // ---- Primitives for scenes S11-S23 (see test plan "Full-backend scenes") ----

        // Gradient fills (S13 acrylic tint/luminosity, S23 shells). radiusX/radiusY = 0 is a plain rectangle.
        virtual void FillRoundedRectangleGradient(const RectF& rect, float radiusX, float radiusY, const LinearGradientDesc& gradient) noexcept = 0;
        virtual void FillPathGradient(const PathData& path, const LinearGradientDesc& gradient) noexcept = 0;

        // S13: drop shadow of a rounded rectangle (ThemeShadow, DropShadow, Flyout/ContentDialog elevation).
        // Gaussian blur with standard deviation blurRadius/2 (DIPs) of the shape offset by offset, filled with color.
        // The shadow is not drawn under the shape itself (knockout), matching ThemeShadow's casting behavior.
        virtual void DrawRoundedRectangleShadow(const RectF& rect, float radiusX, float radiusY, float blurRadius, const PointF& offset, const ColorF& color) noexcept = 0;

        // S13: in-app backdrop blur (AcrylicBrush BackgroundSource=Backdrop). Gaussian-blurs (standard deviation
        // blurRadius/2, DIPs) the pixels already painted this frame inside the rounded rect, in place. The caller then
        // draws tint/luminosity/noise on top. Pixels outside the damage are treated as already painted, so the caller
        // must include the blur's full bounds (rect inflated by 3*sigma) in the damage when anything under it changes.
        virtual void BlurBackdrop(const RectF& rect, float radiusX, float radiusY, float blurRadius) noexcept = 0;

        // ---- clips3d (F3 clips and transforms, F4 3D; test plan "XAML coverage (full backends)") ----

        // Antialiased clip to a path (DIPs, under the current transform; non-rectangular UIElement.Clip, rectangle
        // clips under rotation or skew, Composition geometric clips). Intersects with the clips already pushed.
        virtual void PushGeometryClip(const PathData& path) noexcept = 0;
        virtual void PopGeometryClip() noexcept = 0;

        // 3D (PlaneProjection, Matrix3DProjection, Transform3D, 3D facades). Drawing between push and pop goes to an
        // offscreen layer in layer DIPs (the transform is reset to identity; the layer is the target's size, so only
        // `bounds`, which must fit in it, is kept). On pop, the layer content inside `bounds` is composited through
        // `matrix` (row-vector 4x4, D2D1_MATRIX_4X4_F layout, layer DIPs -> DIPs of the transform active at push;
        // x' = (x*m11 + y*m21 + m41) / w, w = x*m14 + y*m24 + m44) and then through that transform, under the clips and
        // damage active at push. Points with w <= 0 are not drawn. The transform active at push is restored on pop.
        struct Matrix4x4F
        {
            float m[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
        };
        virtual void PushPerspectiveLayer(const RectF& bounds, const Matrix4x4F& matrix) noexcept = 0;
        virtual void PopPerspectiveLayer() noexcept = 0;

        virtual HRESULT EndFrame(_Out_ FrameStats* stats) noexcept = 0;

        // Copies the current target as premultiplied BGRA8. For tests and fidelity capture, not the present path.
        virtual HRESULT ReadPixels(_Out_writes_bytes_(bufferSize) uint8_t* buffer, uint32_t stride, uint32_t bufferSize) noexcept = 0;
        virtual ResourceInventory GetResourceInventory() const noexcept = 0;

        // Presentation (decision: one composition swap chain per root for every immediate mode). Attaches, or with
        // nullptr detaches, the root's swap chain (DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED, created
        // with CreateSwapChainForComposition by the render loop). With a swap chain attached, EndFrame copies only the
        // frame's damage rects from the painter's target into the back buffer and calls Present1 with those dirty
        // rects; ResourceInventory::uploadBytesThisFrame reports that copy. Without one, frames are offscreen only and
        // report 0. D2D painters require the swap chain to be on their own device (E_INVALIDARG otherwise); the GDI+
        //         painter accepts a swap chain on any D3D11 device and copies with UpdateSubresource. Resize also resizes the attached
                // swap chain's buffers (ResizeBuffers); the next frame after attach or Resize is treated as fully damaged.
        virtual HRESULT SetPresentationSwapChain(_In_opt_ IDXGISwapChain1* swapChain) noexcept = 0;

        // ---- shapes (F1 shapes and strokes, F2 brushes) ----

        // Brush fills: a rounded rect (radiusX/radiusY = 0: plain rectangle; radii of half the size: an ellipse) or a
        // path. A Solid brush is equivalent to the color fills.
        virtual void FillRoundedRectangleBrush(const RectF& rect, float radiusX, float radiusY, const BrushDesc& brush) noexcept = 0;
        virtual void FillPathBrush(const PathData& path, const BrushDesc& brush) noexcept = 0;

        // Styled strokes, centered on the outline. style nullptr: solid, flat caps, miter joins (limit 10), the same as
        // StrokeRoundedRectangle / StrokePath. A closed rounded rect or ellipse outline starts (for dashes) where D2D's
        // rectangle, rounded-rectangle and ellipse geometries start: the top-left corner, the start of the top-left arc
        // (left, top + radiusY), and the rightmost point, clockwise.
        virtual void StrokeRoundedRectangleBrush(const RectF& rect, float radiusX, float radiusY, float thickness,
            const BrushDesc& brush, _In_opt_ const StrokeStyleDesc* style) noexcept = 0;
        virtual void StrokePathBrush(const PathData& path, float thickness, const BrushDesc& brush,
            _In_opt_ const StrokeStyleDesc* style) noexcept = 0;

        // Glyph run painted with a brush (gradient text), grayscale antialiased. Color fonts are drawn monochrome with
        // the brush (XAML draws color glyphs with their palette only for solid foregrounds).
        virtual void DrawGlyphRunBrush(const GlyphRunDesc& run, const BrushDesc& brush) noexcept = 0;
        // ---- text ----
        // F6: CompositeMode DestInvert (the TextBox caret). Composites the rect, filled with color under the current
        // transform and clip (antialiased edges), with D2D1_COMPOSITE_MODE_MASK_INVERT over the pixels already painted:
        // O_rgb = (1 - D_rgb) * S + (1 - S_a) * D_rgb (premultiplied), destination alpha unchanged. An opaque white
        // source inverts the destination.
        virtual void InvertRectangle(const RectF& rect, const ColorF& color) noexcept = 0;
    };

    // Dispatches to the per-backend factories below. Composition is not an immediate painter (E_INVALIDARG).
    HRESULT CreateImmediatePainter(Mode mode, const PainterOptions& options, _Out_ std::unique_ptr<IImmediatePainter>& painter) noexcept;

    // Per-backend factories, each in its own source file so backends evolve independently.
    HRESULT CreateGdiPlusPainter(const PainterOptions& options, _Out_ std::unique_ptr<IImmediatePainter>& painter) noexcept;          // GdiPlusPainter.cpp
    HRESULT CreateD2DPainter(Mode mode, _In_ ID3D11Device* device, const PainterOptions& options, _Out_ std::unique_ptr<IImmediatePainter>& painter) noexcept; // D2DPainter.cpp

    // D3D device for the D2D modes (D2DDeviceFactory.cpp). ImmediateD2DSoftware => WARP. ImmediateD2DHardware => a
    // hardware adapter; must fail with DXGI_ERROR_NOT_FOUND rather than silently returning WARP or the Basic Render
    // Driver when no hardware adapter exists.
    HRESULT CreateD3DDeviceForMode(Mode mode, _Outptr_ ID3D11Device** device, _Out_ DeviceIdentity* identity) noexcept;

    // Availability probe registered with SetImmediateModeProbe by the core: creates a small painter (and, for D2D, its
    // device) and maps failures to DeviceUnavailable (no device) or NotImplemented (no painter).
    FallbackReason ProbeImmediateMode(Mode mode) noexcept;

    // Brackets one root's paint with PaintBegin/PaintEnd and emits ResourceInventory, so every backend reports the same
    // events. paint is invoked between BeginFrame and EndFrame; returns the first failure.
    template <typename PaintFn>
    HRESULT PaintTracedFrame(
        IImmediatePainter& painter,
        uint32_t frameNumber,
        uint64_t rootId,
        _In_reads_opt_(damageRectCount) const RectF* damageRects,
        uint32_t damageRectCount,
        PaintFn&& paint,
        _Out_opt_ FrameStats* statsOut = nullptr) noexcept
    {
        TracePaintBegin(frameNumber, rootId);
        FrameStats stats{};
        HRESULT hr = painter.BeginFrame(damageRects, damageRectCount);
        if (SUCCEEDED(hr))
        {
            paint(painter);
            hr = painter.EndFrame(&stats);
        }
        TracePaintEnd(frameNumber, rootId, stats.damagedPixels, stats.paintedPixels, stats.primitiveCount, stats.damageRectCount);
        const auto inventory = painter.GetResourceInventory();
        TraceResourceInventory(painter.GetDeviceIdentity().mode, inventory.surfaceBytes, inventory.cacheBytes,
            inventory.cachedObjectCount, inventory.uploadBytesThisFrame);
        if (statsOut)
        {
            *statsOut = stats;
        }
        return hr;
    }
}
