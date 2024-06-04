// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsViewTrace.h"
#include "ItemsViewItemInvokedEventArgs.h"

ItemsViewItemInvokedEventArgs::ItemsViewItemInvokedEventArgs(const winrt::IInspectable& invokedItem)
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, invokedItem);

    m_invokedItem.set(invokedItem);
}

#pragma region IItemsViewItemInvokedEventArgs

winrt::IInspectable ItemsViewItemInvokedEventArgs::InvokedItem()
{
    return m_invokedItem.get();
}

#pragma endregion
