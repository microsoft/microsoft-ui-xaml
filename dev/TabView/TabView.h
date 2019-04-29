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
    TabViewTabClosingEventArgs(winrt::IInspectable item) { m_item = item; }

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
    ~TabView() {}

    // IFrameworkElement
    void OnApplyTemplate();

    // IUIElement
    virtual winrt::AutomationPeer OnCreateAutomationPeer();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void CloseTab(winrt::TabViewItem item);

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

    bool m_isTabClosing{ false };
    int m_indexToSelect{ 0 };

    tracker_ref<winrt::ContentPresenter> m_tabContentPresenter{ this };
    tracker_ref<winrt::FxScrollViewer> m_scrollViewer{ this };
    tracker_ref<winrt::RepeatButton> m_scrollDecreaseButton{ this };
    tracker_ref<winrt::RepeatButton> m_scrollIncreaseButton{ this };

    winrt::ScrollViewer::Loaded_revoker m_scrollViewerLoadedRevoker{};

    winrt::RepeatButton::Click_revoker m_scrollDecreaseClickRevoker{};
    winrt::RepeatButton::Click_revoker m_scrollIncreaseClickRevoker{};
};
