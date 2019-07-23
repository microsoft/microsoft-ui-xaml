// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsRepeaterElementClearingEventArgs.g.h"

class ItemsRepeaterElementClearingEventArgs :
    public ReferenceTracker<ItemsRepeaterElementClearingEventArgs, winrt::implementation::ItemsRepeaterElementClearingEventArgsT, winrt::composable, winrt::composing>
{
public:
    explicit ItemsRepeaterElementClearingEventArgs(
        const winrt::UIElement& element);

#pragma region IElementClearingEventArgs
    winrt::UIElement Element();
#pragma endregion

    void Update(const winrt::UIElement& element);

private:
    tracker_ref<winrt::UIElement> m_element{ this };
};