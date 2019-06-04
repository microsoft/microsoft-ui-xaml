// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TabView.g.h"
#include "TabView.properties.h"
#include "TabViewTabClosingEventArgs.g.h"

class TabViewTabClosingEventArgs :
    public winrt::implementation::TabViewTabClosingEventArgsT<TabViewTabClosingEventArgs>
{
public:
    TabViewTabClosingEventArgs(winrt::IInspectable const& item) { m_item = item; }

    bool Cancel() { return m_cancel; }
    void Cancel(bool value) { m_cancel = value; }

    winrt::IInspectable Item() { return m_item; }

private:
    bool m_cancel{};
    winrt::IInspectable m_item{};
};

class TabView :
    public ReferenceTracker<TabView, winrt::implementation::TabViewT>,
    public TabViewProperties
{

public:
    TabView();

    // IFrameworkElement
    void OnApplyTemplate();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    void OnTabWidthModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void CloseTab(winrt::TabViewItem const& item);

    //IItemsControlOverrides
    void OnItemsChanged(winrt::IInspectable const& item);

private:
    void OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnScrollViewerLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args);
    void OnScrollDecreaseClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnScrollIncreaseClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);

    void UpdateTabContent();
    void UpdateTabWidths();

    std::optional<int> m_indexToSelectOnSelectionChanged;

    tracker_ref<winrt::ContentPresenter> m_tabContentPresenter{ this };
    tracker_ref<winrt::FxScrollViewer> m_scrollViewer{ this };
    tracker_ref<winrt::RepeatButton> m_scrollDecreaseButton{ this };
    tracker_ref<winrt::RepeatButton> m_scrollIncreaseButton{ this };

    winrt::ScrollViewer::Loaded_revoker m_scrollViewerLoadedRevoker{};

    winrt::RepeatButton::Click_revoker m_scrollDecreaseClickRevoker{};
    winrt::RepeatButton::Click_revoker m_scrollIncreaseClickRevoker{};
};
