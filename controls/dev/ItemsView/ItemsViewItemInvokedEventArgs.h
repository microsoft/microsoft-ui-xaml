// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsViewItemInvokedEventArgs.g.h"

class ItemsViewItemInvokedEventArgs :
    public ReferenceTracker<ItemsViewItemInvokedEventArgs, winrt::implementation::ItemsViewItemInvokedEventArgsT, winrt::composable, winrt::composing>
{
public:
    ItemsViewItemInvokedEventArgs(const winrt::IInspectable& invokedItem);

    ~ItemsViewItemInvokedEventArgs()
    {
        ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

#pragma region IItemsViewItemInvokedEventArgs
    winrt::IInspectable InvokedItem();
#pragma endregion

private:
    tracker_ref<winrt::IInspectable> m_invokedItem{ this };
};
