// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TreeViewItemInvokedEventArgs.g.h"

class TreeViewItemInvokedEventArgs :
    public ReferenceTracker<TreeViewItemInvokedEventArgs, winrt::implementation::TreeViewItemInvokedEventArgsT, winrt::composing, winrt::composable>
{
public:
    TreeViewItemInvokedEventArgs();
    winrt::IInspectable InvokedItem();
    bool Handled();
    void Handled(bool value); 
    void InvokedItem(const winrt::IInspectable& invokedItem);

private:
    bool m_Handled{false};
    tracker_ref<winrt::IInspectable> m_invokedItem{ this };
};
