// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "VirtualizingLayoutContext.h"
#include "VirtualLayoutContextAdapter.h"

VirtualLayoutContextAdapter::VirtualLayoutContextAdapter(winrt::VirtualizingLayoutContext const& virtualizingContext)
{
    m_virtualizingContext = winrt::make_weak(virtualizingContext);
}

#pragma region ILayoutContextOverrides

winrt::IInspectable VirtualLayoutContextAdapter::LayoutStateCore()
{
    if (auto context = m_virtualizingContext.get())
    {
        return context.LayoutState();
    }
    return nullptr;
}

void VirtualLayoutContextAdapter::LayoutStateCore(winrt::IInspectable const& state)
{
    if (auto context = m_virtualizingContext.get())
    {
        context.LayoutStateCore(state);
    }
}

#pragma endregion

#pragma region INonVirtualizingLayoutContextOverrides

winrt::IVectorView<winrt::UIElement> VirtualLayoutContextAdapter::ChildrenCore()
{
    if (!m_children)
    {
        m_children = winrt::make<ChildrenCollection<winrt::UIElement>>(m_virtualizingContext.get());
    }

    return m_children;
}

#pragma endregion
