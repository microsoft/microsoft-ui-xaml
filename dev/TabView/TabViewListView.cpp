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

CppWinRTActivatableClassWithBasicFactory(TabViewListView);

// IItemsControlOverrides

winrt::DependencyObject TabViewListView::GetContainerForItemOverride()
{
    return winrt::make<TabViewItem>();
}

bool TabViewListView::IsItemItsOwnContainerOverride(winrt::IInspectable const& args)
{
    bool isItemItsOwnContainer = false;
    if (args)
    {
        auto item = args.try_as<winrt::TabViewItem>();
        isItemItsOwnContainer = static_cast<bool>(item);
    }
    return isItemItsOwnContainer;
}

void TabViewListView::OnItemsChanged(winrt::IInspectable const& item)
{
    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        auto internalTabView = winrt::get_self<TabView>(tabView);
        internalTabView->OnItemsChanged(item);
    }
}
