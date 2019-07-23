// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SwipeItemInvokedEventArgs.g.h"

class SwipeItemInvokedEventArgs
    : public winrt::implementation::SwipeItemInvokedEventArgsT<SwipeItemInvokedEventArgs>
{
public:
    SwipeItemInvokedEventArgs();

#pragma region ISwipeItemInvokedEventArgs
    winrt::SwipeControl SwipeControl();
#pragma endregion
    void SwipeControl(const winrt::SwipeControl& value);

private:
    winrt::SwipeControl m_swipeControl{};
};

