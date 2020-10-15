// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ElementFactoryGetArgs.h"

#include "ElementFactoryGetArgs.properties.cpp"

#pragma region IElementFactoryGetArgs

winrt::IInspectable ElementFactoryGetArgs::Data()
{
    return m_data.get();
}

void ElementFactoryGetArgs::Data(winrt::IInspectable const& value)
{
    m_data.set(value);
}

winrt::UIElement ElementFactoryGetArgs::Parent()
{
    return m_parent.get();
}

void ElementFactoryGetArgs::Parent(winrt::UIElement const& value)
{
    m_parent.set(value);
}

#pragma endregion

int ElementFactoryGetArgs::Index()
{
    return m_index;
}

void ElementFactoryGetArgs::Index(int value)
{
    m_index = value;
}
