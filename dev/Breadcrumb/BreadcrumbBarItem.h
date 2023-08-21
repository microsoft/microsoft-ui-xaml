﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "BreadcrumbBarElementFactory.h"

#include "BreadcrumbBarItem.g.h"

class BreadcrumbBarItem :
    public ReferenceTracker<BreadcrumbBarItem, winrt::implementation::BreadcrumbBarItemT>
{
public:
    BreadcrumbBarItem();
    ~BreadcrumbBarItem();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    // IFrameworkElement
    void OnApplyTemplate();

    // internal

    // Only used for bug workaround in BreadcrumbElementFactory::RecycleElementCore.
    bool IsEllipsisDropDownItem() const
    {
        return m_isEllipsisDropDownItem;
    }

    void ResetVisualProperties();

    void SetPropertiesForLastItem();
    void SetPropertiesForEllipsisItem();
    void SetParentBreadcrumb(const winrt::BreadcrumbBar& parent);
    void SetEllipsisItem(const winrt::BreadcrumbBarItem& ellipsisItem);
    void SetEllipsisDropDownItemDataTemplate(const winrt::IInspectable& newDataTemplate);
    void SetIndex(const uint32_t index);
    void SetIsEllipsisDropDownItem(bool isEllipsisDropDownItem);

    void RaiseItemClickedEvent(const winrt::IInspectable& content, const uint32_t index);
    void CloseFlyout();
    void OnClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    void OnPointerEntered(winrt::PointerRoutedEventArgs const& args);
    void OnPointerMoved(winrt::PointerRoutedEventArgs const& args);
    void OnPointerExited(winrt::PointerRoutedEventArgs const& args);
    void OnPointerPressed(winrt::PointerRoutedEventArgs const& args);
    void OnPointerReleased(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCanceled(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args);

private:
    void OnLoadedEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnEllipsisItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnBreadcrumbBarItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs & args);
    void OnFlowDirectionChanged(winrt::DependencyObject const&, winrt::DependencyProperty const&);
    void OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnIsEnabledChanged(const winrt::IInspectable& sender, const winrt::DependencyPropertyChangedEventArgs& args);
    void OnVisualPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);

    // Flyout events
    void OnFlyoutElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args);
    void OnFlyoutElementIndexChangedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs&);

    void HookListeners(bool forEllipsisDropDownItem);
    void RevokeListeners();
    void RevokePartsListeners();
    void InstantiateFlyout();
    void OpenFlyout();
    void ResetTrackedPointerId();
    bool IgnorePointerId(const winrt::PointerRoutedEventArgs& args);
    void ProcessPointerOver(const winrt::PointerRoutedEventArgs& args);
    void ProcessPointerCanceled(const winrt::PointerRoutedEventArgs& args);

    void UpdateItemTypeVisualState();
    void UpdateEllipsisDropDownItemCommonVisualState(bool useTransitions);
    void UpdateInlineItemTypeVisualState(bool useTransitions);
    void UpdateButtonCommonVisualState(bool useTransitions);
    void UpdateFlyoutIndex(const winrt::UIElement& element, const uint32_t index);
    winrt::IInspectable CloneEllipsisItemSource(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource);

    // Common item fields

    // Contains the 1-indexed assigned to the element
    uint32_t m_index{};
    bool m_isEllipsisDropDownItem{};

    // Inline item fields

    bool m_isEllipsisItem{};
    bool m_isLastItem{};

    // BreadcrumbBarItem visual representation
    tracker_ref<winrt::Button> m_button{ this };
    // Parent BreadcrumbBarItem to ask for hidden elements
    tracker_ref<winrt::BreadcrumbBar> m_parentBreadcrumb{ this };

    // Flyout content for ellipsis item
    tracker_ref<winrt::Flyout> m_ellipsisFlyout{ this };
    tracker_ref<winrt::ItemsRepeater> m_ellipsisItemsRepeater{ this };
    tracker_ref<winrt::DataTemplate> m_ellipsisDropDownItemDataTemplate{ this };
    com_ptr<BreadcrumbElementFactory> m_ellipsisElementFactory{ nullptr };

    // Ellipsis dropdown item fields

    // BreadcrumbBarItem that owns the flyout
    tracker_ref<winrt::BreadcrumbBarItem> m_ellipsisItem{ this };

    // Visual State tracking
    uint32_t m_trackedPointerId{ 0 };
    bool m_isPressed{ false };
    bool m_isPointerOver{ false };

    // Common item token & revoker

    winrt::event_token m_childPreviewKeyDownToken{};
    RoutedEventHandler_revoker m_keyDownRevoker{};

    // Inline item token & revokers
    winrt::event_token m_flowDirectionChangedToken{};
    winrt::Button::Loaded_revoker m_buttonLoadedRevoker{};
    winrt::Button::Click_revoker m_buttonClickRevoker{};

    winrt::ItemsRepeater::ElementPrepared_revoker m_ellipsisRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementIndexChanged_revoker m_ellipsisRepeaterElementIndexChangedRevoker{};

    PropertyChanged_revoker m_isPressedButtonRevoker{};
    PropertyChanged_revoker m_isPointerOverButtonRevoker{};
    PropertyChanged_revoker m_isEnabledButtonRevoker{};

    // Ellipsis dropdown item revoker

    IsEnabledChanged_revoker m_isEnabledChangedRevoker{};

    // Common Visual States
    static constexpr std::wstring_view s_normalStateName{ L"Normal"sv };
    static constexpr std::wstring_view s_currentStateName{ L"Current"sv };
    static constexpr std::wstring_view s_pointerOverStateName{ L"PointerOver"sv };
    static constexpr std::wstring_view s_pressedStateName{ L"Pressed"sv };
    static constexpr std::wstring_view s_disabledStateName{ L"Disabled"sv };

    // Inline Item Type Visual States
    static constexpr std::wstring_view s_ellipsisStateName{ L"Ellipsis"sv };
    static constexpr std::wstring_view s_ellipsisRTLStateName{ L"EllipsisRTL"sv };
    static constexpr std::wstring_view s_lastItemStateName{ L"LastItem"sv };
    static constexpr std::wstring_view s_defaultStateName{ L"Default"sv };
    static constexpr std::wstring_view s_defaultRTLStateName{ L"DefaultRTL"sv };

    // Item Type Visual States
    static constexpr std::wstring_view s_inlineStateName{ L"Inline"sv };
    static constexpr std::wstring_view s_ellipsisDropDownStateName{ L"EllipsisDropDown"sv };

    // Template Parts
    static constexpr std::wstring_view s_ellipsisItemsRepeaterPartName{ L"PART_EllipsisItemsRepeater"sv };
    static constexpr std::wstring_view s_itemButtonPartName{ L"PART_ItemButton"sv };
    static constexpr std::wstring_view s_itemEllipsisFlyoutPartName{ L"PART_EllipsisFlyout"sv };

    // Automation Names
    static constexpr std::wstring_view s_ellipsisFlyoutAutomationName{ L"EllipsisFlyout"sv };
    static constexpr std::wstring_view s_ellipsisItemsRepeaterAutomationName{ L"EllipsisItemsRepeater"sv };
};
