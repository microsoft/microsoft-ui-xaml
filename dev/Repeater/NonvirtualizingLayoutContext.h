// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutContext.h"
#include "NonVirtualizingLayoutContext.g.h"

class NonVirtualizingLayoutContext :
    public winrt::implementation::NonVirtualizingLayoutContextT<NonVirtualizingLayoutContext, LayoutContext>
{
public:
#pragma region INonVirtualizingLayoutContext
    winrt::IVectorView<winrt::UIElement> Children();
#pragma endregion

#pragma region INonVirtualizingLayoutContextOverrides
    virtual winrt::IVectorView<winrt::UIElement> ChildrenCore();
#pragma endregion

    winrt::VirtualizingLayoutContext GetVirtualizingContextAdapter();

private:
    winrt::VirtualizingLayoutContext m_contextAdapter{ nullptr };
};