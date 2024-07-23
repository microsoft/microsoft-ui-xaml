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
    virtual ~TitleBar();

    // IFrameworkElement
    void OnApplyTemplate();

    // UIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    

private:
    void UpdatePadding();
    void UpdateIcon();
    void UpdateBackButton();
    void UpdatePaneToggleButton();
    void UpdateHeight();
    void UpdateTheme();
    void UpdateTitle();
    void UpdateSubtitle();
    void UpdateHeader();
    void UpdateContent();
    void UpdateFooter();
    void UpdateDragRegion();
    void UpdateInteractableElementsList();

    void OnInputActivationChanged(const winrt::InputActivationListener& sender, const winrt::InputActivationListenerActivationChangedEventArgs& args);
    void OnBackButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnPaneToggleButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnLayoutUpdated(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    void LoadBackButton();
    void LoadPaneToggleButton();

    winrt::event_token m_inputActivationChangedToken{};
    winrt::Button::Click_revoker m_backButtonClickRevoker{};
    winrt::Button::Click_revoker m_paneToggleButtonClickRevoker{};
    winrt::FrameworkElement::ActualThemeChanged_revoker m_actualThemeChangedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_contentChangedRevoker{};

    std::list<winrt::FrameworkElement> m_interactableElementsList{};
    winrt::InputActivationListener m_inputActivationListener = nullptr;

    tracker_ref<winrt::ColumnDefinition> m_leftPaddingColumn{ this };
    tracker_ref<winrt::ColumnDefinition> m_rightPaddingColumn{ this };
    tracker_ref<winrt::Button> m_backButton{ this };
    tracker_ref<winrt::Button> m_paneToggleButton{ this };
    tracker_ref<winrt::TextBlock> m_titleTextBlock{ this };
    tracker_ref<winrt::TextBlock> m_subtitleTextBlock{ this };
    tracker_ref<winrt::Grid> m_contentAreaGrid{ this };
    tracker_ref<winrt::FrameworkElement> m_headerArea{ this };
    tracker_ref<winrt::FrameworkElement> m_contentArea{ this };
    tracker_ref<winrt::FrameworkElement> m_footerArea{ this };

    double m_compactModeThresholdWidth{ 0.0 };

    static constexpr std::wstring_view s_leftPaddingColumnName{ L"LeftPaddingColumn"sv };
    static constexpr std::wstring_view s_rightPaddingColumnName{ L"RightPaddingColumn"sv };
    static constexpr std::wstring_view s_layoutRootPartName{ L"PART_LayoutRoot"sv };
    static constexpr std::wstring_view s_backButtonPartName{ L"PART_BackButton"sv };
    static constexpr std::wstring_view s_paneToggleButtonPartName{ L"PART_PaneToggleButton"sv };
    static constexpr std::wstring_view s_headerContentPresenterPartName{ L"PART_HeaderContentPresenter"sv };
    static constexpr std::wstring_view s_contentPresenterGridPartName{ L"PART_ContentPresenterGrid"sv };
    static constexpr std::wstring_view s_contentPresenterPartName{ L"PART_ContentPresenter"sv };
    static constexpr std::wstring_view s_footerPresenterPartName{ L"PART_FooterContentPresenter"sv };

    static constexpr std::wstring_view s_compactVisualStateName{ L"Compact"sv };
    static constexpr std::wstring_view s_expandedVisualStateName{ L"Expanded"sv };
    static constexpr std::wstring_view s_compactHeightVisualStateName{ L"CompactHeight"sv };
    static constexpr std::wstring_view s_expandedHeightVisualStateName{ L"ExpandedHeight"sv };
    static constexpr std::wstring_view s_iconVisibleVisualStateName{ L"IconVisible"sv };
    static constexpr std::wstring_view s_iconCollapsedVisualStateName{ L"IconCollapsed"sv };
    static constexpr std::wstring_view s_backButtonDeactivatedVisualStateName{ L"BackButtonDeactivated"sv };
    static constexpr std::wstring_view s_backButtonVisibleVisualStateName{ L"BackButtonVisible"sv };
    static constexpr std::wstring_view s_backButtonCollapsedVisualStateName{ L"BackButtonCollapsed"sv };
    static constexpr std::wstring_view s_paneToggleButtonDeactivatedVisualStateName{ L"PaneToggleButtonDeactivated"sv };
    static constexpr std::wstring_view s_paneToggleButtonVisibleVisualStateName{ L"PaneToggleButtonVisible"sv };
    static constexpr std::wstring_view s_paneToggleButtonCollapsedVisualStateName{ L"PaneToggleButtonCollapsed"sv };
    static constexpr std::wstring_view s_titleTextDeactivatedVisualStateName{ L"TitleTextDeactivated"sv };
    static constexpr std::wstring_view s_titleTextVisibleVisualStateName{ L"TitleTextVisible"sv };
    static constexpr std::wstring_view s_titleTextCollapsedVisualStateName{ L"TitleTextCollapsed"sv };
    static constexpr std::wstring_view s_subtitleTextDeactivatedVisualStateName{ L"SubtitleTextDeactivated"sv };
    static constexpr std::wstring_view s_subtitleTextVisibleVisualStateName{ L"SubtitleTextVisible"sv };
    static constexpr std::wstring_view s_subtitleTextCollapsedVisualStateName{ L"SubtitleTextCollapsed"sv };
    static constexpr std::wstring_view s_headerVisibleVisualStateName{ L"HeaderVisible"sv };
    static constexpr std::wstring_view s_headerCollapsedVisualStateName{ L"HeaderCollapsed"sv };
    static constexpr std::wstring_view s_contentVisibleVisualStateName{ L"ContentVisible"sv };
    static constexpr std::wstring_view s_contentCollapsedVisualStateName{ L"ContentCollapsed"sv };
    static constexpr std::wstring_view s_footerVisibleVisualStateName{ L"FooterVisible"sv };
    static constexpr std::wstring_view s_footerCollapsedVisualStateName{ L"FooterCollapsed"sv };

    static constexpr std::wstring_view s_titleBarButtonForegroundColorName{ L"TitleBarButtonForegroundColor"sv };
    static constexpr std::wstring_view s_titleBarButtonBackgroundColorName{ L"TitleBarButtonBackgroundColor"sv };
    static constexpr std::wstring_view s_titleBarButtonHoverForegroundColorName{ L"TitleBarButtonHoverForegroundColor"sv };
    static constexpr std::wstring_view s_titleBarButtonHoverBackgroundColorName{ L"TitleBarButtonHoverBackgroundColor"sv };
    static constexpr std::wstring_view s_titleBarButtonPressedForegroundColorName{ L"TitleBarButtonPressedForegroundColor"sv };
    static constexpr std::wstring_view s_titleBarButtonPressedBackgroundColorName{ L"TitleBarButtonPressedBackgroundColor"sv };
    static constexpr std::wstring_view s_titleBarButtonInactiveForegroundColorName{ L"TitleBarButtonInactiveForegroundColor"sv };
};
