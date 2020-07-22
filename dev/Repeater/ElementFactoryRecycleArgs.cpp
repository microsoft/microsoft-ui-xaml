// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ElementFactoryRecycleArgs.h"

#include "ElementFactoryRecycleArgs.properties.cpp"

#pragma region IElementFactoryRecycleArgs

winrt::UIElement ElementFactoryRecycleArgs::Element()
{
    return m_element.get();
}

void ElementFactoryRecycleArgs::Element(winrt::UIElement const& value)
{
    m_element.set(value);
}

winrt::UIElement ElementFactoryRecycleArgs::Parent()
{
    return m_parent.get();
}

void ElementFactoryRecycleArgs::Parent(winrt::UIElement const& value)
{
    m_parent.set(value);
}

#pragma endregion
