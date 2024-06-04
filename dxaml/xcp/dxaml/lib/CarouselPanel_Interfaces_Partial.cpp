// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      CarouselPanel partial definition for interface methods and methods same as VitualizingStackPanel.

#include "precomp.h"
#include "CarouselPanel.g.h"
#include "ScrollViewer.g.h"
#include "ScrollContentPresenter.g.h"
#include "UIElementCollection.g.h"
#include "ItemContainerGenerator.g.h"
#include "CleanUpVirtualizedItemEventArgs.g.h"
#include "ItemsControl.g.h"
#include "VirtualizingStackPanel.g.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;

// Scroll content by one line to the top.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of what "line" means.
_Check_return_ HRESULT CarouselPanel::LineUpImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    IFC(TranslateVerticalPixelDeltaToOffset(-ScrollViewerLineDelta, offset));
    IFC(SetVerticalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the bottom.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of what "line" means.
_Check_return_ HRESULT CarouselPanel::LineDownImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    IFC(TranslateVerticalPixelDeltaToOffset(ScrollViewerLineDelta, offset));
    IFC(SetVerticalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the left.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of what "line" means.
_Check_return_ HRESULT CarouselPanel::LineLeftImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    IFC(TranslateHorizontalPixelDeltaToOffset(-ScrollViewerLineDelta, offset));
    IFC(SetHorizontalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the right.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of what "line" means.
_Check_return_ HRESULT CarouselPanel::LineRightImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    IFC(TranslateHorizontalPixelDeltaToOffset(ScrollViewerLineDelta, offset));
    IFC(SetHorizontalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the top.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of what "page" means.
_Check_return_ HRESULT CarouselPanel::PageUpImpl()
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
_Check_return_ HRESULT CarouselPanel::PageDownImpl()
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
_Check_return_ HRESULT CarouselPanel::PageLeftImpl()
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
_Check_return_ HRESULT CarouselPanel::PageRightImpl()
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
_Check_return_ HRESULT CarouselPanel::MouseWheelUpImpl()
{
    RRETURN(MouseWheelUp(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelUp implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
CarouselPanel::MouseWheelUp(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    wf::Size size;

    IFC(get_DesiredSize(&size));
    IFC(TranslateVerticalPixelDeltaToOffset(-GetVerticalScrollWheelDelta(size, mouseWheelDelta), offset));
    IFC(SetVerticalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one mousewheel click to the bottom.
// Subclasses can override this method and call SetVerticalOffset to change
// the behavior of the mouse wheel increment.
_Check_return_ HRESULT CarouselPanel::MouseWheelDownImpl()
{
    RRETURN(MouseWheelDown(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelDown implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
CarouselPanel::MouseWheelDown(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    wf::Size size;

    IFC(get_DesiredSize(&size));
    IFC(TranslateVerticalPixelDeltaToOffset(GetVerticalScrollWheelDelta(size, mouseWheelDelta), offset));
    IFC(SetVerticalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one mousewheel click to the left.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of the mouse wheel increment.
_Check_return_ HRESULT CarouselPanel::MouseWheelLeftImpl()
{
    RRETURN(MouseWheelLeft(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelLeft implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
CarouselPanel::MouseWheelLeft(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    wf::Size size;

    IFC(get_DesiredSize(&size));
    IFC(TranslateHorizontalPixelDeltaToOffset(-GetHorizontalScrollWheelDelta(size, mouseWheelDelta), offset));
    IFC(SetHorizontalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Scroll content by one mousewheel click to the right.
// Subclasses can override this method and call SetHorizontalOffset to change
// the behavior of the mouse wheel increment.
_Check_return_ HRESULT CarouselPanel::MouseWheelRightImpl()
{
    RRETURN(MouseWheelRight(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelRight implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
CarouselPanel::MouseWheelRight(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    wf::Size size;

    IFC(get_DesiredSize(&size));
    IFC(TranslateHorizontalPixelDeltaToOffset(GetHorizontalScrollWheelDelta(size, mouseWheelDelta), offset));
    IFC(SetHorizontalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// Set the HorizontalOffset to the passed value.
_Check_return_ HRESULT CarouselPanel::SetHorizontalOffsetImpl(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    DOUBLE extentWidth = 0.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE scrollX = 0.0;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    BOOLEAN bHorizontal = FALSE;

    if (m_bInMeasure)
    {
        goto Cleanup;
    }

    IFC(get_Orientation(&orientation));
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
        IFC(m_ScrollData.put_OffsetX(scrollX));
        IFC(InvalidateMeasure());

    //    if (_horizontalFlickSB != null || DisableRenderScrolling)
    //    {
    //        _scrollData._offset.X = scrollX;

    //        DOUBLE pixelDelta = TranslateHorizontalOffsetToPixelDelta(_scrollData._arrangedOffset.X, _scrollData._offset.X);

    //        int firstContainerIndex = Math.Max(0, m_iVisibleStart - m_iBeforeTrail);
    //        int lastContainerIndex = Math.Max(0, m_iVisibleStart + m_iVisibleCount - 1);   // beforeTrail is not included in m_iVisibleCount

    //        if (Orientation == Orientation.Horizontal &&
    //            (Math.Floor(-pixelDelta) > m_dPrecacheBeforeTrailSize * 0.5 && firstContainerIndex > 0 ||
    //                Math.Floor(pixelDelta) > m_dPrecacheAfterTrailSize * 0.5 && lastContainerIndex < m_nItemsCount - 1 ||
    //                _translatedOffsetState == null) || DisableRenderScrolling)
    //        {
    //            InvalidateMeasure();
    //        }
    //        else
    //        {
    //            SetAndVerifyScrollingData(_scrollData._viewport, _scrollData._extent, _scrollData._offset);
    //        }
    //    }
    //    else
    //    {
    //        DOUBLE pixelDelta = 0.0;

    //        if (new OffsetMemento(Orientation.Horizontal, RealizedChildren.Count, Children.Count, _scrollData).Equals(_translatedOffsetState) &&
    //            _translatedOffsetState.CurrentOffset == _scrollData._offset.X &&
    //            _translatedOffsetState.RequestedOffset == scrollX)
    //        {
    //            pixelDelta = _translatedOffsetState.Delta;
    //        }
    //        else
    //        {
    //            pixelDelta = TranslateHorizontalOffsetToPixelDelta(_scrollData._offset.X, scrollX);
    //        }

    //        _scrollData._offset.X = scrollX;
    //        TranslateTransform.X -= pixelDelta;

    //        if (Orientation == Orientation.Horizontal &&
    //            (Math.Floor(TranslateTransform.X) > m_dPrecacheBeforeTrailSize ||
    //                Math.Floor(-TranslateTransform.X) > m_dPrecacheAfterTrailSize ||
    //                _translatedOffsetState == null))
    //        {
    //            InvalidateMeasure();
    //        }
    //        else
    //        {
    //            SetAndVerifyScrollingData(_scrollData._viewport, _scrollData._extent, _scrollData._offset);
    //        }
    //    }
    }

Cleanup:
    RRETURN(hr);
}

// CarouselPanel implementation of its public MakeVisible method.
// Does not animate the move by default.
_Check_return_ HRESULT CarouselPanel::MakeVisibleImpl(
    _In_ xaml::IUIElement* visual,
    wf::Rect rectangle,
    _Out_ wf::Rect* resultRectangle)
{
    IFC_RETURN(MakeVisibleImpl(
        visual,
        rectangle,
        FALSE /*useAnimation*/,
        DoubleUtil::NaN /*horizontalAlignmentRatio*/,
        DoubleUtil::NaN /*verticalAlignmentRatio*/,
        0.0 /*offsetX*/,
        0.0 /*offsetY*/,
        resultRectangle));
    return S_OK;
}

// CarouselPanel implementation of IScrollInfo.MakeVisible
// The goal is to change offsets to bring the child into view,
// and return a rectangle in our space to make visible.
// The rectangle we return is in the physical dimension the input target rect
// transformed into our pace.
// In the logical dimension, it is our immediate child's rect.
_Check_return_ HRESULT CarouselPanel::MakeVisibleImpl(
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
    // Implementation of IScrollInfo.MakeVisible not supported by CarouselPanel at this point.
    return S_OK;
}

// CarouselPanel reacts to this property by changing its child measurement algorithm.
// If scrolling in a dimension, infinite space is allowed the child; otherwise, available size is preserved.
_Check_return_ HRESULT CarouselPanel::get_CanHorizontallyScrollImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_ScrollData.m_canHorizontallyScroll;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::put_CanHorizontallyScrollImpl(_In_ BOOLEAN value)
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

// CarouselPanel reacts to this property by changing its child measurement algorithm.
// If scrolling in a dimension, infinite space is allowed the child; otherwise, available size is preserved.
_Check_return_ HRESULT CarouselPanel::get_CanVerticallyScrollImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_ScrollData.m_canVerticallyScroll;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::put_CanVerticallyScrollImpl(_In_ BOOLEAN value)
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
_Check_return_ HRESULT CarouselPanel::get_ExtentWidthImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_extent.Width;

    RRETURN(hr);
}

// ExtentHeight contains the vertical size of the scrolled content element
_Check_return_ HRESULT CarouselPanel::get_ExtentHeightImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_extent.Height;

    RRETURN(hr);
}

// ViewportWidth contains the horizontal size of content's visible range
_Check_return_ HRESULT CarouselPanel::get_ViewportWidthImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_viewport.Width;

    RRETURN(hr);
}

// ViewportHeight contains the vertical size of content's visible range
_Check_return_ HRESULT CarouselPanel::get_ViewportHeightImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_viewport.Height;

    RRETURN(hr);
}

// HorizontalOffset is the horizontal offset of the scrolled content
_Check_return_ HRESULT CarouselPanel::get_HorizontalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_ComputedOffset.X;

    RRETURN(hr);
}

// VerticalOffset is the vertical offset of the scrolled content
_Check_return_ HRESULT CarouselPanel::get_VerticalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_ComputedOffset.Y;

    RRETURN(hr);
}

// MinHorizontalOffset is the minimal horizontal offset of the scrolled content
_Check_return_ HRESULT CarouselPanel::get_MinHorizontalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_MinOffset.X;

    RRETURN(hr);
}

// MinVerticalOffset is the minimal vertical offset of the scrolled content
_Check_return_ HRESULT CarouselPanel::get_MinVerticalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_MinOffset.Y;

    RRETURN(hr);
}

// ScrollOwner is the container that controls any scrollbars, headers, etc... that are dependant
// on this IScrollInfo's properties.
_Check_return_ HRESULT CarouselPanel::get_ScrollOwnerImpl(
    _Outptr_ IInspectable** pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;

    *pValue = NULL;

    IFC(m_ScrollData.get_ScrollOwner(&spOwner));

    IFC(spOwner.MoveTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CarouselPanel::put_ScrollOwnerImpl(
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

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::get_AreHorizontalSnapPointsRegular
//
//  Synopsis:
//    Returns True when the horizontal snap points are equidistant
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CarouselPanel::get_AreHorizontalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    xaml_controls::Orientation orientation;

    IFCPTR(pValue);
    *pValue = FALSE;

    IFC(get_Orientation(&orientation));

    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        // We use the CarouselPanel's AreScrollSnapPointsRegular property to answer the question.
        IFC(get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
        *pValue = areScrollSnapPointsRegular;
    }

    // When the orientation is vertical, there are no horizontal snap points.
    // We simply return FALSE then.

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::get_AreVerticalSnapPointsRegular
//
//  Synopsis:
//    Returns True when the vertical snap points are equidistant
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CarouselPanel::get_AreVerticalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    xaml_controls::Orientation orientation;

    IFCPTR(pValue);
    *pValue = FALSE;

    IFC(get_Orientation(&orientation));

    if (orientation == xaml_controls::Orientation_Vertical)
    {
        // We use the CarouselPanel's AreScrollSnapPointsRegular property to answer the question.
        IFC(get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
        *pValue = areScrollSnapPointsRegular;
    }

    // When the orientation is horizontal, there are no vertical snap points.
    // We simply return FALSE then.

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::GetIrregularSnapPoints
//
//  Synopsis:
//    Returns a read-only collection of numbers representing the snap points for
//    the provided orientation. Returns an empty collection when no snap points are present.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CarouselPanel::GetIrregularSnapPointsImpl(
    // The direction of the requested snap points.
    _In_ xaml_controls::Orientation orientation,
    // The alignment used by the caller when applying the requested snap points.
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    // The read-only collection of snap points.
    _Outptr_ wfc::IVectorView<FLOAT>** pValue)
{
    HRESULT hr = S_OK;
    UINT32 cSnapPoints = 0;
    FLOAT* pSnapPoints = NULL;
    ctl::ComPtr<ValueTypeView<FLOAT>> spSnapPointsVTV;

    IFCPTR(pValue);
    *pValue = NULL;

    IFC(GetIrregularSnapPoints(
        orientation == xaml_controls::Orientation_Horizontal,
        alignment == xaml_primitives::SnapPointsAlignment_Near,
        alignment == xaml_primitives::SnapPointsAlignment_Far,
        &pSnapPoints,
        &cSnapPoints));

    IFC(ctl::make(&spSnapPointsVTV));
    IFC(spSnapPointsVTV->SetView(pSnapPoints, cSnapPoints));

    IFC(spSnapPointsVTV.MoveTo(pValue));

Cleanup:
    delete [] pSnapPoints;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::add_HorizontalSnapPointsChanged
//
//  Synopsis:
//    Adds an event handler for the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
CarouselPanel::add_HorizontalSnapPointsChanged(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
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

    IFC(CarouselPanelGenerated::add_HorizontalSnapPointsChanged(pValue, ptToken));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::remove_HorizontalSnapPointsChanged
//
//  Synopsis:
//    Removes an event handler for the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
CarouselPanel::remove_HorizontalSnapPointsChanged(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(CarouselPanelGenerated::remove_HorizontalSnapPointsChanged(tToken));

    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (pEventSource->HasHandlers())
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
//  Function:   CarouselPanel::add_VerticalSnapPointsChanged
//
//  Synopsis:
//    Adds an event handler for the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
CarouselPanel::add_VerticalSnapPointsChanged(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
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

    IFC(CarouselPanelGenerated::add_VerticalSnapPointsChanged(pValue, ptToken));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::remove_VerticalSnapPointsChanged
//
//  Synopsis:
//    Removes an event handler for the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
CarouselPanel::remove_VerticalSnapPointsChanged(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(CarouselPanelGenerated::remove_VerticalSnapPointsChanged(tToken));

    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (pEventSource->HasHandlers())
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
//  Function:   CarouselPanel::OnAreScrollSnapPointsRegularChanged
//
//  Synopsis:
//    Called when the AreSnapPointsChanged property changed.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::OnAreScrollSnapPointsRegularChanged()
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
//  Function:   CarouselPanel::OnHorizontalSnapPointsChanged
//
//  Synopsis:
//    Raises the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::OnHorizontalSnapPointsChanged()
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<EventArgs> spArgs;

    // Create the args
    IFC(ctl::make(&spArgs));

    // Raise the event
    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spArgs.Get())));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::OnVerticalSnapPointsChanged
//
//  Synopsis:
//    Raises the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::OnVerticalSnapPointsChanged()
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<EventArgs> spArgs;

    // Create the args
    IFC(ctl::make(&spArgs));

    // Raise the event
    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spArgs.Get())));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::GetRegularSnapPointKeys
//
//  Synopsis:
//    Determines the keys for regular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::GetRegularSnapPointKeys(
    _In_ xaml_controls::Orientation orientation,
    _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
    _In_ UINT32 nCount,
    _Out_ FLOAT* pSnapPointKey,
    _Out_ FLOAT* pLowerMarginSnapPointKey)
    //_Out_ FLOAT* pUpperMarginSnapPointKey) Use once horizontal carousel is enabled
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spChild;
    wf::Size childDesiredSize = {};

    IFCEXPECT(pSnapPointKey);
    *pSnapPointKey = 0.0;
    IFCEXPECT(pLowerMarginSnapPointKey);
    *pLowerMarginSnapPointKey = 0.0;
    //IFCEXPECT(pUpperMarginSnapPointKey);
    //*pUpperMarginSnapPointKey = 0.0;

    if (nCount > 0)
    {
        IFCEXPECT(pRealizedChildren);
        IFC(pRealizedChildren->GetAt(0, &spChild));
        if (spChild)
        {
            IFC(spChild->get_DesiredSize(&childDesiredSize));

            if (orientation == xaml_controls::Orientation_Vertical)
            {
                *pSnapPointKey = childDesiredSize.Height;
            }
            else
            {
                *pSnapPointKey = childDesiredSize.Width;
            }
        }
    }

    IFC(GetCommonSnapPointKeys(pLowerMarginSnapPointKey /*, pUpperMarginSnapPointKey*/));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::NotifySnapPointsChanges
//
//  Synopsis:
//    Checks if the snap point keys have changed and a notification needs
//    to be raised.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::NotifySnapPointsChanges(
    _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
    _In_ UINT32 nCount)
{
    HRESULT hr = S_OK;
    INT32 cSnapPointKeys = 0;
    FLOAT* pSnapPointKeys = NULL;
    FLOAT snapPointKeysOffset = 0.0;
    FLOAT snapPointKey = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    //FLOAT upperMarginSnapPointKey = 0.0;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    BOOLEAN notifyForHorizontalSnapPoints = FALSE;
    BOOLEAN notifyForVerticalSnapPoints = FALSE;
    xaml_controls::Orientation orientation;

#if DBG
    IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    IFC(get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
    IFC(get_Orientation(&orientation));

    if (orientation == xaml_controls::Orientation_Vertical)
    {
        if (((m_regularSnapPointKey != -1.0) || (m_cIrregularSnapPointKeys != -1)) &&
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
        if (((m_regularSnapPointKey != -1.0) || (m_cIrregularSnapPointKeys != -1)) &&
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
                IFC(GetRegularSnapPointKeys(orientation, pRealizedChildren, nCount, &snapPointKey, &lowerMarginSnapPointKey /*, &upperMarginSnapPointKey*/));
                if (m_regularSnapPointKey != snapPointKey ||
                    m_lowerMarginSnapPointKey != lowerMarginSnapPointKey)
                    // || m_upperMarginSnapPointKey != upperMarginSnapPointKey)
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
        else if (m_cIrregularSnapPointKeys != -1)
        {
            if (!areScrollSnapPointsRegular)
            {
                IFC(GetIrregularSnapPointKeys(orientation, pRealizedChildren, nCount, &pSnapPointKeys, &cSnapPointKeys, &snapPointKeysOffset, &lowerMarginSnapPointKey /*, &upperMarginSnapPointKey*/));
                if (m_cIrregularSnapPointKeys != cSnapPointKeys ||
                    m_irregularSnapPointKeysOffset != snapPointKeysOffset ||
                    m_lowerMarginSnapPointKey != lowerMarginSnapPointKey)
                    // || m_upperMarginSnapPointKey != upperMarginSnapPointKey)
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
                else
                {
                    for (INT32 iSnapPointKey = 0; iSnapPointKey < cSnapPointKeys; iSnapPointKey++)
                    {
                        if (m_pIrregularSnapPointKeys[iSnapPointKey] != pSnapPointKeys[iSnapPointKey])
                        {
                            if (m_bAreSnapPointsKeysHorizontal)
                            {
                                notifyForHorizontalSnapPoints = TRUE;
                            }
                            else
                            {
                                notifyForVerticalSnapPoints = TRUE;
                            }
                            break;
                        }
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
        IFC(NotifySnapPointsChanges(TRUE /*isForHorizontalSnapPoints*/));
    }

    if (notifyForVerticalSnapPoints)
    {
        IFC(NotifySnapPointsChanges(FALSE /*isForHorizontalSnapPoints*/));
    }

Cleanup:
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::NotifySnapPointsChanges
//
//  Synopsis:
//
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::NotifySnapPointsChanges(_In_ BOOLEAN isForHorizontalSnapPoints)
{
    HRESULT hr = S_OK;

    if ((isForHorizontalSnapPoints && m_bNotifyHorizontalSnapPointsChanges && !m_bNotifiedHorizontalSnapPointsChanges) ||
        (!isForHorizontalSnapPoints && m_bNotifyVerticalSnapPointsChanges && !m_bNotifiedVerticalSnapPointsChanges))
    {
        if (isForHorizontalSnapPoints)
        {
            // Raise HorizontalSnapPointsChanged event.
            IFC(OnHorizontalSnapPointsChanged());
            m_bNotifiedHorizontalSnapPointsChanges = TRUE;
        }
        else
        {
            // Raise VerticalSnapPointsChanged event.
            IFC(OnVerticalSnapPointsChanged());
            m_bNotifiedVerticalSnapPointsChanges = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::RefreshIrregularSnapPointKeys
//
//  Synopsis:
//    Refreshes the m_pIrregularSnapPointKeys/m_cIrregularSnapPointKeys
//    fields based on all children.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::RefreshIrregularSnapPointKeys()
{
    HRESULT hr = S_OK;
    INT32 cSnapPointKeys = 0;
    FLOAT* pSnapPointKeys = NULL;
    FLOAT irregularSnapPointKeysOffset = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    //FLOAT upperMarginSnapPointKey = 0.0;
    UINT32 nCount = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    xaml_controls::Orientation orientation;

#ifdef DBG
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    IFC(get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
    ASSERT(!areScrollSnapPointsRegular);
#endif

    IFC(get_Orientation(&orientation));

    IFC(ResetSnapPointKeys());

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

#if DBG
    IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    IFC(GetIrregularSnapPointKeys(
        orientation,
        spRealizedChildren.Get(),
        nCount,
        &pSnapPointKeys,
        &cSnapPointKeys,
        &irregularSnapPointKeysOffset,
        &lowerMarginSnapPointKey));
        //&upperMarginSnapPointKey)); Use once horizontal carousel is enabled

    m_pIrregularSnapPointKeys = pSnapPointKeys;
    m_cIrregularSnapPointKeys = cSnapPointKeys;
    m_irregularSnapPointKeysOffset = irregularSnapPointKeysOffset;
    m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
    //m_upperMarginSnapPointKey = upperMarginSnapPointKey;
    m_bAreSnapPointsKeysHorizontal = (orientation == xaml_controls::Orientation_Horizontal);
    pSnapPointKeys = NULL;

Cleanup:
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::RefreshRegularSnapPointKeys
//
//  Synopsis:
//    Refreshes the m_regularSnapPointKey field based on a single child.
//    Refreshes also the m_lowerMarginSnapPointKey/m_upperMarginSnapPointKey fields based
//    on the current margins.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::RefreshRegularSnapPointKeys()
{
    HRESULT hr = S_OK;
    FLOAT snapPointKey = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    //FLOAT upperMarginSnapPointKey = 0.0;
    UINT32 nCount = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    xaml_controls::Orientation orientation;

#ifdef DBG
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    IFC(get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
    ASSERT(areScrollSnapPointsRegular);
#endif

    IFC(get_Orientation(&orientation));

    IFC(ResetSnapPointKeys());

    m_regularSnapPointKey = 0.0;

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

#if DBG
    IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    IFC(GetRegularSnapPointKeys(orientation, spRealizedChildren.Get(), nCount, &snapPointKey, &lowerMarginSnapPointKey /*, &upperMarginSnapPointKey*/));

    m_bAreSnapPointsKeysHorizontal = (orientation == xaml_controls::Orientation_Horizontal);
    m_regularSnapPointKey = snapPointKey;
    m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
    //m_upperMarginSnapPointKey = upperMarginSnapPointKey;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::ResetSnapPointKeys
//
//  Synopsis:
//    Resets both regular and irregular snap point keys.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::ResetSnapPointKeys()
{
    delete [] m_pIrregularSnapPointKeys;
    m_pIrregularSnapPointKeys = NULL;
    m_cIrregularSnapPointKeys = -1;
    m_regularSnapPointKey = -1.0;
    m_lowerMarginSnapPointKey = 0;
    //m_upperMarginSnapPointKey = 0; Use once horizontal carousel is enabled

    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Function:   CarouselPanel::SetSnapPointsChangeNotificationsRequirement
//
//  Synopsis:
//    Determines whether the CarouselPanel must call NotifySnapPointsChanged
//    when snap points change or not.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CarouselPanel::SetSnapPointsChangeNotificationsRequirement(
    _In_ BOOLEAN isForHorizontalSnapPoints,
    _In_ BOOLEAN notifyChanges)
{
    HRESULT hr = S_OK;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    xaml_controls::Orientation orientation;

    IFC(get_Orientation(&orientation));
    IFC(get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));

    if (isForHorizontalSnapPoints)
    {
        m_bNotifyHorizontalSnapPointsChanges = notifyChanges;
        if (!m_bInMeasure) // Delay the snap points refresh during a measure pass, otherwise there is an assertion failure in debug_AssertRealizedChildrenEqualVisualChildren.
        {                  // CarouselPanel::ArrangeOverride will update the snap points if needed.
            if (orientation == xaml_controls::Orientation_Horizontal && notifyChanges)
            {
                if (areScrollSnapPointsRegular)
                {
                    IFC(RefreshRegularSnapPointKeys());
                }
                else
                {
                    IFC(RefreshIrregularSnapPointKeys());
                }
                m_bNotifiedHorizontalSnapPointsChanges = FALSE;
            }
        }
    }
    else
    {
        m_bNotifyVerticalSnapPointsChanges = notifyChanges;
        if (!m_bInMeasure) // Delay the snap points refresh during a measure pass, otherwise there is an assertion failure in debug_AssertRealizedChildrenEqualVisualChildren.
        {                  // CarouselPanel::ArrangeOverride will update the snap points if needed.
            if (orientation == xaml_controls::Orientation_Vertical && notifyChanges)
            {
                if (areScrollSnapPointsRegular)
                {
                    IFC(RefreshRegularSnapPointKeys());
                }
                else
                {
                    IFC(RefreshIrregularSnapPointKeys());
                }
                m_bNotifiedVerticalSnapPointsChanges = FALSE;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
CarouselPanel::OnCleanUpVirtualizedItem(_In_ xaml_controls::ICleanUpVirtualizedItemEventArgs* e)
{
    HRESULT hr = S_OK;

    //TODO CSP Fixup
    //ctl::ComPtr<VirtualizingStackPanel::CleanUpVirtualizedItemEventEventSourceType> spEventSource;

    //IFC(GetCleanUpVirtualizedItemEventEventSource(&spEventSource));
    //IFC(spEventSource->Raise(ctl::as_iinspectable(this), e));

//Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
CarouselPanel::OnCleanUpVirtualizedItemProtected(_In_ xaml_controls::ICleanUpVirtualizedItemEventArgs* e)
{
    HRESULT hr = S_OK;
    //TODO CSP ctl::ComPtr<FixupIVirtualizingStackPanelOverrides> spVirtuals;

    //if (IsComposed()) {
    //    IFC(ctl::do_query_interface(spVirtuals, this));
    //    IFC(spVirtuals->OnCleanUpVirtualizedItem(e));
    //}
    //else
    //{
    //    IFC(OnCleanUpVirtualizedItem(e));
    //}

//Cleanup:
    return hr;
}

// Called when the Items collection associated with the containing ItemsControl changes.
IFACEMETHODIMP CarouselPanel::OnItemsChanged(
    _In_ IInspectable* sender,
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    BOOLEAN resetMaximumDesiredSize = FALSE;

    IFC_RETURN(CarouselPanelGenerated::OnItemsChanged(sender, args));

    int intAction;
    IFC_RETURN(args->get_Action(&intAction));
    const auto action = static_cast<wfc::CollectionChange>(intAction);

    switch (action)
    {
        case wfc::CollectionChange_ItemInserted:
            IFC_RETURN(OnItemsAdd(args));
            resetMaximumDesiredSize = TRUE;
            break;

        case wfc::CollectionChange_ItemRemoved:
            IFC_RETURN(OnItemsRemove(args));
            resetMaximumDesiredSize = TRUE;
            break;

        case wfc::CollectionChange_ItemChanged:
            IFC_RETURN(OnItemsReplace(args));
            resetMaximumDesiredSize = TRUE;
            break;

        case wfc::CollectionChange_Reset:
            resetMaximumDesiredSize = TRUE;
            break;
    }

    if (resetMaximumDesiredSize)
    {
        BOOLEAN isScrolling = FALSE;
        IFC_RETURN(get_IsScrolling(&isScrolling));
        if (isScrolling)
        {
            // The items changed such that the maximum size may no longer be valid.
            // The next layout pass will update this value.
            wf::Size empty = {};
            m_ScrollData.m_MaxDesiredSize = empty;
        }
    }

    return S_OK;
}

// Called when the UI collection of children is cleared by the base Panel class.
IFACEMETHODIMP
CarouselPanel::OnClearChildren()
{
    HRESULT hr = S_OK;
    IFC(CarouselPanelGenerated::OnClearChildren());
    m_tpRealizedChildren.Clear();
    m_iVisibleStart =  -1;
    m_iFirstVisibleChildIndex = -1;
    m_iVisibleCount = 0;
    m_InternalOffset = -1;

Cleanup:
    RRETURN(hr);
}

// Returns the extent in logical units in the stacking direction.
_Check_return_
HRESULT
CarouselPanel::ComputeLogicalExtent(
    _In_ wf::Size stackDesiredSize,
    _In_ BOOLEAN isHorizontal,
    _Out_ wf::Size& logicalExtent)
{
    HRESULT hr = S_OK;
    BOOLEAN accumulateExtent = FALSE;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
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
        logicalExtent.Width = static_cast<FLOAT>(m_nItemsCount);
        logicalExtent.Height = static_cast<FLOAT>(accumulateExtent ? DoubleUtil::Max(stackDesiredSize.Height * zoomFactor, m_ScrollData.m_extent.Height) : stackDesiredSize.Height * zoomFactor);
    }
    else
    {
        logicalExtent.Width = static_cast<FLOAT>(accumulateExtent ? DoubleUtil::Max(stackDesiredSize.Width * zoomFactor, m_ScrollData.m_extent.Width) : stackDesiredSize.Width * zoomFactor);
        logicalExtent.Height = static_cast<FLOAT>(m_nItemsCount);
    }

Cleanup:
    RRETURN(hr);
}

// Inserts a new container in the visual tree
_Check_return_
HRESULT
CarouselPanel::InsertNewContainer(
    _In_ INT childIndex,
    _In_ xaml::IUIElement* pChild,
    _Inout_ BOOLEAN& visualOrderChanged)
{
    RRETURN(InsertContainer(childIndex, pChild, FALSE, visualOrderChanged));
}

// Inserts a recycled container in the visual tree
_Check_return_
HRESULT
CarouselPanel::InsertRecycledContainer(
    _In_ INT childIndex,
    _In_ xaml::IUIElement* pChild,
    _Inout_ BOOLEAN& visualOrderChanged)
{
    RRETURN(InsertContainer(childIndex, pChild, TRUE, visualOrderChanged));
}

// Inserts a container into the Children collection.  The container is either new or recycled.
_Check_return_ HRESULT CarouselPanel::InsertContainer(
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
        if (spChildAtPosition.Get() == pChild)
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

    IFC_RETURN(get_ItemContainerGenerator(&spGenerator));
    IFC_RETURN(spGenerator->PrepareItemContainer(static_cast<UIElement*>(pChild)));

    return S_OK;
}

// Takes a container returned from Generator.GenerateNext() and places it in the visual tree if necessary.
// Takes into account whether the container is new, recycled, or already realized.
_Check_return_
HRESULT
CarouselPanel::AddContainerFromGenerator(
    _In_ INT childIndex,
    _In_ xaml::IUIElement* pChild,
    _In_ BOOLEAN newlyRealized,
    _Inout_ BOOLEAN& visualOrderChanged)
{
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
                if (spChildAtPosition.Get() == pChild)
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
                IFC(InsertRecycledContainer(childIndex, pChild, visualOrderChanged));
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
        IFC(InsertNewContainer(childIndex, pChild, visualOrderChanged));
    }

#if DBG
    //Debug.WriteLine("==============================Container has been added=========================================");
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
#endif

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
CarouselPanel::OnItemsAdd(
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    HRESULT hr = S_OK;
    xaml_primitives::GeneratorPosition position = {-1, 0};
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
    ctl::ComPtr<xaml::IUIElement> spUIChild;
    wf::Size childSize = {};
    INT itemCount = 0;
    INT itemUICount = 0;
    INT itemIndex = 0;
    UINT childrenCount = 0;

    IFC(args->get_Position(&position));
    IFC(args->get_ItemCount(&itemCount));
    IFC(args->get_ItemUICount(&itemUICount));

    ASSERT(itemCount <= 1, L"Unexpected items count");
    ASSERT(itemUICount <= 1, L"Unexpected containers count");

    IFC(get_ItemContainerGenerator(&spGenerator));
    IFC(spGenerator->IndexFromGeneratorPosition(position, &itemIndex));

    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&childrenCount));

    if ((m_iVisibleStart <= itemIndex && itemIndex <= m_iVisibleStart + m_iVisibleCount)
        || childrenCount == 0)
    {
        BOOLEAN newlyRealized = FALSE;
        BOOLEAN visualOrderChanged = FALSE;
        IFC(spGenerator->StartAt(position, xaml_primitives::GeneratorDirection_Forward, TRUE));

        IFC(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));
        IFC(spGenerator->Stop());

        // now position should be realized position
        IFC(spGenerator->GeneratorPositionFromIndex(itemIndex, &position));

        if (!ctl::is<xaml::IUIElement>(spChildAsDO))
        {
            ASSERT(!newlyRealized, L"The generator realized a null value.");
            spChildAsDO.Reset();
            // We reached the end of the items (because of a group)
            goto Cleanup;
        }
        IFC(spChildAsDO.As(&spUIChild));
        IFC(AddContainerFromGenerator(position.Index, spUIChild.Get(), newlyRealized, visualOrderChanged));
        m_iFirstVisibleChildIndex = MAX(0, m_iFirstVisibleChildIndex);

        IFC(MeasureChild(spUIChild.Get(), m_ScrollData.m_MaxDesiredSize, &childSize));

        if (childrenCount > 0)
        {
            if (position.Index <= m_iFirstVisibleChildIndex)
            {
                m_iVisibleStart++;
                m_iFirstVisibleChildIndex++;
                m_iBeforeTrail++;
            }
            else
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
CarouselPanel::OnItemsRemove(
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
CarouselPanel::OnItemsReplace(
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
CarouselPanel::RemoveChildRange(
    _In_ xaml_primitives::GeneratorPosition position,
    _In_ INT itemCount,
    _In_ INT itemUICount)
{
    HRESULT hr = S_OK;
    BOOLEAN isItemsHost = FALSE;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;

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
                RemoveInternalChildRange(pos, uiCount);

                if (m_bIsVirtualizing && InRecyclingMode())
                {
                    IFC(get_RealizedChildren(&spRealizedChildren));
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
                    else if (pos > 0)
                    {
                        m_iBeforeTrail--;
                    }
                }
            }
        }
    }
Cleanup:
    RRETURN(hr);
}

_Check_return_
 HRESULT
 CarouselPanel::EnsureRealizedChildren()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<TrackerCollection<xaml::UIElement*>> spRealizedChildren;
    ASSERT(InRecyclingMode(), L"This method only applies to recycling mode");

    if (!m_tpRealizedChildren)
    {
        UINT nCount = 0;
        IFC(get_Children(&spChildren));
        IFC(ctl::make(&spRealizedChildren));

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
 CarouselPanel::debug_VerifyRealizedChildren()
{
    HRESULT hr = S_OK;
    ASSERT(m_bIsVirtualizing && InRecyclingMode(), L"Realized children only exist when recycling");
    ASSERT(m_tpRealizedChildren, L"Realized children must exist to verify it");
    //ItemsControl itemsControl = ItemsControl.GetItemsOwner(this);

    //if (ItemContainerGenerator != null && itemsControl != null)
    //{
    //    foreach (UIElement child in Children)
    //    {
    //        int dataIndex = (IndexFromContainer(child);

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
 CarouselPanel::debug_AssertRealizedChildrenEqualVisualChildren()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    UINT nVisualCount = 0;
    UINT nRealizedCount = 0;
    if (m_bIsVirtualizing && InRecyclingMode())
    {
        IFC(get_Children(&spChildren));
        IFC(spChildren->get_Size(&nVisualCount));
        if (m_tpRealizedChildren)
        {
            IFC(m_tpRealizedChildren->get_Size(&nRealizedCount));
        }
        ASSERT(nRealizedCount == nVisualCount, L"Realized and visual children count must match");

        for (UINT i = 0; i < nVisualCount; i++)
        {
            ctl::ComPtr<xaml::IUIElement> spVisualChild;
            ctl::ComPtr<xaml::IUIElement> spRealizedChild;

            IFC(spChildren->GetAt(i, &spVisualChild));
            IFC(m_tpRealizedChildren->GetAt(i, &spRealizedChild));
            ASSERT(spRealizedChild == spVisualChild, L"Realized and visual children must match");
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
 HRESULT
 CarouselPanel::debug_DumpRealizedChildren()
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
            if (!ctl::is<xaml_controls::IListBoxItem>(spContainer))
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
 CarouselPanel::debug_DumpVisualChildren()
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
            if (!ctl::is<xaml_controls::IListBoxItem>(spContainer))
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
CarouselPanel::ChildIndexFromRealizedIndex(
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
                    childIndex = i;
                    goto Cleanup;
                }
            }

            ASSERT(FALSE, "We should have found a child");
        }
    }
Cleanup:
    RRETURN(hr);
}

// Recycled containers still in the Children collection at the end of Measure should be disconnected
// from the visual tree.  Otherwise they're still visible to things like Arrange, keyboard navigation, etc.
_Check_return_ HRESULT CarouselPanel::DisconnectRecycledContainers()
{
    UINT realizedIndex = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spIGenerator;

    UINT nVisualCount = 0;
    UINT nRealizedCount = 0;

#if DBG
    //Debug.WriteLine("=======================Disconnecting Recycled containers================================");
    //Debug.WriteLine("==============================Pre-disconnect state======================================");
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
#endif
    IFC_RETURN(get_Children(&spChildren));
    IFC_RETURN(spChildren->get_Size(&nVisualCount));

    if (nVisualCount > 0)
    {
        IFC_RETURN(get_RealizedChildren(&spRealizedChildren));
        IFC_RETURN(spRealizedChildren->get_Size(&nRealizedCount));

        for (UINT i = 0; i < nVisualCount; i++)
        {
            ctl::ComPtr<xaml::IUIElement> spVisualChild;
            ctl::ComPtr<xaml::IUIElement> spRealizedChild;

            IFC_RETURN(spChildren->GetAt(i, &spVisualChild));
            if (nRealizedCount > realizedIndex)
            {
                IFC_RETURN(spRealizedChildren->GetAt(realizedIndex, &spRealizedChild));
            }

            if (spVisualChild == spRealizedChild)
            {
                realizedIndex++;
            }
            else
            {
                // The visual child is a recycled container
                IFC_RETURN(spChildren->RemoveAt(i--));
                IFC_RETURN(spChildren->get_Size(&nVisualCount));
            }
        }
    }

#if DBG
    //Debug.WriteLine("==============================Post-disconnect state=====================================");
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
    //Debug.WriteLine("=======================Recycled containers disconnected=================================");

    IFC_RETURN(debug_VerifyRealizedChildren());
    IFC_RETURN(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    // After disconnecting containers we need to ensure we clear the recycled containers list.
    IFC_RETURN(GetItemContainerGenerator(&spIGenerator, nullptr));
    ItemContainerGenerator* pGenerator =  spIGenerator.Cast<ItemContainerGenerator>();
    if (pGenerator)
    {
        IFC_RETURN(pGenerator->ClearRecyclingQueue());
    }

    return S_OK;
}

_Check_return_
HRESULT
CarouselPanel::IndexToGeneratorPositionForStart(
    _In_ INT index,
    _Out_ INT& childIndex,
    _Out_ xaml_primitives::GeneratorPosition& position)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    IFC(get_ItemContainerGenerator(&spGenerator));

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
CarouselPanel::NotifyCleanupItem(
    _In_ IInspectable* pItem,
    _In_ xaml::IUIElement* pChild,
    _In_ xaml_controls::IItemsControl* pItemsControl,
    _Out_ BOOLEAN& bCanceled)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CleanUpVirtualizedItemEventArgs> spArgs;

    bCanceled = FALSE;

    // Create the args
    IFC(ctl::make(&spArgs));
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
CarouselPanel::CleanupRange(
    _In_ INT startIndex,
    _In_ INT count)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    xaml_primitives::GeneratorPosition position = {startIndex, 0};

    IFC(get_ItemContainerGenerator(&spGenerator));

    if (InRecyclingMode())
    {
        ASSERT(startIndex >= 0 && count > 0);
        IFC(get_RealizedChildren(&spRealizedChildren));

        // 1. cancel any transition on this element
        // 2. set lifecycle of these elements to unloaded
        for (INT unloadingIndex = startIndex; unloadingIndex < startIndex + count; unloadingIndex++)
        {
            ctl::ComPtr<xaml::IUIElement> spChild;
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

        // The call to Recycle has caused the ItemContainerGenerator to remove some items
        // from its list of realized items; we adjust _realizedChildren to match.
        for (INT i = startIndex + count; i > startIndex; )
        {
#if DBG
            ctl::ComPtr<xaml::IUIElement> spChild;
            IFC(spRealizedChildren->GetAt(i - 1, &spChild));
            if (ctl::is<xaml::IFrameworkElement>(spChild))
            {
                BOOLEAN hasFocus = FALSE;
                IFC(static_cast<FrameworkElement*>(spChild.Cast<UIElement>())->HasFocus(&hasFocus));
                ASSERT(!hasFocus, L"Recycling focused item");
            }
#endif
            IFC(spRealizedChildren->RemoveAt(--i));
        }
    }
    else
    {
        IFC(spGenerator->Remove(position, count));
        IFC(RemoveInternalChildRange(startIndex, count));
    }
    AdjustFirstVisibleChildIndex(startIndex, count);

Cleanup:
    RRETURN(hr);
}

// Called after 'count' items were removed or recycled from the Generator.  m_iFirstVisibleChildIndex is the
// index of the first visible container.  This index isn't exactly the child position in the UIElement collection;
// it's actually the index of the realized container inside the generator.  Since we've just removed some realized
// containers from the generator (by calling Remove or Recycle), we have to adjust the first visible child index.
void
CarouselPanel::AdjustFirstVisibleChildIndex(_In_ INT startIndex, _In_ INT count)
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

// Sets up IsVirtualizing, VirtualizationMode
//
// IsVirtualizing is TRUE if turned on via the items control and if the panel has a viewport.
// VSP has a viewport if it's either the scrolling panel or it was given MeasureData.
_Check_return_
HRESULT
CarouselPanel::SetVirtualizationState(
    _In_ xaml_controls::IItemsControl* pItemsControl)
{
    HRESULT hr = S_OK;
    xaml_controls::VirtualizationMode mode = xaml_controls::VirtualizationMode_Standard;
    ctl::ComPtr<xaml::IDependencyObject> spDO;

    if (pItemsControl)
    {
        IFC(ctl::do_query_interface(spDO, pItemsControl));

        BOOLEAN isScrolling = FALSE;
        IFC(VirtualizingStackPanelFactory::GetVirtualizationModeStatic(spDO.Get(), &mode))

        // Set IsVirtualizing.  This panel can only virtualize if IsVirtualizing is set on its ItemsControl and it has viewport data.
        // It has viewport data if it's either the scroll host or was given viewport information by measureData.

        // if ItemsControl is using CarouselPanel then ItemsControl is in Virtualizing mode
        // if we want user to control IsVirtualizing property then we should use
        // "if (GetIsVirtualizing(itemsControl) && IsScrolling)"

        IFC(get_IsScrolling(&isScrolling));
        if (isScrolling)
        {
            IFC(put_IsVirtualizing(TRUE));
            if (m_bIsVirtualizing)
            {
                IFC(static_cast<ItemsControl*>(pItemsControl)->SetVirtualizationStateByPanel());
            }
        }
    }
    else
    {
        IFC(put_IsVirtualizing(FALSE));
    }

    m_VirtualizationMode = mode;

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
CarouselPanel::MeasureChild(
    _In_ xaml::IUIElement* pChild,
    _In_ wf::Size layoutSlotSize,
    _Out_opt_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;
    wf::Size childDesiredSize = {};
    wf::Size updatedDesiredSize = {};

    IFC(pChild->get_DesiredSize(&childDesiredSize));
    IFC(pChild->Measure(layoutSlotSize));
    IFC(pChild->get_DesiredSize(&updatedDesiredSize));

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
    RRETURN(hr);
}

_Check_return_
HRESULT
CarouselPanel::ResetScrolling()
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
CarouselPanel::OnScrollChange()
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

// Gets the logical offset given a pixel delta.
_Check_return_
HRESULT
CarouselPanel::ComputeLogicalOffset(
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
CarouselPanel::ComputeTotalRealizedChildDimension(
    _Out_ FLOAT& cumulatedChildDim, _Out_ UINT& nCount)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation;
    wf::Size childDesiredSize = {};

    nCount = 0;
    cumulatedChildDim = 0.0;

    IFC(get_Orientation(&orientation));
    if (!m_tpOrderedChildrenList)
    {
        goto Cleanup;
    }

    IFC(m_tpOrderedChildrenList->get_Size(&nCount));

    if (nCount > 0)
    {
        for (UINT32 childIndex = 0; childIndex < nCount; childIndex++)
        {
            ctl::ComPtr<xaml::IUIElement> spChild;

            IFC(m_tpOrderedChildrenList->GetAt(childIndex, &spChild));
            if (spChild)
            {
                IFC(spChild->get_DesiredSize(&childDesiredSize));

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

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
CarouselPanel::GetGeneratedIndex(
    _In_ INT childIndex,
    _Out_ INT& generatedIndex)
{
    HRESULT hr = S_OK;
    xaml_primitives::GeneratorPosition position = {childIndex, 0};
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;

    generatedIndex = 0;

    IFC(get_ItemContainerGenerator(&spGenerator));
    IFC(spGenerator->IndexFromGeneratorPosition(position, &generatedIndex));

Cleanup:
    RRETURN(hr);
}

// Finds the focused child along with the previous and next focusable children.  Used only when recycling containers;
// the standard mode has a different cleanup algorithm
_Check_return_
HRESULT
CarouselPanel::FindFocusedChildInRealizedChildren(
    _Out_ INT& focusedChild,
    _Out_ INT& previousFocusable,
    _Out_ INT& nextFocusable)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    ctl::ComPtr<xaml::IUIElement> spChild;
    ctl::ComPtr<xaml::IFrameworkElement> spFE;

    focusedChild = previousFocusable = nextFocusable = -1;

    UINT nCount = 0;
    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

    for (UINT i = 0; i < nCount; i++)
    {
        IFC(spRealizedChildren->GetAt(i, &spChild));
        spFE = spChild.AsOrNull<xaml::IFrameworkElement>();

        if (spFE)
        {
            BOOLEAN hasFocus = FALSE;
            IFC(spFE.Cast<FrameworkElement>()->HasFocus(&hasFocus));

            if (hasFocus)
            {
                focusedChild = i;
                previousFocusable = i - 1;
                nextFocusable = i + 1;
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_
HRESULT
CarouselPanel::put_IsVirtualizing(_In_ BOOLEAN isVirtualizing)
{
    HRESULT hr = S_OK;
    BOOLEAN isItemsHost = FALSE;

    IFC(get_IsItemsHost(&isItemsHost));
    isVirtualizing = isVirtualizing && isItemsHost;

    if (!isVirtualizing)
    {
        m_tpRealizedChildren.Clear();
    }

    m_bIsVirtualizing = isVirtualizing;

Cleanup:
    RRETURN(hr);
}

// Returns the list of childen that have been realized by the Generator.
// We must use this method whenever we interact with the Generator's index.
// In recycling mode the Children collection also contains recycled containers and thus does
// not map to the Generator's list.
_Check_return_
HRESULT
CarouselPanel::get_RealizedChildren(
    _Outptr_ wfc::IVector<xaml::UIElement*>** ppRealizedChildren)
{
    HRESULT hr = S_OK;
    IFCPTR(ppRealizedChildren);

    if (m_bIsVirtualizing && InRecyclingMode())
    {
        IFC(EnsureRealizedChildren());
        IFC(m_tpRealizedChildren.CopyTo(ppRealizedChildren));
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
CarouselPanel::TranslateHorizontalPixelDeltaToOffset(
    _In_ DOUBLE delta,
    _Out_ DOUBLE& value)
{
    HRESULT hr = S_OK;
    BOOLEAN canHorizontallyScroll = FALSE;
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

    IFC(get_CanHorizontallyScroll(&canHorizontallyScroll));

    if (!canHorizontallyScroll)
    {
        goto Cleanup;
    }

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
        goto Cleanup;
    }

    IFC(pCurrentOffsetState->put_Delta(delta));
    IFC(pCurrentOffsetState->put_CurrentOffset(currentOffset));

    delete m_pTranslatedOffsetState;
    m_pTranslatedOffsetState = NULL;

    IFC(get_Orientation(&orientation));
    bVertical = (orientation == xaml_controls::Orientation_Vertical);

    if (!m_bIsVirtualizing || bVertical || delta == 0.0)
    {
        offset = currentOffset + delta;
    }
    // During an ItemsSource reset, we recycle everything and nRealizedChildrenCount will be equal to 0.
    // In that case, we should return a zero offset.
    else if (nRealizedChildrenCount != 0u)
    {
        DOUBLE logicalOffset = DoubleUtil::Floor(currentOffset);
        INT currentChildIndex = m_iFirstVisibleChildIndex + (int)logicalOffset - m_iVisibleStart;
        DOUBLE itemLogicalOffset = DoubleUtil::Fractional(currentOffset);

        if (delta < 0)
        {
            while (delta < 0 && currentChildIndex >= 0 && currentChildIndex < static_cast<INT>(nRealizedChildrenCount))
            {
                wf::Size desiredSize = {};
                DOUBLE itemWidth = 0.0;
                DOUBLE itemOffsetLeft = 0.0;

                IFC(spRealizedChildren->GetAt(currentChildIndex, &spChild));
                IFC(spChild->get_DesiredSize(&desiredSize));
                spChild.Reset();

                itemWidth = desiredSize.Width * zoomFactor;
                itemOffsetLeft = itemLogicalOffset * itemWidth;

                itemOffsetLeft += delta;
                if (itemOffsetLeft >= 0)
                {
                    logicalOffset += itemOffsetLeft / itemWidth;
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
                delta = itemOffsetLeft;
            }
        }
        else
        {
            while (delta > 0 && currentChildIndex >= 0 && currentChildIndex < static_cast<INT>(nRealizedChildrenCount))
            {
                wf::Size desiredSize = {};
                DOUBLE itemWidth = 0.0;
                DOUBLE itemOffsetRight = 0.0;

                IFC(spRealizedChildren->GetAt(currentChildIndex, &spChild));
                IFC(spChild->get_DesiredSize(&desiredSize));
                spChild.Reset();

                itemWidth = desiredSize.Width * zoomFactor;
                itemOffsetRight = (1 - itemLogicalOffset) * itemWidth;

                itemOffsetRight -= delta;
                if (itemOffsetRight >= 0)
                {
                    logicalOffset += (itemWidth - itemOffsetRight) / itemWidth;
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
                delta = -itemOffsetRight;
            }
        }

        offset = logicalOffset;
    }

    value = DoubleUtil::Max(offset, 0.0);

Cleanup:
    delete pCurrentOffsetState;
    RRETURN(hr);
}


