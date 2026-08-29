// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "ButtonInteractionInvokedEventArgs.h"

ButtonInteractionInvokedEventArgs::ButtonInteractionInvokedEventArgs(winrt::UIElement const& target)
{
    m_target.set(target);
}

winrt::UIElement ButtonInteractionInvokedEventArgs::Target()
{
    return m_target.get();
}
