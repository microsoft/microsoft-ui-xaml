// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsRepeaterElementPreparedEventArgs.g.h"

class ItemsRepeaterElementPreparedEventArgs :
    public ReferenceTracker<ItemsRepeaterElementPreparedEventArgs, winrt::implementation::ItemsRepeaterElementPreparedEventArgsT, winrt::composable, winrt::composing>
{
public:
    ItemsRepeaterElementPreparedEventArgs(
        const winrt::UIElement& element,
        int index);

#pragma region IElementPreparedEventArgs
    winrt::UIElement Element();
    int32_t Index();
#pragma endregion

    void Update(const winrt::UIElement& element, int index);

private:
    tracker_ref<winrt::UIElement> m_element{ this };
    int m_index{};
    winrt::hstring m_viewType;
};