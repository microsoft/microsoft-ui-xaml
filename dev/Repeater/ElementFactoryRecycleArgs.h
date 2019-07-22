// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ElementFactoryRecycleArgs.g.h"

class ElementFactoryRecycleArgs : 
    public ReferenceTracker<ElementFactoryRecycleArgs, winrt::implementation::ElementFactoryRecycleArgsT, winrt::composable, winrt::composing>
{
public:
    ElementFactoryRecycleArgs() = default;

#pragma region IElementFactoryRecycleArgs
    winrt::UIElement Element();
    void Element(winrt::UIElement const& value);

    winrt::UIElement Parent();
    void Parent(winrt::UIElement const& value);
#pragma endregion

private:
    tracker_ref<winrt::UIElement> m_element{ this };
    tracker_ref<winrt::UIElement> m_parent{ this };
};