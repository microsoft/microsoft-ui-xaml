﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AnimatedIcon.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include <AnimatedIconTestHooks.h>
#include "Utils.h"
#include <mutex>

static constexpr wstring_view s_progressPropertyName{ L"Progress"sv };
static constexpr wstring_view s_foregroundPropertyName{ L"Foreground"sv };
static constexpr wstring_view s_transitionInfix{ L"To"sv };
static constexpr wstring_view s_transitionStartSuffix{ L"_Start"sv };
static constexpr wstring_view s_transitionEndSuffix{ L"_End"sv };
AnimatedIcon::AnimatedIcon()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_AnimatedIcon);
    m_progressPropertySet = winrt::Window::Current().Compositor().CreatePropertySet();
    m_progressPropertySet.InsertScalar(s_progressPropertyName, 0);
    Loaded({ this, &AnimatedIcon::OnLoaded });

    RegisterPropertyChangedCallback(winrt::IconElement::ForegroundProperty(), { this, &AnimatedIcon::OnForegroundPropertyChanged});
    RegisterPropertyChangedCallback(winrt::FrameworkElement::FlowDirectionProperty(), { this, &AnimatedIcon::OnFlowDirectionPropertyChanged });
}

void AnimatedIcon::OnApplyTemplate()
{
    __super::OnApplyTemplate();
    // Construct the visual from the Source property in on apply template so that it participates
    // in the initial measure for the object.
    ConstructAndInsertVisual();
    auto const panel = winrt::VisualTreeHelper::GetChild(*this, 0).as<winrt::Panel>();
    m_rootPanel.set(panel);
    m_currentState = GetState(*this);

    if (panel)
    {
        // Animated icon implements IconElement through PathIcon. We don't need the Path that
        // PathIcon creates, however when you set the foreground on AnimatedIcon, it assumes
        // its grid's first child has a fill property which it sets by known index. So we
        // keep this child around but collapse it so this behavior doesn't crash us when the
        // fallback is used.
        if (panel.Children().Size() > 0)
        {
            if (auto const path = panel.Children().GetAt(0))
            {
                path.Visibility(winrt::Visibility::Collapsed);
            }
        }
        if (auto const visual = m_animatedVisual.get())
        {
            winrt::ElementCompositionPreview::SetElementChildVisual(panel, visual.RootVisual());
        }

        TrySetForegroundProperty();
    }
}

void AnimatedIcon::OnLoaded(winrt::IInspectable const&, winrt::RoutedEventArgs const&)
{
    // AnimatedIcon might get added to a UI which has already set the State property on an ancestor.
    // If this is the case and the animated icon being added doesn't have its own state property
    // We copy the ancestor value when we load. Additionally we attach to our ancestor's property
    // changed event for AnimatedIcon.State to copy the value to AnimatedIcon.
    auto const property = winrt::AnimatedIcon::StateProperty();

    auto const [ancestorWithState, stateValue] = [this, property]()
    {
        auto parent = winrt::VisualTreeHelper::GetParent(*this);
        while (parent)
        {
            auto const stateValue = parent.GetValue(property);
            if (!unbox_value<winrt::hstring>(stateValue).empty())
            {
                return std::make_tuple(parent, stateValue);
            }
            parent = winrt::VisualTreeHelper::GetParent(parent);
        }
        return std::make_tuple(static_cast<winrt::DependencyObject>(nullptr), winrt::box_value(winrt::hstring{}));
    }();

    if (unbox_value<winrt::hstring>(GetValue(property)).empty())
    {
        SetValue(property, stateValue);
    }

    if (ancestorWithState)
    {
        m_ancestorStatePropertyChangedRevoker = RegisterPropertyChanged(ancestorWithState, property, { this, &AnimatedIcon::OnAncestorAnimatedIconStatePropertyChanged });
    }

    // Wait until loaded to apply the fallback icon source property because we need the icon source
    // properties to be set before we create the icon element from it.  If those poperties are bound in,
    // they will not have been set during OnApplyTemplate.
    OnFallbackIconSourcePropertyChanged(nullptr);
}


winrt::Size AnimatedIcon::MeasureOverride(winrt::Size const& availableSize)
{
    if (auto const visual = m_animatedVisual.get())
    {
        // Animated Icon scales using the Uniform strategy, meaning that it scales the horizonal and vertical
        // dimensions equally by the maximum amount that doesn't exceed the available size in either dimension.
        // If the available size is infinite in both dimensions then we don't scale the visual. Otherwise, we
        // calculate the scale factor by comparing the default visual size to the available size. This produces 2
        // scale factors, one for each dimension. We choose the smaller of the scale factors to not exceed the
        // available size in that dimension.
        auto const visualSize = visual.Size();
        if (visualSize != winrt::float2::zero())
        {
            const auto widthScale = availableSize.Width == std::numeric_limits<double>::infinity() ? std::numeric_limits<float>::infinity() : availableSize.Width / visualSize.x;
            const auto heightScale = availableSize.Height == std::numeric_limits<double>::infinity() ? std::numeric_limits<float>::infinity() : availableSize.Height / visualSize.y;
            if (widthScale == std::numeric_limits<double>::infinity() && heightScale == std::numeric_limits<double>::infinity())
            {
                return visualSize;
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
        auto const scale = [finalSize, visualSize]()
        {
            auto scale = static_cast<winrt::float2>(finalSize) / visualSize;
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
        const auto rootVisual = visual.RootVisual();
        rootVisual.Offset({ offset, 0.0f });
        rootVisual.Size(arrangedSize);
        rootVisual.Scale({ scale, 1.0f });
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
}

void AnimatedIcon::OnAncestorAnimatedIconStatePropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyProperty& args)
{
    SetValue(AnimatedIconProperties::s_StateProperty, sender.GetValue(args));
}

// When we receive a state change it might be erroneous. This is because these state changes often come from Animated Icon's parent control's
// Visual state manager.  Many of our controls assume that it is okay to call GoToState("Foo") followed immediately by GoToState("Bar") in
// order to end up in the "Bar" state. However Animated Icon also cares what state you are coming from, so this pattern would not trigger
// the NormalToBar transition but instead NormalToFoo followed by FooToBar. Since we can't change these controls logic in WinUI2 we handle
// this by waiting until the next layout cycle to play an animated icon transition. However, the state dependency property changed is not
// enough to ensure that a layout updated will trigger, so we must also invalidate a layout property, arrange was chosen arbitrarily.
void AnimatedIcon::OnStatePropertyChanged()
{
    m_pendingState = ValueHelper<winrt::hstring>::CastOrUnbox(this->GetValue(AnimatedIconStateProperty()));
    m_layoutUpdatedRevoker = this->LayoutUpdated(winrt::auto_revoke, { this, &AnimatedIcon::OnLayoutUpdatedAfterStateChanged });
    SharedHelpers::QueueCallbackForCompositionRendering(
        [strongThis = get_strong()]
        {
            strongThis->InvalidateArrange();
        }
    );
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
            // If we already have too many queued states, cancel the current animation with the previously queued transition
            // then Queue this new transition.
            if (m_queuedStates.size() >= m_queueLength)
            {
                TransitionAndUpdateStates(m_currentState, m_queuedStates.front());
            }
            m_queuedStates.push(m_pendingState);
        }
        else
        {
            TransitionAndUpdateStates(m_currentState, m_pendingState);
        }
        break;
    case winrt::AnimatedIconAnimationQueueBehavior::SpeedUpQueueOne:
        if (m_isPlaying)
        {
            // If we already have too many queued states, cancel the current animation with the previously queued transition
            //  played speed up then Queue this new transition.
            if (m_queuedStates.size() >= m_queueLength)
            {
                // Cancel the previous animation completed handler, before we cancel that animation by starting a new one.
                if (m_batch)
                {
                    m_batchCompletedRevoker.revoke();
                }
                TransitionAndUpdateStates(m_currentState, m_queuedStates.front(), m_speedUpMultiplier);
                m_queuedStates.push(m_pendingState);
            }
            else
            {
                m_queuedStates.push(m_pendingState);
                if (!m_isSpeedUp)
                {
                    // Cancel the previous animation completed handler, before we cancel that animation by starting a new one.
                    if (m_batch)
                    {
                        m_batchCompletedRevoker.revoke();
                    }

                    m_isSpeedUp = true;

                    auto const markers = Source().Markers();
                    winrt::hstring transitionEndName = StringUtil::FormatString(L"%1!s!%2!s!%3!s!%4!s!", m_previousState.c_str(), s_transitionInfix.data(), m_currentState.c_str(), s_transitionEndSuffix.data());
                    auto const hasEndMarker = markers.HasKey(transitionEndName);
                    if (hasEndMarker)
                    {
                        PlaySegment(NAN, static_cast<float>(markers.Lookup(transitionEndName)), nullptr, m_speedUpMultiplier);
                    }
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
    std::once_flag cleanedUpFlag;
    std::function<void()> cleanupAction = [this, fromState, toState, &cleanedUpFlag]()
    {
        std::call_once(cleanedUpFlag, [this, fromState, toState]() {
                m_previousState = fromState;
                m_currentState = toState;
                if (!m_queuedStates.empty())
                {
                    m_queuedStates.pop();
                }
            });
    };
    TransitionStates(fromState, toState, cleanupAction, playbackMultiplier);
    cleanupAction();
}

void AnimatedIcon::TransitionStates(const winrt::hstring& fromState, const winrt::hstring& toState, const std::function<void()>& cleanupAction, float playbackMultiplier)
{
    if (auto const source = Source())
    {
        if (auto const markers = source.Markers())
        {
            winrt::hstring transitionName = StringUtil::FormatString(L"%1!s!%2!s!%3!s!", fromState.c_str(), s_transitionInfix.data(), toState.c_str());
            winrt::hstring transitionStartName = StringUtil::FormatString(L"%1!s!%2!s!", transitionName.c_str(), s_transitionStartSuffix.data());
            winrt::hstring transitionEndName = StringUtil::FormatString(L"%1!s!%2!s!", transitionName.c_str(), s_transitionEndSuffix.data());

            auto const hasStartMarker = markers.HasKey(transitionStartName);
            auto const hasEndMarker = markers.HasKey(transitionEndName);
            if (hasStartMarker && hasEndMarker)
            {
                auto const fromProgress = static_cast<float>(markers.Lookup(transitionStartName));
                auto const toProgress = static_cast<float>(markers.Lookup(transitionEndName));
                PlaySegment(fromProgress, toProgress, cleanupAction, playbackMultiplier);
                m_lastAnimationSegmentStart = transitionStartName;
                m_lastAnimationSegmentEnd = transitionEndName;
            }
            else if (hasEndMarker)
            {
                auto const toProgress = static_cast<float>(markers.Lookup(transitionEndName));
                m_progressPropertySet.InsertScalar(s_progressPropertyName, toProgress);
                m_lastAnimationSegmentStart = L"";
                m_lastAnimationSegmentEnd = transitionEndName;
            }
            else if (hasStartMarker)
            {
                auto const toProgress = static_cast<float>(markers.Lookup(transitionStartName));
                m_progressPropertySet.InsertScalar(s_progressPropertyName, toProgress);
                m_lastAnimationSegmentStart = transitionStartName;
                m_lastAnimationSegmentEnd = L"";
            }
            else if (markers.HasKey(transitionName))
            {
                auto const toProgress = static_cast<float>(markers.Lookup(transitionName));
                m_progressPropertySet.InsertScalar(s_progressPropertyName, toProgress);
                m_lastAnimationSegmentStart = L"";
                m_lastAnimationSegmentEnd = transitionName;
            }
            else if (markers.HasKey(toState))
            {
                auto const toProgress = static_cast<float>(markers.Lookup(toState));
                m_progressPropertySet.InsertScalar(s_progressPropertyName, toProgress);
                m_lastAnimationSegmentStart = L"";
                m_lastAnimationSegmentEnd = toState;
            }
            else
            {
                // Since we can't find an animation for this transition, try to find one that ends in the same place
                // and cut to that position instead.
                auto const [found, value] = [toState, markers, this]()
                {
                    winrt::hstring fragment = StringUtil::FormatString(L"%1!s!%2!s!%3!s!", s_transitionInfix.data(), toState.c_str(), s_transitionEndSuffix.data());
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
                    m_progressPropertySet.InsertScalar(s_progressPropertyName, value);
                }
                else
                {
                    // We also support setting the state proprety to a float value, which instructs the animated icon
                    // to animate the Progress property to the provided value. Because wcstof returns 0.0f when the
                    // provided string doesn't parse to a float we can't distinguish between the string "0.0" and
                    // the string "a" (for example) from the parse output alone. Instead we use the wcstof's second
                    // parameter to determine if the 0.0 return value came from a valid parse or from the default return.
                    wchar_t* strEnd = nullptr;
                    auto const parsedFloat = wcstof(toState.c_str(), &strEnd);

                    if(strEnd == toState.c_str() + toState.size())
                    {
                        PlaySegment(NAN, parsedFloat, cleanupAction, playbackMultiplier);
                        m_lastAnimationSegmentStart = L"";
                        m_lastAnimationSegmentEnd = toState;
                    }
                    else
                    {
                        // None of our attempt to find an animation to play or frame to show have worked, so just cut
                        // to frame 0.
                        m_progressPropertySet.InsertScalar(s_progressPropertyName, 0.0);
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

void AnimatedIcon::PlaySegment(float from, float to, const std::function<void()>& cleanupAction, float playbackMultiplier)
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
        m_progressPropertySet.InsertScalar(s_progressPropertyName, to);
        if (cleanupAction)
        {
            cleanupAction();
        }
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
        m_progressPropertySet.StartAnimation(s_progressPropertyName, animation);
        m_batch.End();
    }
}

void AnimatedIcon::OnSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    if(!ConstructAndInsertVisual())
    {
        SetRootPanelChildToFallbackIcon();
    }
}

void AnimatedIcon::UpdateMirrorTransform()
{
    auto const scaleTransform = [this]()
    {
        if (!m_scaleTransform)
        {
            // Initialize the scale transform that will be used for mirroring and the
            // render transform origin as center in order to have the icon mirrored in place.
            winrt::Windows::UI::Xaml::Media::ScaleTransform scaleTransform;

            RenderTransform(scaleTransform);
            RenderTransformOrigin({ 0.5, 0.5 });
            m_scaleTransform.set(scaleTransform);
            return scaleTransform;
        }
        return m_scaleTransform.get();
    }();


    scaleTransform.ScaleX(FlowDirection() == winrt::FlowDirection::RightToLeft && !MirroredWhenRightToLeft() && m_canDisplayPrimaryContent ? -1.0f : 1.0f);
}

void AnimatedIcon::OnMirroredWhenRightToLeftPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateMirrorTransform();
}

bool AnimatedIcon::ConstructAndInsertVisual()
{
    auto const visual = [this]()
    {
        if (auto const source = Source())
        {
            TrySetForegroundProperty(source);

            winrt::IInspectable diagnostics{};
            auto const visual = source.TryCreateAnimatedVisual(winrt::Window::Current().Compositor(), diagnostics);
            m_animatedVisual.set(visual);
            return visual ? visual.RootVisual() : nullptr;
        }
        else
        {
            m_animatedVisual.set(nullptr);
            return static_cast<winrt::Visual>(nullptr);
        }
    }();

    if (auto const rootPanel = m_rootPanel.get())
    {
        winrt::ElementCompositionPreview::SetElementChildVisual(rootPanel, visual);
    }

    UpdateMirrorTransform();

    if (visual)
    {
        m_canDisplayPrimaryContent = true;
        if (auto const rootPanel = m_rootPanel.get())
        {
            // Remove the second child, if it exists, as this is the fallback icon.
            // Which we don't need because we have a visual now.
            if (rootPanel.Children().Size() > 1)
            {
                rootPanel.Children().RemoveAt(1);
            }
        }
        visual.Properties().InsertScalar(s_progressPropertyName, 0.0F);

        // Tie the animated visual's Progress property to the player Progress with an ExpressionAnimation.
        auto const compositor = visual.Compositor();
        auto const expression = StringUtil::FormatString(L"_.%1!s!", s_progressPropertyName.data());
        auto const progressAnimation = compositor.CreateExpressionAnimation(expression);
        progressAnimation.SetReferenceParameter(L"_", m_progressPropertySet);
        visual.Properties().StartAnimation(s_progressPropertyName, progressAnimation);

        return true;
    }
    else
    {
        m_canDisplayPrimaryContent = false;
        return false;
    }
}

void AnimatedIcon::OnFallbackIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    if (!m_canDisplayPrimaryContent)
    {
        SetRootPanelChildToFallbackIcon();
    }
}

void AnimatedIcon::SetRootPanelChildToFallbackIcon()
{
    if (auto const iconSource = FallbackIconSource())
    {
        auto const iconElement = iconSource.CreateIconElement();
        if (auto const rootPanel = m_rootPanel.get())
        {
            // Remove the second child, if it exists, as this is the previous
            // fallback icon which we don't need because we have a visual now.
            if (rootPanel.Children().Size() > 1)
            {
                rootPanel.Children().RemoveAt(1);
            }
            rootPanel.Children().Append(iconElement);
        }
    }
}

void AnimatedIcon::OnForegroundPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    m_foregroundColorPropertyChangedRevoker.revoke();
    if (auto const foregroundSolidColorBrush = Foreground().try_as<winrt::SolidColorBrush>())
    {
        m_foregroundColorPropertyChangedRevoker = RegisterPropertyChanged(foregroundSolidColorBrush, winrt::SolidColorBrush::ColorProperty(), { this, &AnimatedIcon::OnForegroundBrushColorPropertyChanged });
        TrySetForegroundProperty(foregroundSolidColorBrush.Color());
    }
}

void AnimatedIcon::OnFlowDirectionPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    UpdateMirrorTransform();
}

void AnimatedIcon::OnForegroundBrushColorPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    TrySetForegroundProperty(sender.GetValue(args).as<winrt::Color>());
}

void AnimatedIcon::TrySetForegroundProperty(winrt::IAnimatedVisualSource2 const& source)
{
    if (auto const foregroundSolidColorBrush = Foreground().try_as<winrt::SolidColorBrush>())
    {
        TrySetForegroundProperty(foregroundSolidColorBrush.Color(), source);
    }
}

void AnimatedIcon::TrySetForegroundProperty(winrt::Color color, winrt::IAnimatedVisualSource2 const& source)
{
    auto const localSource = source ? source : Source();
    if (localSource)
    {
        localSource.SetColorProperty(s_foregroundPropertyName, color);
    }
}

void AnimatedIcon::OnAnimationCompleted(winrt::IInspectable const&, winrt::CompositionBatchCompletedEventArgs const&)
{
    if (m_batch)
    {
        m_batchCompletedRevoker.revoke();
    }
    m_isPlaying = false;

    switch (m_queueBehavior)
    {
    case winrt::AnimatedIconAnimationQueueBehavior::Cut:
        break;
    case winrt::AnimatedIconAnimationQueueBehavior::QueueOne:
        if (!m_queuedStates.empty())
        {
            TransitionAndUpdateStates(m_currentState, m_queuedStates.front());
        }
        break;
    case winrt::AnimatedIconAnimationQueueBehavior::SpeedUpQueueOne:
        if (!m_queuedStates.empty())
        {
            if (m_queuedStates.size() == 1)
            {
                TransitionAndUpdateStates(m_currentState, m_queuedStates.front());
            }
            else
            {
                TransitionAndUpdateStates(m_currentState, m_queuedStates.front(), m_isSpeedUp ? m_speedUpMultiplier : 1.0f);
            }
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

void AnimatedIcon::SetQueueLength(unsigned int length)
{
    m_queueLength = length;
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
