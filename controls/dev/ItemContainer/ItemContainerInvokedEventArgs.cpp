// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemContainerInvokedEventArgs.h"

ItemContainerInvokedEventArgs::ItemContainerInvokedEventArgs(const winrt::ItemContainerInteractionTrigger& interactionTrigger, const winrt::IInspectable& originalSource)
{
    m_interactionTrigger = interactionTrigger;
    m_originalSource.set(originalSource);
}

#pragma region IItemContainerInvokedEventArgs

winrt::IInspectable ItemContainerInvokedEventArgs::OriginalSource()
{
    return m_originalSource.get();
}

winrt::ItemContainerInteractionTrigger ItemContainerInvokedEventArgs::InteractionTrigger()
{
    return m_interactionTrigger;
}

bool ItemContainerInvokedEventArgs::Handled()
{
    return m_handled;
}

void ItemContainerInvokedEventArgs::Handled(bool value)
{
    m_handled = value;
}

#pragma endregion
