// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ProgressRingIndeterminate.h"

#include "ProgressRing.g.h"
#include "ProgressRing.properties.h"

class ProgressRing :
    public ReferenceTracker<ProgressRing, winrt::implementation::ProgressRingT>,
    public ProgressRingProperties
{

public:
    ProgressRing();

    winrt::AutomationPeer OnCreateAutomationPeer();
    // IFrameworkElement
    void OnApplyTemplate();

    void OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnForegroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnForegroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnBackgroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnBackgroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);

private:
    void OnRangeBasePropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnOpacityPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void ApplyLottieAnimation();
    void SetLottieForegroundColor(winrt::impl::com_ref<AnimatedVisuals::ProgressRingIndeterminate> progressRingIndeterminate);
    void SetLottieBackgroundColor(winrt::impl::com_ref<AnimatedVisuals::ProgressRingIndeterminate> progressRingIndeterminate);
    void UpdateStates();
    void UpdateSegment();
    void UpdateRing();
    double GetStrokeThickness();

    tracker_ref<winrt::Grid> m_layoutRoot{ this };
    tracker_ref<winrt::Path> m_outlinePath{ this };
    tracker_ref<winrt::PathFigure> m_outlineFigure{ this };
    tracker_ref<winrt::ArcSegment> m_outlineArc{ this };
    tracker_ref<winrt::Path> m_ringPath{ this };
    tracker_ref<winrt::PathFigure> m_ringFigure{ this };
    tracker_ref<winrt::ArcSegment> m_ringArc{ this };
    tracker_ref<winrt::AnimatedVisualPlayer> m_player{ this };
};
