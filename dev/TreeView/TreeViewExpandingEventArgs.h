// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "TreeViewNode.h"

#include "TreeViewExpandingEventArgs.g.h"

class TreeViewExpandingEventArgs :
    public ReferenceTracker<
        TreeViewExpandingEventArgs,
        winrt::implementation::TreeViewExpandingEventArgsT,
        winrt::composing,
        winrt::composable>
{
public:
    TreeViewExpandingEventArgs();
    winrt::TreeViewNode Node();
    void Node(const winrt::TreeViewNode& value);
    winrt::IInspectable Item();

private:
    tracker_ref<winrt::TreeViewNode> m_node{ this };
};
