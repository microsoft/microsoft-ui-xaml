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

// Measure ancestors naturally, then measure the last item in the larger of the
// inline and overflow slots. Choose the visible suffix from these final sizes.
winrt::Size BreadcrumbLayout::MeasureOverride(winrt::NonVirtualizingLayoutContext const& context, winrt::Size const& availableSize)
{
    const auto children = context.Children();
    const int itemCount = static_cast<int>(children.Size());

    // Preparing a new last item can change the previous last item's template.
    const winrt::UIElement lastItem = itemCount > 1 ? children.GetAt(itemCount - 1) : nullptr;

    // Keep ancestor measure constraints stable across resizes, including repeated zero-width layouts.
    const winrt::Size naturalSize(
        availableSize.Width > 0 ? std::numeric_limits<float>::infinity() : 0,
        availableSize.Height);
    int oversizedAncestor = -1;
    float ancestorsWidth{};
    float ellipsisWidth{};

    // The first item is the overflow button; its measured width is the cost of hiding ancestors.
    m_ellipsisButton = itemCount > 0 ? children.GetAt(0) : nullptr;

    if (m_ellipsisButton)
    {
        m_ellipsisButton.Measure(availableSize);
        ellipsisWidth = m_ellipsisButton.DesiredSize().Width;
    }

    for (int i = 1; i < itemCount - 1; ++i)
    {
        const auto item = children.GetAt(i);
        item.Measure(naturalSize);
        const auto desiredSize = item.DesiredSize();
        ancestorsWidth += std::min(desiredSize.Width, availableSize.Width);

        if (desiredSize.Width > availableSize.Width)
        {
            oversizedAncestor = i;
        }
    }

    // Leave the last item measured at the same constraint used to decide overflow.
    // If its desired size does not change, the visibility decision stays valid too.
    float lastItemWidth = std::max(0.0f, availableSize.Width - std::min(ancestorsWidth, ellipsisWidth));
    winrt::Size lastItemSize{};

    if (lastItem)
    {
        lastItem.Measure(winrt::Size(lastItemWidth, availableSize.Height));
        lastItemSize = lastItem.DesiredSize();
    }

    m_ellipsisIsRendered = ancestorsWidth > ellipsisWidth &&
        ancestorsWidth + lastItemSize.Width > availableSize.Width;
    m_firstRenderedItemIndexAfterEllipsis = itemCount > 0 ? itemCount - 1 : 0;
    bool constrainedAncestor{};

    if (m_ellipsisIsRendered)
    {
        m_firstRenderedItemIndexAfterEllipsis = GetFirstBreadcrumbBarItemToArrange(children, availableSize, constrainedAncestor);
    }
    else if (oversizedAncestor >= 0)
    {
        // An oversized ancestor can still be visible alongside zero-width items.
        children.GetAt(oversizedAncestor).Measure(availableSize);
        constrainedAncestor = true;
    }

    if (constrainedAncestor)
    {
        ancestorsWidth = 0;

        for (int i = 1; i < itemCount - 1; ++i)
        {
            ancestorsWidth += std::min(children.GetAt(i).DesiredSize().Width, availableSize.Width);
        }

        const float updatedWidth = std::max(0.0f, availableSize.Width - std::min(ancestorsWidth, ellipsisWidth));

        if (updatedWidth != lastItemWidth)
        {
            lastItem.Measure(winrt::Size(updatedWidth, availableSize.Height));
            lastItemSize = lastItem.DesiredSize();
        }

        m_ellipsisIsRendered = ancestorsWidth > ellipsisWidth &&
            ancestorsWidth + lastItemSize.Width > availableSize.Width;

        if (!m_ellipsisIsRendered)
        {
            m_firstRenderedItemIndexAfterEllipsis = itemCount - 1;
        }
    }

    // Width constraints can also reduce height (for example, in a Viewbox).
    // Only the final sizes of visible items should reserve vertical space.
    const int firstItemToRender = m_ellipsisIsRendered ? m_firstRenderedItemIndexAfterEllipsis : 1;
    return winrt::Size(ancestorsWidth + lastItemSize.Width, GetBreadcrumbBarItemsHeight(children, firstItemToRender));
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

int BreadcrumbLayout::GetFirstBreadcrumbBarItemToArrange(
    winrt::IVectorView<winrt::UIElement> const& children,
    winrt::Size const& availableSize,
    bool& constrainedAncestor)
{
    const int itemCount = static_cast<int>(children.Size());
    float accumLength = children.GetAt(itemCount - 1).DesiredSize().Width +
        m_ellipsisButton.DesiredSize().Width;

    for (int i = itemCount - 2; i >= 1; --i)
    {
        const auto item = children.GetAt(i);
        float itemWidth = item.DesiredSize().Width;

        if (accumLength == 0 && itemWidth > availableSize.Width)
        {
            item.Measure(availableSize);
            itemWidth = item.DesiredSize().Width;
            constrainedAncestor = true;
        }

        float newAccumLength = accumLength + std::min(itemWidth, availableSize.Width);

        if (newAccumLength > availableSize.Width)
        {
            return i + 1;
        }

        accumLength = newAccumLength;
    }

    return 1;
}

float BreadcrumbLayout::GetBreadcrumbBarItemsHeight(winrt::IVectorView<winrt::UIElement> const& children, int firstItemToRender)
{
    float maxElementHeight{};

    if (m_ellipsisIsRendered)
    {
        maxElementHeight = m_ellipsisButton.DesiredSize().Height;
    }

    const int itemCount = static_cast<int>(children.Size());

    for (int i = firstItemToRender; i < itemCount; ++i)
    {
        maxElementHeight = std::max(maxElementHeight, children.GetAt(i).DesiredSize().Height);
    }

    return maxElementHeight;
}

// Arranging is performed in a single step, as many elements are tried to be drawn going from the last element
// towards the first one, if there's not enough space, then the ellipsis button is drawn
winrt::Size BreadcrumbLayout::ArrangeOverride(winrt::NonVirtualizingLayoutContext const& context, winrt::Size const& finalSize)
{
    const int itemCount = GetItemCount(context);
    int firstElementToRender{ 1 };
    m_visibleItemsCount = 0;

    // Use the visible suffix selected during measure. At least the ellipsis and
    // the last item are rendered when the leading items overflow.
    if (m_ellipsisIsRendered)
    {
        firstElementToRender = m_firstRenderedItemIndexAfterEllipsis;
    }

    float accumulatedWidths{};
    const float maxElementHeight = GetBreadcrumbBarItemsHeight(context.Children(), firstElementToRender);

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
