// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBoxView.g.h"
#include "ScrollViewer.g.h"
#include "TextBoxView.h"

#undef min
#undef max

using namespace DirectUI;
using namespace xaml;
using namespace xaml_controls;

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
TextBoxView::TextBoxView() :
    m_pScrollOwnerWeakRef(NULL)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by an instance.
//
//---------------------------------------------------------------------------
TextBoxView::~TextBoxView()
{
    if (m_pScrollOwnerWeakRef != NULL)
    {
        IScrollViewer* pScrollViewer = NULL;
        VERIFYHR(ctl::resolve_weakref(m_pScrollOwnerWeakRef, pScrollViewer));
        if (pScrollViewer != NULL)
        {
            VERIFYHR(static_cast<ScrollViewer*>(pScrollViewer)->SetDirectManipulationStateChangeHandler(NULL));
        }
        ReleaseInterface(pScrollViewer);
        ReleaseInterface(m_pScrollOwnerWeakRef);
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the vertical scrolling enabled status.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_CanVerticallyScrollImpl(_Out_ BOOLEAN* pCanVerticallyScroll)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;

    CoreImports::TextBoxView_GetScrollData(static_cast<CTextBoxView*>(GetHandle()), &pScrollData);

    *pCanVerticallyScroll = pScrollData->CanVerticallyScroll;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, sets the vertical scrolling enabled status.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::put_CanVerticallyScrollImpl(_In_ BOOLEAN canVerticallyScroll)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;
    CTextBoxView* pTextBoxView = static_cast<CTextBoxView*>(GetHandle());

    CoreImports::TextBoxView_GetScrollData(pTextBoxView, &pScrollData);
    CoreImports::TextBoxView_SetScrollEnabled(pTextBoxView, pScrollData->CanHorizontallyScroll, !!canVerticallyScroll);

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the horizontal scrolling enabled status.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_CanHorizontallyScrollImpl(_Out_ BOOLEAN* pCanHorizontallyScroll)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;

    CoreImports::TextBoxView_GetScrollData(static_cast<CTextBoxView*>(GetHandle()), &pScrollData);

    *pCanHorizontallyScroll = pScrollData->CanHorizontallyScroll;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, sets the horizontal scrolling enabled status.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::put_CanHorizontallyScrollImpl(_In_ BOOLEAN canHorizontallyScroll)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;
    CTextBoxView* pTextBoxView = static_cast<CTextBoxView*>(GetHandle());

    CoreImports::TextBoxView_GetScrollData(pTextBoxView, &pScrollData);
    CoreImports::TextBoxView_SetScrollEnabled(pTextBoxView, !!canHorizontallyScroll, pScrollData->CanVerticallyScroll);

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the content width.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_ExtentWidthImpl(_Out_ DOUBLE* pExtentWidth)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;

    CoreImports::TextBoxView_GetScrollData(static_cast<CTextBoxView* >(GetHandle()), &pScrollData);

    *pExtentWidth = pScrollData->ExtentWidth;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the content height.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_ExtentHeightImpl(_Out_ DOUBLE* pExtentHeight)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;

    CoreImports::TextBoxView_GetScrollData(static_cast<CTextBoxView*>(GetHandle()), &pScrollData);

    *pExtentHeight = pScrollData->ExtentHeight;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the viewport width.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_ViewportWidthImpl(_Out_ DOUBLE* pViewportWidth)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;

    CoreImports::TextBoxView_GetScrollData(static_cast<CTextBoxView*>(GetHandle()), &pScrollData);

    *pViewportWidth = pScrollData->ViewportWidth;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, sets the viewport width.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_ViewportHeightImpl(_Out_ DOUBLE* pViewportHeight)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;

    CoreImports::TextBoxView_GetScrollData(static_cast<CTextBoxView*>(GetHandle()), &pScrollData);

    *pViewportHeight = pScrollData->ViewportHeight;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the horizontal offsets of the viewport.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_HorizontalOffsetImpl(_Out_ DOUBLE* pHorizontalOffset)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;
    xaml::FlowDirection scrollOwnerFlowDirection = xaml::FlowDirection_LeftToRight;

    IFC(GetScrollOwnerFlowDirection(&scrollOwnerFlowDirection));

    CoreImports::TextBoxView_GetScrollData(static_cast<CTextBoxView*>(GetHandle()), &pScrollData);
    // TextBoxView always stays in LTR mode since the hosted RichEdit control does not support
    // the same FlowDirection construct as Jupiter. However, ScrollViewer is in RTL mode. Do
    // the conversion between LTR and RTL in this IScrollInfo interop layer.
    if (scrollOwnerFlowDirection == FlowDirection_RightToLeft)
    {
        *pHorizontalOffset = std::max<DOUBLE>(0.0, pScrollData->ExtentWidth - pScrollData->HorizontalOffset - pScrollData->ViewportWidth);
    }
    else
    {
        *pHorizontalOffset = pScrollData->HorizontalOffset;
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the vertical offsets of the viewport.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_VerticalOffsetImpl(_Out_ DOUBLE* pVerticalOffset)
{
    HRESULT hr = S_OK;
    const CTextBoxView_ScrollData* pScrollData = NULL;

    CoreImports::TextBoxView_GetScrollData(static_cast<CTextBoxView*>(GetHandle()), &pScrollData);

    *pVerticalOffset = pScrollData->VerticalOffset;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the minimal horizontal offsets of the viewport.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_MinHorizontalOffsetImpl(_Out_ DOUBLE* pMinHorizontalOffset)
{
    HRESULT hr = S_OK;

    *pMinHorizontalOffset = 0.0;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the minimal vertical offsets of the viewport.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_MinVerticalOffsetImpl(_Out_ DOUBLE* pMinVerticalOffset)
{
    HRESULT hr = S_OK;

    *pMinVerticalOffset = 0.0;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, gets the containing scroll owner.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_ScrollOwnerImpl(_Outptr_ IInspectable** ppScrollOwner)
{
    HRESULT hr = S_OK;
    IFC(ctl::resolve_weakref(m_pScrollOwnerWeakRef, *ppScrollOwner));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, sets the containing scroll owner.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::put_ScrollOwnerImpl(_In_opt_ IInspectable* pScrollOwner)
{
    HRESULT hr = S_OK;
    IScrollViewer* pScrollViewer = NULL;
    IWeakReference* pOldOwnerRef = m_pScrollOwnerWeakRef;

    if (pOldOwnerRef != NULL)
    {
        IFC(ctl::resolve_weakref(pOldOwnerRef, pScrollViewer));
        if (pScrollViewer != NULL)
        {
            IFC(static_cast<ScrollViewer*>(pScrollViewer)->SetDirectManipulationStateChangeHandler(NULL));
        }
        ReleaseInterface(pScrollViewer);
    }

    if (pScrollOwner)
    {
        IFC(ctl::as_weakref(m_pScrollOwnerWeakRef, pScrollOwner));
        IFC(ctl::do_query_interface(pScrollViewer, pScrollOwner));
        IFC(static_cast<ScrollViewer*>(pScrollViewer)->SetDirectManipulationStateChangeHandler(static_cast<DirectManipulationStateChangeHandler*>(this)));
    }

Cleanup:
    ReleaseInterface(pOldOwnerRef);
    ReleaseInterface(pScrollViewer);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport up by one line.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::LineUpImpl()
{
    RRETURN(TextBoxViewScroll(CTextBoxView_ScrollCommand::LineUp));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport down by one line.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::LineDownImpl()
{
    RRETURN(TextBoxViewScroll(CTextBoxView_ScrollCommand::LineDown));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport left by one line.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::LineLeftImpl()
{
    HRESULT hr = S_OK;
    xaml::FlowDirection scrollOwnerFlowDirection = xaml::FlowDirection_LeftToRight;
    CTextBoxView_ScrollCommand scrollCommand = CTextBoxView_ScrollCommand::LineLeft;

    // TextBoxView always stays in LTR mode since the hosted RichEdit control does not support
    // the same FlowDirection construct as Jupiter. However, ScrollViewer is in RTL mode. Do
    // the conversion between LTR and RTL in this IScrollInfo interop layer.
    IFC(GetScrollOwnerFlowDirection(&scrollOwnerFlowDirection));
    scrollCommand = scrollOwnerFlowDirection == xaml::FlowDirection_LeftToRight ? CTextBoxView_ScrollCommand::LineLeft : CTextBoxView_ScrollCommand::LineRight;
    IFC(TextBoxViewScroll(scrollCommand));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport right by one line.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::LineRightImpl()
{
    HRESULT hr = S_OK;
    xaml::FlowDirection scrollOwnerFlowDirection = xaml::FlowDirection_LeftToRight;
    CTextBoxView_ScrollCommand scrollCommand = CTextBoxView_ScrollCommand::LineRight;

    // TextBoxView always stays in LTR mode since the hosted RichEdit control does not support
    // the same FlowDirection construct as Jupiter. However, ScrollViewer is in RTL mode. Do
    // the conversion between LTR and RTL in this IScrollInfo interop layer.
    IFC(GetScrollOwnerFlowDirection(&scrollOwnerFlowDirection));
    scrollCommand = scrollOwnerFlowDirection == xaml::FlowDirection_LeftToRight ? CTextBoxView_ScrollCommand::LineRight : CTextBoxView_ScrollCommand::LineLeft;
    IFC(TextBoxViewScroll(scrollCommand));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport up by one page.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::PageUpImpl()
{
    RRETURN(TextBoxViewScroll(CTextBoxView_ScrollCommand::PageUp));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport down by one page.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::PageDownImpl()
{
    RRETURN(TextBoxViewScroll(CTextBoxView_ScrollCommand::PageDown));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport left by one page.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::PageLeftImpl()
{
    HRESULT hr = S_OK;
    xaml::FlowDirection scrollOwnerFlowDirection = xaml::FlowDirection_LeftToRight;
    CTextBoxView_ScrollCommand scrollCommand = CTextBoxView_ScrollCommand::PageLeft;

    // TextBoxView always stays in LTR mode since the hosted RichEdit control does not support
    // the same FlowDirection construct as Jupiter. However, ScrollViewer is in RTL mode. Do
    // the conversion between LTR and RTL in this IScrollInfo interop layer.
    IFC(GetScrollOwnerFlowDirection(&scrollOwnerFlowDirection));
    scrollCommand = scrollOwnerFlowDirection == xaml::FlowDirection_LeftToRight ? CTextBoxView_ScrollCommand::PageLeft : CTextBoxView_ScrollCommand::PageRight;
    IFC(TextBoxViewScroll(scrollCommand));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport right by one page.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::PageRightImpl()
{
    HRESULT hr = S_OK;
    xaml::FlowDirection scrollOwnerFlowDirection = xaml::FlowDirection_LeftToRight;
    CTextBoxView_ScrollCommand scrollCommand = CTextBoxView_ScrollCommand::PageRight;

    // TextBoxView always stays in LTR mode since the hosted RichEdit control does not support
    // the same FlowDirection construct as Jupiter. However, ScrollViewer is in RTL mode. Do
    // the conversion between LTR and RTL in this IScrollInfo interop layer.
    IFC(GetScrollOwnerFlowDirection(&scrollOwnerFlowDirection));
    scrollCommand = scrollOwnerFlowDirection == xaml::FlowDirection_LeftToRight ? CTextBoxView_ScrollCommand::PageRight : CTextBoxView_ScrollCommand::PageLeft;
    IFC(TextBoxViewScroll(scrollCommand));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport up by one mouse wheel delta.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP
TextBoxView::MouseWheelUp(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN passToScrollOwner = FALSE;
    IScrollViewer* pViewer = NULL;

    IFC(get_ShouldPassWheelMessageToScrollOwner(passToScrollOwner));

    if (passToScrollOwner)
    {
        IFC(PassWheelMessageToScrollOwner(ZoomDirection_In));
    }
    else
    {
        bool scrolled = false;
        IFC(TextBoxViewScroll(CTextBoxView_ScrollCommand::MouseWheelUp, mouseWheelDelta, &scrolled));
        if (!scrolled)
        {
            IFC(ctl::resolve_weakref(m_pScrollOwnerWeakRef, pViewer));
            ScrollViewer* pScrollViewer = static_cast<ScrollViewer*>(pViewer);

            if (pScrollViewer)
            {
                pScrollViewer->m_handleScrollInfoWheelEvent = FALSE;
            }
        }
    }
Cleanup:
    ReleaseInterface(pViewer);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport down by one mouse wheel delta.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP
TextBoxView::MouseWheelDown(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN passToScrollOwner = FALSE;
    IScrollViewer* pViewer = NULL;

    IFC(get_ShouldPassWheelMessageToScrollOwner(passToScrollOwner));

    if (passToScrollOwner)
    {
        IFC(PassWheelMessageToScrollOwner(ZoomDirection_Out));
    }
    else
    {
        bool scrolled = false;
        IFC(TextBoxViewScroll(CTextBoxView_ScrollCommand::MouseWheelDown, mouseWheelDelta, &scrolled));
        if (!scrolled)
        {
            IFC(ctl::resolve_weakref(m_pScrollOwnerWeakRef, pViewer));
            ScrollViewer* pScrollViewer = static_cast<ScrollViewer*>(pViewer);
            if (pScrollViewer)
            {
                pScrollViewer->m_handleScrollInfoWheelEvent = FALSE;
            }
        }
    }
Cleanup:
    ReleaseInterface(pViewer);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport left by one mouse wheel delta.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP
TextBoxView::MouseWheelLeft(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    xaml::FlowDirection scrollOwnerFlowDirection = xaml::FlowDirection_LeftToRight;
    CTextBoxView_ScrollCommand scrollCommand = CTextBoxView_ScrollCommand::MouseWheelLeft;

    // TextBoxView always stays in LTR mode since the hosted RichEdit control does not support
    // the same FlowDirection construct as Jupiter. However, ScrollViewer is in RTL mode. Do
    // the conversion between LTR and RTL in this IScrollInfo interop layer.
    IFC(GetScrollOwnerFlowDirection(&scrollOwnerFlowDirection));
    scrollCommand = scrollOwnerFlowDirection == xaml::FlowDirection_LeftToRight ? CTextBoxView_ScrollCommand::MouseWheelLeft : CTextBoxView_ScrollCommand::MouseWheelRight;
    IFC(TextBoxViewScroll(scrollCommand, mouseWheelDelta));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport right by one mouse wheel delta.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP
TextBoxView::MouseWheelRight(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    xaml::FlowDirection scrollOwnerFlowDirection = xaml::FlowDirection_LeftToRight;
    CTextBoxView_ScrollCommand scrollCommand = CTextBoxView_ScrollCommand::MouseWheelRight;

    // TextBoxView always stays in LTR mode since the hosted RichEdit control does not support
    // the same FlowDirection construct as Jupiter. However, ScrollViewer is in RTL mode. Do
    // the conversion between LTR and RTL in this IScrollInfo interop layer.
    IFC(GetScrollOwnerFlowDirection(&scrollOwnerFlowDirection));
    scrollCommand = scrollOwnerFlowDirection == xaml::FlowDirection_LeftToRight ? CTextBoxView_ScrollCommand::MouseWheelRight : CTextBoxView_ScrollCommand::MouseWheelLeft;
    IFC(TextBoxViewScroll(scrollCommand, mouseWheelDelta));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport horizontally to a given position.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::SetHorizontalOffsetImpl(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    CTextBoxView* pTextBoxView = static_cast<CTextBoxView*>(GetHandle());
    const CTextBoxView_ScrollData* pScrollData = NULL;
    xaml::FlowDirection scrollOwnerFlowDirection = xaml::FlowDirection_LeftToRight;

    CoreImports::TextBoxView_GetScrollData(pTextBoxView, &pScrollData);
    IFC(GetScrollOwnerFlowDirection(&scrollOwnerFlowDirection));
    // TextBoxView always stays in LTR mode since the hosted RichEdit control does not support
    // the same FlowDirection construct as Jupiter. However, ScrollViewer is in RTL mode. Do
    // the conversion between LTR and RTL in this IScrollInfo interop layer.
    if (scrollOwnerFlowDirection == FlowDirection_RightToLeft)
    {
        offset = pScrollData->ExtentWidth - offset - pScrollData->ViewportWidth;
    }

    IFC(CoreImports::TextBoxView_SetScrollOffsets(pTextBoxView, offset, pScrollData->VerticalOffset));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls the viewport vertically to a given position.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::SetVerticalOffsetImpl(_In_ DOUBLE offset)
{
    CTextBoxView* pTextBoxView = static_cast<CTextBoxView*>(GetHandle());
    const CTextBoxView_ScrollData* pScrollData = NULL;

    CoreImports::TextBoxView_GetScrollData(pTextBoxView, &pScrollData);

    RRETURN(CoreImports::TextBoxView_SetScrollOffsets(pTextBoxView, pScrollData->HorizontalOffset, offset));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IScrollInfo override, scrolls a rect into view.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::MakeVisibleImpl(
    _In_ IUIElement* visual,
    wf::Rect targetRect,
    BOOLEAN useAnimation,
    DOUBLE horizontalAlignmentRatio,
    DOUBLE verticalAlignmentRatio,
    DOUBLE offsetX,
    DOUBLE offsetY,
    _Out_ wf::Rect* resultRectangle,
    _Out_opt_ DOUBLE* appliedOffsetX,
    _Out_opt_ DOUBLE* appliedOffsetY)
{
    if (appliedOffsetX)
    {
        *appliedOffsetX = 0.0;
    }

    if (appliedOffsetY)
    {
        *appliedOffsetY = 0.0;
    }

    XRECTF xcpTargetRect = { targetRect.X, targetRect.Y, targetRect.Width, targetRect.Height };
    XRECTF xcpVisibleBounds = {};

    // Ignoring useAnimation param - the TextBoxView control does not support asynchronous animated moves to a rect.
    IFC_RETURN(static_cast<CTextBoxView*>(GetHandle())->MakeVisible(
        xcpTargetRect,
        horizontalAlignmentRatio,
        verticalAlignmentRatio,
        offsetX,
        offsetY,
        &xcpVisibleBounds,
        appliedOffsetX,
        appliedOffsetY));

    resultRectangle->X = xcpVisibleBounds.X;
    resultRectangle->Y = xcpVisibleBounds.Y;
    resultRectangle->Width = xcpVisibleBounds.Width;
    resultRectangle->Height = xcpVisibleBounds.Height;

    return S_OK;
}

// Invokes the IScrollOwner::NotifyHorizontalOffsetChanging and IScrollOwner::NotifyVerticalOffsetChanging callbacks for whichever offset is changing.
_Check_return_ HRESULT TextBoxView::NotifyOffsetsChanging(
    _In_ CDependencyObject* textBoxView,
    XDOUBLE oldHorizontalOffset,
    XDOUBLE newHorizontalOffset,
    XDOUBLE oldVerticalOffset,
    XDOUBLE newVerticalOffset)
{
    bool isHorizontalOffsetChanging = oldHorizontalOffset != newHorizontalOffset;
    bool isVerticalOffsetChanging = oldVerticalOffset != newVerticalOffset;

    if (isHorizontalOffsetChanging || isVerticalOffsetChanging)
    {
        ctl::ComPtr<DependencyObject> textBoxViewPeer;
        ctl::ComPtr<IInspectable> scrollOwnerAsInspectable;

        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(textBoxView, &textBoxViewPeer));
        IFC_RETURN(static_cast<TextBoxView*>(textBoxViewPeer.Get())->get_ScrollOwner(&scrollOwnerAsInspectable));

        if (scrollOwnerAsInspectable)
        {
            ctl::ComPtr<IScrollOwner> scrollOwner = scrollOwnerAsInspectable.AsOrNull<IScrollOwner>();

            if (scrollOwner)
            {
                if (isHorizontalOffsetChanging)
                {
                    IFC_RETURN(scrollOwner->NotifyHorizontalOffsetChanging(newHorizontalOffset, newVerticalOffset));
                }
                if (isVerticalOffsetChanging)
                {
                    IFC_RETURN(scrollOwner->NotifyVerticalOffsetChanging(newHorizontalOffset, newVerticalOffset));
                }
            }
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Reverse pinvoke handler, notifies the containing scroll owner (if
//      any) that the viewport, extent, or scroll offsets have changed.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::InvalidateScrollInfo(_In_ CDependencyObject* pTextBoxView)
{
    HRESULT hr = S_OK;
    DependencyObject* pPeer = NULL;
    TextBoxView* pTextBoxViewPeer = NULL;
    IInspectable* pScrollInspectable = NULL;
    IScrollOwner* pScrollOwner = NULL;

    ASSERT(pTextBoxView != NULL);

    // Get the framework peer.
    IFC(DXamlCore::GetCurrent()->GetPeer(pTextBoxView, &pPeer));
    pTextBoxViewPeer = static_cast<TextBoxView*>(pPeer);

    // Do the work.
    IFC(pTextBoxViewPeer->get_ScrollOwner(&pScrollInspectable));
    IFC(ctl::do_query_interface(pScrollOwner, pScrollInspectable));
    if (pScrollOwner)
    {
        IFC(pScrollOwner->InvalidateScrollInfoImpl());
    }

Cleanup:
    ReleaseInterface(pScrollInspectable);
    ReleaseInterface(pScrollOwner);
    ctl::release_interface(pPeer);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      DirectManipulationStateChangeHandler implementation.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::NotifyStateChange(
    _In_ DMManipulationState state,
    _In_ FLOAT xCumulativeTranslation,
    _In_ FLOAT yCumulativeTranslation,
    _In_ FLOAT zCumulativeFactor,
    _In_ FLOAT xCenter,
    _In_ FLOAT yCenter,
    _In_ BOOLEAN isInertial,
    _In_ BOOLEAN isTouchConfigurationActivated,
    _In_ BOOLEAN isBringIntoViewportConfigurationActivated)
{
    HRESULT hr = S_OK;
    CTextBoxView* pTextBoxView = static_cast<CTextBoxView*>(GetHandle());

    if (state == DMManipulationStarted)
    {
        CoreImports::TextBoxView_DirectManipulationStarted(pTextBoxView);
    }
    else if (state == DMManipulationCompleted)
    {
        CoreImports::TextBoxView_DirectManipulationCompleted(pTextBoxView);
    }
    else if (state == DMManipulationDelta)
    {
        xaml::FlowDirection scrollOwnerFlowDirection = xaml::FlowDirection_LeftToRight;
        FLOAT xDelta = 0.0;

        // TextBoxView always stays in LTR mode since the hosted RichEdit control does not support
        // the same FlowDirection construct as Jupiter. However, ScrollViewer is in RTL mode. Do
        // the conversion between LTR and RTL in this IScrollInfo interop layer.
        IFC(GetScrollOwnerFlowDirection(&scrollOwnerFlowDirection));
        xDelta = scrollOwnerFlowDirection == xaml::FlowDirection_LeftToRight ? xCumulativeTranslation : -xCumulativeTranslation;
        CoreImports::TextBoxView_DirectManipulationDelta(pTextBoxView, xDelta, yCumulativeTranslation, zCumulativeFactor);
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the flowdirection value for scroll owner hosting TextBoxView.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::GetScrollOwnerFlowDirection(xaml::FlowDirection* pValue)
{
    HRESULT hr = S_OK;
    IFrameworkElement* pScrollOwner = NULL;

    IFC(ctl::resolve_weakref(m_pScrollOwnerWeakRef, pScrollOwner));
    if (pScrollOwner)
    {
        IFC(pScrollOwner->get_FlowDirection(pValue));
    }

Cleanup:
    ReleaseInterface(pScrollOwner);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called by CTextBoxView to post a dispatcher callback for async update of caret element.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::CaretChanged(_In_ CDependencyObject* pNativeTextBoxView)
{
    RRETURN(PostDispatcherCallback(pNativeTextBoxView, &TextBoxView::CaretChangedImpl));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Dispatcher callback to update the caret element
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::CaretChangedImpl()
{
    HRESULT hr = S_OK;
    CoreImports::TextBoxView_CaretChanged(static_cast<CTextBoxView*>(GetHandle()));
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called by CTextBoxView to post a dispatcher callback for async update of caret visibility
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::CaretVisibilityChanged(_In_ CDependencyObject* pNativeTextBoxView)
{
    RRETURN(PostDispatcherCallback(pNativeTextBoxView, &TextBoxView::CaretVisibilityChangedImpl));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Dispatcher callback to update the caret visibility
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::CaretVisibilityChangedImpl()
{
    HRESULT hr = S_OK;
    CoreImports::TextBoxView_CaretVisibilityChanged(static_cast<CTextBoxView*>(GetHandle()));
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called by CTextBoxView to post a dispatcher callback for async
//      view invalidation.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::InvalidateView(_In_ CDependencyObject* pNativeTextBoxView)
{
    RRETURN(PostDispatcherCallback(pNativeTextBoxView, &TextBoxView::InvalidateViewImpl));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Dispatcher callback to invalidate the view.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::InvalidateViewImpl()
{
    HRESULT hr = S_OK;
    CoreImports::TextBoxView_InvalidateView(static_cast<CTextBoxView*>(GetHandle()));
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called by CTextBoxView to post a dispatcher callback for async update of caret visibility
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::PostDispatcherCallback(
    _In_ CDependencyObject* pNativeTextBoxView,
    _In_ CoreDispatcherTextBoxViewCallbackHandler pfnEventHandler)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget = NULL;
    ctl::WeakRefPtr spWeakRef;

    ASSERT(pNativeTextBoxView);
    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeTextBoxView, spTarget.GetAddressOf()));

    IFC(spTarget.AsWeak(&spWeakRef));

    IFC(spTarget->GetXamlDispatcherNoRef()->RunAsync(
        MakeCallback(
            spWeakRef,
            pfnEventHandler)));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Check whether we should defer our scroll wheel handling to scroll owner.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::get_ShouldPassWheelMessageToScrollOwner(_Out_ BOOLEAN &shouldPass)
{
    HRESULT hr = S_OK;
    wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;

    IFC(Control::GetKeyboardModifiers(&modifiers));

Cleanup:
    shouldPass = IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Control);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Allow our scroll owner to handle the last mouse wheel message.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::PassWheelMessageToScrollOwner(_In_ ZoomDirection zoomDirection)
{
    HRESULT hr = S_OK;
    IScrollOwner* pScrollOwner = NULL;

    IFC(ctl::resolve_weakref(m_pScrollOwnerWeakRef, pScrollOwner));
    if (pScrollOwner)
    {
        IFC(pScrollOwner->ProcessPureInertiaInputMessage(zoomDirection));
    }

Cleanup:
    ReleaseInterface(pScrollOwner);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Scrolls the TextBoxView
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBoxView::TextBoxViewScroll(
    _In_ CTextBoxView_ScrollCommand command,
    _In_ XUINT32 mouseWheelDelta,
    _Out_opt_ bool *pScrolled)
{
    RRETURN(CoreImports::TextBoxView_Scroll(static_cast<CTextBoxView*>(GetHandle()), command, mouseWheelDelta, pScrolled));
}
