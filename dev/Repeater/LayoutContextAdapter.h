// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutContext.h"
#include "VirtualizingLayoutContext.h"
#include "NonvirtualizingLayoutContext.h"

class LayoutContextAdapter :
    public winrt::implements<LayoutContextAdapter, VirtualizingLayoutContext>
{
public:
    LayoutContextAdapter(winrt::NonVirtualizingLayoutContext const& nonVirtualizingContext);

#pragma region ILayoutContextOverrides

    winrt::IInspectable LayoutStateCore() override;
    void LayoutStateCore(winrt::IInspectable const& state) override;

#pragma endregion

#pragma region IVirtualizingLayoutContextOverrides

    int32_t ItemCountCore() override;
    winrt::IInspectable GetItemAtCore(int index) override;
    winrt::UIElement GetOrCreateElementAtCore(int index, winrt::ElementRealizationOptions const& options) override;
    void RecycleElementCore(winrt::UIElement const& element) override;
    int32_t GetElementIndexCore(winrt::UIElement const& element) override;

    winrt::Rect RealizationRectCore() override;

    int32_t RecommendedAnchorIndexCore() override;

    winrt::Point LayoutOriginCore();
    void LayoutOriginCore(winrt::Point const& value);

#pragma endregion

private:
    winrt::weak_ref<winrt::NonVirtualizingLayoutContext> m_nonVirtualizingContext{ nullptr };
};
