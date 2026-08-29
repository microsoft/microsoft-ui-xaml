// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "NonVirtualizingLayoutContext.h"
#include "LayoutContextAdapter.h"

#include "NonVirtualizingLayoutContext.properties.cpp"

#pragma region INonVirtualizingLayoutContext

winrt::IVectorView<winrt::UIElement> NonVirtualizingLayoutContext::Children()
{
    return overridable().ChildrenCore();
}

#pragma endregion

#pragma region INonVirtualizingLayoutContextOverrides

winrt::IVectorView<winrt::UIElement> NonVirtualizingLayoutContext::ChildrenCore()
{
    throw winrt::hresult_not_implemented();
}

#pragma endregion

winrt::VirtualizingLayoutContext NonVirtualizingLayoutContext::GetVirtualizingContextAdapter()
{
    if (!m_contextAdapter)
    {
        m_contextAdapter = winrt::make<LayoutContextAdapter>(get_strong().as<winrt::NonVirtualizingLayoutContext>());
    }
    return m_contextAdapter;
}
