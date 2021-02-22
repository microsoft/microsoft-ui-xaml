// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "BreadcrumbDropDownElementFactory.h"

#include "BreadcrumbItem.g.h"

class BreadcrumbItem :
    public ReferenceTracker<BreadcrumbItem, winrt::implementation::BreadcrumbItemT>
{
public:
    BreadcrumbItem();
    ~BreadcrumbItem();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    // IFrameworkElement
    void OnApplyTemplate();

    // internal
    void ResetVisualProperties();

    void SetPropertiesForLastNode();
    void SetPropertiesForEllipsisNode();
    void SetParentBreadcrumb(const winrt::Breadcrumb& parent);
    void SetDropDownItemDataTemplate(const winrt::IInspectable& newDataTemplate);
    void SetIndex(const uint32_t index);

    void RaiseItemClickedEvent(const winrt::IInspectable& content, const uint32_t index);
    void CloseFlyout();
    void OnClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

private:
    void OnLoadedEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnEllipsisItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnBreadcrumbItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs & args);
    void OnFlowDirectionChanged(winrt::DependencyObject const&, winrt::DependencyProperty const&);
    void OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnVisualPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnPointerEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);

    // Flyout events
    void OnFlyoutElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args);
    void OnFlyoutElementIndexChangedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs&);

    void HookListeners();
    void RevokeListeners();
    void RevokePartsListeners();
    void InstantiateFlyout();
    void OpenFlyout();

    void UpdateItemTypeVisualState(bool useTransitions);
    void UpdateCommonVisualState(bool useTransitions);

    void UpdateFlyoutIndex(const winrt::UIElement& element, const uint32_t index);
    winrt::IInspectable CloneEllipsisItemSource(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource);

    bool m_isEllipsisNode{};
    bool m_isLastNode{};

    // Contains the 1-indexed assigned to the element
    uint32_t m_index{};

    // BreadcrumbItem visual representation
    tracker_ref<winrt::Button> m_breadcrumbItemButton{ this };

    // Flyout content for ellipsis item
    tracker_ref<winrt::FlyoutBase> m_ellipsisFlyout{ this };
    tracker_ref<winrt::ItemsRepeater> m_ellipsisItemsRepeater{ this };
    tracker_ref<winrt::DataTemplate> m_dropDownItemDataTemplate{ this };
    com_ptr<BreadcrumbDropDownElementFactory> m_ellipsisElementFactory{ nullptr };

    // Parent BreadcrumbItem to ask for hidden elements
    tracker_ref<winrt::Breadcrumb> m_parentBreadcrumb{ this };

    winrt::Button::Loaded_revoker m_breadcrumbItemButtonLoadedRevoker{};
    winrt::Button::Click_revoker m_breadcrumbItemButtonClickRevoker{};
    RoutedEventHandler_revoker m_keyDownRevoker{};

    winrt::ItemsRepeater::ElementPrepared_revoker m_ellipsisRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementIndexChanged_revoker m_ellipsisRepeaterElementIndexChangedRevoker{};

    PropertyChanged_revoker m_isPressedButtonRevoker{};
    PropertyChanged_revoker m_isPointerOverButtonRevoker{};
    PropertyChanged_revoker m_isEnabledButtonRevoker{};

    // Revokers for events that change visual states
    winrt::UIElement::PointerEntered_revoker m_breadcrumbItemButtonPointerEnteredRevoker{};
    winrt::UIElement::PointerExited_revoker m_breadcrumbItemButtonPointerExitedRevoker{};
    winrt::UIElement::PointerPressed_revoker m_breadcrumbItemButtonPointerPressedRevoker{};
    winrt::UIElement::PointerReleased_revoker m_breadcrumbItemButtonPointerReleasedRevoker{};
    winrt::UIElement::PointerCanceled_revoker m_breadcrumbItemButtonPointerCanceledRevoker{};
    winrt::UIElement::PointerCaptureLost_revoker m_breadcrumbItemButtonPointerCaptureLostRevoker{};

    // Common Visual States
    static constexpr std::wstring_view s_normalStateName{ L"Normal"sv };
    static constexpr std::wstring_view s_currentStateName{ L"Current"sv };
    static constexpr std::wstring_view s_pointerOverStateName{ L"PointerOver"sv };
    static constexpr std::wstring_view s_pressedStateName{ L"Pressed"sv };
    static constexpr std::wstring_view s_disabledStateName{ L"Disabled"sv };

    // Item Type Visual States
    static constexpr std::wstring_view s_ellipsisStateName{ L"Ellipsis"sv };
    static constexpr std::wstring_view s_ellipsisRTLStateName{ L"EllipsisRTL"sv };
    static constexpr std::wstring_view s_lastItemStateName{ L"LastItem"sv };
    static constexpr std::wstring_view s_defaultStateName{ L"Default"sv };
    static constexpr std::wstring_view s_defaultRTLStateName{ L"DefaultRTL"sv };

    // Template Parts
    static constexpr std::wstring_view s_ellipsisItemsRepeaterPartName{ L"PART_EllipsisItemsRepeater"sv };
    static constexpr std::wstring_view s_breadcrumbItemButtonPartName{ L"PART_BreadcrumbItemButton"sv };

    // Automation Names
    static constexpr std::wstring_view s_ellipsisFlyoutAutomationName{ L"EllipsisFlyout"sv };
    static constexpr std::wstring_view s_ellipsisItemAutomationName{ L"EllipsisItem"sv };
    static constexpr std::wstring_view s_ellipsisItemsRepeaterAutomationName{ L"EllipsisItemsRepeater"sv };
};
