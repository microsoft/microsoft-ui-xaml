// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "TabViewListView.h"
#include "TabViewItem.h"
#include "TabView.h"
#include "SharedHelpers.h"

#include "TabViewListView.properties.cpp"

TabViewListView::TabViewListView()
{
    SetDefaultStyleKey(this);

    ContainerContentChanging({ this, &TabViewListView::OnContainerContentChanging });
}

// IItemsControlOverrides

winrt::DependencyObject TabViewListView::GetContainerForItemOverride()
{
    return winrt::make<TabViewItem>();
}

bool TabViewListView::IsItemItsOwnContainerOverride(winrt::IInspectable const& args)
{
    bool isItemItsOwnContainer = false;
    if (const auto item = args.try_as<winrt::TabViewItem>())
    {
        isItemItsOwnContainer = static_cast<bool>(item);
    }
    return isItemItsOwnContainer;
}

void TabViewListView::OnItemsChanged(winrt::IInspectable const& item)
{
    __super::OnItemsChanged(item);

    if (const auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        const auto internalTabView = winrt::get_self<TabView>(tabView);
        internalTabView->OnItemsChanged(item);
    }
}

void TabViewListView::PrepareContainerForItemOverride(const winrt::DependencyObject& element, const winrt::IInspectable& item)
{
    const auto itemContainer = element.as<winrt::TabViewItem>();
    const auto tvi = winrt::get_self<TabViewItem>(itemContainer);
    bool k = itemContainer.Template() == nullptr ? false : true;
    bool f = tvi->Template() == nullptr ? false : true;
    

    // Due to virtualization, a TabViewItem might be recycled to display a different tab data item.
    // In that case, there is no need to set the TabWidthMode of the TabViewItem or its parent TabView
    // as they are already set correctly here.
    //
    // We know we are currently looking at a TabViewItem being recycled if its parent TabView has already been set.
    if (!tvi->GetParentTabView())
    {
        if (const auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
        {
           /* const auto internalTabView = winrt::get_self<TabView>(tabView);
            const auto selectedIndex = tabView.SelectedIndex();
            internalTabView->UpdateBottomStrokes(selectedIndex, selectedIndex); */
            tvi->OnTabViewWidthModeChanged(tabView.TabWidthMode());
            tvi->SetParentTabView(tabView);
        }
    }

    __super::PrepareContainerForItemOverride(element, item);
}

void TabViewListView::OnContainerContentChanging(const winrt::IInspectable& sender, const winrt::Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs& args)
{
    if (const auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        const auto internalTabView = winrt::get_self<TabView>(tabView);
        internalTabView->UpdateTabContent();
    }
}
