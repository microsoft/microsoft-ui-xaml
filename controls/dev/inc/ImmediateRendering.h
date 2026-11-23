// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// ---- transitions ----
// Immediate rendering prototype (WINUI_RENDERING_BACKEND=gdiplus|d2d-hw|d2d-sw, docs/design-notes/
// immediate-rendering-backends-proposal.md, "Full XAML coverage: transitions"). Those backends paint the XAML tree
// themselves: Composition animations started on hand-off visuals can't be read back, so touching a hand-off visual makes
// the window fall back to Composition. Controls that animate themselves check this and run the same motion as XAML
// Storyboards (key frames with KeySplines matching their Composition cubic beziers) on RenderTransform/Opacity instead.
// Storyboards also render correctly when the window does fall back, so the check only needs the process-wide setting.
namespace ImmediateRendering
{
    inline bool IsRequested()
    {
        static const bool requested = []()
        {
            wchar_t value[32]{};
            const DWORD length = GetEnvironmentVariableW(L"WINUI_RENDERING_BACKEND", value, ARRAYSIZE(value));
            return length > 0 && length < ARRAYSIZE(value) && _wcsicmp(value, L"composition") != 0;
        }();
        return requested;
    }

    // Key frame animations equivalent to Composition ScalarKeyFrameAnimations (normalized key times, per-segment easing).
    // A non-zero delay behaves like Composition's DelayTime: key times are offset by it (use Preset for
    // AnimationDelayBehavior::SetInitialValueBeforeDelay).
    class KeyFrames
    {
    public:
        KeyFrames(const winrt::Storyboard& storyboard, const winrt::DependencyObject& target, const wchar_t* property,
            winrt::TimeSpan duration, winrt::TimeSpan delay = {})
            : m_duration(duration), m_delay(delay)
        {
            m_animation.Duration(winrt::DurationHelper::FromTimeSpan(delay + duration));
            m_animation.EnableDependentAnimation(true);
            winrt::Storyboard::SetTarget(m_animation, target);
            winrt::Storyboard::SetTargetProperty(m_animation, property);
            storyboard.Children().Append(m_animation);
        }

        // Linear segment ending at `progress`.
        KeyFrames& Linear(float progress, double value)
        {
            winrt::LinearDoubleKeyFrame frame;
            Add(frame, progress, value);
            return *this;
        }

        // Cubic bezier segment (CompositionCubicBezierEasingFunction) ending at `progress`.
        KeyFrames& Bezier(float progress, double value, winrt::float2 p1, winrt::float2 p2)
        {
            winrt::SplineDoubleKeyFrame frame;
            winrt::KeySpline spline;
            spline.ControlPoint1({ p1.x, p1.y });
            spline.ControlPoint2({ p2.x, p2.y });
            frame.KeySpline(spline);
            Add(frame, progress, value);
            return *this;
        }

        // Single-step segment with IsFinalStepSingleFrame: the value jumps at the key time.
        KeyFrames& Step(float progress, double value)
        {
            winrt::DiscreteDoubleKeyFrame frame;
            Add(frame, progress, value);
            return *this;
        }

        // Holds alue from the start of the delay (SetInitialValueBeforeDelay).
        KeyFrames& Preset(double value)
        {
            winrt::DiscreteDoubleKeyFrame frame;
            frame.KeyTime(winrt::KeyTimeHelper::FromTimeSpan(winrt::TimeSpan{}));
            frame.Value(value);
            m_animation.KeyFrames().Append(frame);
            return *this;
        }

    private:
        void Add(const winrt::DoubleKeyFrame& frame, float progress, double value)
        {
            const auto at = m_delay + std::chrono::duration_cast<winrt::TimeSpan>(m_duration * static_cast<double>(progress));
            frame.KeyTime(winrt::KeyTimeHelper::FromTimeSpan(at));
            frame.Value(value);
            m_animation.KeyFrames().Append(frame);
        }

        winrt::DoubleAnimationUsingKeyFrames m_animation;
        winrt::TimeSpan m_duration;
        winrt::TimeSpan m_delay;
    };

    // The CompositeTransform the immediate paths animate (created when the element has no RenderTransform). Returns null
    // when the element already has another kind of RenderTransform, which is left alone (the caller skips the animation).
    inline winrt::CompositeTransform EnsureCompositeTransform(const winrt::UIElement& element)
    {
        const auto existing = element.RenderTransform();
        if (existing)
        {
            return existing.try_as<winrt::CompositeTransform>();
        }
        winrt::CompositeTransform transform;
        element.RenderTransform(transform);
        return transform;
    }
}
