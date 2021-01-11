// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "BreadcrumbItem.g.h"

class BreadcrumbItem :
    public ReferenceTracker<BreadcrumbItem, winrt::implementation::BreadcrumbItemT>
{
public:
    BreadcrumbItem();
    ~BreadcrumbItem();

    // IFrameworkElement
    void OnApplyTemplate();
    void RevokeListeners();

    // internal
    void ResetVisualProperties();
    void SetPropertiesForLastNode();
    void SetPropertiesForEllipsisNode();
    void SetItemsRepeater(const winrt::Breadcrumb& parent);
    void SetFlyoutDataTemplate(const winrt::IInspectable& newDataTemplate);

private:
    void OnLoadedEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnEllipsisItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnBreadcrumbItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs & args);
    void OnFlyoutElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args);
    void OnFlyoutElementKeyDownEvent(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs&);
    void OnFlyoutElementClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    void OpenFlyout();
    void CloseFlyout();

    void InstantiateFlyout();
    winrt::IInspectable CloneEllipsisItemSource(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource);

    bool m_isEllipsisNode{};
    bool m_isLastNode{};
    bool m_isChevronVisible{};
    winrt::GridLength m_chevronOriginalWidth;

    // BreadcrumbItem visual representation
    tracker_ref<winrt::Button> m_breadcrumbItemButton{ this };

    // Flyout content for ellipsis item
    tracker_ref<winrt::FlyoutBase> m_ellipsisFlyout{ this };
    tracker_ref<winrt::ItemsRepeater> m_ellipsisItemsRepeater{ this };
    tracker_ref<winrt::DataTemplate> m_ellipsisDataTemplate{ this };

    // Parent BreadcrumbItem to ask for hidden elements
    tracker_ref<winrt::Breadcrumb> m_parentBreadcrumb{ this };

    winrt::Button::Loaded_revoker m_breadcrumbItemButtonLoadedRevoker{};
    winrt::Button::Click_revoker m_breadcrumbItemButtonClickRevoker{};
    winrt::ItemsRepeater::ElementPrepared_revoker m_flyoutRepeaterElementPreparedRevoker{};
    winrt::Button::Click_revoker m_clickRevoker{};

    winrt::UIElement::PointerPressed_revoker m_pointerPressedRevoker{};
};
