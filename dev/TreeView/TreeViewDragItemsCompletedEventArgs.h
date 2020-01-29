// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "TreeViewDragItemsCompletedEventArgs.g.h"

using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;

class TreeViewDragItemsCompletedEventArgs :
    public ReferenceTracker<TreeViewDragItemsCompletedEventArgs, winrt::implementation::TreeViewDragItemsCompletedEventArgsT, winrt::composing, winrt::composable>
{
public:
    TreeViewDragItemsCompletedEventArgs(const winrt::DragItemsCompletedEventArgs& args, const winrt::IInspectable& newParent);
    DataPackageOperation DropResult() const;
    winrt::IVectorView<winrt::IInspectable> Items();
    winrt::IInspectable NewParentItem();

private:
    winrt::DragItemsCompletedEventArgs m_dragItemsCompletedEventArgs{ nullptr };
    winrt::IInspectable m_newParentItem;
};
