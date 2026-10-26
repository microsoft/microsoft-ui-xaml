// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbLayout.h"
#include "BreadcrumbBar.h"

BreadcrumbLayout::BreadcrumbLayout()
{
}

BreadcrumbLayout::BreadcrumbLayout(const winrt::BreadcrumbBar& breadcrumb)
{
    m_breadcrumb = winrt::make_weak(breadcrumb);
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

// Measure natural widths before choosing the visible suffix, then constrain
// visible items to the space left by the ellipsis and preceding items.
winrt::Size BreadcrumbLayout::MeasureOverride(winrt::NonVirtualizingLayoutContext const& context, winrt::Size const& availableSize)
{
    winrt::Size accumulatedCrumbsSize(0, 0);
    const auto children = context.Children();
    const int itemCount = static_cast<int>(children.Size());

    // Keep a stable measure constraint across resizes, including repeated zero-width layouts.
    const winrt::Size naturalSize(
        availableSize.Width > 0 ? std::numeric_limits<float>::infinity() : 0,
        availableSize.Height);
    int itemToConstrain = -1;
    float lastItemWidth{};
    float ellipsisWidth{};

    for (int i = 0; i < itemCount; ++i)
    {
        const auto item = children.GetAt(i);
        item.Measure(naturalSize);
        auto desiredSize = item.DesiredSize();

        // The first item is the overflow button; measure it separately without
        // counting its width when deciding whether the breadcrumbs fit.
        if (i == 0)
        {
            m_ellipsisButton = item;

            if (desiredSize.Width > availableSize.Width)
            {
                item.Measure(availableSize);
                desiredSize = item.DesiredSize();
            }

            ellipsisWidth = desiredSize.Width;
        }
        else
        {
            // A single oversized item should trim without showing the overflow button.
            accumulatedCrumbsSize.Width += std::min(desiredSize.Width, availableSize.Width);
            accumulatedCrumbsSize.Height = std::max(accumulatedCrumbsSize.Height, desiredSize.Height);
            lastItemWidth = desiredSize.Width;

            if (desiredSize.Width > availableSize.Width)
            {
                itemToConstrain = i;
            }
        }
    }

    m_ellipsisIsRendered = accumulatedCrumbsSize.Width > availableSize.Width;
    m_firstRenderedItemIndexAfterEllipsis = itemCount > 0 ? itemCount - 1 : 0;
    float constrainedWidth = availableSize.Width;

    if (m_ellipsisIsRendered)
    {
        // Keep this selection for arrange: constrained items must not make
        // additional ancestors appear to fit.
        m_firstRenderedItemIndexAfterEllipsis = GetFirstBreadcrumbBarItemToArrange(children, availableSize.Width);

        // Match the suffix selection's addition order, avoiding rounding-induced
        // remeasurement (and trimming) of items that already fit.
        if (std::min(lastItemWidth, availableSize.Width) + ellipsisWidth > availableSize.Width)
        {
            itemToConstrain = itemCount - 1;
            constrainedWidth = std::max(0.0f, availableSize.Width - ellipsisWidth);
        }
        else if (itemToConstrain < static_cast<int>(m_firstRenderedItemIndexAfterEllipsis))
        {
            itemToConstrain = -1;
        }
    }

    // At positive widths, at most one visible item needs a smaller constraint.
    // Remeasure it so its content can trim or wrap within its actual layout slot,
    // and account for any increased height. Other items retain their cached natural sizes.
    if (itemToConstrain >= 0)
    {
        const auto item = children.GetAt(itemToConstrain);
        item.Measure(winrt::Size(constrainedWidth, availableSize.Height));
        accumulatedCrumbsSize.Height = std::max(accumulatedCrumbsSize.Height, item.DesiredSize().Height);
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

int BreadcrumbLayout::GetFirstBreadcrumbBarItemToArrange(winrt::IVectorView<winrt::UIElement> const& children, float availableWidth)
{
    const int itemCount = static_cast<int>(children.Size());
    float accumLength = std::min(children.GetAt(itemCount - 1).DesiredSize().Width, availableWidth) +
        m_ellipsisButton.DesiredSize().Width;

    for (int i = itemCount - 2; i >= 0; --i)
    {
        float newAccumLength = accumLength + std::min(children.GetAt(i).DesiredSize().Width, availableWidth);

        if (newAccumLength > availableWidth)
        {
            return i + 1;
        }

        accumLength = newAccumLength;
    }

    return 0;
}

float BreadcrumbLayout::GetBreadcrumbBarItemsHeight(winrt::NonVirtualizingLayoutContext const& context, int firstItemToRender)
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
    m_visibleItemsCount = 0;

    // Use the visible suffix selected during measure. At least the ellipsis and
    // the last item are rendered when the leading items overflow.
    if (m_ellipsisIsRendered)
    {
        firstElementToRender = m_firstRenderedItemIndexAfterEllipsis;
    }

    float accumulatedWidths{};
    const float maxElementHeight = GetBreadcrumbBarItemsHeight(context, firstElementToRender);

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

    if (const auto& breadcrumb = m_breadcrumb.get())
    {
        auto breadcrumbImpl = winrt::get_self<BreadcrumbBar>(breadcrumb);
        breadcrumbImpl->ReIndexVisibleElementsForAccessibility();
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
