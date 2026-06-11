// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "TreeViewSelectionChangedEventArgs.g.h"

class TreeViewSelectionChangedEventArgs :
    public ReferenceTracker<TreeViewSelectionChangedEventArgs, winrt::implementation::TreeViewSelectionChangedEventArgsT, winrt::composing, winrt::composable>
{
public:
    TreeViewSelectionChangedEventArgs(const winrt::IVector<winrt::IInspectable> addedItems, const winrt::IVector<winrt::IInspectable> removedItems);
    winrt::IVector<winrt::IInspectable> AddedItems();
    winrt::IVector<winrt::IInspectable> RemovedItems();

private:
    tracker_ref<winrt::IVector<winrt::IInspectable>> m_addedItems{ this };
    tracker_ref<winrt::IVector<winrt::IInspectable>> m_removedItems{ this };
};
