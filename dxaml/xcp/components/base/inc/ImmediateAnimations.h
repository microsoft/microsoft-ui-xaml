// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <vector>

// F10 (immediate-rendering prototype, branch "transitions"): XAML-owned animations that Composition would run
// (UIElement Scalar/Vector3 transitions, BrushTransition, ConnectedAnimation) evaluated by XAML itself when the window
// renders with an immediate backend. Curves and durations match the Composition animations XAML builds
// (components\comptree\SharedTransitionAnimations.cpp). XAML-type free so it can be unit tested in isolation.
// App-supplied Composition animations (UIElement.StartAnimation, implicit show/hide) cannot be read back and keep the
// per-window fallback. See docs/design-notes/immediate-rendering-backends-proposal.md, "Full XAML coverage: transitions".
namespace ImmediateAnimations
{
    // Composition CubicBezierEasingFunction(control point 1, control point 2): y for progress x in [0, 1].
    float EvaluateCubicBezier(float x1, float y1, float x2, float y2, float x) noexcept;

    struct Curve
    {
        bool linear = true;
        float x1 = 0, y1 = 0, x2 = 1, y2 = 1;

        float Evaluate(float progress) const noexcept;
        static Curve Linear() noexcept { return {}; }
        static Curve Bezier(float x1, float y1, float x2, float y2) noexcept { return { false, x1, y1, x2, y2 }; }
        // Scalar/Vector3Transition (SharedTransitionAnimations::EnsureEasingFunction).
        static Curve PropertyTransition() noexcept { return Bezier(0.8f, 0.0f, 0.2f, 1.0f); }
    };

    // SharedTransitionAnimations::SetAnimationDuration clamps to Composition's [1 ms, 24 days]. In: 100 ns ticks; out: us.
    int64_t ClampDurationMicroseconds(int64_t ticks) noexcept;

    enum class Channel : uint8_t
    {
        Opacity,          // 1 component
        Rotation,         // 1 component, degrees
        Scale,            // 3 components
        Translation,      // 3 components
        BackgroundColor,  // 4 components, A R G B in [0, 255] (BrushTransition, linear RGB interpolation)
        Count
    };

    constexpr uint32_t ComponentCount(Channel channel) noexcept
    {
        return channel == Channel::BackgroundColor ? 4 : (channel == Channel::Scale || channel == Channel::Translation) ? 3 : 1;
    }

    // Diagnostics/tests (WINUI_IMMEDIATE_ANIMATION_HOLD=<0..1>): every XAML-owned animation is held at this progress and
    // never completes, so captures are deterministic. Read once.
    bool TryGetHoldProgress(_Out_ float* progress) noexcept;

    // Linear progress in [0, 1] (honors the hold); *finished is true once the animation reached its end.
    float GetProgress(int64_t startUs, int64_t durationUs, int64_t nowUs, _Out_opt_ bool* finished = nullptr) noexcept;

    // Monotonic microseconds (QPC).
    int64_t NowMicroseconds() noexcept;

    // Running property transitions keyed by (owner, channel). Owners are opaque (CUIElement*). One store per UI thread;
    // entries are a few dozen bytes and live only while a transition runs.
    class TransitionStore
    {
    public:
        // Starts a transition to `to`. Its start value is the current animated value when a transition of the channel
        // is still running (Composition's "this.StartingValue" hand off), else `from`. Components whose bit is clear in
        // animateMask snap to `to` (Vector3Transition.Components).
        void Start(const void* owner, Channel channel, _In_reads_(4) const float* from, _In_reads_(4) const float* to,
            uint32_t animateMask, int64_t durationUs, int64_t nowUs, const Curve& curve);
        void Cancel(const void* owner, Channel channel) noexcept;
        void RemoveOwner(const void* owner) noexcept;

        // Current value; false when no transition of the channel is running (the static value applies).
        bool TryGetValue(const void* owner, Channel channel, int64_t nowUs, _Out_writes_(4) float* value) const noexcept;
        bool HasOwner(const void* owner) const noexcept;
        bool Empty() const noexcept { return m_entries.empty(); }
        size_t Size() const noexcept { return m_entries.size(); }

        // Drops finished transitions; returns true when any is still running.
        bool Prune(int64_t nowUs) noexcept;

    private:
        struct Entry
        {
            const void* owner = nullptr;
            Channel channel = Channel::Opacity;
            float from[4] = {};
            float to[4] = {};
            int64_t startUs = 0;
            int64_t durationUs = 0;
            Curve curve;
        };

        static void Evaluate(const Entry& entry, int64_t nowUs, _Out_writes_(4) float* value) noexcept;
        std::vector<Entry> m_entries;
    };

    TransitionStore& GetThreadStore() noexcept;

    // Whether an element's window renders with the immediate walk (registered by the core renderer: false once the
    // element's island fell back to Composition). Without a registration: the process selection.
    using IsTargetImmediateFn = bool (*)(_In_ const void* element);
    void SetIsTargetImmediateCallback(IsTargetImmediateFn callback) noexcept;
    bool IsTargetImmediate(_In_ const void* element) noexcept;

    // ---- ConnectedAnimation (CConnectedAnimation::StartSpriteAnimations) ----
    // Composition animates one host visual: Offset from the source to the destination position, Scale from 1 to
    // destination/source size, an InsetClip from the source to the destination insets and the cross-fade weight
    // (destination over source) 0 -> 1, all with the configuration's duration and easing. The Gravity configuration
    // combines an effect with the primary motion (Offset += (0, 80, 32) * dip, Scale *= 1 + 0.1 * dip when the primary
    // scale stays 1) whose dip peaks at 66% of the duration on linear time. Offsets are island DIPs; insets are in the
    // host visual's local (pre-scale) space.
    struct ConnectedParams
    {
        float fromX = 0, fromY = 0, toX = 0, toY = 0;  // host offset
        float toScaleX = 1, toScaleY = 1;              // host scale (from 1)
        float fromInsets[4] = {}, toInsets[4] = {};    // left, top, right, bottom
        bool gravity = false;
    };

    struct ConnectedFrame
    {
        float offsetX = 0, offsetY = 0;   // host offset
        float scaleX = 1, scaleY = 1;     // host scale (about the host origin)
        float insets[4] = {};             // left, top, right, bottom
        float crossFade = 0;              // destination weight
        float liftZ = 0;                  // Gravity Offset.Z (shadow depth)
    };

    // Gravity dip weight for linear progress: 0 -> 1 at 66% (ease-in bezier) -> 0 (ease-out bezier).
    float EvaluateGravityDip(float progress) noexcept;

    ConnectedFrame EvaluateConnected(const ConnectedParams& params, const Curve& curve, float progress) noexcept;

    // Opacity of the source snapshot drawn under the destination at weight w. A CrossFadeEffect is a per-pixel lerp;
    // drawing the destination over the source at opacity w is exact for opaque destination pixels with the source at
    // 1 and for transparent ones with the source at 1 - w. 1 - w^3 stays close to both and reaches 0 at the end.
    inline float ConnectedSourceOpacity(float crossFade) noexcept { return 1 - crossFade * crossFade * crossFade; }
}