// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      OrientedVirtualizingPanel
//      Represents a OrientedVirtualizingPanel.
//      It is a base class for VirtualizingStackPanel and WrapGrid
//      Responsibilities:
//      - Scrolling (Implements IScrollInfo)
//      - Generating containers, clearing them, reclycling themainers which are focused
//        or visible. Also it recuses the containers for better performance
//      - Generates only those cont
//      - Insert, add, remove containers
//      - Implements IOrientedPanel interface for logical and physical Orientation

#include "precomp.h"
#include "OrientedVirtualizingPanel.g.h"
#include "ScrollViewer.g.h"
#include "ScrollContentPresenter.g.h"
#include "ListViewBase.g.h"
#include "UIElementCollection.g.h"
#include "CleanUpVirtualizedItemEventArgs.g.h"
#include "PropertyMetadata.g.h"
#include "ItemContainerGenerator.g.h"
#include <math.h>
#include "VisualTreeHelper.h"

//#define OVP_DEBUG

using namespace DirectUI;
using namespace DirectUISynonyms;

OrientedVirtualizingPanel::OrientedVirtualizingPanel():
        m_VirtualizationMode(xaml_controls::VirtualizationMode_Standard),
        m_iVisibleStart(-1),
        m_iVisibleCount(0),
        m_nItemsCount(0),
        m_iCacheStart(0),
        m_iFirstVisibleChildIndex(-1),
        m_iBeforeTrail(0),
        m_iAfterTrail(0),
        m_dPrecacheWindowSize(200),
        m_dPrecacheBeforeTrailSize(0),
        m_dPrecacheAfterTrailSize(0),
        m_bItemBasedScrolling(FALSE),
        m_pTranslatedOffsetState(NULL),
        m_bIsVirtualizing(FALSE),
        m_bInMeasure(FALSE),
        m_itemsPerLine(1),
        m_lineCount(0),
        m_bInManipulation(FALSE),
        m_bNotifyLayoutRefresh(FALSE),
        m_bNotifyHorizontalSnapPointsChanges(FALSE),
        m_bNotifyVerticalSnapPointsChanges(FALSE),
        m_bNotifiedHorizontalSnapPointsChanges(FALSE),
        m_bNotifiedVerticalSnapPointsChanges(FALSE),
        m_bAreSnapPointsKeysHorizontal(FALSE),
        m_lowerMarginSnapPointKey(0.0),
        m_upperMarginSnapPointKey(0.0),
        m_irregularSnapPointKeysOffset(0.0),
        m_regularSnapPointKey(0.0),
        m_fZoomFactor(1)
{
    m_arrangedItemsRect.X = 0;
    m_arrangedItemsRect.Y = 0;
    m_arrangedItemsRect.Width = 0;
    m_arrangedItemsRect.Height = 0;
}

OrientedVirtualizingPanel::~OrientedVirtualizingPanel()
{
    delete m_pTranslatedOffsetState;
    m_pTranslatedOffsetState = NULL;

    m_tpRealizedChildren.Clear();
}

// Supports the IManipulationDataProvider interface.
_Check_return_
HRESULT
OrientedVirtualizingPanel::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IManipulationDataProvider)))
    {
        *ppObject = static_cast<IManipulationDataProvider*>(this);
    }
    else
    {
        RRETURN(OrientedVirtualizingPanelGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Scroll content by one line to the top.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of what "line" means.
_Check_return_ HRESULT OrientedVirtualizingPanel::LineUpImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE pixelDelta = -ScrollViewerLineDelta;
    IFC(TranslateVerticalPixelDeltaToOffset(pixelDelta, offset));
    IFC(SetVerticalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the bottom.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of what "line" means.
_Check_return_ HRESULT OrientedVirtualizingPanel::LineDownImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE pixelDelta = ScrollViewerLineDelta;
    IFC(TranslateVerticalPixelDeltaToOffset(pixelDelta, offset));
    IFC(SetVerticalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the left.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of what "line" means.
_Check_return_ HRESULT OrientedVirtualizingPanel::LineLeftImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE pixelDelta = -ScrollViewerLineDelta;
    IFC(TranslateHorizontalPixelDeltaToOffset(pixelDelta, offset));
    IFC(SetHorizontalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the right.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of what "line" means.
_Check_return_ HRESULT OrientedVirtualizingPanel::LineRightImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE pixelDelta = ScrollViewerLineDelta;
    IFC(TranslateHorizontalPixelDeltaToOffset(pixelDelta, offset));
    IFC(SetHorizontalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the top.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of what "page" means.
_Check_return_ HRESULT OrientedVirtualizingPanel::PageUpImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE viewport = 0.0;
    IFC(get_VerticalOffset(&offset));
    IFC(get_ViewportHeight(&viewport));
    IFC(SetVerticalOffset(offset - viewport));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the bottom.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of what "page" means.
_Check_return_ HRESULT OrientedVirtualizingPanel::PageDownImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE viewport = 0.0;
    IFC(get_VerticalOffset(&offset));
    IFC(get_ViewportHeight(&viewport));
    IFC(SetVerticalOffset(offset + viewport));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the left.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of what "page" means.
_Check_return_ HRESULT OrientedVirtualizingPanel::PageLeftImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE viewport = 0.0;
    IFC(get_HorizontalOffset(&offset));
    IFC(get_ViewportWidth(&viewport));
    IFC(SetHorizontalOffset(offset - viewport));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the right.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of what "page" means.
_Check_return_ HRESULT OrientedVirtualizingPanel::PageRightImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE viewport = 0.0;
    IFC(get_HorizontalOffset(&offset));
    IFC(get_ViewportWidth(&viewport));
    IFC(SetHorizontalOffset(offset + viewport));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one mousewheel click to the top.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of the mouse wheel increment.
_Check_return_ HRESULT OrientedVirtualizingPanel::MouseWheelUpImpl()
{
    RRETURN(MouseWheelUp(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelUp implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
OrientedVirtualizingPanel::MouseWheelUp(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN passToScrollOwner = FALSE;

    IFC(get_ShouldPassWheelMessageToScrollOwner(passToScrollOwner));

    if (passToScrollOwner)
    {
        IFC(PassWheelMessageToScrollOwner(ZoomDirection_In));
    }
    else
    {
        xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
        DOUBLE offset = 0.0;
        wf::Size size;

        IFC(get_DesiredSize(&size));
        IFC(get_PhysicalOrientation(&orientation));
        if (orientation == xaml_controls::Orientation_Vertical)
        {
            DOUBLE pixelDelta = -GetVerticalScrollWheelDelta(size, mouseWheelDelta);
            IFC(TranslateVerticalPixelDeltaToOffset(pixelDelta, offset));
            IFC(SetVerticalOffset(offset));
        }
        else
        {
            DOUBLE pixelDelta = -GetHorizontalScrollWheelDelta(size, mouseWheelDelta);
            IFC(TranslateHorizontalPixelDeltaToOffset(pixelDelta, offset));
            IFC(SetHorizontalOffset(offset));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one mousewheel click to the bottom.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of the mouse wheel increment.
_Check_return_ HRESULT OrientedVirtualizingPanel::MouseWheelDownImpl()
{
    RRETURN(MouseWheelDown(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelDown implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
OrientedVirtualizingPanel::MouseWheelDown(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN passToScrollOwner = FALSE;

    IFC(get_ShouldPassWheelMessageToScrollOwner(passToScrollOwner));

    if (passToScrollOwner)
    {
        IFC(PassWheelMessageToScrollOwner(ZoomDirection_Out));
    }
    else
    {
        xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
        DOUBLE offset = 0.0;
        wf::Size size;

        IFC(get_DesiredSize(&size));
        IFC(get_PhysicalOrientation(&orientation));
        if (orientation == xaml_controls::Orientation_Vertical)
        {
            DOUBLE pixelDelta = GetVerticalScrollWheelDelta(size, mouseWheelDelta);
            IFC(TranslateVerticalPixelDeltaToOffset(pixelDelta, offset));
            IFC(SetVerticalOffset(offset));
        }
        else
        {
            DOUBLE pixelDelta = GetHorizontalScrollWheelDelta(size, mouseWheelDelta);
            IFC(TranslateHorizontalPixelDeltaToOffset(pixelDelta, offset));
            IFC(SetHorizontalOffset(offset));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one mousewheel click to the left.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of the mouse wheel increment.
_Check_return_ HRESULT OrientedVirtualizingPanel::MouseWheelLeftImpl()
{
    RRETURN(MouseWheelLeft(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelLeft implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
OrientedVirtualizingPanel::MouseWheelLeft(_In_ UINT mouseWheelDelta)
{
    DOUBLE offset = 0.0;
    wf::Size size;

    IFC_RETURN(get_DesiredSize(&size));
    DOUBLE pixelDelta = -GetHorizontalScrollWheelDelta(size, mouseWheelDelta);
    IFC_RETURN(TranslateHorizontalPixelDeltaToOffset(pixelDelta, offset));
    IFC_RETURN(SetHorizontalOffset(offset));

    return S_OK;
}

// Scroll content by one mousewheel click to the right.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of the mouse wheel increment.
_Check_return_ HRESULT OrientedVirtualizingPanel::MouseWheelRightImpl()
{
    RRETURN(MouseWheelRight(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelRight implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
OrientedVirtualizingPanel::MouseWheelRight(_In_ UINT mouseWheelDelta)
{
    DOUBLE offset = 0.0;
    wf::Size size;

    IFC_RETURN(get_DesiredSize(&size));
    DOUBLE pixelDelta = GetHorizontalScrollWheelDelta(size, mouseWheelDelta);
    IFC_RETURN(TranslateHorizontalPixelDeltaToOffset(pixelDelta, offset));
    IFC_RETURN(SetHorizontalOffset(offset));

    return S_OK;
}

// Set the HorizontalOffset to the passed value.
_Check_return_ HRESULT OrientedVirtualizingPanel::SetHorizontalOffsetImpl(_In_ DOUBLE offset)
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
        m_IndexToEnsureInView = -1;
        IFC(m_ScrollData.put_OffsetX(scrollX));
        IFC(IsInDirectManipulationZoom(bIsInDMZoom));
        if (!bIsInDMZoom)
        {
            IFC(InvalidateMeasure());
        }

        // in vsp, a scroll will trigger layout which will trigger transitions
        IFC(put_IsIgnoringTransitions(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// Set the VerticalOffset to the passed value.
_Check_return_ HRESULT OrientedVirtualizingPanel::SetVerticalOffsetImpl(_In_ DOUBLE offset)
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
        m_IndexToEnsureInView = -1;
        IFC(m_ScrollData.put_OffsetY(scrollY));
        IFC(IsInDirectManipulationZoom(bIsInDMZoom));
        if (!bIsInDMZoom)
        {
            IFC(InvalidateMeasure());
        }

        // in vsp, a scroll will trigger layout which will trigger transitions
        IFC(put_IsIgnoringTransitions(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// OrientedVirtualizingPanel implementation of its public MakeVisible method.
// Does not animate the move by default.
_Check_return_ HRESULT OrientedVirtualizingPanel::MakeVisibleImpl(
    // The element that should become visible.
    _In_ xaml::IUIElement* visual,
    // A rectangle representing in the visual's coordinate space to
    // make visible.
    wf::Rect rectangle,
    _Out_ wf::Rect* returnValue)
{
    IFC_RETURN(MakeVisibleImpl(
        visual,
        rectangle,
        FALSE /*useAnimation*/,
        DoubleUtil::NaN /*horizontalAlignmentRatio*/,
        DoubleUtil::NaN /*verticalAlignmentRatio*/,
        0.0 /*offsetX*/,
        0.0 /*offsetY*/,
        returnValue));
    return S_OK;
}

// OrientedVirtualizingPanel implementation of IScrollInfo.MakeVisible
// The goal is to change offsets to bring the child into view,
// and return a rectangle in our space to make visible.
// The rectangle we return is in the physical dimension the input target rect
// transformed into our pace.
// In the logical dimension, it is our immediate child's rect.
_Check_return_ HRESULT OrientedVirtualizingPanel::MakeVisibleImpl(
    // The element that should become visible.
    _In_ xaml::IUIElement* visual,
    // A rectangle representing in the visual's coordinate space to
    // make visible.
    wf::Rect rectangle,
    // When set to True, the DManip ZoomToRect method is invoked.
    BOOLEAN useAnimation,
    DOUBLE horizontalAlignmentRatio,
    DOUBLE verticalAlignmentRatio,
    DOUBLE offsetX,
    DOUBLE offsetY,
    _Out_ wf::Rect* resultRectangle,
    _Out_opt_ DOUBLE* appliedOffsetX,
    _Out_opt_ DOUBLE* appliedOffsetY)
{
    // Implementation of IScrollInfo.MakeVisible not supported by OrientedVirtualizingPanel at this point.

    *resultRectangle = DirectUI::RectUtil::CreateEmptyRect();
    return S_OK;
}

// OrientedVirtualizingPanel reacts to this property by changing its child measurement algorithm.
// If scrolling in a dimension, infinite space is allowed the child; otherwise, available size is preserved.
_Check_return_ HRESULT OrientedVirtualizingPanel::get_CanHorizontallyScrollImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_ScrollData.m_canHorizontallyScroll;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT OrientedVirtualizingPanel::put_CanHorizontallyScrollImpl(_In_ BOOLEAN value)
{
    HRESULT hr = S_OK;
    if (m_ScrollData.m_canHorizontallyScroll != value)
    {
        m_ScrollData.m_canHorizontallyScroll = value;
        IFC(InvalidateMeasure());
    }

Cleanup:
    RRETURN(hr);
}

// OrientedVirtualizingPanel reacts to this property by changing its child measurement algorithm.
// If scrolling in a dimension, infinite space is allowed the child; otherwise, available size is preserved.
_Check_return_ HRESULT OrientedVirtualizingPanel::get_CanVerticallyScrollImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_ScrollData.m_canVerticallyScroll;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT OrientedVirtualizingPanel::put_CanVerticallyScrollImpl(_In_ BOOLEAN value)
{
    HRESULT hr = S_OK;
    if (m_ScrollData.m_canVerticallyScroll != value)
    {
        m_ScrollData.m_canVerticallyScroll = value;
        IFC(InvalidateMeasure());
    }

Cleanup:
    RRETURN(hr);
}

// ExtentWidth contains the horizontal size of the scrolled content element
_Check_return_ HRESULT OrientedVirtualizingPanel::get_ExtentWidthImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_extent.Width;

    RRETURN(hr);
}

// ExtentHeight contains the vertical size of the scrolled content element
_Check_return_ HRESULT OrientedVirtualizingPanel::get_ExtentHeightImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_extent.Height;

    RRETURN(hr);
}

// ViewportWidth contains the horizontal size of content's visible range
_Check_return_ HRESULT OrientedVirtualizingPanel::get_ViewportWidthImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_viewport.Width;

    RRETURN(hr);
}

// ViewportHeight contains the vertical size of content's visible range
_Check_return_ HRESULT OrientedVirtualizingPanel::get_ViewportHeightImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_viewport.Height;

    RRETURN(hr);
}

// HorizontalOffset is the horizontal offset of the scrolled content
_Check_return_ HRESULT OrientedVirtualizingPanel::get_HorizontalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_ComputedOffset.X;

    RRETURN(hr);
}

// VerticalOffset is the vertical offset of the scrolled content
_Check_return_ HRESULT OrientedVirtualizingPanel::get_VerticalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_ComputedOffset.Y;

    RRETURN(hr);
}

// MinHorizontalOffset is the minimal horizontal offset of the scrolled content
_Check_return_ HRESULT OrientedVirtualizingPanel::get_MinHorizontalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_MinOffset.X;

    RRETURN(hr);
}

// MinVerticalOffset is the minimal vertical offset of the scrolled content
_Check_return_ HRESULT OrientedVirtualizingPanel::get_MinVerticalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_MinOffset.Y;

    RRETURN(hr);
}

// ScrollOwner is the container that controls any scrollbars, headers, etc... that are dependant
// on this IScrollInfo's properties.
_Check_return_ HRESULT OrientedVirtualizingPanel::get_ScrollOwnerImpl(
    _Outptr_ IInspectable** pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spOwner));

    IFC(spOwner.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT OrientedVirtualizingPanel::put_ScrollOwnerImpl(
    _In_opt_ IInspectable* value)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOldOwner;
    ctl::ComPtr<IScrollOwner> spNewOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spOldOwner));
    IFC(ctl::do_query_interface(spNewOwner, value));

    if (spOldOwner != spNewOwner)
    {
        IFC(ResetScrolling());
        IFC(m_ScrollData.put_ScrollOwner(spNewOwner.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// compute scroll offsets for Arrange
// TODO: Need to move this method to WrapGrid since VSP is not using it and has specific WrapGrid calculation
_Check_return_
HRESULT OrientedVirtualizingPanel::ComputeScrollOffsetForArrange(_Out_ wf::Point* pOffset)
{
    HRESULT hr = S_OK;
    wf::Point arrangeOffset = {0.0, 0.0};
    BOOLEAN isScrolling = FALSE;
    BOOLEAN isHorizontal = FALSE;
    FLOAT zoomFactor = 1.0;
    ctl::ComPtr<IScrollOwner> spOwner;

    IFCPTR(pOffset);

    IFC(m_ScrollData.get_ScrollOwner(&spOwner));

    //ScrollVector currentTransform = {TranslateTransform.X, TranslateTransform.Y};
    m_ScrollData.m_ArrangedOffset = m_ScrollData.m_ComputedOffset;
    IFC(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);
    //
    // Compute scroll offset and seed it into rcChild.
    //
    IFC(get_IsScrolling(&isScrolling));
    if (isScrolling)
    {
        xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
        IFC(get_PhysicalOrientation(&orientation));
        isHorizontal = orientation == xaml_controls::Orientation_Horizontal;

        FLOAT estimatedOffset = 0.0;
        IFC(ComputeUnrealizedChildrenEstimatedDimension(estimatedOffset));

        if (isHorizontal)
        {
            arrangeOffset.X = estimatedOffset;
        }
        else
        {
            arrangeOffset.Y = estimatedOffset;
        }
    }

    pOffset->X = arrangeOffset.X;
    pOffset->Y = arrangeOffset.Y;

#if OVP_DEBUG
    XCHAR szValue[250];
        IFCEXPECT(swprintf_s(szValue, 250, L"arrangeOffset.X - %f arrangeOffset.Y - %f m_bInManipulation - %d",
            arrangeOffset.X,
            arrangeOffset.Y,
            m_bInManipulation) >= 0);
        Trace(szValue);
#endif

Cleanup:
    RRETURN(hr);
}

// Called when the Items collection associated with the containing ItemsControl changes.
IFACEMETHODIMP
OrientedVirtualizingPanel::OnItemsChanged(
    _In_ IInspectable* sender,
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    BOOLEAN isScrolling = FALSE;

    IFC_RETURN(OrientedVirtualizingPanelGenerated::OnItemsChanged(sender, args));
    int intAction;
    IFC_RETURN(args->get_Action(&intAction));
    const auto action = static_cast<wfc::CollectionChange>(intAction);

    switch (action)
    {
        case wfc::CollectionChange_ItemInserted:
            IFC_RETURN(OnItemsAdd(args));
            break;

        case wfc::CollectionChange_ItemRemoved:
            IFC_RETURN(OnItemsRemove(args));
            break;

        case wfc::CollectionChange_ItemChanged:
            IFC_RETURN(OnItemsReplace(args));
            break;
    }

    IFC_RETURN(get_IsScrolling(&isScrolling));
    if (isScrolling)
    {
        // The items changed such that the maximum size may no longer be valid.
        // The next layout pass will update this value.
        wf::Size empty = {};
        m_ScrollData.m_MaxDesiredSize = empty;
    }

    // This rect is being used to provide items bounds.
    // Clear it every time when collection get update.
    // Thus items bounds will be invalidated and incremental loading will start if more items needed.
    m_arrangedItemsRect.X = 0;
    m_arrangedItemsRect.Y = 0;
    m_arrangedItemsRect.Width = 0;
    m_arrangedItemsRect.Height = 0;

    return S_OK;
}

// Called when the UI collection of children is cleared by the base Panel class.
IFACEMETHODIMP
OrientedVirtualizingPanel::OnClearChildren()
{
    HRESULT hr = S_OK;
    IFC(OrientedVirtualizingPanelGenerated::OnClearChildren());
    m_tpRealizedChildren.Clear();
    m_iVisibleStart = -1;
    m_iFirstVisibleChildIndex = -1;
    m_iBeforeTrail = m_iAfterTrail = m_iVisibleCount = 0;

Cleanup:
    RRETURN(hr);
}

// Gets the offset in pixels even for logical scrolling scenarios.
// When bUseProvidedLogicalOffset Offset is True, the logical offset provided is used.
_Check_return_
HRESULT
OrientedVirtualizingPanel::ComputePixelOffset(
    _In_ BOOLEAN isForHorizontalOrientation,
    _In_ BOOLEAN bUseProvidedLogicalOffset,
    _In_ DOUBLE logicalOffset,
    _Out_ DOUBLE& offset)
{
    ctl::ComPtr<xaml::IUIElement> spChild;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    FLOAT estimatedOffset = 0.0;
    DOUBLE physicalOffset = 0.0;
    DOUBLE fractionalItemOffset = 0.00;
    wf::Size childDesiredSize = {0.0, 0.0};
    FLOAT zoomFactor = 1.0;
    INT firstVisibleChildIndex = m_iFirstVisibleChildIndex;
    INT iBeforeTrail = m_iBeforeTrail;
    INT logicalOffsetDifference = 0;
    DOUBLE actualOffset = 0;
    ASSERT(!m_bItemBasedScrolling);

    // Dimension of unrealized children before realized ones:
    IFC_RETURN(ComputeUnrealizedChildrenEstimatedDimension(estimatedOffset));
    IFC_RETURN(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    // Dimension of realized children scrolled off:
    if (isForHorizontalOrientation)
    {
        IFC_RETURN(get_HorizontalOffset(&actualOffset));
    }
    else
    {
        IFC_RETURN(get_VerticalOffset(&actualOffset));
    }

    if (bUseProvidedLogicalOffset)
    {
        logicalOffsetDifference = static_cast<INT>(logicalOffset) - static_cast<INT>(actualOffset);
    }
    else
    {
        logicalOffset = actualOffset;
    }

    firstVisibleChildIndex += logicalOffsetDifference;
    iBeforeTrail += logicalOffsetDifference;

    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));

    fractionalItemOffset = DoubleUtil::Fractional(logicalOffset);

    INT iRealizedChildIndex = firstVisibleChildIndex - m_itemsPerLine;
    if (fractionalItemOffset > 0 && firstVisibleChildIndex >= 0)
    {
        UINT realizedChildrenSize = 0;

        IFC_RETURN(spRealizedChildren->get_Size(&realizedChildrenSize));
        if (static_cast<UINT>(firstVisibleChildIndex) < realizedChildrenSize)
        {
            IFC_RETURN(spRealizedChildren->GetAt(firstVisibleChildIndex, &spChild));
            if (spChild)
            {
                IFC_RETURN(GetDesiredSize(spChild.Get(), &childDesiredSize));
            }
            spChild = nullptr;
            physicalOffset = fractionalItemOffset * (isForHorizontalOrientation ? childDesiredSize.Width : childDesiredSize.Height);
        }
    }

    if (iBeforeTrail > 0)
    {
        while (iRealizedChildIndex >= firstVisibleChildIndex - iBeforeTrail && iRealizedChildIndex >= 0)
        {
            IFC_RETURN(spRealizedChildren->GetAt(iRealizedChildIndex, &spChild));
            if(spChild)
            {
                IFC_RETURN(GetDesiredSize(spChild.Get(), &childDesiredSize));
                spChild = nullptr;
                physicalOffset += isForHorizontalOrientation ? childDesiredSize.Width : childDesiredSize.Height;
            }
            iRealizedChildIndex = iRealizedChildIndex - m_itemsPerLine;
        }
    }

    offset = (DOUBLE) estimatedOffset + physicalOffset;
    offset *= zoomFactor;

    return S_OK;
}

// Returns the extent in logical units in the stacking direction.
_Check_return_
HRESULT
OrientedVirtualizingPanel::ComputeLogicalExtent(
    _In_ wf::Size stackDesiredSize,
    _In_ BOOLEAN isHorizontal,
    _Out_ wf::Size& logicalExtent)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    BOOLEAN accumulateExtent = FALSE;
    FLOAT zoomFactor = 1.0;
    logicalExtent.Height = 0;
    logicalExtent.Width = 0;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(spScrollOwner->IsInChildInvalidateMeasure(accumulateExtent));
    }

    IFC(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);
    if (isHorizontal)
    {
        logicalExtent.Width = static_cast<FLOAT>(m_lineCount);
        logicalExtent.Height = static_cast<FLOAT>(accumulateExtent ? DoubleUtil::Max(stackDesiredSize.Height * zoomFactor, m_ScrollData.m_extent.Height) : stackDesiredSize.Height * zoomFactor);
    }
    else
    {
        logicalExtent.Width = static_cast<FLOAT>(accumulateExtent ? DoubleUtil::Max(stackDesiredSize.Width * zoomFactor, m_ScrollData.m_extent.Width) : stackDesiredSize.Width * zoomFactor);
        logicalExtent.Height = static_cast<FLOAT>(m_lineCount);
    }

Cleanup:
    RRETURN(hr);
}

// Called when we ran out of children before filling up the viewport.
_Check_return_
HRESULT
OrientedVirtualizingPanel::GeneratePreviousItems(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _Inout_ DOUBLE& logicalVisibleSpace,
    _Inout_ wf::Size& stackDesiredSize,
    _In_ wf::Size layoutSlotSize,
    _In_ BOOLEAN isHorizontal,
    _In_ BOOLEAN adjustPositions,
    _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml::IUIElement> spChild;
    wf::Size childDesiredSize = {};
    BOOLEAN isScrolling = FALSE;
    IFC_RETURN(get_IsScrolling(&isScrolling));
    ASSERT(isScrolling, L"Only the scrolling panel can fill remaining space");

    DOUBLE childPixelOffset = 0.0;
    INT childIndex = m_iFirstVisibleChildIndex;
    UINT nCount = 0;
    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));

    if (childIndex >= 0 && childIndex < static_cast<INT>(nCount))
    {
        IFC_RETURN(spRealizedChildren->GetAt(childIndex, &spChild));

        // First visible child has already been measured. Desired size is valid
        IFC_RETURN(GetDesiredSize(spChild.Get(), &childDesiredSize));

        if (isHorizontal)
        {
            childPixelOffset = childDesiredSize.Width * DoubleUtil::Fractional(m_ScrollData.get_OffsetX());
            stackDesiredSize.Width += static_cast<FLOAT>(childPixelOffset);

            if (adjustPositions)
            {
                IFC_RETURN(m_ScrollData.put_OffsetX(DoubleUtil::Floor(m_ScrollData.get_OffsetX())));
                m_dPrecacheBeforeTrailSize = 0.0;
            }
        }
        else
        {
            childPixelOffset = childDesiredSize.Height * DoubleUtil::Fractional(m_ScrollData.get_OffsetY());
            stackDesiredSize.Height += static_cast<FLOAT>(childPixelOffset);

            if (adjustPositions)
            {
                IFC_RETURN(m_ScrollData.put_OffsetY(DoubleUtil::Floor(m_ScrollData.get_OffsetY())));
                m_dPrecacheBeforeTrailSize = 0.0;
            }
        }
        spChild = nullptr;
    }

    bool hasRemainingLogicalVisibleSpace = DoubleUtil::GreaterThan(static_cast<float>(logicalVisibleSpace), static_cast<float>(childPixelOffset));

    logicalVisibleSpace -= childPixelOffset;

    while ((hasRemainingLogicalVisibleSpace || (!adjustPositions && m_iBeforeTrail == 0))
        && (m_iVisibleCount + m_iBeforeTrail < maxNumOfItemsToGenerateInCurrentMeasureCycle))
    {
        BOOLEAN notPreviousChild = FALSE;
        // for childIndex==0 isGenerated will be false
        BOOLEAN isGenerated = FALSE;

        // Generate previous child for entire line
        for(UINT i = 0; i < m_itemsPerLine; ++i)
        {
            IFC_RETURN(PreviousChildIsGenerated(pItemsControlHint, childIndex, isGenerated));
            if (!isGenerated)
            {
                IFC_RETURN(GeneratePreviousChild(pItemsControlHint, childIndex, layoutSlotSize, &spChild));
                if (!spChild)
                {
                    notPreviousChild = TRUE;
                    break;
                }
                IFC_RETURN(GetDesiredSize(spChild.Get(), &childDesiredSize));
            }
            else
            {
                IFC_RETURN(spRealizedChildren->GetAt(--childIndex, &spChild));
                IFC_RETURN(MeasureChild(spChild.Get(), layoutSlotSize, &childDesiredSize));
            }
            spChild = nullptr;
        }

        if (notPreviousChild)
        {
            break;
        }

        if (isHorizontal)
        {
            if (adjustPositions)
            {
                IFC_RETURN(m_ScrollData.put_OffsetX(m_ScrollData.get_OffsetX() - 1));
            }
        }
        else
        {
            if (adjustPositions)
            {
                IFC_RETURN(m_ScrollData.put_OffsetY(m_ScrollData.get_OffsetY() - 1));
            }
        }

        // Account for the child in the panel's desired size
        if (isHorizontal)
        {
            hasRemainingLogicalVisibleSpace = DoubleUtil::GreaterThan(static_cast<float>(logicalVisibleSpace), childDesiredSize.Width);
            logicalVisibleSpace -= childDesiredSize.Width;
            stackDesiredSize.Width += childDesiredSize.Width;
            if (DoubleUtil::LessThan(logicalVisibleSpace, 0.0))
            {
                stackDesiredSize.Width += static_cast<FLOAT>(logicalVisibleSpace);
            }
            stackDesiredSize.Height = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Height, childDesiredSize.Height));
        }
        else
        {
            hasRemainingLogicalVisibleSpace = DoubleUtil::GreaterThan(static_cast<float>(logicalVisibleSpace), childDesiredSize.Height);
            logicalVisibleSpace -= childDesiredSize.Height;
            stackDesiredSize.Height += childDesiredSize.Height;
            if (DoubleUtil::LessThan(logicalVisibleSpace, 0.0))
            {
                stackDesiredSize.Height += static_cast<FLOAT>(logicalVisibleSpace);
            }
            stackDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Width, childDesiredSize.Width));
        }

        if (adjustPositions)
        {
            m_iVisibleCount += m_itemsPerLine;
        }
        else
        {
            m_dPrecacheBeforeTrailSize += isHorizontal ? childDesiredSize.Width : childDesiredSize.Height;
            m_iBeforeTrail += m_itemsPerLine;
        }
#if OVP_DEBUG
        WCHAR szValue[100];
        IFCEXPECT_RETURN(swprintf_s(szValue, 100, L"m_iBeforeTrail - %d m_iVisibleCount - %d",
            m_iBeforeTrail,
            m_iVisibleCount) >= 0);
        Trace(szValue);
# endif
    }

    if (DoubleUtil::IsInfinity(logicalVisibleSpace))
    {
        logicalVisibleSpace = 0.0;
        hasRemainingLogicalVisibleSpace = false;
    }

    if (adjustPositions)
    {
        if (DoubleUtil::LessThan(logicalVisibleSpace, 0.0))
        {
            if (isHorizontal)
            {
                IFC_RETURN(m_ScrollData.put_OffsetX(m_ScrollData.get_OffsetX() + 1 - (childDesiredSize.Width + logicalVisibleSpace) / childDesiredSize.Width));
            }
            else
            {
                IFC_RETURN(m_ScrollData.put_OffsetY(m_ScrollData.get_OffsetY() + 1 - (childDesiredSize.Height + logicalVisibleSpace) / childDesiredSize.Height));
            }

            m_dPrecacheBeforeTrailSize = -logicalVisibleSpace;
            logicalVisibleSpace = 0.0;
            hasRemainingLogicalVisibleSpace = false;
        }

        m_iFirstVisibleChildIndex = childIndex;
        m_iVisibleStart = 0;

        if (nCount != 0)
        {
            BOOLEAN isItemsHost = FALSE;
            IFC_RETURN(get_IsItemsHost(&isItemsHost));
            if (isItemsHost)
            {
                IFC_RETURN(GetGeneratedIndex(m_iFirstVisibleChildIndex, pItemsControlHint, m_iVisibleStart));
                m_iVisibleStart = m_iVisibleStart / m_itemsPerLine;
            }
        }
    }

    return S_OK;
}

// Called when we ran out of children before filling up the viewport.
_Check_return_
HRESULT
OrientedVirtualizingPanel::FillRemainingSpace(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _Inout_ DOUBLE& logicalVisibleSpace,
    _Inout_ wf::Size& stackDesiredSize,
    _In_ wf::Size layoutSlotSize,
    _In_ BOOLEAN isHorizontal,
    _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle)
{
    RRETURN(GeneratePreviousItems(pItemsControlHint, logicalVisibleSpace, stackDesiredSize, layoutSlotSize, isHorizontal, TRUE, maxNumOfItemsToGenerateInCurrentMeasureCycle));
}

// Updates ScrollData's offset, extent, and viewport in logical units.
_Check_return_
HRESULT
OrientedVirtualizingPanel::UpdateLogicalScrollData(
    _Inout_ wf::Size& stackDesiredSize,
    _In_ wf::Size constraint,
    _In_ DOUBLE logicalVisibleSpace,
    _In_ wf::Size extent,
    _In_ INT lastViewport,
    _In_ BOOLEAN isHorizontal)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml::IUIElement> spFirstVisibleChild;
    BOOLEAN isScrolling = FALSE;
    wf::Size viewport = constraint;
    ScrollVector offset = m_ScrollData.get_Offset();

    TraceUpdateLogicalScrollDataBegin();

    auto guard = wil::scope_exit([]()
    {
        TraceUpdateLogicalScrollDataEnd();
    });

    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));

    IFC_RETURN(get_IsScrolling(&isScrolling));
    ASSERT(isScrolling, L"this computes logical scroll data");

    // If we have not yet set the last child in the viewport, set it to the last child.
    if (lastViewport == -1)
    {
        lastViewport = m_lineCount - 1;
    }

    INT logicalExtent = m_lineCount;
    DOUBLE logicalViewport = lastViewport - m_iVisibleStart;

    UINT nCount = 0;
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));

    if (isHorizontal)
    {
        if (m_bItemBasedScrolling)
        {
            offset.X = static_cast<FLOAT>(m_iVisibleStart);
        }

        DOUBLE childWidth = 0.0;
        if (m_iFirstVisibleChildIndex >= 0 && m_iFirstVisibleChildIndex < static_cast<INT>(nCount))
        {
            wf::Size desiredSize = {};
            IFC_RETURN(spRealizedChildren->GetAt(m_iFirstVisibleChildIndex, &spFirstVisibleChild));
            IFC_RETURN(GetDesiredSize(spFirstVisibleChild.Get(), &desiredSize));
            childWidth = desiredSize.Width;
        }

        DOUBLE logicalDelta = m_bItemBasedScrolling ?
                                DoubleUtil::GreaterThanOrClose(logicalVisibleSpace, 0.0) ? 1 : 0
                                : (childWidth != 0 ? (childWidth + logicalVisibleSpace) / childWidth : 0.0) - DoubleUtil::Fractional(offset.X);

        viewport.Width = static_cast<FLOAT>(DoubleUtil::Max(logicalViewport + logicalDelta, 0));

        // In case last item is visible because we scroll all the way to the right and scrolling is on
        // we want desired size not to be smaller than constraint to avoid another relayout
        if (logicalExtent > viewport.Width && !DoubleUtil::IsPositiveInfinity(constraint.Width))
        {
            stackDesiredSize.Width = constraint.Width;
        }
    }
    else
    {
        if (m_bItemBasedScrolling)
        {
            offset.Y = static_cast<FLOAT>(m_iVisibleStart);
        }

        DOUBLE childHeight = 0.0;
        if (m_iFirstVisibleChildIndex >= 0 && m_iFirstVisibleChildIndex < static_cast<INT>(nCount))
        {
            wf::Size desiredSize = {};
            IFC_RETURN(spRealizedChildren->GetAt(m_iFirstVisibleChildIndex, &spFirstVisibleChild));
            IFC_RETURN(GetDesiredSize(spFirstVisibleChild.Get(), &desiredSize));
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
    }

    // Coerce the viewport offset.
    offset.X = MAX(0.0f, MIN(offset.X, extent.Width - viewport.Width));
    offset.Y = MAX(0.0f, MIN(offset.Y, extent.Height - viewport.Height));

    // Since we can offset and clip our content, we never need to be larger than the parent suggestion.
    // If we returned the full size of the content, we would always be so big we didn't need to scroll.  :)
    stackDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Min(stackDesiredSize.Width, constraint.Width));
    stackDesiredSize.Height = static_cast<FLOAT>(DoubleUtil::Min(stackDesiredSize.Height, constraint.Height));

    // When scrolling, the maximum horizontal or vertical size of items can cause the desired size of the
    // panel to change, which can cause the scrolling owner re-layout as well when it is not necessary.
    // We will thus remember the maximum desired size and always return that. The actual arrangement and
    // clipping still be calculated from actual scroll data values.
    // The maximum desired size is reset when the items change.
    if (isHorizontal)
    {
        m_ScrollData.m_MaxDesiredSize.Width = stackDesiredSize.Width;
        m_ScrollData.m_MaxDesiredSize.Height = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Height, m_ScrollData.m_MaxDesiredSize.Height));
    }
    else
    {
        m_ScrollData.m_MaxDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Width, m_ScrollData.m_MaxDesiredSize.Width));
        m_ScrollData.m_MaxDesiredSize.Height = stackDesiredSize.Height;
    }

    stackDesiredSize = m_ScrollData.m_MaxDesiredSize;

    // Verify Scroll Info, invalidate ScrollOwner if necessary.
    IFC_RETURN(SetAndVerifyScrollingData(viewport, extent, offset));

    return S_OK;
}

// Returns the index of the first item visible (even partially) in the viewport.
INT
OrientedVirtualizingPanel::ComputeIndexOfFirstVisibleItem(
    _In_ BOOLEAN isHorizontal,
    _Out_ DOUBLE& firstItemOffset)
{
    DOUBLE offset = isHorizontal ? m_ScrollData.get_OffsetX() : m_ScrollData.get_OffsetY();

    ASSERT(!DoubleUtil::IsNegativeInfinity(offset) && !DoubleUtil::IsPositiveInfinity(offset));

    // offset of the top of the first visible child from the top of the viewport.
    firstItemOffset = DoubleUtil::Fractional(offset);

    return static_cast<INT>(MAX(MIN(ceil(static_cast<XFLOAT>(m_nItemsCount) / static_cast<XFLOAT>(m_itemsPerLine)) - 1, offset), -1));
}

// Inserts a new container in the visual tree
_Check_return_
HRESULT
OrientedVirtualizingPanel::InsertNewContainer(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _In_ INT childIndex,
    _In_ xaml::IUIElement* pChild,
    _Inout_ BOOLEAN& visualOrderChanged)
{
    RRETURN(InsertContainer(pItemsControlHint, childIndex, pChild, FALSE, visualOrderChanged));
}

// Inserts a recycled container in the visual tree
_Check_return_
HRESULT
OrientedVirtualizingPanel::InsertRecycledContainer(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _In_ INT childIndex,
    _In_ xaml::IUIElement* pChild,
    _Inout_ BOOLEAN& visualOrderChanged)
{
    RRETURN(InsertContainer(pItemsControlHint, childIndex, pChild, TRUE, visualOrderChanged));
}

// Inserts a container into the Children collection.  The container is either new or recycled.
_Check_return_
HRESULT
OrientedVirtualizingPanel::InsertContainer(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _In_ INT childIndex,
    _In_ xaml::IUIElement* pChild,
    _In_ BOOLEAN isRecycled,
    _Inout_ BOOLEAN& visualOrderChanged)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml::IUIElement> spChildAtPosition;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;

    ASSERT(pChild, L"Null container was generated");
    IFCEXPECT_RETURN(pChild);

    IFC_RETURN(get_Children(&spChildren));

    //
    // Find the index in the Children collection where we hope to insert the container.
    // This is done by looking up the index of the container BEFORE the one we hope to insert.
    //
    // We have to do it this way because there could be recycled containers between the container we're looking for and the one before it.
    // By finding the index before the place we want to insert and adding one, we ensure that we'll insert the new container in the
    // proper location.
    //
    // In recycling mode childIndex is the index in the _realizedChildren list, not the index in the
    // Children collection.  We have to convert the index; we'll call the index in the Children collection
    // the visualTreeIndex.
    //

    INT visualTreeIndex = 0;
    BOOLEAN shouldInsertRealized = TRUE;

    if (childIndex > 0)
    {
        IFC_RETURN(ChildIndexFromRealizedIndex(childIndex - 1, visualTreeIndex));
        visualTreeIndex++;
    }

    UINT nCount = 0;
    IFC_RETURN(spChildren->get_Size(&nCount));

    if (isRecycled && visualTreeIndex < static_cast<INT>(nCount))
    {
        IFC_RETURN(spChildren->GetAt(visualTreeIndex, &spChildAtPosition));
        if (spChildAtPosition.Get()== pChild)
        {
            // Don't insert if a recycled container is in the proper place already
            shouldInsertRealized = FALSE;
        }
    }

    //
    // Keep realizedChildren in sync w/ the visual tree.
    // First do realized children, otherwise we end up
    // inserting children twice.
    //
    if (m_bIsVirtualizing && InRecyclingMode())
    {
        IFC_RETURN(get_RealizedChildren(&spRealizedChildren));
        IFC_RETURN(spRealizedChildren->InsertAt(childIndex, pChild));
    }

    if (shouldInsertRealized)
    {
        BOOLEAN hasParent = FALSE;
        if (visualTreeIndex < static_cast<INT>(nCount))
        {
            if (isRecycled)
            {
                IFC_RETURN(VisualTreeHelper::HasParentStatic(static_cast<UIElement*>(pChild), &hasParent));
                if (hasParent)
                {
                    // If the container is recycled we have to remove it from its place in the visual tree and
                    // insert it in the proper location.   For perf we'll use an internal Move API that moves
                    // the first parameter to right before the second one.
                    UINT nIndex = 0;
                    BOOLEAN bFound = FALSE;
                    IFC_RETURN(spChildren->IndexOf(pChild, &nIndex, &bFound));
                    ASSERT(bFound, L"Container not found.");
                    IFCEXPECT_RETURN(bFound);
                    IFC_RETURN(spChildren.Cast<UIElementCollection>()->MoveInternal(nIndex, MIN(visualTreeIndex, static_cast<INT>(nCount))));
                    visualOrderChanged = TRUE;
                }
                else
                {
                    IFC_RETURN(InsertInternalChild(visualTreeIndex, pChild));
                }
            }
            else
            {
                IFC_RETURN(InsertInternalChild(visualTreeIndex, pChild));
            }
        }
        else
        {
            if (isRecycled && nCount > 0)
            {
                IFC_RETURN(VisualTreeHelper::HasParentStatic(static_cast<UIElement*>(pChild), &hasParent));
                if (hasParent)
                {
                    // Recycled container is still in the tree; move it to the end
                    UINT nIndex = 0;
                    BOOLEAN bFound = FALSE;
                    IFC_RETURN(spChildren->IndexOf(pChild, &nIndex, &bFound));
                    ASSERT(bFound, L"Container not found.");
                    IFCEXPECT_RETURN(bFound);
                    IFC_RETURN(spChildren.Cast<UIElementCollection>()->MoveInternal(nIndex, nCount));
                    visualOrderChanged = TRUE;
                }
                else
                {
                    IFC_RETURN(AddInternalChild(pChild));
                }
            }
            else
            {
                IFC_RETURN(AddInternalChild(pChild));
            }
        }
    }

    IFC_RETURN(GetItemContainerGenerator(&spGenerator, pItemsControlHint));
    IFC_RETURN(spGenerator->PrepareItemContainer(static_cast<UIElement*>(pChild)));

    return S_OK;
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::PreviousChildIsGenerated(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _In_ INT childIndex,
    _Out_ BOOLEAN& isGenerated)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    xaml_primitives::GeneratorPosition position = {childIndex, 0};

    isGenerated = FALSE;

    IFC(GetItemContainerGenerator(&spGenerator, pItemsControlHint));
    IFC(spGenerator->IndexFromGeneratorPosition(position, &childIndex));

    if (childIndex > 0)
    {
        IFC(spGenerator->GeneratorPositionFromIndex(--childIndex, &position));
        isGenerated = (position.Offset == 0 && position.Index >= 0);
    }

Cleanup:
    RRETURN(hr);
}


// Takes a container returned from Generator.GenerateNext() and places it in the visual tree if necessary.
// Takes into account whether the container is new, recycled, or already realized.
_Check_return_
HRESULT
OrientedVirtualizingPanel::AddContainerFromGenerator(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _In_ INT childIndex,
    _In_ xaml::IUIElement* pChild,
    _In_ BOOLEAN newlyRealized,
    _Inout_ BOOLEAN& visualOrderChanged)
{
    TraceVirtualizationAddBegin();
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml::IUIElement> spChildAtPosition;

#if DBG
    //Debug.WriteLine("==============================Adding container=========================================");
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
#endif

    if (!newlyRealized)
    {
        //
        // Container is either realized or recycled.  If it's realized do nothing; it already exists in the visual
        // tree in the proper place.
        //

        if (InRecyclingMode())
        {
            UINT nCount = 0;
            BOOLEAN shouldInsertRealized = TRUE;
            // Note there's no check for IsVirtualizing here.  If the user has just flipped off virtualization it's possible that
            // the Generator will still return some recycled containers until its list runs out.
            IFC(get_RealizedChildren(&spChildren));
            IFC(spChildren->get_Size(&nCount));

            if (childIndex < static_cast<INT>(nCount))
            {
                IFC(spChildren->GetAt(childIndex, &spChildAtPosition));
                if (spChildAtPosition.Get()== pChild)
                {
                    shouldInsertRealized = FALSE;
                }
            }

            if (shouldInsertRealized)
            {
#if DBG
                UINT nIndex = 0;
                BOOLEAN bFound = FALSE;
                IFC(spChildren->IndexOf(pChild, &nIndex, &bFound));
                ASSERT(!bFound, L"we incorrectly identified a recycled container");
#endif
                //
                // We have a recycled container (if it was a realized container it would have been returned in the
                // proper location).  Note also that recycled containers are NOT in the _realizedChildren list.
                //
                IFC(InsertRecycledContainer(pItemsControlHint, childIndex, pChild, visualOrderChanged));
            }
        }
        else
        {
            // Not recycling; realized container
#if DBG
            IFC(get_Children(&spChildren));
            IFC(spChildren->GetAt(childIndex, &spChildAtPosition));
            ASSERT(pChild == spChildAtPosition.Get(), L"Wrong child was generated");
#endif
        }
    }
    else
    {
        IFC(InsertNewContainer(pItemsControlHint, childIndex, pChild, visualOrderChanged));
    }

#if DBG
    //Debug.WriteLine("==============================Container has been added=========================================");
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
#endif

    if (m_isGeneratingNewContainers)
    {
        IFC(OrientedVirtualizingPanelFactory::SetIsContainerGeneratedForInsertStatic(pChild, TRUE));
    }

Cleanup:
    TraceVirtualizationAddEnd();
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::GeneratePreviousChild(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _In_ INT childIndex,
    _In_ wf::Size layoutSlotSize,
    _Outptr_ xaml::IUIElement** ppChild)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
    ctl::ComPtr<xaml::IUIElement> spChild;
    xaml_primitives::GeneratorPosition position = {childIndex, 0};
    INT newIndex = 0;

    IFCPTR(ppChild);
    *ppChild = NULL;

    IFC(GetItemContainerGenerator(&spGenerator, pItemsControlHint));
    IFC(spGenerator->IndexFromGeneratorPosition(position, &newIndex));
    if (--newIndex >= 0)
    {
        BOOLEAN visualOrderChanged = FALSE;

        INT newGeneratedIndex = 0;
        IFC(IndexToGeneratorPositionForStart(pItemsControlHint, newIndex, newGeneratedIndex, position));
        IFC(spGenerator->StartAt(position, xaml_primitives::GeneratorDirection_Forward, TRUE));

        BOOLEAN newlyRealized = FALSE;
        IFC(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));
        IFC(spGenerator->Stop());

#if DBG
        if (!ctl::is<xaml::IUIElement>(spChildAsDO.Get()))
        {
            ASSERT(FALSE, L"Null child was generated");
        }
#endif

        IFC(spChildAsDO.As(&spChild));
        IFC(AddContainerFromGenerator(pItemsControlHint, childIndex, spChild.Get(), newlyRealized, visualOrderChanged));

        if (childIndex <= m_iFirstVisibleChildIndex)
        {
            m_iFirstVisibleChildIndex++;
        }

        IFC(MeasureChild(spChild.Get(), layoutSlotSize, NULL));

        if (visualOrderChanged)
        {
            ASSERT(m_bIsVirtualizing && InRecyclingMode(), L"We should only modify the visual order when in recycling mode");
        }

        IFC(spChild.CopyTo(ppChild));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::OnItemsAdd(
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
    ctl::ComPtr<xaml::IUIElement> spUIChild;
    xaml_primitives::GeneratorPosition position = {-1, 0};
    INT itemCount = 0;
    INT itemUICount = 0;
    INT itemIndex = 0;
    UINT childrenCount = 0;
    FLOAT cumulatedChildDim = 0.0;
    BOOLEAN bIsHorizontal = FALSE;
    FLOAT availableSize = 0.0;
    UINT nlineCount = 0;

    IFC(args->get_Position(&position));
    IFC(args->get_ItemCount(&itemCount));
    IFC(args->get_ItemUICount(&itemUICount));

    ASSERT(itemCount <= 1, L"Unexpected items count");
    ASSERT(itemUICount <= 1, L"Unexpected containers count");

    IFC(get_ItemContainerGenerator(&spGenerator));
    IFC(spGenerator->IndexFromGeneratorPosition(position, &itemIndex));

    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&childrenCount));

    if ((VisibleStartItemIndex() <= itemIndex
        && itemIndex <= VisibleStartItemIndex() + m_iVisibleCount)
        || childrenCount == 0)
    {
        BOOLEAN newlyRealized = FALSE;
        BOOLEAN visualOrderChanged = FALSE;
        IFC(spGenerator->StartAt(position, xaml_primitives::GeneratorDirection_Forward, TRUE));

        IFC(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));
        IFC(spGenerator->Stop());

        // now position should be realized position
        IFC(spGenerator->GeneratorPositionFromIndex(itemIndex, &position));

        if (!ctl::is<xaml::IUIElement>(spChildAsDO.Get()))
        {
            ASSERT(!newlyRealized, L"The generator realized a null value.");
            // We reached the end of the items (because of a group)
            goto Cleanup;
        }
        IFC(spChildAsDO.As(&spUIChild));

        IFC(AddContainerFromGenerator(NULL, position.Index, spUIChild.Get(), newlyRealized, visualOrderChanged));
        m_iFirstVisibleChildIndex = MAX(0, m_iFirstVisibleChildIndex);

        IFC(OrientedVirtualizingPanelFactory::SetIsContainerGeneratedForInsertStatic(spUIChild.Get(), TRUE));
        IFC(MeasureChildForItemsChanged(spUIChild.Get()));

        if (position.Index < m_iFirstVisibleChildIndex)
        {
            m_iVisibleStart++;
            m_iFirstVisibleChildIndex++;
            m_iBeforeTrail++;
        }
        else
        {
            IFC(ComputeTotalRealizedChildrenDimension(cumulatedChildDim, nlineCount));
            IFC(IsHorizontal(bIsHorizontal));
            if (bIsHorizontal)
            {
                availableSize = m_LastSetAvailableSize.Width;
            }
            else
            {
                availableSize = m_LastSetAvailableSize.Height;
            }

            // Increase visible count only if we still have space available in visible region.
            // This will prevent realizing all items being added if they were being added in a for loop.
            if (cumulatedChildDim < availableSize)
            {
                if (position.Index - m_iFirstVisibleChildIndex > m_iVisibleCount - m_iAfterTrail)
                {
                    m_iAfterTrail++;
                }

                m_iVisibleCount++;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_
HRESULT
OrientedVirtualizingPanel::OnItemsReplace(
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
    ctl::ComPtr<xaml::IUIElement> spUIChild;
    xaml_primitives::GeneratorPosition position = {-1, 0};
    INT itemCount = 0;
    INT itemUICount = 0;
    BOOLEAN newlyRealized = FALSE;
    BOOLEAN visualOrderChanged = FALSE;

    IFC(args->get_Position(&position));
    IFC(args->get_ItemCount(&itemCount));
    IFC(args->get_ItemUICount(&itemUICount));

    ASSERT(itemCount <= 1, L"Unexpected items count");
    ASSERT(itemUICount <= 1, L"Unexpected containers count");

    IFC(RemoveChildRange(position, itemCount, itemUICount));

    IFC(get_ItemContainerGenerator(&spGenerator));

    IFC(spGenerator->StartAt(position, xaml_primitives::GeneratorDirection_Forward, TRUE));

    IFC(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));
    IFC(spGenerator->Stop());

    IFCEXPECT(!newlyRealized);

    IFC(spChildAsDO.As(&spUIChild));
    IFC(AddContainerFromGenerator(NULL, position.Index, spUIChild.Get(), newlyRealized, visualOrderChanged));
    IFC(MeasureChildForItemsChanged(spUIChild.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::OnItemsRemove(
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    HRESULT hr = S_OK;
    xaml_primitives::GeneratorPosition position = {-1, 0};
    INT itemCount = 0;
    INT itemUICount = 0;

    IFC(args->get_Position(&position));
    IFC(args->get_ItemCount(&itemCount));
    IFC(args->get_ItemUICount(&itemUICount));
    IFC(RemoveChildRange(position, itemCount, itemUICount));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::RemoveChildRange(
    _In_ xaml_primitives::GeneratorPosition position,
    _In_ INT itemCount,
    _In_ INT itemUICount)
{
    HRESULT hr = S_OK;
    BOOLEAN isItemsHost = FALSE;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml::IUIElement> spVisualChild;
    ctl::ComPtr<xaml::IUIElement> spRealizedChild;

    IFC(get_IsItemsHost(&isItemsHost))
    if (isItemsHost)
    {
        INT pos = position.Index;
        if (position.Offset > 0)
        {
            // An item is being removed after the one at the index
            pos++;
        }

        UINT nCount = 0;
        IFC(get_Children(&spChildren));
        IFC(spChildren->get_Size(&nCount));

        if (pos < static_cast<INT>(nCount))
        {
            int uiCount = itemUICount;
            ASSERT((itemCount == itemUICount) || (itemUICount == 0), L"Both ItemUICount and ItemCount should be equal or ItemUICount should be 0.");
            if (uiCount > 0)
            {
                IFC(get_RealizedChildren(&spRealizedChildren));
                UINT nRealizedItemsCount = 0;
                IFC(spRealizedChildren->get_Size(&nRealizedItemsCount));

                // If there are no extra containers in visual tree, remove childRange from Pos
                if(nCount == nRealizedItemsCount)
                {
                    RemoveInternalChildRange(pos, uiCount);
                }
                else
                {
                    // Now there may be extra containers in visual tree, since we don't dispose them
                    // so the position of deleted container may be different in Visual Tree
                    // Realised items doesn't have extra containers, so to get the exact postion in Visual Tree,
                    // we need to get realized item and find same container in Visual Tree.
                    IFC(spRealizedChildren->GetAt(pos, &spRealizedChild));
                    for (UINT j = 0; j < nCount; ++j)
                    {
                        IFC(spChildren->GetAt(j, &spVisualChild));
                        if(spVisualChild == spRealizedChild)
                        {
                            RemoveInternalChildRange(j, uiCount);
                            break;
                        }
                    }
                }

                if (m_bIsVirtualizing && InRecyclingMode())
                {
                    for (int i = pos + uiCount; i > pos; )
                    {
                        IFC(spRealizedChildren->RemoveAt(--i));
                    }
                }

                if (position.Offset <= 0)
                {
                    if (pos >= m_iFirstVisibleChildIndex)
                    {
                        INT realVisibleItemCount = m_iVisibleCount - m_iAfterTrail;
                        if (pos > realVisibleItemCount)
                        {
                            m_iAfterTrail--;
                        }
                        m_iVisibleCount--;
                    }
                    else if (pos >= 0)
                    {
                        m_iBeforeTrail --;
                        --m_iFirstVisibleChildIndex;
                    }
                }
            }
        }
    }
Cleanup:
    RRETURN(hr);
}

void
OrientedVirtualizingPanel::AdjustCacheWindow()
{
    //
    // Adjust the container cache window such that the viewport is always contained inside.
    //

    // firstViewport is the index of the first container in the viewport, not counting the before trail.
    // m_iVisibleCount is the total number of items we generated. It already contains the _afterTrail.

    // First and last containers that we must keep in view; index is into the data item collection
    if (m_nItemsCount == 0)
    {
        m_iCacheStart = 0;
        return;
    }

    INT firstContainerIndex = MAX(0, VisibleStartItemIndex() - m_iBeforeTrail);
    INT lastContainerIndex = MAX(0, VisibleStartItemIndex() + m_iVisibleCount - 1);   // beforeTrail is not included in m_iVisibleCount
    if (lastContainerIndex >= static_cast<INT>(m_nItemsCount))
    {
        lastContainerIndex = m_nItemsCount - 1;
        firstContainerIndex = MAX(0, lastContainerIndex - m_iVisibleCount - m_iBeforeTrail + 1);
    }

    INT cacheEnd = MAX(0, m_iCacheStart + m_iBeforeTrail + m_iVisibleCount - 1);
    if (firstContainerIndex < m_iCacheStart)
    {
        // shift the cache start up
        m_iCacheStart = firstContainerIndex;
    }
    else if (lastContainerIndex > cacheEnd)
    {
        // shift the cache start down
        m_iCacheStart += (lastContainerIndex - cacheEnd);
    }

#if DBG
    cacheEnd = MAX(0, m_iCacheStart + m_iBeforeTrail + m_iVisibleCount - 1);
    // cacheEnd might be greater then m_nItemsCount in case of we used to cache all the containers and any number of items were deleted on runtime programmatically.
    ASSERT(m_iCacheStart <= firstContainerIndex && cacheEnd >= firstContainerIndex + m_iBeforeTrail + m_iVisibleCount - 1, L"The container cache window is out of place");
#endif
}

BOOLEAN
OrientedVirtualizingPanel::IsOutsideCacheWindow(_In_ INT itemIndex)
{
    return (itemIndex < m_iCacheStart || itemIndex >= m_iCacheStart + m_iBeforeTrail + m_iVisibleCount);
}

_Check_return_ HRESULT
OrientedVirtualizingPanel::IsInsideViewWindowLocationBased (
    _In_ IUIElement* pContainer,
    _In_ wf::Size constraint,
    _Out_ BOOLEAN& result)
{
    // used because a container might be animating and should still be visible
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spElement;
    ctl::ComPtr<LayoutInformation> spLayoutInformation;
    wf::Rect layoutSlot = {};
    wf::Rect constraintRect = {};

    IFC(ctl::make<LayoutInformation>(&spLayoutInformation));

    IFC(ctl::do_query_interface(spElement, pContainer));

    IFC(spLayoutInformation->GetLayoutSlot(spElement.Get(), &layoutSlot));

    // If the item doesn't have a layout width and height its not in the view window.
    // This would happen when we measure more items in first measure pass because the available size information was wrong
    // and in second measure pass we should recycle them when we have the right available size but since they have never been laid out
    // their layoutSlot height and width comes out to be 0 and they could still intersect with the constraint rect.
    if (layoutSlot.Height <= 0 && layoutSlot.Width <= 0)
    {
        result = FALSE;
        goto Cleanup;
    }

    constraintRect.Height = constraint.Height;
    constraintRect.Width = constraint.Width;

    IFC(RectUtil::Intersect(layoutSlot, constraintRect));
    result = !DoubleUtil::IsInfinity(layoutSlot.X);

Cleanup:
    RRETURN(hr);
}

BOOLEAN
OrientedVirtualizingPanel::IsInsideViewWindowIndexBased(_In_ INT containerIndex)
{
    return (containerIndex >= m_iBeforeTrail && containerIndex < m_iBeforeTrail + m_iVisibleCount - m_iAfterTrail); // visiblecount includes aftertrail
}

// Immediately cleans up any containers that have gone offscreen.  Called by MeasureOverride.
// When recycling this runs before generating and measuring children; otherwise it runs after.
_Check_return_
HRESULT
OrientedVirtualizingPanel::CleanupContainers(
    _In_ xaml_controls::IItemsControl* pItemsControl,
    _In_ wf::Size constraint)
{
    TraceVirtualizationCleanupBegin();

    auto guard = wil::scope_exit([]()
    {
        TraceVirtualizationCleanupEnd();
    });

    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl = pItemsControl;
    ctl::ComPtr<xaml_controls::IListViewBase> spListViewBase = nullptr;
    xaml_primitives::GeneratorPosition startPos = {-1, 0};


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
    INT focusedChild = -1;  // child indices for the focused item

    BOOLEAN performCleanup = FALSE;

    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));

#if DBG
    //Trace(L"==============================Cleanup containers========================================");
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
#endif

    UINT nCount = 0;
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));

    if (nCount == 0)
    {
        return S_OK;// nothing to do
    }

    AdjustCacheWindow();

    IFC_RETURN(FindFocusedChildInRealizedChildren(focusedChild));

    BOOLEAN bIsIgnoringTransitions = FALSE;
    IFC_RETURN(get_IsIgnoringTransitions(&bIsIgnoringTransitions));

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
        IFC_RETURN(GetGeneratedIndex(childIndex, pItemsControl, itemIndex));

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

                IFC_RETURN(CleanupRange(pItemsControl, cleanupRangeStart, cleanupCount));
                IFC_RETURN(spRealizedChildren->get_Size(&nCount));

                // CleanupRange just modified the _realizedChildren list.  Adjust the childIndex.
                childIndex -= cleanupCount;
                focusedChild -= cleanupCount;

                cleanupCount = 0;
                cleanupRangeStart = -1;
            }
        }

        // Assume non-recyclable container;
        performCleanup = TRUE;

        if (IsOutsideCacheWindow(itemIndex) &&
            childIndex != focusedChild)
        {
            BOOLEAN isInsideViewWindow = FALSE;
            ctl::ComPtr<xaml::IUIElement> spChild = nullptr;

            IFC_RETURN(spRealizedChildren->GetAt(childIndex, &spChild));

            // Only check to see if an Item is inside the view window if we are not ignoring transitions.

            // so this is about elements that are not currently being transitioned, but they might have been
            // moved offscreen by the insertion of a bunch of other elements. This might push them off the
            // cachewindow, but in truth they are still there, waiting for a transition to move them
            // to their new location.

            // notice that we do not differentiate between transitioning items or not transitioning items
            if (!bIsIgnoringTransitions)
            {
                IFC_RETURN(IsInsideViewWindowLocationBased(spChild.Get(), constraint, isInsideViewWindow));
            }

            if (!isInsideViewWindow)
            {
                ctl::ComPtr<IInspectable> spItem;
                BOOLEAN isOwnContainer = FALSE;
                if (!spItems)
                {
                    IFC_RETURN(GetItemContainerGenerator(&spGenerator, pItemsControl));
                    IFC_RETURN(spGenerator.Cast<ItemContainerGenerator>()->get_View(&spItems));
                }

                IFC_RETURN(spItems->GetAt(itemIndex, &spItem));
                if (!ctl::value_is<xaml_controls::IGroupItem>(spItem.Get()))
                {
                    IFC_RETURN(static_cast<ItemsControl*>(pItemsControl)->IsItemItsOwnContainer(spItem.Get(), &isOwnContainer));
                }

                if (!isOwnContainer)
                {
                    BOOLEAN isContainerDragDropOwner = FALSE;
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

    }

    if (cleanupRangeStart >= 0 && cleanupCount > 0)
    {
        IFC_RETURN(CleanupRange(pItemsControl, cleanupRangeStart, cleanupCount));

        //
        // Figure out the position of the first visible item
        //
        IFC_RETURN(IndexToGeneratorPositionForStart(pItemsControl, VisibleStartItemIndex(), m_iFirstVisibleChildIndex, startPos));
    }

#if DBG
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
    //Trace(L"==============================Containers cleaned========================================");
#endif

    return S_OK;
}

_Check_return_
 HRESULT
 OrientedVirtualizingPanel::EnsureRealizedChildren()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<TrackerCollection<xaml::UIElement*>> spRealizedChildren;
    ASSERT(InRecyclingMode(), L"This method only applies to recycling mode");

    if (!m_tpRealizedChildren)
    {
        UINT nCount = 0;
        IFC(get_Children(&spChildren));
        IFC(ctl::make<TrackerCollection<xaml::UIElement*>>(&spRealizedChildren));

        IFC(spChildren->get_Size(&nCount));
        for (UINT i = 0; i < nCount; i++)
        {
            ctl::ComPtr<xaml::IUIElement> spChild;

            IFC(spChildren->GetAt(i, &spChild));
            IFC(spRealizedChildren->Append(spChild.Get()));
        }

        SetPtrValue(m_tpRealizedChildren, spRealizedChildren);
    }

Cleanup:
    RRETURN(hr);
}

#if DBG
// Debug method that ensures the _realizedChildren list matches the realized containers in the Generator.
_Check_return_
 HRESULT
 OrientedVirtualizingPanel::debug_VerifyRealizedChildren()
{
    HRESULT hr = S_OK;
    ASSERT(m_bIsVirtualizing && InRecyclingMode(), L"Realized children only exist when recycling");
    ASSERT(m_tpRealizedChildren, L"Realized children must exist to verify it");
    //ItemsControl itemsControl = ItemsControl.GetItemsOwner(this);

    //if (ItemContainerGenerator != null && itemsControl != null)
    //{
    //    foreach (UIElement child in Children)
    //    {
    //        int dataIndex = ((ItemContainerGenerator)ItemContainerGenerator).IndexFromContainer(child);

    //        if (dataIndex == -1)
    //        {
    //            // Child is not in the generator's realized container list (i.e. it's a recycled container): ensure it's NOT in _realizedChildren.
    //            Debug.Assert(!_realizedChildren.Contains(child), "_realizedChildren should not contain recycled containers");
    //        }
    //        else
    //        {
    //            // Child is a realized container; ensure it's in _realizedChildren at the proper place.
    //            GeneratorPosition position = ((ItemContainerGenerator)ItemContainerGenerator).GeneratorPositionFromIndex(dataIndex);
    //            Debug.Assert(_realizedChildren[position.Index] == child, "_realizedChildren is corrupt!");
    //        }
    //    }
    //}
    RRETURN(hr);
}

_Check_return_
 HRESULT
 OrientedVirtualizingPanel::debug_AssertRealizedChildrenEqualVisualChildren()
{
    HRESULT hr = S_OK;

    UINT nVisualCount = 0;
    UINT nRealizedCount = 0;
    if (m_bIsVirtualizing && InRecyclingMode())
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

        IFC(get_Children(&spChildren));
        IFC(spChildren->get_Size(&nVisualCount));
        if (m_tpRealizedChildren)
        {
            IFC(m_tpRealizedChildren->get_Size(&nRealizedCount));
        }

        IFCEXPECT_ASSERT(nRealizedCount <= nVisualCount);

        for (UINT i = 0; i < nRealizedCount; i++)
        {
            ctl::ComPtr<xaml::IUIElement> spVisualChild;
            ctl::ComPtr<xaml::IUIElement> spRealizedChild;

            IFC(spChildren->GetAt(i, &spVisualChild));
            IFC(m_tpRealizedChildren->GetAt(i, &spRealizedChild));
            IFCEXPECT_ASSERT(spRealizedChild == spVisualChild);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
 HRESULT
 OrientedVirtualizingPanel::debug_DumpRealizedChildren()
{
    HRESULT hr = S_OK;

    Trace(L"===================================Realized Children====================================");
    if (m_bIsVirtualizing && InRecyclingMode())
    {
        if (!m_tpRealizedChildren)
        {
            Trace(L"RealizedChildren list haven't been created yet");
        }
        UINT nCount = 0;
        IFC(m_tpRealizedChildren->get_Size(&nCount));
        for (UINT i = 0; i < nCount; i++)
        {
            ctl::ComPtr<xaml::IUIElement> spContainer;

            IFC(m_tpRealizedChildren->GetAt(i, &spContainer));
            if (!ctl::is<xaml_controls::IListBoxItem>(spContainer.Get()))
            {
                //Trace(L"Pos[%d]: Unexpected container", i);
            }
            else
            {
                //Trace(L"Pos[%d]: ListBoxItem Item={1} Content={2}", i);
            }
        }
    }
    Trace(L"===================================Realized Children====================================");

Cleanup:
    RRETURN(hr);
}

_Check_return_
 HRESULT
 OrientedVirtualizingPanel::debug_DumpVisualChildren()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    Trace(L"===================================Visual Children====================================");
    if (m_bIsVirtualizing && InRecyclingMode())
    {
        UINT nCount = 0;
        IFC(get_Children(&spChildren));
        IFC(spChildren->get_Size(&nCount));
        for (UINT i = 0; i < nCount; i++)
        {
            ctl::ComPtr<xaml::IUIElement> spContainer;

            IFC(spChildren->GetAt(i, &spContainer));
            if (!ctl::is<xaml_controls::IListBoxItem>(spContainer.Get()))
            {
                //Trace(L"Pos[%d]: Unexpected container", i);
            }
            else
            {
                //Trace(L"Pos[%d]: ListBoxItem Item={1} Content={2}", i);
            }
        }
    }
    Trace(L"===================================Visual Children====================================");

Cleanup:
    RRETURN(hr);
}
#endif

// Takes an index from the realized list and returns the corresponding index in the Children collection
_Check_return_
HRESULT
OrientedVirtualizingPanel::ChildIndexFromRealizedIndex(
    _In_ INT realizedChildIndex,
    _Out_ INT& childIndex)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml::IUIElement> spRealizedChild;

    childIndex = realizedChildIndex;
    //
    // If we're not recycling containers then we're not using a realizedChild index and no translation is necessary
    //
    if (m_bIsVirtualizing && InRecyclingMode())
    {
        UINT nCount = 0;
        IFC(get_RealizedChildren(&spRealizedChildren));
        IFC(spRealizedChildren->get_Size(&nCount));

        if (realizedChildIndex < static_cast<INT>(nCount))
        {
            IFC(spRealizedChildren->GetAt(realizedChildIndex, &spRealizedChild));
            IFC(get_Children(&spChildren));
            IFC(spChildren->get_Size(&nCount));

            for (UINT i = realizedChildIndex; i < nCount; i++)
            {
                ctl::ComPtr<xaml::IUIElement> spChild;

                IFC(spChildren->GetAt(i, &spChild));
                if (spChild == spRealizedChild)
                {
                    childIndex = static_cast<INT>(i);
                    goto Cleanup;
                }
            }

            ASSERT(FALSE, "We should have found a child");
        }
    }
Cleanup:
    RRETURN(hr);
}

// Recycled containers still in the Children collection at the end of Measure should be moved to the end of Children collection
_Check_return_
HRESULT
OrientedVirtualizingPanel::CollectRecycledContainers()
{
    HRESULT hr = S_OK;

    UINT realizedIndex = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;

    UINT nVisualCount = 0;
    UINT nRealizedCount = 0;

    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&nVisualCount));

    if (nVisualCount > 0)
    {
        IFC(get_RealizedChildren(&spRealizedChildren));
        IFC(spRealizedChildren->get_Size(&nRealizedCount));

        for (UINT i = 0; i < nVisualCount; i++)
        {
            ctl::ComPtr<xaml::IUIElement> spVisualChild;

            IFC(spChildren->GetAt(i, &spVisualChild));
            if (nRealizedCount > realizedIndex)
            {
                ctl::ComPtr<xaml::IUIElement> spRealizedChild;

                IFC(spRealizedChildren->GetAt(realizedIndex, &spRealizedChild));

                if (spVisualChild == spRealizedChild)
                {
                    realizedIndex++;
                }
                else
                {
                    // The visual child is a recycled container
                    IFC(spChildren.Cast<UIElementCollection>()->MoveInternal(i--, nVisualCount));
                }
            }
        }
    }

#if DBG
    //Debug.WriteLine("==============================Post-disconnect state=====================================");
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
    //Debug.WriteLine("=======================Recycled containers disconnected=================================");

    IFC(debug_VerifyRealizedChildren());
    IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::IndexToGeneratorPositionForStart(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _In_ INT index,
    _Out_ INT& childIndex,
    _Out_ xaml_primitives::GeneratorPosition& position)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;

    IFC(GetItemContainerGenerator(&spGenerator, pItemsControlHint));

    childIndex = 0;
    position.Index = -1;
    position.Offset = 0;

    if (spGenerator)
    {
        IFC(spGenerator->GeneratorPositionFromIndex(index, &position));
    }
    else
    {
        position.Index = -1;
        position.Offset = index + 1;
    }

    // determine the position in the children collection for the first
    // generated container.  This assumes that generator.StartAt will be called
    // with direction=Forward and  allowStartAtRealizedItem=TRUE.
    childIndex = (position.Offset == 0) ? position.Index : position.Index + 1;

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::NotifyCleanupItem(
    _In_ IInspectable* pItem,
    _In_ xaml::IUIElement* pChild,
    _In_ xaml_controls::IItemsControl* pItemsControl,
    _Out_ BOOLEAN& bCanceled)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CleanUpVirtualizedItemEventArgs> spArgs;

    bCanceled = FALSE;

    // Create the args
    IFC(ctl::make<CleanUpVirtualizedItemEventArgs>(&spArgs));
    IFC(spArgs->put_Value(pItem));
    IFC(spArgs->put_UIElement(pChild));
    IFC(spArgs->put_Cancel(FALSE));

    IFC(OnCleanUpVirtualizedItemProtected(spArgs.Get()));

    IFC(spArgs->get_Cancel(&bCanceled));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::OnCleanUpVirtualizedItemProtected(_In_ xaml_controls::ICleanUpVirtualizedItemEventArgs* e)
{
    HRESULT hr = S_OK;

    RRETURN(hr);
}
//
//_Check_return_
//HRESULT
//OrientedVirtualizingPanel::FireOnCleanupVirtualizedItems(_In_ xaml_controls::ICleanUpVirtualizedItemEventArgs* pArgs)
//{
//    HRESULT hr = S_OK;
//
//    RRETURN(hr);
//}

_Check_return_
HRESULT
OrientedVirtualizingPanel::CleanupRange(
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _In_ INT startIndex,
    _In_ INT count)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    xaml_primitives::GeneratorPosition position = {startIndex, 0};
    ctl::ComPtr<IUIElement> spChild;

    IFC(GetItemContainerGenerator(&spGenerator, pItemsControlHint));

    if (InRecyclingMode())
    {
        ASSERT(startIndex >= 0 && count > 0);
        IFC(get_RealizedChildren(&spRealizedChildren));

        // 1. cancel any transition on this element
        // 2. set lifecycle of these elements to unloaded
        for (INT unloadingIndex = startIndex; unloadingIndex < startIndex + count; unloadingIndex++)
        {
            IFC(spRealizedChildren->GetAt(unloadingIndex, &spChild));

            // 1. we cancel because this improves our performance dramatically.
            // the reason we would have liked to keep the elements transitioning (and basically not recycle them)
            // is that we would like the element to still be transitioning if the user moves the screen a bit and back
            // however, this means we can't use the container for recycling and we will have to generate a new one
            // which is prohibitively costly.
            IFC(CoreImports::UIElement_CancelTransition(static_cast<CUIElement*>(spChild.Cast<UIElement>()->GetHandle())));

            // 2. simulate a leave, which is what we are effectively doing by cleaning up this range
            IFC(CoreImports::UIElement_SetIsLeaving(static_cast<CUIElement*>(spChild.Cast<UIElement>()->GetHandle()), true));
        }

        IFC(spGenerator->Recycle(position, count));

#if OVP_DEBUG
        WCHAR szValue[250];
        IFCEXPECT(swprintf_s(szValue, 250, L"Cleanup Range: Count: %d startIndex: %d",
            count,
            startIndex) >= 0);
        Trace(szValue);
#endif

        // The call to Recycle has caused the ItemContainerGenerator to remove some items
        // from its list of realized items; we adjust _realizedChildren to match.
        for (INT i = startIndex + count - 1; i >= startIndex; i--)
        {
#if DBG
            IFC(spRealizedChildren->GetAt(i, &spChild));
            if (ctl::is<xaml::IFrameworkElement>(spChild.Get()))
            {
                BOOLEAN hasFocus = FALSE;
                IFC(spChild.Cast<FrameworkElement>()->HasFocus(&hasFocus));
                ASSERT(!hasFocus, L"Recycling focused item");
            }
#endif
            IFC(spRealizedChildren->RemoveAt(i));
        }
    }
    else
    {
        // by removing, we'll actually cancel transitions (leave impl on the core)
        IFC(spGenerator->Remove(position, count));
        IFC(RemoveInternalChildRange(startIndex, count));
    }
    AdjustFirstVisibleChildIndex(startIndex, count);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
OrientedVirtualizingPanel::get_ShouldPassWheelMessageToScrollOwner(_Out_ BOOLEAN &shouldPass)
{
    HRESULT hr = S_OK;
    wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;

    IFC(Control::GetKeyboardModifiers(&modifiers));

Cleanup:
    shouldPass = IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Control);
    RRETURN(hr);
}

_Check_return_ HRESULT
OrientedVirtualizingPanel::PassWheelMessageToScrollOwner(_In_ ZoomDirection zoomDirection)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spOwner));
    if (spOwner)
    {
        IFC(spOwner->ProcessPureInertiaInputMessage(zoomDirection));
    }

Cleanup:
    RRETURN(hr);
}

// Called after 'count' items were removed or recycled from the Generator.  m_iFirstVisibleChildIndex is the
// index of the first visible container.  This index isn't exactly the child position in the UIElement collection;
// it's actually the index of the realized container inside the generator.  Since we've just removed some realized
// containers from the generator (by calling Remove or Recycle), we have to adjust the first visible child index.
void
OrientedVirtualizingPanel::AdjustFirstVisibleChildIndex(_In_ INT startIndex, _In_ INT count)
{
    // Update the index of the first visible generated child
    if (startIndex < m_iFirstVisibleChildIndex)
    {
        int endIndex = startIndex + count - 1;
        if (endIndex < m_iFirstVisibleChildIndex)
        {
            // The first visible index is after the items that were removed
            m_iFirstVisibleChildIndex -= count;
        }
        else
        {
            // The first visible index was within the items that were removed
            m_iFirstVisibleChildIndex = startIndex;
        }
    }
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::MeasureChild(
    _In_ xaml::IUIElement* pChild,
    _In_ wf::Size layoutSlotSize,
    _Out_opt_ wf::Size* returnValue)
{
    TraceMeasureChildBegin();
    HRESULT hr = S_OK;
    wf::Size childDesiredSize = {};
    wf::Size updatedDesiredSize = {};

    IFC(pChild->get_DesiredSize(&childDesiredSize));
    IFC(pChild->Measure(layoutSlotSize));

    IFC(GetDesiredSize(pChild, &updatedDesiredSize));

    if (childDesiredSize.Height != updatedDesiredSize.Height ||
        childDesiredSize.Width != updatedDesiredSize.Width)
    {
        childDesiredSize = updatedDesiredSize;

        // Reset the _maxDesiredSize cache if child DesiredSize changes
        wf::Size empty = {};
        m_ScrollData.m_MaxDesiredSize = empty;
    }

    if (returnValue)
    {
        *returnValue = childDesiredSize;
    }

Cleanup:
    TraceMeasureChildEnd();
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::ResetScrolling()
{
    HRESULT hr = S_OK;
    BOOLEAN isScrolling = FALSE;
    IFC(InvalidateMeasure());

    // Clear scrolling data.  Because of thrash (being disconnected & reconnected, &c...), we may
    IFC(get_IsScrolling(&isScrolling));
    if (isScrolling)
    {
        m_ScrollData.ClearLayout();
    }

Cleanup:
    RRETURN(hr);
}

// OnScrollChange is an override called whenever the IScrollInfo exposed scrolling state changes on this element.
// At the time this method is called, scrolling state is in its new, valid state.
_Check_return_
HRESULT
OrientedVirtualizingPanel::OnScrollChange()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(spScrollOwner->InvalidateScrollInfoImpl());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::SetAndVerifyScrollingData(
    _In_ wf::Size viewport,
    _In_ wf::Size extent,
    _In_ ScrollVector offset)
{
    BOOLEAN isScrolling = FALSE;

    IFC_RETURN(get_IsScrolling(&isScrolling));
    ASSERT(isScrolling);

    // Detect changes to the viewport, extent, and offset
    BOOLEAN viewportChanged = !DoubleUtil::AreClose(viewport.Height, m_ScrollData.m_viewport.Height)
        || !DoubleUtil::AreClose(viewport.Width, m_ScrollData.m_viewport.Width);

    BOOLEAN extentChanged = !DoubleUtil::AreClose(extent.Height, m_ScrollData.m_extent.Height)
        || !DoubleUtil::AreClose(extent.Width, m_ScrollData.m_extent.Width);

    BOOLEAN offsetChanged = !DoubleUtil::AreClose(offset.X, m_ScrollData.m_ComputedOffset.X)
        || !DoubleUtil::AreClose(offset.Y, m_ScrollData.m_ComputedOffset.Y);

    // Update data and fire scroll change notifications
    IFC_RETURN(m_ScrollData.put_Offset(offset));
    if (viewportChanged || extentChanged || offsetChanged)
    {
        m_ScrollData.m_viewport = viewport;
        m_ScrollData.m_extent = extent;
        m_ScrollData.m_ComputedOffset = offset;

        IFC_RETURN(OnScrollChange());
    }

    return S_OK;
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::ComputePhysicalFromLogicalOffset(
    _In_ INT logicalOffset,
    _In_ DOUBLE fractionalItemOffset,
    _In_ BOOLEAN isHorizontal,
    _Out_ DOUBLE& physicalOffset)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml::IUIElement> spChild;
    wf::Size firstVisibleChildSize = {};

    physicalOffset = 0.0;

    DOUBLE offset = 0.0;
    if (isHorizontal)
    {
        IFC_RETURN(get_HorizontalOffset(&offset));
    }
    else
    {
        IFC_RETURN(get_VerticalOffset(&offset));
    }

    INT firstVisibleChildIndex = m_iFirstVisibleChildIndex + static_cast<INT>(offset) - m_iVisibleStart;
    UINT nCount = 0;

    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));
    if (firstVisibleChildIndex >= 0 && static_cast<INT>(nCount) > firstVisibleChildIndex)
    {
        IFC_RETURN(spRealizedChildren->GetAt(firstVisibleChildIndex, &spChild));
        IFC_RETURN(GetDesiredSize(spChild.Get(), &firstVisibleChildSize));
    }

    physicalOffset = m_bItemBasedScrolling ? 0.0 : -1 * fractionalItemOffset * (isHorizontal ? firstVisibleChildSize.Width : firstVisibleChildSize.Height);
    ASSERT(logicalOffset == 0 || logicalOffset < static_cast<INT>(nCount));

    for (INT i = 0; i < logicalOffset; i++)
    {
        wf::Size childDesiredSize = {};
        IFC_RETURN(spRealizedChildren->GetAt(i, &spChild));
        IFC_RETURN(GetDesiredSize(spChild.Get(), &childDesiredSize));
        physicalOffset -= isHorizontal
            ? childDesiredSize.Width
            : childDesiredSize.Height;
    }

    return S_OK;
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::GetGeneratedIndex(
    _In_ INT childIndex,
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
    _Out_ INT& generatedIndex)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    xaml_primitives::GeneratorPosition position = {childIndex, 0};

    generatedIndex = 0;

    IFC(GetItemContainerGenerator(&spGenerator, pItemsControlHint));
    IFC(spGenerator->IndexFromGeneratorPosition(position, &generatedIndex));

Cleanup:
    RRETURN(hr);
}

// Finds the focused child along with the previous and next focusable children.  Used only when recycling containers;
// the standard mode has a different cleanup algorithm
_Check_return_
HRESULT
OrientedVirtualizingPanel::FindFocusedChildInRealizedChildren(
    _Out_ INT& focusedChild)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;

    focusedChild = -1;

    UINT nCount = 0;
    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

    for (UINT i = 0; i < nCount; i++)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;
        ctl::ComPtr<xaml::IFrameworkElement> spFE;

        IFC(spRealizedChildren->GetAt(i, &spChild));
        spFE = spChild.AsOrNull<xaml::IFrameworkElement>();

        if (spFE)
        {
            BOOLEAN hasFocus = FALSE;
            IFC(spFE.Cast<FrameworkElement>()->HasFocus(&hasFocus));

            if (hasFocus)
            {
                focusedChild = i;
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::get_IsScrolling(_Out_ BOOLEAN* pbIsScrolling)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOwner;

    IFCPTR(pbIsScrolling);
    IFC(get_ScrollOwner(&spOwner));

    *pbIsScrolling = (!!spOwner);

Cleanup:
    RRETURN(hr);
}

// Returns the list of childen that have been realized by the Generator.
// We must use this method whenever we interact with the Generator's index.
// In recycling mode the Children collection also contains recycled containers and thus does
// not map to the Generator's list.
_Check_return_
HRESULT
OrientedVirtualizingPanel::get_RealizedChildren(
    _Outptr_ wfc::IVector<xaml::UIElement*>** ppRealizedChildren)
{
    HRESULT hr = S_OK;
    IFCPTR(ppRealizedChildren);

    if (m_bIsVirtualizing && InRecyclingMode())
    {
        IFC(EnsureRealizedChildren());
        *ppRealizedChildren = m_tpRealizedChildren.Get();
        AddRefInterface(*ppRealizedChildren);
    }
    else
    {
        IFC(get_Children(ppRealizedChildren));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::TranslateHorizontalPixelDeltaToOffset(
    _Inout_ DOUBLE& delta,
    _Out_ DOUBLE& value)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml::IUIElement> spChild;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    BOOLEAN bVertical = FALSE;
    FLOAT zoomFactor = 1.0;
    DOUBLE currentOffset = m_ScrollData.get_OffsetX();
    DOUBLE offset = 0.0;
    UINT nRealizedChildrenCount = 0;
    UINT nVisualChildrenCount = 0;

    OffsetMemento* pCurrentOffsetState = NULL;

    value = 0.0;
    IFC(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nRealizedChildrenCount));

    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&nVisualChildrenCount));

    pCurrentOffsetState = new OffsetMemento(
        xaml_controls::Orientation_Horizontal,
        nRealizedChildrenCount,
        nVisualChildrenCount,
        m_ScrollData);

    if (pCurrentOffsetState->Equals(m_pTranslatedOffsetState) &&
        m_pTranslatedOffsetState->get_CurrentOffset() == currentOffset &&
        m_pTranslatedOffsetState->get_Delta() == delta)
    {
        value = m_pTranslatedOffsetState->get_RequestedOffset();
        delta = m_pTranslatedOffsetState->get_UnusedDelta();
        goto Cleanup;
    }

    IFC(pCurrentOffsetState->put_Delta(delta));
    IFC(pCurrentOffsetState->put_CurrentOffset(currentOffset));

    delete m_pTranslatedOffsetState;
    m_pTranslatedOffsetState = NULL;

    IFC(get_PhysicalOrientation(&orientation));
    bVertical = (orientation == xaml_controls::Orientation_Vertical);

    if (!m_bIsVirtualizing || bVertical || delta == 0.0)
    {
        offset = currentOffset + delta;
    }
    else
    {
        DOUBLE logicalOffset = DoubleUtil::Floor(currentOffset);
        INT currentChildIndex = m_iFirstVisibleChildIndex + (int)logicalOffset - m_iVisibleStart;
        DOUBLE itemLogicalOffset = DoubleUtil::Fractional(currentOffset);

        if (delta < 0)
        {
            while (delta < 0 && logicalOffset >= 0)
            {
                wf::Size desiredSize = {};
                DOUBLE itemWidth = 0.0;
                DOUBLE itemOffsetLeft = 0.0;

                if (currentChildIndex >= 0 && currentChildIndex < static_cast<INT>(nRealizedChildrenCount))
                {
                    IFC(spRealizedChildren->GetAt(currentChildIndex, &spChild));
                    IFC(GetDesiredSize(spChild.Get(), &desiredSize));
                    itemWidth = desiredSize.Width;
                }
                else
                {
                    FLOAT totalRealizedDimension = 0;
                    UINT nCount = 0;
                    IFC(ComputeTotalRealizedChildrenDimension(totalRealizedDimension, nCount));
                    itemWidth = totalRealizedDimension/nCount;
                }

                itemWidth *= zoomFactor;
                itemOffsetLeft = itemLogicalOffset * itemWidth;

                itemOffsetLeft += delta;
                delta = MIN(0, itemOffsetLeft);

                if (itemOffsetLeft >= 0)
                {
                    logicalOffset += itemOffsetLeft / itemWidth;
                    m_pTranslatedOffsetState = pCurrentOffsetState;
                    IFC(m_pTranslatedOffsetState->put_RequestedOffset(logicalOffset));
                    IFC(m_pTranslatedOffsetState->put_UnusedDelta(delta));
                    pCurrentOffsetState = NULL;
                }
                else
                {
                    itemLogicalOffset = 1;
                    logicalOffset--;
                    currentChildIndex--;
                }
            }
        }
        else
        {
            while (delta > 0 && logicalOffset < m_nItemsCount)
            {
                wf::Size desiredSize = {};
                DOUBLE itemWidth = 0.0;
                DOUBLE itemOffsetRight = 0.0;

                if (currentChildIndex >= 0 && currentChildIndex < static_cast<INT>(nRealizedChildrenCount))
                {
                    IFC(spRealizedChildren->GetAt(currentChildIndex, &spChild));
                    IFC(GetDesiredSize(spChild.Get(), &desiredSize));
                    itemWidth = desiredSize.Width;
                }
                else
                {
                    FLOAT totalRealizedDimension = 0;
                    UINT nCount = 0;
                    IFC(ComputeTotalRealizedChildrenDimension(totalRealizedDimension, nCount));
                    itemWidth = totalRealizedDimension/nCount;
                }

                itemWidth *= zoomFactor;
                itemOffsetRight = (1 - itemLogicalOffset) * itemWidth;

                itemOffsetRight -= delta;
                delta = MAX(0, -itemOffsetRight);

                if (itemOffsetRight >= 0)
                {
                    logicalOffset += (itemWidth - itemOffsetRight) / itemWidth;
                    m_pTranslatedOffsetState = pCurrentOffsetState;
                    IFC(m_pTranslatedOffsetState->put_RequestedOffset(logicalOffset));
                    IFC(m_pTranslatedOffsetState->put_UnusedDelta(delta));
                    pCurrentOffsetState = NULL;
                }
                else
                {
                    itemLogicalOffset = 0;
                    logicalOffset++;
                    currentChildIndex++;
                }
            }
        }

        offset = logicalOffset;
    }

    value = DoubleUtil::Max(offset, 0.0);

Cleanup:
    delete pCurrentOffsetState;
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::TranslateVerticalPixelDeltaToOffset(
    _Inout_ DOUBLE& delta,
    _Out_ DOUBLE& value)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml::IUIElement> spChild;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    BOOLEAN bHorizontal = FALSE;
    FLOAT zoomFactor = 1.0;
    DOUBLE currentOffset = m_ScrollData.get_OffsetY();
    DOUBLE offset = 0.0;
    UINT nRealizedChildrenCount = 0;
    UINT nVisualChildrenCount = 0;

    OffsetMemento* pCurrentOffsetState = NULL;

    value = 0.0;
    IFC(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

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
        delta = m_pTranslatedOffsetState->get_UnusedDelta();
        goto Cleanup;
    }

    IFC(pCurrentOffsetState->put_Delta(delta));
    IFC(pCurrentOffsetState->put_CurrentOffset(currentOffset));

    delete m_pTranslatedOffsetState;
    m_pTranslatedOffsetState = NULL;

    IFC(get_PhysicalOrientation(&orientation));
    bHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    if (!m_bIsVirtualizing || bHorizontal || delta == 0.0)
    {
        offset = currentOffset + delta;
    }
    else
    {
        DOUBLE logicalOffset = DoubleUtil::Floor(currentOffset);
        INT currentChildIndex = m_iFirstVisibleChildIndex + (int)logicalOffset - m_iVisibleStart;
        DOUBLE itemLogicalOffset = DoubleUtil::Fractional(currentOffset);

        if (delta < 0 && logicalOffset >= 0)
        {
            while (delta < 0)
            {
                wf::Size desiredSize = {};
                DOUBLE itemHeight = 0.0;
                DOUBLE itemOffsetTop = 0.0;

                if (currentChildIndex >= 0 && currentChildIndex < static_cast<INT>(nRealizedChildrenCount))
                {
                    IFC(spRealizedChildren->GetAt(currentChildIndex, &spChild));
                    IFC(GetDesiredSize(spChild.Get(), &desiredSize));
                    itemHeight = desiredSize.Height;
                }
                else
                {
                    FLOAT totalRealizedDimension = 0;
                    UINT nCount = 0;
                    IFC(ComputeTotalRealizedChildrenDimension(totalRealizedDimension, nCount));
                    itemHeight = totalRealizedDimension/nCount;
                }

                itemHeight *= zoomFactor;
                itemOffsetTop = itemLogicalOffset * itemHeight;

                itemOffsetTop += delta;
                delta = MIN(0, itemOffsetTop);

                if (itemOffsetTop >= 0)
                {
                    logicalOffset += itemOffsetTop / itemHeight;
                    m_pTranslatedOffsetState = pCurrentOffsetState;
                    IFC(m_pTranslatedOffsetState->put_RequestedOffset(logicalOffset));
                    IFC(m_pTranslatedOffsetState->put_UnusedDelta(delta));
                    pCurrentOffsetState = NULL;
                }
                else
                {
                    itemLogicalOffset = 1;
                    logicalOffset--;
                    currentChildIndex--;
                }
            }
        }
        else
        {
            while (delta > 0 && logicalOffset < m_nItemsCount)
            {
                wf::Size desiredSize = {};
                DOUBLE itemHeight = 0.0;
                DOUBLE itemOffsetBottom = 0.0;

                if (currentChildIndex >= 0 && currentChildIndex < static_cast<INT>(nRealizedChildrenCount))
                {
                    IFC(spRealizedChildren->GetAt(currentChildIndex, &spChild));
                    IFC(GetDesiredSize(spChild.Get(), &desiredSize));
                    itemHeight = desiredSize.Height;
                }
                else
                {
                    FLOAT totalRealizedDimension = 0;
                    UINT nCount = 0;
                    IFC(ComputeTotalRealizedChildrenDimension(totalRealizedDimension, nCount));
                    itemHeight = totalRealizedDimension/nCount;
                }

                itemHeight *= zoomFactor;
                itemOffsetBottom = (1 - itemLogicalOffset) * itemHeight;

                itemOffsetBottom -= delta;
                delta = MAX(0, -itemOffsetBottom);

                if (itemOffsetBottom >= 0)
                {
                    logicalOffset += (itemHeight - itemOffsetBottom) / itemHeight;
                    m_pTranslatedOffsetState = pCurrentOffsetState;
                    IFC(m_pTranslatedOffsetState->put_RequestedOffset(logicalOffset));
                    IFC(m_pTranslatedOffsetState->put_UnusedDelta(delta));
                    pCurrentOffsetState = NULL;
                }
                else
                {
                    itemLogicalOffset = 0;
                    logicalOffset++;
                    currentChildIndex++;
                }
            }
        }

        offset = logicalOffset;
    }

    value = DoubleUtil::Max(offset, 0.0);

Cleanup:
    delete pCurrentOffsetState;
    RRETURN(hr);
}

// Returns the Visible start Index
INT OrientedVirtualizingPanel::VisibleStartItemIndex()
{
    return MAX(-1, m_iVisibleStart * static_cast<INT>(m_itemsPerLine));
}

// Helper method checks whether the given item is first Item in Line or not
bool OrientedVirtualizingPanel::IsFirstItemInLine(_In_ INT itemIndex)
{
    return itemIndex % m_itemsPerLine == 0;
}

// Returns the Desired size of container
// In case of FixedSizeItems, returns the calculated Fixed size
_Check_return_
HRESULT
OrientedVirtualizingPanel::GetDesiredSize(_In_ IUIElement* pChild, _Out_ wf::Size* pDesiredSize)
{
    HRESULT hr = S_OK;

    IFC(pChild->get_DesiredSize(pDesiredSize));

Cleanup:
    RRETURN(hr);
}

//  IScrollSnapPointsInfo implementation

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::get_AreHorizontalSnapPointsRegular
//
//  Synopsis:
//    Returns True when the horizontal snap points are equidistant
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT OrientedVirtualizingPanel::get_AreHorizontalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation;

    *pValue = FALSE;

    IFC(get_PhysicalOrientation(&orientation));

    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        IFC(AreScrollSnapPointsRegular(pValue));
    }

    // When the orientation is vertical, there are no horizontal snap points.
    // We simply return FALSE then.

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::get_AreVerticalSnapPointsRegular
//
//  Synopsis:
//    Returns True when the vertical snap points are equidistant
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT OrientedVirtualizingPanel::get_AreVerticalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation;

    *pValue = FALSE;

    IFC(get_PhysicalOrientation(&orientation));

    if (orientation == xaml_controls::Orientation_Vertical)
    {
        IFC(AreScrollSnapPointsRegular(pValue));
    }

    // When the orientation is horizontal, there are no vertical snap points.
    // We simply return FALSE then.

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::GetIrregularSnapPoints
//
//  Synopsis:
//    Returns a read-only collection of numbers representing the snap points for
//    the provided orientation. Returns an empty collection when no snap points are present.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT OrientedVirtualizingPanel::GetIrregularSnapPointsImpl(
    // The direction of the requested snap points.
    _In_ xaml_controls::Orientation orientation,
    // The alignment used by the caller when applying the requested snap points.
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    // The read-only collection of snap points.
    _Outptr_ wfc::IVectorView<FLOAT>** pValue)
{
    HRESULT hr = S_OK;
    IFC(GetIrregularSnapPointsInternal(orientation, alignment, pValue));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::GetRegularSnapPoints
//
//  Synopsis:
//    Returns an original offset and interval for equidistant snap points for
//    the provided orientation. Returns 0 when no snap points are present.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT OrientedVirtualizingPanel::GetRegularSnapPointsImpl(
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
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    UINT32 nCount = 0;
    FLOAT childDim = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    FLOAT upperMarginSnapPointKey = 0.0;
    wf::Size childDesiredSize = {};
    xaml_controls::Orientation physicalOrientation;

    IFCPTR(pOffset);
    *pOffset = 0.0;
    IFCPTR(pInterval);
    *pInterval = 0.0;

    IFC(get_PhysicalOrientation(&physicalOrientation));

    if (orientation == physicalOrientation)
    {
        IFC(AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));

        if (!areScrollSnapPointsRegular)
        {
            // Accessing the regular snap points while AreScrollSnapPointsRegular is False is not supported.
            IFC(E_FAIL); // TODO: Replace with custom error code. Something similar to InvalidOperationException.
        }

        IFC(ResetSnapPointKeys());

        IFC(GetCommonSnapPointKeys(&lowerMarginSnapPointKey, &upperMarginSnapPointKey));

        IFC(get_RealizedChildren(&spRealizedChildren));
        IFC(spRealizedChildren->get_Size(&nCount));

#if DBG
        IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

        for (INT childIndex = m_iFirstVisibleChildIndex - m_iBeforeTrail;
             childIndex < m_iFirstVisibleChildIndex + m_iVisibleCount;
             childIndex++)
        {
            ctl::ComPtr<xaml::IUIElement> spChild;

            IFC(spRealizedChildren->GetAt(childIndex, &spChild));
            if (spChild)
            {
                IFC(GetDesiredSize(spChild.Get(), &childDesiredSize));

                if (orientation == xaml_controls::Orientation_Vertical)
                {
                    childDim = childDesiredSize.Height;
                }
                else
                {
                    childDim = childDesiredSize.Width;
                }

                switch (alignment)
                {
                case xaml_primitives::SnapPointsAlignment_Near:
                    *pOffset = lowerMarginSnapPointKey;
                    break;
                case xaml_primitives::SnapPointsAlignment_Center:
                    // Snap points are centered on the children
                    *pOffset = childDim / 2 + lowerMarginSnapPointKey;
                    break;
                case xaml_primitives::SnapPointsAlignment_Far:
                    *pOffset = upperMarginSnapPointKey;
                    break;
                }

                *pInterval = childDim;
                break;
            }
        }

        m_regularSnapPointKey = childDim;
        m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
        m_upperMarginSnapPointKey = upperMarginSnapPointKey;
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


//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::add_HorizontalSnapPointsChanged
//
//  Synopsis:
//    Adds an event handler for the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
OrientedVirtualizingPanel::add_HorizontalSnapPointsChanged(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (!pEventSource->HasHandlers())
    {
        IFC(SetSnapPointsChangeNotificationsRequirement(
            true /*isForHorizontalSnapPoints*/,
            true /*notifyChanges*/));
    }

    IFC(OrientedVirtualizingPanelGenerated::add_HorizontalSnapPointsChanged(pValue, ptToken));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::remove_HorizontalSnapPointsChanged
//
//  Synopsis:
//    Removes an event handler for the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
OrientedVirtualizingPanel::remove_HorizontalSnapPointsChanged(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(OrientedVirtualizingPanelGenerated::remove_HorizontalSnapPointsChanged(tToken));

    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));

    if (!pEventSource->HasHandlers())
    {
        IFC(SetSnapPointsChangeNotificationsRequirement(
            true /*isForHorizontalSnapPoints*/,
            false /*notifyChanges*/));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::add_VerticalSnapPointsChanged
//
//  Synopsis:
//    Adds an event handler for the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
OrientedVirtualizingPanel::add_VerticalSnapPointsChanged(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (!pEventSource->HasHandlers())
    {
        IFC(SetSnapPointsChangeNotificationsRequirement(
            false /*isForHorizontalSnapPoints*/,
            true /*notifyChanges*/));
    }

    IFC(OrientedVirtualizingPanelGenerated::add_VerticalSnapPointsChanged(pValue, ptToken));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::remove_VerticalSnapPointsChanged
//
//  Synopsis:
//    Removes an event handler for the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
OrientedVirtualizingPanel::remove_VerticalSnapPointsChanged(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(OrientedVirtualizingPanelGenerated::remove_VerticalSnapPointsChanged(tToken));

    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (!pEventSource->HasHandlers())
    {
        IFC(SetSnapPointsChangeNotificationsRequirement(
            false /*isForHorizontalSnapPoints*/,
            false /*notifyChanges*/));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::AreScrollSnapPointsRegular
//
//  Synopsis:
//    Returns True because the base OrientedVirtualizingPanel's snap points are always equidistant.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::AreScrollSnapPointsRegular(_Out_ BOOLEAN* pAreScrollSnapPointsRegular)
{
    HRESULT hr = S_OK;

    IFCPTR(pAreScrollSnapPointsRegular);
    *pAreScrollSnapPointsRegular = TRUE;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::OnAreScrollSnapPointsRegularChanged
//
//  Synopsis:
//    Called when the AreSnapPointsChanged property changed.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::OnAreScrollSnapPointsRegularChanged()
{
    HRESULT hr = S_OK;
    UINT32 nCount = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

    IFC(NotifySnapPointsChanges(spRealizedChildren.Get(), nCount));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::OnHorizontalSnapPointsChanged
//
//  Synopsis:
//    Raises the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::OnHorizontalSnapPointsChanged()
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<EventArgs> spArgs;

    // Create the args
    IFC(ctl::make<EventArgs>(&spArgs));

    // Raise the event
    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spArgs.Get())));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::OnVerticalSnapPointsChanged
//
//  Synopsis:
//    Raises the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::OnVerticalSnapPointsChanged()
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<EventArgs> spArgs;

    // Create the args
    IFC(ctl::make<EventArgs>(&spArgs));

    // Raise the event
    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spArgs.Get())));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::GetRegularSnapPointKeys
//
//  Synopsis:
//    Determines the keys for regular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::GetRegularSnapPointKeys(
    _In_ xaml_controls::Orientation orientation,
    _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
    _In_ UINT32 nCount,
    _Out_ FLOAT* pSnapPointKey,
    _Out_ FLOAT* pLowerMarginSnapPointKey,
    _Out_ FLOAT* pUpperMarginSnapPointKey)
{
    HRESULT hr = S_OK;
    wf::Size childDesiredSize = {};

    IFCEXPECT(pSnapPointKey);
    *pSnapPointKey = 0.0;
    IFCEXPECT(pLowerMarginSnapPointKey);
    *pLowerMarginSnapPointKey = 0.0;
    IFCEXPECT(pUpperMarginSnapPointKey);
    *pUpperMarginSnapPointKey = 0.0;

    for (INT childIndex = m_iFirstVisibleChildIndex - m_iBeforeTrail;
         childIndex < m_iFirstVisibleChildIndex + m_iVisibleCount;
         childIndex++)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;

        IFCEXPECT(pRealizedChildren);
        IFC(pRealizedChildren->GetAt(childIndex, &spChild));
        if (spChild)
        {
            IFC(GetDesiredSize(spChild.Get(), &childDesiredSize));

            if (orientation == xaml_controls::Orientation_Vertical)
            {
                *pSnapPointKey = childDesiredSize.Height;
            }
            else
            {
                *pSnapPointKey = childDesiredSize.Width;
            }
            break;
        }
    }

    IFC(GetCommonSnapPointKeys(pLowerMarginSnapPointKey, pUpperMarginSnapPointKey));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::GetCommonSnapPointKeys
//
//  Synopsis:
//    Determines the common keys for regular and irregular snap points.
//    Those keys are the left/right margins for a horizontal panel,
//    or the top/bottom margins for a vertical panel.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::GetCommonSnapPointKeys(
    _Out_ FLOAT* pLowerMarginSnapPointKey,
    _Out_ FLOAT* pUpperMarginSnapPointKey)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IFrameworkElement> spFrameworkElement;
    xaml_controls::Orientation orientation;
    xaml::Thickness margins;

    IFCEXPECT(pLowerMarginSnapPointKey);
    *pLowerMarginSnapPointKey = 0.0;
    IFCEXPECT(pUpperMarginSnapPointKey);
    *pUpperMarginSnapPointKey = 0.0;

    IFC(get_PhysicalOrientation(&orientation));

    spFrameworkElement = ctl::query_interface_cast<xaml::IFrameworkElement>(this);
    if (spFrameworkElement)
    {
        IFC(spFrameworkElement->get_Margin(&margins));
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            *pLowerMarginSnapPointKey = static_cast<FLOAT>(margins.Left);
            *pUpperMarginSnapPointKey = static_cast<FLOAT>(margins.Right);
        }
        else
        {
            *pLowerMarginSnapPointKey = static_cast<FLOAT>(margins.Top);
            *pUpperMarginSnapPointKey = static_cast<FLOAT>(margins.Bottom);
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::NotifySnapPointsChanges
//
//  Synopsis:
//    Checks if the snap point keys have changed and a notification needs
//    to be raised.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::NotifySnapPointsChanges(
    _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
    _In_ UINT32 nCount)
{
    HRESULT hr = S_OK;
    FLOAT snapPointKey = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    FLOAT upperMarginSnapPointKey = 0.0;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    BOOLEAN notifyForHorizontalSnapPoints = FALSE;
    BOOLEAN notifyForVerticalSnapPoints = FALSE;
    xaml_controls::Orientation orientation;

#if DBG
    IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    IFC(AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
    IFC(get_PhysicalOrientation(&orientation));

    if (orientation == xaml_controls::Orientation_Vertical)
    {
        if ((m_regularSnapPointKey != -1.0) &&
            m_bAreSnapPointsKeysHorizontal && m_bNotifyHorizontalSnapPointsChanges)
        {
            // Last computed snap point keys were for horizontal orientation.
            // New orientation is vertical.
            // Consumer wants notifications for horizontal snap points.
            notifyForHorizontalSnapPoints = TRUE;
        }
    }
    else
    {
        if ((m_regularSnapPointKey != -1.0) &&
            !m_bAreSnapPointsKeysHorizontal && m_bNotifyVerticalSnapPointsChanges)
        {
            // Last computed snap point keys were for vertical orientation.
            // New orientation is horizontal.
            // Consumer wants notifications for vertical snap points.
            notifyForVerticalSnapPoints = TRUE;
        }
    }

    if ((m_bNotifyHorizontalSnapPointsChanges && orientation == xaml_controls::Orientation_Horizontal &&
         m_bAreSnapPointsKeysHorizontal && !m_bNotifiedHorizontalSnapPointsChanges) ||
        (m_bNotifyVerticalSnapPointsChanges && orientation == xaml_controls::Orientation_Vertical &&
         !m_bAreSnapPointsKeysHorizontal && !m_bNotifiedVerticalSnapPointsChanges))
    {
        if (m_regularSnapPointKey != -1.0)
        {
            if (areScrollSnapPointsRegular)
            {
                IFC(GetRegularSnapPointKeys(orientation, pRealizedChildren, nCount, &snapPointKey, &lowerMarginSnapPointKey, &upperMarginSnapPointKey));
                if (m_regularSnapPointKey != snapPointKey ||
                    m_lowerMarginSnapPointKey != lowerMarginSnapPointKey ||
                    m_upperMarginSnapPointKey != upperMarginSnapPointKey)
                {
                    if (m_bAreSnapPointsKeysHorizontal)
                    {
                        notifyForHorizontalSnapPoints = TRUE;
                    }
                    else
                    {
                        notifyForVerticalSnapPoints = TRUE;
                    }
                }
            }
            else
            {
                if (m_bAreSnapPointsKeysHorizontal)
                {
                    notifyForHorizontalSnapPoints = TRUE;
                }
                else
                {
                    notifyForVerticalSnapPoints = TRUE;
                }
            }
        }
    }

    if (notifyForHorizontalSnapPoints)
    {
        IFC(NotifySnapPointChanges(TRUE /*isForHorizontalSnapPoints*/));
    }

    if (notifyForVerticalSnapPoints)
    {
        IFC(NotifySnapPointChanges(FALSE /*isForHorizontalSnapPoints*/));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::NotifySnapPointChanges
//
//  Synopsis:
//
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::NotifySnapPointChanges(_In_ BOOLEAN isForHorizontalSnapPoints)
{
    HRESULT hr = S_OK;

    if ((isForHorizontalSnapPoints && m_bNotifyHorizontalSnapPointsChanges && !m_bNotifiedHorizontalSnapPointsChanges) ||
        (!isForHorizontalSnapPoints && m_bNotifyVerticalSnapPointsChanges && !m_bNotifiedVerticalSnapPointsChanges))
    {
        if (isForHorizontalSnapPoints)
        {
            // Raise HorizontalSnapPointsChanged event.
            m_bNotifiedHorizontalSnapPointsChanges = TRUE;
            IFC(OnHorizontalSnapPointsChanged());
        }
        else
        {
            // Raise VerticalSnapPointsChanged event.
            m_bNotifiedVerticalSnapPointsChanges = TRUE;
            IFC(OnVerticalSnapPointsChanged());
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::RefreshRegularSnapPointKeys
//
//  Synopsis:
//    Refreshes the m_regularSnapPointKey field based on a single child.
//    Refreshes also the m_lowerMarginSnapPointKey/m_upperMarginSnapPointKey fields based
//    on the current margins.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::RefreshRegularSnapPointKeys()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    FLOAT snapPointKey = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    FLOAT upperMarginSnapPointKey = 0.0;
    UINT32 nCount = 0;
    xaml_controls::Orientation orientation;

#ifdef DBG
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    IFC(AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
    ASSERT(areScrollSnapPointsRegular);
#endif

    IFC(get_PhysicalOrientation(&orientation));

    IFC(ResetSnapPointKeys());

    m_regularSnapPointKey = 0.0;

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

#if DBG
    IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    IFC(GetRegularSnapPointKeys(orientation, spRealizedChildren.Get(), nCount, &snapPointKey, &lowerMarginSnapPointKey, &upperMarginSnapPointKey));

    m_bAreSnapPointsKeysHorizontal = (orientation == xaml_controls::Orientation_Horizontal);
    m_regularSnapPointKey = snapPointKey;
    m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
    m_upperMarginSnapPointKey = upperMarginSnapPointKey;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::ResetSnapPointKeys
//
//  Synopsis:
//    Resets regular snap point keys.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::ResetSnapPointKeys()
{
    m_regularSnapPointKey = -1.0;
    m_lowerMarginSnapPointKey = 0;
    m_upperMarginSnapPointKey = 0;
    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::SetSnapPointsChangeNotificationsRequirement
//
//  Synopsis:
//    Determines whether the OrientedVirtualizingPanel must call NotifySnapPointsChanged
//    when snap points change or not.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT OrientedVirtualizingPanel::SetSnapPointsChangeNotificationsRequirement(
    _In_ BOOLEAN isForHorizontalSnapPoints,
    _In_ BOOLEAN notifyChanges)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation;

    IFC(get_PhysicalOrientation(&orientation));
    if (isForHorizontalSnapPoints)
    {
        m_bNotifyHorizontalSnapPointsChanges = notifyChanges;
        if (orientation == xaml_controls::Orientation_Horizontal && notifyChanges)
        {
            IFC(RefreshRegularSnapPointKeys());
            m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        }
    }
    else
    {
        m_bNotifyVerticalSnapPointsChanges = notifyChanges;
        if (orientation == xaml_controls::Orientation_Vertical && notifyChanges)
        {
            IFC(RefreshRegularSnapPointKeys());
            m_bNotifiedVerticalSnapPointsChanges = FALSE;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Methods accessed by scrolling owner to support DirectManipulation and edge scrolling.

// Sets the m_bInManipulation flag to tell whether a manipulation is in progress or not.
_Check_return_
HRESULT
OrientedVirtualizingPanel::UpdateInManipulation(
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
_Check_return_
HRESULT
OrientedVirtualizingPanel::SetZoomFactor(
    _In_ FLOAT newZoomFactor)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsInDMZoom = FALSE;

    if (m_fZoomFactor != newZoomFactor)
    {
        m_fZoomFactor = newZoomFactor;

        IFC(IsInDirectManipulationZoom(bIsInDMZoom));
        if (!bIsInDMZoom)
        {
            IFC(InvalidateMeasure());

            // In vsp, a zoom will trigger layout which will trigger unwanted transitions
            IFC(put_IsIgnoringTransitions(TRUE));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Gets the scrolling extent in pixels even for logical scrolling scenarios.
_Check_return_
HRESULT
OrientedVirtualizingPanel::ComputePixelExtent(
    _In_ bool ignoreZoomFactor,
    _Out_ DOUBLE& extent)
{
    HRESULT hr = S_OK;
    UINT nLineCount = 0;
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
    IFC(ComputeTotalRealizedChildrenDimension(cumulatedChildDim, nLineCount));

    if (nLineCount > 0)
    {
        extent = (DOUBLE) (cumulatedChildDim + cumulatedChildDim / nLineCount * (m_lineCount - nLineCount));
        extent = extent * zoomFactor;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the offset in pixels even for logical scrolling scenarios.
_Check_return_
HRESULT
OrientedVirtualizingPanel::ComputePixelOffset(
    _In_ BOOLEAN isForHorizontalOrientation,
    _Out_ DOUBLE& offset)
{
    RRETURN(ComputePixelOffset(isForHorizontalOrientation, FALSE /*bUseInputLogicalOffset*/, 0, offset));
}

// Gets the logical offset given a pixel delta.
_Check_return_
HRESULT
OrientedVirtualizingPanel::ComputeLogicalOffset(
    _In_ BOOLEAN isForHorizontalOrientation,
    _Inout_ DOUBLE& pixelDelta,
    _Out_ DOUBLE& logicalOffset)
{
    HRESULT hr = S_OK;

    if (isForHorizontalOrientation)
    {
        IFC(TranslateHorizontalPixelDeltaToOffset(pixelDelta, logicalOffset));
    }
    else
    {
        IFC(TranslateVerticalPixelDeltaToOffset(pixelDelta, logicalOffset));
    }

Cleanup:
    RRETURN(hr);
}

// Computes the total dimension of all realized children
_Check_return_
HRESULT
OrientedVirtualizingPanel::ComputeTotalRealizedChildrenDimension(
    _Out_ FLOAT& cumulatedChildDim, _Out_ UINT& nlineCount)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    xaml_controls::Orientation orientation;
    wf::Size childDesiredSize = {};
    UINT offsetItemCount = 0;

    nlineCount = 0;
    cumulatedChildDim = 0.0;

    IFC(get_PhysicalOrientation(&orientation));

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nlineCount));

   // when the last line has less items than m_itemsPerLine, cumulatedChildDim needs to be adjusted.
    offsetItemCount = nlineCount % m_itemsPerLine;

    if (nlineCount > 0)
    {
        for (UINT32 childIndex = 0; childIndex < nlineCount; childIndex++)
        {
            ctl::ComPtr<xaml::IUIElement> spChild;

            IFC(spRealizedChildren->GetAt(childIndex, &spChild));
            if (spChild)
            {
                IFC(GetDesiredSize(spChild.Get(), &childDesiredSize));

                if (orientation == xaml_controls::Orientation_Horizontal)
                {
                    cumulatedChildDim += childDesiredSize.Width;
                }
                else
                {
                    cumulatedChildDim += childDesiredSize.Height;
                }
            }
        }
    }

    // if the last line has less items than m_itemsPerCount, add the dimension for those items in cumulatedChildDim
    if(offsetItemCount > 0)
    {
        cumulatedChildDim += (m_itemsPerLine - offsetItemCount) * cumulatedChildDim/nlineCount;
    }

    // use ceil to get line count, when last line has less items, it should return correct nLineCount
    nlineCount = static_cast<XUINT32>(ceil(static_cast<XFLOAT>(nlineCount) / static_cast<XFLOAT>(m_itemsPerLine)));
    cumulatedChildDim = cumulatedChildDim / m_itemsPerLine;

Cleanup:
    RRETURN(hr);
}

// Computes the estimated dimension of the unrealized children ahead of the realized ones.
_Check_return_
HRESULT
OrientedVirtualizingPanel::ComputeUnrealizedChildrenEstimatedDimension(
    _Out_ FLOAT& dimension)
{
    HRESULT hr = S_OK;
    UINT nLineCount = 0;
    FLOAT cumulatedChildDim = 0.0;
    INT beforeTrailLines = 0;

    dimension = 0.0;

    // calculate before trail lines from m_iBeforeTrail
    beforeTrailLines =  m_iBeforeTrail / m_itemsPerLine;
    if (m_iVisibleStart - beforeTrailLines > 0)
    {
        IFC(ComputeTotalRealizedChildrenDimension(cumulatedChildDim, nLineCount));
        if (nLineCount > 0)
        {
            dimension = (m_iVisibleStart - beforeTrailLines) * cumulatedChildDim / nLineCount;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
OrientedVirtualizingPanel::GetIrregularSnapPointsInternal(
    // The direction of the requested snap points.
    _In_ xaml_controls::Orientation orientation,
    // The alignment used by the caller when applying the requested snap points.
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    // The read-only collection of snap points.
    _Outptr_ wfc::IVectorView<FLOAT>** pValue)
{
    RRETURN(S_OK);
}

// Logical Orientation override
_Check_return_ HRESULT OrientedVirtualizingPanel::get_LogicalOrientation(
    _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    *pValue = xaml_controls::Orientation_Vertical;

Cleanup:
    RRETURN(hr);
}

// Physical Orientation override
_Check_return_ HRESULT OrientedVirtualizingPanel::get_PhysicalOrientation(
     _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    *pValue = xaml_controls::Orientation_Vertical;

Cleanup:
    RRETURN(hr);
}

// Get the closest element information to the point.
_Check_return_ HRESULT OrientedVirtualizingPanel::GetClosestElementInfo(
    _In_ wf::Point position,
    _Out_ xaml_primitives::ElementInfo* returnValue)
{
    HRESULT hr = S_OK;

    returnValue->m_childIndex = -1;
    returnValue->m_childIsHeader = FALSE;

    RRETURN(hr);
}

// Get the index where an item should be inserted if it were dropped at
// the given position.  This will be used by live reordering.
_Check_return_ HRESULT OrientedVirtualizingPanel::GetInsertionIndex(
    _In_ wf::Point position,
    _Out_ INT* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);

    *returnValue = -1;

Cleanup:
    RRETURN(hr);
}

 // Gets a series of BOOLEAN values indicating whether a given index is
// positioned on the leftmost, topmost, rightmost, or bottommost
// edges of the layout.  This can be useful for both determining whether
// to tilt items at the edges of rows or columns as well as providing
// data for portal animations.
_Check_return_ HRESULT OrientedVirtualizingPanel::IsLayoutBoundary(
    _In_ INT index,
    _Out_ BOOLEAN* pIsLeftBoundary,
    _Out_ BOOLEAN* pIsTopBoundary,
    _Out_ BOOLEAN* pIsRightBoundary,
    _Out_ BOOLEAN* pIsBottomBoundary)
{
    HRESULT hr = S_OK;

    IFCPTR(pIsLeftBoundary);
    IFCPTR(pIsTopBoundary);
    IFCPTR(pIsRightBoundary);
    IFCPTR(pIsBottomBoundary);

    *pIsLeftBoundary = FALSE;
    *pIsTopBoundary = FALSE;
    *pIsRightBoundary = FALSE;
    *pIsBottomBoundary = FALSE;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT OrientedVirtualizingPanel::GetItemsBounds(
                _Out_ wf::Rect* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);

    *returnValue = m_arrangedItemsRect;

Cleanup:
    RRETURN(hr);
}

// Get the indexes where an item should be inserted if it were dropped at
// the given position
_Check_return_ HRESULT OrientedVirtualizingPanel::GetInsertionIndexesImpl(
    _In_ wf::Point position,
    _Out_ INT* pFirst,
    _Out_ INT* pSecond)
{
    int insertionIndex = -1;
    BOOLEAN isLeftBoundary = FALSE;
    BOOLEAN isRightBoundary = FALSE;
    BOOLEAN isTopBoundary = FALSE;
    BOOLEAN isBottomBoundary = FALSE;
    BOOLEAN firstCheck = FALSE;
    BOOLEAN secondCheck = FALSE;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFCPTR_RETURN(pFirst);
    IFCPTR_RETURN(pSecond);

    *pFirst = -1;
    *pSecond = -1;

    IFC_RETURN(GetInsertionIndex(position, &insertionIndex));
    IFC_RETURN(IsLayoutBoundary(insertionIndex, &isLeftBoundary, &isTopBoundary, &isRightBoundary, &isBottomBoundary));

    *pFirst = insertionIndex - 1;
    *pSecond = insertionIndex;

    IFC_RETURN(get_PhysicalOrientation(&orientation));
    if (orientation == xaml_controls::Orientation_Vertical)
    {
        firstCheck = isTopBoundary;
        secondCheck = isBottomBoundary;
    }
    else
    {
        firstCheck = isLeftBoundary;
        secondCheck = isRightBoundary;
    }

    // make sure we're not at the edges of the panel
    if (firstCheck)
    {
        *pFirst = -1;
    }
    else if (secondCheck)
    {
        *pSecond = -1;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the index of the last element visible in the viewport
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
OrientedVirtualizingPanel::GetLastItemIndexInViewport(
    _In_ IScrollInfo* pScrollInfo,
    _Inout_ INT* pResult)
{
    // m_iVisibleCount contains items which are visible + AfterTrail items, it doesn't contain before trail items
    *pResult = m_iVisibleStart * m_itemsPerLine                  // Index of first visible item
             + (m_iVisibleCount - m_iAfterTrail)                 // Number of visible items after post cache
             - 1;                                                // First visible item gets included twice
    RRETURN(S_OK);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculates items per page based on the current viewport size.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
OrientedVirtualizingPanel::GetItemsPerPage(
    _In_ IScrollInfo* pScrollInfo,
    _Out_ DOUBLE* pItemsPerPage)
{
    // m_iVisibleCount contains items which are visible + AfterTrail items, it doesn't contain before trail items
    *pItemsPerPage = m_iVisibleCount                                   // Number of visible items including post cache
                  - m_iAfterTrail;                                     // post cache items
    RRETURN(S_OK);
}

_Check_return_ HRESULT OrientedVirtualizingPanel::IsHorizontal(
    _Out_ BOOLEAN& bIsHorizontal)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    IFC(get_PhysicalOrientation(&orientation));
    bIsHorizontal = orientation == xaml_controls::Orientation_Horizontal;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT OrientedVirtualizingPanel::GetZoomFactor(
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

_Check_return_ HRESULT OrientedVirtualizingPanel::IsInDirectManipulationZoom(
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

_Check_return_ HRESULT OrientedVirtualizingPanel::RaiseVirtualizedCollectionUpdatedEvent(
    _In_ wf::Rect contentBounds)
{
    HRESULT hr = S_OK;

    if (EventEnabledVirtualizedCollectionUpdatedInfo())
    {
        ctl::ComPtr<IScrollOwner> spOuterOwner;
        IFC(m_ScrollData.get_ScrollOwner(&spOuterOwner));
        if (spOuterOwner)
        {
            ctl::ComPtr<IScrollInfo> spScrollInfo;
            IFC(spOuterOwner.As(&spScrollInfo));
            if (spScrollInfo)
            {
                ctl::ComPtr<IScrollOwner> spInnerOwner;
                IFC(spScrollInfo->get_ScrollOwner(&spInnerOwner));
                if (spInnerOwner)
                {
                    ctl::ComPtr<IScrollViewer> spScrollViewer;
                    IFC(spInnerOwner.As(&spScrollViewer));
                    if (spScrollViewer)
                    {
                        DOUBLE viewportWidth = 0;
                        DOUBLE viewportHeight = 0;
                        DOUBLE extentWidth = 0;
                        DOUBLE extentHeight = 0;

                        IFC(spScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportWidth(NULL, FALSE, &viewportWidth));
                        IFC(spScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportHeight(NULL, FALSE, &viewportHeight));
                        IFC(spScrollViewer.Cast<ScrollViewer>()->ComputePixelExtentWidth(&extentWidth));
                        IFC(spScrollViewer.Cast<ScrollViewer>()->ComputePixelExtentHeight(&extentHeight));

                        TraceVirtualizedCollectionUpdatedInfo(
                            (UINT64)spScrollViewer.Cast<ScrollViewer>()->GetHandle(),
                            DoubleUtil::IsPositiveInfinity(contentBounds.X) ? 0 : contentBounds.X,
                            DoubleUtil::IsPositiveInfinity(contentBounds.Y) ? 0 : contentBounds.Y,
                            contentBounds.Width,
                            contentBounds.Height,
                            static_cast<FLOAT>(viewportWidth),
                            static_cast<FLOAT>(viewportHeight),
                            static_cast<FLOAT>(extentWidth),
                            static_cast<FLOAT>(extentHeight),
                            FALSE
                            );
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


// Updates the non-virtualized offset irrespective of the extent, before the coming MeasureOverride
// execution updates the extent based on the new zoom factor.
_Check_return_ HRESULT OrientedVirtualizingPanel::SetNonVirtualizingOffset(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    ASSERT(!m_bInMeasure);
    ASSERT(!m_bItemBasedScrolling);

    if (offset < 0.0f)
    {
        goto Cleanup;
    }

    IFC(get_PhysicalOrientation(&orientation));
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

// Since we don't disconnect/distroy the containers, there may be extra visual child/container
// where there is no associated items. These containers are arranged outside view
_Check_return_ HRESULT OrientedVirtualizingPanel::ArrangeExtraContainers(
    _In_ bool isHorizontal)
{
    HRESULT hr = S_OK;

    UINT realizedIndex = 0;
    UINT nVisualCount = 0;
    UINT nRealizedCount = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    wf::Size childDesiredSize = {};
    wf::Rect extraChild = {0,0,0,0};

#if DBG
    UINT extraContainerCount = 0;
#endif


    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&nVisualCount));

    if (nVisualCount > 0)
    {
        IFC(get_RealizedChildren(&spRealizedChildren));
        IFC(spRealizedChildren->get_Size(&nRealizedCount));
        ASSERT(nRealizedCount <= nVisualCount, L"Realized children are more than visual children");

        if(nRealizedCount < nVisualCount)
        {
            for (UINT i = 0; i < nVisualCount; i++)
            {
                ctl::ComPtr<xaml::IUIElement> spVisualChild;
                ctl::ComPtr<xaml::IUIElement> spRealizedChild;

                IFC(spChildren->GetAt(i, &spVisualChild));
                if (nRealizedCount > realizedIndex)
                {
                    IFC(spRealizedChildren->GetAt(realizedIndex, &spRealizedChild));
                }

                if (spVisualChild == spRealizedChild)
                {
                    realizedIndex++;
                }
                else
                {
                    CUIElement* pCUIElementNoRef = NULL;

                    IFC(spVisualChild->get_DesiredSize(&childDesiredSize));

                    // Arranging elements at the FLT_MAX could cause issues with
                    // graphics stack. For e.g. anything greater than 1<<21 may cause
                    // issues with D2D glyph computations. Hence we arrange the extra child at a
                    // large offset but not at FLT_MAX.
                    if(isHorizontal)
                    {
                        extraChild.Y = VirtualizingPanel::ExtraContainerArrangeOffset;
                    }
                    else
                    {
                        extraChild.X = VirtualizingPanel::ExtraContainerArrangeOffset;
                    }

                    extraChild.Height = childDesiredSize.Height;
                    extraChild.Width = childDesiredSize.Width;
                    // Turn off layout transitions
                    pCUIElementNoRef = static_cast<CUIElement*>(spVisualChild.Cast<UIElement>()->GetHandle());
                    IFC(CoreImports::UIElement_SetIsEntering(pCUIElementNoRef, FALSE));
                    IFC(CoreImports::UIElement_SetCurrentTransitionLocation(pCUIElementNoRef, extraChild.X, extraChild.Y, extraChild.Width, extraChild.Height));
                    IFC(spVisualChild->Arrange(extraChild));
    #if DBG
                    extraContainerCount++;
    #endif
                }
            }
        }
    }

#if OVP_DEBUG
    if(extraContainerCount > 0)
    {
       WCHAR szValue[250];
       IFCEXPECT(swprintf_s(szValue, 250, L"extraContainerCount: %d\n", extraContainerCount) >= 0);
       Trace(szValue);
    }
#endif

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT OrientedVirtualizingPanel::GetSizeOfFirstVisibleChild(
    _Out_ wf::Size& size)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml::IUIElement> spRealizedChild;
    UINT nRealizedCount = 0;

    IFC(get_RealizedChildren(&spRealizedChildren));
    if (spRealizedChildren)
    {
        IFC(spRealizedChildren->get_Size(&nRealizedCount));
        if (nRealizedCount > 0 && m_iFirstVisibleChildIndex >= 0 && m_iFirstVisibleChildIndex < (INT)nRealizedCount)
        {
            IFC(spRealizedChildren->GetAt(m_iFirstVisibleChildIndex, &spRealizedChild));
            IFC(spRealizedChild->get_DesiredSize(&size));
        }
    }

Cleanup:
    RRETURN(hr);
}


