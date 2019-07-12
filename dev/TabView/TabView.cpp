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
#include <Vector.h>

static constexpr double c_tabMinimumWidth = 48.0;
static constexpr double c_tabMaximumWidth = 200.0;

static constexpr wstring_view c_tabViewItemMinWidthName{ L"TabViewItemMinWidth"sv };
static constexpr wstring_view c_tabViewItemMaxWidthName{ L"TabViewItemMaxWidth"sv };

// TODO: what is the right number and should this be customizable?
static constexpr double c_scrollAmount = 50.0;

TabView::TabView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TabView);

    auto items = winrt::make<Vector<winrt::IInspectable>>();
    SetValue(s_ItemsProperty, items);

    SetDefaultStyleKey(this);

    Loaded({ this, &TabView::OnLoaded });
    SizeChanged({ this, &TabView::OnSizeChanged });
    KeyUp({ this, &TabView::OnTabViewKeyUp });
}

void TabView::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    m_tabContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(L"TabContentPresenter", controlProtected));
    m_rightContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(L"RightContentPresenter", controlProtected));
    
    m_leftContentColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"LeftContentColumn", controlProtected));
    m_tabColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"TabColumn", controlProtected));
    m_addButtonColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"AddButtonColumn", controlProtected));
    m_rightContentColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"RightContentColumn", controlProtected));

    m_tabContainerGrid.set(GetTemplateChildT<winrt::Grid>(L"TabContainerGrid", controlProtected));

    m_listView.set([this, controlProtected]() {
        auto listView = GetTemplateChildT<winrt::ListView>(L"TabListView", controlProtected);
        if (listView)
        {
            m_listViewLoadedRevoker = listView.Loaded(winrt::auto_revoke, { this, &TabView::OnListViewLoaded });
            m_listViewSelectionChangedRevoker = listView.SelectionChanged(winrt::auto_revoke, { this, &TabView::OnListViewSelectionChanged });
        }
        return listView;
    }());

    m_addButton.set([this, controlProtected]() {
        auto addButton = GetTemplateChildT<winrt::Button>(L"AddButton", controlProtected);
        if (addButton)
        {
            // Do localization for the add button
            if (winrt::AutomationProperties::GetName(addButton).empty())
            {
                auto addButtonName = ResourceAccessor::GetLocalizedStringResource(SR_TabViewAddButtonName);
                winrt::AutomationProperties::SetName(addButton, addButtonName);
            }

            auto toolTip = winrt::ToolTipService::GetToolTip(addButton);
            if (!toolTip)
            {
                winrt::ToolTip tooltip = winrt::ToolTip();
                tooltip.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_TabViewAddButtonTooltip)));
                winrt::ToolTipService::SetToolTip(addButton, tooltip);
            }

            m_addButtonClickRevoker = addButton.Click(winrt::auto_revoke, { this, &TabView::OnAddButtonClick });
        }
        return addButton;
    }());

    UpdateItemsSource();
}

void TabView::UpdateItemsSource()
{
    if (auto listView = m_listView.get())
    {
        if (ItemsSource())
        {
            listView.ItemsSource(ItemsSource());
        }
        else
        {
            listView.ItemsSource(Items());
        }
    }
}

void TabView::OnItemsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateItemsSource();
}

void TabView::OnItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateItemsSource();
}

void TabView::OnSelectedIndexPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateSelectedIndex();
}

void TabView::OnSelectedItemPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateSelectedItem();
}

void TabView::OnTabWidthModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateTabWidths();
}

void TabView::OnAddButtonClick(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    m_addButtonClickEventSource(*this, args);
}

winrt::AutomationPeer TabView::OnCreateAutomationPeer()
{
    return winrt::make<TabViewAutomationPeer>(*this);
}

void TabView::OnLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    UpdateTabContent();
}

void TabView::OnListViewLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    if (ReadLocalValue(s_SelectedIndexProperty) != winrt::DependencyProperty::UnsetValue())
    {
        UpdateSelectedIndex();
    }
    if (ReadLocalValue(s_SelectedItemProperty) != winrt::DependencyProperty::UnsetValue())
    {
        UpdateSelectedItem();
    }

    if (auto listView = m_listView.get())
    {
        SelectedIndex(listView.SelectedIndex());
        SelectedItem(listView.SelectedItem());

        m_scrollViewer.set([this, listView]() {
            auto scrollViewer = SharedHelpers::FindInVisualTreeByName(listView, L"ScrollViewer").as<winrt::FxScrollViewer>();
            if (scrollViewer)
            {
                m_scrollViewerLoadedRevoker = scrollViewer.Loaded(winrt::auto_revoke, { this, &TabView::OnScrollViewerLoaded });
            }
            return scrollViewer;
        }());
    }
}

void TabView::OnScrollViewerLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    if (auto&& scrollViewer = m_scrollViewer.get())
    {
        m_scrollDecreaseButton.set([this, scrollViewer]() {
            auto decreaseButton = SharedHelpers::FindInVisualTreeByName(scrollViewer, L"ScrollDecreaseButton").as<winrt::RepeatButton>();
            m_scrollDecreaseClickRevoker = decreaseButton.Click(winrt::auto_revoke, { this, &TabView::OnScrollDecreaseClick });
            return decreaseButton;
        }());

        m_scrollIncreaseButton.set([this, scrollViewer]() {
            auto increaseButton = SharedHelpers::FindInVisualTreeByName(scrollViewer, L"ScrollIncreaseButton").as<winrt::RepeatButton>();
            m_scrollIncreaseClickRevoker = increaseButton.Click(winrt::auto_revoke, { this, &TabView::OnScrollIncreaseClick });
            return increaseButton;
        }());
    }

    UpdateTabWidths();
}

void TabView::OnSizeChanged(const winrt::IInspectable&, const winrt::SizeChangedEventArgs&)
{
    UpdateTabWidths();
}

void TabView::OnItemsChanged(winrt::IInspectable const& item)
{
    if (auto args = item.as< winrt::IVectorChangedEventArgs>())
    {
        int numItems = static_cast<int>(Items().Size());
        if (args.CollectionChange() == winrt::CollectionChange::ItemRemoved && numItems > 0)
        {
            if (SelectedIndex() == static_cast<int32_t>(args.Index()))
            {
                // Find the closest tab to select instead.
                int startIndex = static_cast<int>(args.Index());
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
                        SelectedItem(Items().GetAt(index));
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
}

void TabView::OnListViewSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args)
{
    if (auto listView = m_listView.get())
    {
        SelectedIndex(listView.SelectedIndex());
        SelectedItem(listView.SelectedItem());
    }

    UpdateTabContent();

    m_selectionChangedEventSource(sender, args);
}

void TabView::UpdateTabContent()
{
    if (auto tabContentPresenter = m_tabContentPresenter.get())
    {
        if (!SelectedItem())
        {
            tabContentPresenter.Content(nullptr);
            tabContentPresenter.ContentTemplate(nullptr);
            tabContentPresenter.ContentTemplateSelector(nullptr);
        }
        else
        {
            if (auto container = ContainerFromItem(SelectedItem()).as<winrt::ListViewItem>())
            {
                tabContentPresenter.Content(container.Content());
                tabContentPresenter.ContentTemplate(container.ContentTemplate());
                tabContentPresenter.ContentTemplateSelector(container.ContentTemplateSelector());
            }
        }
    }
}

void TabView::CloseTab(winrt::TabViewItem const& container)
{
    if (auto listView = m_listView.get())
    {
        if (auto item = listView.ItemFromContainer(container))
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
}

void TabView::OnScrollDecreaseClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto scrollViewer = m_scrollViewer.get())
    {
        scrollViewer.ChangeView(std::max(0.0, scrollViewer.HorizontalOffset() - c_scrollAmount), nullptr, nullptr);
    }
}

void TabView::OnScrollIncreaseClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto scrollViewer = m_scrollViewer.get())
    {
        scrollViewer.ChangeView(std::min(scrollViewer.ScrollableWidth(), scrollViewer.HorizontalOffset() + c_scrollAmount), nullptr, nullptr);
    }
}

void TabView::UpdateTabWidths()
{
    double tabWidth = std::numeric_limits<double>::quiet_NaN();

    if (auto tabGrid = m_tabContainerGrid.get())
    {
        // Add up width taken by custom content and + button
        double widthTaken = 0.0;
        if (auto leftContentColumn = m_leftContentColumn.get())
        {
            widthTaken += leftContentColumn.ActualWidth();
        }
        if (auto addButtonColumn = m_addButtonColumn.get())
        {
            widthTaken += addButtonColumn.ActualWidth();
        }
        if (auto rightContentColumn = m_rightContentColumn.get())
        {
            if (auto rightContentPresenter = m_rightContentPresenter.get())
            {
                winrt::Size rightContentSize = rightContentPresenter.DesiredSize();
                rightContentColumn.MinWidth(rightContentSize.Width);
                widthTaken += rightContentSize.Width;
            }
        }

        if (auto tabColumn = m_tabColumn.get())
        {
            auto availableWidth = ActualWidth() - widthTaken;

            if (TabWidthMode() == winrt::TabViewWidthMode::SizeToContent)
            {
                tabColumn.MaxWidth(availableWidth);
                tabColumn.Width(winrt::GridLengthHelper::FromValueAndType(1.0, winrt::GridUnitType::Auto));
            }
            else if (TabWidthMode() == winrt::TabViewWidthMode::Equal)
            {
                // Tabs should all be the same size, proportional to the amount of space.
                double minTabWidth = unbox_value<double>(SharedHelpers::FindResource(c_tabViewItemMinWidthName, winrt::Application::Current().Resources(), box_value(c_tabMinimumWidth)));
                double maxTabWidth = unbox_value<double>(SharedHelpers::FindResource(c_tabViewItemMaxWidthName, winrt::Application::Current().Resources(), box_value(c_tabMaximumWidth)));

                // Calculate the proportional width of each tab given the width of the ScrollViewer.
                auto padding = Padding();
                double tabWidthForScroller = (availableWidth - (padding.Left + padding.Right)) / (double)(Items().Size());

                tabWidth = std::clamp(tabWidthForScroller, minTabWidth, maxTabWidth);

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

                // Size tab column to needed size
                tabColumn.MaxWidth(availableWidth);
                auto requiredWidth = tabWidth * Items().Size();
                if (requiredWidth >= availableWidth)
                {
                    tabColumn.Width(winrt::GridLengthHelper::FromPixels(availableWidth));
                }
                else
                {
                    tabColumn.Width(winrt::GridLengthHelper::FromValueAndType(1.0, winrt::GridUnitType::Auto));
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


void TabView::UpdateSelectedItem()
{
    if (auto listView = m_listView.get())
    {
        // Setting ListView.SelectedItem will not work here in all cases.
        // The reason why that doesn't work but this does is unknown.
        auto container = listView.ContainerFromItem(SelectedItem());
        if (auto lvi = container.as<winrt::ListViewItem>())
        {
            lvi.IsSelected(true);
        }
    }
}

void TabView::UpdateSelectedIndex()
{
    if (auto listView = m_listView.get())
    {
        listView.SelectedIndex(SelectedIndex());
    }
}

winrt::DependencyObject TabView::ContainerFromItem(winrt::IInspectable const& item)
{
    if (auto listView = m_listView.get())
    {
        return listView.ContainerFromItem(item);
    }
    return nullptr;
}

winrt::DependencyObject TabView::ContainerFromIndex(int index)
{
    if (auto listView = m_listView.get())
    {
        return listView.ContainerFromIndex(index);
    }
    return nullptr;
}

void TabView::OnTabViewKeyUp(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args)
{
    winrt::CoreVirtualKeyStates ctrlState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Control);

    if (IsEnabled()
        && SelectedItem()
        && (ctrlState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down)
    {
        winrt::VirtualKey key = args.Key();

        if (key == winrt::VirtualKey::F4)
        {
            if (auto selectedTab = SelectedItem().as<winrt::TabViewItem>())
            {
                if (selectedTab.IsCloseable())
                {
                    // Close the tab on ctrl + F4
                    CloseTab(selectedTab);
                    args.Handled(true);
                }
            }
        }
    }
}
