// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ProgressBarTemplateSettings.h"

#include "ProgressBar.g.h"
#include "ProgressBar.properties.h"

class ProgressBar :
    public ReferenceTracker<ProgressBar, winrt::implementation::ProgressBarT>,
    public ProgressBarProperties
{

public:
    ProgressBar();

    // IFrameworkElement
    void OnApplyTemplate();

    // Internal
    void OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnShowErrorPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnShowPausedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void OnRangeBasePropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);

    void SetProgressBarIndicatorWidth();
    void UpdateStates();
    void UpdateWidthBasedTemplateSettings();
    void OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&);

    winrt::Grid::Loaded_revoker m_layoutRootLoadedRevoker{};
    winrt::Rectangle::Loaded_revoker m_progressBarIndicatorRevoker{};

    tracker_ref<winrt::Grid> m_layoutRoot{ this };
    tracker_ref<winrt::Rectangle> m_progressBarIndicator{ this };

    bool m_shouldUpdateWidthBasedTemplateSettings = false;
    bool m_isUpdating = true;

    static constexpr wstring_view s_LayoutRootName{ L"LayoutRoot" };
    static constexpr wstring_view s_ProgressBarIndicatorName{ L"ProgressBarIndicator" };
    static constexpr wstring_view s_ErrorStateName{ L"Error" };
    static constexpr wstring_view s_PausedStateName{ L"Paused" };
    static constexpr wstring_view s_IndeterminateStateName{ L"Indeterminate" };
    static constexpr wstring_view s_DeterminateStateName{ L"Determinate" };
    static constexpr wstring_view s_UpdatingStateName{ L"Updating" };
};
