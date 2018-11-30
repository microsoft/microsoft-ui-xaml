// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TreeViewCollapsedEventArgs.h"

TreeViewCollapsedEventArgs::TreeViewCollapsedEventArgs()
{
}

winrt::TreeViewNode TreeViewCollapsedEventArgs::Node()
{
    return m_node.get();
}

void TreeViewCollapsedEventArgs::Node(const winrt::TreeViewNode& value)
{
    m_node.set(value);
}

winrt::IInspectable TreeViewCollapsedEventArgs::Item()
{
    return m_node.get().Content();
}