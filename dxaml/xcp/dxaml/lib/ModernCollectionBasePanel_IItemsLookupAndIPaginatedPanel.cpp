// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"
#include "IScrollInfo.g.h"
#include "IScrollOwner.h"

using namespace DirectUI;

#undef min
#undef max

// Get the last visible item index if not grouping, and get the last visible group index if we are grouping
_Check_return_ HRESULT ModernCollectionBasePanel::GetLastItemIndexInViewport(
    _In_ DirectUI::IScrollInfo* scrollInfo,
    _Out_ INT* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = -1;
    
    wf::Rect window = {};
    xaml_controls::Orientation orientation;
    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));

    // Let's determine the window of interest
    {
        DOUBLE horizontalScrollOffset;
        DOUBLE verticalScrollOffset;
        DOUBLE viewportWidth;
        DOUBLE viewportHeight;
        FLOAT zoomFactor = 1.0f;
        wf::Rect scrollViewport;

        ctl::ComPtr<IInspectable> spScrollOwnerAsInspectable;

        IFC(scrollInfo->get_HorizontalOffset(&horizontalScrollOffset));
        IFC(scrollInfo->get_VerticalOffset(&verticalScrollOffset));
        IFC(scrollInfo->get_ViewportWidth(&viewportWidth));
        IFC(scrollInfo->get_ViewportHeight(&viewportHeight));

        IFC(scrollInfo->get_ScrollOwner(&spScrollOwnerAsInspectable));
        if (spScrollOwnerAsInspectable)
        {
            ctl::ComPtr<IScrollOwner> spScrollOwner = spScrollOwnerAsInspectable.AsOrNull<IScrollOwner>();
            if (spScrollOwner)
            {
                IFC(spScrollOwner->get_ZoomFactorImpl(&zoomFactor));
            }
        }

        scrollViewport.X = static_cast<float>(horizontalScrollOffset);
        scrollViewport.Y = static_cast<float>(verticalScrollOffset);
        scrollViewport.Width = static_cast<float>(viewportWidth);
        scrollViewport.Height = static_cast<float>(viewportHeight);
        IFC(CalculateVisibleWindowFromScrollData(scrollViewport, zoomFactor, &window));
    }

    // Let's walk the realized elements back from the end and grab the first one that passes the far edge of the window
    if (!m_cacheManager.IsGrouping())
    {
        INT32 closestValidContainerIndex = -1;
        for (INT32 validContainerIndex = m_containerManager.GetValidContainerCount() - 1; validContainerIndex >= 0 && closestValidContainerIndex < 0; --validContainerIndex)
        {
            ctl::ComPtr<IUIElement> spContainer;
            IFC(m_containerManager.GetContainerAtValidIndex(validContainerIndex, &spContainer));

            if (!GetElementIsSentinel(spContainer))
            {
                wf::Rect bounds = GetBoundsFromElement(spContainer);

                switch (orientation)
                {
                case xaml_controls::Orientation_Horizontal:
                    if (bounds.X < window.X + window.Width)
                    {
                        closestValidContainerIndex = validContainerIndex;
                    }
                    break;

                case xaml_controls::Orientation_Vertical:
                    if (bounds.Y < window.Y + window.Height)
                    {
                        closestValidContainerIndex = validContainerIndex;
                    }
                    break;
                }
            }
        }

        if (closestValidContainerIndex >= 0)
        {
            *returnValue = m_containerManager.GetItemIndexFromValidIndex(closestValidContainerIndex);
        }
    }
    else
    {
        INT32 closestValidHeaderIndex = -1;
        for (INT32 validHeaderIndex = m_containerManager.GetValidHeaderCount() - 1; validHeaderIndex >= 0 && closestValidHeaderIndex < 0; --validHeaderIndex)
        {
            ctl::ComPtr<IUIElement> spHeader;
            IFC(m_containerManager.GetHeaderAtValidIndex(validHeaderIndex, &spHeader));

            if (!GetElementIsSentinel(spHeader))
            {
                wf::Rect bounds = GetBoundsFromElement(spHeader);

                switch (orientation)
                {
                case xaml_controls::Orientation_Horizontal:
                    if (bounds.X < window.X + window.Width)
                    {
                        closestValidHeaderIndex = validHeaderIndex;
                    }
                    break;

                case xaml_controls::Orientation_Vertical:
                    if (bounds.Y < window.Y + window.Height)
                    {
                        closestValidHeaderIndex = validHeaderIndex;
                    }
                    break;
                }
            }
        }

        if (closestValidHeaderIndex >= 0)
        {
            *returnValue = m_containerManager.GetGroupIndexFromValidIndex(closestValidHeaderIndex);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetItemsPerPage(
    _In_ DirectUI::IScrollInfo* scrollInfo,
    _Out_ DOUBLE* returnValue)
{
    HRESULT hr = S_OK;

    wf::Rect window;

    // Let's determine the window of interest
    {
        DOUBLE horizontalScrollOffset;
        DOUBLE verticalScrollOffset;
        DOUBLE viewportWidth;
        DOUBLE viewportHeight;
        FLOAT zoomFactor = 1.0f;
        wf::Rect scrollViewport;

        ctl::ComPtr<IInspectable> spScrollOwnerAsInspectable;

        IFC(scrollInfo->get_HorizontalOffset(&horizontalScrollOffset));
        IFC(scrollInfo->get_VerticalOffset(&verticalScrollOffset));
        IFC(scrollInfo->get_ViewportWidth(&viewportWidth));
        IFC(scrollInfo->get_ViewportHeight(&viewportHeight));

        IFC(scrollInfo->get_ScrollOwner(&spScrollOwnerAsInspectable));
        if (spScrollOwnerAsInspectable)
        {
            ctl::ComPtr<IScrollOwner> spScrollOwner = spScrollOwnerAsInspectable.AsOrNull<IScrollOwner>();
            if (spScrollOwner)
            {
                IFC(spScrollOwner->get_ZoomFactorImpl(&zoomFactor));
            }
        }

        scrollViewport.X = static_cast<float>(horizontalScrollOffset);
        scrollViewport.Y = static_cast<float>(verticalScrollOffset);
        scrollViewport.Width = static_cast<float>(viewportWidth);
        scrollViewport.Height = static_cast<float>(viewportHeight);
        IFC(CalculateVisibleWindowFromScrollData(scrollViewport, zoomFactor, &window));
    }

    IFC(GetItemsPerPageImpl(window, returnValue));

Cleanup:
    RRETURN(hr);
}

// Virtual helper method to get the ItemsPerPage that can be overridden by derived classes.
_Check_return_ HRESULT
ModernCollectionBasePanel::GetItemsPerPageImpl(
    _In_ wf::Rect window,
    _Out_ DOUBLE* pItemsPerPage)
{
    HRESULT hr = S_OK;
    FLOAT panelExtent = 0;
    FLOAT windowSize = 0;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    *pItemsPerPage = 0;

    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));

    // We're going to follow (non-virtualizing) StackPanel's lead here
    // Meaning, we'll return an average items per page based on the total number or items
    // and the panel's extent in units of how many pages it spans
    {
        wf::Size desiredSize;
        IFC(get_DesiredSize(&desiredSize));

        switch (orientation)
        {
        case xaml_controls::Orientation_Horizontal:
            panelExtent = desiredSize.Width;
            windowSize = window.Width;
            break;

        case xaml_controls::Orientation_Vertical:
            panelExtent = desiredSize.Height;
            windowSize = window.Height;
            break;
        }
    }

    if (!DoubleUtil::AreClose(panelExtent, 0))
    {
        // This is easy when thinking in terms of units conversion/cancellation:
        // (elements/panel) * (pixels/window) / (pixels/panel) => (elements/panel) * (panels/window) => (elements/window)
        *pItemsPerPage = m_elementCountAtLastMeasure * (windowSize / panelExtent);
    }

Cleanup:
    RRETURN(hr);
}
