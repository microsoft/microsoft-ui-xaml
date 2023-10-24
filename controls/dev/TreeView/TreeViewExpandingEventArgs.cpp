// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TreeViewExpandingEventArgs.h"

winrt::TreeViewNode TreeViewExpandingEventArgs::Node()
{
    return m_node.get();
}

void TreeViewExpandingEventArgs::Node(const winrt::TreeViewNode& value)
{
    m_node.set(value);
}

winrt::IInspectable TreeViewExpandingEventArgs::Item()
{
    return m_node.get().Content();
}
