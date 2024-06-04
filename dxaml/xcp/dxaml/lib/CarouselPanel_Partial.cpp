// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CarouselPanel.g.h"
#include "ListViewBase.g.h"
#include "ComboBox.g.h"
#include "ItemContainerGenerator.g.h"
#include "ScrollContentPresenter.g.h"
#include "IScrollOwner.h"

using namespace DirectUI;

CarouselPanel::CarouselPanel():
        m_VirtualizationMode(xaml_controls::VirtualizationMode_Standard),
        m_iVisibleStart(-1),
        m_iVisibleCount(0),
        m_nItemsCount(0),
        m_iFirstVisibleChildIndex(-1),
        m_iBeforeTrail(0),
        m_iAfterTrail(0),
        m_dPrecacheWindowSize(250),
        m_dPrecacheBeforeTrailSize(0),
        m_dPrecacheAfterTrailSize(0),
        m_minimumDesiredWindowWidth(0),
        m_bItemBasedScrolling(FALSE),
        m_pTranslatedOffsetState(NULL),
        m_bIsVirtualizing(FALSE),
        m_bInMeasure(FALSE),
        m_bNotifyHorizontalSnapPointsChanges(FALSE),
        m_bNotifyVerticalSnapPointsChanges(FALSE),
        m_bNotifiedHorizontalSnapPointsChanges(FALSE),
        m_bNotifiedVerticalSnapPointsChanges(FALSE),
        m_bAreSnapPointsKeysHorizontal(FALSE),
        m_lowerMarginSnapPointKey(0.0),
        //m_upperMarginSnapPointKey(0.0), Use once horizontal carousel is enabled
        m_irregularSnapPointKeysOffset(0.0),
        m_regularSnapPointKey(0.0),
        m_pIrregularSnapPointKeys(NULL),
        m_cIrregularSnapPointKeys(0),
        m_iCurrentLoop(m_CarouselOffsetStart),
        m_InternalOffset(-1),
        m_bFirstMeasureHasBeenCalled(FALSE),
        m_iNullItemPosition(-1),
        m_SizeOfVisualsInPopup(0.0),
        m_bInManipulation(FALSE),
        m_bNotifyLayoutRefresh(FALSE),
        m_bShouldCarousel(FALSE),
        m_LastComputedPixelExtent(0.0),
        m_LastComputedPixelOffset(0.0),
        m_fZoomFactor(1)
{
}

CarouselPanel::~CarouselPanel()
{
    delete m_pTranslatedOffsetState;
    m_pTranslatedOffsetState = NULL;
    delete [] m_pIrregularSnapPointKeys;
}

// Supports the IManipulationDataProvider interface.
_Check_return_ HRESULT
CarouselPanel::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IManipulationDataProvider)))
    {
        *ppObject = static_cast<IManipulationDataProvider*>(this);
    }
    else
    {
        RRETURN(CarouselPanelGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Provides the behavior for the Arrange pass of layout.  Classes
// can override this method to define their own Arrange pass
// behavior.
IFACEMETHODIMP CarouselPanel::ArrangeOverride(
    // The computed size that is used to arrange the content.
    _In_ wf::Size arrangeSize,
    // The size of the control.
    _Out_ wf::Size* returnValue)
{
    wf::Rect rcChild = {0, 0, arrangeSize.Width, arrangeSize.Height};
    DOUBLE previousChildSize = 0.0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml::IUIElement> spChild;
    DOUBLE dimension = 0.0;
    wf::Size childSize = {};
    FLOAT zoomFactor = 1.0;

    auto guard = wil::scope_exit([this]()
    {
        // a scroll will trigger layout which will trigger transitions
        // that scroll has now been processed, so new transitions are allowed to be created
        VERIFYHR(put_IsIgnoringTransitions(FALSE));
    });

    m_ScrollData.m_ArrangedOffset = m_ScrollData.m_ComputedOffset;
    IFC_RETURN(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    IFC_RETURN(ComputePixelOffsetOfChildAtIndex(m_iVisibleStart, dimension));
    for(INT i = 0; i < m_iBeforeTrail; i++)
    {
        IFC_RETURN(m_tpOrderedChildrenList->GetAt(i, &spChild));
        IFC_RETURN(spChild->get_DesiredSize(&childSize));
        dimension -= childSize.Height;
    }
    rcChild.Y = static_cast<FLOAT>(dimension);

    //
    // Arrange and Position Children.
    //
#if DBG
    IFC_RETURN(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    // Loop through ordered list to arrange items.
    BOOLEAN bNullItemArranged = FALSE;
    UINT nCount = 0;
    BOOLEAN isIgnoring = FALSE; // trying to keep logic as much the same as other panels to make merging easier

    IFC_RETURN(get_IsIgnoringTransitions(&isIgnoring));
    if (!isIgnoring && m_bInManipulation)
    {
        // make sure not to setup transitions during DM
        IFC_RETURN(put_IsIgnoringTransitions(TRUE));
        isIgnoring = TRUE;
    }

    IFC_RETURN(m_tpOrderedChildrenList->get_Size(&nCount));
    for (UINT i = 0; i < nCount; )
    {
        if (m_bShouldCarousel && !bNullItemArranged && m_iNullItemPosition == i)
        {
            bNullItemArranged = TRUE;
            if (i == 0) // For the case Separator item is 1st to be arranged.
            {
                IFC_RETURN(GetEstimatedRealizedChildSize(childSize));
                rcChild.Y += childSize.Height;
            }
            else
            {
                rcChild.Y += static_cast<FLOAT>(previousChildSize);
            }
        }
        else
        {
            // We loop through ordered child list vs realized child list as VSP does
            // because ordered child list has the correct layout order.
            IFC_RETURN(m_tpOrderedChildrenList->GetAt(i, &spChild));
            IFC_RETURN(spChild->get_DesiredSize(&childSize));

            rcChild.Y += static_cast<FLOAT>(previousChildSize);
            previousChildSize = childSize.Height;
            rcChild.Height = static_cast<FLOAT>(previousChildSize);
            rcChild.Width = static_cast<FLOAT>(DoubleUtil::Max(arrangeSize.Width, childSize.Width));

            IFC_RETURN(spChild->Arrange(rcChild));
            ++i;
        }
    }

    rcChild.Y += static_cast<FLOAT>(previousChildSize);

    // Layout focused items in the end to avoid overlay.
    IFC_RETURN(EnsureRealizedItemsNotPartOfOderedListAreNotInVisibleArea(rcChild));

    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));
    // Snap point might have changed, which might require to raise an event.
    IFC_RETURN(NotifySnapPointsChanges(spRealizedChildren.Get(), nCount));

    IFC_RETURN(SetupItemBoundsClip());

    if (!m_bShouldMeasureBuffers)
    {
        IFC_RETURN(SetFillBuffersTimer());
    }

    if (m_bNotifyLayoutRefresh)
    {
        ctl::ComPtr<IScrollOwner> spOwner;

        IFC_RETURN(m_ScrollData.get_ScrollOwner(&spOwner));
        m_bNotifyLayoutRefresh = FALSE;

        if (spOwner)
        {
            IFC_RETURN(spOwner->NotifyLayoutRefreshed());
        }
    }

    *returnValue = arrangeSize;

    return S_OK;
}

// Arrange all realized items which are not part of ordered list out of viewable area.
_Check_return_ HRESULT CarouselPanel::EnsureRealizedItemsNotPartOfOderedListAreNotInVisibleArea(
    _In_ wf::Rect rcChild)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spUIChild;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    UINT nOrderedCount = 0;
    UINT nRealizedCount = 0;
    INT itemIndex = 0;
    wf::Size childSize = {};

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nRealizedCount));
    IFC(m_tpOrderedChildrenList->get_Size(&nOrderedCount));
    if (nOrderedCount >= nRealizedCount)
    {
        // All realized children are part of ordered children in visible region.
        goto Cleanup;
    }

    rcChild.X -= rcChild.Width;

    for(UINT childIndex = 0; childIndex < nRealizedCount; childIndex++)
    {
        IFC(spRealizedChildren->GetAt(childIndex, &spUIChild));
        IFC(GetGeneratedIndex(childIndex, itemIndex));

        if (IsOutOfVisibleRange(itemIndex)) // Arrange this element out of viewable region
        {
            IFC(spUIChild->get_DesiredSize(&childSize));
            rcChild.X -= childSize.Width -1;
            IFC(spUIChild->Arrange(rcChild));
        }
    }

Cleanup:
    RRETURN(hr);

}

// Measure
// Cleanup not required containers from pRealized
// Clear ordered list
// Generate and Measure items in viewport
// If there are still items left for generation
//  - Generate after cache
//  - Generate before cache
// Cleanup and recycle not required containers.
IFACEMETHODIMP CarouselPanel::MeasureOverride(
    // Measurement constraints, a control cannot return a size larger than the
    // constraint.
    _In_ wf::Size availableSize,
    // The desired size of the control.
    _Out_ wf::Size* returnValue)
{
    wf::Size stackDesiredSize = {};
    wf::Size layoutSlotSize = availableSize;
    DOUBLE firstItemOffset = 0.0;       // Offset of the top of the first child relative to the top of the viewport.
    INT lastViewport = -1;            // Last child index in the viewport.  -1 indicates we have not yet iterated through the last child.
    DOUBLE logicalVisibleSpace = 0;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    FLOAT zoomFactor = 1.0;
    INT maxNumOfItemsToGenerateInCurrentMeasureCycle;
    BOOLEAN bIsInDMZoom = FALSE;
    BOOLEAN isScrolling = FALSE;

    auto guard = wil::scope_exit([this]()
    {
        m_bInMeasure = FALSE;
    });

    m_bInMeasure = TRUE;

    if(!m_bFirstMeasureHasBeenCalled)
    {
        m_bFirstMeasureHasBeenCalled = TRUE;
        m_SizeOfVisualsInPopup = m_InitialMeasureHeight - availableSize.Height;
    }

    // Collect information from the ItemsControl, if there is one.
    IFC_RETURN(ItemsControl::GetItemsOwner(this, &spItemsControl));

    if (spItemsControl)
    {
        UINT itemsCount = 0;
        IFC_RETURN(get_ItemContainerGenerator(&spGenerator));
        IFC_RETURN(spGenerator.Cast<ItemContainerGenerator>()->get_View(&spItems));
        IFC_RETURN(spItems->get_Size(&itemsCount));
        m_nItemsCount = itemsCount;

        IFC_RETURN(get_IsScrolling(&isScrolling));
        if (ctl::is<xaml_controls::IComboBox>(spItemsControl))
        {
            m_bShouldCarousel = isScrolling && spItemsControl.Cast<ComboBox>()->GetShouldCarousel();
        }

        if (m_bShouldCarousel)
        {
            // increase count for separator item
            m_nItemsCount++;
        }
        else
        {
            m_InternalOffset = m_ScrollData.get_OffsetY();
        }
    }
    else
    {
        m_nItemsCount = 0;
    }

    IFC_RETURN(SetVirtualizationState(spItemsControl.Get()));
    IFC_RETURN(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    //
    // Initialize child sizing and iterator data
    // Allow children as much size as they want along the stack.
    //
    availableSize.Height = availableSize.Height / zoomFactor;
    layoutSlotSize.Height = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
    if (isScrolling)
    {
        BOOLEAN canHorizontallyScroll = FALSE;
        IFC_RETURN(get_CanHorizontallyScroll(&canHorizontallyScroll));
        if (canHorizontallyScroll)
        {
            layoutSlotSize.Width = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
        }
    }
    logicalVisibleSpace = availableSize.Height;

    if (availableSize.Height < m_LastSetAvailableSize.Height || availableSize.Width < m_LastSetAvailableSize.Width)
    {
        // Reset the _maxDesiredSize cache if available size reduces from last available size.
        wf::Size empty = {};
        m_ScrollData.m_MaxDesiredSize = empty;
    }

    m_LastSetAvailableSize = availableSize;
    m_LastSetChildLayoutSlotSize = layoutSlotSize;

    // Compute index of first item in the viewport
    m_iVisibleStart = isScrolling ? ComputeIndexOfFirstVisibleItem(firstItemOffset) : 0;
    m_iNullItemPosition = -1;

    // If recycling clean up before generating children.
    if (m_bIsVirtualizing)
    {
        IFC_RETURN(CleanupContainers(spItemsControl.Get()));
#if DBG
        if (InRecyclingMode())
        {
            IFC_RETURN(debug_VerifyRealizedChildren());
        }
#endif
    }

    //If this measure requires ensuring a particular item is in View, we need to correct offset and m_iVisibleStart
    if (m_IndexToEnsureInView >= 0)
    {
        IFC_RETURN(CorrectOffsetForScrollIntoView(availableSize.Height, FALSE, spItemsControl.Get(), layoutSlotSize, firstItemOffset));
        m_IndexToEnsureInView = -1;
    }

    // Clear ordered list. Should be cleared only after calling Cleanup containers.
    IFC_RETURN(EnsureOrderedList());

    // Restrict number of items being generated to the number of items generated in previous cycle if we are currently zooming.
    IFC_RETURN(IsInDirectManipulationZoom(bIsInDMZoom));
    maxNumOfItemsToGenerateInCurrentMeasureCycle = bIsInDMZoom ? m_iVisibleCount + m_iBeforeTrail : m_nItemsCount;

    //
    // Main loop: generate and measure all children (or all visible children if virtualizing).
    //
    m_iVisibleCount = 0;
    m_iBeforeTrail = 0;
    m_iAfterTrail = 0;
    m_dPrecacheAfterTrailSize = 0;

    INT lastGeneratedChildIndex = 0;
    if (m_nItemsCount > 0)
    {
        IFC_RETURN(GenerateAndMeasureItemsInViewport(
            isScrolling,
            lastViewport,
            firstItemOffset,
            availableSize,
            maxNumOfItemsToGenerateInCurrentMeasureCycle,
            stackDesiredSize,
            layoutSlotSize,
            logicalVisibleSpace,
            lastGeneratedChildIndex));
    }
    else
    {
        ASSERT(!isScrolling, L"CarouselPanel should not be in scrolling mode when items count = 0");
    }

#if DBG
    if (m_bIsVirtualizing && InRecyclingMode())
    {
        IFC_RETURN(debug_VerifyRealizedChildren());
    }
#endif
    //
    // Adjust the scroll offset, extent, etc.
    //
    if (isScrolling)
    {
        // Compute the extent before we fill remaining space and modify the stack desired size
        wf::Size extent = {};
        IFC_RETURN(ComputeLogicalExtent(stackDesiredSize, FALSE, extent));
        INT maxItemsLeftToGenerateOrRealize = 0;
        if (m_iVisibleCount < m_nItemsCount && m_bShouldMeasureBuffers)
        {
            maxItemsLeftToGenerateOrRealize = m_nItemsCount - m_iVisibleCount;
            INT firstUnrealizedContainerIndex = m_iVisibleStart + m_iVisibleCount;   // beforeTrail is not included in m_iVisibleCount
            INT newlyRealizedCount = 0;
            // Realize any items still available in after cache
            if (maxItemsLeftToGenerateOrRealize > 0)
            {
                IFC_RETURN(GenerateAndMeasureItemsInBuffer(
                    firstUnrealizedContainerIndex,
                    m_dPrecacheWindowSize,
                    TRUE,
                    maxItemsLeftToGenerateOrRealize,
                    layoutSlotSize,
                    maxNumOfItemsToGenerateInCurrentMeasureCycle,
                    stackDesiredSize,
                    logicalVisibleSpace,
                    newlyRealizedCount));
            }

            maxItemsLeftToGenerateOrRealize -= newlyRealizedCount;

            // Realize any items still available in before cache
            if (maxItemsLeftToGenerateOrRealize > 0)
            {
                // We generate upto however many generated in after cache in before cache.
                maxItemsLeftToGenerateOrRealize = std::min(maxItemsLeftToGenerateOrRealize, m_iAfterTrail);
                // Generate at least one item in before cache
                maxItemsLeftToGenerateOrRealize = std::max(maxItemsLeftToGenerateOrRealize, 1);

                firstUnrealizedContainerIndex = m_iVisibleStart - maxItemsLeftToGenerateOrRealize;
                IFC_RETURN(GenerateAndMeasureItemsInBuffer(
                    firstUnrealizedContainerIndex,
                    DoubleUtil::PositiveInfinity, // Since we are going to generate forward we cannot rely on space to fit right set of items.
                    FALSE,
                    maxItemsLeftToGenerateOrRealize,
                    layoutSlotSize,
                    maxNumOfItemsToGenerateInCurrentMeasureCycle,
                    stackDesiredSize,
                    logicalVisibleSpace,
                    newlyRealizedCount));
            }
        }

        // Compute Scrolling data such as extent, viewport, and offset.
        IFC_RETURN(UpdateLogicalScrollData(stackDesiredSize, availableSize, logicalVisibleSpace, extent, lastViewport));
    }
    else
    {
        // If the window size changed the panel could have become non-pannable and we should clear out old scroll data.
        IFC_RETURN(ResetScrollData());
        m_iNullItemPosition = -1;
    }

    //
    // Cleanup items no longer in the viewport
    //
    if (m_bIsVirtualizing)
    {
        IFC_RETURN(CleanupContainers(spItemsControl.Get()));

        if (InRecyclingMode())
        {
            IFC_RETURN(DisconnectRecycledContainers());
        }
    }

#if DBG
    IFC_RETURN(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    stackDesiredSize.Height *= zoomFactor;
    *returnValue = stackDesiredSize;

    return S_OK;
}

// Generates items for buffer before or after viewport.
// Starts from given index (firstUnrealizedContainerIndex) and wraps if hits the boundary.
// Limits generation until max number of items are already generated or the allowed buffer space is already used.
_Check_return_ HRESULT CarouselPanel::GenerateAndMeasureItemsInBuffer(
    _In_ INT firstUnrealizedContainerIndex,
    _In_ DOUBLE unusedBuffer,
    _In_ BOOLEAN bIsAfterBuffer,
    _In_ INT maxItemsLeftToGenerateOrRealize,
    _In_ wf::Size layoutSlotSize,
    _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle,
    _Inout_ wf::Size& stackDesiredSize,
    _Inout_ DOUBLE& logicalVisibleSpace,
    _Out_ INT& newRealizedCount)
{
    HRESULT hr = S_OK;
    xaml_primitives::GeneratorPosition startPos = {};
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    wf::Size childDesiredSize = {0,0};
    BOOLEAN visualOrderChanged = FALSE;
    BOOLEAN bNullItemAdded = FALSE;
    INT childIndex = -1;
    UINT nOrderedCount = 0;
    newRealizedCount = 0;

    if (m_bShouldCarousel)
    {
        firstUnrealizedContainerIndex = WrapIndex(firstUnrealizedContainerIndex);
    }
    else if (!bIsAfterBuffer)
    {
        // We might get firtUnrealized as -ve value for buffer before viewport.
        // In non carousel case we want to ensure we generate atleast the items above visible start
        // even if the total number is less than what we would have generated in carousel case.
        // Adjust maxItemsLeftToGenerateOrRealize and firstUnrealizedContainerIndex accordingly for this case here.
        maxItemsLeftToGenerateOrRealize = MIN(maxItemsLeftToGenerateOrRealize, m_iVisibleStart);
        firstUnrealizedContainerIndex = m_iVisibleStart - maxItemsLeftToGenerateOrRealize;
    }

    if (firstUnrealizedContainerIndex < 0 || firstUnrealizedContainerIndex >= m_nItemsCount)
    {
        goto Cleanup;
    }

    IFC(GetEstimatedRealizedChildSize(childDesiredSize)); // If Separator Item is the first item being measured.
    if (firstUnrealizedContainerIndex == m_nItemsCount - 1)
    {
        if (bIsAfterBuffer)
        {
            IFC(m_tpOrderedChildrenList->get_Size(&nOrderedCount));
            m_iNullItemPosition = (INT) nOrderedCount;
            m_iVisibleCount++;
            m_iAfterTrail++;
        }
        else
        {
            m_iNullItemPosition = newRealizedCount;
            m_iBeforeTrail++;
        }

        unusedBuffer -= childDesiredSize.Height;
        newRealizedCount++;
        bNullItemAdded = TRUE;
        firstUnrealizedContainerIndex = 0;
    }

    IFC(IndexToGeneratorPositionForStart(firstUnrealizedContainerIndex, childIndex, startPos));
    if (childIndex >= m_nItemsCount-1)
    {
        childIndex = childIndex % (m_nItemsCount-1);
    }

    IFC(get_ItemContainerGenerator(&spGenerator));
    IFC(spGenerator->StartAt(startPos, xaml_primitives::GeneratorDirection_Forward, TRUE));
    while (DoubleUtil::GreaterThan(unusedBuffer-childDesiredSize.Height,0)
        && newRealizedCount < maxItemsLeftToGenerateOrRealize
        && m_iVisibleCount + m_iBeforeTrail < maxNumOfItemsToGenerateInCurrentMeasureCycle)
    {
        if (firstUnrealizedContainerIndex >= m_nItemsCount)
        {
            firstUnrealizedContainerIndex = 0;
            IFC(spGenerator->Stop());
            INT newChildIndex = -1;
            IFC(IndexToGeneratorPositionForStart(firstUnrealizedContainerIndex, newChildIndex, startPos));
            childIndex = 0; /// start inserting in the beginning.
            IFC(spGenerator->StartAt(startPos, xaml_primitives::GeneratorDirection_Forward, TRUE));
        }

        DOUBLE childLogicalSize = 0.0;
        if (firstUnrealizedContainerIndex == m_nItemsCount-1)
        {
            bNullItemAdded = TRUE;
            if (bIsAfterBuffer)
            {
                IFC(m_tpOrderedChildrenList->get_Size(&nOrderedCount));
                m_iNullItemPosition = (INT) nOrderedCount;
            }
            else
            {
                m_iNullItemPosition = newRealizedCount;
            }
        }
        else
        {
            ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
            ctl::ComPtr<xaml::IUIElement> spUIChild;

            // Get next child.
            BOOLEAN newlyRealized = FALSE;
            IFC(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));

            if (!ctl::is<xaml::IUIElement>(spChildAsDO))
            {
                ASSERT(!newlyRealized, L"The generator realized a null value.");
                spChildAsDO.Reset();
                // We reached the end of the items (because of a group)
                break;
            }

            IFC(spChildAsDO.As(&spUIChild));
            IFC(AddContainerFromGenerator(childIndex++, spUIChild.Get(), newlyRealized, visualOrderChanged));
            IFC(MeasureChild(spUIChild.Get(), layoutSlotSize, &childDesiredSize));

            if (bIsAfterBuffer)
            {
                IFC(m_tpOrderedChildrenList->Append(spUIChild.Get()));
            }
            else
            {
                IFC(m_tpOrderedChildrenList->InsertAt(bNullItemAdded ? newRealizedCount-1:newRealizedCount, spUIChild.Get()));
                if (!bNullItemAdded && m_iNullItemPosition >= 0)
                {
                    m_iNullItemPosition++;
                }
            }
        }

        if (bIsAfterBuffer)
        {
            m_iVisibleCount++;
            m_iAfterTrail++;
        }
        else
        {
            m_iBeforeTrail++;
        }

        newRealizedCount++;
        // Accumulate child size.
        childLogicalSize = childDesiredSize.Height;
        stackDesiredSize.Height += static_cast<FLOAT>(childLogicalSize);

        m_minimumDesiredWindowWidth = DoubleUtil::Max(childDesiredSize.Width, m_minimumDesiredWindowWidth);
        stackDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Width, m_minimumDesiredWindowWidth));

        if (bIsAfterBuffer)
        {
            m_dPrecacheAfterTrailSize += childLogicalSize;
        }
        else
        {
            m_dPrecacheBeforeTrailSize += childLogicalSize;
        }

        unusedBuffer -= childLogicalSize;

        // Loop around and generate another item
        firstUnrealizedContainerIndex++;
    }
    IFC(spGenerator->Stop());

#if DBG
    IFC(debug_CheckRealizedChildrenCount());
#endif

Cleanup:
    RRETURN(hr);
}


// Generates items currently in the viewport.
_Check_return_ HRESULT CarouselPanel::GenerateAndMeasureItemsInViewport(
    _In_ BOOLEAN isScrolling,
    _Inout_ INT& lastViewport,
    _In_ DOUBLE firstItemOffset,
    _In_ wf::Size availableSize,
    _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle,
    _Inout_ wf::Size& stackDesiredSize,
    _In_ wf::Size layoutSlotSize,
    _Inout_ DOUBLE& logicalVisibleSpace,
    _Out_ INT& lastGeneratedChildIndex)
{
    HRESULT hr = S_OK;
    xaml_primitives::GeneratorPosition startPos = {};
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    wf::Size childDesiredSize = {};
    BOOLEAN visualOrderChanged = FALSE;
    INT childIndex = m_iFirstVisibleChildIndex;
    lastGeneratedChildIndex = childIndex;
    INT currentItemIndex = m_iVisibleStart;
    INT numOfItemsGenerated = 0;
    UINT nOrderedCount = 0;

    IFC(GetEstimatedRealizedChildSize(childDesiredSize)); // If Separator Item is the first item being measured.
    if (isScrolling && m_bShouldCarousel && currentItemIndex == m_nItemsCount - 1)
    {
        IFC(m_tpOrderedChildrenList->get_Size(&nOrderedCount));
        m_iNullItemPosition = (INT)nOrderedCount;
        logicalVisibleSpace -= childDesiredSize.Height * (1 - firstItemOffset);
        m_dPrecacheBeforeTrailSize = childDesiredSize.Height * firstItemOffset;
        m_iVisibleCount++;
        numOfItemsGenerated++;
        currentItemIndex = 0;
    }

    if (numOfItemsGenerated == m_nItemsCount || numOfItemsGenerated == maxNumOfItemsToGenerateInCurrentMeasureCycle)
    {
        goto Cleanup;
    }

    IFC(get_ItemContainerGenerator(&spGenerator));
    IFC(IndexToGeneratorPositionForStart(currentItemIndex, childIndex, startPos));
    IFC(spGenerator->StartAt(startPos, xaml_primitives::GeneratorDirection_Forward, TRUE));

    while (numOfItemsGenerated < m_nItemsCount && numOfItemsGenerated < maxNumOfItemsToGenerateInCurrentMeasureCycle)
    {
        if (isScrolling && m_bShouldCarousel && currentItemIndex >= m_nItemsCount)
        {
            currentItemIndex = 0;
            IFC(spGenerator->Stop());
            INT newChildIndex = -1;
            IFC(IndexToGeneratorPositionForStart(currentItemIndex, newChildIndex, startPos));
            childIndex = 0; // start inserting in the beginning.
            IFC(spGenerator->StartAt(startPos, xaml_primitives::GeneratorDirection_Forward, TRUE));
        }

        DOUBLE childLogicalSize = 0.0;
        if (isScrolling && m_bShouldCarousel && currentItemIndex == m_nItemsCount-1)
        {
            IFC(m_tpOrderedChildrenList->get_Size(&nOrderedCount));
            m_iNullItemPosition = (INT)nOrderedCount;
        }
        else
        {
            ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
            ctl::ComPtr<xaml::IUIElement> spUIChild;

            // Get next child.
            BOOLEAN newlyRealized = FALSE;
            IFC(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));

            if (!ctl::is<xaml::IUIElement>(spChildAsDO))
            {
                ASSERT(!newlyRealized, L"The generator realized a null value.");
                spChildAsDO.Reset();
                // We reached the end of the items (because of a group)
                break;
            }

            IFC(spChildAsDO.As(&spUIChild));
            lastGeneratedChildIndex = childIndex;
            IFC(AddContainerFromGenerator(childIndex++, spUIChild.Get(), newlyRealized, visualOrderChanged));
            IFC(MeasureChild(spUIChild.Get(), layoutSlotSize, &childDesiredSize));
            IFC(m_tpOrderedChildrenList->Append(spUIChild.Get()));
        }

        m_iVisibleCount++;
        numOfItemsGenerated++;

        // Accumulate child size.
        if (currentItemIndex == m_iVisibleStart)
        {
            childLogicalSize = childDesiredSize.Height * (1 - firstItemOffset);
            m_dPrecacheBeforeTrailSize = childDesiredSize.Height * firstItemOffset;
        }
        else
        {
            childLogicalSize = childDesiredSize.Height;
        }

        stackDesiredSize.Height += static_cast<FLOAT>(childLogicalSize);
        m_minimumDesiredWindowWidth = DoubleUtil::Max(childDesiredSize.Width, m_minimumDesiredWindowWidth);
        stackDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Width, m_minimumDesiredWindowWidth));

        // Adjust remaining viewport space if we are scrolling and within the viewport region.
        // While scrolling (not virtualizing), we always measure children before and after the viewport.
        if (isScrolling && lastViewport == -1)
        {
            logicalVisibleSpace -= childLogicalSize;
            if (DoubleUtil::LessThanOrClose(logicalVisibleSpace, 0.0))
            {
                lastViewport = currentItemIndex;
                m_dPrecacheAfterTrailSize = -logicalVisibleSpace;
            }
        }

        // When under a viewport, virtualizing and at or beyond the first element, stop creating elements when out of space.
        if (m_bIsVirtualizing)
        {
            if (DoubleUtil::LessThanOrClose(availableSize.Height, stackDesiredSize.Height))
            {
                // Either we passed the limit or the child was focusable

                if (lastViewport != -1 && lastViewport != currentItemIndex)
                {
                    m_iAfterTrail++;
                    m_dPrecacheAfterTrailSize += childLogicalSize;
                }

                // The end of this child is outside the viewport.  Check if we want to generate some more.
                // We should have at least 1 item after the view
                if (m_iAfterTrail > 0 && m_dPrecacheAfterTrailSize >= m_dPrecacheWindowSize - logicalVisibleSpace)
                {
                    break;
                }
            }
        }
        // Loop around and generate another item
        currentItemIndex++;
    }
    IFC(spGenerator->Stop());

#if DBG
    IFC(debug_CheckRealizedChildrenCount());
#endif

Cleanup:
    RRETURN(hr);
}

// Immediately cleans up any containers that have gone offscreen.  Called by MeasureOverride.
// When recycling this runs before generating and measuring children; otherwise it runs after.
_Check_return_ HRESULT CarouselPanel::CleanupContainers(
    _In_ xaml_controls::IItemsControl* pItemsControl)
{
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
    ctl::ComPtr<xaml_controls::IListViewBase> spListViewBase;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl = pItemsControl;

    ASSERT(m_bIsVirtualizing, L"Can't clean up containers if not virtualizing");
    ASSERT(pItemsControl, L"We can't cleanup if we aren't the itemshost");

    //
    // It removes items outside of the container cache window (a logical 'window' at
    // least as large as the viewport).
    //
    // firstViewport is the index of first data item that will be in the viewport
    // at the end of Measure.  This is effectively the scroll offset.
    //
    // m_iVisibleStart is index of the first data item that was previously at the top of the viewport
    // At the end of a Measure pass m_iVisibleStart == firstViewport.
    //
    // m_iVisibleCount is the number of data items that were previously visible in the viewport.

    INT cleanupRangeStart = -1;
    INT cleanupCount = 0;
    INT itemIndex = -1;              // data item index used to compare with the cache window position.
    INT lastItemIndex = -1;
    INT focusedChild = -1, previousFocusable = -1, nextFocusable = -1;  // child indices for the focused item and before and after focus trail items

    BOOLEAN performCleanup = FALSE;

    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));

    UINT nCount = 0;
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));

    if (nCount == 0)
    {
        return S_OK;// nothing to do
    }

    IFC_RETURN(FindFocusedChildInRealizedChildren(focusedChild, previousFocusable, nextFocusable));

    //
    // Iterate over all realized children and recycle the ones that are eligible.  Items NOT eligible for recycling
    // have one or more of the following properties
    //
    //  - inside the cache window
    //  - the item is its own container
    //  - has keyboard focus
    //  - is the first focusable item before or after the focused item
    //  - the CleanupVirtualizedItem event was canceled
    //  - is the primary dragged item.
    //

    spListViewBase = spItemsControl.AsOrNull<xaml_controls::IListViewBase>();

    for (UINT childIndex = 0; childIndex < nCount; childIndex++)
    {
        lastItemIndex = itemIndex;
        IFC_RETURN(GetGeneratedIndex(childIndex, itemIndex));

        if (itemIndex - lastItemIndex != 1)
        {
            // There's a generated gap between the current item and the last.  Clean up the last range of items.
            performCleanup = TRUE;
        }

        if (performCleanup)
        {
            if (cleanupRangeStart >= 0 && cleanupCount > 0)
            {
                //
                // We've hit a non-virtualizable container or a non-contiguous section.
                //

                IFC_RETURN(CleanupRange(cleanupRangeStart, cleanupCount));
                IFC_RETURN(spRealizedChildren->get_Size(&nCount));

                // CleanupRange just modified the _realizedChildren list.  Adjust the childIndex.
                childIndex -= cleanupCount;
                focusedChild -= cleanupCount;
                previousFocusable -= cleanupCount;
                nextFocusable -= cleanupCount;

                cleanupCount = 0;
                cleanupRangeStart = -1;
            }
        }

        // Assume non-recyclable container;
        performCleanup = TRUE;

        if (IsOutOfVisibleRange(itemIndex) &&
            childIndex != focusedChild &&
            childIndex != previousFocusable &&
            childIndex != nextFocusable)
        {
            ctl::ComPtr<IInspectable> spItem;

            BOOLEAN isOwnContainer = FALSE;
            if (!spItems)
            {
                IFC_RETURN(get_ItemContainerGenerator(&spGenerator));
                IFC_RETURN(spGenerator.Cast<ItemContainerGenerator>()->get_View(&spItems));
            }

            IFC_RETURN(spItems->GetAt(itemIndex, &spItem));
            if (!ctl::is<xaml_controls::IGroupItem>(spItem))
            {
                IFC_RETURN(static_cast<ItemsControl*>(pItemsControl)->IsItemItsOwnContainer(spItem.Get(), &isOwnContainer));
            }

            if (!isOwnContainer)
            {
                ctl::ComPtr<xaml::IUIElement> spChild = nullptr;
                BOOLEAN isContainerDragDropOwner = FALSE;

                IFC_RETURN(spRealizedChildren->GetAt(childIndex, &spChild));
                if (spListViewBase && spChild)
                {
                    IFC_RETURN(spListViewBase.Cast<ListViewBase>()->IsContainerDragDropOwner(
                        spChild.Get(), &isContainerDragDropOwner));
                }

                if (!isContainerDragDropOwner)
                {

                    BOOLEAN bCanceled = FALSE;
                    IFC_RETURN(NotifyCleanupItem(spItem.Get(), spChild.Get(), pItemsControl, bCanceled));
                    if (!bCanceled)
                    {
                        //
                        // The container is eligible to be virtualized
                        //
                        performCleanup = FALSE;

                        if (cleanupRangeStart == -1)
                        {
                            cleanupRangeStart = childIndex;
                        }

                        cleanupCount++;
                    }
                }
            }
        }
    }

    if (cleanupRangeStart >= 0 && cleanupCount > 0)
    {
        IFC_RETURN(CleanupRange(cleanupRangeStart, cleanupCount));
    }

    return S_OK;
}

// Determines if the given item index is out of the visible Range.
BOOLEAN CarouselPanel::IsOutOfVisibleRange(
    _In_ INT itemIndex)
{
    INT visibleStart = WrapIndex(m_iVisibleStart - m_iBeforeTrail);
    INT visibleEnd = WrapIndex(m_iVisibleStart + m_iVisibleCount - 1);

    return (visibleStart < visibleEnd) ?
                                (itemIndex < visibleStart || visibleEnd < itemIndex)
                                :  (itemIndex < visibleStart && visibleEnd < itemIndex);
}

// Ensures ordered list is cleared and intialized.
_Check_return_ HRESULT CarouselPanel::EnsureOrderedList()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<TrackerCollection<xaml::UIElement*>> spOrderedChildrenList;

    if (m_tpOrderedChildrenList)
    {
        IFC(m_tpOrderedChildrenList->Clear());
    }
    else
    {
        IFC(ctl::make(&spOrderedChildrenList));
        SetPtrValue(m_tpOrderedChildrenList, spOrderedChildrenList);
    }

Cleanup:
    RRETURN(hr);
}

// Updates ScrollData's offset, extent, and viewport in logical units.
_Check_return_ HRESULT CarouselPanel::UpdateLogicalScrollData(
    _Inout_ wf::Size& stackDesiredSize,
    _In_ wf::Size constraint,
    _In_ DOUBLE logicalVisibleSpace,
    _In_ wf::Size extent,
    _In_ INT lastViewport)
{
    BOOLEAN isScrolling = FALSE;
    ctl::ComPtr<xaml::IUIElement> spFirstVisibleChild;
    wf::Size viewport = constraint;
    ScrollVector offset = m_ScrollData.get_Offset();
    wf::Size desiredSize = {};

    IFC_RETURN(get_IsScrolling(&isScrolling));
    ASSERT(isScrolling, L"this computes logical scroll data");

    // If we have not yet set the last child in the viewport, set it to the last child.
    if (lastViewport == -1)
    {
        lastViewport = m_nItemsCount - 1;
    }

    INT logicalExtent = m_nItemsCount;
    DOUBLE logicalViewport = lastViewport - m_iVisibleStart;
    logicalViewport = static_cast<DOUBLE>(WrapIndex(static_cast<INT>(logicalViewport)));

    UINT nCount = 0;
    IFC_RETURN(m_tpOrderedChildrenList->get_Size(&nCount));

    offset.X = static_cast<FLOAT>(DoubleUtil::Max(0, DoubleUtil::Min(offset.X, extent.Width - viewport.Width)));

    if (m_bItemBasedScrolling)
    {
        offset.Y = static_cast<FLOAT>(m_iVisibleStart);
    }

    DOUBLE childHeight = 0.0;
    if (m_iBeforeTrail + 1 >= 0 && m_iBeforeTrail + 1 < static_cast<INT>(nCount))
    {
        IFC_RETURN(m_tpOrderedChildrenList->GetAt(m_iBeforeTrail + 1, &spFirstVisibleChild));
        IFC_RETURN(spFirstVisibleChild->get_DesiredSize(&desiredSize));
        childHeight = desiredSize.Height;
    }

    DOUBLE logicalDelta = m_bItemBasedScrolling ?
            DoubleUtil::GreaterThanOrClose(logicalVisibleSpace, 0.0) ? 1 : 0
            : (childHeight != 0 ? (childHeight + logicalVisibleSpace) / childHeight : 0.0) - DoubleUtil::Fractional(offset.Y);

    viewport.Height = static_cast<FLOAT>(DoubleUtil::Max(logicalViewport + logicalDelta, 0));

    // In case last item is visible because we scroll all the way to the bottom and scrolling is on
    // we want desired size not to be smaller than constraint to avoid another relayout
    if (logicalExtent > viewport.Height && !DoubleUtil::IsPositiveInfinity(constraint.Height))
    {
        stackDesiredSize.Height = constraint.Height;
    }

    // Since we can offset and clip our content, we never need to be larger than the parent suggestion.
    // If we returned the full size of the content, we would always be so big we didn't need to scroll.  :)
    stackDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Min(stackDesiredSize.Width, constraint.Width));
    stackDesiredSize.Height = static_cast<FLOAT>(DoubleUtil::Min(stackDesiredSize.Height, constraint.Height));

    // When scrolling, the maximum horizontal or vertical size of items can cause the desired size of the
    // panel to change, which can cause the owning ScrollViewer re-layout as well when it is not necessary.
    // We will thus remember the maximum desired size and always return that. The actual arrangement and
    // clipping still be calculated from actual scroll data values.
    // The maximum desired size is reset when the items change.
    m_ScrollData.m_MaxDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Width, m_ScrollData.m_MaxDesiredSize.Width));
    m_ScrollData.m_MaxDesiredSize.Height = stackDesiredSize.Height;

    stackDesiredSize = m_ScrollData.m_MaxDesiredSize;
    // Disable changing thumbsize for now.
    if (m_bShouldCarousel)
    {
        IFC_RETURN(CalculateOffsetAndSize(m_iVisibleStart, viewport, extent, offset.Y));
    }

    // Verify Scroll Info, invalidate ScrollOwner if necessary.
    IFC_RETURN(SetAndVerifyScrollingData(viewport, extent, offset));

    return S_OK;
}

// Calculate the thumb size and offset.
// Update viewport if thumb size needs to be reduced
// Reset offset if hitting the boundary.
_Check_return_ HRESULT CarouselPanel::CalculateOffsetAndSize(
    _In_ INT index,
    _Inout_ wf::Size& viewport,
    _In_ wf::Size extent,
    _Inout_ DOUBLE& offset)
{
    HRESULT hr = S_OK;
    DOUBLE thumbSize = 0;
    DOUBLE increment = viewport.Height/extent.Height;
    DOUBLE maxThumbSize = increment * viewport.Height;
    DOUBLE boundary = DoubleUtil::Round(viewport.Height, 0);
    DOUBLE calculatedOffset = increment*index;

    if (boundary - calculatedOffset >= maxThumbSize/2)
    {
        thumbSize = DoubleUtil::Min(maxThumbSize, boundary-calculatedOffset);
    }
    else
    {
        thumbSize = calculatedOffset + maxThumbSize - boundary;
        offset = 0;
    }

    // If thumbSize is not maxThumbSize we need to adjust viewport size and offset
    if (thumbSize < maxThumbSize)
    {
        viewport.Height = static_cast<FLOAT>(viewport.Height * thumbSize/maxThumbSize);
        if (offset != 0)
        {
            // We are going down and offset should be set to maximum.
            offset = extent.Height - viewport.Height;
        }
    }

    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::SetAndVerifyScrollingData(
    _In_ wf::Size viewport,
    _In_ wf::Size extent,
    _In_ ScrollVector offset)
{
    BOOLEAN isScrolling = FALSE;
    DOUBLE computedPixelExtent = 0;
    DOUBLE computedPixelOffset = 0;

    IFC_RETURN(get_IsScrolling(&isScrolling));
    ASSERT(isScrolling);

    // Detect changes to the viewport, extent, and offset
    BOOLEAN viewportChanged = !DoubleUtil::AreClose(viewport.Height, m_ScrollData.m_viewport.Height)
        || !DoubleUtil::AreClose(viewport.Width, m_ScrollData.m_viewport.Width);

    BOOLEAN extentChanged = !DoubleUtil::AreClose(extent.Height, m_ScrollData.m_extent.Height)
        || !DoubleUtil::AreClose(extent.Width, m_ScrollData.m_extent.Width);

    BOOLEAN offsetChanged = !DoubleUtil::AreClose(offset.X, m_ScrollData.m_ComputedOffset.X)
        || !DoubleUtil::AreClose(offset.Y, m_ScrollData.m_ComputedOffset.Y);

    // If nothing has changed check to see if pixel extent/offset have changed.
    if (!(viewportChanged || extentChanged || offsetChanged))
    {
        IFC_RETURN(ComputePixelExtent(false /*ignoreZoomFactor*/, computedPixelExtent));
        IFC_RETURN(ComputePixelOffset(FALSE, TRUE, offset.Y, computedPixelOffset));
        extentChanged = extentChanged || !DoubleUtil::AreClose(computedPixelExtent, m_LastComputedPixelExtent);
        offsetChanged = offsetChanged || !DoubleUtil::AreClose(computedPixelOffset, m_LastComputedPixelOffset);
    }

    // Update data and fire scroll change notifications
    IFC_RETURN(m_ScrollData.put_Offset(offset));
    if (viewportChanged || extentChanged || offsetChanged)
    {
        m_ScrollData.m_viewport = viewport;
        m_ScrollData.m_extent = extent;
        m_ScrollData.m_ComputedOffset = offset;

        if (!m_bShouldCarousel)
        {
            m_InternalOffset = offset.Y;
        }

        IFC_RETURN(OnScrollChange());
    }

    return S_OK;
}

// Wraps index according to m_nItemsCount
INT CarouselPanel::WrapIndex(
    _In_ INT index)
{
    ASSERT(m_nItemsCount != 0, L"WrapIndex should not be called when items count = 0");
    if (m_nItemsCount == 0)
    {
        return 0;
    }

    while (index < 0)
    {
        index += m_nItemsCount;
    }

    if (index >= m_nItemsCount)
    {
        index = index % m_nItemsCount;
    }

    return index;
}


// Query parent scroll owner and itemsControl (ComboBox) if scrolling is allowed.
_Check_return_ HRESULT CarouselPanel::get_IsScrolling(
    _Out_ BOOLEAN* pbIsScrolling)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOwner;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl;
    BOOLEAN isScrolling = FALSE;

    IFC(get_ScrollOwner(&spOwner));
    isScrolling = (spOwner != NULL);

    IFC(ItemsControl::GetItemsOwner(this, &spItemsControl));

    if (ctl::is<xaml_controls::IComboBox>(spItemsControl))
    {
        isScrolling = isScrolling && spItemsControl.Cast<ComboBox>()->GetIsPopupPannable();
    }
    ASSERT(!isScrolling || isScrolling && m_nItemsCount > 0, L"Wrong scrolling state");

Cleanup:
    *pbIsScrolling = isScrolling;

    RRETURN(hr);
}

// Methods accessed by owning ScrollViewer to support DirectManipulation.
// Gets the scrolling extent in pixels even for logical scrolling scenarios.
// To achieve carouseling we inflate the extent by m_DMExtentMultiplier
_Check_return_
HRESULT
CarouselPanel::ComputePixelExtent(
    _In_ bool ignoreZoomFactor,
    _Out_ DOUBLE& extent)
{
    HRESULT hr = S_OK;
    UINT nCount = 0;
    FLOAT cumulatedChildDim = 0.0;
    FLOAT zoomFactor = 1.0;

    if (!ignoreZoomFactor)
    {
        IFC(GetZoomFactor(&zoomFactor));
        ASSERT(zoomFactor == m_fZoomFactor);
    }

    // Return the estimated total dimension of unrealized items + dimension of all realized ones.
    extent = 0.0;

    // Get total dimension of realized children
    IFC(ComputeTotalRealizedChildDimension(cumulatedChildDim, nCount));

    if (nCount > 0)
    {
        extent = (DOUBLE) ((cumulatedChildDim + cumulatedChildDim / nCount * (m_nItemsCount - nCount)));
        if (m_bShouldCarousel)
        {
            // If we are carouseling we inflate the extent by m_DMExtentMultiplier to achieve the carouselling behavior.
            extent = extent * m_DMExtentMultiplier;
        }
        extent = extent * zoomFactor;
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::GetIrregularSnapPoints
//
//  Synopsis:
//    Used to retrieve an array of irregular snap points.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CarouselPanel::GetIrregularSnapPoints(
    _In_ BOOLEAN isForHorizontalSnapPoints,  // True when horizontal snap points are requested.
    _In_ BOOLEAN isForLeftAlignment,         // True when requested snap points will align to the left/top of the children
    _In_ BOOLEAN isForRightAlignment,        // True when requested snap points will align to the right/bottom of the children
    _Outptr_opt_result_buffer_(*pcSnapPoints) FLOAT** ppSnapPoints,   // Placeholder for returned array
    _Out_ UINT32* pcSnapPoints)                                   // Number of snap points returned
{
    HRESULT hr = S_OK;
    FLOAT* pSnapPoints = NULL;
    FLOAT* pSnapPointKeys = NULL;
    DOUBLE nextSeparatorItemOffset = 0.0;
    xaml_controls::Orientation orientation;
    DOUBLE extent = 0.0;
    DOUBLE viewport = 0.0;
    wf::Size childSize = {};

    IFCEXPECT(ppSnapPoints);
    *ppSnapPoints = NULL;
    IFCEXPECT(pcSnapPoints);
    *pcSnapPoints = 0;

    IFC(get_Orientation(&orientation));

    if (((orientation == xaml_controls::Orientation_Vertical && !isForHorizontalSnapPoints) ||
        (orientation == xaml_controls::Orientation_Horizontal && isForHorizontalSnapPoints)) && m_bShouldCarousel)
    {
        BOOLEAN areScrollSnapPointsRegular = FALSE;
        IFC(get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
        if (areScrollSnapPointsRegular)
        {
            // Accessing the irregular snap points while AreScrollSnapPointsRegular is True is not supported.
            IFC(E_FAIL); // TODO: Replace with custom error code. Something similar to InvalidOperationException.
        }

        IFC(ResetSnapPointKeys());
        IFC(ComputePixelExtent(false /*ignoreZoomFactor*/, extent));
        extent /= m_DMExtentMultiplier;
        nextSeparatorItemOffset = extent * (m_iCurrentLoop + 1);
        IFC(GetEstimatedRealizedChildSize(childSize));
        viewport = m_LastSetAvailableSize.Height;

        pSnapPoints = new FLOAT[m_cSnapPoints];
        pSnapPointKeys = new FLOAT[m_cSnapPointKeys];

        // We set 4 snap points.
        // 2 above and 2 below the viewport.
        // Above ones are in middle of separator to align to top of the combobox when panning down.
        // Below ones middle of separator - viewport size to align to top of the combobox when panning up.
        pSnapPoints[0] = static_cast<FLOAT>(nextSeparatorItemOffset - childSize.Height/2 - 2 * extent);
        pSnapPoints[1] = pSnapPoints[0] + static_cast<FLOAT>(extent);
        pSnapPoints[2] = static_cast<FLOAT>(nextSeparatorItemOffset - childSize.Height/2 - viewport);
        pSnapPoints[3] = pSnapPoints[2] + static_cast<FLOAT>(extent);

        // If separator item is within view set snap points to next 2 copies.
        if (m_iVisibleStart <= m_nItemsCount - 1 && m_nItemsCount - 1 <= m_iVisibleStart + m_iVisibleCount - m_iAfterTrail)
        {
            pSnapPoints[2] += static_cast<FLOAT>(extent);
            pSnapPoints[3] += static_cast<FLOAT>(extent);
        }

        pSnapPointKeys[0] = static_cast<FLOAT>(nextSeparatorItemOffset);

        *ppSnapPoints = pSnapPoints;
        *pcSnapPoints = m_cSnapPoints;
        pSnapPoints = NULL;
        m_pIrregularSnapPointKeys = pSnapPointKeys;
        m_cIrregularSnapPointKeys = m_cSnapPointKeys;
        m_bAreSnapPointsKeysHorizontal = isForHorizontalSnapPoints;
        pSnapPointKeys = NULL;

        // Next snap point change needs to raise a notification
        if (isForHorizontalSnapPoints)
        {
            m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        }
        else
        {
            m_bNotifiedVerticalSnapPointsChanges = FALSE;
        }
    }

Cleanup:
    delete [] pSnapPoints;
    delete [] pSnapPointKeys;
    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::GetIrregularSnapPointKeys
//
//  Synopsis:
//    Determines the keys for irregular snap points.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CarouselPanel::GetIrregularSnapPointKeys(
    _In_ xaml_controls::Orientation orientation,
    _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
    _In_ UINT32 nCount,
    _Outptr_result_buffer_(*pcSnapPointKeys) FLOAT** ppSnapPointKeys,
    _Out_ INT32* pcSnapPointKeys,
    _Out_ FLOAT* pSnapPointKeysOffset,
    _Out_ FLOAT* pLowerMarginSnapPointKey)
    //_Out_ FLOAT* pUpperMarginSnapPointKey) Use once horizontal carousel is enabled
{
    HRESULT hr = S_OK;
    FLOAT* pCopiedSnapPointKeys = NULL;

    IFCEXPECT(ppSnapPointKeys);
    *ppSnapPointKeys = NULL;
    IFCEXPECT(pcSnapPointKeys);
    *pcSnapPointKeys = 0;
    IFCEXPECT(pSnapPointKeysOffset);
    *pSnapPointKeysOffset = 0.0;
    IFCEXPECT(pLowerMarginSnapPointKey);
    *pLowerMarginSnapPointKey = 0.0;
    //IFCEXPECT(pUpperMarginSnapPointKey);
    //*pUpperMarginSnapPointKey = 0.0;

    *pcSnapPointKeys = m_cIrregularSnapPointKeys;
    if (m_cIrregularSnapPointKeys > 0) {
        pCopiedSnapPointKeys = new FLOAT[m_cIrregularSnapPointKeys];
        memcpy(pCopiedSnapPointKeys, m_pIrregularSnapPointKeys, sizeof(FLOAT)*m_cIrregularSnapPointKeys);
    }

    IFC(GetCommonSnapPointKeys(pLowerMarginSnapPointKey /*, pUpperMarginSnapPointKey*/));

    *ppSnapPointKeys = pCopiedSnapPointKeys;
    pCopiedSnapPointKeys = NULL;

Cleanup:
    delete [] pCopiedSnapPointKeys;
    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::GetCommonSnapPointKeys
//
//  Synopsis:
//    Determines the common keys for regular and irregular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::GetCommonSnapPointKeys(
    _Out_ FLOAT* pLowerMarginSnapPointKey)
    //_Out_ FLOAT* pUpperMarginSnapPointKey) Use once horizontal carousel is enabled
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IFrameworkElement> spFrameworkElement;
    xaml_controls::Orientation orientation;
    xaml::Thickness margins;

    IFCEXPECT(pLowerMarginSnapPointKey);
    *pLowerMarginSnapPointKey = 0.0;
    //IFCEXPECT(pUpperMarginSnapPointKey);
    //*pUpperMarginSnapPointKey = 0.0;

    IFC(get_Orientation(&orientation));

    spFrameworkElement = ctl::query_interface_cast<xaml::IFrameworkElement>(ctl::as_iinspectable(this));
    if (spFrameworkElement)
    {
        IFC(spFrameworkElement->get_Margin(&margins));
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            *pLowerMarginSnapPointKey = static_cast<FLOAT>(margins.Left);
            //*pUpperMarginSnapPointKey = margins.Right;
        }
        else
        {
            *pLowerMarginSnapPointKey = static_cast<FLOAT>(margins.Top);
            //*pUpperMarginSnapPointKey = margins.Bottom;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Gets internal real offset different than scroll data.
// This is the offset used by CSP to work with DM in carouseling mode
DOUBLE CarouselPanel::getInternalOffset()
{
    if (m_InternalOffset < 0)
    {
        m_InternalOffset = m_iVisibleStart < 0 ? 0 : m_iVisibleStart;
    }

    return m_InternalOffset;
}

_Check_return_ HRESULT CarouselPanel::TranslateVerticalPixelDeltaToOffset(
    _In_ DOUBLE delta,
    _Out_ DOUBLE& value)
{
    HRESULT hr = S_OK;
    BOOLEAN canVerticallyScroll = FALSE;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    OffsetMemento* pCurrentOffsetState = NULL;
    BOOLEAN bHorizontal = FALSE;
    FLOAT zoomFactor = 1.0;

    DOUBLE currentOffset = getInternalOffset();
    DOUBLE offset = 0.0;
    UINT nRealizedChildrenCount = 0;
    UINT nVisualChildrenCount = 0;
    value = 0.0;

    IFC(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    IFC(get_CanVerticallyScroll(&canVerticallyScroll));

    if (!canVerticallyScroll)
    {
        goto Cleanup;
    }

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nRealizedChildrenCount));

    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&nVisualChildrenCount));

    pCurrentOffsetState = new OffsetMemento(
        xaml_controls::Orientation_Vertical,
        nRealizedChildrenCount,
        nVisualChildrenCount,
        m_ScrollData);

    if (pCurrentOffsetState->Equals(m_pTranslatedOffsetState) &&
        m_pTranslatedOffsetState->get_CurrentOffset() == currentOffset &&
        m_pTranslatedOffsetState->get_Delta() == delta)
    {
        value = m_pTranslatedOffsetState->get_RequestedOffset();
        goto Cleanup;
    }

    IFC(pCurrentOffsetState->put_Delta(delta));
    IFC(pCurrentOffsetState->put_CurrentOffset(currentOffset));

    delete m_pTranslatedOffsetState;
    m_pTranslatedOffsetState = NULL;

    IFC(get_Orientation(&orientation));
    bHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    if (!m_bIsVirtualizing || bHorizontal || delta == 0.0)
    {
        offset = currentOffset + delta;
    }
    // During an ItemsSource reset, we recycle everything and nRealizedChildrenCount will be equal to 0.
    // In that case, we should return a zero offset.
    else if(nRealizedChildrenCount != 0u)
    {

        DOUBLE logicalOffset = DoubleUtil::Floor(currentOffset);
        INT currentChildIndex =  (m_bShouldCarousel) ?  m_iFirstVisibleChildIndex : m_iFirstVisibleChildIndex + (int)logicalOffset - m_iVisibleStart;
        if (currentChildIndex < 0)
        {
            currentChildIndex += nRealizedChildrenCount;
        }
        DOUBLE itemLogicalOffset = DoubleUtil::Fractional(currentOffset);
        wf::Size desiredSize = {0,0};

        if (delta < 0)
        {
            while (delta < 0)
            {
                DOUBLE itemHeight = 0.0;
                DOUBLE itemOffsetTop = 0.0;

                if (logicalOffset != m_nItemsCount-1)
                {
                    ctl::ComPtr<xaml::IUIElement> spChild;

                    if (currentChildIndex < 0)
                    {
                        currentChildIndex += nRealizedChildrenCount;
                    }

                    currentChildIndex = std::max(0, std::min(currentChildIndex, static_cast<int>(nRealizedChildrenCount)-1));
                    IFC(spRealizedChildren->GetAt(currentChildIndex, &spChild));
                    IFC(spChild->get_DesiredSize(&desiredSize));
                }
                else if (desiredSize.Height <= 0) // For Separator Item.
                {
                    IFC(GetEstimatedRealizedChildSize(desiredSize));
                }

                itemHeight = desiredSize.Height * zoomFactor;
                itemOffsetTop = itemLogicalOffset * itemHeight;

                itemOffsetTop += delta;
                if (itemOffsetTop >= 0)
                {
                    logicalOffset += itemOffsetTop / itemHeight;
                    m_pTranslatedOffsetState = pCurrentOffsetState;
                    IFC(m_pTranslatedOffsetState->put_RequestedOffset(logicalOffset));
                    pCurrentOffsetState = NULL;
                }
                else
                {
                    itemLogicalOffset = 1;
                    logicalOffset--;
                    currentChildIndex--;
                }
                delta = itemOffsetTop;
            }
        }
        else
        {
            while (delta > 0)
            {
                DOUBLE itemHeight = 0.0;
                DOUBLE itemOffsetBottom = 0.0;

                if (logicalOffset != m_nItemsCount-1)
                {
                    ctl::ComPtr<xaml::IUIElement> spChild;

                    if (currentChildIndex >= static_cast<INT>(nRealizedChildrenCount))
                    {
                        currentChildIndex -= nRealizedChildrenCount;
                    }

                    currentChildIndex = std::max(0, std::min(currentChildIndex, static_cast<int>(nRealizedChildrenCount)-1));
                    IFC(spRealizedChildren->GetAt(currentChildIndex, &spChild));
                    IFC(spChild->get_DesiredSize(&desiredSize));
                }
                else if (desiredSize.Height <= 0) // For Separator Item.
                {
                    IFC(GetEstimatedRealizedChildSize(desiredSize));
                }

                itemHeight = desiredSize.Height * zoomFactor;
                itemOffsetBottom = (1 - itemLogicalOffset) * itemHeight;

                itemOffsetBottom -= delta;
                if (itemOffsetBottom >= 0)
                {
                    logicalOffset += (itemHeight - itemOffsetBottom) / itemHeight;
                    m_pTranslatedOffsetState = pCurrentOffsetState;
                    IFC(m_pTranslatedOffsetState->put_RequestedOffset(logicalOffset));
                    pCurrentOffsetState = NULL;
                }
                else
                {
                    itemLogicalOffset = 0;
                    logicalOffset++;
                    currentChildIndex++;
                }
                delta = -itemOffsetBottom;
            }
        }

        offset = logicalOffset;
    }

    if(m_nItemsCount > 0 && m_bShouldCarousel)
    {
        while (offset >= m_nItemsCount)
        {
            m_iCurrentLoop++;
            offset -= m_nItemsCount;
        }

        while (offset < 0)
        {
            m_iCurrentLoop--;
            offset += m_nItemsCount;
        }
    }

    value = offset;

Cleanup:
    delete pCurrentOffsetState;
    RRETURN(hr);
}

// Returns the index of the first item visible (even partially) in the viewport.
INT CarouselPanel::ComputeIndexOfFirstVisibleItem(
    _Out_ DOUBLE& firstItemOffset)
{
    DOUBLE offset = getInternalOffset();
    firstItemOffset = DoubleUtil::Fractional(offset);

    return WrapIndex(static_cast<INT>(offset));
}

_Check_return_ HRESULT CarouselPanel::ComputePixelOffsetOfChildAtIndex(
    _In_ INT index,
    _Out_ DOUBLE& offset)
{
    HRESULT hr = S_OK;
    ASSERT(!m_bItemBasedScrolling);
    FLOAT cumulatedChildDim = 0.0;
    DOUBLE extent = 0.0;
    UINT nCount = 0;

    // Get total dimension of realized children
    IFC(ComputeTotalRealizedChildDimension(cumulatedChildDim, nCount));

    extent = (DOUBLE) (cumulatedChildDim + cumulatedChildDim / nCount * (m_nItemsCount - nCount));

    // Dimension of all children before visible started one:
    offset = index * cumulatedChildDim / nCount;

    // If carouseling, factor in the inflated extent.
    if (m_bShouldCarousel)
    {
        offset += extent * m_iCurrentLoop ;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the offset in pixels even for logical scrolling scenarios.
_Check_return_ HRESULT CarouselPanel::ComputePixelOffset(
    _In_ BOOLEAN isForHorizontalOrientation,
    _Out_ DOUBLE& offset)
{
    RRETURN(ComputePixelOffset(isForHorizontalOrientation, FALSE /*bUseInputLogicalOffset*/, 0, offset));
}

_Check_return_ HRESULT CarouselPanel::ComputePixelOffset(
    _In_ BOOLEAN isForHorizontalOrientation,
    _In_ BOOLEAN bUseProvidedLogicalOffset,
    _In_ DOUBLE logicalOffset,
    _Out_ DOUBLE& offset)
{
    HRESULT hr = S_OK;
    DOUBLE physicalOffset = 0.0;
    DOUBLE fractionalItemOffset = 0.00;
    wf::Size childDesiredSize = {};
    FLOAT zoomFactor = 1.0;

    if (!bUseProvidedLogicalOffset)
    {
        // Dimension of realized children scrolled off:
        if (isForHorizontalOrientation)
        {
            IFC(get_HorizontalOffset(&logicalOffset));
        }
        else
        {
            logicalOffset = getInternalOffset();
        }
    }

    fractionalItemOffset = DoubleUtil::Fractional(logicalOffset);
    if (fractionalItemOffset > 0 && m_iBeforeTrail >= 0)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;

        IFC(m_tpOrderedChildrenList->GetAt(m_iBeforeTrail, &spChild));
        IFC(spChild->get_DesiredSize(&childDesiredSize));
        fractionalItemOffset = fractionalItemOffset * (isForHorizontalOrientation ? childDesiredSize.Width : childDesiredSize.Height);
    }

    IFC(ComputePixelOffsetOfChildAtIndex(static_cast<INT>(logicalOffset), physicalOffset));
    offset = (DOUBLE) physicalOffset + fractionalItemOffset;
    IFC(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);
    offset *= zoomFactor;

Cleanup:
    RRETURN(hr);
}

// Set the VerticalOffset to the passed value.
_Check_return_ HRESULT CarouselPanel::SetVerticalOffsetImpl(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    DOUBLE extentHeight = 0.0;
    DOUBLE viewportHeight = 0.0;
    DOUBLE scrollY = 0.0;

    if (m_bInMeasure)
    {
        goto Cleanup;
    }

    IFC(get_ExtentHeight(&extentHeight));
    IFC(get_ViewportHeight(&viewportHeight));

    // If there are no items we should not do anything.
    if (m_nItemsCount <= 0)
    {
        goto Cleanup;
    }

    IFC(ScrollContentPresenter::ValidateInputOffset(offset, m_ScrollData.m_MinOffset.Y, m_bShouldCarousel ? DoubleUtil::MaxValue : extentHeight - viewportHeight, &scrollY));

    // If we are scrolling by item, then round the offset
    if (m_bItemBasedScrolling)
    {
        scrollY = DoubleUtil::Floor(scrollY);
    }

    if (!DoubleUtil::AreClose(scrollY, m_InternalOffset))
    {
        BOOLEAN bIsInDMZoom = FALSE;
        m_IndexToEnsureInView = -1;
        IFC(m_ScrollData.put_OffsetY(scrollY));
        m_InternalOffset = scrollY;

        IFC(IsInDirectManipulationZoom(bIsInDMZoom));
        if (!bIsInDMZoom)
        {
            IFC(InvalidateMeasure());
        }
    }

Cleanup:
    RRETURN(hr);
}


// Computes the estimated dimension of the unrealized children ahead of the realized ones.
_Check_return_ HRESULT CarouselPanel::ComputeUnrealizedChildrenEstimatedDimension(
    _Out_ FLOAT& dimension)
{
    HRESULT hr = S_OK;
    UINT nCount = 0;
    FLOAT cumulatedChildDim = 0.0;

    dimension = 0.0;

#ifdef DBG
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    IFC(get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
    ASSERT(!areScrollSnapPointsRegular);
#endif

    // m_iVisibleStart - m_iBeforeTrail is the number of actual unrealized children ahead of realized ones.
    INT numOfUnrealizedChildren = m_iVisibleStart - m_iBeforeTrail;
    if (numOfUnrealizedChildren > 0)
    {
        IFC(ComputeTotalRealizedChildDimension(cumulatedChildDim, nCount));
        if (nCount > 0)
        {
            dimension = numOfUnrealizedChildren * cumulatedChildDim / nCount;
        }
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT CarouselPanel::ComputePhysicalFromLogicalOffset(
    _In_ INT logicalOffset,
    _In_ DOUBLE fractionalItemOffset,
    _In_ BOOLEAN isHorizontal,
    _Out_ DOUBLE& physicalOffset)
{
    HRESULT hr = S_OK;
    wf::Size firstVisibleChildSize = {};

    physicalOffset = 0.0;

    UINT nCount = 0;

    IFC(m_tpOrderedChildrenList->get_Size(&nCount));
    if (static_cast<INT>(nCount) > m_iBeforeTrail + 1)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;

        IFC(m_tpOrderedChildrenList->GetAt(m_iBeforeTrail + 1, &spChild));
        IFC(spChild->get_DesiredSize(&firstVisibleChildSize));
    }

    physicalOffset = m_bItemBasedScrolling ? 0.0 : -1 * fractionalItemOffset * (isHorizontal ? firstVisibleChildSize.Width : firstVisibleChildSize.Height);
    ASSERT(logicalOffset == 0 || logicalOffset < static_cast<INT>(nCount));

    for (INT i = 0; i < logicalOffset; i++)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;

        wf::Size childDesiredSize = {};
        IFC(m_tpOrderedChildrenList->GetAt(i, &spChild));
        IFC(spChild->get_DesiredSize(&childDesiredSize));
        physicalOffset -= isHorizontal
            ? childDesiredSize.Width
            : childDesiredSize.Height;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::GetEstimatedRealizedChildSize(
    _Out_ wf::Size& childSize)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spChild;

    childSize.Height = 0;
    childSize.Width = 0;

    IFC(GetRealizedFirstChild(&spChild));

    if (spChild)
    {
        IFC(spChild->get_DesiredSize(&childSize));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::GetRealizedFirstChild(
    _Outptr_ xaml::IUIElement**  ppRealizedFirstChild)
{
    HRESULT hr = S_OK;
    UINT nCount = 0;

    IFCPTR(ppRealizedFirstChild);
    *ppRealizedFirstChild = NULL;

    if (m_tpOrderedChildrenList)
    {
        IFC(m_tpOrderedChildrenList->get_Size(&nCount));

        if (nCount > 0)
        {
            ctl::ComPtr<xaml::IUIElement> spChild;

            IFC(m_tpOrderedChildrenList->GetAt(0, &spChild));
            IFC(spChild.CopyTo(ppRealizedFirstChild));
        }
    }

Cleanup:
    RRETURN(hr);
}

#if DBG

// Debug helper method to check state of realized and ordered items list.
_Check_return_ HRESULT CarouselPanel::debug_CheckRealizedChildrenCount()
{
    HRESULT hr = S_OK;
    UINT nCount = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

    UINT oCount = 0;
    IFC(m_tpOrderedChildrenList->get_Size(&oCount));

Cleanup:
    RRETURN(hr);
}

#endif

_Check_return_ HRESULT CarouselPanel::get_PhysicalOrientation(
    _Out_ xaml_controls::Orientation* orientation)
{
    RRETURN(get_Orientation(orientation));
}

// CSP currently only works for vertical orientation.
_Check_return_ HRESULT CarouselPanel::get_Orientation(
    _Out_ xaml_controls::Orientation* orientation)
{
    *orientation = xaml_controls::Orientation_Vertical;
    RRETURN(S_OK);
}

_Check_return_ HRESULT CarouselPanel::get_AreScrollSnapPointsRegular(
    _Out_ BOOLEAN* retValue)
{
    *retValue = FALSE;
    RRETURN(S_OK);
}

_Check_return_ HRESULT CarouselPanel::ResetScrollData()
{
    HRESULT hr = S_OK;
    m_ScrollData.ClearLayout();
    m_ScrollData.m_canHorizontallyScroll = FALSE;
    m_ScrollData.m_canVerticallyScroll = FALSE;
    IFC(OnScrollChange());

Cleanup:
    RRETURN(hr);
}

void CarouselPanel::ResetMinimumDesiredWindowWidth()
{
    m_minimumDesiredWindowWidth = 0;
}

void CarouselPanel::ResetOffsetLoop()
{
    m_iCurrentLoop = m_CarouselOffsetStart;
}

DOUBLE CarouselPanel::GetMeasureDeltaForVisualsBetweenPopupAndCarouselPanel()
{
    return m_SizeOfVisualsInPopup;
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::GetRegularSnapPoints
//
//  Synopsis:
//    Returns an original offset and interval for equidistant snap points for
//    the provided orientation. Returns 0 when no snap points are present.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CarouselPanel::GetRegularSnapPointsImpl(
    // The direction of the requested snap points.
    _In_ xaml_controls::Orientation orientation,
    // The alignment used by the caller when applying the requested snap points.
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    // The offset of the first snap point.
    _Out_ FLOAT* pOffset,
    // The interval between the regular snap points.
    _Out_ FLOAT* pInterval)
{
    HRESULT hr = S_OK;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    UINT32 nCount = 0;
    FLOAT childDim = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    //FLOAT upperMarginSnapPointKey = 0.0;
    wf::Size childDesiredSize = {};
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml::IUIElement> spChild;
    xaml_controls::Orientation stackingOrientation;

    IFCPTR(pOffset);
    *pOffset = 0.0;
    IFCPTR(pInterval);
    *pInterval = 0.0;

    IFC(get_Orientation(&stackingOrientation));

    if (orientation == stackingOrientation)
    {
        IFC(get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));

        if (!areScrollSnapPointsRegular)
        {
            // Accessing the regular snap points while AreScrollSnapPointsRegular is False is not supported.
            IFC(E_FAIL); // TODO: Replace with custom error code. Something similar to InvalidOperationException.
        }

        IFC(ResetSnapPointKeys());

        IFC(GetCommonSnapPointKeys(&lowerMarginSnapPointKey /*, &upperMarginSnapPointKey*/));

        IFC(get_RealizedChildren(&spRealizedChildren));
        IFC(spRealizedChildren->get_Size(&nCount));

#if DBG
        IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

        if (nCount > 0)
        {
            IFC(spRealizedChildren->GetAt(0, &spChild));
            if (spChild)
            {
                IFC(spChild->get_DesiredSize(&childDesiredSize));

                if (orientation == xaml_controls::Orientation_Vertical)
                {
                    childDim = childDesiredSize.Height;
                }
                else
                {
                    childDim = childDesiredSize.Width;
                }

                *pOffset = childDim / 2 + lowerMarginSnapPointKey;
                *pInterval = childDim;
            }
        }

        m_regularSnapPointKey = childDim;
        m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
        //m_upperMarginSnapPointKey = upperMarginSnapPointKey;
        m_bAreSnapPointsKeysHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

        // Next snap point change needs to raise a notification
        if (m_bAreSnapPointsKeysHorizontal)
        {
            m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        }
        else
        {
            m_bNotifiedVerticalSnapPointsChanges = FALSE;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Sets the m_bInManipulation flag to tell whether a manipulation is in progress or not.
_Check_return_
HRESULT
CarouselPanel::UpdateInManipulation(
    _In_ BOOLEAN isInManipulation,
    _In_ BOOLEAN isInLiveTree,
    _In_ DOUBLE nonVirtualizingOffset)
{
    HRESULT hr = S_OK;

    m_bInManipulation = isInManipulation;

    if (isInLiveTree)
    {
        m_bNotifyLayoutRefresh = TRUE;
        IFC(InvalidateMeasure());

        if (!isInManipulation)
        {
            IFC(put_IsIgnoringTransitions(TRUE));

            // Since InvalidateMeasure() is skipped when IsInDirectManipulationZoom returns True,
            // the non virtualized offset may not have been updated during the manipulation.
            // This call will push the offset at the end of the manipulation based on the final extent.
            IFC(SetNonVirtualizingOffset(nonVirtualizingOffset));

            // force layout to occur synchronously so that the IsIgnoringTransitions flag takes hold
            // this will force layout all the full visual tree, but one would expect not much to be dirty
            // when exiting DM.
            IFC(UpdateLayout());    // will reset the isIgnoring flag
        }
    }

Cleanup:
    RRETURN(hr);
}

// Updates the zoom factor
_Check_return_ HRESULT CarouselPanel::SetZoomFactor(
    _In_ FLOAT newZoomFactor)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsInDMZoom = FALSE;

    m_fZoomFactor = newZoomFactor;

    IFC(IsInDirectManipulationZoom(bIsInDMZoom));
    if (!bIsInDMZoom)
    {
        IFC(InvalidateMeasure());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::ScrollIntoView(
    _In_ UINT index,
    _In_ BOOLEAN isGroupItemIndex,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;
    BOOLEAN bHorizontal = FALSE;
    DOUBLE groupIndex = index;
    DOUBLE verticalOffset = 0;
    DOUBLE viewportHeight = 0;
    DOUBLE scrollToOffset = 0;
    INT indexToEnsureInView = -1;

    if(!isGroupItemIndex)
    {
        IFC(GetIndexInGroupView(index, groupIndex));
    }

    // Get the scroll Owner
    IFC(m_ScrollData.get_ScrollOwner(&spOwner));

    switch (alignment)
    {
    case xaml_controls::ScrollIntoViewAlignment_Leading:
        {
            IFC(spOwner->ScrollToVerticalOffsetImpl(groupIndex));
            break;
        }

    case xaml_controls::ScrollIntoViewAlignment_Default:
    default:
        {
            IFC(get_VerticalOffset(&verticalOffset));
            IFC(get_ViewportHeight(&viewportHeight));
            // If scrollIndex is above the Vertical offset, bring it into view
            if (verticalOffset - groupIndex > 0 || viewportHeight < 1)
            {
                IFC(spOwner->ScrollToVerticalOffsetImpl(groupIndex));
            }
            // if ScrollIndex is below the viewport Size
            else if(verticalOffset + viewportHeight < groupIndex + 1)
            {
                IFC(GetEstimatedOffsetForScrollIntoView(groupIndex, viewportHeight, bHorizontal, scrollToOffset, indexToEnsureInView));
                IFC(spOwner->ScrollToVerticalOffsetImpl(scrollToOffset));
                m_IndexToEnsureInView = indexToEnsureInView;
            }
            break;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::GetZoomFactor(
    _Out_ FLOAT* zoomFactor)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(spScrollOwner->get_ZoomFactorImpl(zoomFactor));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::IsInDirectManipulationZoom(
    _Out_ BOOLEAN& bIsInDirectManipulationZoom)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    bIsInDirectManipulationZoom = FALSE;
    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));

    if (spScrollOwner)
    {
        IFC(spScrollOwner->IsInDirectManipulationZoom(bIsInDirectManipulationZoom));
    }

Cleanup:
    RRETURN(hr);
}

// ScrollIntoView calls this method to get correct estimated offset when the item being scrolled into view is below viewport.
// If the item being scrolled into view is in realized list we will walk up to find the correct offset
// Else we return indexToEnsureInView to the item being scrolled into view and Measure will ensure that item is in view.
_Check_return_ HRESULT CarouselPanel::GetEstimatedOffsetForScrollIntoView(
    _In_ DOUBLE index,
    _In_ DOUBLE viewportSize,
    _In_ BOOLEAN bHorizontal,
    _Out_ DOUBLE& scrollToOffset,
    _Out_ INT& indexToEnsureInView)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    xaml_primitives::GeneratorPosition position = {};
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    wf::Size childDesiredSize = {};
    FLOAT availableSize = bHorizontal ? m_LastSetAvailableSize.Width : m_LastSetAvailableSize.Height;
    FLOAT childSize = 0;
    INT logicalIndex = static_cast<INT>(index);
    UINT nCount = 0;
    UINT currentRealizedIndex = 0;
    scrollToOffset = index - viewportSize + 1;
    indexToEnsureInView = -1;

    // If item is realized, measure the offset based on pixel sizes.
    IFC(GetItemContainerGenerator(&spGenerator, NULL));
    if (spGenerator)
    {
        IFC(get_RealizedChildren(&spRealizedChildren));
        IFC(spRealizedChildren->get_Size(&nCount));
        if (logicalIndex < static_cast<INT>(nCount))
        {
            IFC(spGenerator->GeneratorPositionFromIndex(logicalIndex, &position));
            currentRealizedIndex = static_cast<UINT>(position.Index);
            while (position.Offset == 0 && currentRealizedIndex < nCount && availableSize > childSize ) // All items are realized
            {
                ctl::ComPtr<xaml::IUIElement> spChild;

                availableSize -= childSize;
                IFC(spRealizedChildren->GetAt(currentRealizedIndex, &spChild));
                IFC(spChild->get_DesiredSize(&childDesiredSize));

                // If Width and Height are zero, ensure this item has been measured, otherwise scroll offset will be off.
                if (childDesiredSize.Width == 0 && childDesiredSize.Height == 0)
                {
                    IFC(MeasureChild(spChild.Get(), m_LastSetChildLayoutSlotSize, &childDesiredSize));
                }

                childSize = bHorizontal ? childDesiredSize.Width : childDesiredSize.Height;
                logicalIndex--;
                IFC(spGenerator->GeneratorPositionFromIndex(logicalIndex, &position));
                currentRealizedIndex = static_cast<UINT>(position.Index);
            }

            logicalIndex++; // +1 to get to last measured item.
        }

        if (availableSize <= childSize)
        {
            scrollToOffset = logicalIndex + (childSize-availableSize)/childSize;
        }
        else // Atleast 1 item above was not realized
        {
            indexToEnsureInView = static_cast<INT>(index);
        }
    }

Cleanup:
    RRETURN(hr);
}

// If ScrollIntoView has set and item to be ensured in view
// We will realize backwards from that item to top of viewport such that bottom of that item aligns with bottom of viewport.
_Check_return_ HRESULT CarouselPanel::CorrectOffsetForScrollIntoView(
    _In_ DOUBLE viewportSize,
    _In_ BOOLEAN bHorizontal,
    _In_ xaml_controls::IItemsControl* pItemsControl,
    _In_ wf::Size layoutSlotSize,
    _Out_ DOUBLE& firstItemOffset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    xaml_primitives::GeneratorPosition position = {};
    wf::Size childDesiredSize = {};
    INT currentIndex=0;
    INT childIndex=0;
    DOUBLE usedSize = 0;
    BOOLEAN visualOrderChanged = FALSE;
    BOOLEAN newlyRealized = FALSE;

    ASSERT(m_IndexToEnsureInView >= 0, L"Index to ensure in View should be greater than 0.");

    currentIndex = m_IndexToEnsureInView <= m_nItemsCount ? m_IndexToEnsureInView : m_nItemsCount;
    m_IndexToEnsureInView = -1; // reset index to be scrolled into view.
    IFC(GetItemContainerGenerator(&spGenerator, pItemsControl));
    while(usedSize < viewportSize && currentIndex >= 0)
    {
        ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
        ctl::ComPtr<xaml::IUIElement> spUIChild;

        IFC(IndexToGeneratorPositionForStart(currentIndex, childIndex, position));
        // We generate forward because backward generation has issues.
        // This should be changed to backward generation to improve performance alongside GeneratePerviousChild method.
        IFC(spGenerator->StartAt(position, xaml_primitives::GeneratorDirection_Forward, TRUE));
        IFC(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));

        if (!ctl::is<xaml::IUIElement>(spChildAsDO))
        {
            ASSERT(!newlyRealized, L"The generator realized a null value.");
            spChildAsDO.Reset();
            // We reached the end of the items (because of a group)
            break;
        }

        IFC(spChildAsDO.As(&spUIChild));
        IFC(AddContainerFromGenerator(childIndex, spUIChild.Get(), newlyRealized, visualOrderChanged));
        IFC(MeasureChild(spUIChild.Get(), layoutSlotSize, &childDesiredSize));
        if (bHorizontal)
        {
            usedSize += childDesiredSize.Width;
        }
        else
        {
            usedSize += childDesiredSize.Height;
        }

        currentIndex--;
        IFC(spGenerator->Stop());
    }

    currentIndex++; // +1 to get to last measured item.
    if (usedSize >= viewportSize) // We have realized all items above to fill the viewable region.
    {
        m_iVisibleStart = currentIndex;
        if (bHorizontal && childDesiredSize.Width > 0)
        {
            IFC(m_ScrollData.put_OffsetX(currentIndex + (usedSize - viewportSize)/childDesiredSize.Width));
            firstItemOffset = DoubleUtil::Fractional(m_ScrollData.get_OffsetX());
        }
        else if (childDesiredSize.Height > 0)
        {
            IFC(m_ScrollData.put_OffsetY(currentIndex + (usedSize - viewportSize)/childDesiredSize.Height));
            firstItemOffset = DoubleUtil::Fractional(m_ScrollData.get_OffsetY());
        }
    }

Cleanup:
    RRETURN(hr);
}

// Updates the non-virtualized offset irrespective of the extent, before the coming MeasureOverride
// execution updates the extent based on the new zoom factor.
_Check_return_ HRESULT CarouselPanel::SetNonVirtualizingOffset(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    ASSERT(!m_bItemBasedScrolling);

    if (offset < 0.0f)
    {
        // m_bInMeasure may be set to True here when a synchronous BringIntoViewport operation completes during a measure pass.
        goto Cleanup;
    }

    ASSERT(!m_bInMeasure);

    IFC(get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        IFC(m_ScrollData.put_OffsetY(offset));
    }
    else
    {
        IFC(m_ScrollData.put_OffsetX(offset));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::GetSizeOfFirstVisibleChild(
    _Out_ wf::Size& size)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spRealizedChild;
    UINT nCount = 0;

    if (m_tpOrderedChildrenList)
    {
        IFC(m_tpOrderedChildrenList->get_Size(&nCount));
        if (nCount > 0 && m_iBeforeTrail < (INT)nCount)
        {
            IFC(m_tpOrderedChildrenList->GetAt(m_iBeforeTrail, &spRealizedChild));
            IFC(spRealizedChild->get_DesiredSize(&size));
        }
    }

Cleanup:
    RRETURN(hr);
}




