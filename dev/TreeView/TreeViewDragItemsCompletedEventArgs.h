// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "TreeViewDragItemsCompletedEventArgs.g.h"

using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;

class TreeViewDragItemsCompletedEventArgs :
    public ReferenceTracker<TreeViewDragItemsCompletedEventArgs, winrt::implementation::TreeViewDragItemsCompletedEventArgsT, winrt::composing, winrt::composable>
{
public:
    void DragItemsCompletedEventArgs(const winrt::DragItemsCompletedEventArgs& args);
    DataPackageOperation DropResult() const;
    winrt::IVectorView<winrt::IInspectable> Items();
    void NewParent(const winrt::IInspectable& parent);
    winrt::IInspectable NewParent();

private:
    winrt::DragItemsCompletedEventArgs m_dragItemsCompletedEventArgs{ nullptr };
    winrt::IInspectable m_newParent;
};
