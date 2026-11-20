// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediateIndependent.h>
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <limits>
#include <string>

namespace ImmediateIndependent
{
    namespace
    {
        constexpr double c_timeTolerance = 0.0005; // CTimeline::s_timeTolerance
        constexpr float c_pi = 3.14159265358979323846f;

        bool IsExpired(const OptionalTime& expiration, double parentTime) noexcept
        {
            return expiration.hasValue && expiration.value <= parentTime + c_timeTolerance;
        }

        // CTransform::BuildRotateMatrix uses exact values at quarter turns.
        void SinCos(float degrees, _Out_ float* s, _Out_ float* c) noexcept
        {
            float normalized = std::fmod(degrees, 360.0f);
            if (normalized < 0) normalized += 360.0f;
            if (normalized == 0) { *s = 0; *c = 1; return; }
            if (normalized == 90) { *s = 1; *c = 0; return; }
            if (normalized == 180) { *s = 0; *c = -1; return; }
            if (normalized == 270) { *s = -1; *c = 0; return; }
            const float radians = degrees * c_pi / 180.0f;
            *s = std::sin(radians);
            *c = std::cos(radians);
        }
    }

    Matrix3x2F Multiply(const Matrix3x2F& a, const Matrix3x2F& b) noexcept
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
        if (std::fabs(det) < 1e-12)
        {
            *inverse = Matrix3x2F{};
            return false;
        }
        const double inv = 1.0 / det;
        Matrix3x2F r;
        r.m11 = static_cast<float>(m.m22 * inv);
        r.m12 = static_cast<float>(-m.m12 * inv);
        r.m21 = static_cast<float>(-m.m21 * inv);
        r.m22 = static_cast<float>(m.m11 * inv);
        r.dx = static_cast<float>((static_cast<double>(m.m21) * m.dy - static_cast<double>(m.m22) * m.dx) * inv);
        r.dy = static_cast<float>((static_cast<double>(m.m12) * m.dx - static_cast<double>(m.m11) * m.dy) * inv);
        *inverse = r;
        return true;
    }

    PointF TransformPoint(const Matrix3x2F& m, PointF p) noexcept
    {
        return { p.x * m.m11 + p.y * m.m21 + m.dx, p.x * m.m12 + p.y * m.m22 + m.dy };
    }

    RectF TransformBounds(const Matrix3x2F& m, const RectF& r) noexcept
    {
        const PointF corners[] = {
            TransformPoint(m, { r.left, r.top }),
            TransformPoint(m, { r.right, r.top }),
            TransformPoint(m, { r.left, r.bottom }),
            TransformPoint(m, { r.right, r.bottom }),
        };
        RectF out{ corners[0].x, corners[0].y, corners[0].x, corners[0].y };
        for (const PointF& p : corners)
        {
            out.left = (std::min)(out.left, p.x);
            out.top = (std::min)(out.top, p.y);
            out.right = (std::max)(out.right, p.x);
            out.bottom = (std::max)(out.bottom, p.y);
        }
        return out;
    }

    bool IsIdentity(const Matrix3x2F& m) noexcept
    {
        return m.m11 == 1 && m.m12 == 0 && m.m21 == 0 && m.m22 == 1 && m.dx == 0 && m.dy == 0;
    }

    Matrix3x2F TranslationMatrix(float x, float y) noexcept
    {
        Matrix3x2F m;
        m.dx = x;
        m.dy = y;
        return m;
    }

    Matrix3x2F ScaleMatrix(float sx, float sy, float cx, float cy) noexcept
    {
        Matrix3x2F m;
        m.m11 = sx;
        m.m22 = sy;
        m.dx = cx * (1 - sx);
        m.dy = cy * (1 - sy);
        return m;
    }

    Matrix3x2F RotationMatrix(float degrees, float cx, float cy) noexcept
    {
        float s, c;
        SinCos(degrees, &s, &c);
        Matrix3x2F m;
        m.m11 = c;
        m.m12 = s;
        m.m21 = -s;
        m.m22 = c;
        m.dx = cx * (1 - c) + cy * s;
        m.dy = cy * (1 - c) - cx * s;
        return m;
    }

    Matrix3x2F SkewMatrix(float degreesX, float degreesY, float cx, float cy) noexcept
    {
        const float tanX = std::tan(degreesX * c_pi / 180.0f);
        const float tanY = std::tan(degreesY * c_pi / 180.0f);
        Matrix3x2F m;
        m.m12 = tanY;
        m.m21 = tanX;
        m.dx = -cy * tanX;
        m.dy = -cx * tanY;
        return m;
    }

    OptionalTime ComputeExpiration(const TimingSpec& spec) noexcept
    {
        float repeatCount = 1;
        double limitingTime = (std::numeric_limits<double>::max)();
        if (spec.repeat == RepeatKind::Duration)
        {
            limitingTime = spec.repeatDuration;
        }
        else
        {
            // Forever repeat behaviors report a count too; it only matters for the zero check.
            repeatCount = spec.repeat == RepeatKind::Forever ? 1.0f : spec.repeatCount;
        }

        OptionalTime effective;
        if ((!spec.naturalForever && spec.naturalDuration <= 0) || repeatCount <= 0)
        {
            effective = { true, 0.0 };
        }
        else if (spec.repeat == RepeatKind::Forever || (spec.naturalForever && spec.repeat == RepeatKind::Count))
        {
            effective = { false, 0.0 };
        }
        else if (spec.naturalForever)
        {
            effective = { true, limitingTime };
        }
        else
        {
            double scale = spec.repeat == RepeatKind::Duration ? limitingTime / spec.speedRatio : repeatCount / spec.speedRatio;
            if (spec.autoReverse)
            {
                scale *= 2.0;
            }
            effective = { true, (std::min)(scale * spec.naturalDuration, limitingTime) };
        }

        if (effective.hasValue)
        {
            effective.value += spec.beginTime;
        }
        return effective;
    }

    ClockSample EvaluateClock(const TimingSpec& spec, bool parentHasTime, double parentTime) noexcept
    {
        ClockSample sample;
        if (!parentHasTime || parentTime < spec.beginTime)
        {
            return sample;
        }

        const OptionalTime expiration = ComputeExpiration(spec);
        sample.state = ClockState::Active;
        if (IsExpired(expiration, parentTime))
        {
            if (!spec.holdEnd)
            {
                sample.state = ClockState::Stopped;
                return sample;
            }
            sample.state = ClockState::Filling;
            parentTime = expiration.value;
        }

        double currentTime = (parentTime - spec.beginTime) * spec.speedRatio;
        if (spec.progressForever)
        {
            sample.progress = 0;
        }
        else if (spec.progressDuration <= 0)
        {
            currentTime = 0;
            sample.progress = 1;
        }
        else
        {
            const float iterations = static_cast<float>(currentTime / spec.progressDuration);
            const float iteration = std::floor(iterations);
            sample.iteration = static_cast<uint32_t>((std::max)(0.0f, iteration));
            float progress = iterations - iteration;
            if ((iterations >= 1.0f && progress <= 0.001f) || progress >= 0.999f)
            {
                if (IsExpired(expiration, parentTime))
                {
                    progress = (spec.autoReverse && (sample.iteration % 2) == 0) ? 0.0f : 1.0f;
                }
            }
            if (spec.autoReverse && (sample.iteration % 2) == 1)
            {
                progress = 1.0f - progress;
            }
            sample.progress = progress;
            currentTime = static_cast<double>(progress) * spec.progressDuration;
        }

        sample.time = currentTime;
        sample.hasTime = true;
        return sample;
    }

    ClockSample EvaluateChain(_In_reads_(count) const TimingSpec* chain, size_t count, double rootParentTime) noexcept
    {
        ClockSample sample;
        bool hasTime = true;
        double time = rootParentTime;
        for (size_t i = 0; i < count; ++i)
        {
            sample = EvaluateClock(chain[i], hasTime, time);
            if (sample.state == ClockState::NotStarted || sample.state == ClockState::Stopped)
            {
                // Stopped/not-started ancestors give their children no time: the leaf is not applied either.
                ClockSample none;
                none.state = sample.state;
                return none;
            }
            hasTime = sample.hasTime;
            time = sample.time;
        }
        return sample;
    }

    float SampleCurve(const ValueCurve& curve, float progress) noexcept
    {
        if (curve.samples.empty())
        {
            return curve.baseValue;
        }
        if (curve.samples.size() == 1 || !(progress > 0))
        {
            return curve.samples.front();
        }
        if (progress >= 1)
        {
            return curve.samples.back();
        }
        const float position = progress * static_cast<float>(curve.samples.size() - 1);
        const size_t index = static_cast<size_t>(position);
        const float fraction = position - static_cast<float>(index);
        const float a = curve.samples[index];
        const float b = curve.samples[(std::min)(index + 1, curve.samples.size() - 1)];
        return a + (b - a) * fraction;
    }

    float EvaluateTrack(const AnimationTrack& track, double elapsedSeconds, _Out_opt_ bool* running) noexcept
    {
        if (track.kind == AnimationTrack::Kind::Linear)
        {
            if (running) *running = track.rate != 0;
            return track.value0 + static_cast<float>(track.rate * elapsedSeconds);
        }

        const ClockSample sample = EvaluateChain(track.chain.data(), track.chain.size(), track.rootParentTime0 + elapsedSeconds);
        if (running) *running = sample.state == ClockState::Active;
        if (sample.state == ClockState::NotStarted || sample.state == ClockState::Stopped)
        {
            return track.curve.baseValue;
        }
        return SampleCurve(track.curve, sample.progress);
    }

    Matrix3x2F EvaluateTransform(const TransformModel& model, _In_opt_ const float* trackValues, size_t trackCount) noexcept
    {
        Matrix3x2F result;
        for (const TransformOp& op : model.ops)
        {
            float p[TransformOp::c_maxParams];
            for (size_t i = 0; i < TransformOp::c_maxParams; ++i)
            {
                const int8_t track = op.track[i];
                p[i] = (track >= 0 && trackValues && static_cast<size_t>(track) < trackCount) ? trackValues[track] : op.params[i];
            }

            Matrix3x2F m;
            switch (op.type)
            {
            case TransformOpType::Matrix:
                m = { p[0], p[1], p[2], p[3], p[4], p[5] };
                break;
            case TransformOpType::Translate:
                m = TranslationMatrix(p[0], p[1]);
                break;
            case TransformOpType::Scale:
                m = ScaleMatrix(p[0], p[1], p[2], p[3]);
                break;
            case TransformOpType::Rotate:
                m = RotationMatrix(p[0], p[1], p[2]);
                break;
            case TransformOpType::Skew:
                m = SkewMatrix(p[0], p[1], p[2], p[3]);
                break;
            case TransformOpType::Composite:
            {
                // CCompositeTransform::GetTransform: -center, scale, skew, rotate, +center+translate.
                m = TranslationMatrix(-p[0], -p[1]);
                m = Multiply(m, ScaleMatrix(p[2], p[3], 0, 0));
                m = Multiply(m, SkewMatrix(p[4], p[5], 0, 0));
                m = Multiply(m, RotationMatrix(p[6], 0, 0));
                m = Multiply(m, TranslationMatrix(p[0] + p[7], p[1] + p[8]));
                break;
            }
            }
            result = Multiply(result, m);
        }
        return result;
    }

    Matrix3x2F ComputeLayerDelta(const Matrix3x2F& world0, PointF origin, const Matrix3x2F& render0, const Matrix3x2F& render1) noexcept
    {
        // world0 = T(-o) * render0 * T(o) * rest; moving to render1 changes it to T(-o) * render1 * T(o) * rest, so
        // delta = world0^-1 * T(-o) * render1 * render0^-1 * T(o) * world0.
        Matrix3x2F render0Inverse, world0Inverse;
        if (!Invert(render0, &render0Inverse) || !Invert(world0, &world0Inverse))
        {
            // A collapsed layer (zero scale) cannot be re-derived; draw nothing new (the caller hides it).
            return Matrix3x2F{ 0, 0, 0, 0, 0, 0 };
        }
        Matrix3x2F q = TranslationMatrix(-origin.x, -origin.y);
        q = Multiply(q, render1);
        q = Multiply(q, render0Inverse);
        q = Multiply(q, TranslationMatrix(origin.x, origin.y));
        return Multiply(Multiply(world0Inverse, q), world0);
    }

    RectF TransformDeviceBounds(const Matrix3x2F& delta, const RectF& deviceRect, float scale) noexcept
    {
        if (!(scale > 0) || deviceRect.right <= deviceRect.left || deviceRect.bottom <= deviceRect.top)
        {
            return deviceRect;
        }
        const RectF dips{ deviceRect.left / scale, deviceRect.top / scale, deviceRect.right / scale, deviceRect.bottom / scale };
        const RectF moved = TransformBounds(delta, dips);
        return { std::floor(moved.left * scale) - 1, std::floor(moved.top * scale) - 1,
                 std::ceil(moved.right * scale) + 1, std::ceil(moved.bottom * scale) + 1 };
    }

    void BuildArc(float cx, float cy, float radius, float startDegrees, float sweepDegrees,
        _Inout_ std::vector<RenderingBackend::PathVerb>& verbs, _Inout_ std::vector<PointF>& points)
    {
        using RenderingBackend::PathVerb;
        if (!(radius > 0) || sweepDegrees == 0)
        {
            return;
        }
        const int segments = (std::max)(1, static_cast<int>(std::ceil(std::fabs(sweepDegrees) / 90.0f - 1e-4f)));
        const float step = sweepDegrees / static_cast<float>(segments) * c_pi / 180.0f;
        const float k = 4.0f / 3.0f * std::tan(step / 4.0f);
        float angle = startDegrees * c_pi / 180.0f;
        PointF current{ cx + radius * std::cos(angle), cy + radius * std::sin(angle) };
        verbs.push_back(PathVerb::MoveTo);
        points.push_back(current);
        for (int i = 0; i < segments; ++i)
        {
            const float next = angle + step;
            const PointF end{ cx + radius * std::cos(next), cy + radius * std::sin(next) };
            verbs.push_back(PathVerb::CubicTo);
            points.push_back({ current.x - k * radius * std::sin(angle), current.y + k * radius * std::cos(angle) });
            points.push_back({ end.x + k * radius * std::sin(next), end.y - k * radius * std::cos(next) });
            points.push_back(end);
            current = end;
            angle = next;
        }
    }

    bool DeviceLostSimulation::ShouldLose(uint32_t presentedFrame, uint32_t lossesSoFar) const noexcept
    {
        if (presentedFrame == 0 || (limit != 0 && lossesSoFar >= limit))
        {
            return false;
        }
        if (every != 0 && presentedFrame % every == 0)
        {
            return true;
        }
        return std::find(frames.begin(), frames.end(), presentedFrame) != frames.end();
    }

    DeviceLostSimulation DeviceLostSimulation::Parse(_In_opt_z_ const wchar_t* spec)
    {
        DeviceLostSimulation result;
        if (!spec)
        {
            return result;
        }

        const wchar_t* cursor = spec;
        while (*cursor)
        {
            const wchar_t* end = cursor;
            while (*end && *end != L',' && *end != L';')
            {
                ++end;
            }

            std::wstring token(cursor, end);
            token.erase(std::remove_if(token.begin(), token.end(), [](wchar_t c) { return c == L' ' || c == L'\t'; }), token.end());
            for (wchar_t& c : token)
            {
                c = static_cast<wchar_t>(towlower(c));
            }

            auto parseNumber = [](const std::wstring& text, _Out_ uint32_t* value) {
                if (text.empty() || text.size() > 9 || !std::all_of(text.begin(), text.end(), [](wchar_t c) { return c >= L'0' && c <= L'9'; }))
                {
                    *value = 0;
                    return false;
                }
                *value = static_cast<uint32_t>(std::wcstoul(text.c_str(), nullptr, 10));
                return true;
            };

            uint32_t value = 0;
            if (token.rfind(L"every:", 0) == 0)
            {
                if (parseNumber(token.substr(6), &value)) result.every = value;
            }
            else if (token.rfind(L"limit:", 0) == 0)
            {
                if (parseNumber(token.substr(6), &value)) result.limit = value;
            }
            else if (parseNumber(token, &value) && value != 0)
            {
                result.frames.push_back(value);
            }

            cursor = *end ? end + 1 : end;
        }
        return result;
    }

    bool ScaleChangeInvalidatesCaches(float oldScale, float newScale) noexcept
    {
        return std::fabs(oldScale - newScale) > 1e-4f;
    }
}
