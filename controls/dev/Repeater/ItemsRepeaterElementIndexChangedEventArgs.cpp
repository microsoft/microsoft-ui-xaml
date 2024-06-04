// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsRepeater.common.h"
#include "ItemsRepeaterElementIndexChangedEventArgs.h"

ItemsRepeaterElementIndexChangedEventArgs::ItemsRepeaterElementIndexChangedEventArgs(
    const winrt::UIElement& element,
    int oldIndex,
    int newIndex)
{
    Update(element, oldIndex, newIndex);
}
#pragma region IElementPreparedEventArgs

winrt::UIElement ItemsRepeaterElementIndexChangedEventArgs::Element()
{
    return m_element.get();
}

int32_t ItemsRepeaterElementIndexChangedEventArgs::OldIndex()
{
    return m_oldIndex;
}

int32_t ItemsRepeaterElementIndexChangedEventArgs::NewIndex()
{
    return m_newIndex;
}

#pragma endregion

void ItemsRepeaterElementIndexChangedEventArgs::Update(const winrt::UIElement& element, int oldIndex, int newIndex)
{
    m_element.set(element);
    m_oldIndex = oldIndex;
    m_newIndex = newIndex;
}