// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TabViewItem.g.h"
#include "TabViewItem.properties.h"

class TabViewItem :
    public ReferenceTracker<TabViewItem, winrt::implementation::TabViewItemT>,
    public TabViewItemProperties
{

public:
    TabViewItem();
    ~TabViewItem() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

 private:
    tracker_ref<winrt::Button> m_closeButton{ this };

    void UpdateCloseButton();

    PropertyChanged_revoker m_IsSelectedChangedRevoker{};
    PropertyChanged_revoker m_CanCloseTabsChangedRevoker{};
    winrt::ButtonBase::Click_revoker m_closeButtonClickRevoker{};

    void OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnCloseButtonPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnCloseButtonClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
};
