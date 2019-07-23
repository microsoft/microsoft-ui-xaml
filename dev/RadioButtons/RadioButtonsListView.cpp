// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RadioButtonsListView.h"
#include "RadioButtonsListViewItem.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "common.h"

CppWinRTActivatableClassWithBasicFactory(RadioButtonsListView);

// IItemsControlOverrides

winrt::DependencyObject RadioButtonsListView::GetContainerForItemOverride()
{
    return winrt::make<RadioButtonsListViewItem>();
}

bool RadioButtonsListView::IsItemItsOwnContainerOverride(winrt::IInspectable const& args)
{
    bool isItemItsOwnContainer = false;
    if (args)
    {
        auto item = args.try_as<winrt::RadioButtonsListViewItem>();
        isItemItsOwnContainer = static_cast<bool>(item);
    }
    return isItemItsOwnContainer;
}
