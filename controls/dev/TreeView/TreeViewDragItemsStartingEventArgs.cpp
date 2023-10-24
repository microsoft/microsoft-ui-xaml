// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "TreeViewDragItemsStartingEventArgs.h"

TreeViewDragItemsStartingEventArgs::TreeViewDragItemsStartingEventArgs(const winrt::DragItemsStartingEventArgs& args)
{
    m_dragItemsStartingEventArgs = args;
}

bool TreeViewDragItemsStartingEventArgs::Cancel() const
{
    return m_dragItemsStartingEventArgs.Cancel();
}

void TreeViewDragItemsStartingEventArgs::Cancel(const bool value)
{
    m_dragItemsStartingEventArgs.Cancel(value);
}

DataPackage TreeViewDragItemsStartingEventArgs::Data() const
{
    return m_dragItemsStartingEventArgs.Data();
}

winrt::IVector<winrt::IInspectable> TreeViewDragItemsStartingEventArgs::Items()
{
    return m_dragItemsStartingEventArgs.Items();
}
