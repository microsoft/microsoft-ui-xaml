// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include "pch.h"
#include "common.h"

#include "Vector.h"
#include "NonVirtualizingLayout.h"

class BreadcrumbLayout :
    public winrt::implements<BreadcrumbLayout, NonVirtualizingLayout>
{
public:
    BreadcrumbLayout();
    BreadcrumbLayout(const winrt::Breadcrumb& breadcrumb);

    ~BreadcrumbLayout();

    winrt::Size MeasureOverride(
        winrt::NonVirtualizingLayoutContext const& context,
        winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(
        winrt::NonVirtualizingLayoutContext const& context,
        winrt::Size const& finalSize);

    bool EllipsisIsRendered();
    uint32_t FirstRenderedItemIndexAfterEllipsis();
    uint32_t GetVisibleItemsCount();

private:
    void ArrangeItem(const winrt::UIElement& breadcrumbItem, float& accumulatedWidths, float maxElementHeight);
    void ArrangeItem(const winrt::NonVirtualizingLayoutContext& context, int index, float& accumulatedWidths, float maxElementHeight);
    void HideItem(const winrt::UIElement& breadcrumbItem);
    void HideItem(const winrt::NonVirtualizingLayoutContext& context, int index);
    int GetFirstBreadcrumbItemToArrange(winrt::NonVirtualizingLayoutContext const& context);
    float GetBreadcrumbItemsHeight(winrt::NonVirtualizingLayoutContext const& context, int firstItemToRender);

    uint32_t GetItemCount(winrt::NonVirtualizingLayoutContext const& context);
    winrt::UIElement GetElementAt(winrt::NonVirtualizingLayoutContext const& context, uint32_t index);

    winrt::Size m_availableSize{};
    winrt::BreadcrumbItem m_ellipsisButton{nullptr};
    winrt::Breadcrumb m_breadcrumb{ nullptr };

    bool m_ellipsisIsRendered{};
    uint32_t m_firstRenderedItemIndexAfterEllipsis{};
    uint32_t m_visibleItemsCount{};
};
