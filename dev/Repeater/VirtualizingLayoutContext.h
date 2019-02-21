// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutContext.h"
#include "VirtualizingLayoutContext.g.h"

class VirtualizingLayoutContext :
    public winrt::implementation::VirtualizingLayoutContextT<VirtualizingLayoutContext, LayoutContext>
{
public:

#pragma region IVirtualizingLayoutContext
    int32_t ItemCount();
    winrt::IInspectable GetItemAt(int index);
    winrt::UIElement GetOrCreateElementAt(int index);
    winrt::UIElement GetOrCreateElementAt(int index, winrt::ElementRealizationOptions const& options);
    void RecycleElement(winrt::UIElement const& element);

    winrt::Rect RealizationRect();

    int32_t RecommendedAnchorIndex();

    winrt::Point LayoutOrigin();
    void LayoutOrigin(winrt::Point const& result);
#pragma endregion

#pragma region IVirtualizingLayoutContextOverrides
    virtual int32_t ItemCountCore();
    virtual winrt::IInspectable GetItemAtCore(int index);
    virtual winrt::UIElement GetElementAtCore(int index, winrt::ElementRealizationOptions const& options);
    virtual void RecycleElementCore(winrt::UIElement const& element);

    virtual winrt::Rect RealizationRectCore();

    virtual int32_t RecommendedAnchorIndexCore();

    virtual winrt::Point LayoutOriginCore();
    virtual void LayoutOriginCore(winrt::Point const& value);
#pragma endregion

    winrt::NonVirtualizingLayoutContext GetNonVirtualizingContextAdapter();

private:
    winrt::NonVirtualizingLayoutContext m_contextAdapter{ nullptr };
};
