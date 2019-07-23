// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutContext.h"
#include "NonvirtualizingLayoutContext.h"
#include "VirtualizingLayoutContext.h"

class LayoutContextAdapter :
    public winrt::implements<LayoutContextAdapter, VirtualizingLayoutContext>
{
public:
    explicit LayoutContextAdapter(winrt::NonVirtualizingLayoutContext const& nonVirtualizingContext);

#pragma region ILayoutContextOverrides

    winrt::IInspectable LayoutStateCore();
    void LayoutStateCore(winrt::IInspectable const& state);

#pragma endregion

#pragma region IVirtualizingLayoutContextOverrides

    int32_t ItemCountCore();
    winrt::IInspectable GetItemAtCore(int index);
    winrt::UIElement GetOrCreateElementAtCore(int index, winrt::ElementRealizationOptions const& options);
    void RecycleElementCore(winrt::UIElement const& element);
    int32_t GetElementIndexCore(winrt::UIElement const& element);

    winrt::Rect RealizationRectCore();

    int32_t RecommendedAnchorIndexCore();

    winrt::Point LayoutOriginCore();
    void LayoutOriginCore(winrt::Point const& value);

#pragma endregion

private:
    winrt::weak_ref<winrt::NonVirtualizingLayoutContext> m_nonVirtualizingContext{ nullptr };
};
