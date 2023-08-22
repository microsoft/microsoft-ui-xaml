// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TitleBar.g.h"
#include "TitleBar.properties.h"

class TitleBar :
    public ReferenceTracker<TitleBar, winrt::implementation::TitleBarT>,
    public TitleBarProperties
{

public:
    TitleBar();
    ~TitleBar() {}

    // IFrameworkElement
    void OnApplyTemplate();

    // UIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    void OnIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsBackButtonVisiblePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnCustomContentPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnTitlePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void UpdateVisibility();
    void UpdatePadding();
    void UpdateIcon();
    void UpdateBackButton();
    void UpdateHeight();
    void UpdateTheme();
    void UpdateTitle();

    void OnWindowActivated(const winrt::IInspectable& sender, const winrt::WindowActivatedEventArgs& args);
    void OnTitleBarMetricsChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnTitleBarIsVisibleChanged(const winrt::CoreApplicationViewTitleBar& sender, const winrt::IInspectable& args);
    void OnBackButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);

    winrt::CoreApplicationViewTitleBar::LayoutMetricsChanged_revoker m_titleBarMetricsChangedRevoker{};
    winrt::CoreApplicationViewTitleBar::IsVisibleChanged_revoker m_titleBarIsVisibleChangedRevoker{};
    winrt::Button::Click_revoker m_backButtonClickRevoker{};
    winrt::FrameworkElement::ActualThemeChanged_revoker m_actualThemeChangedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_customContentChangedRevoker{};

    tracker_ref<winrt::ColumnDefinition> m_leftPaddingColumn{ this };
    tracker_ref<winrt::ColumnDefinition> m_rightPaddingColumn{ this };
    tracker_ref<winrt::Grid> m_layoutRoot{ this };
    tracker_ref<winrt::TextBlock> m_titleTextBlock{ this };
    tracker_ref<winrt::FrameworkElement> m_customArea{ this };

    bool m_isTitleSquished{ false };
};
