// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbLayout.h"
#include "Breadcrumb.h"
#include "BreadcrumbItem.h"

BreadcrumbLayout::BreadcrumbLayout()
{
}

BreadcrumbLayout::BreadcrumbLayout(const winrt::Breadcrumb& breadcrumb)
{
    m_breadcrumb = breadcrumb;
}

BreadcrumbLayout::~BreadcrumbLayout()
{
}

uint32_t BreadcrumbLayout::GetItemCount(winrt::NonVirtualizingLayoutContext const& context)
{
    return static_cast<uint32_t>(context.Children().Size());
}

winrt::UIElement BreadcrumbLayout::GetElementAt(winrt::NonVirtualizingLayoutContext const& context, uint32_t index)
{
    return context.Children().GetAt(index);
}

// Measuring is performed in a single step, every element is measured, including the ellipsis
// item, but the total amount of space needed is only composed of the non-ellipsis breadcrumbs
winrt::Size BreadcrumbLayout::MeasureOverride(winrt::NonVirtualizingLayoutContext const& context, winrt::Size const& availableSize)
{
    m_availableSize = availableSize;

    winrt::Size accumulatedCrumbsSize(0, 0);

    for (uint32_t i = 0; i < GetItemCount(context); ++i)
    {
        auto breadcrumbItem = GetElementAt(context, i).as<BreadcrumbItem>();
        breadcrumbItem->Measure(availableSize);

        if (i != 0)
        {
            accumulatedCrumbsSize.Width += breadcrumbItem->DesiredSize().Width;
            accumulatedCrumbsSize.Height = std::max(accumulatedCrumbsSize.Height, breadcrumbItem->DesiredSize().Height);
        }
    }

    // Save a reference to the ellipsis button to avoid querying for it multiple times
    if (GetItemCount(context) > 0)
    {
        if (const auto& ellipsisButton = GetElementAt(context, 0).try_as<winrt::BreadcrumbItem>())
        {
            m_ellipsisButton = ellipsisButton;
        }
    }

    if (accumulatedCrumbsSize.Width > availableSize.Width)
    {
        m_ellipsisIsRendered = true;   
    }
    else
    {
        m_ellipsisIsRendered = false;
    }

    return accumulatedCrumbsSize;
}

void BreadcrumbLayout::ArrangeItem(const winrt::UIElement& breadcrumbItem, float& accumulatedWidths, float maxElementHeight)
{
    const winrt::Size elementSize = breadcrumbItem.DesiredSize();
    const winrt::Rect arrangeRect(accumulatedWidths, 0, elementSize.Width, maxElementHeight);
    breadcrumbItem.Arrange(arrangeRect);

    accumulatedWidths += elementSize.Width;
}

void BreadcrumbLayout::ArrangeItem(const winrt::NonVirtualizingLayoutContext& context, int index, float& accumulatedWidths, float maxElementHeight)
{
    const auto& element = GetElementAt(context, index);
    ArrangeItem(element, accumulatedWidths, maxElementHeight);
}

void BreadcrumbLayout::HideItem(const winrt::UIElement& breadcrumbItem)
{
    const winrt::Rect arrangeRect(0, 0, 0, 0);
    breadcrumbItem.Arrange(arrangeRect);
}

void BreadcrumbLayout::HideItem(const winrt::NonVirtualizingLayoutContext& context, int index)
{
    const auto& element = GetElementAt(context, index);
    HideItem(element);
}

int BreadcrumbLayout::GetFirstBreadcrumbItemToArrange(winrt::NonVirtualizingLayoutContext const& context)
{
    const int itemCount = GetItemCount(context);
    float accumLength = GetElementAt(context, itemCount - 1).DesiredSize().Width +
        m_ellipsisButton.DesiredSize().Width;

    for (int i = itemCount - 2; i >= 0; --i)
    {
        float newAccumLength = accumLength + GetElementAt(context, i).DesiredSize().Width;
        if (newAccumLength > m_availableSize.Width)
        {
            return i + 1;
        }
        accumLength = newAccumLength;
    }

    return 0;
}

float BreadcrumbLayout::GetBreadcrumbItemsHeight(winrt::NonVirtualizingLayoutContext const& context, int firstItemToRender)
{
    float maxElementHeight{};

    if (m_ellipsisIsRendered)
    {
        maxElementHeight = m_ellipsisButton.DesiredSize().Height;
    }

    for (uint32_t i = firstItemToRender; i < GetItemCount(context); ++i)
    {
        maxElementHeight = std::max(maxElementHeight, GetElementAt(context, i).DesiredSize().Height);
    }

    return maxElementHeight;
}

// Arranging is performed in a single step, as many elements are tried to be drawn going from the last element
// towards the first one, if there's not enough space, then the ellipsis button is drawn
winrt::Size BreadcrumbLayout::ArrangeOverride(winrt::NonVirtualizingLayoutContext const& context, winrt::Size const& finalSize)
{
    const int itemCount = GetItemCount(context);
    int firstElementToRender{};
    m_firstRenderedItemIndexAfterEllipsis = itemCount - 1;
    m_visibleItemsCount = 0;

    // If the ellipsis must be drawn, then we find the index (x) of the first element to be rendered, any element with
    // a lower index than x will be hidden (except for the ellipsis button) and every element after x (including x) will
    // be drawn. At the very least, the ellipis and the last item will be rendered
    if (m_ellipsisIsRendered)
    {
        firstElementToRender = GetFirstBreadcrumbItemToArrange(context);
        m_firstRenderedItemIndexAfterEllipsis = firstElementToRender;
    }

    float accumulatedWidths{};
    float maxElementHeight = GetBreadcrumbItemsHeight(context, firstElementToRender);

    // If there is at least one element, we may render the ellipsis item
    if (itemCount > 0)
    {
        const auto& ellipsisButton = m_ellipsisButton;

        if (m_ellipsisIsRendered)
        {
            ArrangeItem(ellipsisButton, accumulatedWidths, maxElementHeight);
        }
        else
        {
            HideItem(ellipsisButton);
        }
    }

    // For each item, if the item has an equal or larger index to the first element to render, then
    // render it, otherwise, hide it and add it to the list of hidden items
    for (int i = 1; i < itemCount; ++i)
    {
        if (i < firstElementToRender)
        {
            HideItem(context, i);
        }
        else
        {
            ArrangeItem(context, i, accumulatedWidths, maxElementHeight);
            ++m_visibleItemsCount;
        }
    }

    if (const auto& breadcrumb = m_breadcrumb.try_as<Breadcrumb>())
    {
        breadcrumb->ReIndexVisibleElementsForAccessibility();
    }

    return finalSize;
}

bool BreadcrumbLayout::EllipsisIsRendered()
{
    return m_ellipsisIsRendered;
}

uint32_t BreadcrumbLayout::FirstRenderedItemIndexAfterEllipsis()
{
    return m_firstRenderedItemIndexAfterEllipsis;
}

uint32_t BreadcrumbLayout::GetVisibleItemsCount()
{
    return m_visibleItemsCount;
}
