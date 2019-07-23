// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TreeViewDragItemsCompletedEventArgs.h"
#include "Vector.h"
#include "common.h"

void TreeViewDragItemsCompletedEventArgs::DragItemsCompletedEventArgs(const winrt::DragItemsCompletedEventArgs& args)
{
    m_dragItemsCompletedEventArgs = args;
}

DataPackageOperation TreeViewDragItemsCompletedEventArgs::DropResult() const
{
    return m_dragItemsCompletedEventArgs.DropResult();
}

winrt::IVectorView<winrt::IInspectable> TreeViewDragItemsCompletedEventArgs::Items()
{
    return m_dragItemsCompletedEventArgs.Items();
}