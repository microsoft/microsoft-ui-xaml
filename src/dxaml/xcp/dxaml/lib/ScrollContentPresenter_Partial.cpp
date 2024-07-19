// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScrollContentPresenter.g.h"
#include "ScrollViewer.g.h"
#include "ItemsPresenter.g.h"
#include "RectangleGeometry.g.h"
#include "InputPaneThemeTransition.g.h"
#include "TransitionCollection.g.h"
#include "TextBox.g.h"
#include "CalendarPanel.g.h"
#include "AppBar.g.h"
#include "Page.g.h"
#include "Window.g.h"
#include <DependencyLocator.h>
#include <DoubleUtil.h>
#include "focusmgr.h"
#include <DCompTreeHost.h>
#include "VisualTreeHelper.h"
#include <RootScale.h>
#include "LayoutCycleDebugSettings.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation debug outputs, and 0 otherwise
#define DMSCP_DBG 0
//#define DM_DEBUG

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation verbose debug outputs, and 0 otherwise
#define DMSCPv_DBG 0

// Initializes a new instance of the ScrollContentPresenter class.
ScrollContentPresenter::ScrollContentPresenter()
    : m_isClipPropertySet(FALSE)
    , m_isTopLeftHeaderChild(FALSE)
    , m_isTopHeaderChild(FALSE)
    , m_isLeftHeaderChild(FALSE)
    , m_isTabIndexSet(FALSE)
    , m_tabIndex(0)
    , m_isInputPaneShow(FALSE)
    , m_pScrollData(NULL)
    , m_fZoomFactor(1)
    , m_fLastZoomFactorApplied(1)
    , m_isSemanticZoomPresenter(FALSE)
    , m_scrollRequested(FALSE)
    , m_isChildActualWidthUsedAsExtent(false)
    , m_isChildActualHeightUsedAsExtent(false)
    , m_isChildActualWidthUpdated(true)
    , m_isChildActualHeightUpdated(true)
{
    m_unpublishedExtentSize.Width = m_unpublishedExtentSize.Height = 0.0f;
}

// Destroys an instance of the ScrollContentPresenter class.
ScrollContentPresenter::~ScrollContentPresenter()
{
    m_trTopLeftHeader.Clear();
    m_trTopHeader.Clear();
    m_trLeftHeader.Clear();

    delete m_pScrollData;
}

// Hooks up the Unloaded event handler.
_Check_return_ HRESULT ScrollContentPresenter::Initialize()
{
    ctl::ComPtr<xaml::IRoutedEventHandler> unloadedEventHandler;
    EventRegistrationToken unloadedToken;

    IFC_RETURN(ScrollContentPresenterGenerated::Initialize());

    unloadedEventHandler.Attach(
        new ClassMemberEventHandler <
        ScrollContentPresenter,
        xaml_controls::IScrollContentPresenter,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs >(this, &ScrollContentPresenter::OnUnloaded, true /* subscribingToSelf */ ));
    IFC_RETURN(add_Unloaded(unloadedEventHandler.Get(), &unloadedToken));

    static_cast<CUIElement*>(GetHandle())->RegisterAsScroller();

    return S_OK;
}

// Property that controls how ScrollContentPresenter measures its
// Child during layout.  If true, it measures child at infinite
// space in this dimension.
_Check_return_ HRESULT ScrollContentPresenter::get_CanVerticallyScrollImpl(
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN canScroll = FALSE;
    BOOLEAN isScrollClient = FALSE;

    *pValue = FALSE;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        canScroll = pScrollData->m_canVerticallyScroll;
    }

    *pValue = canScroll;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollContentPresenter::put_CanVerticallyScrollImpl(
    _In_ BOOLEAN value)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        if (pScrollData->m_canVerticallyScroll != value)
        {
            pScrollData->m_canVerticallyScroll = value;
            IFC(InvalidateMeasure());
        }
    }

Cleanup:
    RRETURN(hr);
}

// Property that controls how ScrollContentPresenter measures its
// Child during layout.  If true, it measures child at infinite
// space in this dimension.
_Check_return_ HRESULT ScrollContentPresenter::get_CanHorizontallyScrollImpl(
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN canScroll = FALSE;
    BOOLEAN isScrollClient = FALSE;

    *pValue = FALSE;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        canScroll = pScrollData->m_canHorizontallyScroll;
    }

    *pValue = canScroll;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollContentPresenter::put_CanHorizontallyScrollImpl(
    _In_ BOOLEAN value)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        if (pScrollData->m_canHorizontallyScroll != value)
        {
            pScrollData->m_canHorizontallyScroll = value;
            IFC(InvalidateMeasure());
        }
    }

Cleanup:
    RRETURN(hr);
}

// Gets the horizontal size of the extent.
_Check_return_ HRESULT ScrollContentPresenter::get_ExtentWidthImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    *pValue = 0.0;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        *pValue = pScrollData->m_extent.Width;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the vertical size of the extent.
_Check_return_ HRESULT ScrollContentPresenter::get_ExtentHeightImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    *pValue = 0.0;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        *pValue = pScrollData->m_extent.Height;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the horizontal size of the viewport for this content.
_Check_return_ HRESULT ScrollContentPresenter::get_ViewportWidthImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    *pValue = 0.0;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        *pValue = pScrollData->m_viewport.Width;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the vertical size of the viewport for this content.
_Check_return_ HRESULT ScrollContentPresenter::get_ViewportHeightImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    *pValue = 0.0;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        *pValue = pScrollData->m_viewport.Height;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the horizontal offset of the scrolled content.
_Check_return_ HRESULT ScrollContentPresenter::get_HorizontalOffsetImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    *pValue = 0.0;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        *pValue = pScrollData->m_ComputedOffset.X;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the vertical offset of the scrolled content.
_Check_return_ HRESULT ScrollContentPresenter::get_VerticalOffsetImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    *pValue = 0.0;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        *pValue = pScrollData->m_ComputedOffset.Y;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the minimal horizontal offset of the scrolled content.
_Check_return_ HRESULT ScrollContentPresenter::get_MinHorizontalOffsetImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    *pValue = 0.0;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        *pValue = pScrollData->m_MinOffset.X;
    }

Cleanup:
    RRETURN(hr);
}

// Gets the minimal vertical offset of the scrolled content.
_Check_return_ HRESULT ScrollContentPresenter::get_MinVerticalOffsetImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    *pValue = 0.0;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        *pValue = pScrollData->m_MinOffset.Y;
    }

Cleanup:
    RRETURN(hr);
}

// ScrollOwner is the container that controls any scrollbars,
// headers, etc... that are dependent on this IScrollInfo's
// properties.  Implementers of IScrollInfo should call
// InvalidateScrollInfo() on this object when properties change.
_Check_return_ HRESULT ScrollContentPresenter::get_ScrollOwnerImpl(
    _Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;
    BOOLEAN isScrollClient = FALSE;

    *ppValue = NULL;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        IFC(pScrollData->get_ScrollOwner(&spOwner));
    }

    IFC(spOwner.MoveTo(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollContentPresenter::put_ScrollOwnerImpl(
    _In_opt_ IInspectable* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;
    BOOLEAN isScrollClient = FALSE;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        ScrollData* pScrollData = NULL;
        IFC(get_ScrollData(&pScrollData));
        IFC(ctl::do_query_interface(spOwner, pValue));
        IFC(pScrollData->put_ScrollOwner(spOwner.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the top.
_Check_return_ HRESULT ScrollContentPresenter::LineUpImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:LineUpImpl.", this));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        IFC(get_VerticalOffset(&offset));
        IFC(SetVerticalOffset(offset - ScrollViewerLineDelta));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the bottom.
_Check_return_ HRESULT ScrollContentPresenter::LineDownImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:LineDownImpl.", this));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        IFC(get_VerticalOffset(&offset));
        IFC(SetVerticalOffset(offset + ScrollViewerLineDelta));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the left.
_Check_return_ HRESULT ScrollContentPresenter::LineLeftImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:LineLeftImpl.", this));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        IFC(get_HorizontalOffset(&offset));
        IFC(SetHorizontalOffset(offset - ScrollViewerLineDelta));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the right.
_Check_return_ HRESULT ScrollContentPresenter::LineRightImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:LineRightImpl.", this));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        IFC(get_HorizontalOffset(&offset));
        IFC(SetHorizontalOffset(offset + ScrollViewerLineDelta));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the top.
_Check_return_ HRESULT ScrollContentPresenter::PageUpImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:PageUpImpl.", this));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        DOUBLE viewport = 0.0;
        XSIZEF sizeHeaders = {};
        IFC(get_VerticalOffset(&offset));
        IFC(get_ViewportHeight(&viewport));
        IFC(GetZoomedHeadersSize(&sizeHeaders));
        viewport = DoubleUtil::Max(ScrollViewerLineDelta, viewport - sizeHeaders.height);
        IFC(SetVerticalOffset(offset - viewport));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the bottom.
_Check_return_ HRESULT ScrollContentPresenter::PageDownImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:PageDownImpl.", this));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        DOUBLE viewport = 0.0;
        XSIZEF sizeHeaders = {};
        IFC(get_VerticalOffset(&offset));
        IFC(get_ViewportHeight(&viewport));
        IFC(GetZoomedHeadersSize(&sizeHeaders));
        viewport = DoubleUtil::Max(ScrollViewerLineDelta, viewport - sizeHeaders.height);
        IFC(SetVerticalOffset(offset + viewport));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the left.
_Check_return_ HRESULT ScrollContentPresenter::PageLeftImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:PageLeftImpl.", this));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        DOUBLE viewport = 0.0;
        XSIZEF sizeHeaders = {};
        IFC(get_HorizontalOffset(&offset));
        IFC(get_ViewportWidth(&viewport));
        IFC(GetZoomedHeadersSize(&sizeHeaders));
        viewport = DoubleUtil::Max(ScrollViewerLineDelta, viewport - sizeHeaders.width);
        IFC(SetHorizontalOffset(offset - viewport));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the right.
_Check_return_ HRESULT ScrollContentPresenter::PageRightImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:PageRightImpl.", this));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        DOUBLE viewport = 0.0;
        XSIZEF sizeHeaders = {};
        IFC(get_HorizontalOffset(&offset));
        IFC(get_ViewportWidth(&viewport));
        IFC(GetZoomedHeadersSize(&sizeHeaders));
        viewport = DoubleUtil::Max(ScrollViewerLineDelta, viewport - sizeHeaders.width);
        IFC(SetHorizontalOffset(offset + viewport));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the top.
_Check_return_ HRESULT ScrollContentPresenter::MouseWheelUpImpl()
{
    RRETURN(MouseWheelUp(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelUp implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
ScrollContentPresenter::MouseWheelUp(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:MouseWheelUp - mouseWheelDelta=%d.",
            this, mouseWheelDelta));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        BOOLEAN canVerticallyScroll = FALSE;
        DOUBLE offset = 0.0;
        wf::Size size;

        IFC(get_DesiredSize(&size));
        IFC(get_CanVerticallyScroll(&canVerticallyScroll));
        if (canVerticallyScroll)
        {
            IFC(get_VerticalOffset(&offset));
            IFC(SetVerticalOffset(offset - GetVerticalScrollWheelDelta(size, mouseWheelDelta)));
        }
        else
        {
            IFC(get_HorizontalOffset(&offset));
            IFC(SetHorizontalOffset(offset - GetHorizontalScrollWheelDelta(size, mouseWheelDelta)));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one line to the bottom.
_Check_return_ HRESULT ScrollContentPresenter::MouseWheelDownImpl()
{
    RRETURN(MouseWheelDown(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelDown implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
ScrollContentPresenter::MouseWheelDown(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:MouseWheelDown - mouseWheelDelta=%d.",
            this, mouseWheelDelta));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        BOOLEAN canVerticallyScroll = FALSE;
        DOUBLE offset = 0.0;
        wf::Size size;

        IFC(get_DesiredSize(&size));
        IFC(get_CanVerticallyScroll(&canVerticallyScroll));
        if (canVerticallyScroll)
        {
            IFC(get_VerticalOffset(&offset));
            IFC(SetVerticalOffset(offset + GetVerticalScrollWheelDelta(size, mouseWheelDelta)));
        }
        else
        {
            IFC(get_HorizontalOffset(&offset));
            IFC(SetHorizontalOffset(offset + GetHorizontalScrollWheelDelta(size, mouseWheelDelta)));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the left.
_Check_return_ HRESULT ScrollContentPresenter::MouseWheelLeftImpl()
{
    RRETURN(MouseWheelLeft(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelLeft implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
ScrollContentPresenter::MouseWheelLeft(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:MouseWheelLeft - mouseWheelDelta=%d.",
            this, mouseWheelDelta));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        wf::Size size;

        IFC(get_DesiredSize(&size));
        IFC(get_HorizontalOffset(&offset));
        IFC(SetHorizontalOffset(offset - GetHorizontalScrollWheelDelta(size, mouseWheelDelta)));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll content by one page to the right.
_Check_return_ HRESULT ScrollContentPresenter::MouseWheelRightImpl()
{
    RRETURN(MouseWheelRight(ScrollViewerDefaultMouseWheelDelta));
}

// IScrollInfo::MouseWheelRight implementation which takes the mouse wheel delta into account.
IFACEMETHODIMP
ScrollContentPresenter::MouseWheelRight(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:MouseWheelRight - mouseWheelDelta=%d.",
            this, mouseWheelDelta));
    }
#endif // DM_DEBUG

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        DOUBLE offset = 0.0;
        wf::Size size;

        IFC(get_DesiredSize(&size));
        IFC(get_HorizontalOffset(&offset));
        IFC(SetHorizontalOffset(offset + GetHorizontalScrollWheelDelta(size, mouseWheelDelta)));
    }

Cleanup:
    RRETURN(hr);
}

// Set the HorizontalOffset to the passed value.
_Check_return_ HRESULT ScrollContentPresenter::SetHorizontalOffsetImpl(
    _In_ DOUBLE offset)
{
#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:SetHorizontalOffsetImpl - offset=%f.",
            this, offset));
    }
#endif // DM_DEBUG

    IFC_RETURN(SetHorizontalOffsetPrivate(offset));
    return S_OK;
}

_Check_return_ HRESULT ScrollContentPresenter::SetHorizontalOffsetPrivate(
    double offset, _Out_opt_ bool* isScrollRequested, _Out_opt_ double* currentOffset, _Out_opt_ double* requestedOffset)
{
    BOOLEAN canHorizontallyScroll = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:SetHorizontalOffsetPrivate - offset=%f.",
            this, offset));
    }
#endif // DM_DEBUG

    if (isScrollRequested)
    {
        *isScrollRequested = false;
    }
    if (currentOffset)
    {
        *currentOffset = 0.0;
    }

    IFC_RETURN(get_CanHorizontallyScroll(&canHorizontallyScroll));
    if (canHorizontallyScroll)
    {
        DOUBLE scrollX = 0.0;
        DOUBLE currentX = 0.0;
        ScrollData* pScrollData = NULL;
        DOUBLE extentWidth = 0.0;
        DOUBLE viewportWidth = 0.0;

        IFC_RETURN(get_ExtentWidth(&extentWidth));
        IFC_RETURN(get_ViewportWidth(&viewportWidth));

        IFC_RETURN(get_ScrollData(&pScrollData));
        IFC_RETURN(ValidateInputOffset(offset, pScrollData->m_MinOffset.X, extentWidth - viewportWidth, &scrollX));

        currentX = pScrollData->get_OffsetX();

        if (currentOffset)
        {
            *currentOffset = currentX;
        }

        if (!DoubleUtil::AreClose(currentX, scrollX))
        {
            IFC_RETURN(pScrollData->put_OffsetX(scrollX));
            IFC_RETURN(InvalidateArrange());
            m_scrollRequested = TRUE;
            if (isScrollRequested)
            {
                *isScrollRequested = true;
            }
            if (requestedOffset)
            {
                *requestedOffset = scrollX;
            }
        }
    }
    return S_OK;
}

// Set the VerticalOffset to the passed value.
_Check_return_ HRESULT ScrollContentPresenter::SetVerticalOffsetImpl(
    _In_ DOUBLE offset)
{
#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:SetVerticalOffsetImpl - offset=%f.",
            this, offset));
    }
#endif // DM_DEBUG

    IFC_RETURN(SetVerticalOffsetPrivate(offset));
    return S_OK;
}

_Check_return_ HRESULT ScrollContentPresenter::SetVerticalOffsetPrivate(
    double offset, _Out_opt_ bool* isScrollRequested, _Out_opt_ double* currentOffset, _Out_opt_ double* requestedOffset)
{
    BOOLEAN canVerticallyScroll = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:SetVerticalOffsetPrivate - offset=%f.",
            this, offset));
    }
#endif // DM_DEBUG

    if (isScrollRequested)
    {
        *isScrollRequested = false;
    }
    if (currentOffset)
    {
        *currentOffset = 0.0;
    }

    IFC_RETURN(get_CanVerticallyScroll(&canVerticallyScroll));
    if (canVerticallyScroll)
    {
        DOUBLE scrollY = 0.0;
        DOUBLE currentY = 0.0;
        ScrollData* pScrollData = NULL;
        DOUBLE extentHeight = 0.0;
        DOUBLE viewportHeight = 0.0;

        IFC_RETURN(get_ExtentHeight(&extentHeight));
        IFC_RETURN(get_ViewportHeight(&viewportHeight));

        IFC_RETURN(get_ScrollData(&pScrollData));
        IFC_RETURN(ValidateInputOffset(offset, pScrollData->m_MinOffset.Y, extentHeight - viewportHeight, &scrollY));

        currentY = pScrollData->get_OffsetY();

        if (currentOffset)
        {
            *currentOffset = currentY;
        }

        if (!DoubleUtil::AreClose(currentY, scrollY))
        {
            IFC_RETURN(pScrollData->put_OffsetY(scrollY));
            IFC_RETURN(InvalidateArrange());
            m_scrollRequested = TRUE;
            if (isScrollRequested)
            {
                *isScrollRequested = true;
            }
            if (requestedOffset)
            {
                *requestedOffset = scrollY;
            }
        }
    }
    return S_OK;
}

// Set the HorizontalOffset and VerticalOffset to the passed values, using the provided extents to determine the upper boundaries.
_Check_return_ HRESULT ScrollContentPresenter::SetOffsetsWithExtents(
    _In_ DOUBLE offsetX,
    _In_ DOUBLE offsetY,
    _In_ DOUBLE extentWidth,
    _In_ DOUBLE extentHeight)
{
    HRESULT hr = S_OK;
    BOOLEAN bCanHorizontallyScroll = FALSE;
    BOOLEAN bCanVerticallyScroll = FALSE;
    BOOLEAN bIsOffsetChanged = FALSE;
    ScrollData* pScrollData = NULL;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:SetOffsetsWithExtents offsetX=%f, offsetY=%f, extentWidth=%f, extentHeight=%f.",
            this, offsetX, offsetY, extentWidth, extentHeight));
    }
#endif // DM_DEBUG

    IFC(get_CanHorizontallyScroll(&bCanHorizontallyScroll));
    if (bCanHorizontallyScroll)
    {
        DOUBLE scrollX = 0.0;
        DOUBLE currentX = 0.0;
        DOUBLE viewportWidth = 0.0;

        IFC(get_ViewportWidth(&viewportWidth));

        IFC(get_ScrollData(&pScrollData));
        IFC(ValidateInputOffset(offsetX, pScrollData->m_MinOffset.X, extentWidth - viewportWidth, &scrollX));

        currentX = pScrollData->get_OffsetX();
        if (!DoubleUtil::AreClose(currentX, scrollX))
        {
            pScrollData->put_OffsetX(scrollX);
            bIsOffsetChanged = TRUE;
        }
    }

    IFC(get_CanVerticallyScroll(&bCanVerticallyScroll));
    if (bCanVerticallyScroll)
    {
        DOUBLE scrollY = 0.0;
        DOUBLE currentY = 0.0;
        DOUBLE viewportHeight = 0.0;

        IFC(get_ViewportHeight(&viewportHeight));

        if (!pScrollData)
        {
            IFC(get_ScrollData(&pScrollData));
        }
        IFC(ValidateInputOffset(offsetY, pScrollData->m_MinOffset.Y, extentHeight - viewportHeight, &scrollY));

        currentY = pScrollData->get_OffsetY();
        if (!DoubleUtil::AreClose(currentY, scrollY))
        {
            pScrollData->put_OffsetY(scrollY);
            bIsOffsetChanged = TRUE;
        }
    }

    if (bIsOffsetChanged)
    {
        IFC(InvalidateArrange());
        m_scrollRequested = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

// ScrollContentPresenter implementation of its public MakeVisible method.
// Does not animate the move by default.
_Check_return_ HRESULT ScrollContentPresenter::MakeVisibleImpl(
    // The element that should become visible.
    _In_ xaml::IUIElement* visual,
    // A rectangle representing in the visual's coordinate space to
    // make visible.
    wf::Rect rectangle,
    // A rectangle in the IScrollInfo's coordinate space that has
    // been made visible.  Other ancestors to in turn make this new
    // rectangle visible.  The rectangle should generally be a
    // transformed version of the input rectangle.  In some cases,
    // like when the input rectangle cannot entirely fit in the
    // viewport, the return value might be smaller.
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

// This scrolls to make the rectangle in the UIElement's coordinate
// space visible.
// Alignment ratios are either -1 (i.e. no alignment to apply) or between
// 0 and 1. For instance when the alignment ratio is 0, the near edge of
// the 'rectangle' needs to align with the near edge of the viewport.
// 'offset' is an additional amount of scrolling requested, beyond the
// normal amount to bring the target into view and potentially align it.
// That additional offset is only applied when the 'rectangle' does not
// step outside the extents.
// The 'appliedOffset' returned specifies how much of 'offset' was applied
// so that potential parent bring-into-view contributors can attempt to
// apply the remainder offset.
_Check_return_ HRESULT ScrollContentPresenter::MakeVisibleImpl(
    // The element that should become visible.
    _In_ xaml::IUIElement* visual,
    // A rectangle representing in the visual's coordinate space to make visible.
    wf::Rect rectangle,
    // When set to True, the DManip ZoomToRect method is invoked.
    BOOLEAN useAnimation,
    DOUBLE horizontalAlignmentRatio,
    DOUBLE verticalAlignmentRatio,
    DOUBLE offsetX,
    DOUBLE offsetY,
    // A rectangle in the IScrollInfo's coordinate space that has
    // been made visible.  Other ancestors to in turn make this new
    // rectangle visible.  The rectangle should generally be a
    // transformed version of the input rectangle.  In some cases,
    // like when the input rectangle cannot entirely fit in the
    // viewport, the return value might be smaller.
    _Out_ wf::Rect* resultRectangle,
    _Out_opt_ DOUBLE* appliedOffsetX,
    _Out_opt_ DOUBLE* appliedOffsetY)
{
    BOOLEAN isEmpty = FALSE;
    BOOLEAN isAncestor = FALSE;
    BOOLEAN isVisualDirectChild = FALSE;
    BOOLEAN isVisualInTopLeftHeader = FALSE;
    BOOLEAN isVisualInTopHeader = FALSE;
    BOOLEAN isVisualInLeftHeader = FALSE;
    BOOLEAN isVisualInContent = FALSE;
    wf::Rect transformedRect = {};
    wf::Rect viewport = {};
    wf::Rect unhandledRect = {};
    DOUBLE horizontalOffset = 0.0;
    DOUBLE verticalOffset = 0.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE viewportHeight = 0.0;
    FLOAT viewportLeft = 0.0;
    FLOAT viewportRight = 0.0;
    FLOAT viewportTop = 0.0;
    FLOAT viewportBottom = 0.0;
    FLOAT rectLeft = 0.0;
    FLOAT rectRight = 0.0;
    FLOAT rectTop = 0.0;
    FLOAT rectBottom = 0.0;
    FLOAT minX = 0.0;
    FLOAT minY = 0.0;
    FLOAT zoomFactor = 1.0;
    FLOAT targetZoomFactor = 1.0;
    DOUBLE appliedOffsetXTmp = 0.0;
    DOUBLE appliedOffsetYTmp = 0.0;
    XSIZEF sizeHeaders = {};
    ctl::ComPtr<IDependencyObject> spVisualAsDO;
    ctl::ComPtr<IInspectable> spHorizontalOffset;
    ctl::ComPtr<IInspectable> spVerticalOffset;
    ctl::ComPtr<wf::IReference<DOUBLE>> spHorizontalOffsetReference;
    ctl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffsetReference;
    ctl::ComPtr<Page> spPageBetween;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: MakeVisibleImpl - rectangle=(%f, %f, %f, %f), useAnimation=%d.",
            this, rectangle.X, rectangle.Y, rectangle.Width, rectangle.Height, useAnimation));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"                   horizontalAlignmentRatio=%lf, verticalAlignmentRatio=%lf, offsetX=%lf, offsetY=%lf).",
            horizontalAlignmentRatio, verticalAlignmentRatio, offsetX, offsetY));
    }
#endif // DM_DEBUG

    if (appliedOffsetX)
    {
        *appliedOffsetX = 0.0;
    }

    if (appliedOffsetY)
    {
        *appliedOffsetY = 0.0;
    }

    // Handle cases where we don't have to do anything
    IFC_RETURN(RectUtil::GetIsEmpty(rectangle, &isEmpty));
    isEmpty = isEmpty || !visual || visual == this;
    if (!isEmpty)
    {
        IFC_RETURN(IsAncestorOfAndMostAncestorPageBetween(static_cast<UIElement*>(visual), &isAncestor, &spPageBetween));
    }
    if (isEmpty || !isAncestor)
    {
        rectangle = RectUtil::CreateEmptyRect();
    }
    else
    {
        ctl::ComPtr<xaml_media::IGeneralTransform> spChildTransform;
        BOOLEAN isScrollClient = FALSE;
        BOOLEAN handled = FALSE;

        // Compute the child's rect relative to (0,0) in our coordinate space.
        IFC_RETURN(visual->TransformToVisual(this, &spChildTransform));
        IFC_RETURN(spChildTransform->TransformBounds(rectangle, &transformedRect));

        rectangle = transformedRect;

        // Rectangle to return in case ChangeView is a no-op.
        unhandledRect = rectangle;

        // Compute the area taken up by the potential ScrollViewer headers
        IFC_RETURN(GetZoomedHeadersSize(&sizeHeaders));

        // Adjust the target rectangle based on those headers
        rectangle.X -= sizeHeaders.width;
        rectangle.Y -= sizeHeaders.height;

        IFC_RETURN(IsScrollClient(&isScrollClient));
        if (isScrollClient)
        {
            // Check if visual belongs to a header.
            IFC_RETURN(ctl::do_query_interface<xaml::IDependencyObject>(spVisualAsDO, visual));
            IFC_RETURN(GetHeaderOwnership(
                spVisualAsDO.Cast<DependencyObject>(),
                &isVisualDirectChild,
                &isVisualInTopLeftHeader,
                &isVisualInTopHeader,
                &isVisualInLeftHeader,
                &isVisualInContent));

            if (!isVisualInTopLeftHeader)
            {
                ctl::ComPtr<IScrollOwner> spScrollOwner;
                ctl::ComPtr<IScrollViewer> spScrollViewer;

                IFC_RETURN(get_ScrollOwner(&spScrollOwner));
                if (spScrollOwner != nullptr)
                {
                    IFC_RETURN(spScrollOwner.As(&spScrollViewer));
                }

                // Initialize the viewport
                if (spScrollViewer && useAnimation)
                {
                    if (spScrollViewer.Cast<ScrollViewer>()->IsInManipulation())
                    {
                        DOUBLE targetHorizontalOffset = 0.0;
                        DOUBLE targetVerticalOffset = 0.0;

                        // MakeVisible is invoked during a manipulation. Access the current DManip Service view for the owning ScrollViewer,
                        // as well as the final view.
                        IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->GetDManipView(&horizontalOffset, &verticalOffset, &zoomFactor));
                        IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->GetTargetView(&targetHorizontalOffset, &targetVerticalOffset, &targetZoomFactor));
                        // Use the target view instead of the current view to avoid overriding the ongoing request. If no target view could be
                        // provided fall back to using the current view.
                        if (targetHorizontalOffset != -1.0 && targetVerticalOffset != -1.0 && targetZoomFactor != -1.0f)
                        {
                            ASSERT(zoomFactor == targetZoomFactor);

                            rectangle.X += static_cast<FLOAT>(horizontalOffset - targetHorizontalOffset);
                            rectangle.Y += static_cast<FLOAT>(verticalOffset - targetVerticalOffset);

                            horizontalOffset = targetHorizontalOffset;
                            verticalOffset = targetVerticalOffset;
                        }
                    }
                    else
                    {
                        // Take into account the overbounce offsets which are reflected in the spChildTransform transform.
                        horizontalOffset = spScrollViewer.Cast<ScrollViewer>()->GetUnboundHorizontalOffset();
                        verticalOffset = spScrollViewer.Cast<ScrollViewer>()->GetUnboundVerticalOffset();
                    }
                }
                else
                {
                    IFC_RETURN(get_HorizontalOffset(&horizontalOffset));
                    IFC_RETURN(get_VerticalOffset(&verticalOffset));
                }

                // Compute the offsets required to minimally scroll the child maximally into view.

                if (isVisualInLeftHeader)
                {
                    // visual is not allowed to scroll horizontally
                    minX = static_cast<FLOAT>(horizontalOffset);
                }
                else
                {
                    IFC_RETURN(get_ViewportWidth(&viewportWidth));
                    viewport.X = static_cast<FLOAT>(horizontalOffset);
                    viewport.Width = MAX(0.0f, static_cast<FLOAT>(viewportWidth)-sizeHeaders.width);
                    rectangle.X += static_cast<FLOAT>(horizontalOffset);

                    wf::Rect rectangleWithAlignment = rectangle;

                    if (!DoubleUtil::IsNaN(horizontalAlignmentRatio))
                    {
                        // Account for the horizontal alignment ratio.
                        ASSERT(horizontalAlignmentRatio >= 0.0 && horizontalAlignmentRatio <= 1.0);
                        rectangleWithAlignment.X += static_cast<FLOAT>((rectangleWithAlignment.Width - viewport.Width) * horizontalAlignmentRatio);
                        rectangleWithAlignment.Width = viewport.Width;
                    }

                    IFC_RETURN(RectUtil::GetLeft(viewport, &viewportLeft));
                    IFC_RETURN(RectUtil::GetRight(viewport, &viewportRight));
                    IFC_RETURN(RectUtil::GetLeft(rectangleWithAlignment, &rectLeft));
                    IFC_RETURN(RectUtil::GetRight(rectangleWithAlignment, &rectRight));
                    IFC_RETURN(ComputeScrollOffsetWithMinimalScroll(viewportLeft, viewportRight, rectLeft, rectRight, &minX));

                    // If the target offset is within bounds and an offset was provided, apply as much of it as possible while remaining within bounds.
                    if (offsetX != 0.0 && minX >= 0.0f && spScrollViewer)
                    {
                        DOUBLE scrollableWidth = 0.0;

                        IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->get_ScrollableWidth(&scrollableWidth));
                        if (minX <= scrollableWidth)
                        {
                            if (offsetX > 0.0)
                            {
                                appliedOffsetXTmp = MIN(minX, offsetX);
                            }
                            else
                            {
                                appliedOffsetXTmp = -MIN(static_cast<FLOAT>(scrollableWidth) - minX, -offsetX);
                            }
                            minX -= static_cast<FLOAT>(offsetX);
                        }
                    }
                }

                if (isVisualInTopHeader)
                {
                    // visual is not allowed to scroll vertically
                    minY = static_cast<FLOAT>(verticalOffset);
                }
                else
                {
                    // if applicable additionally reduce the viewport height by the space occluded by a page bottom appbar
                    DOUBLE pageBottomAppBarScrollOffset = 0.0;
                    if (spPageBetween)
                    {
                        IFC_RETURN(GetFullScreenPageBottomAppBarHeight(spPageBetween.Get(), &pageBottomAppBarScrollOffset));
                    }

                    IFC_RETURN(get_ViewportHeight(&viewportHeight));
                    viewport.Y = static_cast<FLOAT>(verticalOffset);
                    viewport.Height = MAX(0.0f, static_cast<FLOAT>(viewportHeight - pageBottomAppBarScrollOffset)-sizeHeaders.height);
                    rectangle.Y += static_cast<FLOAT>(verticalOffset);

                    wf::Rect rectangleWithAlignment = rectangle;

                    if (!DoubleUtil::IsNaN(verticalAlignmentRatio))
                    {
                        // Account for the vertical alignment ratio.
                        ASSERT(verticalAlignmentRatio >= 0.0 && verticalAlignmentRatio <= 1.0);
                        rectangleWithAlignment.Y += static_cast<FLOAT>((rectangleWithAlignment.Height - viewport.Height) * verticalAlignmentRatio);
                        rectangleWithAlignment.Height = viewport.Height;
                    }

                    IFC_RETURN(RectUtil::GetTop(viewport, &viewportTop));
                    IFC_RETURN(RectUtil::GetBottom(viewport, &viewportBottom));
                    IFC_RETURN(RectUtil::GetTop(rectangleWithAlignment, &rectTop));
                    IFC_RETURN(RectUtil::GetBottom(rectangleWithAlignment, &rectBottom));
                    IFC_RETURN(ComputeScrollOffsetWithMinimalScroll(viewportTop, viewportBottom, rectTop, rectBottom, &minY));

                    // If the target offset is within bounds and an offset was provided, apply as much of it as possible while remaining within bounds.
                    if (offsetY != 0.0 && minY >= 0.0f && spScrollViewer)
                    {
                        DOUBLE scrollableHeight = 0.0;

                        IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->get_ScrollableHeight(&scrollableHeight));
                        if (minY <= scrollableHeight)
                        {
                            if (offsetY > 0.0)
                            {
                                appliedOffsetYTmp = MIN(minY, offsetY);
                            }
                            else
                            {
                                appliedOffsetYTmp = -MIN(static_cast<FLOAT>(scrollableHeight) - minY, -offsetY);
                            }
                            minY -= static_cast<FLOAT>(offsetY);
                        }
                    }
                }

                // We have computed the scrolling offsets; scroll to them.
                if (spScrollViewer && useAnimation)
                {
                    DOUBLE targetHorizontalOffset = static_cast<DOUBLE>(MAX(0, minX));
                    DOUBLE targetVerticalOffset = static_cast<DOUBLE>(MAX(0, minY));

                    // No need to call ChangeView during a manipulation if the requested view coincides with the final view.
                    if (!spScrollViewer.Cast<ScrollViewer>()->IsInManipulation() ||
                        !DoubleUtil::AreClose(horizontalOffset, targetHorizontalOffset) ||
                        !DoubleUtil::AreClose(verticalOffset, targetVerticalOffset))
                    {
                        IFC_RETURN(PropertyValue::CreateFromDouble(targetHorizontalOffset, &spHorizontalOffset));
                        IFC_RETURN(spHorizontalOffset.As(&spHorizontalOffsetReference));
                        IFC_RETURN(PropertyValue::CreateFromDouble(targetVerticalOffset, &spVerticalOffset));
                        IFC_RETURN(spVerticalOffset.As(&spVerticalOffsetReference));
                        // disableAnimation is FALSE by default, which is what we want here.
#ifdef DM_DEBUG
                        if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                        {
                            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                L"                   Calling ChangeView(ho=%f, vo=%f, null).", targetHorizontalOffset, targetVerticalOffset));
                        }
#endif // DM_DEBUG
                        IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->ChangeViewInternal(
                            spHorizontalOffsetReference.Get() /*pHorizontalOffset*/,
                            spVerticalOffsetReference.Get()   /*pVerticalOffset*/,
                            NULL  /*pZoomFactor*/,
                            NULL  /*pOldZoomFactor*/,
                            FALSE /*forceChangeToCurrentView*/,
                            TRUE  /*adjustWithMandatorySnapPoints*/,
                            TRUE  /*skipDuringTouchContact*/,
                            TRUE  /*skipAnimationWhileRunning*/,
                            FALSE /*disableAnimation*/,
                            TRUE  /*applyAsManip*/,
                            FALSE /*transformIsInertiaEnd*/,
                            TRUE  /*isForMakeVisible*/,
                            &handled));

                        if (handled)
                        {
                            // Make sure the resulting minX/minY offsets are within bounds so the final viewport is correctly evaluated below.
                            DOUBLE scrollableDim = 0.0;

                            IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->get_ScrollableWidth(&scrollableDim));
                            minX = static_cast<FLOAT>(MIN(targetHorizontalOffset, scrollableDim));

                            IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->get_ScrollableHeight(&scrollableDim));
                            minY = static_cast<FLOAT>(MIN(targetVerticalOffset, scrollableDim));
                        }
                    }
#ifdef DM_DEBUG
                    else if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                            L"                   Skipping request during ongoing manipulation to (ho=%f, vo=%f, zf=%f).",
                            horizontalOffset, verticalOffset, zoomFactor));
                    }
#endif // DM_DEBUG
                }
                else
                {
                    bool isScrollRequested = false;
                    double currentOffset = 0.0;
                    double requestedOffset = 0.0;

                    // We fall back to calling SetHorizontalOffset/SetVerticalOffset when
                    // ScrollViewer::ChangeView is not called.
                    if (horizontalOffset != minX)
                    {
#ifdef DM_DEBUG
                        if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                        {
                            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                L"                   Calling SetHorizontalOffsetPrivate(%f).", minX));
                        }
#endif // DM_DEBUG
                        IFC_RETURN(SetHorizontalOffsetPrivate(static_cast<DOUBLE>(minX), &isScrollRequested, &currentOffset, &requestedOffset));

                        // Make sure the resulting minX offset is within bounds so the final viewport is correctly evaluated below.
                        if (isScrollRequested)
                        {
                            minX = static_cast<FLOAT>(requestedOffset);
                            handled = TRUE;
                        }
                        else
                        {
                            minX = static_cast<FLOAT>(currentOffset);
                        }
                    }

                    if (verticalOffset != minY)
                    {
#ifdef DM_DEBUG
                        if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                        {
                            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                L"                   Calling SetVerticalOffsetPrivate(%f).", minY));
                        }
#endif // DM_DEBUG
                        IFC_RETURN(SetVerticalOffsetPrivate(static_cast<DOUBLE>(minY), &isScrollRequested, &currentOffset, &requestedOffset));

                        // Make sure the resulting minY offset is within bounds so the final viewport is correctly evaluated below.
                        if (isScrollRequested)
                        {
                            minY = static_cast<FLOAT>(requestedOffset);
                            handled = TRUE;
                        }
                        else
                        {
                            minY = static_cast<FLOAT>(currentOffset);
                        }
                    }
                }
            }

            if (handled)
            {
                // Compute the visible rectangle of the child relative to the viewport.
                viewport.X = minX;
                viewport.Y = minY;

                // Do not include the applied offset so that potential parent bring-into-view contributors ignore that shift.
                viewport.X += static_cast<FLOAT>(appliedOffsetXTmp);
                viewport.Y += static_cast<FLOAT>(appliedOffsetYTmp);

                IFC_RETURN(RectUtil::Intersect(rectangle, viewport));

                IFC_RETURN(RectUtil::GetIsEmpty(rectangle, &isEmpty));
                if (!isEmpty)
                {
                    rectangle.X = rectangle.X - viewport.X + sizeHeaders.width;
                    rectangle.Y = rectangle.Y - viewport.Y + sizeHeaders.height;
                }
            }
            else
            {
                rectangle = unhandledRect;
            }
        }
    }

    if (appliedOffsetX)
    {
        *appliedOffsetX = appliedOffsetXTmp;
    }

    if (appliedOffsetY)
    {
        *appliedOffsetY = appliedOffsetYTmp;
    }

    // Return the rectangle
    *resultRectangle = rectangle;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        if (resultRectangle)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                L"                   Return resultRectangle=(%f, %f, %f, %f).", resultRectangle->X, resultRectangle->Y, resultRectangle->Width, resultRectangle->Height));
        }
        if (appliedOffsetX || appliedOffsetY)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                L"                   Return appliedOffset=(%f, %f).", appliedOffsetX ? *appliedOffsetX : 0.0f, appliedOffsetY ? *appliedOffsetY : 0.0f));
        }
    }
#endif // DM_DEBUG
    return S_OK;
}

// copied from DependencyObject::IsAncestorOf and modified
// to capture and return the most ancestor Page control of
// the pElement passed in if one exists.
_Check_return_ HRESULT ScrollContentPresenter::IsAncestorOfAndMostAncestorPageBetween(
    _In_ DependencyObject* pElement,
    _Out_ BOOLEAN* pIsAncestor,
    _Outptr_result_maybenull_ Page** ppMostAncestorPageBetween)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spAncestor = this;
    ctl::ComPtr<IDependencyObject> spCurrent = pElement;
    ctl::ComPtr<IDependencyObject> spNext = nullptr;
    ctl::ComPtr<Page> spMostAncestorPageBetween;

    IFCPTR(pIsAncestor);

    while (spCurrent && spCurrent != spAncestor)
    {
        ctl::ComPtr<IPage> spPageBetween;

        IFC(VisualTreeHelper::GetParentStatic(spCurrent.Get(), &spNext));

        if (SUCCEEDED(spNext.As(&spPageBetween)))
        {
            spMostAncestorPageBetween = spPageBetween.Cast<Page>();
        }

        spCurrent = spNext;
    }

    *pIsAncestor = (spAncestor == spCurrent);
    *ppMostAncestorPageBetween = spMostAncestorPageBetween.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollContentPresenter::GetFullScreenPageBottomAppBarHeight(
    _In_ Page* page,
    _Out_ DOUBLE* bottomAppBarHeight)
{
    // tolerance copied from pageApplyingLayoutBoundsTolerance in Page_Partial.cpp
    static float pageApplyingLayoutBoundsTolerance = 0.1f;
    IFCEXPECT_RETURN(page);

    *bottomAppBarHeight = 0.0;

    Window* currentWindow = nullptr;
    IFC_RETURN(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(page, &currentWindow));
    if (nullptr == currentWindow)
    {
        return S_FALSE;
    }

    wf::Rect currentWindowBounds;
    IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &currentWindowBounds));

    DOUBLE width;
    DOUBLE height;
    IFC_RETURN(page->get_ActualWidth(&width));
    IFC_RETURN(page->get_ActualHeight(&height));

    // is fullscreen window?
    if (DoubleUtil::Abs(currentWindowBounds.Width - static_cast<float>(width)) < pageApplyingLayoutBoundsTolerance &&
        DoubleUtil::Abs(currentWindowBounds.Height - static_cast<float>(height)) < pageApplyingLayoutBoundsTolerance)
    {
        ctl::ComPtr<IAppBar> spBottomAppBar;
        IFC_RETURN(page->get_BottomAppBar(&spBottomAppBar));

        // has bottom appbar?
        if (spBottomAppBar)
        {
            IFC_RETURN(spBottomAppBar.Cast<AppBar>()->get_ActualHeight(bottomAppBarHeight));
        }
    }

    return S_OK;
}

// Provides the behavior for the Measure pass of layout.  Classes can override
// this method to define their own Measure pass behavior.
IFACEMETHODIMP ScrollContentPresenter::MeasureOverride(
    // Measurement constraints, a control cannot return a size larger than the
    // constraint.
    _In_ wf::Size availableSize,
    // The desired size of the control.
    _Out_ wf::Size* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spChild;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    ctl::ComPtr<IScrollViewer> spScrollViewer;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<xaml::IFrameworkElement> spContentAsFE;
    ctl::ComPtr<xaml::IFrameworkElement> spChildAsFE;
    ctl::ComPtr<xaml::IUIElement> spTopLeftHeader;
    ctl::ComPtr<xaml::IUIElement> spTopHeader;
    ctl::ComPtr<xaml::IUIElement> spLeftHeader;
    ScrollData* pScrollData = NULL;
    wf::Size topLeftHeaderDesiredSize = {};
    wf::Size topHeaderDesiredSize = {};
    wf::Size leftHeaderDesiredSize = {};
    wf::Size headersDesiredSize = {};
    wf::Size desiredSize = {};
    wf::Size desiredSizeZoomed = {};
    bool adjustDesiredSize = false;
    bool canUseActualSizeAsExtent = false;
    wf::Size toBeAdjustedDesiredSize = { 0, 0 };
    BOOLEAN isScrollClient = FALSE;
    FLOAT zoomFactor = 1.0f;
    wf::Size layoutSize = { 0, 0 };
    xaml::HorizontalAlignment horizontalContentAlignment = xaml::HorizontalAlignment_Center;
    xaml::VerticalAlignment verticalContentAlignment = xaml::VerticalAlignment_Center;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:MeasureOverride availableSize=%f,%f.",
            this, availableSize.Width, availableSize.Height));
    }
#endif // DM_DEBUG

    *pReturnValue = desiredSize; // Set return value to (0, 0)

    IFC(get_TopLeftHeader(&spTopLeftHeader));
    IFC(get_TopHeader(&spTopHeader));
    IFC(get_LeftHeader(&spLeftHeader));

    ASSERT(!m_isTopLeftHeaderChild || spTopLeftHeader.Get());
    ASSERT(!m_isTopHeaderChild || spTopHeader.Get());
    ASSERT(!m_isLeftHeaderChild || spLeftHeader.Get());

    IFC(GetPrimaryChild(&spChild));

    // If there is no child but scroll data exists, it should be updated with an extent of 0.
    IFC(get_ScrollData(&pScrollData));
    IFC(IsScrollClient(&isScrollClient));

    if (!isScrollClient)
    {
        if (spTopLeftHeader || spTopHeader || spLeftHeader)
        {
            // Custom IScrollInfo implementations are not supported when a header is set.
            IFC(ErrorHelper::OriginateErrorUsingResourceID(E_NOT_SUPPORTED, ERROR_SCROLLVIEWER_UNSUPPORTED_ISCROLLINFO_WITH_HEADER));
        }

        if (spChild)
        {
            IFC(ScrollContentPresenterGenerated::MeasureOverride(availableSize, &desiredSize));
        }
    }
    else
    {
        if (spChild)
        {
            spChildAsFE = spChild.AsOrNull<xaml::IFrameworkElement>();
        }

        if (spChildAsFE && (spTopLeftHeader || spTopHeader || spLeftHeader))
        {
            // Check if the ScrollContentPresenter's content is a FrameworkElement or
            // if the default ControlTemplate with Grid/TextBlock is used instead.
            IFC(get_Content(&spContent));
            spContentAsFE = spContent.AsOrNull<xaml::IFrameworkElement>();
            if (spContentAsFE)
            {
                IFC(spChildAsFE->get_HorizontalAlignment(&horizontalContentAlignment));
                if (horizontalContentAlignment != xaml::HorizontalAlignment_Left)
                {
                    // Only the Left horizontal alignment is supported when a header is set.
                    IFC(ErrorHelper::OriginateErrorUsingResourceID(E_NOT_SUPPORTED, ERROR_SCROLLVIEWER_UNSUPPORTED_HORIZONTALALIGNMENT_WITH_HEADER));
                }

                IFC(spChildAsFE->get_VerticalAlignment(&verticalContentAlignment));
                if (verticalContentAlignment != xaml::VerticalAlignment_Top)
                {
                    // Only the Top vertical alignment is supported when a header is set.
                    IFC(ErrorHelper::OriginateErrorUsingResourceID(E_NOT_SUPPORTED, ERROR_SCROLLVIEWER_UNSUPPORTED_VERTICALALIGNMENT_WITH_HEADER));
                }
            }
            // else: no alignment check is done when the default Grid/TextBlock ControlTemplate is used.
        }

        IFC(pScrollData->get_ScrollOwner(&spScrollOwner));
        if (spScrollOwner)
        {
            IFC(spScrollOwner.As(&spScrollViewer));
        }

        if (spScrollViewer)
        {
            if (spLeftHeader && !m_isLeftHeaderChild)
            {
                // Add the left header as a child.
                IFC(AddHeader(spScrollViewer, spTopLeftHeader, spTopHeader, spLeftHeader, FALSE /*isTopHeader*/, TRUE /*isLeftHeader*/));
            }

            if (spTopHeader && !m_isTopHeaderChild)
            {
                // Add the top header as a child.
                IFC(AddHeader(spScrollViewer, spTopLeftHeader, spTopHeader, spLeftHeader, TRUE /*isTopHeader*/, FALSE /*isLeftHeader*/));
            }

            if (spTopLeftHeader && !m_isTopLeftHeaderChild)
            {
                // Add the top-left header as a child.
                IFC(AddHeader(spScrollViewer, spTopLeftHeader, spTopHeader, spLeftHeader, TRUE /*isTopHeader*/, TRUE /*isLeftHeader*/));
            }
        }

        wf::Size childAvailableSize = availableSize;
        // when set to true, this means that we wanted to set to infinity but were blocked in doing it.
        bool childPreventsInfiniteAvailableWidth = false;
        bool childPreventsInfiniteAvailableHeight = false;

        BOOLEAN sizesContentToTemplatedParent = FALSE;

        if (spScrollViewer)
        {
            // When ScrollContentPresenter.SizesContentToTemplatedParent is True, the child's available size
            // is set to the templated parent's (typically the ScrollViewer's) available size.
            IFC(get_SizesContentToTemplatedParent(&sizesContentToTemplatedParent));
            if (sizesContentToTemplatedParent)
            {
                // Note: Accessing the templated parent with get_TemplatedParent and using LayoutInformation::GetAvailableSize
                // would not work because it returns an out-of-date available size. So the owning ScrollViewer is asked directly
                // what its latest available size was.
                childAvailableSize = spScrollViewer.Cast<ScrollViewer>()->GetLatestAvailableSize();
            }
        }

        if (pScrollData->m_canHorizontallyScroll)
        {
            childPreventsInfiniteAvailableWidth = spChildAsFE && !spChildAsFE.Cast<FrameworkElement>()->WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(xaml_controls::Orientation::Orientation_Horizontal);

            // An infinite available width is given to the child unless:
            // - this ScrollContentPresenter belongs to a SemanticZoom control.
            // - the child FrameworkElement blocks an infinite available width - this is the case for a ModernCollectionBasePanel that is virtualizing vertically.
            // - this ScrollContentPresenter's SizesContentToTemplatedParent property is set to True.
            if (!m_isSemanticZoomPresenter && !childPreventsInfiniteAvailableWidth && !sizesContentToTemplatedParent)
            {
                childAvailableSize.Width = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
            }
        }
        else if (spChildAsFE)
        {
            xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
            xaml::FlowDirection childFlowDirection = xaml::FlowDirection_LeftToRight;
            IFC(get_FlowDirection(&flowDirection));
            IFC(spChildAsFE->get_FlowDirection(&childFlowDirection));
            if (flowDirection != childFlowDirection && !sizesContentToTemplatedParent)
            {
                childAvailableSize.Width = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
            }
        }

        if (pScrollData->m_canVerticallyScroll)
        {
            childPreventsInfiniteAvailableHeight = spChildAsFE && !spChildAsFE.Cast<FrameworkElement>()->WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(xaml_controls::Orientation::Orientation_Vertical);

            // An infinite available height is given to the child unless:
            // - this ScrollContentPresenter belongs to a SemanticZoom control.
            // - the child FrameworkElement blocks an infinite available height - this is the case for a ModernCollectionBasePanel that is virtualizing horizontally.
            // - this ScrollContentPresenter's SizesContentToTemplatedParent property is set to True.
            if (!m_isSemanticZoomPresenter && !childPreventsInfiniteAvailableHeight && !sizesContentToTemplatedParent)
            {
                childAvailableSize.Height = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
            }
        }

        bool headersAreNonClipping = false;

        // We wanted to set to infinity, but we didn't. Certain panels can deal with non clipping subtrees.
        ctl::ComPtr<IItemsPresenter> spIP = spChildAsFE.AsOrNull<IItemsPresenter>();
        if (spIP)
        {
            headersAreNonClipping = static_cast<ItemsPresenter*>(spIP.Get())->EvaluateAndSetNonClippingBehavior(childPreventsInfiniteAvailableWidth || childPreventsInfiniteAvailableHeight);
        }

        if (spScrollOwner)
        {
            IFC(spScrollOwner->get_ZoomFactorImpl(&zoomFactor));
            ASSERT(zoomFactor == m_fZoomFactor);
        }

        if (spTopLeftHeader || spTopHeader || spLeftHeader)
        {
            // When at least one header element is shown, the plateau scale returned by RootScale is combined with the owning ScrollViewer's ZoomFactor
            // so the CUIElement::LayoutRound method can correctly snap the four quadrants based on both factors. put_GlobalScaleFactor is pushing the global
            // scale factor into sparse storage for whichever of the four quadrants exist.
            const float globalScaleFactor = RootScale::GetRasterizationScaleForElement(GetHandle()) * zoomFactor;

            if (spTopLeftHeader)
            {
                IFC(spTopLeftHeader.Cast<UIElement>()->put_GlobalScaleFactor(globalScaleFactor));
                IFC(spTopLeftHeader->Measure(childAvailableSize));
                IFC(spTopLeftHeader->get_DesiredSize(&topLeftHeaderDesiredSize));
            }
            if (spTopHeader)
            {
                IFC(spTopHeader.Cast<UIElement>()->put_GlobalScaleFactor(globalScaleFactor));
                static_cast<CUIElement*>(spTopHeader.Cast<UIElement>()->GetHandle())->SetIsNonClippingSubtree(headersAreNonClipping);
                IFC(spTopHeader->Measure(childAvailableSize));
                IFC(spTopHeader->get_DesiredSize(&topHeaderDesiredSize));
            }
            if (spLeftHeader)
            {
                IFC(spLeftHeader.Cast<UIElement>()->put_GlobalScaleFactor(globalScaleFactor));
                IFC(spLeftHeader->Measure(childAvailableSize));
                IFC(spLeftHeader->get_DesiredSize(&leftHeaderDesiredSize));
            }
            if (spChild)
            {
                IFC(spChild.Cast<UIElement>()->put_GlobalScaleFactor(globalScaleFactor));
            }
        }

        headersDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Max(topLeftHeaderDesiredSize.Width, leftHeaderDesiredSize.Width));
        headersDesiredSize.Height = static_cast<FLOAT>(DoubleUtil::Max(topLeftHeaderDesiredSize.Height, topHeaderDesiredSize.Height));

        if (spChild)
        {
            ctl::ComPtr<ICalendarPanel> spChildAsCalendarPanel;
            IFC(spChild->Measure(childAvailableSize));

            IFC(spChild->get_DesiredSize(&desiredSize));

            spChildAsCalendarPanel = spChild.AsOrNull<ICalendarPanel>();
            if (spChildAsCalendarPanel)
            {
                wf::Size desiredViewportSizeFromPanel = { 0, 0 };
                adjustDesiredSize = true;
                // In CalendarPanel, the SCP's desired size should be determined by the Panel so Panel can decide
                // the numbers (rows and cols) of items showing in the viewport.
                // Note: the SCP's scrollcontent extent is still determined by Panel's desired size.
                // see more details from CalendarView::MeasureOverride in file CalendarView_Partial.cpp
                spChildAsCalendarPanel.Cast<CalendarPanel>()->GetDesiredViewportSize(&desiredViewportSizeFromPanel);
                toBeAdjustedDesiredSize.Width = desiredViewportSizeFromPanel.Width - desiredSize.Width;
                toBeAdjustedDesiredSize.Height = desiredViewportSizeFromPanel.Height - desiredSize.Height;
            }
        }

        desiredSize.Width += headersDesiredSize.Width;
        desiredSize.Height += headersDesiredSize.Height;

        if (pScrollData != nullptr)
        {
            if (spChild == nullptr)
            {
                // Irrespective of the presence of headers, the desired size is (0, 0) when ScrollViewer.Content is null.
                ASSERT(desiredSizeZoomed.Width == 0.0f);
                ASSERT(desiredSizeZoomed.Height == 0.0f);
                if (m_isChildActualWidthUsedAsExtent)
                {
                    // No need to use the actual child width as the extent width.
                    StopUseOfActualWidthAsExtent();
                }
                if (m_isChildActualHeightUsedAsExtent)
                {
                    // No need to use the actual child height as the extent height.
                    StopUseOfActualHeightAsExtent();
                }
                IFC(VerifyScrollData(pScrollData->m_viewport /*viewport*/, desiredSizeZoomed /*extent*/));
            }
            else
            {
                if (spScrollViewer)
                {
                    layoutSize = spScrollViewer.Cast<ScrollViewer>()->GetLayoutSize();
                }
                // blow over the reported size to use the passed in size. This will increase the extent.
                if (layoutSize.Width != 0.0f && layoutSize.Height != 0.0f)
                {
                    // This situation only arises with the SemanticZoom control which applies a pseudo-LayoutTransform to the ScrollViewer.Content element.
                    // The SemanticZoom provides a layout size to the ScrollViewer to be imposed to its ScrollContentPresenter.
                    desiredSizeZoomed.Width = static_cast<FLOAT>(layoutSize.Width + headersDesiredSize.Width) * zoomFactor;
                    desiredSizeZoomed.Height = static_cast<FLOAT>(layoutSize.Height + headersDesiredSize.Height) * zoomFactor;

                    if (m_isChildActualWidthUsedAsExtent)
                    {
                        // In case the actual size was being used as the extent, use the imposed layoutSize instead.
                        StopUseOfActualWidthAsExtent();
                    }
                    if (m_isChildActualHeightUsedAsExtent)
                    {
                        // In case the actual size was being used as the extent, use the imposed layoutSize instead.
                        StopUseOfActualHeightAsExtent();
                    }
                }
                else
                {
                    desiredSizeZoomed.Width = desiredSize.Width * zoomFactor;
                    desiredSizeZoomed.Height = desiredSize.Height * zoomFactor;

                    bool setExtent = false;
                    wf::Size extentSize = pScrollData->m_extent;

                    if (m_isChildActualWidthUsedAsExtent)
                    {
                        IFC(CanUseActualWidthAsExtent(spScrollOwner.Get(), spScrollViewer.Get(), spChildAsFE.Get(), &canUseActualSizeAsExtent));
                        if (!canUseActualSizeAsExtent)
                        {
#ifdef DM_DEBUG
                            if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                    L"DMSCP[0x%p]: MeasureOverride - Stopping use of actual width as extent.", this));
                            }
#endif // DM_DEBUG
                            StopUseOfActualWidthAsExtent();
                        }
                        else if (pScrollData->m_canHorizontallyScroll && desiredSize.Width >= pScrollData->m_viewport.Width)
                        {
                            // After switching to the mode where the child's actual width is used as extent, push an extent that is larger
                            // than the viewport as early as possible to the owning ScrollViewer. This is important for ModernCollectionBasePanel
                            // which may trigger a call to ScrollViewer::ChangeViewInternal inside its ArrangeOverride.
                            setExtent = true;

                            if (sizesContentToTemplatedParent && desiredSize.Width == pScrollData->m_viewport.Width)
                            {
                                desiredSizeZoomed.Width = extentSize.Width;
                            }
                            else
                            {
                                extentSize.Width = desiredSizeZoomed.Width;
                            }
                        }
                    }

                    if (m_isChildActualHeightUsedAsExtent)
                    {
                        IFC(CanUseActualHeightAsExtent(spScrollOwner.Get(), spScrollViewer.Get(), spChildAsFE.Get(), &canUseActualSizeAsExtent));
                        if (!canUseActualSizeAsExtent)
                        {
#ifdef DM_DEBUG
                            if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                    L"DMSCP[0x%p]: MeasureOverride - Stopping use of actual height as extent.", this));
                            }
#endif // DM_DEBUG
                            StopUseOfActualHeightAsExtent();
                        }
                        else if (pScrollData->m_canVerticallyScroll && desiredSize.Height >= pScrollData->m_viewport.Height)
                        {
                            // After switching to the mode where the child's actual height is used as extent, push an extent that is larger
                            // than the viewport as early as possible to the owning ScrollViewer. This is important for ModernCollectionBasePanel
                            // which may trigger a call to ScrollViewer::ChangeViewInternal inside its ArrangeOverride.
                            setExtent = true;

                            if (sizesContentToTemplatedParent && desiredSize.Height == pScrollData->m_viewport.Height)
                            {
                                desiredSizeZoomed.Height = extentSize.Height;
                            }
                            else
                            {
                                extentSize.Height = desiredSizeZoomed.Height;
                            }
                        }
                    }

                    if (!m_isChildActualWidthUsedAsExtent && m_isChildActualHeightUsedAsExtent)
                    {
                        // Only the actual height is used as extent. Make sure the latest desired width is used as extent.
                        setExtent = true;
                        extentSize.Width = desiredSizeZoomed.Width;
                        // The extent height pushed to the ScrollViewer in the VerifyScrollData call below is not up-to-date.
                        // The updated height will be pushed to the ScrollViewer in the VerifyScrollData call made in the coming ArrangeOverride.
                        // The m_isChildActualHeightUpdated flag is temporarily set to False during the VerifyScrollData call below so the
                        // ScrollViewer does not prematurely reset its m_contentHeightRequested field in its InvalidateScrollInfo implementation.
                        m_isChildActualHeightUpdated = false;
                    }
                    else if (m_isChildActualWidthUsedAsExtent && !m_isChildActualHeightUsedAsExtent)
                    {
                        // Only the actual width is used as extent. Make sure the latest desired height is used as extent.
                        setExtent = true;
                        extentSize.Height = desiredSizeZoomed.Height;
                        // The extent width pushed to the ScrollViewer in the VerifyScrollData call below is not up-to-date.
                        // The updated width will be pushed to the ScrollViewer in the VerifyScrollData call made in the coming ArrangeOverride.
                        // The m_isChildActualWidthUpdated flag is temporarily set to False during the VerifyScrollData call below so the
                        // ScrollViewer does not prematurely reset its m_contentWidthRequested field in its InvalidateScrollInfo implementation.
                        m_isChildActualWidthUpdated = false;
                    }

                    if (setExtent)
                    {
                        IFC(VerifyScrollData(pScrollData->m_viewport, extentSize));
                    }
                }

                // Do not attempt to update the IScrollInfo extent when this ScrollContentPresenter
                // operates in the mode where the child's actual size is used as extent.
                if (m_isChildActualWidthUsedAsExtent)
                {
                    m_unpublishedExtentSize.Width = desiredSizeZoomed.Width;
                }
                if (m_isChildActualHeightUsedAsExtent)
                {
                    m_unpublishedExtentSize.Height = desiredSizeZoomed.Height;
                }

                if (!m_isChildActualWidthUsedAsExtent && !m_isChildActualHeightUsedAsExtent)
                {
                    // If we're handling scrolling (as the physical scrolling client, validate properties).
                    IFC(VerifyScrollData(pScrollData->m_viewport /*viewport*/, desiredSizeZoomed /*extent*/));
                }
            }
        }
    }

    if (adjustDesiredSize)
    {
        // When we need to adjust desired size, we ignore the available size.
        desiredSize.Width = desiredSize.Width + toBeAdjustedDesiredSize.Width;
        desiredSize.Height = desiredSize.Height + toBeAdjustedDesiredSize.Height;
    }
    else if (layoutSize.Width != 0.0f && layoutSize.Height != 0.0f)
    {
        // SemanticZoom's ScrollViewer case. Use the enforced layoutSize rather than the child's desiredSize.
        // This matches how desiredSizeZoomed is evaluated for the VerifyScrollData call above.
        desiredSize.Width = static_cast<FLOAT>(DoubleUtil::Min(availableSize.Width, layoutSize.Width));
        desiredSize.Height = static_cast<FLOAT>(DoubleUtil::Min(availableSize.Height, layoutSize.Height));
    }
    else
    {
        desiredSize.Width = static_cast<FLOAT>(DoubleUtil::Min(availableSize.Width, desiredSize.Width));
        desiredSize.Height = static_cast<FLOAT>(DoubleUtil::Min(availableSize.Height, desiredSize.Height));
    }

    *pReturnValue = desiredSize;

Cleanup:
#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/,
            L"DMSCPv[0x%p]:MeasureOverride desiredSizeZoomed=%f,%f, desiredSize=%f,%f.", this, desiredSizeZoomed.Width, desiredSizeZoomed.Height, pReturnValue->Width, pReturnValue->Height));
        if (spScrollViewer != nullptr)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/,
                L"                   IsRootSV=%d.", spScrollViewer.Cast<ScrollViewer>()->IsRootScrollViewerDbg()));
        }
    }
#endif // DM_DEBUG

    m_isChildActualWidthUpdated = true;
    m_isChildActualHeightUpdated = true;

    // Let ScrollViewer know that child sizes might have changed
    if (spScrollViewer)
    {
        const HRESULT localHr = spScrollViewer.Cast<ScrollViewer>()->OnScrollContentPresenterMeasured();
        if (FAILED(localHr))
        {
            hr = localHr;
        }
    }

    RRETURN(hr);
}

// Provides the behavior for the Arrange pass of layout.  Classes can override
// this method to define their own Arrange pass behavior.
IFACEMETHODIMP ScrollContentPresenter::ArrangeOverride(
    // The computed size that is used to arrange the content.
    _In_ wf::Size finalSize,
    // The size of the control.
    _Out_ wf::Size* pReturnValue)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:ArrangeOverride finalSize=%f,%f.",
            this, finalSize.Width, finalSize.Height));
    }
#endif // DM_DEBUG

    do
    {
        ctl::ComPtr<DependencyObject> spTemplatedParent;
        ctl::ComPtr<IUIElement> spChild;
        ctl::ComPtr<IUIElement> spTopLeftHeader;
        ctl::ComPtr<IUIElement> spTopHeader;
        ctl::ComPtr<IUIElement> spLeftHeader;
        ctl::ComPtr<IScrollOwner> spScrollOwner;
        ctl::ComPtr<IScrollViewer> spScrollViewer;
        ctl::ComPtr<IDependencyObject> spChildAsDO;
        ctl::ComPtr<IFrameworkElement> spChildAsFE;
        ScrollData* pScrollData = NULL;
        wf::Rect childRect = {};
        wf::Size desiredSize = {};
        wf::Size topLeftHeaderDesiredSize = {};
        wf::Size topHeaderDesiredSize = {};
        wf::Size leftHeaderDesiredSize = {};
        wf::Size extentSize = {};
        xaml::Thickness margins;
        BOOLEAN isScrollClient = FALSE;
        BOOLEAN isHeaderArranged = FALSE;
        DOUBLE actualWidth = 0.0;
        DOUBLE actualHeight = 0.0;
        FLOAT offsetX = 0.0f;
        FLOAT offsetY = 0.0f;
        FLOAT zoomFactor = 1.0f;
        FLOAT currentZoomFactor = 1.0f;
        bool canUseActualSizeAsExtent = false;

        // NOTE: We are updating the clip only if there is a scroll owner that hosts
        // this control. This is a limited fix for 22803.
        IFC(get_TemplatedParent(&spTemplatedParent));
        if (spTemplatedParent)
        {
            IFC(UpdateClip(finalSize));
        }

        IFC(get_TopLeftHeader(&spTopLeftHeader));
        IFC(get_TopHeader(&spTopHeader));
        IFC(get_LeftHeader(&spLeftHeader));

        ASSERT(!m_isTopLeftHeaderChild || spTopLeftHeader.Get());
        ASSERT(!m_isTopHeaderChild || spTopHeader.Get());
        ASSERT(!m_isLeftHeaderChild || spLeftHeader.Get());

        IFC(GetPrimaryChild(&spChild));
        spChildAsFE = spChild.AsOrNull<xaml::IFrameworkElement>();

        // Verifies IScrollInfo properties & invalidates ScrollViewer if necessary.
        m_scrollRequested = FALSE;

        IFC(get_ScrollOwner(&spScrollOwner));
        IFC(spScrollOwner.As(&spScrollViewer));
        IFC(get_ScrollData(&pScrollData));
        IFC(IsScrollClient(&isScrollClient));

#ifdef DM_DEBUG
        if ((DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK) && spScrollViewer != nullptr)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/,
                L"                   IsRootSV=%d.", spScrollViewer.Cast<ScrollViewer>()->IsRootScrollViewerDbg()));
        }
#endif // DM_DEBUG

        if (isScrollClient && pScrollData)
        {
            extentSize = pScrollData->m_extent;
            if (m_isChildActualWidthUsedAsExtent)
            {
                // Check if the mode where the child's actual width is used for the IScrollInfo extent must be left.
                if (m_unpublishedExtentSize.Width > 0 &&
                    m_unpublishedExtentSize.Width == finalSize.Width &&
                    extentSize.Width != m_unpublishedExtentSize.Width)
                {
                    // Use the unpublished desired width which ends up being the final arrangement width.
                    extentSize.Width = m_unpublishedExtentSize.Width;
#ifdef DM_DEBUG
                    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                            L"DMSCP[0x%p]: ArrangeOverride - Stopping use of actual width as extent.", this));
                    }
#endif // DM_DEBUG
                    StopUseOfActualWidthAsExtent();
                }
            }
            if (m_isChildActualHeightUsedAsExtent)
            {
                // Check if the mode where the child's actual height is used for the IScrollInfo extent must be left.
                if (m_unpublishedExtentSize.Height > 0 &&
                    m_unpublishedExtentSize.Height == finalSize.Height &&
                    extentSize.Height != m_unpublishedExtentSize.Height)
                {
                    // Use the unpublished desired height which ends up being the final arrangement height.
                    extentSize.Height = m_unpublishedExtentSize.Height;
#ifdef DM_DEBUG
                    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                            L"DMSCP[0x%p]: ArrangeOverride - Stopping use of actual height as extent.", this));
                    }
#endif // DM_DEBUG
                    StopUseOfActualHeightAsExtent();
                }
            }
            IFC(VerifyScrollData(finalSize /*viewport*/, extentSize /*extent*/));
        }

        if (m_isTopLeftHeaderChild)
        {
            IFC(get_TopLeftHeader(&spTopLeftHeader));
            if (spTopLeftHeader)
            {
                IFC(spTopLeftHeader->get_DesiredSize(&topLeftHeaderDesiredSize));
                isHeaderArranged = TRUE;
            }
        }

        if (m_isTopHeaderChild)
        {
            IFC(get_TopHeader(&spTopHeader));
            if (spTopHeader)
            {
                IFC(spTopHeader->get_DesiredSize(&topHeaderDesiredSize));
                isHeaderArranged = TRUE;
            }
        }

        if (m_isLeftHeaderChild)
        {
            IFC(get_LeftHeader(&spLeftHeader));
            if (spLeftHeader)
            {
                IFC(spLeftHeader->get_DesiredSize(&leftHeaderDesiredSize));
                isHeaderArranged = TRUE;
            }
        }

        if (spChild && isScrollClient)
        {
            if (spScrollOwner)
            {
                IFC(spScrollOwner->get_ZoomFactorImpl(&currentZoomFactor));
                ASSERT(currentZoomFactor == m_fZoomFactor);
            }

            if (spScrollViewer && spScrollViewer.Cast<ScrollViewer>()->IsInManipulation())
            {
                offsetX = -(spScrollViewer.Cast<ScrollViewer>()->GetPreDirectManipulationOffsetX());
                offsetY = -(spScrollViewer.Cast<ScrollViewer>()->GetPreDirectManipulationOffsetY());
                zoomFactor = spScrollViewer.Cast<ScrollViewer>()->GetPreDirectManipulationZoomFactor();
            }
            else
            {
                if (spScrollViewer && spScrollViewer.Cast<ScrollViewer>()->IsInDirectManipulationCompletion())
                {
                    IFC(spScrollViewer.Cast<ScrollViewer>()->PostDirectManipulationLayoutRefreshed());
                }
                offsetX = -static_cast<FLOAT>(pScrollData->m_ComputedOffset.X);
                offsetY = -static_cast<FLOAT>(pScrollData->m_ComputedOffset.Y);
                zoomFactor = currentZoomFactor;
            }
        }
        else if (isHeaderArranged && spScrollOwner)
        {
            IFC(spScrollOwner->get_ZoomFactorImpl(&zoomFactor));
            ASSERT(zoomFactor == m_fZoomFactor);
        }

        if (spTopLeftHeader)
        {
            childRect.X = childRect.Y = 0;
            childRect.Width = static_cast<FLOAT>(topLeftHeaderDesiredSize.Width);
            childRect.Height = static_cast<FLOAT>(topLeftHeaderDesiredSize.Height);
            IFC(spTopLeftHeader->Arrange(childRect));
        }

        if (spTopHeader)
        {
            childRect.X = static_cast<FLOAT>(DoubleUtil::Max(topLeftHeaderDesiredSize.Width, leftHeaderDesiredSize.Width));
            childRect.Y = 0;
            childRect.Width = static_cast<FLOAT>(topHeaderDesiredSize.Width);
            childRect.Height = static_cast<FLOAT>(topHeaderDesiredSize.Height);
            IFC(spTopHeader->Arrange(childRect));
        }

        if (spLeftHeader)
        {
            childRect.X = 0;
            childRect.Y = static_cast<FLOAT>(DoubleUtil::Max(topLeftHeaderDesiredSize.Height, topHeaderDesiredSize.Height));
            childRect.Width = static_cast<FLOAT>(leftHeaderDesiredSize.Width);
            childRect.Height = static_cast<FLOAT>(leftHeaderDesiredSize.Height);
            IFC(spLeftHeader->Arrange(childRect));
        }

        if (spChild)
        {
            IFC(spChild->get_DesiredSize(&desiredSize));

            childRect.X = static_cast<FLOAT>(DoubleUtil::Max(topLeftHeaderDesiredSize.Width, leftHeaderDesiredSize.Width));
            childRect.Y = static_cast<FLOAT>(DoubleUtil::Max(topLeftHeaderDesiredSize.Height, topHeaderDesiredSize.Height));

            // This is needed to stretch the child to arrange space.
            childRect.Width = static_cast<FLOAT>(DoubleUtil::Max(desiredSize.Width, finalSize.Width));
            childRect.Height = static_cast<FLOAT>(DoubleUtil::Max(desiredSize.Height, finalSize.Height));

            IFC(spChild->Arrange(childRect));

            if (isScrollClient && pScrollData)
            {
                if (spChild.Cast<UIElement>()->IsArrangeDirty())
                {
                    if (m_isChildActualWidthUsedAsExtent || m_isChildActualHeightUsedAsExtent)
                    {
                        // When operating in the mode where the child's actual width or height is used for the IScrollInfo extent
                        // and the child is still marked dirty for layout, make sure that this ScrollContentPresenter::ArrangeOverride
                        // is invoked again so that the correct content extent can be pushed to the owning ScrollViewer with a call to
                        // VerifyScrollData in the 'else' branch below once the child got arranged.
                        IFC(InvalidateArrange());
                    }
                }
                else
                {
                    extentSize = pScrollData->m_extent;

                    // Check if the mode where the child's actual size is used for the IScrollInfo extent must be entered.
                    // To minimize the occurrences of this mode, it is restricted to cases that use a Stretch alignment.
                    IFC(CanUseActualWidthAsExtent(spScrollOwner.Get(), spScrollViewer.Get(), spChildAsFE.Get(), &canUseActualSizeAsExtent));
                    // MeasureOverride is expected to leave the special mode when its call to CanUseActualWidthAsExtent returns False.
                    ASSERT(!(!canUseActualSizeAsExtent && m_isChildActualWidthUsedAsExtent));
                    if (canUseActualSizeAsExtent)
                    {
                        // Determine the child's actual width, including the margins which are included in the IScrollInfo extent.
                        ASSERT(spChildAsFE != nullptr);
                        IFC(spChildAsFE->get_ActualWidth(&actualWidth));
                        IFC(spChildAsFE->get_Margin(&margins));
                        actualWidth = DoubleUtil::Max(0.0, actualWidth + margins.Left + margins.Right);

                        BOOLEAN useLayoutRounding = FALSE;
                        IFC(get_UseLayoutRounding(&useLayoutRounding));
                        if (useLayoutRounding)
                        {
                            // Apply the same rounding on the content width as for the viewport width, i.e. finalSize, provided as a parameter.
                            // This is to avoid situations where the content width ends up being slightly larger than the viewport width and
                            // incorrectly causes the horizontal scrollbar to appear.
                            FLOAT actualWidthF = static_cast<FLOAT>(actualWidth);
                            IFC(spChild.Cast<UIElement>()->LayoutRound(actualWidthF, &actualWidthF));
                            actualWidth = actualWidthF;
                        }

                        // Limit the width to the viewport width when horizontal scrolling is disabled.
                        if (!pScrollData->m_canHorizontallyScroll && actualWidth > pScrollData->m_viewport.Width)
                        {
                            actualWidth = pScrollData->m_viewport.Width;
                        }

                        if (m_isChildActualWidthUsedAsExtent ||
                            (pScrollData->m_extent.Width > 0 && !DoubleUtil::AreWithinTolerance(actualWidth * currentZoomFactor, pScrollData->m_extent.Width, ScrollViewerScrollRoundingTolerance)))
                        {
                            bool actualWidthWithRoundedDownMarginsMatchesExtentWidth = false;
                            const float scale = RootScale::GetRasterizationScaleForElement(GetHandle());
                            const float roundingStep = 1.0f / scale;

                            if (!m_isChildActualWidthUsedAsExtent && useLayoutRounding && margins.Left + margins.Right >= roundingStep)
                            {
                                // c.f. RS5 bug 18604282. The desired width and computed actual width may differ by a single rounding step because the FrameworkElement.ActualWidth was rounded up
                                // while it was not in the desired size.
                                // Example at global scale factor of 1.5: FrameworkElement.ActualWidth is rounded up from 238 to 238.66 in CFrameworkElement::ArrangeCore. With a Margin.Left of 11px
                                // and a Margin.Right of 12px, the DesiredSize.Width is set to LayoutRound(238 + LayoutRound(23)) == 261.33 in CFrameworkElement::MeasureCore. Thus the check above
                                // "Is LayoutRound(238.66 + 23)==262 equal to 261.33?" fails.
                                // Verifying if that is the case below.
                                float actualWidthWithRoundedDownMargins;

                                IFC(spChild.Cast<UIElement>()->LayoutRound(static_cast<float>(actualWidth - roundingStep), &actualWidthWithRoundedDownMargins));
                                actualWidthWithRoundedDownMarginsMatchesExtentWidth =
                                    DoubleUtil::AreWithinTolerance(static_cast<double>(actualWidthWithRoundedDownMargins) * currentZoomFactor, pScrollData->m_extent.Width, ScrollViewerScrollRoundingTolerance);
                            }

                            if (!actualWidthWithRoundedDownMarginsMatchesExtentWidth)
                            {
                                // When m_isChildActualWidthUsedAsExtent==False, the extent previously set in MeasureOverride does not match the resulting width after Arrange.
                                // Override the extent based on the new actual width.
                                // When m_isChildActualWidthUsedAsExtent==True, this ScrollContentPresenter already uses the child's actual width as the extent. This remains
                                // the case until a new Content is set or CanUseActualWidthAsExtent returns False.
                                if (!m_isChildActualWidthUsedAsExtent)
                                {
#ifdef DM_DEBUG
                                    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                                    {
                                        IGNOREHR(spChildAsFE->get_ActualHeight(&actualHeight));
                                        actualHeight = DoubleUtil::Max(0.0, actualHeight + margins.Top + margins.Bottom);
                                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                            L"DMSCP[0x%p]: ArrangeOverride - Starting use of actual width as extent.", this));
                                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                            L"                   pScrollData->m_extent=(%f, %f), actualWidth=%f, actualHeight=%f, currentZoomFactor=%4.8lf.",
                                            pScrollData->m_extent.Width, pScrollData->m_extent.Height, actualWidth, actualHeight, currentZoomFactor));
                                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                            L"                   pScrollData->m_viewport=(%f, %f).",
                                            pScrollData->m_viewport.Width, pScrollData->m_viewport.Height));
                                    }
#endif // DM_DEBUG
                                    StartUseOfActualWidthAsExtent();
                                }

                                // Finally use the child's actual width for the IScrollInfo extent.
                                extentSize.Width = static_cast<FLOAT>(actualWidth) * currentZoomFactor;
                            }
                        }
                    }

                    // Check if the mode where the child's actual height is used for the IScrollInfo extent must be entered.
                    // To minimize the occurrences of this mode, it is restricted to cases that use a Stretch alignment.
                    IFC(CanUseActualHeightAsExtent(spScrollOwner.Get(), spScrollViewer.Get(), spChildAsFE.Get(), &canUseActualSizeAsExtent));
                    // MeasureOverride is expected to leave the special mode when its call to CanUseActualHeightAsExtent returns False.
                    ASSERT(!(!canUseActualSizeAsExtent && m_isChildActualHeightUsedAsExtent));
                    if (canUseActualSizeAsExtent)
                    {
                        // Determine the child's actual height, including the margins which are included in the IScrollInfo extent.
                        ASSERT(spChildAsFE != nullptr);
                        IFC(spChildAsFE->get_ActualHeight(&actualHeight));
                        IFC(spChildAsFE->get_Margin(&margins));
                        actualHeight = DoubleUtil::Max(0.0, actualHeight + margins.Top + margins.Bottom);

                        BOOLEAN useLayoutRounding = FALSE;
                        IFC(get_UseLayoutRounding(&useLayoutRounding));
                        if (useLayoutRounding)
                        {
                            // Apply the same rounding on the content height as for the viewport height, i.e. finalSize, provided as a parameter.
                            // This is to avoid situations where the content height ends up being slightly larger than the viewport height and
                            // incorrectly causes the vertical scrollbar to appear.
                            FLOAT actualHeightF = static_cast<FLOAT>(actualHeight);
                            IFC(spChild.Cast<UIElement>()->LayoutRound(actualHeightF, &actualHeightF));
                            actualHeight = actualHeightF;
                        }

                        // Limit the height to the viewport height when vertical scrolling is disabled.
                        if (!pScrollData->m_canVerticallyScroll && actualHeight > pScrollData->m_viewport.Height)
                        {
                            actualHeight = pScrollData->m_viewport.Height;
                        }

                        if (m_isChildActualHeightUsedAsExtent ||
                            (pScrollData->m_extent.Height > 0 && !DoubleUtil::AreWithinTolerance(actualHeight * currentZoomFactor, pScrollData->m_extent.Height, ScrollViewerScrollRoundingTolerance)))
                        {
                            bool actualHeightWithRoundedDownMarginsMatchesExtentHeight = false;
                            const float scale = RootScale::GetRasterizationScaleForElement(GetHandle());
                            const float roundingStep = 1.0f / scale;

                            if (!m_isChildActualHeightUsedAsExtent && useLayoutRounding && margins.Top + margins.Bottom >= roundingStep)
                            {
                                // c.f. RS5 bug 18604282. The desired height and computed actual height may differ by a single rounding step because the FrameworkElement.ActualHeight was rounded up
                                // while it was not in the desired size.
                                // Example at global scale factor of 1.5: FrameworkElement.ActualHeight is rounded up from 238 to 238.66 in CFrameworkElement::ArrangeCore. With a Margin.Top of 11px
                                // and a Margin.Bottom of 12px, the DesiredSize.Height is set to LayoutRound(238 + LayoutRound(23)) == 261.33 in CFrameworkElement::MeasureCore. Thus the check above
                                // "Is LayoutRound(238.66 + 23)==262 equal to 261.33?" fails.
                                // Verifying if that is the case below.
                                float actualHeightWithRoundedDownMargins;

                                IFC(spChild.Cast<UIElement>()->LayoutRound(static_cast<float>(actualHeight - roundingStep), &actualHeightWithRoundedDownMargins));
                                actualHeightWithRoundedDownMarginsMatchesExtentHeight =
                                    DoubleUtil::AreWithinTolerance(static_cast<double>(actualHeightWithRoundedDownMargins) * currentZoomFactor, pScrollData->m_extent.Height, ScrollViewerScrollRoundingTolerance);
                            }

                            if (!actualHeightWithRoundedDownMarginsMatchesExtentHeight)
                            {
                                // When m_isChildActualHeightUsedAsExtent==False, the extent previously set in MeasureOverride does not match the resulting height after Arrange.
                                // Override the extent based on the new actual height.
                                // When m_isChildActualHeightUsedAsExtent==True, this ScrollContentPresenter already uses the child's actual height as the extent. This remains
                                // the case until a new Content is set or CanUseActualHeightAsExtent returns False.
                                if (!m_isChildActualHeightUsedAsExtent)
                                {
#ifdef DM_DEBUG
                                    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
                                    {
                                        IGNOREHR(spChildAsFE->get_ActualWidth(&actualWidth));
                                        actualWidth = DoubleUtil::Max(0.0, actualWidth + margins.Left + margins.Right);
                                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                            L"DMSCP[0x%p]: ArrangeOverride - Starting use of actual height as extent.", this));
                                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                            L"                   pScrollData->m_extent=(%f, %f), actualWidth=%f, actualHeight=%f, currentZoomFactor=%4.8lf.",
                                            pScrollData->m_extent.Width, pScrollData->m_extent.Height, actualWidth, actualHeight, currentZoomFactor));
                                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                                            L"                   pScrollData->m_viewport=(%f, %f).",
                                            pScrollData->m_viewport.Width, pScrollData->m_viewport.Height));
                                    }
#endif // DM_DEBUG
                                    StartUseOfActualHeightAsExtent();
                                }

                                // Finally use the child's actual height for the IScrollInfo extent.
                                extentSize.Height = static_cast<FLOAT>(actualHeight) * currentZoomFactor;
                            }
                        }
                    }
                    if (m_isChildActualWidthUsedAsExtent || m_isChildActualHeightUsedAsExtent)
                    {
                        IFC(VerifyScrollData(pScrollData->m_viewport, extentSize));
                    }
                }
            }
        }
    }
    while (m_scrollRequested);

    *pReturnValue = finalSize;

Cleanup:
    RRETURN(hr);
}

// Determine how down we need to scroll to accommodate the desired view.
_Check_return_ HRESULT ScrollContentPresenter::ComputeScrollOffsetWithMinimalScroll(
    _In_ FLOAT topView,
    _In_ FLOAT bottomView,
    _In_ FLOAT topChild,
    _In_ FLOAT bottomChild,
    _Out_ FLOAT* pOffset)
{
    BOOLEAN above = FALSE;
    BOOLEAN below = FALSE;
    BOOLEAN larger = FALSE;

    above = DoubleUtil::LessThan(topChild, topView) && DoubleUtil::LessThan(bottomChild, bottomView);
    below = DoubleUtil::GreaterThan(bottomChild, bottomView) && DoubleUtil::GreaterThan(topChild, topView);
    larger = (bottomChild - topChild) > (bottomView - topView);

    // # CHILD POSITION       CHILD SIZE      SCROLL      REMEDY
    // 1 Above viewport       <= viewport     Down        Align top edge of child & viewport
    // 2 Above viewport       > viewport      Down        Align bottom edge of child & viewport
    // 3 Below viewport       <= viewport     Up          Align bottom edge of child & viewport
    // 4 Below viewport       > viewport      Up          Align top edge of child & viewport
    // 5 Entirely within viewport             NA          No scroll.
    // 6 Spanning viewport                    NA          No scroll.
    //
    // Note: "Above viewport" = childTop above viewportTop, childBottom above viewportBottom
    //       "Below viewport" = childTop below viewportTop, childBottom below viewportBottom
    // This child thus may overlap with the viewport, but will scroll the same direction
    if ((above && !larger) || (below && larger))
    {
        // Handle Cases:  1 & 4 above
        *pOffset = topChild;
    }
    else if (above || below)
    {
        // Handle Cases: 2 & 3 above
        *pOffset = bottomChild - (bottomView - topView);
    }
    else
    {
        // Handle cases: 5 & 6 above.
        *pOffset = topView;
    }

    return S_OK;
}

// Ensure the offset we're scrolling to is valid.
_Check_return_ HRESULT ScrollContentPresenter::ValidateInputOffset(
    _In_ DOUBLE offset,
    _In_ DOUBLE minOffset,
    _In_ DOUBLE maxOffset,
    _Out_ DOUBLE* pValidatedOffset)
{
    HRESULT hr = S_OK;

    if (DoubleUtil::IsNaN(offset))
    {
        // throw new ArgumentOutOfRangeException("offset");
        IFC(E_FAIL);
    }

    *pValidatedOffset = DoubleUtil::Max(minOffset, DoubleUtil::Min(offset, maxOffset));

Cleanup:
    RRETURN(hr);
}

// Apply a template to the ScrollContentPresenter.
IFACEMETHODIMP ScrollContentPresenter::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    if (m_isChildActualWidthUsedAsExtent)
    {
#ifdef DM_DEBUG
        if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                L"DMSCP[0x%p]: OnApplyTemplate - Stopping use of actual width as extent.", this));
        }
#endif // DM_DEBUG
        // Since a new Content is set, assume that the default behavior of using its desired size as the IScrollInfo extent is acceptable.
        StopUseOfActualWidthAsExtent();
    }

    if (m_isChildActualHeightUsedAsExtent)
    {
#ifdef DM_DEBUG
        if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                L"DMSCP[0x%p]: OnApplyTemplate - Stopping use of actual height as extent.", this));
        }
#endif // DM_DEBUG
        // Since a new Content is set, assume that the default behavior of using its desired size as the IScrollInfo extent is acceptable.
        StopUseOfActualHeightAsExtent();
    }

    IFC(ScrollContentPresenterGenerated::OnApplyTemplate());

    // Get our scrolling owner and content talking.
    IFC(HookupScrollingComponents());

Cleanup:
    RRETURN(hr);
}

// Returns an offset coerced into the [0, Extent - Viewport] range.
DOUBLE ScrollContentPresenter::CoerceOffset(
    _In_ DOUBLE offset,
    _In_ DOUBLE extent,
    _In_ DOUBLE viewport)
{
    if (offset > extent - viewport)
    {
        offset = extent - viewport;
    }

    if (offset < 0)
    {
        offset = 0;
    }

    return offset;
}

// Helper method to get our ScrollViewer owner and its scrolling content
// talking.  Method introduces the current owner/content, and clears a from any
// previous content.
_Check_return_ HRESULT ScrollContentPresenter::HookupScrollingComponents()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollViewer> spScrollContainer;
    ctl::ComPtr<DependencyObject> spTemplatedParent;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    ctl::ComPtr<IScrollInfo> spChildScrollInfo;
    ctl::ComPtr<IScrollInfo> spCurrentScrollInfo;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    ctl::ComPtr<IDependencyObject> spChildAsDO;
    ctl::ComPtr<IItemsPresenter> spContentAsIP;
    ctl::ComPtr<IOrientedVirtualizingPanel> spVirtualizingPanel;
    ctl::ComPtr<IManipulationDataProvider> spProvider;

    // We need to introduce our IScrollInfo to our ScrollViewer (and break any
    // previous links).
    IFC(get_TemplatedParent(&spTemplatedParent));
    spScrollContainer = spTemplatedParent.AsOrNull<xaml_controls::IScrollViewer>();

    IFC(m_wrScrollInfo.As(&spCurrentScrollInfo));

    // If our content is not an IScrollInfo, we should have selected a style
    // that contains one.
    if (spScrollContainer)
    {
        // We need to get an IScrollInfo to introduce to the ScrollViewer.
        // 1. Try our content...
        IFC(get_Content(&spContent));
        spScrollInfo = spContent.AsOrNull<IScrollInfo>();

        // 2. Our child might be an ItemsPresenter. In this case check its child for being an IScrollInfo
        spContentAsIP = spContent.AsOrNull<IItemsPresenter>();
        if (spContentAsIP)
        {
            INT childCount = 0;
            BOOLEAN bTemplateApplied = FALSE;
            IFC(spContentAsIP.Cast<ItemsPresenter>()->InvokeApplyTemplate(&bTemplateApplied));

            IFC(VisualTreeHelper::GetChildrenCountStatic(spContentAsIP.Cast<ItemsPresenter>(), &childCount));
            if (childCount > 0)
            {
                // At this point, we either have one child (the panel) or three (header, panel and then footer).
                INT panelIndex = (childCount == 1) ? 0 : 1;

                IFC(VisualTreeHelper::GetChildStatic(spContentAsIP.Cast<ItemsPresenter>(), panelIndex, &spChildAsDO));
                spChildScrollInfo = spChildAsDO.AsOrNull<IScrollInfo>();

                if (!spChildScrollInfo)
                {
                    // if our child doesn't support IScrollInfo then ScrollContentPresenter is IScrollInfo provider.
                    spScrollInfo = nullptr;
                }
                else
                {
                    // if our child supports IScrollInfo but not IOrientedVirtualizingPanel then let child be IScrollInfo provider.
                    spVirtualizingPanel = spChildAsDO.AsOrNull<IOrientedVirtualizingPanel>();
                    if (!spVirtualizingPanel)
                    {
                        spScrollInfo = spChildScrollInfo;
                    }

                    //else ItemsPresenter became IScrollInfo provider.
                }
            }
        }

        // 3. As a final fallback, we use ourself.
        if (!spScrollInfo)
        {
            spScrollInfo = ctl::query_interface_cast<IScrollInfo>(ctl::as_iunknown(this));
        }

        // Detach any differing previous IScrollInfo from ScrollViewer
        if (spScrollInfo != spCurrentScrollInfo && spCurrentScrollInfo)
        {
            BOOLEAN isScrollClient = FALSE;

            IFC(IsScrollClient(&isScrollClient));
            if (isScrollClient)
            {
                ScrollData* pScrollData = NULL;
                delete m_pScrollData;
                m_pScrollData = NULL;
                IFC(get_ScrollData(&pScrollData));
            }
            else
            {
                IFC(spCurrentScrollInfo->put_ScrollOwner(NULL));
            }
        }

        // Introduce our ScrollViewer and IScrollInfo to each other.
        if (spScrollInfo)
        {
            m_wrScrollInfo = nullptr;
            IFC(spScrollInfo.AsWeak(&m_wrScrollInfo));
            IFC(spScrollInfo->put_ScrollOwner(spScrollContainer.Get()));
            IFC(spScrollContainer.Cast<ScrollViewer>()->put_ScrollInfo(spScrollInfo.Get()));

            spProvider = spScrollInfo.AsOrNull<IManipulationDataProvider>();
            if (spProvider)
            {
                IFC(spProvider->SetZoomFactor(m_fZoomFactor));
            }
        }
    }
    // We're not really in a valid scrolling scenario.  Break any previous
    // references, and get us back into a totally unlinked state.
    else if (spCurrentScrollInfo)
    {
        ctl::ComPtr<IInspectable> spOldScrollOwner;

        IFC(spCurrentScrollInfo->get_ScrollOwner(&spOldScrollOwner));
        IFC(spOldScrollOwner.As(&spScrollOwner));

        if (spScrollOwner)
        {
            IFC(spScrollOwner->put_ScrollInfo(NULL));
        }

        IFC(spCurrentScrollInfo->put_ScrollOwner(NULL));
        m_wrScrollInfo = nullptr;
        delete m_pScrollData;
        m_pScrollData = NULL;
    }

Cleanup:
    RRETURN(hr);
}

// register this instance as under control of a semanticzoom control
_Check_return_ HRESULT ScrollContentPresenter::RegisterAsSemanticZoomPresenter()
{
    m_isSemanticZoomPresenter = TRUE;

    RRETURN(S_OK);
}

// Determine a specialized clip for text scenarios.
_Check_return_ HRESULT ScrollContentPresenter::CalculateTextBoxClipRect(
    _In_ wf::Size availableSize,
    _Out_ wf::Rect* pClipRect)
{
    HRESULT hr = S_OK;

    // Special case for a scroll content presenter containing the text of a
    // TextBox or a RichtextBox: we don't want to clip to the layout boundaries
    // of the text, as that will clip any ovehanging glyph strokes, such as the
    // bottom of a lowercase italic f in a Latin font like Times New Roman, or a
    // Lam or Alif in any Arabic font. See bug 82041 for an example.
    //
    // If this scroll content presenter hosts a TextBoxView or a
    // RichTextBoxView, and if either end of the text is fully in view, then we
    // allow glyphs at those ends to overhang into the padding of the containing
    // ScrollViewer and the 1 pixel selection highlight border by extending the
    // clipping rectangle.

    ctl::ComPtr<IScrollViewer> spScrollViewer;
    ctl::ComPtr<ITextBox> spTextBoxParent;
    ctl::ComPtr<DependencyObject> spTemplatedParent;
    ctl::ComPtr<DependencyObject> spTemplatedGrandParent;

    DOUBLE glyphOverhangLeft = 0.0;
    DOUBLE glyphOverhangRight = 0.0;
    DOUBLE extentWidth = 0.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE offset = 0.0;
    // TODO: Add back when we have RichTextBox
    //    ctl::ComPtr<RichTextBox> spRichTextBoxParent;
    xaml::TextWrapping wrapping = xaml::TextWrapping_NoWrap;
    xaml_controls::ScrollBarVisibility visibility = xaml_controls::ScrollBarVisibility_Disabled;
    ScrollData* pScrollData = NULL;
    xaml::Thickness scrollViewerPadding = {};
    DOUBLE availableWidth = 0.0;
    DOUBLE availableHeight = 0.0;
    wf::Rect clipRect = {};

    IFC(get_TemplatedParent(&spTemplatedParent));
    spScrollViewer = spTemplatedParent.AsOrNull<xaml_controls::IScrollViewer>();
    IFC(get_ScrollData(&pScrollData));
    extentWidth = pScrollData->m_extent.Width;
    viewportWidth = pScrollData->m_viewport.Width;
    offset = pScrollData->get_OffsetX();

    IFC(spScrollViewer.Cast<ScrollViewer>()->get_TemplatedParent(&spTemplatedGrandParent));
    spTextBoxParent = spTemplatedGrandParent.AsOrNull<xaml_controls::ITextBox>();
    // TODO: Add back when we have RichTextBox
    //    spRichTextBoxParent = spTemplatedGrandParent.AsOrNull<xaml_controls::IRichTextBox>();

    // Detemine the TextWrapping and HorizontalScrollBarVisiblity properties.
    if (spTextBoxParent)
    {
        IFC(spTextBoxParent->get_TextWrapping(&wrapping));
        // TODO: Add back when TextBox has a HorizontalScrollBarVisibility property
        //        IFC(spTextBoxParent->get_HorizontalScrollBarVisibility(&visibility));
    }
    // TODO: Add back when we have RichTextBox
    //    else if (spRichTextBoxParent)
    //    {
    //        IFC(spRichTextBoxParent->get_TextWrapping(&wrapping));
    //        IFC(spRichTextBoxParent->get_HorizontalScrollBarVisibility(&visibility));
    //    }

    // Determine the space to reserve for left and right glyph overhang
    IFC(spScrollViewer.Cast<ScrollViewer>()->get_Padding(&scrollViewerPadding));
    if (wrapping == xaml::TextWrapping_Wrap)
    {
        // If TextWrapping="wrap" then the text always fits the margins and we
        // always want to allow glyphs to overhang into both margins.
        glyphOverhangLeft = scrollViewerPadding.Left + 1.0;
        glyphOverhangRight = scrollViewerPadding.Right + 1.0;
    }
    else
    {
        // We're not wrapping.
        // The left end is quite easy:
        if (viewportWidth > extentWidth || offset == 0)
        {
            // Left end of content is fully in view
            glyphOverhangLeft = scrollViewerPadding.Left + 1.0;
        }

        // The right end is not so easy, because when client disables the
        // horizontal scrollbar we don't bother to measure the extent beyond
        // the viewport width. So with a disabled horizontal scrollbar we can
        // only trust the extent measurement when it is less than the viewport
        // width.
        if (viewportWidth > extentWidth ||
            (visibility != xaml_controls::ScrollBarVisibility_Disabled &&
            DoubleUtil::Abs(extentWidth - offset + viewportWidth) <= 1.0))
        {
            // Right end of content is fully in view
            glyphOverhangRight = scrollViewerPadding.Right + 1.0;
        }
    }

    // Note that we only want to expand the clip. We use Math.Max to
    // enforce this for cases where the client provides negative values
    // for padding left and/or right.
    glyphOverhangLeft = DoubleUtil::Max(0.0, glyphOverhangLeft);
    glyphOverhangRight = DoubleUtil::Max(0.0, glyphOverhangRight);

    // Return the clipping rectangle with the calculated overhangs.
    availableWidth = availableSize.Width;
    availableHeight = availableSize.Height;
    clipRect.X = static_cast<FLOAT>(-glyphOverhangLeft);
    clipRect.Y = 0;
    clipRect.Width = static_cast<FLOAT>(availableWidth + glyphOverhangLeft + glyphOverhangRight);
    clipRect.Height = static_cast<FLOAT>(availableHeight);
    *pClipRect = clipRect;

Cleanup:
    RRETURN(hr);
}

// ScrollContentPresenter clips its content to arrange size.
// No clip is applied if its CanContentRenderOutsideBounds property is set to True though.
_Check_return_ HRESULT ScrollContentPresenter::UpdateClip(
    _In_ wf::Size availableSize)
{
    BOOLEAN canContentRenderOutsideBounds = FALSE;

    IFC_RETURN(get_CanContentRenderOutsideBounds(&canContentRenderOutsideBounds));

    if (canContentRenderOutsideBounds)
    {
        if (m_isClipPropertySet)
        {
            IFC_RETURN(put_Clip(nullptr));
            m_isClipPropertySet = false;
        }
    }
    else
    {
        if (!m_isClipPropertySet)
        {
            ctl::ComPtr<RectangleGeometry> spClippingGeometry;
            IFC_RETURN(ctl::make<RectangleGeometry>(&spClippingGeometry));
            SetPtrValue(m_tpClippingRectangle, spClippingGeometry);
            IFC_RETURN(put_Clip(m_tpClippingRectangle.Get()));
            m_isClipPropertySet = true;
        }

        wf::Rect clipRect = {};

        // TODO: Add back when we have ITextBoxView/IRichTextBoxView
        //    IFC(get_TemplatedParent(&pTemplatedParent));
        //    IFC(get_Content(&pContent));
        //
        //    if (ctl::is<IScrollViewer>(ctl::as_iinspectable(pTemplatedParent)) &&
        //        (ctl::is<ITextBoxView>(pContentAsII) || ctl::is<IRichTextBoxView>(pContentAsII)))
        //    {
        //        // We may need to allow glyphs to overhang into the ScrollViewers padding
        //        IFC(CalculateTextBoxClipRect(availableSize, &clip));
        //        clipRect = clip;
        //    }
        //    else
        //    {
        clipRect.X = clipRect.Y = 0;
        clipRect.Width = availableSize.Width;
        clipRect.Height = availableSize.Height;
        //    }
        IFC_RETURN(m_tpClippingRectangle->put_Rect(clipRect));
    }

    return S_OK;
}

// Called when a criteria for the CanUseActualWidthAsExtent or CanUseActualHeightAsExtent evaluation changed.
// Calls InvalidateMeasure when the evaluation actually changes so the special
// mode can be entered or exited.
_Check_return_ HRESULT ScrollContentPresenter::RefreshUseOfActualSizeAsExtent(
    _In_ UIElement* pManipulatedElement)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;
    ScrollData* pScrollData = NULL;

    IFC(IsScrollClient(&isScrollClient));
    IFC(get_ScrollData(&pScrollData));
    if (isScrollClient && pScrollData != nullptr)
    {
        ctl::ComPtr<IScrollOwner> spScrollOwner;
        ctl::ComPtr<IScrollViewer> spScrollViewer;
        ctl::ComPtr<IFrameworkElement> spContentFE;
        bool canUseActualWidthAsExtent = false;
        bool canUseActualHeightAsExtent = false;

        IFC(get_ScrollOwner(&spScrollOwner));
        IFC(spScrollOwner.As(&spScrollViewer));
        IFC(ctl::do_query_interface(spContentFE, pManipulatedElement));

        IFC(CanUseActualWidthAsExtent(
            spScrollOwner.Get(),
            spScrollViewer.Get(),
            spContentFE.Get(),
            &canUseActualWidthAsExtent));

        if (canUseActualWidthAsExtent == m_isChildActualWidthUsedAsExtent)
        {
            IFC(CanUseActualHeightAsExtent(
                spScrollOwner.Get(),
                spScrollViewer.Get(),
                spContentFE.Get(),
                &canUseActualHeightAsExtent));
        }

        if (m_isChildActualWidthUsedAsExtent != canUseActualWidthAsExtent || m_isChildActualHeightUsedAsExtent != canUseActualHeightAsExtent)
        {
#ifdef DM_DEBUG
            if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
                    L"DMSCP[0x%p]: RefreshUseOfActualSizeAsExtent - Calling InvalidateMeasure.", this));
            }
#endif // DM_DEBUG
            IFC(InvalidateMeasure());
        }
    }

Cleanup:
    return hr;
}

// Determines whether the mode that uses the child's actual size for the IScrollInfo extent is applicable or not.
// The answer is partially evaluated with a temporary reg key.
_Check_return_ HRESULT ScrollContentPresenter::CanUseActualWidthAsExtent(
    _In_opt_ IScrollOwner* pScrollOwner,
    _In_opt_ IScrollViewer* pScrollViewer,
    _In_opt_ IFrameworkElement* pContentFE,
    _Out_ bool* pCanUseActualWidthAsExtent)
{
    ASSERT(pCanUseActualWidthAsExtent);
    *pCanUseActualWidthAsExtent = false;

    if (pContentFE == nullptr)
    {
        return S_OK;
    }

    xaml::HorizontalAlignment horizontalContentFEAlignment = xaml::HorizontalAlignment_Center;
    IFC_RETURN(pContentFE->get_HorizontalAlignment(&horizontalContentFEAlignment));
    if (horizontalContentFEAlignment != xaml::HorizontalAlignment_Stretch)
    {
        // In order to minimize the risks for regressions, we only stop using
        // the child's desired size in known problematic situations. Bugs have
        // only surfaced when the Stretched alignment is used.
        // Do not enter the special mode unless a Stretch alignment is used.
        return S_OK;
    }

    if (static_cast<FrameworkElement*>(pContentFE)->IsWidthSpecified())
    {
        // When the child has a non-default Width, MinWidth or MaxWidth, the
        // desired width reflects the correct extent to push via IScrollInfo,
        // while the actual width does not.
        return S_OK;
    }

    if (pScrollViewer != nullptr)
    {
        // Do not enter the special mode when the ScrollViewer is using an imposed layout size.
        // This situation arises with the SemanticZoom control which imposes a size for the
        // ScrollContentPresenter's child. See how ScrollContentPresenter::MeasureOverride
        // uses GetLayoutSize() for the desired size and IScrollInfo extent size.
        wf::Size layoutSize = static_cast<ScrollViewer*>(pScrollViewer)->GetLayoutSize();
        if (layoutSize.Width != 0.0f)
        {
            return S_OK;
        }
    }

    *pCanUseActualWidthAsExtent = true;

    return S_OK;
}

_Check_return_ HRESULT ScrollContentPresenter::CanUseActualHeightAsExtent(
    _In_opt_ IScrollOwner* pScrollOwner,
    _In_opt_ IScrollViewer* pScrollViewer,
    _In_opt_ IFrameworkElement* pContentFE,
    _Out_ bool* pCanUseActualHeightAsExtent)
{
    ASSERT(pCanUseActualHeightAsExtent);
    *pCanUseActualHeightAsExtent = false;

    if (pContentFE == nullptr)
    {
        return S_OK;
    }

    xaml::VerticalAlignment verticalContentFEAlignment = xaml::VerticalAlignment_Center;
    IFC_RETURN(pContentFE->get_VerticalAlignment(&verticalContentFEAlignment));
    if (verticalContentFEAlignment != xaml::VerticalAlignment_Stretch)
    {
        // In order to minimize the risks for regressions, we only stop using
        // the child's desired size in known problematic situations. Bugs have
        // only surfaced when the Stretched alignment is used.
        // Do not enter the special mode unless a Stretch alignment is used.
        return S_OK;
    }

    if (static_cast<FrameworkElement*>(pContentFE)->IsHeightSpecified())
    {
        // When the child has a non-default Height, MinHeight or MaxHeight, the
        // desired height reflects the correct extent to push via IScrollInfo,
        // while the actual height does not.
        return S_OK;
    }

    if (pScrollViewer != nullptr)
    {
        // Do not enter the special mode when the ScrollViewer is using an imposed layout size.
        // This situation arises with the SemanticZoom control which imposes a size for the
        // ScrollContentPresenter's child. See how ScrollContentPresenter::MeasureOverride
        // uses GetLayoutSize() for the desired size and IScrollInfo extent size.
        wf::Size layoutSize = static_cast<ScrollViewer*>(pScrollViewer)->GetLayoutSize();
        if (layoutSize.Height != 0.0f)
        {
            return S_OK;
        }
    }

    *pCanUseActualHeightAsExtent = true;

    return S_OK;
}

// Verifies scrolling data using the passed viewport and extent as newly
// computed values.  Checks the X/Y offset and coerces them into the range
// [0, Extent - ViewportSize].  If extent, viewport, or the newly coerced
// offsets are different than the existing offset, caches are updated and
// InvalidateScrollInfo() is called.
_Check_return_ HRESULT ScrollContentPresenter::VerifyScrollData(
    _In_ wf::Size viewport,
    _In_ wf::Size extent)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    BOOLEAN valid = FALSE;
    ScrollData* pScrollData = NULL;
    FLOAT oldViewportWidth = 0.0;
    FLOAT oldViewportHeight = 0.0;
    FLOAT oldExtentWidth = 0.0;
    FLOAT oldExtentHeight = 0.0;
    BOOLEAN coerce = FALSE;

    // Update cache values of viewport/extent sizes first, then coerce offsets
    // as these sizes may have changed.
    IFC(get_ScrollData(&pScrollData));
    oldViewportWidth = pScrollData->m_viewport.Width;
    oldViewportHeight = pScrollData->m_viewport.Height;
    valid = (oldViewportWidth == viewport.Width && oldViewportHeight == viewport.Height);
    pScrollData->m_viewport.Width = viewport.Width;
    pScrollData->m_viewport.Height = viewport.Height;

    oldExtentWidth = pScrollData->m_extent.Width;
    oldExtentHeight = pScrollData->m_extent.Height;
    valid &= (oldExtentWidth == extent.Width && oldExtentHeight == extent.Height);
    pScrollData->m_extent.Width = extent.Width;
    pScrollData->m_extent.Height = extent.Height;

    IFC(CoerceOffsets(&coerce));
    valid &= coerce;

    m_fLastZoomFactorApplied = m_fZoomFactor;

    IFC(pScrollData->get_ScrollOwner(&spScrollOwner));
    if (!valid && spScrollOwner)
    {
        StoreLayoutCycleWarningContext(oldViewportWidth, oldViewportHeight, oldExtentWidth, oldExtentHeight, pScrollData);
        IFC(spScrollOwner->InvalidateScrollInfoImpl());
    }

Cleanup:
    RRETURN(hr);
}

// Coerce both of the offsets using CoerceOffset method and store them as the
// new computed offsets if they've changed.
_Check_return_ HRESULT ScrollContentPresenter::CoerceOffsets(
    _Out_ BOOLEAN* pIsValid)
{
    HRESULT hr = S_OK;
    ScrollData* pScrollData = NULL;
    BOOLEAN valid = FALSE;
    DOUBLE newX = 0.0;
    DOUBLE newY = 0.0;
    DOUBLE offset = 0.0;
    DOUBLE extent = 0.0;
    DOUBLE viewport = 0.0;
    DOUBLE computedX = 0.0;
    DOUBLE computedY = 0.0;

#ifdef DBG
    BOOLEAN isScrollClient = FALSE;
    IGNOREHR(IsScrollClient(&isScrollClient));
    ASSERT(isScrollClient);
#endif // DBG

    IFC(get_ScrollData(&pScrollData));

    offset = pScrollData->get_OffsetX();
    extent = pScrollData->m_extent.Width;
    viewport = pScrollData->m_viewport.Width;
    newX = CoerceOffset(offset, extent, viewport);

    offset = pScrollData->get_OffsetY();
    extent = pScrollData->m_extent.Height;
    viewport = pScrollData->m_viewport.Height;
    newY = CoerceOffset(offset, extent, viewport);

    computedX = pScrollData->m_ComputedOffset.X;
    computedY = pScrollData->m_ComputedOffset.Y;
    valid = DoubleUtil::AreClose(newX, computedX) && DoubleUtil::AreClose(newY, computedY);

    pScrollData->m_ComputedOffset.X = newX;
    pScrollData->m_ComputedOffset.Y = newY;

    if (!pScrollData->m_canHorizontallyScroll)
    {
        // Reset the horizontal offset when m_canHorizontallyScroll becomes False (for example
        // when HorizontalScrollbarVisibility becomes Disabled while there is an existing offset)
        ASSERT(pScrollData->m_ComputedOffset.X == 0.0);
        if (pScrollData->get_OffsetX() != 0.0f)
        {
            IFC(pScrollData->put_OffsetX(0.0f));
        }
    }

    if (!pScrollData->m_canVerticallyScroll)
    {
        // Reset the vertical offset when m_canVerticallyScroll becomes False (for example
        // when VerticalScrollbarVisibility becomes Disabled while there is an existing offset)
        ASSERT(pScrollData->m_ComputedOffset.Y == 0.0);
        if (pScrollData->get_OffsetY() != 0.0f)
        {
            IFC(pScrollData->put_OffsetY(0.0f));
        }
    }

    *pIsValid = valid;

Cleanup:
    RRETURN(hr);
}

// Gets a value indicating whether the current ScrollContentPresenter is a
// ScrollViewer's client.
_Check_return_ HRESULT ScrollContentPresenter::IsScrollClient(
    _Out_ BOOLEAN* pbIsScrollClient)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollInfo> spCurrentScrollInfo;

    IFCEXPECT(pbIsScrollClient);
    *pbIsScrollClient = FALSE;

    IFC(m_wrScrollInfo.As(&spCurrentScrollInfo));

    *pbIsScrollClient = spCurrentScrollInfo.Get() == this;

Cleanup:
    RRETURN(hr);
}

// Gets a value indicating whether the current ScrollData's m_Offset
// and m_ComputedOffset are in sync or not.
_Check_return_ HRESULT ScrollContentPresenter::AreScrollOffsetsInSync(
    _Out_ BOOLEAN& areScrollOffsetsInSync)
{
    HRESULT hr = S_OK;
    ScrollData* pScrollData = NULL;

    areScrollOffsetsInSync = FALSE;

    IFC(get_ScrollData(&pScrollData));
    if (pScrollData)
    {
        areScrollOffsetsInSync =
            DoubleUtil::AreClose(pScrollData->get_OffsetX(), pScrollData->m_ComputedOffset.X) &&
            DoubleUtil::AreClose(pScrollData->get_OffsetY(), pScrollData->m_ComputedOffset.Y);
    }

Cleanup:
    RRETURN(hr);
}

// Get (or create on demand) the ScrollContentPresenter's scrolling
// state.
_Check_return_ HRESULT ScrollContentPresenter::get_ScrollData(
    _Outptr_ ScrollData** ppScrollData)
{
    HRESULT hr = S_OK;

    if (!m_pScrollData)
    {
        IFC(ScrollData::Create(&m_pScrollData));
    }

    *ppScrollData = m_pScrollData;

Cleanup:
    RRETURN(hr);
}

// Called to let the peer know when InputPane is showing
void ScrollContentPresenter::NotifyInputPaneStateChange(
    _In_ BOOLEAN isInputPaneShow)
{
    m_isInputPaneShow = isInputPaneShow;
}

// Called to let the peer know when InputPane transition is applied.
_Check_return_ HRESULT ScrollContentPresenter::ApplyInputPaneTransition(
    _In_ BOOLEAN isInputPaneTransitionEnabled)
{
    HRESULT hr = S_OK;

    if (!m_tpInputPaneThemeTransition)
    {
        ctl::ComPtr<wfc::IVector<xaml_animation::Transition*>> spTransitionCollection;
        ctl::ComPtr<InputPaneThemeTransition> spTransition;

        IFC(ctl::make<InputPaneThemeTransition>(&spTransition));

        IFC(get_ContentTransitions(&spTransitionCollection));
        if (!spTransitionCollection)
        {
            ctl::ComPtr<TransitionCollection> spNewTransitionCollection;

            IFC(ctl::make<TransitionCollection>(&spNewTransitionCollection));
            IFC(put_ContentTransitions(spNewTransitionCollection.Get()));
            spTransitionCollection = spNewTransitionCollection;
        }
        IFC(spTransitionCollection->Append(spTransition.Get()));

        SetPtrValue(m_tpInputPaneThemeTransition, spTransition);
    }

    m_tpInputPaneThemeTransition.Cast<InputPaneThemeTransition>()->SetInputPaneState(m_isInputPaneShow);
    m_tpInputPaneThemeTransition.Cast<InputPaneThemeTransition>()->SetInputPaneTransitionState(isInputPaneTransitionEnabled);

Cleanup:
    RRETURN(hr);
}

// Updates the zoom factor
_Check_return_ HRESULT ScrollContentPresenter::SetZoomFactor(
    _In_ FLOAT newZoomFactor)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:SetZoomFactor oldZF=%f, newZF=%f.",
            this, m_fZoomFactor, newZoomFactor));
    }
#endif // DM_DEBUG

    m_fZoomFactor = newZoomFactor;

    IFC(IsScrollClient(&isScrollClient));
    if (isScrollClient)
    {
        IFC(InvalidateMeasure());
    }
    else
    {
        ctl::ComPtr<IManipulationDataProvider> spProvider;
        ctl::ComPtr<IScrollInfo> spScrollInfo;

        IFC(m_wrScrollInfo.As(&spScrollInfo));
        spProvider = spScrollInfo.AsOrNull<IManipulationDataProvider>();
        if (spProvider)
        {
            IFC(spProvider->SetZoomFactor(m_fZoomFactor));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Returns the TopLeftHeader element if previously set.
_Check_return_ HRESULT ScrollContentPresenter::get_TopLeftHeader(
    _Outptr_result_maybenull_ IUIElement** ppTopLeftHeader)
{
    HRESULT hr = S_OK;

    IFCPTR(ppTopLeftHeader);
    IFC(m_trTopLeftHeader.CopyTo(ppTopLeftHeader));

Cleanup:
    RRETURN(hr);
}

// Sets the TopLeftHeader element
_Check_return_ HRESULT ScrollContentPresenter::put_TopLeftHeader(
    _In_opt_ const ctl::ComPtr<IUIElement>& spTopLeftHeader,
    _In_ ScrollViewer* pOwningScrollViewer)
{
#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:put_TopLeftHeader spTopLeftHeader=0x%p.", this, spTopLeftHeader));
    }
#endif // DM_DEBUG

    ASSERT(pOwningScrollViewer);

    IFC_RETURN(RemoveTopLeftHeader(pOwningScrollViewer, true /*removeFromChildrenCollection*/));
    SetPtrValue(m_trTopLeftHeader, spTopLeftHeader);
    IFC_RETURN(InvalidateMeasure());

    return S_OK;
}

// Returns the TopHeader element if previously set.
_Check_return_ HRESULT ScrollContentPresenter::get_TopHeader(
    _Outptr_result_maybenull_ IUIElement** ppTopHeader)
{
    HRESULT hr = S_OK;

    IFCPTR(ppTopHeader);
    IFC(m_trTopHeader.CopyTo(ppTopHeader));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollContentPresenter::put_TopHeader(
    _In_opt_ const ctl::ComPtr<IUIElement>& spTopHeader,
    _In_ ScrollViewer* pOwningScrollViewer)
{
#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:put_TopHeader spTopHeader=0x%p.", this, spTopHeader));
    }
#endif // DM_DEBUG

    ASSERT(pOwningScrollViewer);

    IFC_RETURN(RemoveTopHeader(pOwningScrollViewer, true /*removeFromChildrenCollection*/));
    SetPtrValue(m_trTopHeader, spTopHeader);
    IFC_RETURN(InvalidateMeasure());

    return S_OK;
}

// Returns the LeftHeader element if previously set.
_Check_return_ HRESULT ScrollContentPresenter::get_LeftHeader(
    _Outptr_result_maybenull_ IUIElement** ppLeftHeader)
{
    HRESULT hr = S_OK;

    IFCPTR(ppLeftHeader);
    IFC(m_trLeftHeader.CopyTo(ppLeftHeader));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollContentPresenter::put_LeftHeader(
    _In_opt_ const ctl::ComPtr<IUIElement>& spLeftHeader,
    _In_ ScrollViewer* pOwningScrollViewer)
{
#ifdef DM_DEBUG
    if (DMSCPv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | XCP_TRACE_VERBOSE | DMSCPv_DBG) /*traceType*/, L"DMSCPv[0x%p]:put_LeftHeader spLeftHeader=0x%p.", this, spLeftHeader));
    }
#endif // DM_DEBUG

    ASSERT(pOwningScrollViewer);

    IFC_RETURN(RemoveLeftHeader(pOwningScrollViewer, true /*removeFromChildrenCollection*/));
    SetPtrValue(m_trLeftHeader, spLeftHeader);
    IFC_RETURN(InvalidateMeasure());

    return S_OK;
}

// Called when this ScrollContentPresenter leaves the tree.
_Check_return_ HRESULT ScrollContentPresenter::OnUnloaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: OnUnloaded.", this));
    }
#endif // DM_DEBUG

    RRETURN(UnparentHeaders());
}

// Called by the owning ScrollViewer when the Content property is changing.
_Check_return_ HRESULT ScrollContentPresenter::OnContentChanging(
    _In_ IInspectable* pOldContent)
{
    auto spOldChild = ctl::query_interface_cast<xaml::IUIElement>(pOldContent);
    if (spOldChild)
    {
        IFC_RETURN(spOldChild.Cast<UIElement>()->ResetGlobalScaleFactor());
    }

    return S_OK;
}

// Called when the parent of this ScrollContentPresenter changed.
_Check_return_ HRESULT ScrollContentPresenter::OnTreeParentUpdated(
    _In_opt_ CDependencyObject* pNewParent,
    _In_ BOOLEAN isParentAlive)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: OnTreeParentUpdated pNewParent=0x%p, isParentAlive=%d.",
            this, pNewParent, isParentAlive));
    }
#endif // DM_DEBUG

    IFC(ScrollContentPresenterGenerated::OnTreeParentUpdated(pNewParent, isParentAlive));

    if (!pNewParent && !DXamlCore::IsShutdownStatic())
    {
        IFC(UnparentHeaders());
        m_trTopLeftHeader.Clear();
        m_trTopHeader.Clear();
        m_trLeftHeader.Clear();
    }

Cleanup:
    RRETURN(hr);
}

// Called when a ScrollContentPresenter dependency property changed.
_Check_return_ HRESULT ScrollContentPresenter::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(ScrollContentPresenterGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ScrollContentPresenter_CanContentRenderOutsideBounds:
        IFC_RETURN(InvalidateArrange());
        break;
    }

    return S_OK;
}

// Called when the children of the ScrollContentPresenter have been cleared.
// Clears the flags that record the headers as children, and marks the headers as associated if present.
_Check_return_ HRESULT ScrollContentPresenter::OnChildrenCleared()
{
    ScrollData* pScrollData = nullptr;
    ScrollViewer* pScrollViewer = nullptr;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    ctl::ComPtr<IScrollViewer> spScrollViewer;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/, L"DMSCP[0x%p]: OnChildrenCleared.", this));
    }
#endif // DM_DEBUG

    IFC_RETURN(get_ScrollData(&pScrollData));
    if (pScrollData)
    {
        IFC_RETURN(pScrollData->get_ScrollOwner(&spScrollOwner));
        if (spScrollOwner)
        {
            IFC_RETURN(spScrollOwner.As(&spScrollViewer));
            if (spScrollViewer)
            {
                pScrollViewer = spScrollViewer.Cast<ScrollViewer>();
            }
        }
    }

    IFC_RETURN(RemoveTopLeftHeader(pScrollViewer, false /*removeFromChildrenCollection*/));
    IFC_RETURN(RemoveTopHeader(pScrollViewer, false /*removeFromChildrenCollection*/));
    IFC_RETURN(RemoveLeftHeader(pScrollViewer, false /*removeFromChildrenCollection*/));

    return S_OK;
}

// Clears the flags that record the headers as children, and
// marks the headers as associated if present.
_Check_return_ HRESULT ScrollContentPresenter::UnparentHeaders()
{
    ScrollData* pScrollData = nullptr;
    ScrollViewer* pScrollViewer = nullptr;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    ctl::ComPtr<IScrollViewer> spScrollViewer;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/, L"DMSCP[0x%p]: UnparentHeaders.", this));
    }
#endif // DM_DEBUG

    if (m_isTopLeftHeaderChild || m_isTopHeaderChild || m_isLeftHeaderChild)
    {
        IFC_RETURN(get_ScrollData(&pScrollData));
        if (pScrollData)
        {
            IFC_RETURN(pScrollData->get_ScrollOwner(&spScrollOwner));
            if (spScrollOwner)
            {
                IFC_RETURN(spScrollOwner.As(&spScrollViewer));
                if (spScrollViewer)
                {
                    pScrollViewer = spScrollViewer.Cast<ScrollViewer>();
                }
            }
        }
        IFC_RETURN(RemoveTopLeftHeader(pScrollViewer, true /*removeFromChildrenCollection*/));
        IFC_RETURN(RemoveTopHeader(pScrollViewer, true /*removeFromChildrenCollection*/));
        IFC_RETURN(RemoveLeftHeader(pScrollViewer, true /*removeFromChildrenCollection*/));
    }

    return S_OK;
}

// Returns the size of the potential headers, taking the zoom factor into account.
_Check_return_ HRESULT ScrollContentPresenter::GetZoomedHeadersSize(_Out_ XSIZEF* pSize)
{
    pSize->width = pSize->height = 0.0;

    ctl::ComPtr<IScrollOwner> spScrollOwner;
    IFC_RETURN(get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner != nullptr)
    {
        ctl::ComPtr<IScrollViewer> spScrollViewer;
        IFC_RETURN(spScrollOwner.As(&spScrollViewer));
        if (spScrollViewer != nullptr)
        {
            IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->GetHeadersSize(pSize));
            pSize->width *= m_fZoomFactor;
            pSize->height *= m_fZoomFactor;
        }
    }

    return S_OK;
}

// Retrieves the primary child as a IUIElement, taking the potential headers into account.
_Check_return_ HRESULT ScrollContentPresenter::GetPrimaryChild(
    _Outptr_ xaml::IUIElement** ppChild)
{
    INT childHeaderCount = 0;
    INT childCount = 0;

    *ppChild = nullptr;

    if (m_isTopLeftHeaderChild)
    {
        childHeaderCount++;
    }

    if (m_isTopHeaderChild)
    {
        childHeaderCount++;
    }

    if (m_isLeftHeaderChild)
    {
        childHeaderCount++;
    }

    IFC_RETURN(VisualTreeHelper::GetChildrenCountStatic(this, &childCount));
    if (childCount > childHeaderCount)
    {
        ctl::ComPtr<IDependencyObject> spChildAsDO;
        ctl::ComPtr<IUIElement> spChild;

        IFC_RETURN(VisualTreeHelper::GetChildStatic(this, 0, &spChildAsDO));
        spChild = spChildAsDO.AsOrNull<xaml::IUIElement>();
        IFC_RETURN(spChild.MoveTo(ppChild));
    }

    return S_OK;
}

// Adds a header to this ScrollContentPresenter's children.
_Check_return_ HRESULT ScrollContentPresenter::AddHeader(
    _In_ const ctl::ComPtr<IScrollViewer>& spScrollViewer,
    _In_opt_ const ctl::ComPtr<IUIElement>& spTopLeftHeader,
    _In_opt_ const ctl::ComPtr<IUIElement>& spTopHeader,
    _In_opt_ const ctl::ComPtr<IUIElement>& spLeftHeader,
    _In_ BOOLEAN isTopHeader,
    _In_ BOOLEAN isLeftHeader)
{
    HRESULT hr = S_OK;
    BOOLEAN isTopLeftHeader = isTopHeader && isLeftHeader;
    INT childCount = 0;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: AddHeader isTopHeader=%d, isLeftHeader=%d.", this, isTopHeader, isLeftHeader));
    }
#endif // DM_DEBUG

    ASSERT(spScrollViewer);
    ASSERT(isTopHeader || isLeftHeader);

    IFC(VisualTreeHelper::GetChildrenCountStatic(this, &childCount));
    ASSERT(childCount <= 3);

    if (isTopLeftHeader)
    {
        ASSERT(!m_isTopLeftHeaderChild);
        ASSERT(spTopLeftHeader.Get());
        IFC(spScrollViewer.Cast<ScrollViewer>()->NotifyHeaderParenting(spTopLeftHeader.Get(), TRUE /*isTopHeader*/, TRUE /*isLeftHeader*/));
        IFC(InsertChildInternal(childCount, spTopLeftHeader));
        m_isTopLeftHeaderChild = TRUE;
        IFC(spScrollViewer.Cast<ScrollViewer>()->NotifyHeaderParented(spTopLeftHeader.Get(), TRUE /*isTopHeader*/, TRUE /*isLeftHeader*/));

        UIElement* uielement = spTopLeftHeader.Cast<UIElement>();
        static_cast<CUIElement*>(uielement->GetHandle())->SetIsScrollViewerHeader(true);
    }
    else if (isTopHeader)
    {
        // TopLeftHeader element must be added after the TopHeader so the correct z-order gets applied.
        if (m_isTopLeftHeaderChild)
        {
            childCount--;
        }
        ASSERT(!m_isTopHeaderChild);
        ASSERT(spTopHeader.Get());
        IFC(spScrollViewer.Cast<ScrollViewer>()->NotifyHeaderParenting(spTopHeader.Get(), TRUE /*isTopHeader*/, FALSE /*isLeftHeader*/));
        IFC(InsertChildInternal(childCount, spTopHeader));
        m_isTopHeaderChild = TRUE;
        IFC(spScrollViewer.Cast<ScrollViewer>()->NotifyHeaderParented(spTopHeader.Get(), TRUE /*isTopHeader*/, FALSE /*isLeftHeader*/));

        UIElement* uielement = spTopHeader.Cast<UIElement>();
        static_cast<CUIElement*>(uielement->GetHandle())->SetIsScrollViewerHeader(true);
    }
    else
    {
        ASSERT(isLeftHeader);
        // TopLeftHeader and TopHeader elements must be added after the LeftHeader so the correct z-order gets applied.
        if (m_isTopHeaderChild)
        {
            childCount--;
        }
        if (m_isTopLeftHeaderChild)
        {
            childCount--;
        }
        ASSERT(!m_isLeftHeaderChild);
        ASSERT(spLeftHeader.Get());
        IFC(spScrollViewer.Cast<ScrollViewer>()->NotifyHeaderParenting(spLeftHeader.Get(), FALSE /*isTopHeader*/, TRUE /*isLeftHeader*/));
        IFC(InsertChildInternal(childCount, spLeftHeader));
        m_isLeftHeaderChild = TRUE;
        IFC(spScrollViewer.Cast<ScrollViewer>()->NotifyHeaderParented(spLeftHeader.Get(), FALSE /*isTopHeader*/, TRUE /*isLeftHeader*/));

        UIElement* uielement = spLeftHeader.Cast<UIElement>();
        static_cast<CUIElement*>(uielement->GetHandle())->SetIsScrollViewerHeader(true);
    }

Cleanup:
    RRETURN(hr);
}

// Removes the top-left header from this ScrollContentPresenter's children
// when removeFromChildrenCollection is True, resets its global scale factor
// and notifies the owning ScrollViewer.
_Check_return_ HRESULT ScrollContentPresenter::RemoveTopLeftHeader(
    _In_opt_ ScrollViewer* pScrollViewer,
    _In_ bool removeFromChildrenCollection)
{
    ctl::ComPtr<IUIElement> spTopLeftHeader;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/, L"DMSCP[0x%p]: RemoveTopLeftHeader. removeFromChildrenCollection=%d", this, removeFromChildrenCollection));
    }
#endif // DM_DEBUG

    if (m_isTopLeftHeaderChild)
    {
        IFC_RETURN(get_TopLeftHeader(&spTopLeftHeader));
        if (spTopLeftHeader)
        {
            if (removeFromChildrenCollection)
            {
                IFC_RETURN(RemoveChildInternal(spTopLeftHeader));
            }
            m_isTopLeftHeaderChild = FALSE;

            // Since spTopLeftHeader is no longer used as a header, discard its GlobalScaleFactor sparse storage.
            // Layout rounding will fall back to just using the plateau scale if this element happens to re-enter
            // the tree as a regular element.
            UIElement* uielement = spTopLeftHeader.Cast<UIElement>();
            IFC_RETURN(uielement->ResetGlobalScaleFactor());
            static_cast<CUIElement*>(uielement->GetHandle())->SetIsScrollViewerHeader(false);

            // If spTopLeftHeader was the last header shown, also reset the  GlobalScaleFactor sparse storage for the primary child.
            if (!m_isLeftHeaderChild && !m_isTopHeaderChild)
            {
                IFC_RETURN(ResetPrimaryChildGlobalScaleFactor());
            }

            if (pScrollViewer)
            {
                IFC_RETURN(pScrollViewer->NotifyHeaderUnparented(spTopLeftHeader.Get(), TRUE /*isTopHeader*/, TRUE /*isLeftHeader*/));
            }
        }
    }

    return S_OK;
}

// Removes the top header from this ScrollContentPresenter's children
// when removeFromChildrenCollection is True, resets its global scale factor
// and notifies the owning ScrollViewer.
_Check_return_ HRESULT ScrollContentPresenter::RemoveTopHeader(
    _In_opt_ ScrollViewer* pScrollViewer,
    _In_ bool removeFromChildrenCollection)
{
    ctl::ComPtr<IUIElement> spTopHeader;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/, L"DMSCP[0x%p]: RemoveTopHeader. removeFromChildrenCollection=%d", this, removeFromChildrenCollection));
    }
#endif // DM_DEBUG

    if (m_isTopHeaderChild)
    {
        IFC_RETURN(get_TopHeader(&spTopHeader));
        if (spTopHeader)
        {
            if (removeFromChildrenCollection)
            {
                IFC_RETURN(RemoveChildInternal(spTopHeader));
            }
            m_isTopHeaderChild = FALSE;

            // Since spTopHeader is no longer used as a header, discard its GlobalScaleFactor sparse storage.
            // Layout rounding will fall back to just using the plateau scale if this element happens to re-enter
            // the tree as a regular element.
            UIElement* uielement = spTopHeader.Cast<UIElement>();
            IFC_RETURN(uielement->ResetGlobalScaleFactor());
            static_cast<CUIElement*>(uielement->GetHandle())->SetIsScrollViewerHeader(false);

            // If spTopHeader was the last header shown, also reset the  GlobalScaleFactor sparse storage for the primary child.
            if (!m_isTopLeftHeaderChild && !m_isLeftHeaderChild)
            {
                IFC_RETURN(ResetPrimaryChildGlobalScaleFactor());
            }

            if (pScrollViewer)
            {
                IFC_RETURN(pScrollViewer->NotifyHeaderUnparented(spTopHeader.Get(), TRUE /*isTopHeader*/, FALSE /*isLeftHeader*/));
            }
        }
    }

    return S_OK;
}

// Removes the left header from this ScrollContentPresenter's children
// when removeFromChildrenCollection is True, resets its global scale factor
// and notifies the owning ScrollViewer.
_Check_return_ HRESULT ScrollContentPresenter::RemoveLeftHeader(
    _In_opt_ ScrollViewer* pScrollViewer,
    _In_ bool removeFromChildrenCollection)
{
    ctl::ComPtr<IUIElement> spLeftHeader;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/, L"DMSCP[0x%p]: RemoveLeftHeader. removeFromChildrenCollection=%d", this, removeFromChildrenCollection));
    }
#endif // DM_DEBUG

    if (m_isLeftHeaderChild)
    {
        IFC_RETURN(get_LeftHeader(&spLeftHeader));
        if (spLeftHeader)
        {
            if (removeFromChildrenCollection)
            {
                IFC_RETURN(RemoveChildInternal(spLeftHeader));
            }
            m_isLeftHeaderChild = FALSE;

            // Since spLeftHeader is no longer used as a header, discard its GlobalScaleFactor sparse storage.
            // Layout rounding will fall back to just using the plateau scale if this element happens to re-enter
            // the tree as a regular element.
            UIElement* uielement = spLeftHeader.Cast<UIElement>();
            IFC_RETURN(uielement->ResetGlobalScaleFactor());
            static_cast<CUIElement*>(uielement->GetHandle())->SetIsScrollViewerHeader(false);

            // If spLeftHeader was the last header shown, also reset the  GlobalScaleFactor sparse storage for the primary child.
            if (!m_isTopLeftHeaderChild && !m_isTopHeaderChild)
            {
                IFC_RETURN(ResetPrimaryChildGlobalScaleFactor());
            }

            if (pScrollViewer)
            {
                IFC_RETURN(pScrollViewer->NotifyHeaderUnparented(spLeftHeader.Get(), FALSE /*isTopHeader*/, TRUE /*isLeftHeader*/));
            }
        }
    }

    return S_OK;
}

// Inserts the provided element in the children collection at the specified index.
_Check_return_ HRESULT ScrollContentPresenter::InsertChildInternal(
    _In_ UINT index,
    _In_ const ctl::ComPtr<IUIElement>& spChild)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    ASSERT(spChild.Get());

    IFC(get_ChildrenInternal(&spChildren));
    IFC(spChildren->InsertAt(index, static_cast<UIElement*>(spChild.Get())));

Cleanup:
    RRETURN(hr);
}

// Removes the provided element from the children collection.
_Check_return_ HRESULT ScrollContentPresenter::RemoveChildInternal(
    _In_ const ctl::ComPtr<IUIElement>& spChild)
{
    HRESULT hr = S_OK;
    UINT index = 0;
    BOOLEAN isChildFound = FALSE;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC(get_ChildrenInternal(&spChildren));
    IFC(spChildren->IndexOf(static_cast<UIElement*>(spChild.Get()), &index, &isChildFound));
    if (isChildFound)
    {
        IFC(spChildren->RemoveAt(index));
    }

Cleanup:
    RRETURN(hr);
}

// Discards the GlobalScaleFactor sparse storage of the primary child so that the layout
// rounding method falls back to just using the plateau scale. Invoked when the last header
// element is unparented by this ScrollContentPresenter.
_Check_return_ HRESULT ScrollContentPresenter::ResetPrimaryChildGlobalScaleFactor()
{
    ctl::ComPtr<IUIElement> spChild;

    IFC_RETURN(GetPrimaryChild(&spChild));
    if (spChild)
    {
        IFC_RETURN(spChild.Cast<UIElement>()->ResetGlobalScaleFactor());
    }

    return S_OK;
}

// Override the default tab-based navigation order when headers are present such that
// the tab order is top-left header -> top header -> left header -> content.
// Handle scenarios where the default behavior is to exit the ScrollContentPresenter or remain inside.
_Check_return_ HRESULT ScrollContentPresenter::ProcessTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_opt_ DependencyObject* pCandidateTabStopElement,
    const bool isBackward,
    const bool /*didCycleFocusAtRootVisualScope*/,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsTabStopOverridden)
{
    HRESULT hr = S_OK;
    BOOLEAN isFocusedElementInTopLeftHeader = FALSE;
    BOOLEAN isFocusedElementInTopHeader = FALSE;
    BOOLEAN isFocusedElementInLeftHeader = FALSE;
    BOOLEAN isFocusedElementInContent = FALSE;
    BOOLEAN isCandidateElementInTopLeftHeader = FALSE;
    BOOLEAN isCandidateElementInTopHeader = FALSE;
    BOOLEAN isCandidateElementInLeftHeader = FALSE;
    BOOLEAN isCandidateElementInContent = FALSE;
    BOOLEAN isElementInTopLeftHeader = FALSE;
    BOOLEAN isElementInTopHeader = FALSE;
    BOOLEAN isElementInLeftHeader = FALSE;
    BOOLEAN isElementInContent = FALSE;
    BOOLEAN isElementDirectChild = FALSE;
    BOOLEAN hasDirectChildWithTabIndexSet = FALSE;
    bool hasFocusableChild = false;
    ctl::ComPtr<DependencyObject> spTabStopDO;
    ctl::ComPtr<DependencyObject> spFirstFocusableDO;
    ctl::ComPtr<DependencyObject> spLastFocusableDO;
    ctl::ComPtr<IDependencyObject> spChildAsDO;
    ctl::ComPtr<IDependencyObject> spScrollOwnerParentAsDO;
    ctl::ComPtr<IDependencyObject> spScrollOwnerAsDO;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    CDependencyObject* pTabStopDO = NULL;
    CDependencyObject* pFirstFocusableDO = NULL;
    CDependencyObject* pLastFocusableDO = NULL;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: ProcessTabStopOverride pFocusedElement=0x%p, pCandidateTabStopElement=0x%p, isBackward=%d.",
            this, pFocusedElement, pCandidateTabStopElement, isBackward));
    }
#endif // DM_DEBUG

    IFCPTR(ppNewTabStop);
    *ppNewTabStop = NULL;
    IFCPTR(pIsTabStopOverridden);
    *pIsTabStopOverridden = FALSE;

    if (!m_isTopLeftHeaderChild && !m_isTopHeaderChild && !m_isLeftHeaderChild)
    {
        // No custom navigation needed when there is no header element.
        goto Cleanup;
    }

    // Determine where the currently focused element and new candidate are in
    // relation to the headers and content.
    IFC(AnalyzeTabbingElements(
        pFocusedElement,
        pCandidateTabStopElement,
        &isFocusedElementInTopLeftHeader,
        &isFocusedElementInTopHeader,
        &isFocusedElementInLeftHeader,
        &isFocusedElementInContent,
        &isCandidateElementInTopLeftHeader,
        &isCandidateElementInTopHeader,
        &isCandidateElementInLeftHeader,
        &isCandidateElementInContent));

    if ((isFocusedElementInTopLeftHeader && isCandidateElementInTopLeftHeader) ||
        (isFocusedElementInTopHeader && isCandidateElementInTopHeader) ||
        (isFocusedElementInLeftHeader && isCandidateElementInLeftHeader) ||
        (isFocusedElementInContent && isCandidateElementInContent))
    {
        // No custom navigation is needed when remaining within the same header or content.
        goto Cleanup;
    }

    if ((isCandidateElementInTopLeftHeader ||
        isCandidateElementInTopHeader ||
        isCandidateElementInLeftHeader ||
        isCandidateElementInContent) &&
        (isFocusedElementInTopLeftHeader ||
        isFocusedElementInTopHeader ||
        isFocusedElementInLeftHeader ||
        isFocusedElementInContent))
    {
        // Attempting to remain within the ScrollContentPresenter. Should it be exited instead?
        IFC(HasDirectChildWithTabIndexSet(&hasDirectChildWithTabIndexSet));
        if (hasDirectChildWithTabIndexSet)
        {
            // Handle scenarios where a custom TabIndex value is set on a header or the content.
            IFC(ProcessTabStopPrivate(
                pFocusedElement,
                isBackward,
                isFocusedElementInTopLeftHeader,
                isFocusedElementInTopHeader,
                isFocusedElementInLeftHeader,
                isFocusedElementInContent,
                ppNewTabStop,
                pIsTabStopOverridden));
        }
        else
        {
            // Handle scenarios where all direct TabIndex values are left at INT32_MAX.
            IFC(GetDirectChild(
                isFocusedElementInTopLeftHeader,
                isFocusedElementInTopHeader,
                isFocusedElementInLeftHeader,
                isFocusedElementInContent,
                &spChildAsDO));
            if (spChildAsDO)
            {
                if (isBackward)
                {
                    ASSERT(!pTabStopDO);
                    pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetNextTabStop(GetHandle(), TRUE);

                    if (pTabStopDO)
                    {
                        // Figure out if the next tab stop belongs to one of the four potential direct ScrollContentPresenter children.
                        IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                        IFC(GetHeaderOwnership(
                            spTabStopDO.Get(),
                            &isElementDirectChild,
                            &isElementInTopLeftHeader,
                            &isElementInTopHeader,
                            &isElementInLeftHeader,
                            &isElementInContent));
                        if (isElementDirectChild)
                        {
                            IFC(CoreImports::FocusManager_CanHaveFocusableChildren(
                                static_cast<CDependencyObject*>(spTabStopDO.Cast<DependencyObject>()->GetHandle()),
                                &hasFocusableChild));
                            if (hasFocusableChild)
                            {
                                IFC(CoreImports::FocusManager_GetLastFocusableElement(
                                    static_cast<CDependencyObject*>(spTabStopDO.Cast<DependencyObject>()->GetHandle()),
                                    &pLastFocusableDO));
                                if (pLastFocusableDO)
                                {
                                    IFC(DXamlCore::GetCurrent()->GetPeer(pLastFocusableDO, &spLastFocusableDO));
                                    IFC(spLastFocusableDO.MoveTo(ppNewTabStop));
                                }
                            }
                            else
                            {
                                IFC(spTabStopDO.MoveTo(ppNewTabStop));
                            }
                        }
                        else
                        {
                            IFC(GetDirectChild(
                                isElementInTopLeftHeader,
                                isElementInTopHeader,
                                isElementInLeftHeader,
                                isElementInContent,
                                &spChildAsDO));
                            if (spChildAsDO)
                            {
                                ASSERT(!pLastFocusableDO);
                                IFC(CoreImports::FocusManager_GetLastFocusableElement(
                                    static_cast<CDependencyObject*>(spChildAsDO.Cast<DependencyObject>()->GetHandle()),
                                    &pLastFocusableDO));
                                if (pLastFocusableDO)
                                {
                                    IFC(DXamlCore::GetCurrent()->GetPeer(pLastFocusableDO, &spLastFocusableDO));
                                    IFC(spLastFocusableDO.MoveTo(ppNewTabStop));
                                }
                            }
                            else
                            {
                                // Next tab stop of spChildAsDO does not belong to this ScrollContentPresenter
                                ReleaseInterface(pTabStopDO);
                                pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetPreviousTabStop(GetHandle());

                                if (pTabStopDO)
                                {
                                    IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                                    IFC(spTabStopDO.MoveTo(ppNewTabStop));
                                }
                                else
                                {
                                    IFC(get_ScrollOwner(&spScrollOwner));
                                    if (spScrollOwner)
                                    {
                                        IFC(spScrollOwner.As(&spScrollOwnerAsDO));
                                        if (spScrollOwnerAsDO)
                                        {
                                            IFC(VisualTreeHelper::GetParentStatic(spScrollOwnerAsDO.Get(), &spScrollOwnerParentAsDO));
                                            if (spScrollOwnerParentAsDO)
                                            {
                                                pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetPreviousTabStop(GetHandle());

                                                if (pTabStopDO)
                                                {
                                                    IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                                                    IFC(spTabStopDO.MoveTo(ppNewTabStop));
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetPreviousTabStop(GetHandle());

                        if (pTabStopDO)
                        {
                            IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                            IFC(spTabStopDO.MoveTo(ppNewTabStop));
                        }
                    }
                }
                else
                {
                    // Moving forward.
                    ASSERT(!pTabStopDO);
                    pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetPreviousTabStop(GetHandle());

                    if (pTabStopDO)
                    {
                        // Figure out if the previous tab stop belongs to one of the four potential direct ScrollContentPresenter children.
                        IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                        IFC(GetHeaderOwnership(
                            spTabStopDO.Get(),
                            &isElementDirectChild,
                            &isElementInTopLeftHeader,
                            &isElementInTopHeader,
                            &isElementInLeftHeader,
                            &isElementInContent));
                        if (isElementDirectChild)
                        {
                            IFC(spTabStopDO.MoveTo(ppNewTabStop));
                        }
                        else
                        {
                            IFC(GetDirectChild(
                                isElementInTopLeftHeader,
                                isElementInTopHeader,
                                isElementInLeftHeader,
                                isElementInContent,
                                &spChildAsDO));
                            if (spChildAsDO)
                            {
                                ASSERT(!pFirstFocusableDO);
                                IFC(CoreImports::FocusManager_GetFirstFocusableElement(
                                    static_cast<CDependencyObject*>(spChildAsDO.Cast<DependencyObject>()->GetHandle()),
                                    &pFirstFocusableDO));
                                if (pFirstFocusableDO)
                                {
                                    IFC(DXamlCore::GetCurrent()->GetPeer(pFirstFocusableDO, &spFirstFocusableDO));
                                    IFC(spFirstFocusableDO.MoveTo(ppNewTabStop));
                                }
                            }
                            else
                            {
                                // Previous tab stop of spChildAsDO does not belong to this ScrollContentPresenter
                                ReleaseInterface(pTabStopDO);
                                pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetNextTabStop(GetHandle(), TRUE);

                                if (pTabStopDO)
                                {
                                    IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                                    IFC(spTabStopDO.MoveTo(ppNewTabStop));
                                }
                                else
                                {
                                    IFC(get_ScrollOwner(&spScrollOwner));
                                    if (spScrollOwner)
                                    {
                                        IFC(spScrollOwner.As(&spScrollOwnerAsDO));
                                        if (spScrollOwnerAsDO)
                                        {
                                            IFC(VisualTreeHelper::GetParentStatic(spScrollOwnerAsDO.Get(), &spScrollOwnerParentAsDO));
                                            if (spScrollOwnerParentAsDO)
                                            {
                                                pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetNextTabStop(GetHandle(), FALSE);

                                                if (pTabStopDO)
                                                {
                                                    IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                                                    IFC(spTabStopDO.MoveTo(ppNewTabStop));
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetNextTabStop(GetHandle(), TRUE);

                        if (pTabStopDO)
                        {
                            IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                            IFC(spTabStopDO.MoveTo(ppNewTabStop));
                        }
                    }
                }
            }
            *pIsTabStopOverridden = (*ppNewTabStop != NULL);
        }
        goto Cleanup;
    }

    if (!isCandidateElementInTopLeftHeader &&
        !isCandidateElementInTopHeader &&
        !isCandidateElementInLeftHeader &&
        !isCandidateElementInContent)
    {
        // Attempting to leave the ScrollContentPresenter. Should focus remain inside instead?
        IFC(HasDirectChildWithTabIndexSet(&hasDirectChildWithTabIndexSet));
        if (hasDirectChildWithTabIndexSet)
        {
            // Handle scenarios where a custom TabIndex value is set on a header or the content.
            IFC(ProcessTabStopPrivate(
                pFocusedElement,
                isBackward,
                isFocusedElementInTopLeftHeader,
                isFocusedElementInTopHeader,
                isFocusedElementInLeftHeader,
                isFocusedElementInContent,
                ppNewTabStop,
                pIsTabStopOverridden));
        }
        else
        {
            // Handle scenarios where all direct TabIndex values are left at INT32_MAX.
            IFC(GetDirectChild(
                isFocusedElementInTopLeftHeader,
                isFocusedElementInTopHeader,
                isFocusedElementInLeftHeader,
                isFocusedElementInContent,
                &spChildAsDO));
            if (spChildAsDO)
            {
                if (isBackward)
                {
                    ASSERT(!pTabStopDO);
                    pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetNextTabStop(GetHandle(), TRUE);

                    if (pTabStopDO)
                    {
                        // Figure out if the previous tab stop belongs to one of the four potential direct ScrollContentPresenter children.
                        IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                        IFC(GetHeaderOwnership(
                            spTabStopDO.Get(),
                            &isElementDirectChild,
                            &isElementInTopLeftHeader,
                            &isElementInTopHeader,
                            &isElementInLeftHeader,
                            &isElementInContent));
                        if (isElementDirectChild)
                        {
                            IFC(CoreImports::FocusManager_CanHaveFocusableChildren(
                                static_cast<CDependencyObject*>(spTabStopDO.Cast<DependencyObject>()->GetHandle()),
                                &hasFocusableChild));
                            if (hasFocusableChild)
                            {
                                IFC(CoreImports::FocusManager_GetLastFocusableElement(
                                    static_cast<CDependencyObject*>(spTabStopDO.Cast<DependencyObject>()->GetHandle()),
                                    &pLastFocusableDO));
                                if (pLastFocusableDO)
                                {
                                    IFC(DXamlCore::GetCurrent()->GetPeer(pLastFocusableDO, &spLastFocusableDO));
                                    IFC(spLastFocusableDO.MoveTo(ppNewTabStop));
                                }
                            }
                            else
                            {
                                IFC(spTabStopDO.MoveTo(ppNewTabStop));
                            }
                        }
                        else
                        {
                            IFC(GetDirectChild(
                                isElementInTopLeftHeader,
                                isElementInTopHeader,
                                isElementInLeftHeader,
                                isElementInContent,
                                &spChildAsDO));
                            if (spChildAsDO)
                            {
                                ASSERT(!pLastFocusableDO);
                                IFC(CoreImports::FocusManager_GetLastFocusableElement(
                                    static_cast<CDependencyObject*>(spChildAsDO.Cast<DependencyObject>()->GetHandle()),
                                    &pLastFocusableDO));
                                if (pLastFocusableDO)
                                {
                                    IFC(DXamlCore::GetCurrent()->GetPeer(pLastFocusableDO, &spLastFocusableDO));
                                    IFC(spLastFocusableDO.MoveTo(ppNewTabStop));
                                }
                            }
                        }
                    }
                }
                else
                {
                    // Moving forward.
                    ASSERT(!pTabStopDO);
                    pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetPreviousTabStop(GetHandle());

                    if (pTabStopDO)
                    {
                        // Figure out if the previous tab stop belongs to one of the four potential direct ScrollContentPresenter children.
                        IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                        IFC(GetHeaderOwnership(
                            spTabStopDO.Get(),
                            &isElementDirectChild,
                            &isElementInTopLeftHeader,
                            &isElementInTopHeader,
                            &isElementInLeftHeader,
                            &isElementInContent));
                        if (isElementDirectChild)
                        {
                            IFC(spTabStopDO.MoveTo(ppNewTabStop));
                        }
                        else
                        {
                            IFC(GetDirectChild(
                                isElementInTopLeftHeader,
                                isElementInTopHeader,
                                isElementInLeftHeader,
                                isElementInContent,
                                &spChildAsDO));
                            if (spChildAsDO)
                            {
                                ASSERT(!pFirstFocusableDO);
                                IFC(CoreImports::FocusManager_GetFirstFocusableElement(
                                    static_cast<CDependencyObject*>(spChildAsDO.Cast<DependencyObject>()->GetHandle()),
                                    &pFirstFocusableDO));
                                if (pFirstFocusableDO)
                                {
                                    IFC(DXamlCore::GetCurrent()->GetPeer(pFirstFocusableDO, &spFirstFocusableDO));
                                    IFC(spFirstFocusableDO.MoveTo(ppNewTabStop));
                                }
                            }
                        }
                    }
                }
            }
            *pIsTabStopOverridden = (*ppNewTabStop != NULL);
        }
    }

Cleanup:
    ReleaseInterface(pFirstFocusableDO);
    ReleaseInterface(pLastFocusableDO);
    RRETURN(hr);
}

// Override the default tab-based navigation order when headers are present such that
// the tab order is top-left header -> top header -> left header -> content.
// Handle scenarios where the default behavior is to enter the ScrollContentPresenter from the outside.
_Check_return_ HRESULT ScrollContentPresenter::ProcessCandidateTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_ DependencyObject* pCandidateTabStopElement,
    _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
    const bool isBackward,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsCandidateTabStopOverridden)
{
    HRESULT hr = S_OK;
    BOOLEAN isFocusedElementInTopLeftHeader = FALSE;
    BOOLEAN isFocusedElementInTopHeader = FALSE;
    BOOLEAN isFocusedElementInLeftHeader = FALSE;
    BOOLEAN isFocusedElementInContent = FALSE;
    BOOLEAN isCandidateElementInTopLeftHeader = FALSE;
    BOOLEAN isCandidateElementInTopHeader = FALSE;
    BOOLEAN isCandidateElementInLeftHeader = FALSE;
    BOOLEAN isCandidateElementInContent = FALSE;
    BOOLEAN isElementInTopLeftHeader = FALSE;
    BOOLEAN isElementInTopHeader = FALSE;
    BOOLEAN isElementInLeftHeader = FALSE;
    BOOLEAN isElementInContent = FALSE;
    BOOLEAN isElementDirectChild = FALSE;
    bool hasFocusableChild = false;
    ctl::ComPtr<DependencyObject> spFirstFocusableDO;
    ctl::ComPtr<DependencyObject> spLastFocusableDO;
    ctl::ComPtr<IDependencyObject> spChildAsDO;
    CDependencyObject* pFirstFocusableDO = NULL;
    CDependencyObject* pLastFocusableDO = NULL;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: ProcessCandidateTabStopOverride pFocusedElement=0x%p, pCandidateTabStopElement=0x%p, isBackward=%d.",
            this, pFocusedElement, pCandidateTabStopElement, isBackward));
    }
#endif // DM_DEBUG

    IFCPTR(pCandidateTabStopElement);
    IFCPTR(ppNewTabStop);
    *ppNewTabStop = NULL;
    IFCPTR(pIsCandidateTabStopOverridden);
    *pIsCandidateTabStopOverridden = FALSE;

    if (!m_isTopLeftHeaderChild && !m_isTopHeaderChild && !m_isLeftHeaderChild)
    {
        // No custom navigation needed when there is no header element.
        goto Cleanup;
    }

    // Determine where the currently focused element and new candidate are in
    // relation to the headers and content.
    IFC(AnalyzeTabbingElements(
        pFocusedElement,
        pCandidateTabStopElement,
        &isFocusedElementInTopLeftHeader,
        &isFocusedElementInTopHeader,
        &isFocusedElementInLeftHeader,
        &isFocusedElementInContent,
        &isCandidateElementInTopLeftHeader,
        &isCandidateElementInTopHeader,
        &isCandidateElementInLeftHeader,
        &isCandidateElementInContent));

    // No custom navigation is needed when remaining within the same header or content.
    if ((isFocusedElementInTopLeftHeader && isCandidateElementInTopLeftHeader) ||
        (isFocusedElementInTopHeader && isCandidateElementInTopHeader) ||
        (isFocusedElementInLeftHeader && isCandidateElementInLeftHeader) ||
        (isFocusedElementInContent && isCandidateElementInContent))
    {
        goto Cleanup;
    }

    // No custom navigation is needed when attempting to leave the ScrollContentPresenter.
    if (!isCandidateElementInTopLeftHeader &&
        !isCandidateElementInTopHeader &&
        !isCandidateElementInLeftHeader &&
        !isCandidateElementInContent)
    {
        goto Cleanup;
    }

    if (!isFocusedElementInTopLeftHeader &&
        !isFocusedElementInTopHeader &&
        !isFocusedElementInLeftHeader &&
        !isFocusedElementInContent)
    {
        ASSERT(isCandidateElementInTopLeftHeader || isCandidateElementInTopHeader || isCandidateElementInLeftHeader || isCandidateElementInContent);

        // Attempting to enter the ScrollContentPresenter.

        // Check if the owning direct child has a TabIndex set.
        IFC(GetDirectChild(
            isCandidateElementInTopLeftHeader,
            isCandidateElementInTopHeader,
            isCandidateElementInLeftHeader,
            isCandidateElementInContent,
            &spChildAsDO));
        IFC(GetTabIndex(spChildAsDO, NULL /*pIsTabStop*/, &m_tabIndex));

        if (isBackward)
        {
            // Temporarily setting m_isTabIndexSet for the synchronous GetFirstFocusableElementOverride callback.
            m_isTabIndexSet = TRUE;
            ASSERT(!pFirstFocusableDO);
            IFC(CoreImports::FocusManager_GetFirstFocusableElement(
                static_cast<CDependencyObject*>(GetHandle()),
                &pFirstFocusableDO));
            m_isTabIndexSet = FALSE;
            if (pFirstFocusableDO)
            {
                IFC(DXamlCore::GetCurrent()->GetPeer(pFirstFocusableDO, &spFirstFocusableDO));
                IFC(GetHeaderOwnership(
                    spFirstFocusableDO.Get(),
                    &isElementDirectChild,
                    &isElementInTopLeftHeader,
                    &isElementInTopHeader,
                    &isElementInLeftHeader,
                    &isElementInContent));
                if (isElementDirectChild)
                {
                    IFC(CoreImports::FocusManager_CanHaveFocusableChildren(
                        static_cast<CDependencyObject*>(spFirstFocusableDO.Cast<DependencyObject>()->GetHandle()),
                        &hasFocusableChild));
                    if (hasFocusableChild)
                    {
                        IFC(CoreImports::FocusManager_GetLastFocusableElement(
                            static_cast<CDependencyObject*>(spFirstFocusableDO.Cast<DependencyObject>()->GetHandle()),
                            &pLastFocusableDO));
                        if (pLastFocusableDO)
                        {
                            IFC(DXamlCore::GetCurrent()->GetPeer(pLastFocusableDO, &spLastFocusableDO));
                            IFC(spLastFocusableDO.MoveTo(ppNewTabStop));
                        }
                    }
                    else
                    {
                        IFC(spFirstFocusableDO.MoveTo(ppNewTabStop));
                    }
                }
                else
                {
                    IFC(GetDirectChild(
                        isElementInTopLeftHeader,
                        isElementInTopHeader,
                        isElementInLeftHeader,
                        isElementInContent,
                        &spChildAsDO));
                    if (spChildAsDO)
                    {
                        ASSERT(!pLastFocusableDO);
                        IFC(CoreImports::FocusManager_GetLastFocusableElement(
                            static_cast<CDependencyObject*>(spChildAsDO.Cast<DependencyObject>()->GetHandle()),
                            &pLastFocusableDO));
                        if (pLastFocusableDO)
                        {
                            IFC(DXamlCore::GetCurrent()->GetPeer(pLastFocusableDO, &spLastFocusableDO));
                            IFC(spLastFocusableDO.MoveTo(ppNewTabStop));
                        }
                    }
                }
            }
        }
        else
        {
            // Temporarily setting m_isTabIndexSet for the synchronous GetLastFocusableElementOverride callback.
            m_isTabIndexSet = TRUE;
            ASSERT(!pLastFocusableDO);
            IFC(CoreImports::FocusManager_GetLastFocusableElement(
                static_cast<CDependencyObject*>(GetHandle()),
                &pLastFocusableDO));
            m_isTabIndexSet = FALSE;
            if (pLastFocusableDO)
            {
                IFC(DXamlCore::GetCurrent()->GetPeer(pLastFocusableDO, &spLastFocusableDO));
                IFC(GetHeaderOwnership(
                    spLastFocusableDO.Get(),
                    &isElementDirectChild,
                    &isElementInTopLeftHeader,
                    &isElementInTopHeader,
                    &isElementInLeftHeader,
                    &isElementInContent));
                if (isElementDirectChild)
                {
                    IFC(spLastFocusableDO.MoveTo(ppNewTabStop));
                }
                else
                {
                    IFC(GetDirectChild(
                        isElementInTopLeftHeader,
                        isElementInTopHeader,
                        isElementInLeftHeader,
                        isElementInContent,
                        &spChildAsDO));
                    if (spChildAsDO)
                    {
                        ASSERT(!pFirstFocusableDO);
                        IFC(CoreImports::FocusManager_GetFirstFocusableElement(
                            static_cast<CDependencyObject*>(spChildAsDO.Cast<DependencyObject>()->GetHandle()),
                            &pFirstFocusableDO));
                        if (pFirstFocusableDO)
                        {
                            IFC(DXamlCore::GetCurrent()->GetPeer(pFirstFocusableDO, &spFirstFocusableDO));
                            IFC(spFirstFocusableDO.MoveTo(ppNewTabStop));
                        }
                    }
                }
            }
        }
        *pIsCandidateTabStopOverridden = (*ppNewTabStop != NULL);
    }

Cleanup:
    m_isTabIndexSet = FALSE;
    ReleaseInterface(pFirstFocusableDO);
    ReleaseInterface(pLastFocusableDO);
    RRETURN(hr);
}

// Returns the first focusable element among the headers and content with a TabIndex equal to m_tabIndex.
_Check_return_ HRESULT ScrollContentPresenter::GetFirstFocusableElementOverride(
    _Outptr_ DependencyObject** ppFirstFocusable)
{
    HRESULT hr = S_OK;
    BOOLEAN isTabStop = FALSE;
    INT tabIndex = 0;
    INT minTabIndex = 0;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IDependencyObject> spDependencyObject;
    ctl::ComPtr<IDependencyObject> spFirstFocusable;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: GetFirstFocusableElementOverride - entry.", this));
    }
#endif // DM_DEBUG

    ASSERT(ppFirstFocusable);
    *ppFirstFocusable = NULL;

    if (!m_isTopLeftHeaderChild && !m_isTopHeaderChild && !m_isLeftHeaderChild)
    {
        goto Cleanup;
    }

    if (m_isTabIndexSet)
    {
        IFC(get_Content(&spContent));
        spDependencyObject = spContent.AsOrNull<IDependencyObject>();
        if (spDependencyObject)
        {
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop && tabIndex == m_tabIndex)
            {
                spFirstFocusable = spDependencyObject;
            }
        }

        if (m_isLeftHeaderChild && !spFirstFocusable)
        {
            IFC(get_LeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && tabIndex == m_tabIndex)
                {
                    IFC(spHeader.As<IDependencyObject>(&spFirstFocusable));
                }
            }
        }

        if (m_isTopHeaderChild && !spFirstFocusable)
        {
            IFC(get_TopHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && tabIndex == m_tabIndex)
                {
                    IFC(spHeader.As<IDependencyObject>(&spFirstFocusable));
                }
            }
        }

        if (m_isTopLeftHeaderChild && !spFirstFocusable)
        {
            IFC(get_TopLeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && tabIndex == m_tabIndex)
                {
                    IFC(spHeader.As<IDependencyObject>(&spFirstFocusable));
                }
            }
        }
    }
    else
    {
        minTabIndex = INT32_MAX;

        if (m_isTopLeftHeaderChild)
        {
            IFC(get_TopLeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop)
                {
                    IFC(spHeader.As<IDependencyObject>(&spFirstFocusable));
                    minTabIndex = tabIndex;
                }
            }
        }

        if (m_isTopHeaderChild)
        {
            IFC(get_TopHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex < minTabIndex || !spFirstFocusable))
                {
                    IFC(spHeader.As<IDependencyObject>(&spFirstFocusable));
                    minTabIndex = tabIndex;
                }
            }
        }

        if (m_isLeftHeaderChild)
        {
            IFC(get_LeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex < minTabIndex || !spFirstFocusable))
                {
                    IFC(spHeader.As<IDependencyObject>(&spFirstFocusable));
                    minTabIndex = tabIndex;
                }
            }
        }

        IFC(get_Content(&spContent));
        spDependencyObject = spContent.AsOrNull<IDependencyObject>();
        if (spDependencyObject)
        {
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop && (tabIndex < minTabIndex || !spFirstFocusable))
            {
                spFirstFocusable = spDependencyObject;
            }
        }
    }

    if (spFirstFocusable)
    {
        *ppFirstFocusable = static_cast<DependencyObject*>(spFirstFocusable.Detach());
    }

Cleanup:
    RRETURN(hr);
}

// Returns the last focusable element among the headers and content with a TabIndex equal to m_tabIndex.
_Check_return_ HRESULT ScrollContentPresenter::GetLastFocusableElementOverride(
    _Outptr_ DependencyObject** ppLastFocusable)
{
    HRESULT hr = S_OK;
    BOOLEAN isTabStop = FALSE;
    INT tabIndex = 0;
    INT maxTabIndex = 0;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IDependencyObject> spDependencyObject;
    ctl::ComPtr<IDependencyObject> spLastFocusable;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: GetLastFocusableElementOverride - entry.", this));
    }
#endif // DM_DEBUG

    ASSERT(ppLastFocusable);
    *ppLastFocusable = NULL;

    if (!m_isTopLeftHeaderChild && !m_isTopHeaderChild && !m_isLeftHeaderChild)
    {
        goto Cleanup;
    }

    if (m_isTabIndexSet)
    {
        if (m_isTopLeftHeaderChild)
        {
            IFC(get_TopLeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && tabIndex == m_tabIndex)
                {
                    IFC(spHeader.As<IDependencyObject>(&spLastFocusable));
                }
            }
        }

        if (m_isTopHeaderChild && !spLastFocusable)
        {
            IFC(get_TopHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && tabIndex == m_tabIndex)
                {
                    IFC(spHeader.As<IDependencyObject>(&spLastFocusable));
                }
            }
        }

        if (m_isLeftHeaderChild && !spLastFocusable)
        {
            IFC(get_LeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && tabIndex == m_tabIndex)
                {
                    IFC(spHeader.As<IDependencyObject>(&spLastFocusable));
                }
            }
        }

        if (!spLastFocusable)
        {
            IFC(get_Content(&spContent));
            spDependencyObject = spContent.AsOrNull<IDependencyObject>();
            if (spDependencyObject)
            {
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && tabIndex == m_tabIndex)
                {
                    spLastFocusable = spDependencyObject;
                }
            }
        }
    }
    else
    {
        maxTabIndex = INT32_MIN;

        IFC(get_Content(&spContent));
        spDependencyObject = spContent.AsOrNull<IDependencyObject>();
        if (spDependencyObject)
        {
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop)
            {
                spLastFocusable = spDependencyObject;
                maxTabIndex = tabIndex;
            }
        }

        if (m_isLeftHeaderChild)
        {
            IFC(get_LeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex > maxTabIndex || !spLastFocusable))
                {
                    IFC(spHeader.As<IDependencyObject>(&spLastFocusable));
                    maxTabIndex = tabIndex;
                }
            }
        }

        if (m_isTopHeaderChild)
        {
            IFC(get_TopHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex > maxTabIndex || !spLastFocusable))
                {
                    IFC(spHeader.As<IDependencyObject>(&spLastFocusable));
                    maxTabIndex = tabIndex;
                }
            }
        }

        if (m_isTopLeftHeaderChild)
        {
            IFC(get_TopLeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex > maxTabIndex || !spLastFocusable))
                {
                    IFC(spHeader.As<IDependencyObject>(&spLastFocusable));
                }
            }
        }
    }

    if (spLastFocusable)
    {
        *ppLastFocusable = static_cast<DependencyObject*>(spLastFocusable.Detach());
    }

Cleanup:
    RRETURN(hr);
}

// Determines if a direct child has a custom TabIndex value set, while TabStop is True.
_Check_return_ HRESULT ScrollContentPresenter::HasDirectChildWithTabIndexSet(
    _Out_ BOOLEAN* pHasDirectChildWithTabIndexSet)
{
    HRESULT hr = S_OK;
    BOOLEAN isTabStop = FALSE;
    INT tabIndex = 0;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IDependencyObject> spDependencyObject;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: HasDirectChildWithTabIndexSet - entry.", this));
    }
#endif // DM_DEBUG

    ASSERT(m_isTopLeftHeaderChild || m_isTopHeaderChild || m_isLeftHeaderChild);
    ASSERT(pHasDirectChildWithTabIndexSet);
    *pHasDirectChildWithTabIndexSet = FALSE;

    if (m_isTopLeftHeaderChild)
    {
        IFC(get_TopLeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop && tabIndex != INT32_MAX)
            {
                *pHasDirectChildWithTabIndexSet = TRUE;
                goto Cleanup;
            }
        }
    }

    if (m_isTopHeaderChild)
    {
        IFC(get_TopHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop && tabIndex != INT32_MAX)
            {
                *pHasDirectChildWithTabIndexSet = TRUE;
                goto Cleanup;
            }
        }
    }

    if (m_isLeftHeaderChild)
    {
        IFC(get_LeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop && tabIndex != INT32_MAX)
            {
                *pHasDirectChildWithTabIndexSet = TRUE;
                goto Cleanup;
            }
        }
    }

    IFC(get_Content(&spContent));
    spDependencyObject = spContent.AsOrNull<IDependencyObject>();
    if (spDependencyObject)
    {
        IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
        if (isTabStop && tabIndex != INT32_MAX)
        {
            *pHasDirectChildWithTabIndexSet = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Handles tab-based navigation when a custom TabIndex value is set for
// a header or the content.
_Check_return_ HRESULT ScrollContentPresenter::ProcessTabStopPrivate(
    _In_opt_ DependencyObject* pFocusedElement,
    const bool isBackward,
    BOOLEAN isFocusedElementInTopLeftHeader,
    BOOLEAN isFocusedElementInTopHeader,
    BOOLEAN isFocusedElementInLeftHeader,
    BOOLEAN isFocusedElementInContent,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsTabStopOverridden)
{
    HRESULT hr = S_OK;
    BOOLEAN isTopLeftHeader = FALSE;
    BOOLEAN isTopHeader = FALSE;
    BOOLEAN isLeftHeader = FALSE;
    BOOLEAN isContent = FALSE;
    CDependencyObject* pTabStopDO = NULL;
    CDependencyObject* pFirstFocusableDO = NULL;
    CDependencyObject* pLastFocusableDO = NULL;
    ctl::ComPtr<DependencyObject> spTabStopDO;
    ctl::ComPtr<IDependencyObject> spChildAsDO;
    ctl::ComPtr<DependencyObject> spFirstFocusableDO;
    ctl::ComPtr<DependencyObject> spLastFocusableDO;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: ProcessTabStopPrivate - entry. pFocusedElement=0x%p, isBackward=%d", this, pFocusedElement, isBackward));
    }
#endif // DM_DEBUG

    ASSERT(m_isTopLeftHeaderChild || m_isTopHeaderChild || m_isLeftHeaderChild);
    ASSERT(ppNewTabStop && *ppNewTabStop == NULL);
    ASSERT(pIsTabStopOverridden && *pIsTabStopOverridden == FALSE);

    if (isBackward)
    {
        // Is pFocusedElement the first tab stop?
        IFC(GetFirstFocusableElementPrivate(
            &isTopLeftHeader,
            &isTopHeader,
            &isLeftHeader,
            &isContent));
        if ((isFocusedElementInTopLeftHeader && isTopLeftHeader) ||
            (isFocusedElementInTopHeader && isTopHeader) ||
            (isFocusedElementInLeftHeader && isLeftHeader) ||
            (isFocusedElementInContent && isContent))
        {
            // The ScrollContentPresenter needs to be exited.
            ASSERT(!pTabStopDO);
            pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetPreviousTabStop(GetHandle());

            if (pTabStopDO)
            {
                IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                IFC(spTabStopDO.MoveTo(ppNewTabStop));
                *pIsTabStopOverridden = TRUE;
            }
        }
        else
        {
            // Must navigation be redirected to another direct child?
            IFC(GetPreviousFocusableElementPrivate(
                isFocusedElementInTopLeftHeader,
                isFocusedElementInTopHeader,
                isFocusedElementInLeftHeader,
                isFocusedElementInContent,
                &isTopLeftHeader,
                &isTopHeader,
                &isLeftHeader,
                &isContent,
                &spChildAsDO /*ppPreviousFocusable*/));
            if (spChildAsDO)
            {
                ASSERT(!pLastFocusableDO);
                IFC(CoreImports::FocusManager_GetLastFocusableElement(
                    static_cast<CDependencyObject*>(spChildAsDO.Cast<DependencyObject>()->GetHandle()),
                    &pLastFocusableDO));
                if (pLastFocusableDO)
                {
                    IFC(DXamlCore::GetCurrent()->GetPeer(pLastFocusableDO, &spLastFocusableDO));
                    IFC(spLastFocusableDO.MoveTo(ppNewTabStop));
                }
                else
                {
                    *ppNewTabStop = static_cast<DependencyObject*>(spChildAsDO.Detach());
                }
                *pIsTabStopOverridden = TRUE;
            }
        }
    }
    else
    {
        // Moving forward. Is pFocusedElement the last tab stop?
        IFC(GetLastFocusableElementPrivate(
            &isTopLeftHeader,
            &isTopHeader,
            &isLeftHeader,
            &isContent));
        if ((isFocusedElementInTopLeftHeader && isTopLeftHeader) ||
            (isFocusedElementInTopHeader && isTopHeader) ||
            (isFocusedElementInLeftHeader && isLeftHeader) ||
            (isFocusedElementInContent && isContent))
        {
            // The ScrollContentPresenter needs to be exited.
            ASSERT(!pTabStopDO);
            pTabStopDO = VisualTree::GetFocusManagerForElement(GetHandle())->GetNextTabStop(GetHandle(), TRUE);

            if (pTabStopDO)
            {
                IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopDO, &spTabStopDO));
                IFC(spTabStopDO.MoveTo(ppNewTabStop));
                *pIsTabStopOverridden = TRUE;
            }
        }
        else
        {
            // Must navigation be redirected to another direct child?
            IFC(GetNextFocusableElementPrivate(
                isFocusedElementInTopLeftHeader,
                isFocusedElementInTopHeader,
                isFocusedElementInLeftHeader,
                isFocusedElementInContent,
                &isTopLeftHeader,
                &isTopHeader,
                &isLeftHeader,
                &isContent,
                &spChildAsDO /*ppNextFocusable*/));
            if (spChildAsDO)
            {
                ASSERT(!pFirstFocusableDO);
                IFC(CoreImports::FocusManager_GetFirstFocusableElement(
                    static_cast<CDependencyObject*>(spChildAsDO.Cast<DependencyObject>()->GetHandle()),
                    &pFirstFocusableDO));
                if (pFirstFocusableDO)
                {
                    IFC(DXamlCore::GetCurrent()->GetPeer(pFirstFocusableDO, &spFirstFocusableDO));
                    IFC(spFirstFocusableDO.MoveTo(ppNewTabStop));
                }
                else
                {
                    *ppNewTabStop = static_cast<DependencyObject*>(spChildAsDO.Detach());
                }
                *pIsTabStopOverridden = TRUE;
            }
        }
    }

Cleanup:
    ReleaseInterface(pTabStopDO);
    ReleaseInterface(pFirstFocusableDO);
    ReleaseInterface(pLastFocusableDO);
    RRETURN(hr);
}

// Determines the location of the first focusable element in scenarios where a custom TabIndex value
// is set for a header or the content.
_Check_return_ HRESULT ScrollContentPresenter::GetFirstFocusableElementPrivate(
    _Out_ BOOLEAN* pIsTopLeftHeader,
    _Out_ BOOLEAN* pIsTopHeader,
    _Out_ BOOLEAN* pIsLeftHeader,
    _Out_ BOOLEAN* pIsContent)
{
    HRESULT hr = S_OK;
    BOOLEAN isTabStop = FALSE;
    INT firstFocusableTabIndex = INT32_MAX;
    INT tabIndex = 0;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IDependencyObject> spDependencyObject;
    ctl::ComPtr<IDependencyObject> spFirstFocusable;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: GetFirstFocusableElementPrivate - entry.", this));
    }
#endif // DM_DEBUG

    ASSERT(m_isTopLeftHeaderChild || m_isTopHeaderChild || m_isLeftHeaderChild);
    ASSERT(pIsTopLeftHeader);
    ASSERT(pIsTopHeader);
    ASSERT(pIsLeftHeader);
    ASSERT(pIsContent);

    *pIsTopLeftHeader = FALSE;
    *pIsTopHeader = FALSE;
    *pIsLeftHeader = FALSE;
    *pIsContent = FALSE;

    IFC(get_Content(&spContent));
    spDependencyObject = spContent.AsOrNull<IDependencyObject>();
    if (spDependencyObject)
    {
        IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
        if (isTabStop)
        {
            ASSERT(!spFirstFocusable.Get());
            firstFocusableTabIndex = tabIndex;
            spFirstFocusable = spDependencyObject;
            *pIsContent = TRUE;
        }
    }

    if (m_isLeftHeaderChild)
    {
        IFC(get_LeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop)
            {
                if (tabIndex <= firstFocusableTabIndex || !spFirstFocusable)
                {
                    firstFocusableTabIndex = tabIndex;
                    IFC(spHeader.As<IDependencyObject>(&spFirstFocusable));
                    *pIsContent = FALSE;
                    *pIsLeftHeader = TRUE;
                }
            }
        }
    }

    if (m_isTopHeaderChild)
    {
        IFC(get_TopHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop)
            {
                if (tabIndex <= firstFocusableTabIndex || !spFirstFocusable)
                {
                    firstFocusableTabIndex = tabIndex;
                    IFC(spHeader.As<IDependencyObject>(&spFirstFocusable));
                    *pIsContent = FALSE;
                    *pIsLeftHeader = FALSE;
                    *pIsTopHeader = TRUE;
                }
            }
        }
    }

    if (m_isTopLeftHeaderChild)
    {
        IFC(get_TopLeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop)
            {
                if (tabIndex <= firstFocusableTabIndex || !spFirstFocusable)
                {
                    *pIsContent = FALSE;
                    *pIsLeftHeader = FALSE;
                    *pIsTopHeader = FALSE;
                    *pIsTopLeftHeader = TRUE;
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Determines the location of the last focusable element in scenarios where a custom TabIndex value
// is set for a header or the content.
_Check_return_ HRESULT ScrollContentPresenter::GetLastFocusableElementPrivate(
    _Out_ BOOLEAN* pIsTopLeftHeader,
    _Out_ BOOLEAN* pIsTopHeader,
    _Out_ BOOLEAN* pIsLeftHeader,
    _Out_ BOOLEAN* pIsContent)
{
    HRESULT hr = S_OK;
    BOOLEAN isTabStop = FALSE;
    INT lastFocusableTabIndex = INT32_MIN;
    INT tabIndex = 0;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IDependencyObject> spDependencyObject;
    ctl::ComPtr<IDependencyObject> spLastFocusable;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: GetLastFocusableElementPrivate - entry.", this));
    }
#endif // DM_DEBUG

    ASSERT(m_isTopLeftHeaderChild || m_isTopHeaderChild || m_isLeftHeaderChild);
    ASSERT(pIsTopLeftHeader);
    ASSERT(pIsTopHeader);
    ASSERT(pIsLeftHeader);
    ASSERT(pIsContent);

    *pIsTopLeftHeader = FALSE;
    *pIsTopHeader = FALSE;
    *pIsLeftHeader = FALSE;
    *pIsContent = FALSE;

    if (m_isTopLeftHeaderChild)
    {
        IFC(get_TopLeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop)
            {
                ASSERT(!spLastFocusable.Get());
                lastFocusableTabIndex = tabIndex;
                IFC(spHeader.As<IDependencyObject>(&spLastFocusable));
                *pIsTopLeftHeader = TRUE;
            }
        }
    }

    if (m_isTopHeaderChild)
    {
        IFC(get_TopHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop)
            {
                if (tabIndex >= lastFocusableTabIndex || !spLastFocusable)
                {
                    lastFocusableTabIndex = tabIndex;
                    IFC(spHeader.As<IDependencyObject>(&spLastFocusable));
                    *pIsTopLeftHeader = FALSE;
                    *pIsTopHeader = TRUE;
                }
            }
        }
    }

    if (m_isLeftHeaderChild)
    {
        IFC(get_LeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
            IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
            if (isTabStop)
            {
                if (tabIndex >= lastFocusableTabIndex || !spLastFocusable)
                {
                    lastFocusableTabIndex = tabIndex;
                    IFC(spHeader.As<IDependencyObject>(&spLastFocusable));
                    *pIsTopLeftHeader = FALSE;
                    *pIsTopHeader = FALSE;
                    *pIsLeftHeader = TRUE;
                }
            }
        }
    }

    IFC(get_Content(&spContent));
    spDependencyObject = spContent.AsOrNull<IDependencyObject>();
    if (spDependencyObject)
    {
        IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
        if (isTabStop)
        {
            if (tabIndex >= lastFocusableTabIndex || !spLastFocusable)
            {
                *pIsTopLeftHeader = FALSE;
                *pIsTopHeader = FALSE;
                *pIsLeftHeader = FALSE;
                *pIsContent = TRUE;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Determines the next focusable element among the headers and content for scenarios
// that involve a custom TabIndex value.
_Check_return_ HRESULT ScrollContentPresenter::GetNextFocusableElementPrivate(
    _In_ BOOLEAN isFocusedElementInTopLeftHeader,
    _In_ BOOLEAN isFocusedElementInTopHeader,
    _In_ BOOLEAN isFocusedElementInLeftHeader,
    _In_ BOOLEAN isFocusedElementInContent,
    _Out_ BOOLEAN* pIsTopLeftHeader,
    _Out_ BOOLEAN* pIsTopHeader,
    _Out_ BOOLEAN* pIsLeftHeader,
    _Out_ BOOLEAN* pIsContent,
    _Out_ ctl::ComPtr<IDependencyObject>* pspNextFocusable)
{
    HRESULT hr = S_OK;
    BOOLEAN isTabStop = FALSE;
    INT focusedTabIndex = 0;
    INT nextFocusableTabIndex = 0;
    INT tabIndex = 0;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IDependencyObject> spDependencyObject;
    ctl::ComPtr<IDependencyObject> spNextFocusable;
    ctl::ComPtr<IDependencyObject> spFocusedDirectChild;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: GetNextFocusableElementPrivate isFocusedElementInTopLeftHeader=%d, isFocusedElementInTopHeader=%d, isFocusedElementInLeftHeader=%d, isFocusedElementInContent=%d.",
            this, isFocusedElementInTopLeftHeader, isFocusedElementInTopHeader, isFocusedElementInLeftHeader, isFocusedElementInContent));
    }
#endif // DM_DEBUG

    ASSERT(m_isTopLeftHeaderChild || m_isTopHeaderChild || m_isLeftHeaderChild);
    ASSERT(isFocusedElementInTopLeftHeader || isFocusedElementInTopHeader || isFocusedElementInLeftHeader || isFocusedElementInContent);
    ASSERT(pIsTopLeftHeader);
    ASSERT(pIsTopHeader);
    ASSERT(pIsLeftHeader);
    ASSERT(pIsContent);
    ASSERT(pspNextFocusable);

    *pIsTopLeftHeader = FALSE;
    *pIsTopHeader = FALSE;
    *pIsLeftHeader = FALSE;
    *pIsContent = FALSE;
    *pspNextFocusable = nullptr;

    IFC(GetDirectChild(
        isFocusedElementInTopLeftHeader /*topLeftHeader*/,
        isFocusedElementInTopHeader /*topHeader*/,
        isFocusedElementInLeftHeader /*leftHeader*/,
        isFocusedElementInContent /*content*/,
        &spFocusedDirectChild));

    if (spFocusedDirectChild)
    {
        IFC(spFocusedDirectChild.As<IDependencyObject>(&spDependencyObject));
        IFC(GetTabIndex(spDependencyObject, &isTabStop, &focusedTabIndex));

        // Next focusable element
        // - has the smallest tab index >= focusedTabIndex
        // - when multiple elements have the same smallest tab index, top/left header wins over top header, etc...
        // - only elements with tab index == focusedTabIndex placed after the focused element are eligible
        if (!isFocusedElementInContent)
        {
            IFC(get_Content(&spContent));
            spDependencyObject = spContent.AsOrNull<IDependencyObject>();
            if (spDependencyObject)
            {
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                ASSERT(!spNextFocusable.Get());
                if (isTabStop && tabIndex >= focusedTabIndex)
                {
                    nextFocusableTabIndex = tabIndex;
                    spNextFocusable = spDependencyObject;
                    *pIsContent = TRUE;
                }
            }
        }

        if (!isFocusedElementInLeftHeader && m_isLeftHeaderChild)
        {
            IFC(get_LeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex <= nextFocusableTabIndex || !spNextFocusable))
                {
                    if (tabIndex > focusedTabIndex || (tabIndex == focusedTabIndex && (isFocusedElementInTopLeftHeader || isFocusedElementInTopHeader)))
                    {
                        nextFocusableTabIndex = tabIndex;
                        IFC(spHeader.As<IDependencyObject>(&spNextFocusable));
                        *pIsContent = FALSE;
                        *pIsLeftHeader = TRUE;
                    }
                }
            }
        }

        if (!isFocusedElementInTopHeader && m_isTopHeaderChild)
        {
            IFC(get_TopHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex <= nextFocusableTabIndex || !spNextFocusable))
                {
                    if (tabIndex > focusedTabIndex || (tabIndex == focusedTabIndex && isFocusedElementInTopLeftHeader))
                    {
                        nextFocusableTabIndex = tabIndex;
                        IFC(spHeader.As<IDependencyObject>(&spNextFocusable));
                        *pIsContent = FALSE;
                        *pIsLeftHeader = FALSE;
                        *pIsTopHeader = TRUE;
                    }
                }
            }
        }

        if (!isFocusedElementInTopLeftHeader && m_isTopLeftHeaderChild)
        {
            IFC(get_TopLeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex <= nextFocusableTabIndex || !spNextFocusable))
                {
                    if (tabIndex > focusedTabIndex)
                    {
                        IFC(spHeader.As<IDependencyObject>(&spNextFocusable));
                        *pIsContent = FALSE;
                        *pIsLeftHeader = FALSE;
                        *pIsTopHeader = FALSE;
                        *pIsTopLeftHeader = TRUE;
                    }
                }
            }
        }

        if (spNextFocusable)
        {
            *pspNextFocusable = std::move(spNextFocusable);
        }
    }

Cleanup:
    RRETURN(hr);
}

// Determines the previous focusable element among the headers and content for scenarios
// that involve a custom TabIndex value.
_Check_return_ HRESULT ScrollContentPresenter::GetPreviousFocusableElementPrivate(
    _In_ BOOLEAN isFocusedElementInTopLeftHeader,
    _In_ BOOLEAN isFocusedElementInTopHeader,
    _In_ BOOLEAN isFocusedElementInLeftHeader,
    _In_ BOOLEAN isFocusedElementInContent,
    _Out_ BOOLEAN* pIsTopLeftHeader,
    _Out_ BOOLEAN* pIsTopHeader,
    _Out_ BOOLEAN* pIsLeftHeader,
    _Out_ BOOLEAN* pIsContent,
    _Out_ ctl::ComPtr<IDependencyObject>* pspPreviousFocusable)
{
    HRESULT hr = S_OK;
    BOOLEAN isTabStop = FALSE;
    INT focusedTabIndex = 0;
    INT previousFocusableTabIndex = 0;
    INT tabIndex = 0;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IDependencyObject> spDependencyObject;
    ctl::ComPtr<IDependencyObject> spPreviousFocusable;
    ctl::ComPtr<IDependencyObject> spFocusedDirectChild;

#ifdef DM_DEBUG
    if (DMSCP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLCONTENTPRESENTER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLCONTENTPRESENTER | DMSCP_DBG) /*traceType*/,
            L"DMSCP[0x%p]: GetPreviousFocusableElementPrivate isFocusedElementInTopLeftHeader=%d, isFocusedElementInTopHeader=%d, isFocusedElementInLeftHeader=%d, isFocusedElementInContent=%d.",
            this, isFocusedElementInTopLeftHeader, isFocusedElementInTopHeader, isFocusedElementInLeftHeader, isFocusedElementInContent));
    }
#endif // DM_DEBUG

    ASSERT(m_isTopLeftHeaderChild || m_isTopHeaderChild || m_isLeftHeaderChild);
    ASSERT(isFocusedElementInTopLeftHeader || isFocusedElementInTopHeader || isFocusedElementInLeftHeader || isFocusedElementInContent);
    ASSERT(pIsTopLeftHeader);
    ASSERT(pIsTopHeader);
    ASSERT(pIsLeftHeader);
    ASSERT(pIsContent);
    ASSERT(pspPreviousFocusable);

    *pIsTopLeftHeader = FALSE;
    *pIsTopHeader = FALSE;
    *pIsLeftHeader = FALSE;
    *pIsContent = FALSE;
    *pspPreviousFocusable = nullptr;

    IFC(GetDirectChild(
        isFocusedElementInTopLeftHeader /*topLeftHeader*/,
        isFocusedElementInTopHeader /*topHeader*/,
        isFocusedElementInLeftHeader /*leftHeader*/,
        isFocusedElementInContent /*content*/,
        &spFocusedDirectChild));

    if (spFocusedDirectChild)
    {
        IFC(spFocusedDirectChild.As<IDependencyObject>(&spDependencyObject));
        IFC(GetTabIndex(spDependencyObject, &isTabStop, &focusedTabIndex));

        // Previous focusable element
        // - has the largest tab index <= focusedTabIndex
        // - when multiple elements have the same largest tab index, top header wins over top/left header, etc...
        // - only elements with tab index == focusedTabIndex placed before the focused element are eligible
        if (!isFocusedElementInTopLeftHeader && m_isTopLeftHeaderChild)
        {
            IFC(get_TopLeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                ASSERT(!spPreviousFocusable.Get());
                if (isTabStop && tabIndex <= focusedTabIndex)
                {
                    previousFocusableTabIndex = tabIndex;
                    IFC(spHeader.As<IDependencyObject>(&spPreviousFocusable));
                    *pIsTopLeftHeader = TRUE;
                }
            }
        }

        if (!isFocusedElementInTopHeader && m_isTopHeaderChild)
        {
            IFC(get_TopHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex >= previousFocusableTabIndex || !spPreviousFocusable))
                {
                    if (tabIndex < focusedTabIndex || (tabIndex == focusedTabIndex && (isFocusedElementInLeftHeader || isFocusedElementInContent)))
                    {
                        previousFocusableTabIndex = tabIndex;
                        IFC(spHeader.As<IDependencyObject>(&spPreviousFocusable));
                        *pIsTopLeftHeader = FALSE;
                        *pIsTopHeader = TRUE;
                    }
                }
            }
        }

        if (!isFocusedElementInLeftHeader && m_isLeftHeaderChild)
        {
            IFC(get_LeftHeader(&spHeader));
            if (spHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spDependencyObject));
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex >= previousFocusableTabIndex || !spPreviousFocusable))
                {
                    if (tabIndex < focusedTabIndex || (tabIndex == focusedTabIndex && isFocusedElementInContent))
                    {
                        previousFocusableTabIndex = tabIndex;
                        IFC(spHeader.As<IDependencyObject>(&spPreviousFocusable));
                        *pIsTopLeftHeader = FALSE;
                        *pIsTopHeader = FALSE;
                        *pIsLeftHeader = TRUE;
                    }
                }
            }
        }

        if (!isFocusedElementInContent)
        {
            IFC(get_Content(&spContent));
            spDependencyObject = spContent.AsOrNull<IDependencyObject>();
            if (spDependencyObject)
            {
                IFC(GetTabIndex(spDependencyObject, &isTabStop, &tabIndex));
                if (isTabStop && (tabIndex >= previousFocusableTabIndex || !spPreviousFocusable))
                {
                    if (tabIndex < focusedTabIndex)
                    {
                        spPreviousFocusable = spDependencyObject;
                        *pIsTopLeftHeader = FALSE;
                        *pIsTopHeader = FALSE;
                        *pIsLeftHeader = FALSE;
                        *pIsContent = TRUE;
                    }
                }
            }
        }

        if (spPreviousFocusable)
        {
            *pspPreviousFocusable = std::move(spPreviousFocusable);
        }
    }

Cleanup:
    RRETURN(hr);
}

// Determines where the currently focused element and new candidate are in
// relation to the headers and content.
_Check_return_ HRESULT ScrollContentPresenter::AnalyzeTabbingElements(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_opt_ DependencyObject* pCandidateTabStopElement,
    _Out_ BOOLEAN* pIsFocusedElementInTopLeftHeader,
    _Out_ BOOLEAN* pIsFocusedElementInTopHeader,
    _Out_ BOOLEAN* pIsFocusedElementInLeftHeader,
    _Out_ BOOLEAN* pIsFocusedElementInContent,
    _Out_ BOOLEAN* pIsCandidateElementInTopLeftHeader,
    _Out_ BOOLEAN* pIsCandidateElementInTopHeader,
    _Out_ BOOLEAN* pIsCandidateElementInLeftHeader,
    _Out_ BOOLEAN* pIsCandidateElementInContent)
{
    HRESULT hr = S_OK;
    BOOLEAN isElementDirectChild = FALSE;

    ASSERT(pIsFocusedElementInTopLeftHeader);
    ASSERT(pIsFocusedElementInTopHeader);
    ASSERT(pIsFocusedElementInLeftHeader);
    ASSERT(pIsFocusedElementInContent);
    ASSERT(pIsCandidateElementInTopLeftHeader);
    ASSERT(pIsCandidateElementInTopHeader);
    ASSERT(pIsCandidateElementInLeftHeader);
    ASSERT(pIsCandidateElementInContent);

    *pIsFocusedElementInTopLeftHeader = FALSE;
    *pIsFocusedElementInTopHeader = FALSE;
    *pIsFocusedElementInLeftHeader = FALSE;
    *pIsFocusedElementInContent = FALSE;
    *pIsCandidateElementInTopLeftHeader = FALSE;
    *pIsCandidateElementInTopHeader = FALSE;
    *pIsCandidateElementInLeftHeader = FALSE;
    *pIsCandidateElementInContent = FALSE;

    if (pFocusedElement)
    {
        IFC(GetHeaderOwnership(
            pFocusedElement,
            &isElementDirectChild,
            pIsFocusedElementInTopLeftHeader,
            pIsFocusedElementInTopHeader,
            pIsFocusedElementInLeftHeader,
            pIsFocusedElementInContent));
    }

    if (pCandidateTabStopElement)
    {
        IFC(GetHeaderOwnership(
            pCandidateTabStopElement,
            &isElementDirectChild,
            pIsCandidateElementInTopLeftHeader,
            pIsCandidateElementInTopHeader,
            pIsCandidateElementInLeftHeader,
            pIsCandidateElementInContent));
    }

Cleanup:
    RRETURN(hr);
}

// Returns the owning quadrant of the provided element and whether or not it is a direct child.
_Check_return_ HRESULT ScrollContentPresenter::GetHeaderOwnership(
    _In_ DependencyObject* pElement,
    _Out_ BOOLEAN* pIsElementDirectChild,
    _Out_ BOOLEAN* pIsElementInTopLeftHeader,
    _Out_ BOOLEAN* pIsElementInTopHeader,
    _Out_ BOOLEAN* pIsElementInLeftHeader,
    _Out_ BOOLEAN* pIsElementInContent)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spHeaderAsDO;
    ctl::ComPtr<IDependencyObject> spContentAsDO;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IUIElement> spContentAsUIElement;
    ctl::ComPtr<IInspectable> spContent;

    ASSERT(pElement);
    ASSERT(pIsElementDirectChild);
    ASSERT(pIsElementInTopLeftHeader);
    ASSERT(pIsElementInTopHeader);
    ASSERT(pIsElementInLeftHeader);
    ASSERT(pIsElementInContent);

    *pIsElementDirectChild = FALSE;
    *pIsElementInTopLeftHeader = FALSE;
    *pIsElementInTopHeader = FALSE;
    *pIsElementInLeftHeader = FALSE;
    *pIsElementInContent = FALSE;

    if (m_isTopLeftHeaderChild)
    {
        IFC(get_TopLeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.Cast<UIElement>()->IsAncestorOf(pElement, pIsElementInTopLeftHeader));
            if (*pIsElementInTopLeftHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spHeaderAsDO));
                *pIsElementDirectChild = (pElement == spHeaderAsDO.Cast<DependencyObject>());
                goto Cleanup;
            }
        }
    }
    if (m_isTopHeaderChild)
    {
        IFC(get_TopHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.Cast<UIElement>()->IsAncestorOf(pElement, pIsElementInTopHeader));
            if (*pIsElementInTopHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spHeaderAsDO));
                *pIsElementDirectChild = (pElement == spHeaderAsDO.Cast<DependencyObject>());
                goto Cleanup;
            }
        }
    }
    if (m_isLeftHeaderChild)
    {
        IFC(get_LeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(spHeader.Cast<UIElement>()->IsAncestorOf(pElement, pIsElementInLeftHeader));
            if (*pIsElementInLeftHeader)
            {
                IFC(spHeader.As<IDependencyObject>(&spHeaderAsDO));
                *pIsElementDirectChild = (pElement == spHeaderAsDO.Cast<DependencyObject>());
                goto Cleanup;
            }
        }
    }

    IFC(get_Content(&spContent));
    spContentAsUIElement = spContent.AsOrNull<IUIElement>();
    if (spContentAsUIElement)
    {
        IFC(spContentAsUIElement.Cast<UIElement>()->IsAncestorOf(pElement, pIsElementInContent));
        if (*pIsElementInContent)
        {
            IFC(spContentAsUIElement.As<IDependencyObject>(&spContentAsDO));
            *pIsElementDirectChild = (pElement == spContentAsDO.Cast<DependencyObject>());
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Returns the requested direct child as a dependency object.
_Check_return_ HRESULT ScrollContentPresenter::GetDirectChild(
    _In_ BOOLEAN topLeftHeader,
    _In_ BOOLEAN topHeader,
    _In_ BOOLEAN leftHeader,
    _In_ BOOLEAN content,
    _Out_ ctl::ComPtr<IDependencyObject>* pspChild)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spChildAsDO;
    ctl::ComPtr<IUIElement> spChildAsUIE;
    ctl::ComPtr<IInspectable> spContent;

    ASSERT(pspChild);
    *pspChild = nullptr;

    if (topLeftHeader || topHeader || leftHeader)
    {
        ASSERT(!content);
        ASSERT(!(topLeftHeader && topHeader));
        ASSERT(!(topLeftHeader && leftHeader));
        ASSERT(!(topHeader && leftHeader));

        if (topLeftHeader)
        {
            IFC(get_TopLeftHeader(&spChildAsUIE));
        }
        else if (topHeader)
        {
            IFC(get_TopHeader(&spChildAsUIE));
        }
        else if (leftHeader)
        {
            IFC(get_LeftHeader(&spChildAsUIE));
        }
        spChildAsDO = spChildAsUIE.AsOrNull<IDependencyObject>();
        *pspChild = std::move(spChildAsDO);
    }
    else if (content)
    {
        IFC(get_Content(&spContent));
        spChildAsDO = spContent.AsOrNull<IDependencyObject>();
        *pspChild = std::move(spChildAsDO);
    }

Cleanup:
    RRETURN(hr);
}

// Determines if an element is focusable or has a focusable child.
_Check_return_ HRESULT ScrollContentPresenter::GetTabIndex(
    _In_ ctl::ComPtr<IDependencyObject> spElement,
    _Out_opt_ BOOLEAN* pIsTabStop,
    _Out_ INT* pTabIndex)
{
    HRESULT hr = S_OK;
    BOOLEAN isEnabled = FALSE;
    CDependencyObject* pFirstFocusableDO = NULL;
    ctl::ComPtr<IControl> spControl;
    ctl::ComPtr<IUIElement> spUIElement;
    ctl::ComPtr<IHyperlink> spHyperlink;

    ASSERT(spElement.Get());
    ASSERT(pTabIndex);

    if (pIsTabStop)
    {
        *pIsTabStop = FALSE;
    }
    *pTabIndex = INT32_MAX;

    spUIElement = spElement.AsOrNull<IUIElement>();
    spControl = spElement.AsOrNull<IControl>();
    if (spUIElement)
    {
        if (pIsTabStop && spControl)
        {
            IFC(spControl->get_IsEnabled(&isEnabled));
            if (isEnabled)
            {
                IFC(spUIElement->get_IsTabStop(pIsTabStop));
            }
        }
        IFC(spUIElement->get_TabIndex(pTabIndex));
    }
    else
    {
        spHyperlink = spElement.AsOrNull<IHyperlink>();
        if (spHyperlink)
        {
            IFC(spHyperlink->get_IsTabStop(pIsTabStop));
            IFC(spHyperlink->get_TabIndex(pTabIndex));
        }
        else if (pIsTabStop)
        {
            // The provided element is not a Control with a IsTabStop property but may have a child with one set to True.
            IFC(CoreImports::FocusManager_GetFirstFocusableElement(
                static_cast<CDependencyObject*>(spElement.Cast<DependencyObject>()->GetHandle()),
                &pFirstFocusableDO));
            *pIsTabStop = (pFirstFocusableDO != NULL);
        }
    }

Cleanup:
    ReleaseInterface(pFirstFocusableDO);
    RRETURN(hr);
}

#ifdef DBG

// Traces value of all ScrollData fields for debugging purposes
void ScrollContentPresenter::TraceScrollData()
{
    ScrollData* pScrollData = NULL;
    get_ScrollData(&pScrollData);

    if (pScrollData)
    {
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG, L"SCP[0x%p]:   TraceScrollData pScrollData:", this));

        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG,
            L"                   m_viewport.Width=%f m_extent.Width=%f m_Offset.X=%f m_ComputedOffset.X=%f m_canHorizontallyScroll=%d.",
            pScrollData->m_viewport.Width,
            pScrollData->m_extent.Width,
            pScrollData->get_OffsetX(),
            pScrollData->m_ComputedOffset.X,
            pScrollData->m_canHorizontallyScroll));

        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG,
            L"                   m_viewport.Height=%f m_extent.Height=%f m_Offset.Y=%f m_ComputedOffset.Y=%f m_canVerticallyScroll=%d.",
            pScrollData->m_viewport.Height,
            pScrollData->m_extent.Height,
            pScrollData->get_OffsetY(),
            pScrollData->m_ComputedOffset.Y,
            pScrollData->m_canVerticallyScroll));
    }
}

#endif // DBG

// Enters the mode where the child's actual size is used for
// the extent exposed through IScrollInfo.
void ScrollContentPresenter::StartUseOfActualWidthAsExtent()
{
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    ctl::ComPtr<IScrollViewer> spScrollViewer;

    ASSERT(!m_isChildActualWidthUsedAsExtent);
    m_isChildActualWidthUsedAsExtent = true;

    IFCFAILFAST(get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner != nullptr)
    {
        IFCFAILFAST(spScrollOwner.As(&spScrollViewer));
        if (spScrollViewer != nullptr)
        {
            spScrollViewer.Cast<ScrollViewer>()->StartUseOfActualSizeAsExtent(true /*isHorizontal*/);
        }
    }
}

// Leaves the mode where the child's actual size is used for
// the extent exposed through IScrollInfo.
void ScrollContentPresenter::StopUseOfActualWidthAsExtent()
{
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    ctl::ComPtr<IScrollViewer> spScrollViewer;

    ASSERT(m_isChildActualWidthUsedAsExtent);
    m_unpublishedExtentSize.Width = 0.0f;
    m_isChildActualWidthUsedAsExtent = false;

    IFCFAILFAST(get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner != nullptr)
    {
        IFCFAILFAST(spScrollOwner.As(&spScrollViewer));
        if (spScrollViewer != nullptr)
        {
            spScrollViewer.Cast<ScrollViewer>()->StopUseOfActualSizeAsExtent(true /*isHorizontal*/);
        }
    }
}

// Enters the mode where the child's actual size is used for
// the extent exposed through IScrollInfo.
void ScrollContentPresenter::StartUseOfActualHeightAsExtent()
{
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    ctl::ComPtr<IScrollViewer> spScrollViewer;

    ASSERT(!m_isChildActualHeightUsedAsExtent);
    m_isChildActualHeightUsedAsExtent = true;

    IFCFAILFAST(get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner != nullptr)
    {
        IFCFAILFAST(spScrollOwner.As(&spScrollViewer));
        if (spScrollViewer != nullptr)
        {
            spScrollViewer.Cast<ScrollViewer>()->StartUseOfActualSizeAsExtent(false /*isHorizontal*/);
        }
    }
}

// Leaves the mode where the child's actual size is used for
// the extent exposed through IScrollInfo.
void ScrollContentPresenter::StopUseOfActualHeightAsExtent()
{
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    ctl::ComPtr<IScrollViewer> spScrollViewer;

    ASSERT(m_isChildActualHeightUsedAsExtent);
    m_unpublishedExtentSize.Height = 0.0f;
    m_isChildActualHeightUsedAsExtent = false;

    IFCFAILFAST(get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner != nullptr)
    {
        IFCFAILFAST(spScrollOwner.As(&spScrollViewer));
        if (spScrollViewer != nullptr)
        {
            spScrollViewer.Cast<ScrollViewer>()->StopUseOfActualSizeAsExtent(false /*isHorizontal*/);
        }
    }
}

// Stores the ScrollContentPresenter's old and new viewport/extent sizes, as well as the offsets when the layout
// iterations get close to the 250 limit in order to ease layout cycles' debugging (that involve a ScrollViewer).
void ScrollContentPresenter::StoreLayoutCycleWarningContext(
    float oldViewportWidth,
    float oldViewportHeight,
    float oldExtentWidth,
    float oldExtentHeight,
    _In_ ScrollData* scrollData)
{
    CUIElement* scrollContentPresenterAsCUIElement = static_cast<CUIElement*>(GetHandle());

    if (!scrollContentPresenterAsCUIElement || !scrollContentPresenterAsCUIElement->StoreLayoutCycleWarningContexts())
    {
        return;
    }

    std::vector<std::wstring> warningInfo;

    std::wstring oldScrollData(L"Old ScrollData: Viewport: ");
    oldScrollData.append(std::to_wstring(oldViewportWidth));
    oldScrollData.append(L"x");
    oldScrollData.append(std::to_wstring(oldViewportHeight));
    oldScrollData.append(L", Extent: ");
    oldScrollData.append(std::to_wstring(oldExtentWidth));
    oldScrollData.append(L"x");
    oldScrollData.append(std::to_wstring(oldExtentHeight));
    warningInfo.push_back(std::move(oldScrollData));

    std::wstring newScrollData(L"New ScrollData: Viewport: ");
    newScrollData.append(std::to_wstring(scrollData->m_viewport.Width));
    newScrollData.append(L"x");
    newScrollData.append(std::to_wstring(scrollData->m_viewport.Height));
    newScrollData.append(L", Extent: ");
    newScrollData.append(std::to_wstring(scrollData->m_extent.Width));
    newScrollData.append(L"x");
    newScrollData.append(std::to_wstring(scrollData->m_extent.Height));
    warningInfo.push_back(std::move(newScrollData));

    std::wstring offset(L"Offset: ");
    offset.append(std::to_wstring(scrollData->get_OffsetX()));
    offset.append(L"x");
    offset.append(std::to_wstring(scrollData->get_OffsetY()));
    warningInfo.push_back(std::move(offset));

    std::wstring childActualSizeInfo(L"IsChildActualSizeUsedAsExtent: ");
    childActualSizeInfo.append(std::to_wstring(m_isChildActualWidthUsedAsExtent));
    childActualSizeInfo.append(L",");
    childActualSizeInfo.append(std::to_wstring(m_isChildActualHeightUsedAsExtent));
    childActualSizeInfo.append(L", IsChildActualSizeUpdated: ");
    childActualSizeInfo.append(std::to_wstring(m_isChildActualWidthUpdated));
    childActualSizeInfo.append(L",");
    childActualSizeInfo.append(std::to_wstring(m_isChildActualHeightUpdated));
    warningInfo.push_back(std::move(childActualSizeInfo));

    scrollContentPresenterAsCUIElement->StoreLayoutCycleWarningContext(warningInfo, nullptr /*layoutManager*/, DEFAULT_WARNING_FRAMES_TO_SKIP + 1);

    if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Low))
    {
        __debugbreak();
    }
}
