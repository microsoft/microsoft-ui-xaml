// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediateAnimations.h>
#include <RenderingBackend.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cwchar>
#include <windows.h>

namespace ImmediateAnimations
{
    namespace
    {
        float Clamp01(float v) noexcept { return v < 0 ? 0 : (v > 1 ? 1 : v); }

        float BezierComponent(float p1, float p2, float t) noexcept
        {
            // B(t) = 3(1-t)^2 t p1 + 3(1-t) t^2 p2 + t^3
            const float u = 1 - t;
            return 3 * u * u * t * p1 + 3 * u * t * t * p2 + t * t * t;
        }

        float BezierDerivative(float p1, float p2, float t) noexcept
        {
            const float u = 1 - t;
            return 3 * u * u * p1 + 6 * u * t * (p2 - p1) + 3 * t * t * (1 - p2);
        }

        IsTargetImmediateFn s_isTargetImmediate = nullptr;
    }

    float EvaluateCubicBezier(float x1, float y1, float x2, float y2, float x) noexcept
    {
        x = Clamp01(x);
        if (x == 0 || x == 1) return x;

        // Newton iterations, then bisection when the slope is too flat (x1/x2 are clamped to [0, 1] by Composition so
        // Bx is monotonic).
        float t = x;
        for (int i = 0; i < 8; ++i)
        {
            const float err = BezierComponent(x1, x2, t) - x;
            if (std::fabs(err) < 1e-6f) return BezierComponent(y1, y2, t);
            const float d = BezierDerivative(x1, x2, t);
            if (std::fabs(d) < 1e-6f) break;
            t -= err / d;
            if (t < 0 || t > 1) break;
        }

        float lo = 0, hi = 1;
        t = x;
        for (int i = 0; i < 40; ++i)
        {
            const float v = BezierComponent(x1, x2, t);
            if (std::fabs(v - x) < 1e-7f) break;
            if (v < x) lo = t; else hi = t;
            t = (lo + hi) / 2;
        }
        return BezierComponent(y1, y2, t);
    }

    float Curve::Evaluate(float progress) const noexcept
    {
        progress = Clamp01(progress);
        return linear ? progress : EvaluateCubicBezier(x1, y1, x2, y2, progress);
    }

    int64_t ClampDurationMicroseconds(int64_t ticks) noexcept
    {
        // CAnimation::s_minWUCKeyFrameAnimationDurationInTicks / s_maxWUCKeyFrameAnimationDurationInTicks.
        constexpr int64_t minTicks = 10000;                          // 1 ms
        constexpr int64_t maxTicks = 24LL * 24 * 60 * 60 * 10000000; // 24 days
        return std::clamp(ticks, minTicks, maxTicks) / 10;
    }

    bool TryGetHoldProgress(_Out_ float* progress) noexcept
    {
        static const float s_hold = []() noexcept
        {
            wchar_t buffer[32] = {};
            const DWORD length = ::GetEnvironmentVariableW(L"WINUI_IMMEDIATE_ANIMATION_HOLD", buffer, ARRAYSIZE(buffer));
            if (length == 0 || length >= ARRAYSIZE(buffer)) return -1.0f;
            wchar_t* end = nullptr;
            const double value = std::wcstod(buffer, &end);
            if (end == buffer || !(value >= 0 && value <= 1)) return -1.0f;
            return static_cast<float>(value);
        }();
        *progress = s_hold < 0 ? 0 : s_hold;
        return s_hold >= 0;
    }

    float GetProgress(int64_t startUs, int64_t durationUs, int64_t nowUs, _Out_opt_ bool* finished) noexcept
    {
        float hold;
        if (TryGetHoldProgress(&hold))
        {
            if (finished) *finished = false;
            return hold;
        }
        const bool done = durationUs <= 0 || nowUs - startUs >= durationUs;
        if (finished) *finished = done;
        if (done) return 1.0f;
        return Clamp01(static_cast<float>(static_cast<double>(nowUs - startUs) / static_cast<double>(durationUs)));
    }

    int64_t NowMicroseconds() noexcept
    {
        static const int64_t s_frequency = []() noexcept
        {
            LARGE_INTEGER f;
            ::QueryPerformanceFrequency(&f);
            return f.QuadPart;
        }();
        LARGE_INTEGER counter;
        ::QueryPerformanceCounter(&counter);
        return static_cast<int64_t>(static_cast<double>(counter.QuadPart) * 1000000.0 / static_cast<double>(s_frequency));
    }

    void TransitionStore::Evaluate(const Entry& entry, int64_t nowUs, _Out_writes_(4) float* value) noexcept
    {
        const float eased = entry.curve.Evaluate(GetProgress(entry.startUs, entry.durationUs, nowUs));
        for (int i = 0; i < 4; ++i)
        {
            value[i] = entry.from[i] + (entry.to[i] - entry.from[i]) * eased;
        }
    }

    void TransitionStore::Start(const void* owner, Channel channel, _In_reads_(4) const float* from, _In_reads_(4) const float* to,
        uint32_t animateMask, int64_t durationUs, int64_t nowUs, const Curve& curve)
    {
        const uint32_t count = ComponentCount(channel);
        Entry entry;
        entry.owner = owner;
        entry.channel = channel;
        entry.startUs = nowUs;
        entry.durationUs = durationUs;
        entry.curve = curve;

        float start[4] = {};
        if (!TryGetValue(owner, channel, nowUs, start))
        {
            std::copy(from, from + count, start);
        }

        bool any = false;
        for (uint32_t i = 0; i < count; ++i)
        {
            entry.to[i] = to[i];
            const bool animate = (animateMask & (1u << i)) != 0;
            entry.from[i] = animate ? start[i] : to[i];
            any |= animate && entry.from[i] != entry.to[i];
        }

        Cancel(owner, channel);
        if (any && durationUs > 0)
        {
            m_entries.push_back(entry);
        }
    }

    void TransitionStore::Cancel(const void* owner, Channel channel) noexcept
    {
        m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
            [&](const Entry& e) { return e.owner == owner && e.channel == channel; }), m_entries.end());
    }

    void TransitionStore::RemoveOwner(const void* owner) noexcept
    {
        m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
            [&](const Entry& e) { return e.owner == owner; }), m_entries.end());
    }

    bool TransitionStore::TryGetValue(const void* owner, Channel channel, int64_t nowUs, _Out_writes_(4) float* value) const noexcept
    {
        for (const Entry& e : m_entries)
        {
            if (e.owner == owner && e.channel == channel)
            {
                bool finished = false;
                GetProgress(e.startUs, e.durationUs, nowUs, &finished);
                if (finished) return false;
                Evaluate(e, nowUs, value);
                return true;
            }
        }
        return false;
    }

    bool TransitionStore::HasOwner(const void* owner) const noexcept
    {
        return std::any_of(m_entries.begin(), m_entries.end(), [&](const Entry& e) { return e.owner == owner; });
    }

    bool TransitionStore::Prune(int64_t nowUs) noexcept
    {
        m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(), [&](const Entry& e)
        {
            bool finished = false;
            GetProgress(e.startUs, e.durationUs, nowUs, &finished);
            return finished;
        }), m_entries.end());
        return !m_entries.empty();
    }

    TransitionStore& GetThreadStore() noexcept
    {
        thread_local TransitionStore s_store;
        return s_store;
    }

    void SetIsTargetImmediateCallback(IsTargetImmediateFn callback) noexcept
    {
        s_isTargetImmediate = callback;
    }

    bool IsTargetImmediate(_In_ const void* element) noexcept
    {
        if (s_isTargetImmediate != nullptr)
        {
            return s_isTargetImmediate(element);
        }
        return RenderingBackend::IsImmediateModeActive();
    }

    float EvaluateGravityDip(float progress) noexcept
    {
        // GravityConnectedAnimationConfiguration: key frames 0 -> 1 at 0.66 (ease-in (0.35,0)-(0.55,1)) -> 0 at 1
        // (ease-out (0.45,0)-(0.55,1)), on the animation's linear time.
        constexpr float peak = 0.66f;
        progress = Clamp01(progress);
        if (progress <= peak)
        {
            return EvaluateCubicBezier(0.35f, 0.0f, 0.55f, 1.0f, progress / peak);
        }
        return 1 - EvaluateCubicBezier(0.45f, 0.0f, 0.55f, 1.0f, (progress - peak) / (1 - peak));
    }

    ConnectedFrame EvaluateConnected(const ConnectedParams& p, const Curve& curve, float progress) noexcept
    {
        const float eased = curve.Evaluate(Clamp01(progress));
        ConnectedFrame f;
        f.offsetX = p.fromX + (p.toX - p.fromX) * eased;
        f.offsetY = p.fromY + (p.toY - p.fromY) * eased;
        f.scaleX = 1 + (p.toScaleX - 1) * eased;
        f.scaleY = 1 + (p.toScaleY - 1) * eased;
        for (int i = 0; i < 4; ++i)
        {
            f.insets[i] = p.fromInsets[i] + (p.toInsets[i] - p.fromInsets[i]) * eased;
        }
        f.crossFade = Clamp01(eased);
        if (p.gravity)
        {
            const float dip = EvaluateGravityDip(progress);
            f.offsetY += 80.0f * dip;
            f.liftZ = 32.0f * dip;
            if (p.toScaleX == 1 && p.toScaleY == 1)
            {
                f.scaleX *= 1 + 0.1f * dip;
                f.scaleY *= 1 + 0.1f * dip;
            }
        }
        return f;
    }
}