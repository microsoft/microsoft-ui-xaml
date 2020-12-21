// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbLayout.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "BreadcrumbItem.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithBasicFactory(BreadcrumbLayout)
}

#include "BreadcrumbLayout.g.cpp"

BreadcrumbLayout::BreadcrumbLayout()
{
    Orientation(winrt::Orientation::Horizontal);
    m_hiddenElements = winrt::make<Vector<winrt::IInspectable>>();
}

BreadcrumbLayout::~BreadcrumbLayout()
{
}

// Measuring is performed in a single step, every element is measured, including the ellipsis
// item, but the total amount of space needed is only composed of the non-ellipsis breadcrumbs
winrt::Size BreadcrumbLayout::MeasureOverride(winrt::VirtualizingLayoutContext const& context, winrt::Size const& availableSize)
{
    m_availableSize = availableSize;

    winrt::Size acumulatedCrumbsSize(0, 0);

    for (int i = 0; i < context.ItemCount(); ++i)
    {
        auto breadcrumbItem = context.GetOrCreateElementAt(i).as<BreadcrumbItem>();
        breadcrumbItem->Measure(availableSize);

        if (i != 0)
        {
            acumulatedCrumbsSize.Width += breadcrumbItem->DesiredSize().Width;
            acumulatedCrumbsSize.Height = std::max(acumulatedCrumbsSize.Height, breadcrumbItem->DesiredSize().Height);
        }
    }

    // Save a reference to the ellipsis button to avoid querying for it multiple times
    if (context.ItemCount() > 0)
    {
        if (const auto& ellipsisButton = context.GetOrCreateElementAt(0).try_as<winrt::BreadcrumbItem>())
        {
            m_ellipsisButton.set(ellipsisButton);
        }
    }

    if (acumulatedCrumbsSize.Width > availableSize.Width)
    {
        m_ellipsisIsRendered = true;   
    }
    else
    {
        m_ellipsisIsRendered = false;
    }

    return acumulatedCrumbsSize;
}

void BreadcrumbLayout::ArrangeItem(const winrt::UIElement& breadcrumbItem, float& accumulatedWidths, float& maxElementHeight)
{
    const winrt::Size elementSize = breadcrumbItem.DesiredSize();
    const winrt::Rect arrangeRect(accumulatedWidths, 0, elementSize.Width, elementSize.Height);
    breadcrumbItem.Arrange(arrangeRect);

    maxElementHeight = std::max(maxElementHeight, elementSize.Height);
    accumulatedWidths += elementSize.Width;
}

void BreadcrumbLayout::ArrangeItem(const winrt::VirtualizingLayoutContext& context, int index, float& accumulatedWidths, float& maxElementHeight)
{
    const auto& element = context.GetOrCreateElementAt(index);
    ArrangeItem(element, accumulatedWidths, maxElementHeight);
}

void BreadcrumbLayout::HideItem(const winrt::UIElement& breadcrumbItem)
{
    const winrt::Rect arrangeRect(0, 0, 0, 0);
    breadcrumbItem.Arrange(arrangeRect);
}

void BreadcrumbLayout::HideItem(const winrt::VirtualizingLayoutContext& context, int index)
{
    const auto& element = context.GetOrCreateElementAt(index);
    HideItem(element);

    if (m_hiddenElements)
    {
        m_hiddenElements.Append(context.GetItemAt(index));
    }
}

int BreadcrumbLayout::GetFirstBreadcrumbItemToArrange(winrt::VirtualizingLayoutContext const& context)
{
    const int itemCount = context.ItemCount();
    float acumLength = context.GetOrCreateElementAt(itemCount - 1).DesiredSize().Width +
        m_ellipsisButton.get().DesiredSize().Width;

    for (int i = itemCount - 2; i >= 0; --i)
    {
        float newAcumLength = acumLength + context.GetOrCreateElementAt(i).DesiredSize().Width;
        if (newAcumLength > m_availableSize.Width)
        {
            return i + 1;
        }
        acumLength = newAcumLength;
    }

    return 0;
}

// Arranging is performed in a single step, as many elements are tried to be drawn going from the last element
// towards the first one, if there's not enough space, then the ellipsis button is drawn
winrt::Size BreadcrumbLayout::ArrangeOverride(winrt::VirtualizingLayoutContext const& context, winrt::Size const& finalSize)
{
    // Hidden element list is cleared so it can be populated later
    if (m_hiddenElements)
    {
        m_hiddenElements.Clear();
    }

    const int itemCount = context.ItemCount();
    int firstElementToRender{};
    m_firstRenderedItemIndexAfterEllipsis = itemCount - 1;

    // If the ellipsis must be drawn, then we find the index (x) of the first element to be rendered, any element with
    // a lower index than x will be hidden (except for the ellipsis button) and every element after x (including x) will
    // be drawn. At the very least, the ellipis and the last item will be rendered
    if (m_ellipsisIsRendered)
    {
        firstElementToRender = GetFirstBreadcrumbItemToArrange(context);
        m_firstRenderedItemIndexAfterEllipsis = firstElementToRender;
    }

    float accumulatedWidths{};
    float maxElementHeight{};

    // If there is at least one element, we may render the ellipsis item
    if (itemCount > 0)
    {
        const auto& ellipsisButton = m_ellipsisButton.get();

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
        }
    }

    return finalSize;
}

winrt::Collections::IVector<winrt::IInspectable> BreadcrumbLayout::HiddenElements()
{
    return m_hiddenElements;
}

bool BreadcrumbLayout::EllipsisIsRendered()
{
    return m_ellipsisIsRendered;
}

uint32_t BreadcrumbLayout::FirstRenderedItemIndexAfterEllipsis()
{
    return m_firstRenderedItemIndexAfterEllipsis;
}
