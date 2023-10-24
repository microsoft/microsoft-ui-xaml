// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsRepeater.common.h"
#include "ItemsRepeaterElementClearingEventArgs.h"

ItemsRepeaterElementClearingEventArgs::ItemsRepeaterElementClearingEventArgs(const winrt::UIElement& element)
{
    Update(element);
}

#pragma region IElementClearingEventArgs

winrt::UIElement ItemsRepeaterElementClearingEventArgs::Element()
{
    return m_element.get();
}

#pragma endregion

void ItemsRepeaterElementClearingEventArgs::Update(const winrt::UIElement& element)
{
    m_element.set(element);
}