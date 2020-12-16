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

// Measuring is performed in a two step basis
// Step 1: During this step the sizes (width) of all BreadcrumbItems are added, if the accumulated width is bigger than
// the available size then an Ellipsis button is created. As the button is created, a flag is raised.
// Step 2: During this step, the BreadcrumbItems are measured again, we could avoid this step, and the ellipsis button is measured.
// The previously raised flag is reset.
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
        if (m_justCreatedEllipsisButton)
        {
            m_justCreatedEllipsisButton = false;
        }
        else
        {
            m_justCreatedEllipsisButton = true;
            InstantiateEllipsisButton(context);
        }
    }
    else
    {
        // remove button?
        m_justCreatedEllipsisButton = false;
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
    if (m_justCreatedEllipsisButton)
    {
        return finalSize;
    }

    if (m_hiddenElements)
    {
        m_hiddenElements.Clear();
    }

    const int itemCount = context.ItemCount();
    bool mustDrawEllipsisButton = (m_ellipsisButton.get() != nullptr);
    int firstElementToRender{};

    if (mustDrawEllipsisButton)
    {
        firstElementToRender = GetFirstBreadcrumbItemToArrange(context);
    }

    float accumulatedWidths{};
    float maxElementHeight{};

    if (itemCount > 0)
    {
        if (mustDrawEllipsisButton)
        {
            ArrangeItem(context, 0, accumulatedWidths, maxElementHeight);
        }
        else
        {
            HideItem(context, 0);
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
