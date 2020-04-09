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
    void ChangeVisualState();

    tracker_ref<winrt::AnimatedVisualPlayer> m_player{ this };
};
