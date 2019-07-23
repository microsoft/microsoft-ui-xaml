// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "TreeViewDragItemsStartingEventArgs.g.h"

using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;

class TreeViewDragItemsStartingEventArgs :
    public ReferenceTracker<TreeViewDragItemsStartingEventArgs, winrt::implementation::TreeViewDragItemsStartingEventArgsT, winrt::composing, winrt::composable>
{
public:
    void DragItemsStartingEventArgs(const winrt::DragItemsStartingEventArgs& args);
    [[nodiscard]] bool Cancel() const;
    void Cancel(bool value);
    [[nodiscard]] DataPackage Data() const;
    winrt::IVector<winrt::IInspectable> Items();

private:
    winrt::DragItemsStartingEventArgs m_dragItemsStartingEventArgs;
};
