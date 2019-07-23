// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "NonvirtualizingLayoutContext.h"

class LayoutPanelLayoutContext :
    public winrt::implements<LayoutPanelLayoutContext, NonVirtualizingLayoutContext>
{
public:
    explicit LayoutPanelLayoutContext(winrt::LayoutPanel const& owner);

#pragma region ILayoutContextOverrides

    winrt::IVectorView<winrt::UIElement> ChildrenCore();

    winrt::IInspectable LayoutStateCore();
    void LayoutStateCore(winrt::IInspectable const& value);

#pragma endregion


private:
    winrt::LayoutPanel GetOwner();

    // We hold a weak reference to prevent a leaking reference
    // cycle between the LayoutPanel and its layout.
    winrt::weak_ref<winrt::LayoutPanel> m_owner{};
};