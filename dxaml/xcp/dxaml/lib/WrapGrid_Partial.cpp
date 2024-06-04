// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      WrapGrid provides the default layout experience for the GridView
//      control. By default this is UI virtualized, it generates only those
//      containers which are being visible or focused

#include "precomp.h"
#include "WrapGrid.g.h"
#include <math.h>
#include "ScrollContentPresenter.g.h"
#include "ItemContainerGenerator.g.h"
#include "VirtualizingStackPanel.g.h"
#include "ItemsControl.g.h"
#include "OrientedSize.h"

//#define WG_DEBUG

using namespace DirectUI;
using namespace DirectUISynonyms;

WrapGrid::WrapGrid() :
    m_preCacheLines(3),
    m_bShouldComputeItemSize(TRUE)
{
    m_itemSize.Width = static_cast<FLOAT>(DoubleUtil::NaN);
    m_itemSize.Height = static_cast<FLOAT>(DoubleUtil::NaN);
    m_startingSize.Width = 0.0;
    m_startingSize.Height = 0.0;
    m_justificationSize.Width = 0.0;
    m_justificationSize.Height = 0.0;
    m_newItemsStartPosition.X = 0.0;
    m_newItemsStartPosition.Y = 0.0;
    m_lastArrangeSize.Width = 0.0;
    m_lastArrangeSize.Height = 0.0;
}

#if PERF_TUNE
//Allows number of cached columns for wrapgrid to be set in registry
_Check_return_
HRESULT
WrapGrid::Initialize()
{
    HRESULT hr = S_OK;
    XUINT32 lineCount = 0;

    IFC(WrapGridGenerated::Initialize());

    // Read Cached line count from Registry
    // This is temporary hack to determine number of columns to cache
    IFC(GetWrapGridCachedLinesCount(&lineCount));
    m_preCacheLines = lineCount;

Cleanup:
    RRETURN(hr);
}
#endif

// Set the HorizontalOffset to the passed value.
IFACEMETHODIMP
WrapGrid::SetHorizontalOffset(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    DOUBLE extentWidth = 0.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE scrollX = 0.0;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    BOOLEAN bHorizontal = FALSE;
    BOOLEAN bIsInDMZoom = FALSE;

    if (m_bInMeasure)
    {
        goto Cleanup;
    }

    IFC(get_PhysicalOrientation(&orientation));
    bHorizontal = (orientation == xaml_controls::Orientation_Horizontal);
    IFC(get_ExtentWidth(&extentWidth));
    IFC(get_ViewportWidth(&viewportWidth));

    IFC(ScrollContentPresenter::ValidateInputOffset(offset, m_ScrollData.m_MinOffset.X, bHorizontal ? DoubleUtil::MaxValue : extentWidth - viewportWidth, &scrollX));

    // If we are scrolling by item, then round the offset
    if (m_bItemBasedScrolling)
    {
        scrollX = DoubleUtil::Floor(scrollX);
    }

    if (!DoubleUtil::AreClose(scrollX, m_ScrollData.get_OffsetX()))
    {
        DOUBLE oldScrollX = m_ScrollData.get_OffsetX();

        IFC(m_ScrollData.put_OffsetX(scrollX));
        IFC(IsInDirectManipulationZoom(bIsInDMZoom));
        if (bIsInDMZoom)
        {
            goto Cleanup;
        }

        if (bHorizontal && !DoubleUtil::AreClose(DoubleUtil::Floor(scrollX), DoubleUtil::Floor(oldScrollX)))
        {
            // when scroll triggers layout it will trigger transitions. So disable transitions while scrolling.
            IFC(put_IsIgnoringTransitions(TRUE));
            IFC(InvalidateMeasure());
        }
        else
        {
            IFC(SetAndVerifyScrollingData(m_ScrollData.m_viewport, m_ScrollData.m_extent, m_ScrollData.get_Offset()));
            if (!m_bInManipulation)
            {
                IFC(put_IsIgnoringTransitions(TRUE));
                if (DoubleUtil::GreaterThanOrClose(scrollX, extentWidth - viewportWidth))
                {
                    IFC(InvalidateMeasure());
                }
                else
                {
                    IFC(InvalidateArrange());
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Set the VerticalOffset to the passed value.
IFACEMETHODIMP
WrapGrid::SetVerticalOffset(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    DOUBLE extentHeight = 0.0;
    DOUBLE viewportHeight = 0.0;
    DOUBLE scrollY = 0.0;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    BOOLEAN bVertical = FALSE;
    BOOLEAN bIsInDMZoom = FALSE;

    if (m_bInMeasure)
    {
        goto Cleanup;
    }

    IFC(get_PhysicalOrientation(&orientation));
    bVertical = (orientation == xaml_controls::Orientation_Vertical);
    IFC(get_ExtentHeight(&extentHeight));
    IFC(get_ViewportHeight(&viewportHeight));

    IFC(ScrollContentPresenter::ValidateInputOffset(offset, m_ScrollData.m_MinOffset.Y, bVertical ? DoubleUtil::MaxValue : extentHeight - viewportHeight, &scrollY));

    // If we are scrolling by item, then round the offset
    if (m_bItemBasedScrolling)
    {
        scrollY = DoubleUtil::Floor(scrollY);
    }

    if (!DoubleUtil::AreClose(scrollY, m_ScrollData.get_OffsetY()))
    {
        DOUBLE oldScrollY = m_ScrollData.get_OffsetY();

        IFC(m_ScrollData.put_OffsetY(scrollY));
        IFC(IsInDirectManipulationZoom(bIsInDMZoom));
        if (bIsInDMZoom)
        {
            goto Cleanup;
        }

        if (bVertical && !DoubleUtil::AreClose(DoubleUtil::Floor(scrollY), DoubleUtil::Floor(oldScrollY)))
        {
            // when scroll triggers layout it will trigger transitions. So disable transitions while scrolling.
            IFC(put_IsIgnoringTransitions(TRUE));
            IFC(InvalidateMeasure());
        }
        else
        {
            IFC(SetAndVerifyScrollingData(m_ScrollData.m_viewport, m_ScrollData.m_extent, m_ScrollData.get_Offset()));
            if (!m_bInManipulation)
            {
                IFC(put_IsIgnoringTransitions(TRUE));
                if (DoubleUtil::GreaterThanOrClose(scrollY, extentHeight - viewportHeight))
                {
                    IFC(InvalidateMeasure());
                }
                else
                {
                    IFC(InvalidateArrange());
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

#if PERF_TUNE
//Allows number of cached columns for wrapgrid to be set in registry

// Read WrapGridCachedLinesCount registry
_Check_return_ HRESULT
WrapGrid::GetWrapGridCachedLinesCount(
    _Inout_ XUINT32 *pValue)
{
    HRESULT hr = S_OK;

    HKEY hkSilverlight = NULL;
    bool bSucceeded = false;
    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);

    IFCPTR(pValue);
    *pValue = m_preCacheLines;
    LONG result;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, XAML_ROOT_KEY, 0, KEY_READ, &hkSilverlight) == ERROR_SUCCESS)
    {
        DWORD dwOutType;
        result = RegQueryValueEx(hkSilverlight, L"WrapGridCachedLinesCount", NULL, &dwOutType, (LPBYTE)&dwValue, &dwSize);
        if (ERROR_SUCCESS == result)
        {
            *pValue = dwValue;
        }
    }

Cleanup:
    if (hkSilverlight)
    {
        RegCloseKey(hkSilverlight);
    }
    RRETURN(hr);
}
#endif

// Handle the custom property changed event and call the OnPropertyChanged2
// methods.

_Check_return_ HRESULT WrapGrid::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(WrapGridGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        // Update the layout when any of the layout properties change
        case KnownPropertyIndex::WrapGrid_ItemWidth:
        case KnownPropertyIndex::WrapGrid_ItemHeight:
        case KnownPropertyIndex::WrapGrid_HorizontalChildrenAlignment:
        case KnownPropertyIndex::WrapGrid_VerticalChildrenAlignment:
        case KnownPropertyIndex::WrapGrid_MaximumRowsOrColumns:
        {
            m_bShouldComputeItemSize = TRUE;
            IFC(InvalidateMeasure());
            break;
        }

        case KnownPropertyIndex::WrapGrid_Orientation:
        {
            IFC(ResetScrolling());
            break;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Measures the items of a WrapGrid in anticipation of arranging them during the
// ArrangeOverride pass.
IFACEMETHODIMP WrapGrid::MeasureOverride(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* returnValue)
{
    TraceVirtualizationMeasureBegin();

    HRESULT hr = S_OK;
    OrientedSize itemSize;
    OrientedSize startingSize;
    OrientedSize justificationSize;
    OrientedSize totalSize;
    OrientedSize requiredSize;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    // MeasureCHildren, this will measure only required children. It uses UIVirtualization
    IFC(MeasureChildren(availableSize, returnValue));

    // Compute Children Alignments
    IFC(ComputeHorizontalAndVerticalAlignment(availableSize, &startingSize, &justificationSize));

    // If there is ChildrenAlignments, we need to add it in Measured size
    if (startingSize.get_Width() > 0 ||
        startingSize.get_Height() > 0 ||
        justificationSize.get_Width() > 0 ||
        justificationSize.get_Height() > 0 )
    {
        // Make sure we don't use DoubleUtil::NaN in our calculations.
        ASSERT(!DoubleUtil::IsNaN(m_itemSize.Width));
        ASSERT(!DoubleUtil::IsNaN(m_itemSize.Height));

        IFC(get_Orientation(&orientation));
        itemSize.put_Orientation(orientation);
        itemSize.put_Width(m_itemSize.Width);
        itemSize.put_Height(m_itemSize.Height);

        requiredSize.put_Orientation(orientation);
        requiredSize.put_Width(availableSize.Width);
        requiredSize.put_Height(availableSize.Height);

        // Compute and return the total amount of size used
        totalSize.put_Orientation(itemSize.get_Orientation());
        totalSize.put_Direct(
            itemSize.get_Direct() * m_itemsPerLine +
            justificationSize.get_Direct() * (m_itemsPerLine + 1) +
            startingSize.get_Direct() * 2.0);

        totalSize.put_Indirect(
            itemSize.get_Indirect() * m_lineCount +
            justificationSize.get_Indirect() * (m_lineCount + 1) +
            startingSize.get_Indirect() * 2.0);

        // Makesure the size doesn't go out of availableSize boundry.
        // Normally when there is scroll, the above total size logic increases the
        // size on scroll direction
        if(totalSize.get_Indirect() > requiredSize.get_Indirect())
        {
            totalSize.put_Indirect(requiredSize.get_Indirect());
        }

        if(totalSize.get_Direct() > requiredSize.get_Direct())
        {
            totalSize.put_Direct(requiredSize.get_Direct());
        }

        IFC(totalSize.AsUnorientedSize(returnValue));
    }

Cleanup:
    TraceVirtualizationMeasureEnd();
    RRETURN(hr);
}

// Calculate Virtualization variables
// Here are the variables which gets updated m_iBeforeTrail, m_iAfterTrail, and m_iVisibleCount
_Check_return_
HRESULT
WrapGrid::ComputeVirtualizationVariables(
    _In_ wf::Size availableSize,
    _In_ DOUBLE firstItemOffset,
    _In_ bool isHorizontal,
    _In_ bool isFirstTime,
    _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle,
    _Out_ XUINT32 *pRealizedLineCount,
    _Out_ INT *pLastViewPort,
    _Out_ wf::Size *pVisibleItemsSize,
    _Out_ DOUBLE *pLogicalVisibleSpace)
{
    XUINT32 afterTrailLines = 0;
    XUINT32 beforeTrailLines = 0;
    XUINT32 defaultAfterTrail = m_preCacheLines + 1; // initially afterCache will have 1 extra line
    XUINT32 defaultBeforeTrail = m_preCacheLines;
    XUINT32 visibleLineInViewport = 0;
    FLOAT zoomFactor = 1.0;
    XUINT32 maxLineCountAllowed;

    // Make sure we don't use DoubleUtil::NaN in our calculations.
    ASSERT(!DoubleUtil::IsNaN(m_itemSize.Width));
    ASSERT(!DoubleUtil::IsNaN(m_itemSize.Height));

    IFCEXPECT_RETURN(pRealizedLineCount);
    IFCEXPECT_RETURN(pLastViewPort);
    IFCEXPECT_RETURN(pLogicalVisibleSpace);
    IFCEXPECT_RETURN(pRealizedLineCount);

    XUINT32 visibleLines = 0;
    IFC_RETURN(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    // calculate line count
    m_lineCount = static_cast<XUINT32>(ceil(static_cast<XFLOAT>(m_nItemsCount) / static_cast<XFLOAT>(m_itemsPerLine)));
    if (isHorizontal)
    {
        availableSize.Width = availableSize.Width / zoomFactor;
        if (DoubleUtil::IsInfinity(availableSize.Width))
        {
            availableSize.Width = m_lineCount * m_itemSize.Width;
        }
        if (m_itemSize.Width > 0)
        {
            visibleLineInViewport = static_cast<XUINT32>(ceil(availableSize.Width / m_itemSize.Width));
        }
    }
    else
    {
        availableSize.Height = availableSize.Height / zoomFactor;
        if(DoubleUtil::IsInfinity(availableSize.Height))
        {
            availableSize.Height = m_lineCount * m_itemSize.Height;
        }
        if (m_itemSize.Height > 0)
        {
            visibleLineInViewport = static_cast<XUINT32>(ceil(availableSize.Height / m_itemSize.Height));
        }
    }

    // initialize with 0
    m_iAfterTrail = 0;
    m_iBeforeTrail = 0;

    // calculate visible Lines
    IFC_RETURN(ComputeVisibleLinesAndLastViewPort(
        availableSize,
        firstItemOffset,
        isHorizontal,
        visibleLineInViewport,
        &visibleLines,
        pVisibleItemsSize,
        pLastViewPort,
        pLogicalVisibleSpace));

    if (visibleLineInViewport < visibleLines)
    {
        defaultAfterTrail -= 1;
    }

    *pRealizedLineCount = visibleLines;

    if (m_lineCount > visibleLines && !isFirstTime)
    {
        maxLineCountAllowed = static_cast<XUINT32>(ceil(static_cast<XFLOAT>(maxNumOfItemsToGenerateInCurrentMeasureCycle) / static_cast<XFLOAT>(m_itemsPerLine)));
        if (visibleLines + defaultAfterTrail + defaultBeforeTrail > maxLineCountAllowed)
        {
            visibleLines = maxLineCountAllowed - defaultAfterTrail - defaultBeforeTrail;
        }

        // Realized line count should be constant after first measure with cached items
        *pRealizedLineCount = visibleLines + defaultAfterTrail + defaultBeforeTrail;

        // Assign default buffer to beforeTrailLines and afterTrailLines
        beforeTrailLines = defaultBeforeTrail;
        afterTrailLines = defaultAfterTrail;

        // Time to compute beforeTrail, AfterTrail, VisibleStart
        if (m_iVisibleStart == 0 || m_iVisibleStart < static_cast<INT>(defaultBeforeTrail))
        {
            // Case 1: when first item is visible on viewport
            // Case 2: when Scrolled a little bit, I mean > 0 lines and < defaultBeforeTrail
            // In these cases beforeTrail lines will be less then it's actual buffer
            // AfterTrailLines will get remaining buffer of beforeTrail
            beforeTrailLines = m_iVisibleStart;
            afterTrailLines = std::min(defaultAfterTrail + defaultBeforeTrail, m_lineCount - visibleLines) - beforeTrailLines;
        }
        else if (m_iVisibleStart + visibleLines + afterTrailLines >= m_lineCount)
        {
            // Case 3: when last item is visible on viewport
            // Case 4: Scroll to almost end, when number of lines in afterTailLines < afterTrailCache
            // in both cases, afterTrail gets less than it's actual buffer
            afterTrailLines = m_lineCount - (m_iVisibleStart + visibleLines);
            beforeTrailLines = std::min(defaultAfterTrail + defaultBeforeTrail, m_lineCount - visibleLines) - afterTrailLines;
        }

        m_iAfterTrail = afterTrailLines * m_itemsPerLine;
        m_iBeforeTrail = beforeTrailLines * m_itemsPerLine;
    }
    IFC_RETURN(ComputeVisibleCount(*pRealizedLineCount));
    if(isHorizontal)
    {
        pVisibleItemsSize->Width *= zoomFactor;
    }
    else
    {
        pVisibleItemsSize->Height *= zoomFactor;
    }

    return S_OK;
}

// Measure children
// This measure only required children, create only required container based on availableSize
_Check_return_
HRESULT
WrapGrid::MeasureChildren(
    // Measurement constraints, a control cannot return a size larger than the
    // constraint.
    _In_ wf::Size availableSize,
    // The desired size of the control.
    _Out_ wf::Size* returnValue)
{
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;

    wf::Size stackDesiredSize = {};
    DOUBLE firstItemOffset = 0.0;       // Offset of the top of the first child relative to the top of the viewport.
    INT lastViewport = -1;            // Last child index in the viewport.  -1 indicates we have not yet iterated through the last child.
    DOUBLE logicalVisibleSpace = 0;
    xaml_primitives::GeneratorPosition startPos = {-1, 0};
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    INT32 tick = 0;
    BOOLEAN bIsInDMZoom = FALSE;
    INT maxNumOfItemsToGenerateInCurrentMeasureCycle;

    XUINT32 realizedLineCount = 0;
    BOOLEAN isScrolling = FALSE;

    auto guard = wil::scope_exit([this]()
    {
        m_isGeneratingNewContainers = FALSE;
        m_bInMeasure = FALSE;
    });

    m_bInMeasure = TRUE;
    IFCPTR_RETURN(returnValue);

    // All Gets
    IFC_RETURN(get_PhysicalOrientation(&orientation));
    IFC_RETURN(get_IsScrolling(&isScrolling));
    IFC_RETURN(ItemsControl::GetItemsOwner(this, &spItemsControl));
    const bool isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    // If we have different available size in the direction of our orientation, we need to recalculate layout variables.
    if ((isHorizontal && !DoubleUtil::AreClose(availableSize.Height, m_LastSetAvailableSize.Height)) ||
        (!isHorizontal && !DoubleUtil::AreClose(availableSize.Width, m_LastSetAvailableSize.Width)) )
    {
        // We just need to set this flag since we're already in a measure pass.
        m_bShouldComputeItemSize = TRUE;
    }

    // Collect information from the ItemsControl, if there is one.
    if (spItemsControl)
    {
        ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;

        IFC_RETURN(GetItemContainerGenerator(&spGenerator, spItemsControl.Get()));

        if (spGenerator)
        {
            IFC_RETURN(spGenerator.Cast<ItemContainerGenerator>()->get_View(&spItems));
            IFC_RETURN(spItems->get_Size(&m_nItemsCount));
        }
    }

    if (!spItemsControl || !spGenerator)
    {
        m_nItemsCount = 0;
    }

    IFC_RETURN(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(static_cast<UIElement*>(this)->GetHandle()), (XINT16*)&tick));

    // the tick was stored by itemscontrol while setting up this itemshost
    // it represents the layouttick in which all containers should be considered to have been 'loaded' on the screen
    m_isGeneratingNewContainers = tick == GetItemsHostValidatedTick();
    IFC_RETURN(SetVirtualizationState(spItemsControl.Get()));
    IFC_RETURN(ComputeLayoutSlotSize(availableSize, isHorizontal, !!isScrolling));

    if (availableSize.Height < m_LastSetAvailableSize.Height || availableSize.Width < m_LastSetAvailableSize.Width)
    {
        // Reset the _maxDesiredSize cache if available size reduces from last available size.
        wf::Size empty = {};
        m_ScrollData.m_MaxDesiredSize = empty;
    }

    m_LastSetAvailableSize = availableSize;

    // Compute index of first item in the viewport
    m_iVisibleStart = ComputeIndexOfFirstVisibleItem(isHorizontal, firstItemOffset);

    // If recycling clean up before generating children.
    if (m_bIsVirtualizing)
    {
        IFC_RETURN(CleanupContainers(spItemsControl.Get(), availableSize));
#if DBG
        if (InRecyclingMode())
        {
            IFC_RETURN(debug_VerifyRealizedChildren());
        }
#endif
    }

#if WG_DEBUG
    WCHAR szValue[250];
    IFCEXPECT_RETURN(swprintf_s(szValue, 250, L"MeasureChildren m_iVisibleStart: %d", m_iVisibleStart) >= 0);
    Trace(szValue);
#endif

    // Restrict number of items being generated to the number of items generated in previous cycle if we are currently zooming.
    IFC_RETURN(IsInDirectManipulationZoom(bIsInDMZoom));
    maxNumOfItemsToGenerateInCurrentMeasureCycle = bIsInDMZoom ? m_iVisibleCount + m_iBeforeTrail : m_nItemsCount;

    m_iBeforeTrail = 0;
    m_iAfterTrail = 0;
    if(!m_bShouldComputeItemSize)
    {
        DOUBLE availableLength = 0;

        availableLength = isHorizontal ? availableSize.Height : availableSize.Width;
        if (DoubleUtil::IsInfinity(availableLength))
        {
            XINT32 maxRowsOrCols = -1;

            IFC_RETURN(get_MaximumRowsOrColumns(&maxRowsOrCols));

            // If items were removed so that we now have less items than m_itemsPerLine,
            // or if items were added and maxRowsOrCols was unset or m_nItemsCount was previously below maxRowsOrCols,
            // then recalculate m_itemsPerLine because it is invalid.
            if (m_nItemsCount < m_itemsPerLine ||
                (m_nItemsCount > m_itemsPerLine && (maxRowsOrCols <= 0 || static_cast<INT>(m_itemsPerLine) < maxRowsOrCols)))
            {
                wf::Size childDesiredSizeIgnored = {};
                // Since m_bShouldComputeItemSize is false, we know we'll have a valid m_itemSize.
                // Pass -1 as childIndex so we a) use m_itemSize instead of childDesiredSize, and b) don't rewrite m_itemSize.
                // This method's signature should be revisited, possibly as part of Blue: 4768 - WrapGrid - handle item size changes.
                IFC_RETURN(ComputeLayoutVariables(-1, availableSize, childDesiredSizeIgnored));
            }
        }

        // This will calculate Virtualization variables
        // for example, m_iBeforeTrail, m_iAfterTrail, m_iVisibleStart, m_iVisibleCount
        IFC_RETURN(ComputeVirtualizationVariables(availableSize, firstItemOffset, isHorizontal, FALSE, maxNumOfItemsToGenerateInCurrentMeasureCycle, &realizedLineCount, &lastViewport, &stackDesiredSize, &logicalVisibleSpace));
    }

    // get the position of the first item from where the items will get realized
    XUINT32 startIndex = VisibleStartItemIndex() - m_iBeforeTrail;
    IFC_RETURN(IndexToGeneratorPositionForStart(spItemsControl.Get(), startIndex, m_iFirstVisibleChildIndex, startPos));

    // Main loop: generate and measure all children (or all visible children if virtualizing).
    if (m_nItemsCount > 0)
    {
        XUINT32 itemCount = 0;
        BOOLEAN visualOrderChanged = FALSE;
        INT childIndex = m_iFirstVisibleChildIndex;
        BOOLEAN newlyRealized = FALSE;
        wf::Size childDesiredSize = {};

        IFC_RETURN(spGenerator->StartAt(startPos, xaml_primitives::GeneratorDirection_Forward, TRUE));
        m_iFirstVisibleChildIndex = m_iFirstVisibleChildIndex + m_iBeforeTrail;
        do
        {
            ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
            ctl::ComPtr<xaml::IUIElement> spUIChild;

            // Get next child.
            newlyRealized = FALSE;
            IFC_RETURN(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));

            if (!ctl::is<xaml::IUIElement>(spChildAsDO.Get()))
            {
                ASSERT(!newlyRealized, L"The generator realized a null value.");
                // We reached the end of the items (because of a group)
                break;
            }

            IFC_RETURN(spChildAsDO.As(&spUIChild));
            IFC_RETURN(AddContainerFromGenerator(spItemsControl.Get(), childIndex++, spUIChild.Get(), newlyRealized, visualOrderChanged));
            IFC_RETURN(MeasureChild(spUIChild.Get(), m_bShouldComputeItemSize ? m_LastSetChildLayoutSlotSize : m_itemSize, &childDesiredSize));

            // First measure will calculate Layout variables
            // for example m_lineCount, m_itemSize, m_itemsPerLine
            // it requires only during firsrt measure
            if(m_bShouldComputeItemSize)
            {
                m_bShouldComputeItemSize = FALSE;
                IFC_RETURN(spUIChild->get_DesiredSize(&childDesiredSize));
                IFC_RETURN(ComputeLayoutVariables(startIndex, availableSize, childDesiredSize));
                IFC_RETURN(ComputeVirtualizationVariables(availableSize, firstItemOffset, isHorizontal, TRUE, maxNumOfItemsToGenerateInCurrentMeasureCycle, &realizedLineCount, &lastViewport, &stackDesiredSize, &logicalVisibleSpace));
            }

            itemCount++;
            // Loop around and generate another item
        }
        while(static_cast<INT>(itemCount) < m_iVisibleCount + m_iBeforeTrail);

        IFC_RETURN(spGenerator->Stop());
    }

    // Adjust the scroll offset, extent, etc.
    if (isScrolling)
    {
        // Compute the extent before we fill remaining space and modify the stack desired size
        wf::Size extent = {};
        IFC_RETURN(ComputeLogicalExtent(stackDesiredSize, isHorizontal, extent));

        // Compute Scrolling data such as extent, viewport, and offset.
        IFC_RETURN(UpdateLogicalScrollData(stackDesiredSize, availableSize, logicalVisibleSpace,
                                    extent, lastViewport, isHorizontal));
    }

    //
    // Collect recycled containers
    //
    if (m_bIsVirtualizing && InRecyclingMode())
    {
        IFC_RETURN(CollectRecycledContainers());
    }

#if DBG
    IFC_RETURN(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    *returnValue = stackDesiredSize;

    return S_OK;
}

// Arranges the items of a WrapGrid.
IFACEMETHODIMP WrapGrid::ArrangeOverride(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    wf::Size unorientedStartingSize = {0.0, 0.0};
    wf::Size unorientedJustificationSize = {0.0, 0.0};
    wf::Point arrangeOffset = {0.0, 0.0};
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    OrientedSize startingSize;
    OrientedSize justificationSize;

    m_lastArrangeSize = finalSize;

    IFC_RETURN(ComputeHorizontalAndVerticalAlignment(finalSize, &startingSize, &justificationSize));

    IFC_RETURN(get_PhysicalOrientation(&orientation));
    IFC_RETURN(startingSize.AsUnorientedSize(&m_startingSize));
    IFC_RETURN(justificationSize.AsUnorientedSize(&m_justificationSize));
    const bool isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);
    IFC_RETURN(ComputeScrollOffsetForArrange(&arrangeOffset));

    //This methods arranges the required Children
    IFC_RETURN(ArrangeChildren(finalSize, m_startingSize, m_justificationSize, isHorizontal, arrangeOffset, returnValue));

    // This method arranges the extra visual children which are not part of realized children
    // It arrage them out of viweport so it is not visible
    IFC_RETURN(ArrangeExtraContainers(isHorizontal));

    // Update the ClipRect
    IFC_RETURN(SetupItemBoundsClip());

    if (!m_bShouldMeasureBuffers)
    {
        IFC_RETURN(SetFillBuffersTimer());
    }

    if (m_bNotifyLayoutRefresh)
    {
        m_bNotifyLayoutRefresh = FALSE;
        ctl::ComPtr<IScrollOwner> spOwner;
        IFC_RETURN(m_ScrollData.get_ScrollOwner(&spOwner));
        if (spOwner)
        {
            IFC_RETURN(spOwner->NotifyLayoutRefreshed());
        }
    }

    return S_OK;
}

// Provides the behavior for the Arrange pass of layout.  Classes
// can override this method to define their own Arrange pass
// behavior.
_Check_return_
HRESULT
WrapGrid::ArrangeChildren(
    // The computed size that is used to arrange the content.
    _In_ wf::Size arrangeSize,
    // starting size in case of alignments
    _In_ wf::Size startingSize,
    // JustificationSize in case of alignment=Stretch
    _In_ wf::Size justificationSize,
    // is Horizontal Orientation
    _In_ bool isHorizontal,
    // arrange offset
    _In_ wf::Point arrangeOffset,
    // The size of the control.
    _Out_ wf::Size* returnValue)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;

    wf::Rect rcChild = {0, 0, arrangeSize.Width, arrangeSize.Height};
    DOUBLE previousChildSize = 0.0;
    wf::Point startDeltaOfNewElements = {0.0, 0.0};
    wf::Rect contentBounds = {static_cast<FLOAT>(DoubleUtil::PositiveInfinity), static_cast<FLOAT>(DoubleUtil::PositiveInfinity), 0, 0};
    ctl::ComPtr<IScrollOwner> spOwner;

    auto guard = wil::scope_exit([this]()
    {
        // a scroll will trigger layout which will trigger transitions
        // that scroll has now been processed, so new transitions are allowed to be created
        VERIFYHR(put_IsIgnoringTransitions(FALSE));
    });

    IFCPTR_RETURN(returnValue);

    IFC_RETURN(m_ScrollData.get_ScrollOwner(&spOwner));

    arrangeOffset.X += startingSize.Width;
    arrangeOffset.Y += startingSize.Height;

    rcChild.X = arrangeOffset.X;
    rcChild.Y = arrangeOffset.Y;

    //
    // Arrange and Position Children.
    //
    UINT nCount = 0;
    BOOLEAN hasAccountedForEdge = FALSE;
    BOOLEAN isIgnoring = FALSE;

    IFC_RETURN(get_IsIgnoringTransitions(&isIgnoring));
    if (!isIgnoring && m_bInManipulation)
    {
        // make sure not to setup transitions during DM
        IFC_RETURN(put_IsIgnoringTransitions(TRUE));
        isIgnoring = TRUE;
    }

    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));

    #if WG_DEBUG
       WCHAR szValue[250];
       IFCEXPECT_RETURN(swprintf_s(szValue, 250, L"Arranger Children: m_iBTrail: %d m_iATrail: %d m_iVisibleStart: %d FVCI: %d nCount: %d m_iVisibleCount: %d",
            m_iBeforeTrail,
            m_iAfterTrail,
            m_iVisibleStart,
            m_iFirstVisibleChildIndex,
            nCount,
            m_iVisibleCount) >= 0);
       Trace(szValue);
    #endif

    // The area in which the items are arranged by subtracting Starting Size and TopLeft Justification size
    m_arrangedItemsRect.X = startingSize.Width + justificationSize.Width;
    m_arrangedItemsRect.Y = startingSize.Height + justificationSize.Height;

    if (!m_bShouldComputeItemSize &&
        !DoubleUtil::IsNaN(m_itemSize.Width) &&
        !DoubleUtil::IsNaN(m_itemSize.Height))
    {
        if (isHorizontal)
        {
            m_arrangedItemsRect.Height = m_itemSize.Height * m_itemsPerLine + (m_itemsPerLine - 1) * justificationSize.Height;
            m_arrangedItemsRect.Width = m_itemSize.Width * m_lineCount + (m_lineCount - 1) * justificationSize.Width;
        }
        else
        {
            m_arrangedItemsRect.Width = m_itemSize.Width * m_itemsPerLine + (m_itemsPerLine - 1) * justificationSize.Width;
            m_arrangedItemsRect.Height = m_itemSize.Height * m_lineCount + (m_lineCount - 1) * justificationSize.Height;
        }
    }
    else
    {
        ASSERT(nCount == 0, L"If m_itemSize is not yet initialized, we expect RealizedChildren to be empty.");
    }

    UINT index = 0;
    for (UINT i = 0; i < nCount; ++i)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;
        wf::Size childSize = {};
        bool isFocusChild = false;

        // When focus item is not part of VisibleItems, BeforeTrail or AfterTrail,
        // we need to arrage that item in separate Column
        if((i == m_iFirstVisibleChildIndex - m_iBeforeTrail) ||
            (i == m_iVisibleCount + m_iFirstVisibleChildIndex))
        {
            index = 0;
        }

        // we are looping through the actual containers; the visual children of this panel.
        IFC_RETURN(spRealizedChildren->GetAt(i, &spChild));
        IFC_RETURN(GetDesiredSize(spChild.Get(), &childSize));

        if (isHorizontal)
        {
            DOUBLE height = childSize.Height;
            if (IsFirstItemInLine(index))
            {
                rcChild.Y = arrangeOffset.Y + justificationSize.Height;
                rcChild.X += static_cast<FLOAT>(previousChildSize + justificationSize.Width);
                previousChildSize = childSize.Width;
                rcChild.Height = static_cast<FLOAT>(height);
                rcChild.Width = static_cast<FLOAT>(previousChildSize);
            }
            else
            {
                rcChild.Y += static_cast<FLOAT>(height + justificationSize.Height);
                rcChild.Width = static_cast<FLOAT>(previousChildSize);
                rcChild.Height = static_cast<FLOAT>(height);
            }
        }
        else
        {
            DOUBLE width = childSize.Width;
            if (IsFirstItemInLine(index))
            {
                rcChild.X = arrangeOffset.X + justificationSize.Width;
                rcChild.Y += static_cast<FLOAT>(previousChildSize + justificationSize.Height);
                previousChildSize = childSize.Height;
                rcChild.Width = static_cast<FLOAT>(width);
                rcChild.Height = static_cast<FLOAT>(previousChildSize);
            }
            else
            {
                rcChild.X += static_cast<FLOAT>(width + justificationSize.Width);
                rcChild.Width = static_cast<FLOAT>(width);
                rcChild.Height = static_cast<FLOAT>(previousChildSize);
            }
        }

        // If not ignoring Trasition
        if (!isIgnoring)
        {
            BOOLEAN isLocationValid = spChild.Cast<UIElement>()->GetIsLocationValid();
            if (!isLocationValid)
            {
                BOOLEAN wasInserted = FALSE;
                IFC_RETURN(OrientedVirtualizingPanelFactory::GetIsContainerGeneratedForInsertStatic(spChild.Get(), &wasInserted));
                if (!wasInserted)
                {
                    // this will be a load transition from the border of the screen
                    if (!hasAccountedForEdge)
                    {
                        if (isHorizontal)
                        {
                            startDeltaOfNewElements.X = m_newItemsStartPosition.X - rcChild.X;
                            startDeltaOfNewElements.Y = m_newItemsStartPosition.Y + startingSize.Height + justificationSize.Height;
                        }
                        else
                        {
                            startDeltaOfNewElements.X = m_newItemsStartPosition.X + startingSize.Width + justificationSize.Width;
                            startDeltaOfNewElements.Y = m_newItemsStartPosition.Y - rcChild.Y;
                        }
                        hasAccountedForEdge = TRUE;
                    }
                    IFC_RETURN(CoreImports::UIElement_SetIsEntering(static_cast<CUIElement*>(spChild.Cast<UIElement>()->GetHandle()), FALSE));

                    IFC_RETURN(CoreImports::UIElement_SetCurrentTransitionLocation(
                        static_cast<CUIElement*>(spChild.Cast<UIElement>()->GetHandle()),
                        rcChild.X + startDeltaOfNewElements.X,
                        rcChild.Y + startDeltaOfNewElements.Y,
                        rcChild.Width,
                        rcChild.Height));
                }
                else
                {
                    // we'll simply check if this element was place outside of the view, in which case we optimize by not starting a transition at all.
                    BOOLEAN isOutsideWindowLocationBased = rcChild.X < (-1 * rcChild.Width) ||
                        rcChild.Y < (-1 * rcChild.Height) ||
                        rcChild.X > arrangeSize.Width ||
                        rcChild.Y > arrangeSize.Height;

                    // if we are arranging an element inside of the viewwindow, we wish it to potentially do a load transition
                    // for that to happen we fake the entered state, by saying it just 'entered' the visual tree, even though virtualization
                    // might have re-used an element that was already in the visual tree.
                    //
                    // if we are arranging outside of the viewwindow, we just want an immediate move, so set the enter to have occurred before this moment
                    // and set the new location before the arrange comes in. When the arrange comes to this new location, no delta will be there so no
                    // transition will have to be setup.
                    IFC_RETURN(CoreImports::UIElement_SetIsEntering(static_cast<CUIElement*>(spChild.Cast<UIElement>()->GetHandle()), !isOutsideWindowLocationBased));

                    // now that it has entered, the start location needs to be set. this corresponds to the currentOffset and the nextGenerationOffset in the
                    // layouttransition storage. A load transition would then be calculated from this point, instead of its old location in the visual tree
                    IFC_RETURN(CoreImports::UIElement_SetCurrentTransitionLocation(static_cast<CUIElement*>(spChild.Cast<UIElement>()->GetHandle()), rcChild.X, rcChild.Y, rcChild.Width, rcChild.Height));
                }
            }
        }

        index += 1;

        isFocusChild = (m_iFirstVisibleChildIndex - m_iBeforeTrail > static_cast<INT>(i)) ||
                       (m_iVisibleCount + m_iFirstVisibleChildIndex -1 < static_cast<INT>(i));

        // If Manipulation is ON, we don't want the focus item to be visible, otherwise we end up seeing it while panning.
        // but since it is one of the realized item, we need to arrange it.
        // Drawing before trail items outside layout area
        // and same for after Trail items
        if (isFocusChild)
        {
            wf::Rect focusChild = rcChild;
            previousChildSize = 0.0;

            // Arranging elements at the FLT_MAX could cause issues with
            // graphics stack. For e.g. anything greater than 1<<21 may cause
            // issues with D2D glyph computations. Hence we arrange the focus child at a
            // large offset but not at FLT_MAX.
            focusChild.Y = VirtualizingPanel::ExtraContainerArrangeOffset;
            focusChild.X = VirtualizingPanel::ExtraContainerArrangeOffset;
            IFC_RETURN(spChild->Arrange(focusChild));
            index = 0;
        }
        else
        {
            IFC_RETURN(spChild->Arrange(rcChild));
        }

        if (!isFocusChild && EventEnabledVirtualizedCollectionUpdatedInfo())
        {
            contentBounds.X = static_cast<FLOAT>(DoubleUtil::Min(rcChild.X, contentBounds.X));
            contentBounds.Y = static_cast<FLOAT>(DoubleUtil::Min(rcChild.Y, contentBounds.Y));
            contentBounds.Width = static_cast<FLOAT>(DoubleUtil::Max(rcChild.X + rcChild.Width - contentBounds.X, contentBounds.Width));
            contentBounds.Height = static_cast<FLOAT>(DoubleUtil::Max(rcChild.Y + rcChild.Height - contentBounds.Y, contentBounds.Height));
        }

        spChild.Cast<UIElement>()->SetIsLocationValid(true);
    }

    // Save the NewItemStartPostionIndex
    if(isHorizontal)
    {
        // If first Item in line
        if (IsFirstItemInLine(index))
        {
            m_newItemsStartPosition.X = rcChild.X + rcChild.Width;
            m_newItemsStartPosition.Y = startingSize.Height + justificationSize.Height;
        }
        else
        {
            m_newItemsStartPosition.X = rcChild.X;
            m_newItemsStartPosition.Y = rcChild.Y + rcChild.Height;
        }
    }
    else
    {
        if (IsFirstItemInLine(index))
        {
            m_newItemsStartPosition.X = startingSize.Width  + justificationSize.Width;
            m_newItemsStartPosition.Y = rcChild.Y + rcChild.Height;
        }
        else
        {
            m_newItemsStartPosition.X = rcChild.X + rcChild.Width;
            m_newItemsStartPosition.Y = rcChild.Y;
        }
    }

    // Snap point might have changed, which might require to raise an event.
    IFC_RETURN(NotifySnapPointsChanges(spRealizedChildren.Get(), nCount));

    IFC_RETURN(RaiseVirtualizedCollectionUpdatedEvent(contentBounds));

    *returnValue = arrangeSize;

    return S_OK;
}

// Sets up IsVirtualizing, VirtualizationMode
//
// IsVirtualizing is TRUE if turned on via the items control and if the panel has a viewport.
// VSP has a viewport if it's either the scrolling panel or it was given MeasureData.
_Check_return_
HRESULT
WrapGrid::SetVirtualizationState(
    _In_ xaml_controls::IItemsControl* pItemsControl)
{
    HRESULT hr = S_OK;

    if (pItemsControl)
    {
        IFC(static_cast<ItemsControl*>(pItemsControl)->SetVirtualizationStateByPanel());

        m_VirtualizationMode = xaml_controls::VirtualizationMode_Recycling;
        m_bIsVirtualizing = TRUE;
    }

Cleanup:
     RRETURN(hr);
}

//Get Items per line
_Check_return_
HRESULT WrapGrid::ComputeLayoutVariables(
    _In_ XUINT32 childIndex,
    _In_ wf::Size availableSize,
    _In_ wf::Size desiredSize)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    XDOUBLE itemWidth = 0.0;
    XDOUBLE itemHeight = 0.0;
    OrientedSize maximumSize;
    XUINT32 itemsPerLine = 0;
    XINT32 maxItemsPerLine = -1;
    OrientedSize itemSize;

    IFC(get_Orientation(&orientation));
    IFC(get_ItemWidth(&itemWidth));
    IFC(get_ItemHeight(&itemHeight));

    // Orient the maximum size
    maximumSize.put_Orientation(orientation);
    maximumSize.put_Width(availableSize.Width);
    maximumSize.put_Height(availableSize.Height);

    // Determine the default item size
    itemSize.put_Orientation(orientation);

    // if itemWidth is not specified, use desiredSize or stored size (m_itemSize) based on whether the first item in current
    // realized children is very first item in Data or not
    if(DoubleUtil::IsNaN(itemWidth))
    {
        itemWidth = childIndex == 0 ? desiredSize.Width : m_itemSize.Width;
    }
    itemSize.put_Width(itemWidth);

    if(DoubleUtil::IsNaN(itemHeight))
    {
        itemHeight = childIndex == 0 ? desiredSize.Height : m_itemSize.Height;
    }
    itemSize.put_Height(itemHeight);

    // The m_itemSize should get updated only for first item in Data
    if(childIndex == 0)
    {
        IFC(itemSize.AsUnorientedSize(&m_itemSize));
    }

    ASSERT(!DoubleUtil::IsNaN(m_itemSize.Width));
    ASSERT(!DoubleUtil::IsNaN(m_itemSize.Height));

    // Determine the number of items that will fit per line
    itemsPerLine = DoubleUtil::IsInfinity(maximumSize.get_Direct()) ?
        m_nItemsCount :
        static_cast<XUINT32>(maximumSize.get_Direct() / itemSize.get_Direct());
    itemsPerLine = __max(1, itemsPerLine);
    IFC(get_MaximumRowsOrColumns(&maxItemsPerLine));
    if (maxItemsPerLine > 0)
    {
        itemsPerLine = __min(itemsPerLine, static_cast<UINT>(maxItemsPerLine));
    }
    m_itemsPerLine = itemsPerLine;

#if WG_DEBUG
    WCHAR szValue[250];
    IFCEXPECT(swprintf_s(szValue, 250, L"ComputeLayoutVariables itemsPerLine: %d m_itemSize.Width: %f m_itemSize.Height: %f",
        itemsPerLine, m_itemSize.Width, m_itemSize.Height) >= 0);
    Trace(szValue);
#endif

Cleanup:
    RRETURN(hr);
}

// Returns the Desired size of container
// In case of FixedSizeItems, returns the calculated Fixed size
_Check_return_
HRESULT
WrapGrid::GetDesiredSize(
    _In_ IUIElement* pChild,
    _Out_ wf::Size* pItemSize)
{
    HRESULT hr = S_OK;

    if(DoubleUtil::IsNaN(m_itemSize.Width) || DoubleUtil::IsNaN(m_itemSize.Height))
    {
        XDOUBLE itemWidth = 0.0;
        XDOUBLE itemHeight = 0.0;
        IFC(get_ItemWidth(&itemWidth));
        IFC(get_ItemHeight(&itemHeight));

        pItemSize->Width = static_cast<FLOAT>(itemWidth);
        pItemSize->Height = static_cast<FLOAT>(itemHeight);
    }
    else
    {
        pItemSize->Width = m_itemSize.Width;
        pItemSize->Height = m_itemSize.Height;
    }

Cleanup:
    RRETURN(hr);
}

// Computes the bounds used to layout the items.
_Check_return_ HRESULT WrapGrid::ComputeHorizontalAndVerticalAlignment(
    // The available size we have to layout the child items
    _In_ wf::Size availableSize,
    // The calculated amount of space we'll reserve from the top/left corner for
    // alignment (this is really being used more like an OrientedPoint, but
    // there's no need to create a separate struct just for this)
    _Out_ OrientedSize* pStartingSize,
    // The calculated amount of space we'll reserve between each item for
    // alignment if we're justifying the items
    _Out_ OrientedSize* pJustificationSize)
{
    HRESULT hr = S_OK;

    IFCPTR(pStartingSize);
    IFCPTR(pJustificationSize);

    if (DoubleUtil::IsNaN(m_itemSize.Width) || DoubleUtil::IsNaN(m_itemSize.Height))
    {
        pStartingSize->put_Width(0.0);
        pStartingSize->put_Height(0.0);
        pJustificationSize->put_Width(0.0);
        pJustificationSize->put_Height(0.0);
    }
    else
    {
        xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
        OrientedSize maximumSize;
        XDOUBLE startingOffset = 0.0;
        XDOUBLE justificationOffset = 0.0;
        xaml::HorizontalAlignment horizontalAlign = xaml::HorizontalAlignment_Left;
        xaml::VerticalAlignment verticalAlign = xaml::VerticalAlignment_Top;
        bool isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

        IFC(get_Orientation(&orientation));
        isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

        // Orient the maximum size
        maximumSize.put_Orientation(orientation);
        maximumSize.put_Width(availableSize.Width);
        maximumSize.put_Height(availableSize.Height);

        pStartingSize->put_Orientation(orientation);
        pJustificationSize->put_Orientation(orientation);

        IFC(get_VerticalChildrenAlignment(&verticalAlign));
        IFC(get_HorizontalChildrenAlignment(&horizontalAlign));

        // Determine how to adjust the items for horizontal content alignment
        IFC(ComputeAlignmentOffsets(
            static_cast<XUINT32>(isHorizontal ? verticalAlign : horizontalAlign),
            maximumSize.get_Width(),
            m_itemSize.Width * (isHorizontal ? m_itemsPerLine : m_lineCount),
            isHorizontal ? m_itemsPerLine : m_lineCount,
            &startingOffset,
            &justificationOffset));
        pStartingSize->put_Width(startingOffset);
        pJustificationSize->put_Width(justificationOffset);

        // Determine how to adjust the items for vertical content alignment
        IFC(ComputeAlignmentOffsets(
            static_cast<XUINT32>(isHorizontal ? horizontalAlign : verticalAlign),
            maximumSize.get_Height(),
            m_itemSize.Height * (isHorizontal ? m_lineCount : m_itemsPerLine),
            isHorizontal ? m_lineCount : m_itemsPerLine,
            &startingOffset,
            &justificationOffset));
        pStartingSize->put_Height(startingOffset);
        pJustificationSize->put_Height(justificationOffset);
    }

Cleanup:
    RRETURN(hr);
}

// Computes the children alignment offsets.
_Check_return_ HRESULT WrapGrid::ComputeAlignmentOffsets(
    // The alignment we're computing offsets for (this is either a
    // HorizontalAlignment or VerticalAlignment value we've forced into a UINT)
    _In_ XUINT32 alignment,
    // The available width (or height, depending on orientation)
    _In_ XDOUBLE availableSize,
    // The width (or height, depending on orientation) required to layout all of
    // the child items
    _In_ XDOUBLE requiredSize,
    // The total number of lines required to layout all of the child items
    _In_ XUINT32 totalLines,
    // The calculated initial top/left offset for alignment
    _Out_ XDOUBLE* pStartingOffset,
    // The calculated offset required between items for alignment
    _Out_ XDOUBLE* pJustificationOffset)
{
    HRESULT hr = S_OK;

    // Ensure the HorizontalAlignment and VerticalAlignment enum values
    // correctly match up because we're making the assumption they do (when we
    // pass them in as the value of our alignment property).  This is a hack,
    // but the enum values never change and the static asserts will create a
    // build error if they do.  If this ever fails, just use some clipboard
    // inheritance to specialize both horizontal and vertical versions of this
    // method.
    static_assert(static_cast<int>(xaml::HorizontalAlignment_Left) == static_cast<int>(xaml::VerticalAlignment_Top), "HorizontalAlignment.Left integral value does not equal VerticalAlignment.Top integral value!");
    static_assert(static_cast<int>(xaml::HorizontalAlignment_Center) == static_cast<int>(xaml::VerticalAlignment_Center), "HorizontalAlignment.Center integral value does not equal VerticalAlignment.Center integral value!");
    static_assert(static_cast<int>(xaml::HorizontalAlignment_Right) == static_cast<int>(xaml::VerticalAlignment_Bottom), "HorizontalAlignment.Right integral value does not equal VerticalAlignment.Bottom integral value!");
    static_assert(static_cast<int>(xaml::HorizontalAlignment_Stretch) == static_cast<int>(xaml::VerticalAlignment_Stretch), "HorizontalAlignment.Stretch integral value does not equal VerticalAlignment.Stretch integral value!");

    IFCPTR(pStartingOffset);
    IFCPTR(pJustificationOffset);
    *pStartingOffset = 0.0;
    *pJustificationOffset = 0.0;

    // There's nothing to align if we have infinite space.  Note that we default
    // to Top or Left alignment for infinite or otherwise unspecified values.
    if (DoubleUtil::IsInfinity(availableSize))
    {
        goto Cleanup;
    }
    else if (alignment == xaml::VerticalAlignment_Center)
    {
        *pStartingOffset = DoubleUtil::Max((availableSize - requiredSize) / 2.0, 0.0);
    }
    else if (alignment == xaml::VerticalAlignment_Bottom) // or HorizontalAlignment_Right
    {
        *pStartingOffset = DoubleUtil::Max(availableSize - requiredSize, 0.0);
    }
    else if (alignment == xaml::VerticalAlignment_Stretch)
    {
        *pJustificationOffset = DoubleUtil::Max((availableSize - requiredSize) / static_cast<XDOUBLE>(totalLines + 1), 0.0);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT WrapGrid::SupportsKeyNavigationAction(
    _In_ xaml_controls::KeyNavigationAction action,
    _Out_ BOOLEAN* pSupportsAction)
{
    // Let the Selector handle Next/Previous.
    *pSupportsAction =
        (action != xaml_controls::KeyNavigationAction_Next) &&
        (action != xaml_controls::KeyNavigationAction_Previous);

    RRETURN(S_OK);
}

// Starting from sourceIndex, returns the index in the given direction.
//
// actionValidForSourceIndex can be FALSE in following conditions:
// If sourceIndex is 0 and Key is Up or Left
// If sourceIndex is maxRow -1, maxCol -1 and Key is Right or Down
// if Key is not Up, Down, Right or Left
_Check_return_ HRESULT WrapGrid::GetTargetIndexFromNavigationAction(
    _In_ UINT sourceIndex,
    _In_ xaml_controls::ElementType /*sourceType*/,
    _In_ xaml_controls::KeyNavigationAction action,
    _In_ BOOLEAN allowWrap,
    _In_ UINT /*itemIndexHintForHeaderNavigation*/,
    _Out_ UINT* computedTargetIndex,
    _Out_ xaml_controls::ElementType* /*computedTargetElementType*/,
    _Out_ BOOLEAN* actionValidForSourceIndex)
{
    HRESULT hr = S_OK;

    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    *computedTargetIndex = 0;
    *actionValidForSourceIndex = FALSE;

    IFC(get_Orientation(&orientation));

    if (m_nItemsCount > 0 && sourceIndex < m_nItemsCount && m_itemsPerLine > 0)
    {
        if (action == xaml_controls::KeyNavigationAction_First)
        {
            *computedTargetIndex = 0;
            *actionValidForSourceIndex = TRUE;
        }
        else if (action == xaml_controls::KeyNavigationAction_Last)
        {
            *computedTargetIndex = m_nItemsCount - 1;
            *actionValidForSourceIndex = TRUE;
        }
        else
        {
            if (orientation == xaml_controls::Orientation_Horizontal)
            {
                switch(action)
                {
                    case xaml_controls::KeyNavigationAction_Up:
                    {
                        action = xaml_controls::KeyNavigationAction_Left;
                        break;
                    }

                    case xaml_controls::KeyNavigationAction_Down:
                    {
                        action = xaml_controls::KeyNavigationAction_Right;
                        break;
                    }

                    case xaml_controls::KeyNavigationAction_Left:
                    {
                        action = xaml_controls::KeyNavigationAction_Up;
                        break;
                    }

                    case xaml_controls::KeyNavigationAction_Right:
                    {
                        action = xaml_controls::KeyNavigationAction_Down;
                        break;
                    }
                }
            }

            IFC(GetTargetIndexFromNavigationActionInVerticalOrientation(sourceIndex, action, allowWrap, computedTargetIndex, actionValidForSourceIndex));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Private method used by GetTargetIndexFromNavigationAction
// Returns index in direction assuming orientation is Vertical
// In case of Horizontal Orientation, GetTargetIndexFromNavigationAction flips the direction and
// calls this method
_Check_return_ HRESULT WrapGrid::GetTargetIndexFromNavigationActionInVerticalOrientation (
            _In_ UINT sourceIndex,
            _In_ xaml_controls::KeyNavigationAction action,
            _In_ BOOLEAN allowWrap,
            _Out_ UINT* computedTargetIndex,
            _Out_ BOOLEAN* actionValidForSourceIndex)
{
    HRESULT hr = S_OK;
    IFCPTR(actionValidForSourceIndex);
    IFCPTR(computedTargetIndex);

    *computedTargetIndex = 0;
    *actionValidForSourceIndex = FALSE;

    if (action == xaml_controls::KeyNavigationAction_Up)
    {
        if(sourceIndex > 0)
        {
            BOOLEAN isTopBoundary = (sourceIndex < static_cast<INT>(m_itemsPerLine));
            if (!isTopBoundary || allowWrap)
            {
                *computedTargetIndex = sourceIndex - 1;
                *actionValidForSourceIndex = TRUE;
            }
        }
    }
    else if (action == xaml_controls::KeyNavigationAction_Down)
    {
        if(sourceIndex + 1 < m_nItemsCount)
        {
            BOOLEAN isBottomBoundary = (sourceIndex >= static_cast<INT>(m_nItemsCount - m_itemsPerLine));
            if (!isBottomBoundary || allowWrap)
            {
                *computedTargetIndex = sourceIndex + 1;
                *actionValidForSourceIndex = TRUE;
            }
        }
    }
    else if (action == xaml_controls::KeyNavigationAction_Left)
    {
        if(sourceIndex > 0 && sourceIndex >= m_itemsPerLine)
        {
            *computedTargetIndex = sourceIndex - m_itemsPerLine;
            *actionValidForSourceIndex = TRUE;
        }
    }
    else if (action == xaml_controls::KeyNavigationAction_Right)
    {
        if(sourceIndex + m_itemsPerLine < m_nItemsCount)
        {
            *computedTargetIndex = sourceIndex + m_itemsPerLine;
            *actionValidForSourceIndex = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Logical Orientation override
_Check_return_ HRESULT WrapGrid::get_LogicalOrientation(
    _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFCPTR(pValue);
    *pValue = orientation;

    IFC(get_Orientation(&orientation));
    *pValue = orientation;

Cleanup:
    RRETURN(hr);
}

// Physical Orientation override
_Check_return_ HRESULT WrapGrid::get_PhysicalOrientation(
     _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFCPTR(pValue);
    *pValue = orientation;

    IFC(get_Orientation(&orientation));
    if(orientation == xaml_controls::Orientation_Vertical)
    {
        *pValue = xaml_controls::Orientation_Horizontal;
    }
    else
    {
        *pValue = xaml_controls::Orientation_Vertical;
    }

Cleanup:
    RRETURN(hr);
}

// Get the closest element information to the point.
_Check_return_ HRESULT WrapGrid::GetClosestElementInfo(
    _In_ wf::Point position,
    _Out_ xaml_primitives::ElementInfo* returnValue)
{
    HRESULT hr = S_OK;

    IFC(WrapGridGenerated::GetClosestElementInfo(position, returnValue));
    IFC(GetClosestOrInsertionIndex(position, FALSE, &returnValue->m_childIndex));

Cleanup:
    RRETURN(hr);
}

// Get the index where an item should be inserted if it were dropped at
// the given position.  This will be used by live reordering.
_Check_return_ HRESULT WrapGrid::GetInsertionIndex(
    _In_ wf::Point position,
    _Out_ INT* returnValue)
{
    HRESULT hr = S_OK;

    IFC(WrapGridGenerated::GetInsertionIndex(position, returnValue));
    IFC(GetClosestOrInsertionIndex(position, TRUE, returnValue));

Cleanup:
    RRETURN(hr);
}

 // Gets a series of BOOLEAN values indicating whether a given index is
// positioned on the leftmost, topmost, rightmost, or bottommost
// edges of the layout.  This can be useful for both determining whether
// to tilt items at the edges of rows or columns as well as providing
// data for portal animations.
_Check_return_ HRESULT WrapGrid::IsLayoutBoundary(
    _In_ INT index,
    _Out_ BOOLEAN* pIsLeftBoundary,
    _Out_ BOOLEAN* pIsTopBoundary,
    _Out_ BOOLEAN* pIsRightBoundary,
    _Out_ BOOLEAN* pIsBottomBoundary)
{
    HRESULT hr = S_OK;
    bool isHorizontal = true;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFC(WrapGridGenerated::IsLayoutBoundary(index, pIsLeftBoundary, pIsTopBoundary, pIsRightBoundary, pIsBottomBoundary));

    IFC(get_PhysicalOrientation(&orientation));
    isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    if(m_itemsPerLine == 1)
    {
        // ComputeLayoutBoundary computation is same for VSP and WrapGrid when there is only 1 item per line
        IFC(VirtualizingStackPanel::ComputeLayoutBoundary(
            index,
            m_nItemsCount,
            isHorizontal,
            pIsLeftBoundary,
            pIsTopBoundary,
            pIsRightBoundary,
            pIsBottomBoundary));
    }
    else
    {
        if(isHorizontal)
        {
            *pIsLeftBoundary = (index < static_cast<INT>(m_itemsPerLine));
            *pIsBottomBoundary = ((index + 1) % m_itemsPerLine == 0) || (index + 1 == m_nItemsCount);
            *pIsTopBoundary = (index % m_itemsPerLine == 0);
            *pIsRightBoundary = (index >= static_cast<INT>(m_nItemsCount - m_itemsPerLine));
        }
        else
        {
            *pIsLeftBoundary = (index % m_itemsPerLine == 0);
            *pIsBottomBoundary = (index >= static_cast<INT>(m_nItemsCount - m_itemsPerLine));
            *pIsTopBoundary = (index < static_cast<INT>(m_itemsPerLine));
            *pIsRightBoundary = ((index + 1) % m_itemsPerLine == 0) || (index + 1 == m_nItemsCount);
        }
    }

Cleanup:
    RRETURN(hr);
}

// This method is used for getting closest as well as Insertion Index
_Check_return_ HRESULT WrapGrid::GetClosestOrInsertionIndex(
    _In_ wf::Point position,
    _In_ bool isInsertionIndex,
    _Out_ INT* returnValue)
{
    BOOLEAN isHorizontal = TRUE;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    wf::Point arrangeOffset = {0.0, 0.0};
    INT xIndex = 0; // Item Index Horizontally
    INT yIndex = 0; // ItemIndex vertically
    INT xItemsCount = 0; // itemsCount horizontally
    INT yItemsCount = 0; // itemsCount vertically
    wf::Point calculatedPosition = { 0.0, 0.0}; // calculated positon
    wf::Size itemSize = {0, 0}; // itemSize which include Justification wf::Size
    DOUBLE firstItemOffset = 0.0;
    INT visibleStartIndex = 0;
    INT insertionOrClosestIndex = 0;
    ctl::ComPtr<IScrollOwner> spOwner;

    IFC_RETURN(m_ScrollData.get_ScrollOwner(&spOwner));

    *returnValue = -1;
    // Make sure we don't use DoubleUtil::NaN in our calculations.
    if (DoubleUtil::IsNaN(m_itemSize.Width) || DoubleUtil::IsNaN(m_itemSize.Height))
    {
        return S_OK;
    }

    // Get Orientation
    IFC_RETURN(get_PhysicalOrientation(&orientation));
    isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    // Take in to account DM's offset.
    DOUBLE offset = 0.0;
    FLOAT zoomFactor = 1.0;
    IFC_RETURN(ComputePixelOffset(isHorizontal, offset));
    IFC_RETURN(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    // ComputePixelOffset multiples by the zoom factor as a last step. We don't want that, so undo it here.
    offset /= zoomFactor;

    if (isHorizontal)
    {
        position.X -= (FLOAT)offset;
    }
    else
    {
        position.Y -= (FLOAT)offset;
    }

    xItemsCount = isHorizontal ?  m_lineCount : m_itemsPerLine; // itemsCount horizontally
    yItemsCount = isHorizontal ?  m_itemsPerLine : m_lineCount; // itemsCount vertically

    // Calculate visibleStartIndex
    visibleStartIndex = MAX(-1, ComputeIndexOfFirstVisibleItem(isHorizontal, firstItemOffset) * static_cast<INT>(m_itemsPerLine));

    // Calculate ArrangeOffset
    if(isHorizontal)
    {
        arrangeOffset.X = static_cast<FLOAT>(firstItemOffset * m_itemSize.Width);
    }
    else
    {
        arrangeOffset.Y = static_cast<FLOAT>(firstItemOffset * m_itemSize.Height);
    }

    // Calculate ItemIndex Horizontally
    // ItemSize will be JustificationWidth/2 + itemWidth + JustificationWidth/2
    itemSize.Width = m_itemSize.Width + m_justificationSize.Width;

    // Arrange Offset gets added to positon so Position gets shifted accordingly
    // Also StartingSize and JustificationWidth/2 is added to the Position
    calculatedPosition.X = position.X - m_startingSize.Width - m_justificationSize.Width/2 + arrangeOffset.X;

    // If the original position is lessthen StartingSize or justificationSize the xIndex will remain 0
    if(position.X > m_startingSize.Width && position.X > m_justificationSize.Width)
    {
        xIndex = static_cast<INT>(DoubleUtil::Floor(calculatedPosition.X / itemSize.Width));
        // if xIndex is > MaxItems Horizontally, restrict it to xItemsCount -1
        // This happens in case of Left Alignment and there is extra space on the right.
        if(xIndex >= xItemsCount)
        {
            xIndex = xItemsCount - 1;
        }
    }

    // Calculate ItemIndex Vertically
    itemSize.Height = m_itemSize.Height + m_justificationSize.Height;
    calculatedPosition.Y = position.Y - m_startingSize.Height - m_justificationSize.Height/2 + arrangeOffset.Y;
    if(position.Y > m_startingSize.Height && position.Y > m_justificationSize.Height)
    {
        yIndex = static_cast<INT>(DoubleUtil::Floor(calculatedPosition.Y / itemSize.Height));
        // if yIndex is > MaxItems Vertically, restrict it to yItemsCount -1
        // This happens in case of Top Alignment and there is extra space on the Bottom.
        if(yIndex >= yItemsCount)
        {
            yIndex = yItemsCount - 1;
        }
    }

    // In case of insertion Index, if the position is on the bottom half of the item
    // we select next Index as InsertionIndex
    if(isInsertionIndex)
    {
        // Horizontal Index increment needed only in case of Vertical Physical Orientation
        if(!isHorizontal &&
            calculatedPosition.X - xIndex * itemSize.Width > itemSize.Width/2)
        {
            ++xIndex;
        }

        if(isHorizontal &&
            calculatedPosition.Y - yIndex * itemSize.Height > itemSize.Height/2)
        {
            ++yIndex;
        }
    }

    if(isHorizontal)
    {
        // Calculate the Actual Index from Realized Children
        insertionOrClosestIndex = visibleStartIndex + xIndex * m_itemsPerLine + yIndex;
    }
    else
    {
        // Calculate the Actual Index from Realized Children
        insertionOrClosestIndex = visibleStartIndex + yIndex * m_itemsPerLine + xIndex;
    }

    // m_nItemsCount check is needed for the case where m_nItemsCount %  m_itemsPerLine > 0
    // (last Line contains < m_itemsPerLine)
    if(insertionOrClosestIndex >= static_cast<INT>(m_nItemsCount))
    {
        insertionOrClosestIndex = isInsertionIndex ? m_nItemsCount : m_nItemsCount -1;
    }

    *returnValue = insertionOrClosestIndex;

    return S_OK;
}

// Scroll the given ItemIndex into the view
// whem m_itemsPerLine > 1, to view the given Index into view, we need to find the line Index and scroll into that line
_Check_return_ HRESULT WrapGrid::ScrollIntoView(
    _In_ UINT index,
    _In_ BOOLEAN isGroupItemIndex,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;

    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    BOOLEAN bHorizontal = FALSE;
    DOUBLE lineIndex = 0;
    DOUBLE verticalOffset = 0;
    DOUBLE viewportHeight = 0;
    DOUBLE viewportWidth = 0;
    DOUBLE horizontalOffset = 0;
    DOUBLE groupIndex = index;

    if(!isGroupItemIndex)
    {
        IFC(GetIndexInGroupView(index, groupIndex));
    }

    // Get the scroll Owner
    IFC(m_ScrollData.get_ScrollOwner(&spOwner));

    // Get Orientation
    IFC(get_PhysicalOrientation(&orientation));
    bHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    // find lineIndex
    lineIndex = groupIndex/m_itemsPerLine;

    // if GroupIndex is equal to index, which means there is no grouping or groups are with 1 item only.
    // In this case we don't need fractional part
    // Fractional part is required only for grouping scenario when bringing group into view is still not enough
    // since some items may still outside viewport.
    if(DoubleUtil::AreClose(groupIndex, index))
    {
        lineIndex = DoubleUtil::Floor(groupIndex/m_itemsPerLine);
    }

    if(!bHorizontal)
    {
        switch (alignment)
        {
        case xaml_controls::ScrollIntoViewAlignment_Leading:
            {
                IFC(spOwner->ScrollToVerticalOffsetImpl(lineIndex));
                break;
            }

        case xaml_controls::ScrollIntoViewAlignment_Default:
        default:
            {
                IFC(get_VerticalOffset(&verticalOffset));
                IFC(get_ViewportHeight(&viewportHeight));
                // If scrollIndex is above the Vertical offset, bring it into view
                if (verticalOffset - lineIndex > 0 || viewportHeight < 1)
                {
                    IFC(spOwner->ScrollToVerticalOffsetImpl(lineIndex));
                }
                // if ScrollIndex is below the viewport Size
                else if(verticalOffset + viewportHeight < lineIndex + 1)
                {
                    IFC(spOwner->ScrollToVerticalOffsetImpl(lineIndex - viewportHeight + 1));
                }
                break;
            }
        }
    }
    else
    {
        switch (alignment)
        {
        case xaml_controls::ScrollIntoViewAlignment_Leading:
            {
                IFC(spOwner->ScrollToHorizontalOffsetImpl(lineIndex));
                break;
            }

        case xaml_controls::ScrollIntoViewAlignment_Default:
        default:
            {
                // Scrolling into Virtualized Dimension
                IFC(get_HorizontalOffset(&horizontalOffset));
                IFC(get_ViewportWidth(&viewportWidth));

                // If ScrollIndex is on the left side of current offset, bring it into view
                if (horizontalOffset - lineIndex > 0 || viewportWidth < 1)
                {
                    IFC(spOwner->ScrollToHorizontalOffsetImpl(lineIndex));
                }
                // if Item is not inside the current offset, then only bring it into the view
                else if(horizontalOffset + viewportWidth < lineIndex + 1)
                {
                    IFC(spOwner->ScrollToHorizontalOffsetImpl(lineIndex - viewportWidth + 1));
                }
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Computes the total dimension of all realized children
// If the focusItem is not part of visibleItems, BeforeTrail or AfterTrail, the separate
// column gets assigned for focusItem
_Check_return_
HRESULT
WrapGrid::ComputeTotalRealizedChildrenDimension(
    _Out_ FLOAT& cumulatedChildDim, _Out_ UINT& nlineCount)
{
    HRESULT hr = S_OK;
    UINT offsetItemCount = 0;
    UINT itemCount = 0;

    nlineCount = 0;
    cumulatedChildDim = 0.0;

    itemCount = m_iVisibleCount + m_iBeforeTrail;

    // when the last line has less items than m_itemsPerLine, cumulatedChildDim needs to be adjusted.
    offsetItemCount = itemCount % m_itemsPerLine;

    if (itemCount > 0 &&
        !DoubleUtil::IsNaN(m_itemSize.Width) &&
        !DoubleUtil::IsNaN(m_itemSize.Width))
    {
        xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
        DOUBLE itemSize = 0;

        IFC(get_PhysicalOrientation(&orientation));
        itemSize = orientation == xaml_controls::Orientation_Horizontal ? m_itemSize.Width : m_itemSize.Height;

        cumulatedChildDim = static_cast<FLOAT>(itemCount * itemSize);
        // if the last line has less items than m_itemsPerCount, add the dimension for those items in cumulatedChildDim
        if(offsetItemCount > 0)
        {
            cumulatedChildDim += static_cast<FLOAT>((m_itemsPerLine - offsetItemCount) * itemSize);
        }
    }

    // use ceil to get line count, when last line has less items, it should return correct nLineCount
    nlineCount = static_cast<XUINT32>(ceil(static_cast<XFLOAT>(itemCount) / static_cast<XFLOAT>(m_itemsPerLine)));
    cumulatedChildDim = cumulatedChildDim / m_itemsPerLine;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT WrapGrid::MeasureChildForItemsChanged(
    _In_ xaml::IUIElement* pChild)
{
    HRESULT hr = S_OK;
    wf::Size desiredSize = {};

    // If we have atleast 1 item ever measured we dont need to measure again in this case.
    if (m_iVisibleCount > 0)
    {
        goto Cleanup;
    }

    IFC(MeasureChild(pChild, m_LastSetChildLayoutSlotSize, &desiredSize));

Cleanup:
    RRETURN(hr);
}

// compute the visible item count
_Check_return_ HRESULT WrapGrid::ComputeVisibleCount(
    _In_ XUINT32 realizedLineCount)
{
    // Calculate VisibleCount
    m_iVisibleCount = realizedLineCount * m_itemsPerLine - m_iBeforeTrail;
    if(m_iVisibleStart * m_itemsPerLine + m_iVisibleCount > m_nItemsCount)
    {
        m_iVisibleCount = m_nItemsCount - m_iVisibleStart * m_itemsPerLine;
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT WrapGrid::ComputeLayoutSlotSize(
    _In_ wf::Size availableSize,
    _In_ bool isHorizontal,
    _In_ bool isScrolling)
{
    HRESULT hr = S_OK;
    wf::Size layoutSlotSize = availableSize;
    DOUBLE itemWidth = 0.0;
    DOUBLE itemHeight = 0.0;

    //
    // Initialize child sizing and iterator data
    // Allow children as much size as they want along the stack,
    // limited by ItemWidth and ItemHeight if these are set.
    //
    IFC(get_ItemWidth(&itemWidth));
    IFC(get_ItemHeight(&itemHeight));

    if (DoubleUtil::IsNaN(itemWidth))
    {
        if (isHorizontal)
        {
            layoutSlotSize.Width = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
        }
        else if (isScrolling)
        {
            BOOLEAN canHorizontallyScroll = FALSE;
            IFC(get_CanHorizontallyScroll(&canHorizontallyScroll));
            if (canHorizontallyScroll)
            {
                layoutSlotSize.Width = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
            }
        }
    }
    else
    {
        layoutSlotSize.Width = static_cast<FLOAT>(itemWidth);
    }

    if (DoubleUtil::IsNaN(itemHeight))
    {
        if (!isHorizontal)
        {
            layoutSlotSize.Height = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
        }
        else if (isScrolling)
        {
            BOOLEAN canVerticallyScroll = FALSE;
            IFC(get_CanVerticallyScroll(&canVerticallyScroll));
            if (canVerticallyScroll)
            {
                layoutSlotSize.Height = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
            }
        }
    }
    else
    {
        layoutSlotSize.Height = static_cast<FLOAT>(itemHeight);
    }

    m_LastSetChildLayoutSlotSize = layoutSlotSize;

Cleanup:
    RRETURN(hr);
}

// Called when the Items collection associated with the containing ItemsControl changes.
IFACEMETHODIMP
WrapGrid::OnItemsChanged(
    _In_ IInspectable* sender,
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    IFC_RETURN(WrapGridGenerated::OnItemsChanged(sender, args));

    int intAction;
    IFC_RETURN(args->get_Action(&intAction));
    const auto action = static_cast<wfc::CollectionChange>(intAction);

    switch (action)
    {
        case wfc::CollectionChange_Reset:
            m_bShouldComputeItemSize = TRUE;
            break;
    }

    return S_OK;
}

// This method computes total number of visible columns/lines on screen as well as the LastViewPort on screen
// It also calculates how many pixels the last item is getting cut from the screen
_Check_return_ HRESULT WrapGrid::ComputeVisibleLinesAndLastViewPort(
    _In_ wf::Size availableSize,
    _In_ DOUBLE firstItemOffset,
    _In_ bool isHorizontal,
    _In_ XUINT32 visibleLines,
    _Out_ XUINT32 *pVisibleLines,
    _Out_ wf::Size *pVisibleItemsSize,
    _Out_ INT *pLastViewPort,
    _Out_ DOUBLE *pLogicalVisibleSpace)
{
    // Make sure we don't use DoubleUtil::NaN in our calculations.
    ASSERT(!DoubleUtil::IsNaN(m_itemSize.Width));
    ASSERT(!DoubleUtil::IsNaN(m_itemSize.Height));

    IFCPTR_RETURN(pVisibleLines);
    IFCPTR_RETURN(pLastViewPort);
    IFCPTR_RETURN(pLogicalVisibleSpace);

    bool isMoreItems = false;
    IFC_RETURN(AreCurrentVisibleItemsAreMoreThanFirstMeasureVisibleItems(availableSize, firstItemOffset, isHorizontal, visibleLines, &isMoreItems));
    if (isMoreItems)
    {
        visibleLines += 1;
    }

    // If we are starting at an offset such that we will not have enough items to show in viewport
    // we need to adjust the offset and visible start to cover the viewport completely.
    // This is achieved by FillRemainingSpace in VSP.
    if (visibleLines + m_iVisibleStart > m_lineCount)
    {
        FLOAT numLinesViewPortCanShow = 0;
        if (isHorizontal)
        {
            if (m_itemSize.Width > 0)
            {
                numLinesViewPortCanShow = availableSize.Width / m_itemSize.Width;
            }
            IFC_RETURN(m_ScrollData.put_OffsetX(MAX(m_lineCount - numLinesViewPortCanShow, 0)));
            firstItemOffset = DoubleUtil::Fractional(m_ScrollData.get_OffsetX());
        }
        else
        {
            if (m_itemSize.Height > 0)
            {
                numLinesViewPortCanShow = availableSize.Height / m_itemSize.Height;
            }
            IFC_RETURN(m_ScrollData.put_OffsetY(MAX(m_lineCount - numLinesViewPortCanShow, 0)));
            firstItemOffset = DoubleUtil::Fractional(m_ScrollData.get_OffsetY());
        }

        visibleLines = MIN(static_cast<XUINT32>(DoubleUtil::Ceil(numLinesViewPortCanShow)), m_lineCount);
        // When empty, m_iVisibleStart is already -1. This just ensures that we don't accidentally clobber that value
        m_iVisibleStart = m_nItemsCount > 0 ? m_lineCount - visibleLines : -1;
    }

    *pLastViewPort = m_iVisibleStart + visibleLines -1;
    if (isHorizontal)
    {
        *pLogicalVisibleSpace = availableSize.Width - ((1 - firstItemOffset) * m_itemSize.Width + (visibleLines - 1) * m_itemSize.Width);
    }
    else
    {
        *pLogicalVisibleSpace = availableSize.Height - ((1 - firstItemOffset) * m_itemSize.Height + (visibleLines - 1) * m_itemSize.Height);
    }

    IFC_RETURN(ComputeVisibleCount(visibleLines));

    if (isHorizontal)
    {
        pVisibleItemsSize->Width = m_itemSize.Width * visibleLines;
        ASSERT(m_iVisibleCount >= 0);
        pVisibleItemsSize->Height = m_itemSize.Height * (visibleLines == 1 ? static_cast<float>(m_iVisibleCount) : m_itemsPerLine);
    }
    else
    {
        pVisibleItemsSize->Height = m_itemSize.Height * visibleLines;
        ASSERT(m_iVisibleCount >= 0);
        pVisibleItemsSize->Width = m_itemSize.Width * (visibleLines == 1 ? static_cast<float>(m_iVisibleCount) : m_itemsPerLine);
    }
    *pVisibleLines = visibleLines;

    return S_OK;
}

// Check whether the number of lines/columns in current visible space is more than first Measure
// for example, if in first measure total columns visible on screen are 5 and after some scrolling,
// total items visible on screen are 6
_Check_return_ HRESULT WrapGrid::AreCurrentVisibleItemsAreMoreThanFirstMeasureVisibleItems(
    _In_ wf::Size availableSize,
    _In_ DOUBLE firstItemOffset,
    _In_ bool isHorizontal,
    _In_ UINT32 visibleLineInViewport,
    _Out_ bool *pIsMore)
{
    HRESULT hr = S_OK;

    // Make sure we don't use DoubleUtil::NaN in our calculations.
    ASSERT(!DoubleUtil::IsNaN(m_itemSize.Width));
    ASSERT(!DoubleUtil::IsNaN(m_itemSize.Height));

    IFCPTR(pIsMore);
    *pIsMore = FALSE;

    if (firstItemOffset > 0)
    {
        if (isHorizontal)
        {
            // if first Item has an Offset, update visible lines count
            XUINT32 firstMeasureLastItemVisibleWidth = static_cast<UINT>(availableSize.Width - (visibleLineInViewport - 1) * m_itemSize.Width);
            XUINT32 firstItemVisibleWidth = static_cast<UINT>((1 - firstItemOffset) * m_itemSize.Width);
            if(firstItemVisibleWidth < firstMeasureLastItemVisibleWidth)
            {
                *pIsMore = TRUE;
            }
        }
        else
        {
            XUINT32 firstMeasureLastItemVisibleHeight = static_cast<UINT>(availableSize.Height - (visibleLineInViewport - 1) * m_itemSize.Height);
            XUINT32 firstItemVisibleHeight = static_cast<UINT>((1 - firstItemOffset) * m_itemSize.Height);
            if (firstItemVisibleHeight < firstMeasureLastItemVisibleHeight)
            {
                *pIsMore = TRUE;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculates items per page based on the current viewport size.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WrapGrid::GetItemsPerPage(
    _In_ IScrollInfo* pScrollInfo,
    _Out_ DOUBLE* pItemsPerPage)
{
    HRESULT hr = S_OK;
    DOUBLE itemWidth = 0.0;
    DOUBLE itemHeight = 0.0;

    *pItemsPerPage = 0;

    // If we've calculated m_itemSize, we should use this.
    if (!DoubleUtil::IsNaN(m_itemSize.Width) &&
        !DoubleUtil::IsNaN(m_itemSize.Height))
    {
        itemWidth = m_itemSize.Width;
        itemHeight = m_itemSize.Height;
    }
    else
    {
        // Otherwise if the customer has specified ItemWidth and ItemHeight, use these.
        IFC(get_ItemWidth(&itemWidth));
        IFC(get_ItemHeight(&itemHeight));

        if (DoubleUtil::IsNaN(itemWidth) ||
            DoubleUtil::IsNaN(itemHeight))
        {
            goto Cleanup;
        }
    }

    if (itemWidth > 0 &&
        itemHeight > 0)
    {
        ASSERT(m_lastArrangeSize.Width >= 0.0);
        ASSERT(m_lastArrangeSize.Height >= 0.0);

        INT rows = static_cast<INT>(DoubleUtil::Ceil(m_lastArrangeSize.Height / itemHeight));
        INT cols = static_cast<INT>(DoubleUtil::Ceil(m_lastArrangeSize.Width / itemWidth));
        *pItemsPerPage = rows * cols;
    }

Cleanup:
    RRETURN(hr);
}

