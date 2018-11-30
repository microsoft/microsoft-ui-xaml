// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "RadioButtons.g.h"
#include "RadioButtons.properties.h"

class RadioButtons :
    public ReferenceTracker<RadioButtons, winrt::implementation::RadioButtonsT>,
    public RadioButtonsProperties
{

public:
    RadioButtons();
    ~RadioButtons() {}

    // IFrameworkElement
    void OnApplyTemplate();

    winrt::DependencyObject ContainerFromItem(winrt::IInspectable const& item);
    winrt::DependencyObject ContainerFromIndex(int index);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void OnListViewLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnListViewSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args);
    void OnListViewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnListViewKeyUp(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);

    void UpdateItemsSource();
    void UpdateMaximumColumns();

    void UpdateSelectedItem();
    void UpdateSelectedIndex();

    bool MoveSelection(int direction);

    bool m_isControlDown{ false };

    tracker_ref<winrt::ListView> m_listView{ this };
    tracker_ref<winrt::ItemsWrapGrid> m_itemsWrapGrid{ this };

    winrt::Control::Loaded_revoker m_listViewLoadedRevoker{};
    winrt::Selector::SelectionChanged_revoker m_listViewSelectionChangedRevoker{};
    winrt::Control::KeyDown_revoker m_listViewKeyDownRevoker{};
    winrt::Control::KeyUp_revoker m_listViewKeyUpRevoker{};
};
