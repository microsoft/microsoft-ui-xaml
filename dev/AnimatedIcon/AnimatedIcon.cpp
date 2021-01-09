// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AnimatedIcon.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include <AnimatedVisuals\ProgressRingDeterminate.h>
#include <AnimatedIconTestHooks.h>
using namespace AnimatedVisuals;

AnimatedIcon::AnimatedIcon()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_AnimatedIcon);
    m_progressPropertySet = winrt::Window::Current().Compositor().CreatePropertySet();
    m_progressPropertySet.InsertScalar(L"Progress", 0);

    RegisterPropertyChangedCallback(winrt::IconElement::ForegroundProperty(), { this, &AnimatedIcon::OnForegroundPropertyChanged});
}

void AnimatedIcon::OnApplyTemplate()
{
    auto const grid = winrt::VisualTreeHelper::GetChild(*this, 0).as<winrt::Grid>();
    m_rootGrid.set(grid);
    m_currentState = GetState(*this);

    if (grid)
    {
        // Animated icon implements IconElement through PathIcon. We don't need the Path that
        // PathIcon creates, so get rid of it.
        grid.Children().Clear();
        OnFallbackIconSourcePropertyChanged(nullptr);
        if (auto const visual = m_animatedVisual.get())
        {
            winrt::ElementCompositionPreview::SetElementChildVisual(grid, visual.RootVisual());
        }
    }
}

winrt::Size AnimatedIcon::MeasureOverride(winrt::Size const& availableSize)
{
    if (auto const visual = m_animatedVisual.get())
    {
        // Animated Icon scales using the Uniform startegy, meaning that it scales the horizonal and vertical
        // dementions equally by the maximum ammount that doesn't exceed the available size in either dimention.
        // If the available size is infinite in both dimentions then we don't scale the visual. Otherwise, we
        // calculate the scale factor by comparing the default visual size to the available size. This produces 2
        // scale factors, one for each dimention. We choose the smaller of the scale factors to not exceed the
        // available size in that dimention.
        auto const visualSize = visual.Size();
        if (visualSize != winrt::float2::zero())
        {
            const auto widthScale = availableSize.Width == std::numeric_limits<double>::infinity() ? std::numeric_limits<float>::infinity() : availableSize.Width / visualSize.x;
            const auto heightScale = availableSize.Height == std::numeric_limits<double>::infinity() ? std::numeric_limits<float>::infinity() : availableSize.Height / visualSize.y;
            if (widthScale == std::numeric_limits<double>::infinity() && heightScale == std::numeric_limits<double>::infinity())
            {
                return winrt::Size{ visualSize.x, visualSize.y };
            }
            else if (widthScale == std::numeric_limits<double>::infinity())
            {
                return winrt::Size{ visualSize.x * heightScale, availableSize.Height };
            }
            else if (heightScale == std::numeric_limits<double>::infinity())
            {
                return winrt::Size{ availableSize.Width, visualSize.y * widthScale };
            }
            else
            {
                return (heightScale > widthScale)
                    ? winrt::Size{ availableSize.Width, visualSize.y * widthScale }
                : winrt::Size{ visualSize.x * heightScale, availableSize.Height };
            }
        }
        return visualSize;
    }
    // If we don't have a visual, we will show the fallback icon, so we need to do a traditional measure.
    else
    {
        return __super::MeasureOverride(availableSize);
    }
}

winrt::Size AnimatedIcon::ArrangeOverride(winrt::Size const& finalSize)
{
    if (auto const visual = m_animatedVisual.get())
    {
        auto const visualSize = visual.Size();
        auto const scale = [finalSize, visual]()
        {
            auto scale = static_cast<winrt::float2>(finalSize) / visual.Size();
            if (scale.x < scale.y)
            {
                scale.y = scale.x;
            }
            else
            {
                scale.x = scale.y;
            }
            return scale;
        }();

        winrt::float2 const arrangedSize = {
            std::min(finalSize.Width / scale.x, visualSize.x),
            std::min(finalSize.Height / scale.y, visualSize.y)
        };
        const auto offset = (finalSize - (visualSize * scale)) / 2;
        const auto z = 0.0F;
        const auto rootVisual = visual.RootVisual();
        rootVisual.Offset({ offset, z });
        rootVisual.Size(arrangedSize);
        rootVisual.Scale({ scale, z });
        return finalSize;
    }
    else
    {
        return __super::ArrangeOverride(finalSize);
    }
}

void AnimatedIcon::OnAnimatedIconStatePropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto const senderAsAnimatedIcon = sender.try_as<AnimatedIcon>())
    {
        senderAsAnimatedIcon->OnStatePropertyChanged();
    }
    else if (winrt::VisualTreeHelper::GetChildrenCount(sender) > 0)
    {
        if (auto const childAsAnimatedIcon = winrt::VisualTreeHelper::GetChild(sender, 0).try_as<winrt::AnimatedIcon>())
        {
            childAsAnimatedIcon.SetValue(AnimatedIconProperties::s_StateProperty, args.NewValue());
        }
    }
}

// When we receive a state change it might be erroneous. This is because these state changes often come from Animated Icon's parent control's
// Visual state manager.  Many of our controls assume that it is okay to call GoToState("Foo") followed immediately by GoToState("Bar") in
// order to end up in the "Bar" state. However Animated Icon also cares what state you are coming from, so this pattern would not trigger
// the NormalToBar transition but instead NormalToFoo followed by FooToBar. Since we can't change these controls logic in WinUI2 we handle
// this by waiting until the next layout cycle to play an animated icon transtion. However, the state dependency property changed is not
// enough to ensure that a layout updated will trigger, so we must also invalidate a layout property, Arrange was chosen arbitrarily.
void AnimatedIcon::OnStatePropertyChanged()
{
    m_pendingState = ValueHelper<winrt::hstring>::CastOrUnbox(this->GetValue(AnimatedIconStateProperty()));
    m_layoutUpdatedRevoker = this->LayoutUpdated(winrt::auto_revoke, { this, &AnimatedIcon::OnLayoutUpdatedAfterStateChanged });
    InvalidateArrange();
}

void AnimatedIcon::OnLayoutUpdatedAfterStateChanged(winrt::IInspectable const& sender, winrt::IInspectable const& args)
{
    m_layoutUpdatedRevoker.revoke();
    switch (m_queueBehavior)
    {
    case winrt::AnimatedIconAnimationQueueBehavior::Cut:
        TransitionAndUpdateStates(m_currentState, m_pendingState);
        break;
    case winrt::AnimatedIconAnimationQueueBehavior::QueueOne:
        if (m_isPlaying)
        {
            // If we already have a queued state, cancel the current animation with the previously queued transtion
            // then Queue this new transition.
            if (!m_queuedState.empty())
            {
                TransitionAndUpdateStates(m_currentState, m_queuedState);
            }
            m_queuedState = m_pendingState;
        }
        else
        {
            TransitionAndUpdateStates(m_currentState, m_pendingState);
        }
        break;
    case winrt::AnimatedIconAnimationQueueBehavior::SpeedUpQueueOne:
        if (m_isPlaying)
        {
            // Cancel the previous animation completed handler, before we cancel that animation by starting a new one.
            if (m_batch)
            {
                m_batchCompletedRevoker.revoke();
            }

            // If we already have a queued state, cancel the current animation with the previously queued transtion
            //  played speed up then Queue this new transition.
            if (!m_queuedState.empty())
            {
                TransitionAndUpdateStates(m_currentState, m_queuedState, m_speedUpMultiplier);
                m_queuedState = m_pendingState;
            }
            else
            {
                m_queuedState = m_pendingState;

                auto const markers = Source().Markers();
                winrt::hstring transitionEndName = m_previousState + L"To" + m_currentState + L"_End";
                auto const hasEndMarker = markers.HasKey(transitionEndName);
                if (hasEndMarker)
                {
                    PlaySegment(NAN, static_cast<float>(markers.Lookup(transitionEndName)), m_speedUpMultiplier);
                }
            }
        }
        else
        {
            TransitionAndUpdateStates(m_currentState, m_pendingState);
        }
        break;
    }
    m_pendingState = L"";
}

void AnimatedIcon::TransitionAndUpdateStates(const winrt::hstring& fromState, const winrt::hstring& toState, float playbackMultiplier)
{
    TransitionStates(fromState, toState, playbackMultiplier);
    m_previousState = fromState;
    m_currentState = toState;
    m_queuedState = L"";
}

void AnimatedIcon::TransitionStates(const winrt::hstring& fromState, const winrt::hstring& toState, float playbackMultiplier)
{
    if (auto const source = Source())
    {
        if (auto const markers = source.Markers())
        {
            winrt::hstring transitionName = fromState + L"To" + toState;
            winrt::hstring transitionStartName = transitionName + L"_Start";
            winrt::hstring transitionEndName = transitionName + L"_End";

            auto const hasStartMarker = markers.HasKey(transitionStartName);
            auto const hasEndMarker = markers.HasKey(transitionEndName);
            if (hasStartMarker && hasEndMarker)
            {
                auto const fromProgress = static_cast<float>(markers.Lookup(transitionStartName));
                auto const toProgress = static_cast<float>(markers.Lookup(transitionEndName));
                PlaySegment(fromProgress, toProgress, playbackMultiplier);
                m_lastAnimationSegmentStart = transitionStartName;
                m_lastAnimationSegmentEnd = transitionEndName;
            }
            else if (hasEndMarker)
            {
                auto const toProgress = static_cast<float>(markers.Lookup(transitionEndName));
                m_progressPropertySet.InsertScalar(L"Progress", toProgress);
                m_lastAnimationSegmentStart = L"";
                m_lastAnimationSegmentEnd = transitionEndName;
            }
            else if (hasStartMarker)
            {
                auto const toProgress = static_cast<float>(markers.Lookup(transitionStartName));
                m_progressPropertySet.InsertScalar(L"Progress", toProgress);
                m_lastAnimationSegmentStart = transitionStartName;
                m_lastAnimationSegmentEnd = L"";
            }
            else if (markers.HasKey(transitionName))
            {
                auto const toProgress = static_cast<float>(markers.Lookup(transitionName));
                m_progressPropertySet.InsertScalar(L"Progress", toProgress);
                m_lastAnimationSegmentStart = L"";
                m_lastAnimationSegmentEnd = transitionName;
            }
            else if (markers.HasKey(toState))
            {
                auto const toProgress = static_cast<float>(markers.Lookup(toState));
                m_progressPropertySet.InsertScalar(L"Progress", toProgress);
                m_lastAnimationSegmentStart = L"";
                m_lastAnimationSegmentEnd = toState;
            }
            else
            {
                // Since we can't find an animation for this transition, try to find one that ends in the same place
                // and cut to that postition instead.
                auto const [found, value] = [toState, markers, this]()
                {
                    winrt::hstring fragment = L"To" + toState + L"_End";
                    for (auto const [key, val] : markers)
                    {
                        std::wstring value = key.data();
                        if (value.find(fragment) != std::wstring::npos)
                        {
                            m_lastAnimationSegmentStart = L"";
                            m_lastAnimationSegmentEnd = key;
                            return std::make_tuple(true, static_cast<float>(val));
                        }
                    }
                    return std::make_tuple(false, 0.0f);
                }();
                if (found)
                {
                    m_progressPropertySet.InsertScalar(L"Progress", value);
                }
                else
                {
                    // We also support setting the state proprety to a float value, which instructs the animated icon
                    // to animated the Progress property to the provided value.
                    auto const stateAsFloat = std::wcstof(toState.data(), nullptr);
                    if(!isnan(stateAsFloat))
                    {
                        PlaySegment(NAN, stateAsFloat, playbackMultiplier);
                        m_lastAnimationSegmentStart = L"";
                        m_lastAnimationSegmentEnd = toState;
                    }
                    else
                    {
                        // None of our attempt to find an animation to play or frame to show have worked, so just cut
                        // to frame 0.
                        m_progressPropertySet.InsertScalar(L"Progress", 0.0);
                        m_lastAnimationSegmentStart = L"";
                        m_lastAnimationSegmentEnd = L"0.0";
                    }
                }
            }
            m_lastAnimationSegment = transitionName;
            AnimatedIconTestHooks::NotifyLastAnimationSegmentChanged(*this);
        }
    }
}

void AnimatedIcon::PlaySegment(float from, float to, float playbackMultiplier)
{
    auto const segmentLength = [from, to, previousSegmentLength = m_previousSegmentLength]()
    {
        if (std::isnan(from))
        {
            return previousSegmentLength;
        }
        return std::abs(to - from);
    }();

    m_previousSegmentLength = segmentLength;
    auto const duration = m_animatedVisual ?
        std::chrono::duration_cast<winrt::TimeSpan>(m_animatedVisual.get().Duration() * segmentLength * (1.0 / playbackMultiplier) * m_durationMultiplier) :
        winrt::TimeSpan::zero();
    // If the duration is really short (< 20ms) don't bother trying to animate, or if animations are disabled.
    if (duration < winrt::TimeSpan{ 20ms } || !SharedHelpers::IsAnimationsEnabled())
    {
        m_progressPropertySet.InsertScalar(L"Progress", to);
        OnAnimationCompleted(nullptr, nullptr);
    }
    else
    {
        auto compositor = m_progressPropertySet.Compositor();
        auto animation = compositor.CreateScalarKeyFrameAnimation();
        animation.Duration(duration);
        auto linearEasing = compositor.CreateLinearEasingFunction();

        // Play from fromProgress.
        if (!std::isnan(from))
        {
            //m_progressPropertySet.InsertScalar(L"Progress", from);
            animation.InsertKeyFrame(0, from);
        }

        // Play to toProgress
        animation.InsertKeyFrame(1, to, linearEasing);

        animation.IterationBehavior(winrt::AnimationIterationBehavior::Count);
        animation.IterationCount(1);

        if (m_batch)
        {
            m_batchCompletedRevoker.revoke();
        }
        m_batch = compositor.CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        m_batchCompletedRevoker = RegisterScopedBatchCompleted(m_batch, { this, &AnimatedIcon::OnAnimationCompleted });

        m_isPlaying = true;
        m_progressPropertySet.StartAnimation(L"Progress", animation);
        m_batch.End();
    }
}

void AnimatedIcon::OnSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    auto const visual = [this]()
    {
        if (auto const source = Source())
        {
            if (auto const ForegroundSolidColorBrush = Foreground().try_as<winrt::SolidColorBrush>())
            {
                source.SetColorProperty(L"Foreground", ForegroundSolidColorBrush.Color());
            }

            if (auto const visual = source.TryCreateAnimatedIconVisual(winrt::Window::Current().Compositor()))
            {
                m_animatedVisual.set(visual);
                return visual.RootVisual();
            }
            else
            {
                m_animatedVisual.set(nullptr);
                return static_cast<winrt::Visual>(nullptr);
            }
        }
        else
        {
            m_animatedVisual.set(nullptr);
            return static_cast<winrt::Visual>(nullptr);
        }
    }();

    if (auto const rootGrid = m_rootGrid.get())
    {
        winrt::ElementCompositionPreview::SetElementChildVisual(rootGrid, visual);
    }

    if (visual)
    {
        m_canDisplayPrimaryContent = true;
        if (auto const rootGrid = m_rootGrid.get())
        {
            rootGrid.Children().Clear();
        }
        visual.Properties().InsertScalar(L"Progress", 0.0F);

        // Tie the animated visual's Progress property to the player Progress with an ExpressionAnimation.
        auto compositor = visual.Compositor();
        auto progressAnimation = compositor.CreateExpressionAnimation(L"_.Progress");
        progressAnimation.SetReferenceParameter(L"_", m_progressPropertySet);
        visual.Properties().StartAnimation(L"Progress", progressAnimation);
    }
    else
    {
        m_canDisplayPrimaryContent = false;
        if (auto const iconSource = FallbackIconSource())
        {
            auto const iconElement = SharedHelpers::MakeIconElementFrom(iconSource);
            if (auto const rootGrid = m_rootGrid.get())
            {
                rootGrid.Children().Clear();
                rootGrid.Children().InsertAt(0, iconElement);
            }
        }
    }
}

void AnimatedIcon::OnFallbackIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    if (!m_canDisplayPrimaryContent)
    {
        if (auto const iconSource = FallbackIconSource())
        {
            auto const iconElement = SharedHelpers::MakeIconElementFrom(iconSource);
            if (auto const rootGrid = m_rootGrid.get())
            {
                rootGrid.Children().Clear();
                rootGrid.Children().InsertAt(0, iconElement);
            }
        }
    }
}

void AnimatedIcon::OnForegroundPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    if (auto const ForegroundSolidColorBrush = Foreground().try_as<winrt::SolidColorBrush>())
    {
        if (auto const source = Source())
        {
            source.SetColorProperty(L"Foreground", ForegroundSolidColorBrush.Color());
        }
    }
}

void AnimatedIcon::OnAnimationCompleted(winrt::IInspectable const&, winrt::CompositionBatchCompletedEventArgs const&)
{
    if (m_batch)
    {
        m_batchCompletedRevoker.revoke();
    }
    m_isPlaying = false;
    //m_currentSegmentLength = 1.0f;
    switch (m_queueBehavior)
    {
    case winrt::AnimatedIconAnimationQueueBehavior::Cut:
        break;
    case winrt::AnimatedIconAnimationQueueBehavior::QueueOne:
        if (!m_queuedState.empty())
        {
            TransitionAndUpdateStates(m_currentState, m_queuedState);
        }
        break;
    case winrt::AnimatedIconAnimationQueueBehavior::SpeedUpQueueOne:
        if (!m_queuedState.empty())
        {
            TransitionAndUpdateStates(m_currentState, m_queuedState);
        }
        break;
    }
}

// Test hooks
void AnimatedIcon::SetAnimationQueueBehavior(winrt::AnimatedIconAnimationQueueBehavior behavior)
{
    m_queueBehavior = behavior;
}


void AnimatedIcon::SetDurationMultiplier(float multiplier)
{
    m_durationMultiplier = multiplier;
}

void AnimatedIcon::SetSpeedUpMultiplier(float multiplier)
{
    m_speedUpMultiplier = multiplier;
}

winrt::hstring AnimatedIcon::GetLastAnimationSegment()
{
    return m_lastAnimationSegment;
}

winrt::hstring AnimatedIcon::GetLastAnimationSegmentStart()
{
    return m_lastAnimationSegmentStart;
}

winrt::hstring AnimatedIcon::GetLastAnimationSegmentEnd()
{
    return m_lastAnimationSegmentEnd;
}
