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
    void HandleTitleChange(const winrt::hstring& oldTitle, const winrt::hstring& newTitle);
    void ResetTitle(winrt::hstring const& lastAppliedTitle);
    void UpdatePadding();
    void UpdateIcon();
    void UpdateBackButton();
    void UpdatePaneToggleButton();
    void UpdateHeight();
    void UpdateTitle();
    void UpdateSubtitle();
    void UpdateLeftHeader();
    void UpdateContent();
    void UpdateRightHeader();
    void UpdateDragRegion();
    void UpdateIconRegion();
    void UpdateInteractableElementsList();
    void UpdateLeftHeaderSpacing();

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
    winrt::Microsoft::UI::Windowing::AppWindow TryGetAppWindow();

    winrt::event_token m_inputActivationChangedToken{};
    winrt::event_token m_windowRectChangedToken{};
    winrt::hstring m_defaultAppWindowTitle{};
    winrt::Button::Click_revoker m_backButtonClickRevoker{};
    winrt::Button::Click_revoker m_paneToggleButtonClickRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_sizeChangedRevoker;
    winrt::FrameworkElement::LayoutUpdated_revoker m_iconLayoutUpdatedRevoker{};
    // Add a cached AppWindow field to avoid repeated GetFromWindowId calls
    winrt::Microsoft::UI::Windowing::AppWindow m_appWindow{ nullptr };
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
    tracker_ref<winrt::Grid> m_contentAreaGrid{ this };
    tracker_ref<winrt::FrameworkElement> m_leftHeaderArea{ this };
    tracker_ref<winrt::FrameworkElement> m_contentArea{ this };
    tracker_ref<winrt::FrameworkElement> m_rightHeaderArea{ this };

    double m_compactModeThresholdWidth{ 0.0 };
    bool m_isCompact{ false };
    bool m_hasDefaultAppWindowTitle{ false };

    static constexpr std::wstring_view s_leftPaddingColumnName{ L"LeftPaddingColumn"sv };
    static constexpr std::wstring_view s_rightPaddingColumnName{ L"RightPaddingColumn"sv };
    static constexpr std::wstring_view s_layoutRootPartName{ L"PART_LayoutRoot"sv };
    static constexpr std::wstring_view s_backButtonPartName{ L"PART_BackButton"sv };
    static constexpr std::wstring_view s_paneToggleButtonPartName{ L"PART_PaneToggleButton"sv };
    static constexpr std::wstring_view s_iconViewboxPartName{ L"PART_Icon"sv };
    static constexpr std::wstring_view s_leftHeaderPresenterPartName{ L"PART_LeftHeaderPresenter"sv };
    static constexpr std::wstring_view s_contentPresenterGridPartName{ L"PART_ContentPresenterGrid"sv };
    static constexpr std::wstring_view s_contentPresenterPartName{ L"PART_ContentPresenter"sv };
    static constexpr std::wstring_view s_rightHeaderPresenterPartName{ L"PART_RightHeaderPresenter"sv };

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
    static constexpr std::wstring_view s_leftHeaderVisibleVisualStateName{ L"LeftHeaderVisible"sv };
    static constexpr std::wstring_view s_leftHeaderCollapsedVisualStateName{ L"LeftHeaderCollapsed"sv };
    static constexpr std::wstring_view s_leftHeaderDeactivatedVisualStateName{ L"LeftHeaderDeactivated"sv };
    static constexpr std::wstring_view s_contentVisibleVisualStateName{ L"ContentVisible"sv };
    static constexpr std::wstring_view s_contentCollapsedVisualStateName{ L"ContentCollapsed"sv };
    static constexpr std::wstring_view s_contentDeactivatedVisualStateName{ L"ContentDeactivated"sv };
    static constexpr std::wstring_view s_rightHeaderVisibleVisualStateName{ L"RightHeaderVisible"sv };
    static constexpr std::wstring_view s_rightHeaderCollapsedVisualStateName{ L"RightHeaderCollapsed"sv };
    static constexpr std::wstring_view s_rightHeaderDeactivatedVisualStateName{ L"RightHeaderDeactivated"sv };
};
