// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "LayoutPanel.h"
#include "LayoutPanelLayoutContext.h"
#include <common.h>
#include <pch.h>

LayoutPanelLayoutContext::LayoutPanelLayoutContext(winrt::LayoutPanel  /*unused*/const& owner)
{
    m_owner = winrt::make_weak(owner);
}

winrt::IVectorView<winrt::UIElement> LayoutPanelLayoutContext::ChildrenCore()
{
    return winrt::get_self<LayoutPanel>(GetOwner())->Children().GetView();
}

winrt::IInspectable LayoutPanelLayoutContext::LayoutStateCore()
{
    return winrt::get_self<LayoutPanel>(GetOwner())->LayoutState();
}

void LayoutPanelLayoutContext::LayoutStateCore(winrt::IInspectable const& value)
{
    winrt::get_self<LayoutPanel>(GetOwner())->LayoutState(value);
}

winrt::LayoutPanel LayoutPanelLayoutContext::GetOwner()
{
    return m_owner.get();
}