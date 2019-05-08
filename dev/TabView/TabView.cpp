// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TabView.h"
#include "TabViewAutomationPeer.h"
#include "DoubleUtil.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "SharedHelpers.h"

static constexpr double c_tabMinimumWidth = 48.0;
static constexpr double c_tabMaximumWidth = 200.0;

static constexpr wstring_view c_tabViewItemMinWidthName{ L"TabViewItemMinWidth"sv };
static constexpr wstring_view c_tabViewItemMaxWidthName{ L"TabViewItemMaxWidth"sv };

// TODO: what is the right number and should this be customizable?
static constexpr double c_scrollAmount = 50.0;

TabView::TabView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TabView);

    SetDefaultStyleKey(this);

    Loaded({ this, &TabView::OnLoaded });
    SelectionChanged({ this, &TabView::OnSelectionChanged });
    SizeChanged({ this, &TabView::OnSizeChanged });
}

void TabView::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    m_tabContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(L"TabContentPresenter", controlProtected));
    m_scrollViewer.set(GetTemplateChildT<winrt::FxScrollViewer>(L"ScrollViewer", controlProtected));
    if (auto scrollViewer = m_scrollViewer.get())
    {
        m_scrollViewerLoadedRevoker = scrollViewer.Loaded(winrt::auto_revoke, { this, &TabView::OnScrollViewerLoaded });
    }
}

void TabView::OnTabWidthModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateTabWidths();
}

winrt::AutomationPeer TabView::OnCreateAutomationPeer()
{
    return winrt::make<TabViewAutomationPeer>(*this);
}

void TabView::OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    UpdateTabContent();
}

void TabView::OnScrollViewerLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (auto scrollViewer = m_scrollViewer.get())
    {
        m_scrollDecreaseButton.set(SharedHelpers::FindInVisualTreeByName(scrollViewer, L"ScrollDecreaseButton").as<winrt::RepeatButton>());
        if (auto decreaseButton = m_scrollDecreaseButton.get())
        {
            m_scrollDecreaseClickRevoker = decreaseButton.Click(winrt::auto_revoke, { this, &TabView::OnScrollDecreaseClick });
        }

        m_scrollIncreaseButton.set(SharedHelpers::FindInVisualTreeByName(scrollViewer, L"ScrollIncreaseButton").as<winrt::RepeatButton>());
        if (auto increaseButton = m_scrollIncreaseButton.get())
        {
            m_scrollIncreaseClickRevoker = increaseButton.Click(winrt::auto_revoke, { this, &TabView::OnScrollIncreaseClick });
        }
    }

    UpdateTabWidths();
}

void TabView::OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args)
{
    UpdateTabWidths();
}

void TabView::OnItemsChanged(winrt::IInspectable const& item)
{
    if (auto args = item.as< winrt::IVectorChangedEventArgs>())
    {
        int numItems = (int)Items().Size();
        if (args.CollectionChange() == winrt::CollectionChange::ItemRemoved && numItems > 0)
        {
            if (SelectedIndex() == (int32_t)args.Index())
            {
                // Find the closest tab to select instead.
                int startIndex = (int)args.Index();
                if (startIndex >= numItems)
                {
                    startIndex = numItems - 1;
                }
                int index = startIndex;

                do
                {
                    auto nextItem = ContainerFromIndex(index).as<winrt::ListViewItem>();

                    if (nextItem && nextItem.IsEnabled() && nextItem.Visibility() == winrt::Visibility::Visible)
                    {
                        // We need to wait until OnSelectionChanged fires to change the selection, otherwise it will get lost.
                        m_indexToSelectOnSelectionChanged = std::optional(index);
                        break;
                    }

                    // try the next item
                    index++;
                    if (index >= numItems)
                    {
                        index = 0;
                    }
                } while (index != startIndex);
            }
        }
    }

    UpdateTabWidths();

    __super::OnItemsChanged(item);
}

void TabView::OnSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args)
{
    if (m_indexToSelectOnSelectionChanged)
    {
        SelectedItem(Items().GetAt(m_indexToSelectOnSelectionChanged.value()));
        m_indexToSelectOnSelectionChanged = {};
    }

    UpdateTabContent();
}

void TabView::UpdateTabContent()
{
    if (auto tabContentPresenter = m_tabContentPresenter.get())
    {
        if (!SelectedItem())
        {
            tabContentPresenter.Content(nullptr);
            tabContentPresenter.ContentTemplate(nullptr);
        }
        else
        {
            auto container = ContainerFromItem(SelectedItem()).as<winrt::ListViewItem>();
            if (container)
            {
                tabContentPresenter.Content(container.Content());
                tabContentPresenter.ContentTemplate(container.ContentTemplate());
            }
        }
    }
}

void TabView::CloseTab(winrt::TabViewItem const& container)
{
    if (auto item = ItemFromContainer(container))
    {
        uint32_t index = 0;
        if (Items().IndexOf(item, index))
        {
            auto args = winrt::make_self<TabViewTabClosingEventArgs>(item);

            m_tabClosingEventSource(*this, *args);

            if (!args->Cancel())
            {
                Items().RemoveAt(index);
            }
        }
    }
}

void TabView::OnScrollDecreaseClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (auto scrollViewer = m_scrollViewer.get())
    {
        scrollViewer.ChangeView(std::max(0.0, scrollViewer.HorizontalOffset() - c_scrollAmount), nullptr, nullptr);
    }
}

void TabView::OnScrollIncreaseClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (auto scrollViewer = m_scrollViewer.get())
    {
        scrollViewer.ChangeView(std::min(scrollViewer.ScrollableWidth(), scrollViewer.HorizontalOffset() + c_scrollAmount), nullptr, nullptr);
    }
}

void TabView::UpdateTabWidths()
{
    double tabWidth = DoubleUtil::NaN;

    if (TabWidthMode() == winrt::TabViewWidthMode::Fixed)
    {
        // Tabs should all be the same size, proportional to the amount of space.
        double minTabWidth = unbox_value<double>(SharedHelpers::FindResource(c_tabViewItemMinWidthName, winrt::Application::Current().Resources(), box_value(c_tabMinimumWidth)));
        double maxTabWidth = unbox_value<double>(SharedHelpers::FindResource(c_tabViewItemMaxWidthName, winrt::Application::Current().Resources(), box_value(c_tabMaximumWidth)));

        if (auto scrollViewer = m_scrollViewer.get())
        {
            // Calculate the proportional width of each tab given the width of the ScrollViewer.
            double padding = Padding().Left + Padding().Right;
            double tabWidthForScroller = (scrollViewer.ActualWidth() - padding) / (double)(Items().Size());
            tabWidth = std::min(std::max(tabWidthForScroller, minTabWidth), maxTabWidth);

            // If the min tab width causes the ScrollViewer to scroll, show the increase/decrease buttons.
            auto decreaseButton = m_scrollDecreaseButton.get();
            auto increaseButton = m_scrollIncreaseButton.get();
            if (decreaseButton && increaseButton)
            {
                if (tabWidthForScroller < tabWidth)
                {
                    decreaseButton.Visibility(winrt::Visibility::Visible);
                    increaseButton.Visibility(winrt::Visibility::Visible);
                }
                else
                {
                    decreaseButton.Visibility(winrt::Visibility::Collapsed);
                    increaseButton.Visibility(winrt::Visibility::Collapsed);
                }
            }
        }
    }

    for (auto item : Items())
    {
        // Set the calculated width on each tab.
        if (auto container = ContainerFromItem(item).as<winrt::ListViewItem>())
        {
            container.Width(tabWidth);
        }
    }
}
