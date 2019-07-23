// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ButtonInteractionInvokedEventArgs.g.h"

class ButtonInteractionInvokedEventArgs :
    public ReferenceTracker<ButtonInteractionInvokedEventArgs, winrt::implementation::ButtonInteractionInvokedEventArgsT, winrt::composing, winrt::composable>
{
public:
    explicit ButtonInteractionInvokedEventArgs(winrt::UIElement const& target);

    winrt::UIElement Target();

private:
    tracker_ref<winrt::UIElement> m_target{ this };
};
