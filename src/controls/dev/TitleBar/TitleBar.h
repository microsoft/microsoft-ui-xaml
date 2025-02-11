// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TitleBarTrace.h"
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
    void GoToState(std::wstring_view const& stateName, bool useTransitions);
    void UpdatePadding();
    void UpdateIcon();
    void UpdateBackButton();
    void UpdatePaneToggleButton();
    void UpdateHeight();
    void UpdateTheme();
    void UpdateTitle();
    void UpdateSubtitle();
    void UpdateLeftContent();
    void UpdateCenterContent();
    void UpdateRightContent();
    void UpdateDragRegion();
    void UpdateIconRegion();
    void UpdateInteractableElementsList();
    void UpdateLeftContentSpacing();

    void OnInputActivationChanged(const winrt::InputActivationListener& sender, const winrt::InputActivationListenerActivationChangedEventArgs& args);
    void OnWindowRectChanged(const winrt::InputNonClientPointerSource& sender, const winrt::WindowRectChangedEventArgs& args);
    void OnBackButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnPaneToggleButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnFlowDirectionChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnIconLayoutUpdated(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    void LoadBackButton();
    void LoadPaneToggleButton();

    winrt::InputNonClientPointerSource const& GetInputNonClientPointerSource();
    winrt::Windows::Graphics::RectInt32 const GetBounds(const winrt::FrameworkElement& element);
    winrt::WindowId GetAppWindowId();

    winrt::event_token m_inputActivationChangedToken{};
    winrt::event_token m_windowRectChangedToken{};
    winrt::Button::Click_revoker m_backButtonClickRevoker{};
    winrt::Button::Click_revoker m_paneToggleButtonClickRevoker{};
    winrt::FrameworkElement::ActualThemeChanged_revoker m_actualThemeChangedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_sizeChangedRevoker;
    winrt::FrameworkElement::LayoutUpdated_revoker m_iconLayoutUpdatedRevoker{};
    PropertyChanged_revoker m_flowDirectionChangedRevoker{};

    std::list<winrt::FrameworkElement> m_interactableElementsList{};
    winrt::InputActivationListener m_inputActivationListener = nullptr;
    winrt::InputNonClientPointerSource m_inputNonClientPointerSource{ nullptr };
    winrt::WindowId m_lastAppWindowId{};

    tracker_ref<winrt::ColumnDefinition> m_leftPaddingColumn{ this };
    tracker_ref<winrt::ColumnDefinition> m_rightPaddingColumn{ this };
    tracker_ref<winrt::Button> m_backButton{ this };
    tracker_ref<winrt::Button> m_paneToggleButton{ this };
    tracker_ref<winrt::FrameworkElement> m_iconViewbox{ this };
    tracker_ref<winrt::Grid> m_centerContentAreaGrid{ this };
    tracker_ref<winrt::FrameworkElement> m_leftContentArea{ this };
    tracker_ref<winrt::FrameworkElement> m_centerContentArea{ this };
    tracker_ref<winrt::FrameworkElement> m_rightContentArea{ this };

    double m_compactModeThresholdWidth{ 0.0 };
    bool m_isCompact{ false };

    static constexpr std::wstring_view s_leftPaddingColumnName{ L"LeftPaddingColumn"sv };
    static constexpr std::wstring_view s_rightPaddingColumnName{ L"RightPaddingColumn"sv };
    static constexpr std::wstring_view s_layoutRootPartName{ L"PART_LayoutRoot"sv };
    static constexpr std::wstring_view s_backButtonPartName{ L"PART_BackButton"sv };
    static constexpr std::wstring_view s_paneToggleButtonPartName{ L"PART_PaneToggleButton"sv };
    static constexpr std::wstring_view s_iconViewboxPartName{ L"PART_Icon"sv };
    static constexpr std::wstring_view s_leftContentPresenterPartName{ L"PART_LeftContentPresenter"sv };
    static constexpr std::wstring_view s_centerContentPresenterGridPartName{ L"PART_CenterContentPresenterGrid"sv };
    static constexpr std::wstring_view s_centerContentPresenterPartName{ L"PART_CenterContentPresenter"sv };
    static constexpr std::wstring_view s_rightContentPresenterPartName{ L"PART_RightContentPresenter"sv };

    static constexpr std::wstring_view s_compactVisualStateName{ L"Compact"sv };
    static constexpr std::wstring_view s_expandedVisualStateName{ L"Expanded"sv };
    static constexpr std::wstring_view s_compactHeightVisualStateName{ L"CompactHeight"sv };
    static constexpr std::wstring_view s_expandedHeightVisualStateName{ L"ExpandedHeight"sv };
    static constexpr std::wstring_view s_defaultSpacingVisualStateName{ L"DefaultSpacing"sv };
    static constexpr std::wstring_view s_negativeInsetVisualStateName{ L"NegativeInsetSpacing"sv };
    static constexpr std::wstring_view s_iconVisibleVisualStateName{ L"IconVisible"sv };
    static constexpr std::wstring_view s_iconCollapsedVisualStateName{ L"IconCollapsed"sv };
    static constexpr std::wstring_view s_iconDeactivatedVisualStateName{ L"IconDeactivated"sv };
    static constexpr std::wstring_view s_backButtonVisibleVisualStateName{ L"BackButtonVisible"sv };
    static constexpr std::wstring_view s_backButtonCollapsedVisualStateName{ L"BackButtonCollapsed"sv };
    static constexpr std::wstring_view s_backButtonDeactivatedVisualStateName{ L"BackButtonDeactivated"sv };
    static constexpr std::wstring_view s_paneToggleButtonVisibleVisualStateName{ L"PaneToggleButtonVisible"sv };
    static constexpr std::wstring_view s_paneToggleButtonCollapsedVisualStateName{ L"PaneToggleButtonCollapsed"sv };
    static constexpr std::wstring_view s_paneToggleButtonDeactivatedVisualStateName{ L"PaneToggleButtonDeactivated"sv };
    static constexpr std::wstring_view s_titleTextVisibleVisualStateName{ L"TitleTextVisible"sv };
    static constexpr std::wstring_view s_titleTextCollapsedVisualStateName{ L"TitleTextCollapsed"sv };
    static constexpr std::wstring_view s_titleTextDeactivatedVisualStateName{ L"TitleTextDeactivated"sv };
    static constexpr std::wstring_view s_subtitleTextVisibleVisualStateName{ L"SubtitleTextVisible"sv };
    static constexpr std::wstring_view s_subtitleTextCollapsedVisualStateName{ L"SubtitleTextCollapsed"sv };
    static constexpr std::wstring_view s_subtitleTextDeactivatedVisualStateName{ L"SubtitleTextDeactivated"sv };
    static constexpr std::wstring_view s_leftContentVisibleVisualStateName{ L"LeftContentVisible"sv };
    static constexpr std::wstring_view s_leftContentCollapsedVisualStateName{ L"LeftContentCollapsed"sv };
    static constexpr std::wstring_view s_leftContentDeactivatedVisualStateName{ L"LeftContentDeactivated"sv };
    static constexpr std::wstring_view s_centerContentVisibleVisualStateName{ L"CenterContentVisible"sv };
    static constexpr std::wstring_view s_centerContentCollapsedVisualStateName{ L"CenterContentCollapsed"sv };
    static constexpr std::wstring_view s_centerContentDeactivatedVisualStateName{ L"CenterContentDeactivated"sv };
    static constexpr std::wstring_view s_rightContentVisibleVisualStateName{ L"RightContentVisible"sv };
    static constexpr std::wstring_view s_rightContentCollapsedVisualStateName{ L"RightContentCollapsed"sv };
    static constexpr std::wstring_view s_rightContentDeactivatedVisualStateName{ L"RightContentDeactivated"sv };

    static constexpr std::wstring_view s_titleBarCaptionButtonForegroundColorName{ L"TitleBarCaptionButtonForegroundColor"sv };
    static constexpr std::wstring_view s_titleBarCaptionButtonBackgroundColorName{ L"TitleBarCaptionButtonBackgroundColor"sv };
    static constexpr std::wstring_view s_titleBarCaptionButtonHoverForegroundColorName{ L"TitleBarCaptionButtonHoverForegroundColor"sv };
    static constexpr std::wstring_view s_titleBarCaptionButtonHoverBackgroundColorName{ L"TitleBarCaptionButtonHoverBackgroundColor"sv };
    static constexpr std::wstring_view s_titleBarCaptionButtonPressedForegroundColorName{ L"TitleBarCaptionButtonPressedForegroundColor"sv };
    static constexpr std::wstring_view s_titleBarCaptionButtonPressedBackgroundColorName{ L"TitleBarCaptionButtonPressedBackgroundColor"sv };
    static constexpr std::wstring_view s_titleBarCaptionButtonInactiveForegroundColorName{ L"TitleBarCaptionButtonInactiveForegroundColor"sv };
};
