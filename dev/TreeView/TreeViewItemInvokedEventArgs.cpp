// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TreeViewItemInvokedEventArgs.h"


TreeViewItemInvokedEventArgs::TreeViewItemInvokedEventArgs()
{
}

winrt::IInspectable TreeViewItemInvokedEventArgs::InvokedItem()
{
    return m_invokedItem.get();
}

bool TreeViewItemInvokedEventArgs::Handled()
{
    return m_Handled;
}

void TreeViewItemInvokedEventArgs::Handled(bool value)
{
    m_Handled = value;
}

void TreeViewItemInvokedEventArgs::InvokedItem(const winrt::IInspectable& invokedItem)
{
    m_invokedItem.set(invokedItem);
}
