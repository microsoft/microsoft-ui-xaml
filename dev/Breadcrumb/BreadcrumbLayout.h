// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include "pch.h"
#include "common.h"

#include "Vector.h"
#include "NonVirtualizingLayout.h"
#include "VirtualizingLayout.h"

#include "BreadcrumbLayout.g.h"

struct BreadcrumbLayout :
    public ReferenceTracker<BreadcrumbLayout, winrt::implementation::BreadcrumbLayoutT, NonVirtualizingLayout>
{
public:
    BreadcrumbLayout();
    ~BreadcrumbLayout();

    winrt::Size MeasureOverride(
        winrt::NonVirtualizingLayoutContext const& context,
        winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(
        winrt::NonVirtualizingLayoutContext const& context,
        winrt::Size const& finalSize);

    bool EllipsisIsRendered();
    uint32_t FirstRenderedItemIndexAfterEllipsis();

private:
    void ArrangeItem(const winrt::UIElement& breadcrumbItem, float& accumulatedWidths, float& maxElementHeight);
    void ArrangeItem(const winrt::NonVirtualizingLayoutContext& context, int index, float& accumulatedWidths, float& maxElementHeight);
    void HideItem(const winrt::UIElement& breadcrumbItem);
    void HideItem(const winrt::NonVirtualizingLayoutContext& context, int index);
    int GetFirstBreadcrumbItemToArrange(winrt::NonVirtualizingLayoutContext const& context);

    uint32_t GetItemCount(winrt::NonVirtualizingLayoutContext const& context);
    winrt::UIElement GetElementAt(winrt::NonVirtualizingLayoutContext const& context, uint32_t index);

    winrt::Size m_availableSize;
    tracker_ref<winrt::BreadcrumbItem> m_ellipsisButton{ this };

    bool m_ellipsisIsRendered{};
    uint32_t m_firstRenderedItemIndexAfterEllipsis{};
};
