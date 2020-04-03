// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "TreeViewSelectionChangedEventArgs.h"

TreeViewSelectionChangedEventArgs::TreeViewSelectionChangedEventArgs(const winrt::IVector<winrt::IInspectable> addedItems, const winrt::IVector<winrt::IInspectable> removedItems)
{
    m_addedItems.set(addedItems);
    m_removedItems.set(removedItems);
}

winrt::IVector<winrt::IInspectable> TreeViewSelectionChangedEventArgs::AddedItems()
{
    return m_addedItems.get();
}

winrt::IVector<winrt::IInspectable> TreeViewSelectionChangedEventArgs::RemovedItems()
{
    return m_removedItems.get();
}
