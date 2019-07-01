// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "TabViewListView.g.h"

class TabViewListView :
    public ReferenceTracker<TabViewListView, winrt::implementation::TabViewListViewT>
{
public:
    TabViewListView();

    // IItemsControlOverrides
    winrt::DependencyObject GetContainerForItemOverride();
    bool IsItemItsOwnContainerOverride(winrt::IInspectable const& item);
    void OnItemsChanged(winrt::IInspectable const& item);

private:
};

