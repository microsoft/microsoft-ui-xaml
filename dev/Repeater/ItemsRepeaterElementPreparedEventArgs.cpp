// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsRepeater.common.h"
#include "ItemsRepeaterElementPreparedEventArgs.h"

ItemsRepeaterElementPreparedEventArgs::ItemsRepeaterElementPreparedEventArgs(
    const winrt::UIElement& element,
    int index)
{
    Update(element, index);
}

#pragma region IElementPreparedEventArgs

winrt::UIElement ItemsRepeaterElementPreparedEventArgs::Element()
{
    return m_element.get();
}

int32_t ItemsRepeaterElementPreparedEventArgs::Index()
{
    return m_index;
}

#pragma endregion

void ItemsRepeaterElementPreparedEventArgs::Update(const winrt::UIElement& element, int index)
{
    m_element.set(element);
    m_index = index;
}