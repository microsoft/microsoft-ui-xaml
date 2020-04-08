// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ProgressRingIndeterminate.h"
#include "ProgressRingTemplateSettings.h"

#include "ProgressRing.g.h"
#include "ProgressRing.properties.h"

class ProgressRing :
    public ReferenceTracker<ProgressRing, winrt::implementation::ProgressRingT>,
    public ProgressRingProperties
{

public:
    ProgressRing();

    winrt::AutomationPeer OnCreateAutomationPeer();

    void OnApplyTemplate();

    void OnIsActivePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnForegroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnForegroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnBackgroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnBackgroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);

private:
    void SetAnimatedVisualPlayerSource();
    void SetLottieForegroundColor(winrt::impl::com_ref<AnimatedVisuals::ProgressRingIndeterminate> progressRingIndeterminate);
    void SetLottieBackgroundColor(winrt::impl::com_ref<AnimatedVisuals::ProgressRingIndeterminate> progressRingIndeterminate);
    void OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void ChangeVisualState();
    void ApplyTemplateSettings();

    tracker_ref<winrt::AnimatedVisualPlayer> m_player{ this };
    tracker_ref<winrt::Grid> m_layoutRoot{ this };

    void UpdateLottieProgress();
    void UpdateStates();
    void UpdateSegment();
    void UpdateRing();
    double GetStrokeThickness();

    double m_oldValue{ 0 };

    tracker_ref<winrt::Grid> m_layoutRoot{ this };
    tracker_ref<winrt::Path> m_outlinePath{ this };
    tracker_ref<winrt::PathFigure> m_outlineFigure{ this };
    tracker_ref<winrt::ArcSegment> m_outlineArc{ this };
    tracker_ref<winrt::Path> m_ringPath{ this };
    tracker_ref<winrt::PathFigure> m_ringFigure{ this };
    tracker_ref<winrt::ArcSegment> m_ringArc{ this };
    tracker_ref<winrt::AnimatedVisualPlayer> m_determinatePlayer{ this };
    tracker_ref<winrt::AnimatedVisualPlayer> m_indeterminatePlayer{ this };
};
