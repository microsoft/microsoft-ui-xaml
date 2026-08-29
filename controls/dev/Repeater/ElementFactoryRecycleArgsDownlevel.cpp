// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ElementFactoryRecycleArgsDownLevel.h"

#pragma region IElementFactoryRecycleArgs

winrt::UIElement ElementFactoryRecycleArgsDownlevel::Element()
{
    return m_element.get();
}

void ElementFactoryRecycleArgsDownlevel::Element(winrt::UIElement const& value)
{
    m_element.set(value);
}

winrt::UIElement ElementFactoryRecycleArgsDownlevel::Parent()
{
    return m_parent.get();
}

void ElementFactoryRecycleArgsDownlevel::Parent(winrt::UIElement const& value)
{
    m_parent.set(value);
}

#pragma endregion
