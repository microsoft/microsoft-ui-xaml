// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "AnimatedVisuals/ProgressRingIndeterminate.h"
#include "AnimatedVisuals/ProgressRingDeterminate.h"

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

    void OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsActivePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnDeterminateSourcePropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnIndeterminateSourcePropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnForegroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnForegroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnBackgroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnBackgroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);

    void OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMaximumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMinimumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void SetAnimatedVisualPlayerSource();
    void SetLottieForegroundColor(const winrt::IAnimatedVisualSource);
    void SetLottieBackgroundColor(const winrt::IAnimatedVisualSource);
    void OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void UpdateStates();
    void ApplyTemplateSettings();
    void UpdateLottieProgress();

    void CoerceMinimum();
    void CoerceMaximum();
    void CoerceValue();
    bool IsInBounds(double value);

    tracker_ref<winrt::Grid> m_layoutRoot{ this };
    tracker_ref<winrt::AnimatedVisualPlayer> m_player{ this };

    double m_oldValue{ 0 };
    bool m_rangeBasePropertyUpdating{ false };
};
