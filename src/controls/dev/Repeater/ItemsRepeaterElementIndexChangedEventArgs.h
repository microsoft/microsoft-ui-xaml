// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsRepeaterElementIndexChangedEventArgs.g.h"

class ItemsRepeaterElementIndexChangedEventArgs :
    public ReferenceTracker<ItemsRepeaterElementIndexChangedEventArgs, winrt::implementation::ItemsRepeaterElementIndexChangedEventArgsT, winrt::composable, winrt::composing>
{
public:
    ItemsRepeaterElementIndexChangedEventArgs(
        const winrt::UIElement& element,
        int oldIndex,
        int newIndex);

#pragma region IElementPreparedEventArgs
    winrt::UIElement Element();
    int32_t OldIndex();
    int32_t NewIndex();
#pragma endregion

    void Update(const winrt::UIElement& element, int oldIndex, int newIndex);

private:
    tracker_ref<winrt::UIElement> m_element{ this };
    int m_oldIndex;
    int m_newIndex;
};