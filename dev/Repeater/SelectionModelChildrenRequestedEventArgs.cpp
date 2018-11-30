// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsRepeater.common.h"
#include "SelectionModelChildrenRequestedEventArgs.h"

SelectionModelChildrenRequestedEventArgs::SelectionModelChildrenRequestedEventArgs(const winrt::IInspectable& source)
{
    Initialize(source);
}

#pragma region ISelectionModelChildrenRequestedEventArgs

winrt::IInspectable SelectionModelChildrenRequestedEventArgs::Source()
{
    return m_source.get();
}

winrt::IInspectable SelectionModelChildrenRequestedEventArgs::Children()
{
    return m_children.get();
}

void SelectionModelChildrenRequestedEventArgs::Children(winrt::IInspectable const& value)
{
    m_children.set(value);
}

#pragma endregion

void SelectionModelChildrenRequestedEventArgs::Initialize(const winrt::IInspectable& source)
{
    m_source.set(source);
    m_children.set(nullptr);
}
