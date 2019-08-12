// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemContainer.h"
#include "TabViewItem.g.h"
#include "TabViewItem.properties.h"
#include "TabViewItemAutomationPeer.h"

class TabViewItem :
    public ReferenceTracker<TabViewItem, winrt::implementation::TabViewItemT, ItemContainer>,
    public TabViewItemProperties
{
public:
    using TabViewItemProperties::EnsureProperties;
    using TabViewItemProperties::ClearProperties;

    TabViewItem();

    // IFrameworkElement
    void OnApplyTemplate();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    void OnIsCloseablePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnHeaderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

 private:
    tracker_ref<winrt::Button> m_closeButton{ this };
    tracker_ref<winrt::ToolTip> m_toolTip{ this };

    void UpdateCloseButton();

    bool m_firstTimeSettingToolTip{ true };

    PropertyChanged_revoker m_CanCloseTabsChangedRevoker{};
    winrt::ButtonBase::Click_revoker m_closeButtonClickRevoker{};

    void OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnCloseButtonPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnCloseButtonClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    winrt::UIElement::DragStarting_revoker m_dragStartingRevoker{};
    winrt::UIElement::DragOver_revoker m_dragOverRevoker{};
    winrt::UIElement::Drop_revoker m_dropRevoker{};

    void OnDragStarting(const winrt::UIElement& sender, const winrt::DragStartingEventArgs& args);
    void OnDragOver(const winrt::IInspectable& sender, const winrt::DragEventArgs& args);
    void OnDrop(const winrt::IInspectable& sender, const winrt::DragEventArgs& args);
   
};
