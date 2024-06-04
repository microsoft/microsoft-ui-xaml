// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "TreeViewDragItemsCompletedEventArgs.h"

TreeViewDragItemsCompletedEventArgs::TreeViewDragItemsCompletedEventArgs(const winrt::DragItemsCompletedEventArgs& args, const winrt::IInspectable& newParentItem)
{
    m_dragItemsCompletedEventArgs = args;
    m_newParentItem = newParentItem;
}


DataPackageOperation TreeViewDragItemsCompletedEventArgs::DropResult() const
{
    return m_dragItemsCompletedEventArgs.DropResult();
}

winrt::IVectorView<winrt::IInspectable> TreeViewDragItemsCompletedEventArgs::Items()
{
    return m_dragItemsCompletedEventArgs.Items();
}

winrt::IInspectable TreeViewDragItemsCompletedEventArgs::NewParentItem()
{
    return m_newParentItem;
}
