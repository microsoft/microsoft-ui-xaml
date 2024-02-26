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
    BreadcrumbLayout(const winrt::BreadcrumbBar& breadcrumb);

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
    int GetFirstBreadcrumbBarItemToArrange(winrt::NonVirtualizingLayoutContext const& context);
    float GetBreadcrumbBarItemsHeight(winrt::NonVirtualizingLayoutContext const& context, int firstItemToRender);

    uint32_t GetItemCount(winrt::NonVirtualizingLayoutContext const& context);
    winrt::UIElement GetElementAt(winrt::NonVirtualizingLayoutContext const& context, uint32_t index);

    winrt::Size m_availableSize{};
    winrt::BreadcrumbBarItem m_ellipsisButton{nullptr};

    // Bug 48360852: [1.4 servicing] BreadcrumbBar leaks in File Explorer
    // A ref-counted pointer creates a reference cycle and a leak. Switch to the weak pointer when the fix is enabled under containment.
    winrt::BreadcrumbBar m_breadcrumb{ nullptr };
    winrt::weak_ref<winrt::BreadcrumbBar> m_breadcrumbWeakRef{ nullptr };  // weak_ref because the BreadcrumbBar already points to us via m_itemsRepeaterLayout

    bool m_ellipsisIsRendered{};
    uint32_t m_firstRenderedItemIndexAfterEllipsis{};
    uint32_t m_visibleItemsCount{};
};
