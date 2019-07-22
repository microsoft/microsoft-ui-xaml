// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SwipeItem.h"
#include "SwipeControl.h"
#include "SwipeItemInvokedEventArgs.h"

SwipeItemInvokedEventArgs::SwipeItemInvokedEventArgs()
= default;

#pragma region ISwipeItemInvokedEventArgs
winrt::SwipeControl SwipeItemInvokedEventArgs::SwipeControl()
{
    return m_swipeControl;
}
#pragma endregion

void SwipeItemInvokedEventArgs::SwipeControl(const winrt::SwipeControl& value)
{
    m_swipeControl = value;
}