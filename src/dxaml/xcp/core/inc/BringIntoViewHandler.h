// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CBringIntoViewHandler
{
public:
    // Ensure the focused element is visible with IHM
    static _Check_return_ HRESULT EnsureFocusedElementBringIntoView(
        _In_opt_ CDependencyObject* focusedElement,
        _In_ CUIElement* const rootElement,
        _In_ const float rootHeightInDips,
        _In_ const float ihmHeightInDips,
        _In_ const float ihmTopInDips,
        _In_ const DirectUI::InputPaneState inputPaneState,
        _In_ bool isIHMShowing,
        _In_ bool forceIntoView,
        _In_ bool animateIfBringIntoView);

    static _Check_return_ HRESULT GetVisibleBoundsAdjustment(
        _In_ CUIElement* const rootElement,
        _In_ XRECTF_RB rectInnerScrollViewerViewportBounds,
        _Out_ XRECTF_RB* rectVisibleBoundsAdjustment);

    static void AdjustBringIntoViewRecHeight(
        _In_ CUIElement* const rootElement,
        _In_ float rootHeightInDips,
        _In_ float topGlobal,
        _In_ float bottomGlobal,
        _Inout_ float &height);

    static void DipsToPhysicalPixels(
        _In_ CDependencyObject* dependencyObject,
        _In_ const XRECTF& rectIn,
        _Out_ XRECTF* rectOut);

    static void DipsToPhysicalPixels(
        _In_ CDependencyObject* dependencyObject,
        _In_ const XTHICKNESS& rectIn,
        _Out_ XTHICKNESS* rectOut);

    static XFLOAT DipsToPhysicalPixels(
        _In_ CDependencyObject* dependencyObject,
        _In_ XFLOAT dips);

    static void PhysicalPixelsToDips(
        _In_ CDependencyObject* dependencyObject,
        _In_ const XRECTF& rectIn,
        _Out_ XRECTF* rectOut);

    static XFLOAT PhysicalPixelsToDips(
        _In_ CDependencyObject* dependencyObject,
        _In_ XFLOAT physicalPixels);

//private:
    static _Check_return_ HRESULT BringFocusedElementIntoView(
        _In_ CUIElement* const rootElement,
        _In_ const float rootHeightInDips,
        _In_ const float ihmHeightInDips,
        _In_ const float ihmTopInDips,
        _In_ const DirectUI::InputPaneState inputPaneState,
        _In_ const bool isIHMShowing,
        _In_ CUIElement* const focusedElement,
        _In_ CUIElement* const innerScrollViewer,
        _In_ const bool forceIntoView,
        _In_ const bool animateIfBringIntoView = false);

    static CUIElement* const GetInnerScroller(_In_ const CUIElement* const element);

    static _Check_return_ HRESULT GetInnermostScrollableScrollerWidthAndHeight(
        _In_ CUIElement* const rootElement,
        _In_ CUIElement* const innerScroller,
        _In_ CUIElement* const focusedElement,
        _In_ const float rootHeightInDips,
        _Out_ XFLOAT& innermostHorizontallyScrollableScrollerWidth,
        _Out_ XFLOAT& innermostVerticallyScrollableScrollerHeight);

    // Gets a focused bounding rect from the current caret position, thus only applies to TextBox, PasswordBox and RichEditBox controls.
    // Adjusts focused element rect based on caret position if the rect is bigger than its ScrollViewer's width or height.
    // Ensures that when moving the caret, it remains away from the edges by a margin of 25% from the total size.
    static _Check_return_ HRESULT GetFocusedElementBoundingRectForCaret(
        _In_ CUIElement* focusedElement,
        _In_ XFLOAT innerSVWidth,
        _In_ XFLOAT innerSVHeight,
        _Inout_ XRECTF& rectFocusedElement);

    // Gets rect of focused HTML element within WebView, so that only that sub element is scrolled into view when IHM is showing.
    static _Check_return_ HRESULT GetFocusedElementBoundingRectForWebView(
        _In_ CUIElement* focusedElement,
        _In_ bool isIHMShowing,
        _In_ XRECTF_RB* focusedElementGlobalRect,
        _Inout_ XRECTF* focusedElementRect);

    // Gets the bounding rectangle for the focused element to land away either from the ScrollViewer, IHM or screen edges.
    // Expands the bounding rectangle by 20 physical pixels on the right and bottom by default. When the current page's
    // LayoutToWindowBounds property is set, a padding of 20 logical pixels is added on all four edges. On top of that, padding
    // are added on all four edges based on the shrunk ApplicationView.VisibleBounds size compared to the CoreWindow bounds.
    // This prevents the focused element from entering the unsafe overscan zones on XBox.
    static _Check_return_ HRESULT GetFocusedElementBoundingRectForPadding(
        _In_ CUIElement* const rootElement,
        _In_ CUIElement* const innerScroller,
        _In_ XFLOAT innerScrollerWidth,
        _In_ XFLOAT innerScrollerHeight,
        _In_ XFLOAT innermostHorizontallyScrollableScrollerWidth,
        _In_ XFLOAT innermostVerticallyScrollableScrollerHeight,
        _In_ XRECTF_RB rectInnerScrollerViewportBounds,
        _In_ bool isIHMShowing,
        _In_ XTHICKNESS focusVisualMargin,
        _Inout_ XRECTF* const focusedElementRect);

    static _Check_return_ HRESULT GetCaretPosition(
        _In_ CUIElement* pFocusedElement,
        _Out_ XPOINTF& pointCaret);

    static _Check_return_ HRESULT GetOpenedPopup(
        _In_ CUIElement* focusedElement,
        _Out_ CPopup** openedPopup);

    static _Check_return_ HRESULT TransformGlobalToLocal(
        _In_ CUIElement* element,
        _In_ const XRECTF& rectGlobal,
        _Out_ XRECTF* rectLocal);

    static _Check_return_ HRESULT GetScrollerGlobalBounds(
        _In_ CUIElement* scroller,
        _Out_ XRECTF_RB* globalBoundsRB);
};
