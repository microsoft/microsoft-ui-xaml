// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ElementFactoryGetArgsDownLevel.h"

#pragma region IElementFactoryGetArgs

winrt::IInspectable ElementFactoryGetArgsDownlevel::Data()
{
    return m_data.get();
}

void ElementFactoryGetArgsDownlevel::Data(winrt::IInspectable const& value)
{
    m_data.set(value);
}

winrt::UIElement ElementFactoryGetArgsDownlevel::Parent()
{
    return m_parent.get();
}

void ElementFactoryGetArgsDownlevel::Parent(winrt::UIElement const& value)
{
    m_parent.set(value);
}

#pragma endregion

int ElementFactoryGetArgsDownlevel::Index()
{
    return m_index;
}

void ElementFactoryGetArgsDownlevel::Index(int value)
{
    m_index = value;
}