// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ElementFactoryGetArgs.g.h"

class ElementFactoryGetArgs :
    public ReferenceTracker<ElementFactoryGetArgs, winrt::implementation::ElementFactoryGetArgsT, winrt::composable, winrt::composing>
{
public:
    ElementFactoryGetArgs() = default;

#pragma region IElementFactoryGetArgs
    winrt::IInspectable Data();
    void Data(winrt::IInspectable const& value);

    winrt::UIElement Parent();
    void Parent(winrt::UIElement const& value);

    int Index();
    void Index(int value);
#pragma endregion

private:
    tracker_ref<winrt::IInspectable> m_data{ this };
    tracker_ref<winrt::UIElement> m_parent{ this };
    int m_index{};
};