// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CInputPaneHandler class declaration.
//    It is for implemention IXcpInputPaneHandler to response Input Pane
//    state changing.

#pragma once

class CContentControl;
class CUIElement;
class CPopup;

#include "XcpInputPaneHandler.h"
#include "enumdefs.h"

class CInputPaneHandler : public IXcpInputPaneHandler
{
public:
    CInputPaneHandler(CScrollContentControl* pRootScrollViewer, bool useVisualRelativePixels)
        : m_pRootScrollViewer(pRootScrollViewer)
        , m_inputPaneState(DirectUI::InputPaneState::InputPaneHidden)
        , m_inputPaneShowingBringIntoViewNotHandled(false)
        , m_bHandledFocusElementInView(false)
        , m_rootSVHeightInDips(0.0f)
        , m_ihmTopInDips(0.0f)
        , m_useVisualRelativePixels(useVisualRelativePixels)
    {
        m_ptLastPointerPosition.x = 0.0f;
        m_ptLastPointerPosition.y = 0.0f;
        m_rectInputPaneInPixels = {};
    }

    // IXcpInputPaneHandler
    _Check_return_ HRESULT Showing(_In_ XRECTF *pOccludedRectangle, _In_ BOOL ensureFocusedElementInView) override;
    _Check_return_ HRESULT Hiding(_In_ BOOL ensureFocusedElementInView) override;
    _Check_return_ HRESULT NotifyEditFocusRemoval() override;
    _Check_return_ HRESULT NotifyEditControlInputPaneHiding() override;

    // Specified the latest pointer position by InputManager
    void SetPointerPosition(_In_ XPOINTF ptPosition);

    // Ensure the focused element is visible with IHM
    _Check_return_ HRESULT EnsureFocusedElementBringIntoView(_In_ bool isIHMShowing, _In_ bool forceIntoView, _In_ bool animateIfBringIntoView = false);

    bool IsInputPaneShowed()
    {
        return m_inputPaneState != DirectUI::InputPaneState::InputPaneHidden;
    }

    bool IsInputPaneShowedBringIntoViewNotHandled()
    {
        return m_inputPaneShowingBringIntoViewNotHandled;
    }

    DirectUI::InputPaneState GetInputPaneState()
    {
        return m_inputPaneState;
    }

    _Check_return_ HRESULT GetInputPaneBounds(_Out_ XRECTF* pInputPaneBounds);

    XRECTF GetInputPaneExposureRect() override;

    _Check_return_ HRESULT GetVisibleBoundsAdjustment(
        _In_ XRECTF_RB rectInnerScrollViewerViewportBounds,
        _Out_ XRECTF_RB* pRectVisibleBoundsAdjustment);

    void AdjustBringIntoViewRecHeight(_In_ float topGlobal, _In_ float bottomGlobal, _Inout_ float &height);

private:
    _Check_return_ HRESULT BringFocusedElementIntoView(
        _In_ const bool isIHMShowing,
        _In_ CUIElement* const focusedElement,
        _In_ CUIElement* const innerScrollViewer,
        _In_ const bool forceIntoView,
        _In_ const bool animateIfBringIntoView = false);
    CUIElement* const GetInnerScroller(_In_ const CUIElement* const element) const;
    _Check_return_ HRESULT GetInnermostScrollableScrollerWidthAndHeight(
        _In_ CUIElement* const innerScroller,
        _In_ CUIElement* const focusedElement,
        _Out_ XFLOAT& innermostHorizontallyScrollableScrollerWidth,
        _Out_ XFLOAT& innermostVerticallyScrollableScrollerHeight);

    // Gets a focused bounding rect from the current caret position, thus only applies to TextBox, PasswordBox and RichEditBox controls.
    // Adjusts focused element rect based on caret position if the rect is bigger than its ScrollViewer's width or height.
    // Ensures that when moving the caret, it remains away from the edges by a margin of 25% from the total size.
    _Check_return_ HRESULT GetFocusedElementBoundingRectForCaret(
        _In_ CUIElement* pFocusedElement,
        _In_ XFLOAT innerSVWidth,
        _In_ XFLOAT innerSVHeight,
        _Inout_ XRECTF& rectFocusedElement);

    // Gets the bounding rectangle for the focused element to land away either from the ScrollViewer, IHM or screen edges.
    // Expands the bounding rectangle by 20 physical pixels on the right and bottom by default. When the current page's
    // LayoutToWindowBounds property is set, a padding of 20 logical pixels is added on all four edges. On top of that, padding
    // are added on all four edges based on the shrunk ApplicationView.VisibleBounds size compared to the CoreWindow bounds.
    // This prevents the focused element from entering the unsafe overscan zones on XBox.
    _Check_return_ HRESULT GetFocusedElementBoundingRectForPadding(
        _In_ CUIElement* const innerScroller,
        _In_ XFLOAT innerScrollerWidth,
        _In_ XFLOAT innerScrollerHeight,
        _In_ XFLOAT innermostHorizontallyScrollableScrollerWidth,
        _In_ XFLOAT innermostVerticallyScrollableScrollerHeight,
        _In_ XRECTF_RB rectInnerScrollerViewportBounds,
        _In_ bool isIHMShowing,
        _In_ XTHICKNESS focusVisualMargin,
        _Inout_ XRECTF* const focusedElementRect);

    _Check_return_ HRESULT GetCaretPosition(
        _In_ CUIElement* pFocusedElement,
        _Out_ XPOINTF& pointCaret);
    _Check_return_ HRESULT GetOpenedPopup(
        _In_ CUIElement* pFocusedElement,
        _Out_ CPopup** ppOpenedPopup);
    _Check_return_ HRESULT TransformGlobalToLocal(
        _In_ CUIElement *pElement,
        _In_ const XRECTF& rectGlobal,
        _Out_ XRECTF* pRectLocal);

    // DEAD_CODE_REMOVAL
    _Check_return_ HRESULT BringTextControlIntoView(_In_ BOOL forceIntoView);

    _Check_return_ HRESULT GetScrollerGlobalBounds(_In_ CUIElement* scroller, _Out_ XRECTF_RB* globalBoundsRB);

private:
    // Root ScrollViewer instance that control the content scrolling by InputPane state notification
    CScrollContentControl*    m_pRootScrollViewer;
    DirectUI::InputPaneState m_inputPaneState;
    bool               m_bHandledFocusElementInView;
    XPOINTF             m_ptLastPointerPosition;

    // Input pane's rect in physical pixels. (Physical pixels are used because of existing callers
    // that expect physical pixels. Can be changed if callers are changed.)
    XRECTF              m_rectInputPaneInPixels;

    // Height of Root Scroll Viewer in DIPs
    XFLOAT              m_rootSVHeightInDips;

    // Top of IHM pane in DIPs
    XFLOAT              m_ihmTopInDips;
    XFLOAT              m_ihmHeightInDips = { 0.0f };

    bool                m_inputPaneShowingBringIntoViewNotHandled;
    bool                m_useVisualRelativePixels;
};
