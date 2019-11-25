// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "TreeViewDragItemsStartingEventArgs.g.h"

using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;

class TreeViewDragItemsStartingEventArgs :
    public ReferenceTracker<TreeViewDragItemsStartingEventArgs, winrt::implementation::TreeViewDragItemsStartingEventArgsT, winrt::composing, winrt::composable>
{
public:
    TreeViewDragItemsStartingEventArgs(const winrt::DragItemsStartingEventArgs& args);
    bool Cancel() const;
    void Cancel(const bool value);
    DataPackage Data() const;
    winrt::IVector<winrt::IInspectable> Items();

private:
    winrt::DragItemsStartingEventArgs m_dragItemsStartingEventArgs;
};
