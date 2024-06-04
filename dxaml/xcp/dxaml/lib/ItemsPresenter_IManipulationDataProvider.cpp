// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemsPresenter.g.h"
#include "VariableSizedWrapGrid.g.h"
#include "Panel.g.h"
#include "ScrollContentPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Uncomment to get debug traces
//#define HEADER_DBG

// IManipulationDataProvider methods accessed by owning ScrollViewer to support DirectManipulation.

_Check_return_ HRESULT ItemsPresenter::get_PhysicalOrientation(
    _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IManipulationDataProvider> spIManipulationDataProvider;
    ctl::ComPtr<IStackPanel> spStackPanel;
    ctl::ComPtr<IVariableSizedWrapGrid> spVSWG;

    IFC(get_Panel(&spPanel));

    spIManipulationDataProvider = spPanel.AsOrNull<IManipulationDataProvider>();

    if (spIManipulationDataProvider)
    {
        IFC(spIManipulationDataProvider->get_PhysicalOrientation(pValue));
    }
    else
    {
        spStackPanel = spPanel.AsOrNull<IStackPanel>();
        if (spStackPanel)
        {
            IFC(spStackPanel->get_Orientation(pValue));
        }
        else
        {
            spVSWG = spPanel.AsOrNull<IVariableSizedWrapGrid>();
            if (spVSWG)
            {
                IFC(spVSWG.Cast<VariableSizedWrapGrid>()->get_PhysicalOrientation(pValue));
            }
            else
            {
                // unknown why we're not looking for IOrientedPanel. I do not want to change
                // behavior across the board, so I'm doing a lookup for our new style panel instead of the
                // interface
                ctl::ComPtr<IModernCollectionBasePanel> spNewStyleVirtualizingPanel;
                spNewStyleVirtualizingPanel = spPanel.AsOrNull<IModernCollectionBasePanel>();
                if (spNewStyleVirtualizingPanel)
                {
                    IFC(spNewStyleVirtualizingPanel.AsOrNull<IOrientedPanel>()->get_PhysicalOrientation(pValue));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


// Called when a manipulation is starting or ending.
_Check_return_ HRESULT ItemsPresenter::UpdateInManipulation(
    _In_ BOOLEAN isInManipulation,
    _In_ BOOLEAN isInLiveTree,
    _In_ DOUBLE nonVirtualizingOffset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IManipulationDataProvider> spIManipulationDataProvider;

    if (isInLiveTree)
    {
        IFC(InvalidateMeasure());
    }

    IFC(get_Panel(&spPanel));

    spIManipulationDataProvider = spPanel.AsOrNull<IManipulationDataProvider>();

    if (spIManipulationDataProvider)
    {
        IFC(spIManipulationDataProvider->UpdateInManipulation(isInManipulation,
                                                        isInLiveTree,
                                                        nonVirtualizingOffset));
    }


Cleanup:
    RRETURN(hr);
}

// Updates the zoom factor
_Check_return_ HRESULT ItemsPresenter::SetZoomFactor(
    _In_ FLOAT newZoomFactor)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IManipulationDataProvider> spIManipulationDataProvider;
    BOOLEAN bIsInDMZoom = FALSE;

    IFC(get_Panel(&spPanel));
    spIManipulationDataProvider = spPanel.AsOrNull<IManipulationDataProvider>();

    if (spIManipulationDataProvider)
    {
        IFC(spIManipulationDataProvider->SetZoomFactor(newZoomFactor));
    }

    m_fZoomFactor = newZoomFactor;

    IFC(IsInDirectManipulationZoom(bIsInDMZoom));
    if (!bIsInDMZoom)
    {
        IFC(InvalidateMeasure());
    }

Cleanup:
    RRETURN(hr);
}

// Gets the scrolling extent in pixels even for logical scrolling scenarios.
_Check_return_ HRESULT ItemsPresenter::ComputePixelExtent(
    _In_ bool ignoreZoomFactor,
    _Out_ DOUBLE& extent)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IManipulationDataProvider> spIManipulationDataProvider;
    wf::Size headerSize = {};
    BOOLEAN isHorizontal = FALSE;
    FLOAT zoomFactor = 1;
    xaml::Thickness padding = {};

    if (!ignoreZoomFactor)
    {
        IFC(GetZoomFactor(&zoomFactor));
        ASSERT(zoomFactor == m_fZoomFactor);
    }

    IFC(get_Panel(&spPanel));
    spIManipulationDataProvider = spPanel.AsOrNull<IManipulationDataProvider>();

    if (spIManipulationDataProvider)
    {
        IFC(spIManipulationDataProvider->ComputePixelExtent(ignoreZoomFactor, extent));
    }

    IFC(GetHeaderSize(headerSize));
    IFC(get_PaddingInternal(&padding));
    IFC(IsHorizontal(isHorizontal));

    if (isHorizontal)
    {
        extent += (headerSize.Width + padding.Left + padding.Right) * zoomFactor;
    }
    else
    {
        extent += (headerSize.Height + padding.Top + padding.Bottom) * zoomFactor;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the offset in pixels even for logical scrolling scenarios.
_Check_return_ HRESULT ItemsPresenter::ComputePixelOffset(
    _In_ BOOLEAN isForHorizontalOrientation,
    _Out_ DOUBLE& offset)
{
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IManipulationDataProvider> spIManipulationDataProvider;
    BOOLEAN isHorizontal = FALSE;
    DOUBLE currentOffset = 0;
    FLOAT zoomFactor = 1;
    DOUBLE innerPanelOffset = 0;
    wf::Size sizeOfFirstVisibleChild = {};

    offset = 0;

    IFC_RETURN(GetZoomFactor(&zoomFactor));

    ASSERT(zoomFactor == m_fZoomFactor);

    IFC_RETURN(get_Panel(&spPanel));
    spIManipulationDataProvider = spPanel.AsOrNull<IManipulationDataProvider>();

    if (spIManipulationDataProvider)
    {
        IFC_RETURN(spIManipulationDataProvider->ComputePixelOffset(isForHorizontalOrientation, offset));
    }

    IFC_RETURN(IsHorizontal(isHorizontal));
    if (isHorizontal)
    {
        IFC_RETURN(get_HorizontalOffset(&currentOffset));
    }
    else
    {
        IFC_RETURN(get_VerticalOffset(&currentOffset));
    }

    IFC_RETURN(GetInnerPanelOffset(isHorizontal, &innerPanelOffset));

    // Account for header/leading padding.
    DOUBLE headerAndLeadingPaddingSize = 0;
    if (DoubleUtil::LessThan(currentOffset, 2))
    {
        IFC_RETURN(GetHeaderAndLeadingPaddingPixelSize(isHorizontal, currentOffset, headerAndLeadingPaddingSize));
    }
    else
    {
        IFC_RETURN(GetHeaderAndLeadingPaddingPixelSize(isHorizontal, headerAndLeadingPaddingSize));
    }

    offset += headerAndLeadingPaddingSize * zoomFactor;

    // Account for trailing padding
    DOUBLE offsetForTrailingPadding = 0;
    DOUBLE firstVisibleItemSize = 0;
    if (DoubleUtil::GreaterThan(currentOffset - innerPanelOffset, 2))
    {
        IFC_RETURN(GetSizeOfContainer(sizeOfFirstVisibleChild));
        firstVisibleItemSize = isHorizontal ? sizeOfFirstVisibleChild.Width : sizeOfFirstVisibleChild.Height;
        offsetForTrailingPadding = DoubleUtil::Min(1, currentOffset - innerPanelOffset - 2) * firstVisibleItemSize;
    }

    offset += offsetForTrailingPadding * zoomFactor;

    return S_OK;
}

// Gets the logical offset given a pixel delta.
_Check_return_ HRESULT ItemsPresenter::ComputeLogicalOffset(
    _In_ BOOLEAN isForHorizontalOrientation,
    _Inout_ DOUBLE& pixelDelta,
    _Out_ DOUBLE& logicalOffset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IManipulationDataProvider> spIManipulationDataProvider;
    DOUBLE currentOffset = 0;
    DOUBLE logicalOffsetInIP = 2;
    DOUBLE logicalOffsetInPanel = 0;
#ifdef HEADER_DBG
    WCHAR szValue[MAX_PATH];
#endif

    logicalOffset = 0;

    if (isForHorizontalOrientation)
    {
        currentOffset = m_ScrollData.get_OffsetX();
    }
    else
    {
        currentOffset = m_ScrollData.get_OffsetY();
    }

    pixelDelta = DoubleUtil::Round(pixelDelta, 0);
    logicalOffset = currentOffset;

    // we done, no more pixels to translate
    if (pixelDelta == 0)
    {
        goto Cleanup;
    }

#ifdef HEADER_DBG
    swprintf_s(szValue, MAX_PATH, L"Compute logical offset currentOffset: %f, pixelDelta: %f\n", currentOffset, pixelDelta);
    Trace(szValue);
#endif

    // Handle pixel delta for padding and header
    if (currentOffset < 2 || (currentOffset == 2 && pixelDelta < 0))
    {
        IFC(TranslatePixelDeltaToOffset(currentOffset, isForHorizontalOrientation, pixelDelta, logicalOffsetInIP));
        pixelDelta = DoubleUtil::Round(pixelDelta, 0);
    }
    logicalOffset = logicalOffsetInIP;

#ifdef HEADER_DBG
    swprintf_s(szValue, MAX_PATH, L"After IP translation pixelDelta: %f, logicalOffsetInHip: %f\n", pixelDelta, logicalOffset);
    Trace(szValue);
#endif

    // we done, no more pixels to translate
    if (pixelDelta == 0 || logicalOffsetInIP == 0)
    {
        goto Cleanup;
    }

    IFC(get_Panel(&spPanel));
    spIManipulationDataProvider = spPanel.AsOrNull<IManipulationDataProvider>();

    if (spIManipulationDataProvider)
    {
        IFC(spIManipulationDataProvider->ComputeLogicalOffset(isForHorizontalOrientation,
                                                             pixelDelta,
                                                             logicalOffsetInPanel));
        pixelDelta = DoubleUtil::Round(pixelDelta, 0);
    }
    logicalOffset += logicalOffsetInPanel;

#ifdef HEADER_DBG
    swprintf_s(szValue, MAX_PATH, L"After panel translation pixelDelta: %f, logicalOffset: %f\n", pixelDelta, logicalOffset);
    Trace(szValue);
#endif

    // we done, no more pixels to translate
    if (pixelDelta >= 0 || logicalOffset > 2)
    {
        goto Cleanup;
    }

    IFC(TranslatePixelDeltaToOffset(2, isForHorizontalOrientation, pixelDelta, logicalOffset));
    pixelDelta = DoubleUtil::Round(pixelDelta, 0);

#ifdef HEADER_DBG
    swprintf_s(szValue, MAX_PATH, L"After panel and re-IP translation pixelDelta: %f, logicalOffset: %f\n", pixelDelta, logicalOffset);
    Trace(szValue);
#endif

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::GetSizeOfFirstVisibleChild(
    _Out_ wf::Size& size)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;
    DOUBLE currentOffset = 0;

    IFC(IsHorizontal(isHorizontal));

    if (isHorizontal)
    {
        IFC(get_HorizontalOffset(&currentOffset));
    }
    else
    {
        IFC(get_VerticalOffset(&currentOffset));
    }

    if (DoubleUtil::GreaterThanOrClose(currentOffset, 2))
    {
        IFC(GetSizeOfContainer(size));
    }
    else
    {
        DOUBLE dimension = 0;
        IFC(GetSentinelItemSize((INT) currentOffset, TRUE, dimension));
        size.Width = static_cast<FLOAT>(dimension);
        IFC(GetSentinelItemSize((INT) currentOffset, FALSE, dimension));
        size.Height = static_cast<FLOAT>(dimension);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::GetSizeOfContainer(
    _Out_ wf::Size& size)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IManipulationDataProvider> spIManipulationDataProvider;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<IUIElement> spFirstChild;

    size.Height = 0;
    size.Width = 0;

    IFC(get_Panel(&spPanel));

    spIManipulationDataProvider = spPanel.AsOrNull<IManipulationDataProvider>();
    if (spIManipulationDataProvider)
    {
        IFC(spIManipulationDataProvider->GetSizeOfFirstVisibleChild(size));
    }
    else
    {
        UINT nCount = 0;
        IFC(spPanel.Cast<Panel>()->get_Children(&spChildren));
        IFC(spChildren->get_Size(&nCount));
        if (nCount > 0)
        {
            IFC(spChildren->GetAt(0, &spFirstChild));
            IFC(spFirstChild->get_DesiredSize(&size));
        }
    }

Cleanup:
    RRETURN(hr);
}

// End of IManipulationDataProvider methods accessed by owning ScrollViewer to support DirectManipulation.

_Check_return_ HRESULT ItemsPresenter::TranslatePixelDeltaToOffset(
    _In_ DOUBLE currentOffset,
    _In_ BOOLEAN isHorizontal,
    _Inout_ DOUBLE& delta,
    _Out_ DOUBLE& value)
{
    HRESULT hr = S_OK;
    INT currentIndex = (INT) currentOffset;
    DOUBLE currentItemSize = 0;
    FLOAT zoomFactor = 1;
    DOUBLE itemLogicalOffset = DoubleUtil::Fractional(currentOffset);
    value = DoubleUtil::Floor(currentOffset);
    IFC(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    if (delta < 0)
    {
        while (delta < 0 && currentIndex >= 0)
        {
            IFC(GetSentinelItemSize(currentIndex, isHorizontal, currentItemSize));
            DOUBLE itemOffsetLeftOrBottom = 0.0;

            currentItemSize *= zoomFactor;
            itemOffsetLeftOrBottom = itemLogicalOffset * currentItemSize;

            itemOffsetLeftOrBottom += delta;
            if (itemOffsetLeftOrBottom >= 0)
            {
                value += itemOffsetLeftOrBottom / currentItemSize;
            }
            else
            {
                itemLogicalOffset = 1;
                value--;
                currentIndex--;
            }

            delta = DoubleUtil::Min(0, itemOffsetLeftOrBottom);
        }

        if (delta < 0)
        {
            delta = 0;
        }
    }
    else
    {
        while (delta > 0 && currentIndex <= 1)
        {
            IFC(GetSentinelItemSize(currentIndex, isHorizontal, currentItemSize));
            DOUBLE itemOffsetRightOrTop = 0.0;

            currentItemSize *= zoomFactor;
            itemOffsetRightOrTop = (1 - itemLogicalOffset) * currentItemSize;

            itemOffsetRightOrTop -= delta;
            if (itemOffsetRightOrTop >= 0)
            {
                value += (currentItemSize - itemOffsetRightOrTop) / currentItemSize;
            }
            else
            {
                itemLogicalOffset = 0;
                value++;
                currentIndex++;
            }

            delta = DoubleUtil::Max(0, -itemOffsetRightOrTop);
        }
    }

    value = DoubleUtil::Max(isHorizontal ? m_ScrollData.m_MinOffset.X : m_ScrollData.m_MinOffset.Y, value);

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemsPresenter::TranslateVerticalPixelDeltaToOffset(
    _In_ DOUBLE currentOffset,
    _Inout_ DOUBLE& delta,
    _Out_ DOUBLE& value)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));
    if (!isHorizontal)
    {
        IFC(ComputeLogicalOffset(isHorizontal, delta, value));
    }
    else
    {
        value = currentOffset + delta;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemsPresenter::TranslateHorizontalPixelDeltaToOffset(
    _In_ DOUBLE currentOffset,
    _Inout_ DOUBLE& delta,
    _Out_ DOUBLE& value)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));
    if (isHorizontal)
    {
        IFC(ComputeLogicalOffset(isHorizontal, delta, value));
    }
    else
    {
        value = currentOffset + delta;
    }

Cleanup:
    RRETURN(hr);
}

