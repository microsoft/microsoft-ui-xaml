// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ImmediatePainter.h>
#include <cstdint>
#include <vector>

// Pure (XAML-independent) logic for the immediate render thread (immediate-rendering prototype, S15/S20/S21):
// matrix helpers, a timing evaluator that mirrors CTimeline's clock math, sampled value curves, transform models,
// the island-space delta that moves a recorded layer, the spinner arc, and the device-lost simulation parser.
namespace ImmediateIndependent
{
    using RenderingBackend::Matrix3x2F;
    using RenderingBackend::PointF;
    using RenderingBackend::RectF;

    // ---- Row-vector affine matrices (a * b applies a, then b; same convention as D2D and CMILMatrix::Append) ----
    Matrix3x2F Multiply(const Matrix3x2F& a, const Matrix3x2F& b) noexcept;
    bool Invert(const Matrix3x2F& m, _Out_ Matrix3x2F* inverse) noexcept;
    PointF TransformPoint(const Matrix3x2F& m, PointF p) noexcept;
    RectF TransformBounds(const Matrix3x2F& m, const RectF& r) noexcept;
    bool IsIdentity(const Matrix3x2F& m) noexcept;
    Matrix3x2F TranslationMatrix(float x, float y) noexcept;
    Matrix3x2F ScaleMatrix(float sx, float sy, float cx, float cy) noexcept;
    // Same results as CTransform::BuildRotateMatrix / BuildSkewMatrix (exact quarter turns).
    Matrix3x2F RotationMatrix(float degrees, float cx, float cy) noexcept;
    Matrix3x2F SkewMatrix(float degreesX, float degreesY, float cx, float cy) noexcept;

    // ---- Timing: one CTimeline's clock, mirroring CTimeline::ComputeStateImpl ----
    enum class RepeatKind : uint8_t { Count, Duration, Forever };

    struct TimingSpec
    {
        double beginTime = 0;            // seconds, parent time
        float speedRatio = 1;
        bool naturalForever = false;     // natural duration (expiration)
        float naturalDuration = 1;       // seconds
        bool progressForever = false;    // duration for progress (GetDurationForProgress)
        float progressDuration = 1;
        RepeatKind repeat = RepeatKind::Count;
        float repeatCount = 1;
        double repeatDuration = 0;       // seconds, RepeatKind::Duration
        bool autoReverse = false;
        bool holdEnd = true;             // FillBehavior::HoldEnd (else Stop)
    };

    struct OptionalTime
    {
        bool hasValue = false;
        double value = 0;
    };

    // Expiration in parent time (CTimeline::ComputeEffectiveDuration + ComputeExpirationTime); no value = forever.
    OptionalTime ComputeExpiration(const TimingSpec& spec) noexcept;

    enum class ClockState : uint8_t { NotStarted, Active, Filling, Stopped };

    struct ClockSample
    {
        ClockState state = ClockState::NotStarted;
        float progress = 0;     // local progress [0, 1] (after autoreverse inversion)
        double time = 0;        // time reported to children
        uint32_t iteration = 0;
        bool hasTime = false;   // children have a parent time
    };

    ClockSample EvaluateClock(const TimingSpec& spec, bool parentHasTime, double parentTime) noexcept;

    // Evaluates a timing chain, root first, at the root's parent time; returns the leaf's sample.
    ClockSample EvaluateChain(_In_reads_(count) const TimingSpec* chain, size_t count, double rootParentTime) noexcept;

    // ---- Values ----
    // A double animation's value as a function of its leaf progress, sampled uniformly on [0, 1] (easing and key frames
    // are evaluated by the UI thread when the snapshot is taken).
    struct ValueCurve
    {
        std::vector<float> samples;
        float baseValue = 0; // value when the clock is NotStarted/Stopped
    };

    float SampleCurve(const ValueCurve& curve, float progress) noexcept;

    // One animated property: a timeline-driven curve, or a linear spin (degrees per second) for indeterminate spinners.
    struct AnimationTrack
    {
        enum class Kind : uint8_t { Timeline, Linear } kind = Kind::Timeline;
        std::vector<TimingSpec> chain;  // root first
        double rootParentTime0 = 0;     // root parent time at the snapshot
        ValueCurve curve;
        float value0 = 0;               // Linear: value at the snapshot
        float rate = 0;                 // Linear: units per second
    };

    // Value of the track elapsedSeconds after the snapshot; *running is false once the clock no longer advances.
    float EvaluateTrack(const AnimationTrack& track, double elapsedSeconds, _Out_opt_ bool* running = nullptr) noexcept;

    // ---- Transform models ----
    enum class TransformOpType : uint8_t { Matrix, Translate, Scale, Rotate, Skew, Composite };

    // Parameters per op (unused trailing parameters are ignored):
    //   Matrix: m11 m12 m21 m22 dx dy | Translate: x y | Scale: sx sy cx cy | Rotate: angle cx cy
    //   Skew: ax ay cx cy | Composite: cx cy sx sy skewX skewY rotation tx ty (CCompositeTransform order)
    struct TransformOp
    {
        static constexpr size_t c_maxParams = 9;
        TransformOpType type = TransformOpType::Matrix;
        float params[c_maxParams] = {};
        int8_t track[c_maxParams] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 }; // index into tracks, or -1 (constant)
    };

    struct TransformModel
    {
        std::vector<TransformOp> ops; // applied in order (TransformGroup children order)
    };

    // trackValues: current value of each track referenced by the ops (may be null when no op is animated).
    Matrix3x2F EvaluateTransform(const TransformModel& model, _In_opt_ const float* trackValues, size_t trackCount) noexcept;

    // Island-space delta for a layer recorded at world0 (element -> island, including RenderTransform render0 applied
    // about origin): a command transform c becomes c * delta when the RenderTransform becomes render1.
    Matrix3x2F ComputeLayerDelta(const Matrix3x2F& world0, PointF origin, const Matrix3x2F& render0, const Matrix3x2F& render1) noexcept;

    // Device-pixel bounds of deviceRect (physical px) moved by an island-space delta (DIPs), rounded outward with a 1px
    // antialiasing margin.
    RectF TransformDeviceBounds(const Matrix3x2F& delta, const RectF& deviceRect, float scale) noexcept;

    // ---- Spinner ----
    // Circular arc as cubic segments of at most 90 degrees (angles in degrees, clockwise from +x in y-down space).
    void BuildArc(float cx, float cy, float radius, float startDegrees, float sweepDegrees,
        _Inout_ std::vector<RenderingBackend::PathVerb>& verbs, _Inout_ std::vector<PointF>& points);

    // ---- Device lost simulation (WINUI_IMMEDIATE_SIMULATE_DEVICE_LOST) ----
    // "every:N" loses the device on every Nth presented frame; "3,10,25" on those frames; "limit:M" caps the number of
    // simulated losses. Tokens are comma separated, e.g. "every:30,limit:100". Frames are 1-based presented frames.
    struct DeviceLostSimulation
    {
        uint32_t every = 0;
        uint32_t limit = 0; // 0 = unlimited
        std::vector<uint32_t> frames;

        bool IsEnabled() const noexcept { return every != 0 || !frames.empty(); }
        bool ShouldLose(uint32_t presentedFrame, uint32_t lossesSoFar) const noexcept;
        static DeviceLostSimulation Parse(_In_opt_z_ const wchar_t* spec);
    };

    // ---- Resize / scale ----
    // True when a painter target must be rebuilt (not just resized) for the new rasterization scale: glyph, path and
    // shadow caches realized at the old scale would otherwise be reused.
    bool ScaleChangeInvalidatesCaches(float oldScale, float newScale) noexcept;
}
