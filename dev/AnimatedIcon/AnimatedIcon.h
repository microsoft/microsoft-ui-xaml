// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "AnimatedIcon.g.h"
#include "AnimatedIcon.properties.h"

class AnimatedIcon :
    public ReferenceTracker<AnimatedIcon, DeriveFromPathIconHelper_base, winrt::AnimatedIcon>,
    public AnimatedIconProperties
{

public:
    AnimatedIcon();
    ~AnimatedIcon() {}

    // IFrameworkElement
    void OnApplyTemplate();
    void OnLoaded(winrt::IInspectable const&, winrt::RoutedEventArgs const&);
    void OnLayoutUpdatedAfterStateChanged(winrt::IInspectable const& sender, winrt::IInspectable const& args);

    // FrameworkElement overrides
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

    void OnSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnFallbackIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    static void OnAnimatedIconStatePropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);
    void OnAncestorAnimatedIconStatePropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);
    void OnStatePropertyChanged();

    static winrt::DependencyProperty AnimatedIconStateProperty() { return s_StateProperty; }

    // TestHooks
    void SetAnimationQueueBehavior(winrt::AnimatedIconAnimationQueueBehavior behavior);
    void SetDurationMultiplier(float multiplier);
    void SetSpeedUpMultiplier(float multiplier);
    winrt::hstring GetLastAnimationSegment();
    winrt::hstring GetLastAnimationSegmentStart();
    winrt::hstring GetLastAnimationSegmentEnd();
private:
    void TransitionAndUpdateStates(const winrt::hstring& fromState, const winrt::hstring& toState, float playbackMultiplier = 1.0f);
    void TransitionStates(const winrt::hstring& fromState, const winrt::hstring& toState, float playtbackMultiplier = 1.0f);
    void PlaySegment(float from, float to, float playbackMultiplier = 1.0f);
    void TrySetForegroundProperty(winrt::Color color, winrt::IAnimatedVisualSource2 const& source = nullptr);
    void TrySetForegroundProperty(winrt::IAnimatedVisualSource2 const& source = nullptr);
    void OnAnimationCompleted(winrt::IInspectable const&, winrt::CompositionBatchCompletedEventArgs const&);
    void OnForegroundPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnForegroundBrushColorPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void SetRootPanelChildToFallbackIcon();

    tracker_ref<winrt::IAnimatedVisual> m_animatedVisual{ this };
    tracker_ref<winrt::Panel> m_rootPanel{ this };

    winrt::hstring m_currentState{ L"" };
    winrt::hstring m_previousState{ L"" };
    winrt::hstring m_queuedState{ L"" };
    winrt::hstring m_pendingState{ L"" };
    winrt::hstring m_lastAnimationSegment{ L"" };
    winrt::hstring m_lastAnimationSegmentStart{ L"" };
    winrt::hstring m_lastAnimationSegmentEnd{ L"" };
    bool m_isPlaying{ false };
    bool m_canDisplayPrimaryContent{ true };
    float m_previousSegmentLength{ 1.0f };
    float m_durationMultiplier{ 1.0 };
    float m_speedUpMultiplier{ 7.0f };


    winrt::Composition::CompositionPropertySet m_progressPropertySet{ nullptr };
    winrt::Composition::CompositionScopedBatch m_batch{ nullptr };

    ScopedBatchCompleted_revoker m_batchCompletedRevoker{ };
    PropertyChanged_revoker m_ancestorStatePropertyChangedRevoker{};
    winrt::FrameworkElement::LayoutUpdated_revoker m_layoutUpdatedRevoker{};
    PropertyChanged_revoker m_foregroundColorPropertyChangedRevoker{};

    winrt::AnimatedIconAnimationQueueBehavior m_queueBehavior{ winrt::AnimatedIconAnimationQueueBehavior::SpeedUpQueueOne };
};
