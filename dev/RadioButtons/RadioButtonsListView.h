// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RadioButtonsListView.g.h"

class RadioButtonsListView :
    public ReferenceTracker<RadioButtonsListView, winrt::implementation::RadioButtonsListViewT>
{
public:
    RadioButtonsListView() = default;;

    // IItemsControlOverrides
    winrt::DependencyObject GetContainerForItemOverride();
    bool IsItemItsOwnContainerOverride(winrt::IInspectable const& item);

private:
};

