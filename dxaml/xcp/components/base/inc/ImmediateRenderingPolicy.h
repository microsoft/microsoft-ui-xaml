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
        // ---- imaging ----
        bool appCompositionBrush = false;       // app XamlCompositionBrushBase subclass (CompositionBrush set by the app)
        bool appXamlLight = false;              // app XamlLight subclass (CompositionLight set by the app)
    };

    // Reason string for the WindowFallback event, or nullptr when the element can be painted immediately.
    const wchar_t* GetFallbackReason(const ElementInterop& interop) noexcept;

    // Snapshot of a hand-off visual (ElementCompositionPreview.GetElementVisual) as polled by the walk. In immediate
    // modes no comp node writes the visual, so every value here was set by the app (or a control).
    struct HandOffVisualState
    {
        float opacity = 1;
        float offset[3] = {};
        float scale[3] = { 1, 1, 1 };
        float rotationAngle = 0;      // radians
        float transformMatrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        float translation[3] = {};   // the "Translation" property set value, when present
        // ---- clips3d ----
        float rotationAxis[3] = { 0, 0, 1 };
        float orientation[4] = { 0, 0, 0, 1 }; // quaternion (x, y, z, w)
        float centerPoint[3] = {};
        float anchorPoint[2] = {};
        float size[2] = {};
        bool animating = false;       // a Composition animation runs on the visual or its clip (values cannot be read)
        bool unsupportedClip = false; // a clip the walk cannot read (path geometry, view box, clip anchor point)
    };

    // True when the hand-off visual needs Composition: a running app animation (its values cannot be read back) or a
    // clip the walk cannot read. Static Offset/Scale/RotationAngle/TransformMatrix/Opacity/Translation and inset,
    // rectangle and geometric (rectangle, rounded rectangle, ellipse) clips are painted by the walk (clips3d).
    bool IsHandOffVisualTouched(const HandOffVisualState& state) noexcept;

    // Static transform of the hand-off visual (row-vector 4x4, element DIPs -> element DIPs, applied before the
    // element's XAML local transform), in Composition's order: -(CenterPoint + AnchorPoint*Size), Scale, Orientation,
    // RotationAngle about RotationAxis, +CenterPoint, TransformMatrix, Offset + Translation. False when identity.
    bool ComputeHandOffVisualTransform(const HandOffVisualState& state, _Out_writes_(16) float matrix[16]) noexcept;

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

    // ---- scrolling ----

    // A scroller's content origin (physical pixels, island space) and world scale in one frame.
    struct ScrollerFrame
    {
        float offsetX = 0;
        float offsetY = 0;
        float zoom = 1;
    };

    // Whether the scroller's previous pixels can be reused by moving them (ScrollPixels): the zoom is unchanged, the
    // content moved by a non-zero whole number of pixels and by less than the viewport. Returns the integer delta.
    // A zoom change (DManip pinch / ZoomToFactor / ScrollPresenter ZoomTo) repaints the viewport instead.
    bool TryGetScrollBlitDelta(const ScrollerFrame& previous, const ScrollerFrame& current, float viewportWidth,
        float viewportHeight, _Out_ int32_t* dx, _Out_ int32_t* dy) noexcept;

    // How a scroller's pixels are presented in a frame: unchanged (no damage from scrolling), moved with ScrollPixels
    // (whole-pixel translation at the same zoom; *dx/*dy set), or repainted (zoom change, sub-pixel or large move).
    enum class ScrollPresentation { Unchanged, Blit, Repaint };
    ScrollPresentation ClassifyScrollerChange(const ScrollerFrame& previous, const ScrollerFrame& current, float viewportWidth,
        float viewportHeight, _Out_ int32_t* dx, _Out_ int32_t* dy) noexcept;

    // ---- DManip ticking (scroll physics) ----

    // In Composition the DComp manipulation compositor ticks DirectManipulation's update manager every frame and
    // applies viewport transforms to the manipulated visuals. Immediate islands have no such visuals, so the immediate
    // frame loop ticks IDirectManipulationUpdateManager::Update itself while a viewport moves, and the walk paints the
    // live content transform. WINUI_IMMEDIATE_DMANIP_TICK=0 (or false/off) restores the previous behaviour for A/B
    // measurement: inertia jumps to its end and animated ChangeView uses the UI-thread tween.
    constexpr const wchar_t* DManipTickVariableName = L"WINUI_IMMEDIATE_DMANIP_TICK";
    bool ParseDManipTickSetting(_In_opt_z_ const wchar_t* text) noexcept;

    // Mirrors XDMViewportStatus (pal\inc\paltypes.h) / DIRECTMANIPULATION_STATUS.
    enum class DManipStatus : uint32_t { Building, Enabled, Disabled, Running, Inertia, Ready, Suspended, AutoRunning };

    // True when DManip moves the viewport's content between frames and needs Update ticks: a contact pans or pinches
    // (Running), inertia, an animated ZoomToRect (wheel, keyboard, BringIntoView, ChangeView) or auto-scroll run.
    bool IsDManipStatusMoving(DManipStatus status) noexcept;

    // Whether the immediate frame loop calls Update this frame: an immediate mode with ticking enabled and at least
    // one moving viewport. False when every viewport is Ready/Enabled/Disabled, so a settled window costs nothing.
    bool ShouldTickDManip(bool immediateMode, bool tickingEnabled, _In_reads_(count) const DManipStatus* statuses, uint32_t count) noexcept;

    // Without ticking an inertial viewport whose element has no composition peer never progresses, so XAML jumps it to
    // the inertia end (CInputServices::StopInertialViewportWithoutCompositorPeer). With immediate ticking it animates.
    bool ShouldStopPeerlessInertia(bool immediateMode, bool tickingEnabled) noexcept;

    // Animated ChangeView/ScrollTo/ZoomTo in immediate modes: DManip's own animation when ticking, otherwise the
    // ScrollViewer UI-thread tween (fallback).
    bool ShouldTweenAnimatedChangeView(bool immediateMode, bool tickingEnabled) noexcept;

    // ---- scrollgaps ----
    // Start latency: DManip starts a wheel/keyboard/ZoomToRect animation's clock at the first Update. Composition's
    // compositor updates on its next tick, so content moves in the first frame; the immediate frame tick alone starts
    // the clock one frame later. So immediate modes call Update right after DManip handled the input (ProcessInput or
    // an animated BringIntoViewport). Not for viewports DManip is not running (Building / Disabled / Suspended).
    bool ShouldUpdateDManipAfterInput(bool immediateMode, bool tickingEnabled, bool handled, DManipStatus status) noexcept;

    // Blit damage of an element that did not move while a scroller's pixels were moved by (dx, dy) inside blit (all
    // physical pixels). cover: where the element draws uniformly (empty: nowhere). A blitted pixel p is wrong only if
    // p or its source p - d is outside cover, so the damage is (bounds ∪ bounds + d) ∩ blit minus cover ∩ (cover + d).
    // A ring (rounded Border, focus rect) around a viewport then damages its ring, not the whole viewport.
    // Writes up to 8 rects; returns the count.
    uint32_t GetBlitDamageOutsideCover(const RenderingBackend::RectF& bounds, const RenderingBackend::RectF& cover,
        const RenderingBackend::RectF& blit, int32_t dx, int32_t dy, _Out_writes_to_(8, return) RenderingBackend::RectF* out) noexcept;

    // Makes damage rects pairwise disjoint (painters sum damaged pixels) and at most maxCount. Overlapping rects are
    // unioned when that adds at most 25% more pixels; otherwise the later one is split around the earlier one
    // (a blit's exposed strip crossing a thin ring stays two thin strips instead of the viewport's bounding box).
    // Over maxCount, the pair whose union grows least is merged.
    void CoalesceDamageRects(_Inout_ std::vector<RenderingBackend::RectF>& rects, uint32_t maxCount);

    // Quantized (1/256) clip edge relative to scrolled content's origin, for content signatures. Unbounded edges
    // (|edge| >= 1e6) hash as a fixed sentinel: "unbounded minus the scroll offset" must not change as content scrolls.
    int64_t QuantizeRelativeClipEdge(float edge, float origin) noexcept;

    // Gap 1 (opt-in, WINUI_IMMEDIATE_OFFTHREAD_SCROLL=1): while the UI thread is blocked during DManip inertia, the render
    // thread translates the scroller's content, walked by the last UI frame with an overscan clip, along an extrapolated
    // approach to DManip's inertia end. (DManip itself cannot be ticked off the UI thread: Update returns UI_E_WRONG_THREAD.)
    bool IsOffThreadScrollEnabled(_In_opt_z_ const wchar_t* value) noexcept;
    // The walk clip of a moving scroller's content (DIPs): the viewport grown by `extent` viewports on every side.
    RenderingBackend::RectF ComputeOffThreadOverscanClip(const RenderingBackend::RectF& viewport, float extent) noexcept;
    // Inertia displacement (DIPs, whole device pixels at `scale`) `elapsedSeconds` after a walk that was `remaining` DIPs
    // short of the inertia end: remaining (1 - e^(-decay t)), clamped to ±(maxDx, maxDy) (the overscan).
    void ComputeOffThreadInertiaDelta(RenderingBackend::PointF remaining, double elapsedSeconds, float decay, float maxDx, float maxDy,
        float scale, _Out_ RenderingBackend::PointF* delta) noexcept;

    // ---- imaging ----

    // Image.NineGrid (DIPs).
    struct NineGridInsets
    {
        float left = 0, top = 0, right = 0, bottom = 0;
    };

    struct NineGridCell
    {
        RenderingBackend::RectF dest;   // element DIPs
        RenderingBackend::RectF source; // bitmap pixels
    };

    // Splits a nine-grid image stretched over `dest` into up to nine sub-bitmap draws, like CompositionNineGridBrush with
    // XAML's settings (WUCBrushManager::GetNineGridBrush): source insets are the insets times the realization scale
    // (pixels), destination insets are the insets in DIPs; corners keep their size, edges stretch along one axis and the
    // center along both. Insets that do not fit (left + right wider than the source or the destination) shrink
    // proportionally. Returns the number of non-empty cells written.
    uint32_t ComputeNineGridCells(const RenderingBackend::RectF& dest, float sourceWidth, float sourceHeight, const NineGridInsets& insets,
        float realizationScaleX, float realizationScaleY, _Out_writes_(9) NineGridCell* cells) noexcept;
}
