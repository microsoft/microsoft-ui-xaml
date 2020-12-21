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

    if (acumulatedCrumbsSize.Width > availableSize.Width)
    {
        InstantiateEllipsisButton(context);
    }
    else
    {
        m_ellipsisButton.set(nullptr);
    }

    return acumulatedCrumbsSize;
}

void BreadcrumbLayout::ArrangeBreadcrumbItem(winrt::UIElement breadcrumbItem, float& accumulatedWidths, float& maxElementHeight)
{
    const winrt::Size elementSize = breadcrumbItem.DesiredSize();
    const winrt::Rect arrangeRect(accumulatedWidths, 0, elementSize.Width, elementSize.Height);
    breadcrumbItem.Arrange(arrangeRect);

    maxElementHeight = std::max(maxElementHeight, elementSize.Height);
    accumulatedWidths += elementSize.Width;
}

void BreadcrumbLayout::ArrangeItem(winrt::VirtualizingLayoutContext const& context, int index, float& accumulatedWidths, float& maxElementHeight)
{
    auto element = context.GetOrCreateElementAt(index);
    ArrangeBreadcrumbItem(element, accumulatedWidths, maxElementHeight);
}

void BreadcrumbLayout::HideItem(winrt::VirtualizingLayoutContext const& context, int index)
{
    auto element = context.GetOrCreateElementAt(index);
    const winrt::Rect arrangeRect(0, 0, 0, 0);
    element.Arrange(arrangeRect);

    if (m_hiddenElements && index != 0)
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

// Arranging is performed in a two step basis
// Step 1: If the flag is raised, do nothing, wait for the result from the second measurement.
// Step 2: The only arranging is done here
winrt::Size BreadcrumbLayout::ArrangeOverride(winrt::VirtualizingLayoutContext const& context, winrt::Size const& finalSize)
{
    if (m_hiddenElements)
    {
        m_hiddenElements.Clear();
    }

    const int itemCount = context.ItemCount();
    bool mustDrawEllipsisButton = (m_ellipsisButton.get() != nullptr);
    int firstElementToRender{};
    m_firstRenderedItemIndexAfterEllipsis = itemCount - 1;

    if (mustDrawEllipsisButton)
    {
        firstElementToRender = GetFirstBreadcrumbItemToArrange(context);
        m_firstRenderedItemIndexAfterEllipsis = firstElementToRender;
    }

    float accumulatedWidths{};
    float maxElementHeight{};

    if (itemCount > 0)
    {
        if (mustDrawEllipsisButton)
        {
            ArrangeItem(context, 0, accumulatedWidths, maxElementHeight);
            m_ellipsisIsRendered = true;
        }
        else
        {
            HideItem(context, 0);
            m_ellipsisIsRendered = false;
        }
    }

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

void BreadcrumbLayout::InstantiateEllipsisButton(winrt::VirtualizingLayoutContext const& context)
{
    if (const auto& ellipsisButton = context.GetOrCreateElementAt(0).try_as<winrt::BreadcrumbItem>())
    {
        if (const auto& ellipsisButtonImpl = winrt::get_self<BreadcrumbItem>(ellipsisButton))
        {
            ellipsisButtonImpl->SetPropertiesForEllipsisNode();
            m_ellipsisButton.set(ellipsisButton);
        }
    }
}
