// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlBehaviorMode.h>
#include <FocusProperties.h>
#include <FocusRectManager.h>
#include "InputServices.h"
#include <RootScale.h>
#include <GeneralTransformHelper.h>
#include <XamlOneCoreTransforms.h>

static const float CaretAlignmentThreshold = 0.75 /* 3/4 position of width or height */;
static const unsigned int ExtraPixelsForBringIntoView = 20;


//------------------------------------------------------------------------
//
//  Method:   GetInnerScroller
//
//  Synopsis:
//      Returns the first scroller that's parent of the specified element.
//
//------------------------------------------------------------------------
CUIElement* const CBringIntoViewHandler::GetInnerScroller(_In_ const CUIElement* const element) 
{
    ASSERT(element);

    CUIElement* scroller = nullptr;
    CUIElement* parent = static_cast<CUIElement*>(element->GetParentInternal());

    while (parent)
    {
        if (parent->IsScroller())
        {
            scroller = parent;
            break;
        }
        parent = static_cast<CUIElement*>(parent->GetParentInternal());
    }

    return scroller;
}

_Check_return_ HRESULT
CBringIntoViewHandler::GetInnermostScrollableScrollerWidthAndHeight(
    _In_ CUIElement* const rootElement,
    _In_ CUIElement* const innerScroller,
    _In_ CUIElement* const focusedElement,
    _In_ const float rootHeightInDips,
    _Out_ XFLOAT& innermostHorizontallyScrollableScrollerWidth,
    _Out_ XFLOAT& innermostVerticallyScrollableScrollerHeight)
{
    CUIElement* current = static_cast<CUIElement*>(innerScroller);
    bool horizontalFound = false;
    bool verticalFound = false;

    IFCPTR_RETURN(innerScroller);
    innermostHorizontallyScrollableScrollerWidth = 0.0f;
    innermostVerticallyScrollableScrollerHeight = 0.0f;

    while (current && !(horizontalFound && verticalFound))
    {
        if (current->IsScroller())
        {
            bool horizontalContentScrollable = false;
            bool verticalContentScrollable = false;
            XRECTF_RB rectCurrentScrollerGlobal = { 0.0f, 0.0f, 0.0f, 0.0f };

            // Is current Scrollable - in horizontal/vertical
            FocusProperties::IsScrollable(current, &horizontalContentScrollable, &verticalContentScrollable);

            if (horizontalContentScrollable || verticalContentScrollable)
            {
                XSIZEF sizeViewportRatios = { 1.0f, 1.0f };

                IFC_RETURN(GetScrollerGlobalBounds(current, &rectCurrentScrollerGlobal));

                // We need to adjust the target rect to account for the top/left headers of ScrollViewer.
                // Third party scrollers are expected to do that when the BringIntoViewRequested event is bubbling up.
                if (current->OfTypeByIndex<KnownTypeIndex::ScrollContentPresenter>())
                {
                    IFC_RETURN(FxCallbacks::UIElement_GetScrollContentPresenterViewportRatios(current, static_cast<CDependencyObject*>(focusedElement), &sizeViewportRatios));
                }

                if (!horizontalFound && horizontalContentScrollable)
                {
                    horizontalFound = true;
                    innermostHorizontallyScrollableScrollerWidth = (rectCurrentScrollerGlobal.right - rectCurrentScrollerGlobal.left) * sizeViewportRatios.width;
                }

                if (!verticalFound && verticalContentScrollable)
                {
                    verticalFound = true;
                    if (current == rootElement)
                    {
                        innermostVerticallyScrollableScrollerHeight = DipsToPhysicalPixels(rootElement, rootHeightInDips);
                    }
                    else
                    {
                        innermostVerticallyScrollableScrollerHeight = (rectCurrentScrollerGlobal.bottom - rectCurrentScrollerGlobal.top) * sizeViewportRatios.height;
                    }
                }
            }
        }

        current = static_cast<CUIElement*>(current->GetParentInternal());
    }

    return S_OK;
}

// Ensure the focused element is visible with IHM showing
_Check_return_ HRESULT
CBringIntoViewHandler::EnsureFocusedElementBringIntoView(
    _In_opt_ CDependencyObject* focusedElement,
    _In_ CUIElement* const rootElement,
    _In_ const float rootHeightInDips,
    _In_ const float ihmHeightInDips,
    _In_ const float ihmTopInDips,
    _In_ const DirectUI::InputPaneState inputPaneState,
    _In_ bool isIHMShowing,
    _In_ bool forceIntoView,
    _In_ bool animateIfBringIntoView)
{
    CFocusManager* focusManager = nullptr;

    if (!focusedElement && rootElement)
    {
        // Scroll the current focused element into the view
        IFCPTR_RETURN(focusManager = VisualTree::GetFocusManagerForElement(rootElement));
        focusedElement = focusManager->GetFocusedElementNoRef();
    }

    // Hyperlink may be the focused element. In this case bring into view logic that relies on UIElement will fail.
    // To prevent this failure, do explicit cast to UIElement, and only proceed if cast succeeds. This will mean that bring
    // into view will not work on Hyperlinks - Hyperlink support for this will need to be added separately.
    CUIElement* focusedUIElement = do_pointer_cast<CUIElement>(focusedElement);

    if (focusedUIElement)
    {
        // Apply the input pane transition to start scrolling animation
        if (isIHMShowing)
        {
            IFC_RETURN(FxCallbacks::UIElement_ApplyInputPaneTransition(focusedUIElement, TRUE));
        }

        CUIElement* innerScroller = GetInnerScroller(focusedUIElement);

        if (innerScroller == nullptr && rootElement->IsScroller())
        {
            // This is a popup scenario and we need to ensure the popup
            // content visible under the root SV bounding rectangle.
            innerScroller = rootElement;
        }

        if (innerScroller)
        {
            IFC_RETURN(BringFocusedElementIntoView(
                rootElement,
                rootHeightInDips,
                ihmHeightInDips,
                ihmTopInDips,
                inputPaneState,
                isIHMShowing,
                focusedUIElement,
                innerScroller,
                forceIntoView,
                animateIfBringIntoView));
        }
    }
    else
    {
        // special BringIntoView case for focusable elements
        if (focusedElement && CFocusableHelper::IsFocusableDO(focusedElement))
        {
            focusedUIElement = CFocusableHelper::GetContainingFrameworkElementIfFocusable(focusedElement);
            if (focusedUIElement)
            {
                XRECTF rectBounding, rectBoundingLocal;
                // ignoreClip true to calculate bounding rect even if it is clipped
                IFC_RETURN(focusedElement->GetContext()->GetTextElementBoundingRect(focusedElement, &rectBounding, /* ignoreClip */ true));

                // Transform focused element's rect from global to local, because BringIntoView takes local coordinates
                IFC_RETURN(TransformGlobalToLocal(focusedUIElement, rectBounding, &rectBoundingLocal));
                focusedUIElement->BringIntoView(rectBoundingLocal, forceIntoView, animateIfBringIntoView, !animateIfBringIntoView /*skipDuringManipulation*/);
            }
        }
    }
    return S_OK;
}

// Bring the focus element into view.
//
// Note: Physical pixels are used to allow comparison against scrollviewer
// and other elements in different parts of the visual tree.
_Check_return_ HRESULT
CBringIntoViewHandler::BringFocusedElementIntoView(
    _In_ CUIElement* const rootElement,
    _In_ const float rootHeightInDips,
    _In_ const float ihmHeightInDips,
    _In_ const float ihmTopInDips,
    _In_ const DirectUI::InputPaneState inputPaneState,
    _In_ const bool isIHMShowing,
    _In_ CUIElement* const focusedElement,
    _In_ CUIElement* const innerScroller,
    _In_ const bool forceIntoView,
    _In_ const bool animateIfBringIntoView)
{
    HRESULT hr  = S_OK;
    XFLOAT innerScrollerHeight = 0.0f;
    XFLOAT innerScrollerWidth = 0.0f;
    XFLOAT innermostHorizontalScrollableScrollerWidth = 0.0f;
    XFLOAT innermostVerticalScrollableScrollerHeight = 0.0f;
    XRECTF rectFocusedElement = {0.0f, 0.0f, 0.0f, 0.0f};
    XRECTF rectFocusedElementLocal = {0.0f, 0.0f, 0.0f, 0.0f};
    XSIZEF sizeViewportRatios = {0.0f, 0.0f};
    XRECTF_RB rectInnerScrollerGlobal = {0.0f, 0.0f, 0.0f, 0.0f};
    XRECTF_RB rectFocusedElementGlobal = {0.0f, 0.0f, 0.0f, 0.0f};
    CPopup* openedPopup = nullptr;
    bool isFlyout = false;
    bool isAppBar = false;
    bool isContentDialog = false;
    XTHICKNESS focusVisualMarginInPhysicalPixels = { 0.0f, 0.0f, 0.0f, 0.0f };

    IFCPTR(focusedElement);
    IFCPTR(innerScroller);

    // Get the global bounds in physical pixels to get the real width and height that contains the render
    // transform and DM zoom factor etc. In HIDPI, this will include  the DPI RenderTransform set
    // on the root visual.
    IFC(GetScrollerGlobalBounds(innerScroller, &rectInnerScrollerGlobal));

    // rectInnerScrollerGlobal is used to compute innerScrollerWidth & innerScrollerHeight, and is handed off 
    // to GetFocusedElementBoundingRectForPadding. In all cases, it needs to be expressed in physical pixels instead of DIPs.
    DipsToPhysicalPixels(innerScroller, rectInnerScrollerGlobal, &rectInnerScrollerGlobal);

    IFC(focusedElement->GetGlobalBounds(&rectFocusedElementGlobal, true /* ignoreClipping */));
    if (XamlOneCoreTransforms::IsEnabled())
    {
        auto contentRoot = VisualTree::GetContentRootForElement(focusedElement);
        if (contentRoot->GetType() != CContentRoot::Type::XamlIslandRoot)
        {
            // In OneCoreTransforms CoreWindow mode, GetGlobalBounds returns logical pixels so we must convert to RasterizedClient
            // Exception for island on WCOS: GetGlobalBounds calls CUIElement::TransformToWorldSpace, which only accounts for the rasterization scale
            // when it walks up to a CXamlIslandRoot for an AppWindow, and not the CRootVisual for a CoreWindow."

            DipsToPhysicalPixels(rootElement, rectFocusedElementGlobal, &rectFocusedElementGlobal);
        }
    }
    
    // Determine how large the scroller's viewport is compared to the scroller's entire size.
    IFC(FxCallbacks::UIElement_GetScrollContentPresenterViewportRatios(innerScroller, static_cast<CDependencyObject*>(focusedElement), &sizeViewportRatios));

    // Set the real width and height on the inner scroller's viewport.
    innerScrollerWidth = (rectInnerScrollerGlobal.right - rectInnerScrollerGlobal.left) * sizeViewportRatios.width;
    innerScrollerHeight = (rectInnerScrollerGlobal.bottom - rectInnerScrollerGlobal.top) * sizeViewportRatios.height;
    if (innerScroller == rootElement)
    {
        innerScrollerHeight = DipsToPhysicalPixels(rootElement, rootHeightInDips);
    }

    // Init the focused element's rect in global coordinates
    rectFocusedElement.X = rectFocusedElementGlobal.left;
    rectFocusedElement.Y = rectFocusedElementGlobal.top;
    rectFocusedElement.Width = rectFocusedElementGlobal.right - rectFocusedElementGlobal.left;
    rectFocusedElement.Height = rectFocusedElementGlobal.bottom - rectFocusedElementGlobal.top;

    // Adjust focused element rect based on caret position if the rect is bigger than its
    // scroller's width or height.
    if (rectFocusedElement.Height > innerScrollerHeight || rectFocusedElement.Width > innerScrollerWidth)
    {
        IFC(GetFocusedElementBoundingRectForCaret(focusedElement, innerScrollerWidth, innerScrollerHeight, rectFocusedElement));
    }

    // Get the Focus Visual bounds (the bounds by which it extends outside the element bounds)
    XTHICKNESS focusVisualMargin = CFocusRectManager::GetFocusVisualMargin(focusedElement, nullptr);
    DipsToPhysicalPixels(rootElement, focusVisualMargin, &focusVisualMarginInPhysicalPixels);

    // Gets the bounding rectangle for the focused element to land away either from the scroller, IHM or screen edges.
    // Pre RS1:
    // Expands the bounding rectangle by 20 physical pixels on the right and bottom by default. When the current page's
    // LayoutToWindowBounds property is set, a padding of 20 logical pixels is added on all four edges. On top of that, padding
    // are added on all four edges based on the shrunk ApplicationView.VisibleBounds size compared to the CoreWindow bounds.
    // This prevents the focused element from entering the unsafe overscan zones on XBox.
    // RS1 and beyond:
    // Expands the bounding rectangle by 20 logical pixels on all sides. On top of that, padding
    // are added on all four edges based on the shrunk ApplicationView.VisibleBounds size compared to the CoreWindow bounds.
    // This prevents the focused element from entering the unsafe overscan zones on XBox.
    // We also account for focus visual margin here. We either extend by 20 logical pixels or by the focus visual margin- whichever
    // is larger.
    IFC(GetInnermostScrollableScrollerWidthAndHeight(
        rootElement,
        innerScroller,
        focusedElement,
        rootHeightInDips,
        innermostHorizontalScrollableScrollerWidth,
        innermostVerticalScrollableScrollerHeight));
    IFC(GetFocusedElementBoundingRectForPadding(
        rootElement,
        innerScroller,
        innerScrollerWidth,
        innerScrollerHeight,
        innermostHorizontalScrollableScrollerWidth,
        innermostVerticalScrollableScrollerHeight,
        rectInnerScrollerGlobal,
        isIHMShowing,
        focusVisualMarginInPhysicalPixels,
        &rectFocusedElement));

    if ((isIHMShowing || !rootElement->IsScroller()) && focusedElement->GetRootOfPopupSubTree())
    {
        IFC(GetOpenedPopup(focusedElement, &openedPopup));
        if (openedPopup)
        {
            if (openedPopup->IsFlyout())
            {
                isFlyout = true;
            }
            else if (openedPopup->IsApplicationBarService())
            {
                isAppBar = true;
            }
            else if (openedPopup->IsContentDialog())
            {
                isContentDialog = true;
            }
        }
    }

    // Apply the sticky bottom AppBar height
    if (!isFlyout)
    {
        AdjustBringIntoViewRecHeight(
            rootElement,
            rootHeightInDips,
            rectFocusedElementGlobal.top,
            rectFocusedElementGlobal.bottom,
            rectFocusedElement.Height);
    }

    // Bring the current element into the specified inner scroller.
    {
        // Transform focused element's rect from global to local, because BringIntoView takes local coordinates
        IFC(TransformGlobalToLocal(focusedElement, rectFocusedElement, &rectFocusedElementLocal));

        //
        // Special case if we're animating Xaml content scrolling out of the way. In this case we're not resizing the
        // RootScrollViewer while the animation is playing. This introduces a problem for BringIntoView, which just makes
        // sure than an element is brought into the viewport of a ScrollViewer. We want to not only bring an element into
        // the viewport, we also want to bring it to the top part of the viewport that's not covered by the SIP.
        //
        // There's a "verticalOffset" parameter we can use to do that, which will be an additional amount to scroll.
        // We calculate whether it's needed based on where the element is already, and we calculate how much is needed
        // by comparing the element's bottom edge (with padding already applied) with the top of the SIP. At most, we need
        // to adjust by the entire height of the SIP (BringIntoView will line it up with the bottom of the SIP, and adjusting
        // by the height of the SIP will bring it to the top).
        //
        double verticalOffset = 0.0;

        if (animateIfBringIntoView && ihmHeightInDips > 0.0f)
        {
            float sipTop = rootHeightInDips;
            float rectBottom = PhysicalPixelsToDips(rootElement, rectFocusedElement.Y + rectFocusedElement.Height);
            if (rectBottom > sipTop)
            {
                float diffToSIPTop = rectBottom - sipTop;
                verticalOffset = -MIN(diffToSIPTop, ihmHeightInDips);
            }
        }

        focusedElement->BringIntoView(
            rectFocusedElementLocal,
            forceIntoView,
            animateIfBringIntoView,
            !animateIfBringIntoView /*skipDuringManipulation*/,
            DirectUI::DoubleUtil::NaN /* horizontalAlignmentRatio */,
            DirectUI::DoubleUtil::NaN /* verticalAlignmentRatio*/ ,
            0.0 /* horizontalOffset */ ,
            verticalOffset
            );
    }

    // In case of Popup, the Popup child is attached under the PopupRoot so that the root ScrollViewer
    // can't bring into view the Popup's child. We try to get the Popup position on the visual root
    // and scroll the Popup position area from the visual root. The parent Popup will be ensured to bring
    // into view, but the parentless Popup can't guarantee into view since the parentless Popup is out of
    // the visual root.
    if (isIHMShowing && !isFlyout && !isAppBar && !isContentDialog && focusedElement->GetRootOfPopupSubTree())
    {
        CUIElement* visualRoot = nullptr;

        // Ensure the focused popup area into view. Popup must have a parent to bring into view.
        if (openedPopup && openedPopup->GetParentInternal())
        {
            XRECTF_RB openedPopupGlobal = {0.0f, 0.0f, 0.0f, 0.0f};
            XFLOAT popupHeightToScroll = 0.0f;

            // Get popup's global bounds in physical pixels
            IFC(openedPopup->GetGlobalBounds(&openedPopupGlobal, TRUE /* ignoreClipping */));

            // Calculate the new popup height to scroll that is the focused element
            // plus the rest of controls that below the focused control.
            // Popup root doesn't have a relationship with the root SV so that this is a workaround
            // to bring the focused control's popup area into view.
            popupHeightToScroll = openedPopupGlobal.bottom - rectFocusedElementGlobal.top;

            ASSERT(DipsToPhysicalPixels(rootElement, ihmTopInDips) >= 0);

            // Update the new focused height if focused control is partially occluded by IHM showing.
            if (rectFocusedElementGlobal.bottom > DipsToPhysicalPixels(rootElement, ihmTopInDips))
            {
                rectFocusedElement.Height = popupHeightToScroll;
            }
        }

        // Apply the target element as the visual root instead of the current element
        // to support the popup scenario since popup node is directly associated with
        // the popup root instead of visual root(which is child of root SV).
        visualRoot = static_cast<CUIElement*>(rootElement->GetContext()->getVisualRoot());
        IFCEXPECT(visualRoot);

        // Set the left/top with the global position to bring the popup into view
        // using the visual root element.
        rectFocusedElement.X = rectFocusedElementGlobal.left;
        rectFocusedElement.Y = rectFocusedElementGlobal.top;

        // Transform focused element's rect from global to local, because BringIntoView takes local coordinates
        IFC(TransformGlobalToLocal(visualRoot, rectFocusedElement, &rectFocusedElementLocal));

        static_cast<CUIElement*>(visualRoot)->BringIntoView(rectFocusedElementLocal, forceIntoView, false /*useAnimation*/, true /*skipDuringManipulation*/);
    }

    // Cancel the transition if the focus element is already in the view
    if ((isIHMShowing) &&
        (rectFocusedElementGlobal.bottom < DipsToPhysicalPixels(rootElement, inputPaneState != DirectUI::InputPaneState::InputPaneHidden ? ihmTopInDips : rootHeightInDips)) &&
        (rectFocusedElementGlobal.right < rootElement->GetActualWidth()))
    {
        IFC(FxCallbacks::UIElement_ApplyInputPaneTransition(rootElement, FALSE/*isTransitionEnabled*/));
    }

Cleanup:
    ReleaseInterface(openedPopup);
    RRETURN(hr);
}

// Apply bottom AppBar's height to the bring into view Rect if necessary
void CBringIntoViewHandler::AdjustBringIntoViewRecHeight(
    _In_ CUIElement* const rootElement,
    _In_ float rootHeightInDips,
    _In_ float topGlobal,
    _In_ float bottomGlobal,
    _Inout_ float &height)
{
    bool isTopAppBarOpen = false;
    bool isTopAppBarSticky = false;
    XFLOAT widthOfTopAppBarInDips = 0.0f;
    XFLOAT heightOfTopAppBarInDips = 0.0f;
    bool isBottomAppBarOpen = false;
    bool isBottomAppBarSticky = false;
    XFLOAT widthOfBottomAppBarInDips = 0.0f;
    XFLOAT heightOfBottomAppBarInDips = 0.0f;

    if SUCCEEDED(FxCallbacks::ApplicationBarService_GetAppBarStatus(
        rootElement->GetPublicRootVisual(),
        &isTopAppBarOpen,
        &isTopAppBarSticky,
        &widthOfTopAppBarInDips,
        &heightOfTopAppBarInDips,
        &isBottomAppBarOpen,
        &isBottomAppBarSticky,
        &widthOfBottomAppBarInDips,
        &heightOfBottomAppBarInDips))
    {
        XFLOAT topOfBottomAppBarGlobal = 0.0f;

        // Get the top position of the bottom AppBar from the available root element's height
        topOfBottomAppBarGlobal = DipsToPhysicalPixels(rootElement, rootHeightInDips - heightOfBottomAppBarInDips);

        // Apply the BottomAppBar's height to avoid the hiding focused control by the sticky BottomAppBar.
        //  1. Focused control's Top should be greater than BottomAppBar.Top position.
        //  Or Focused control's Top+Height should be greater than BottomAppBar.Top position.
        //  2. Focused control's height must be less than the actual scrollable height that is the BottomAppBar.Top position.
        if ((topGlobal >= topOfBottomAppBarGlobal || bottomGlobal > topOfBottomAppBarGlobal) &&
            (height <= topOfBottomAppBarGlobal))
        {
            height += DipsToPhysicalPixels(rootElement, heightOfBottomAppBarInDips);
        }
    }
}

// Get a focused bounding rect from the current caret position.
// Adjusts focused element rect based on caret position if the rect is bigger than its ScrollViewer's width or height.
// Ensures that when moving the caret, it remains away from the edges by a margin of 25% from the total size.
_Check_return_ HRESULT
CBringIntoViewHandler::GetFocusedElementBoundingRectForCaret(
    _In_ CUIElement* focusedElement,
    _In_ XFLOAT innerSVWidth,
    _In_ XFLOAT innerSVHeight,
    _Inout_ XRECTF& rectFocusedElement)
{
    ASSERT(focusedElement);

    XPOINTF pointCaret = { 0.0f, 0.0f };
    // Get the caret position from the focused text control.
    // Note: This is relative to the focused control
    IFC_RETURN(GetCaretPosition(focusedElement, pointCaret));

    if (pointCaret.x < innerSVWidth * CaretAlignmentThreshold)
    {
        // Align the front edge
        rectFocusedElement.Width = MIN(rectFocusedElement.Width - pointCaret.x, innerSVWidth);
    }
    else
    {
        if ((rectFocusedElement.Width  - innerSVWidth * CaretAlignmentThreshold) < innerSVWidth)
        {
            // Align the back edge
            rectFocusedElement.X = rectFocusedElement.X + rectFocusedElement.Width - innerSVWidth;
            rectFocusedElement.Width = innerSVWidth;
        }
        else
        {
            // Align the current caret X position
            rectFocusedElement.X += pointCaret.x ;
            rectFocusedElement.Width = MIN(rectFocusedElement.Width - pointCaret.x, innerSVWidth);
        }
    }

    if (pointCaret.y < innerSVHeight * CaretAlignmentThreshold)
    {
        // Align the top edge
        rectFocusedElement.Height = MIN(rectFocusedElement.Height - pointCaret.y, innerSVHeight);
    }
    else
    {
        if ((rectFocusedElement.Height  - innerSVHeight * CaretAlignmentThreshold) < innerSVHeight)
        {
            // Align the bottom edge
            rectFocusedElement.Y = rectFocusedElement.Y + rectFocusedElement.Height - innerSVHeight;
            rectFocusedElement.Height = innerSVHeight;
        }
        else
        {
            // Align the current caret Y position
            rectFocusedElement.Y += pointCaret.y ;
            rectFocusedElement.Height = MIN(rectFocusedElement.Height - pointCaret.y, innerSVHeight);
        }
    }

    return S_OK;
}

// Potentially expands the bounding rectangle for the focused element to land away either from the ScrollViewer, IHM or screen edges.
_Check_return_ HRESULT
CBringIntoViewHandler::GetFocusedElementBoundingRectForPadding(
    _In_ CUIElement* const rootElement,
    _In_ CUIElement* const innerScroller,
    _In_ XFLOAT innerScrollWidth,
    _In_ XFLOAT innerScrollerHeight,
    _In_ XFLOAT innermostHorizontallyScrollableScrollerWidth,
    _In_ XFLOAT innermostVerticallyScrollableScrollerHeight,
    _In_ XRECTF_RB rectInnerScrollerViewportBounds,
    _In_ bool isIHMShowing,
    _In_ XTHICKNESS focusVisualMargin,
    _Inout_ XRECTF* const focusedElementRect)
{
    XRECTF_RB rectVisibleBoundsAdjustment = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Account for the potential top and left ScrollViewer headers. They shrink the effective viewport of the ScrollViewer.
    if (innerScroller->IsRightToLeft())
    {
        rectInnerScrollerViewportBounds.right = rectInnerScrollerViewportBounds.left + innerScrollWidth;
    }
    else
    {
        rectInnerScrollerViewportBounds.left = rectInnerScrollerViewportBounds.right - innerScrollWidth;
    }
    rectInnerScrollerViewportBounds.top = rectInnerScrollerViewportBounds.bottom - innerScrollerHeight;

    {
        IFC_RETURN(GetVisibleBoundsAdjustment(rootElement, rectInnerScrollerViewportBounds, &rectVisibleBoundsAdjustment));

        // Pad all sides of the focused element rectangle with the sizes returned by GetVisibleBoundsAdjustment, without going
        // beyond the size of the inner SV's viewport.
        if (focusedElementRect->Width < innermostHorizontallyScrollableScrollerWidth && rectVisibleBoundsAdjustment.left > 0.0f)
        {
            const float leftPadding = MIN(rectVisibleBoundsAdjustment.left, innermostHorizontallyScrollableScrollerWidth - focusedElementRect->Width);
            focusedElementRect->X -= leftPadding;
            focusedElementRect->Width += leftPadding;
        }

        if (focusedElementRect->Height < innermostVerticallyScrollableScrollerHeight && rectVisibleBoundsAdjustment.top > 0.0f)
        {
            const float topPadding = MIN(rectVisibleBoundsAdjustment.top, innermostVerticallyScrollableScrollerHeight - focusedElementRect->Height);
            focusedElementRect->Y -= topPadding;
            focusedElementRect->Height += topPadding;
        }

        if (focusedElementRect->Width < innermostHorizontallyScrollableScrollerWidth && rectVisibleBoundsAdjustment.right > 0.0f)
        {
            const float rightPadding = MIN(rectVisibleBoundsAdjustment.right, innermostHorizontallyScrollableScrollerWidth - focusedElementRect->Width);
            focusedElementRect->Width += rightPadding;
        }

        if (focusedElementRect->Height < innermostVerticallyScrollableScrollerHeight &&
            rectVisibleBoundsAdjustment.bottom > 0.0f &&
            !isIHMShowing)
        {
            const float bottomPadding = MIN(rectVisibleBoundsAdjustment.bottom, innermostVerticallyScrollableScrollerHeight - focusedElementRect->Height);
            focusedElementRect->Height += bottomPadding;
        }
    }

    // If focused element width or height is 0, do not add padding.
    // Adding padding to zero height/width is not going to help anyway.
    // It fixes a regression in Edge where a DcompVisualElement has 0 global bounds
    if (focusedElementRect->Width > 0 && focusedElementRect->Height > 0)
    {
        // adjust for focus visual margin if larger than 20 logical pixels, if not just use 20 logical pixels.
        // Negative values show by how much the focus visual goes outside the element
        const XFLOAT extraPhysicalPixelsForBringIntoView = DipsToPhysicalPixels(innerScroller, static_cast<XFLOAT>(ExtraPixelsForBringIntoView));
        XRECTF_RB paddingAdjustment = { 0.0f, 0.0f, 0.0f, 0.0f };

        paddingAdjustment.left = MIN(focusVisualMargin.left < 0 ? -focusVisualMargin.left : 0, innermostHorizontallyScrollableScrollerWidth - focusedElementRect->Width);
        paddingAdjustment.left = MAX(paddingAdjustment.left, extraPhysicalPixelsForBringIntoView);

        paddingAdjustment.top = MIN(focusVisualMargin.top < 0 ? -focusVisualMargin.top : 0, innermostVerticallyScrollableScrollerHeight - focusedElementRect->Height);
        paddingAdjustment.top = MAX(paddingAdjustment.top, extraPhysicalPixelsForBringIntoView);

        paddingAdjustment.right = MIN(focusVisualMargin.right < 0.0f ? -focusVisualMargin.right : 0, innermostHorizontallyScrollableScrollerWidth - focusedElementRect->Width);
        paddingAdjustment.right = MAX(paddingAdjustment.right, extraPhysicalPixelsForBringIntoView);

        paddingAdjustment.bottom = MIN(focusVisualMargin.bottom < 0.0f ? -focusVisualMargin.bottom : 0, innermostVerticallyScrollableScrollerHeight - focusedElementRect->Height);
        paddingAdjustment.bottom = MAX(paddingAdjustment.bottom, extraPhysicalPixelsForBringIntoView);

        if (focusedElementRect->Width + paddingAdjustment.left < innermostHorizontallyScrollableScrollerWidth)
        {
            focusedElementRect->X -= paddingAdjustment.left;
            focusedElementRect->Width += paddingAdjustment.left;
        }

        if (focusedElementRect->Height + paddingAdjustment.top < innermostVerticallyScrollableScrollerHeight)
        {
            focusedElementRect->Y -= paddingAdjustment.top;
            focusedElementRect->Height += paddingAdjustment.top;
        }

        if (focusedElementRect->Width + paddingAdjustment.right < innermostHorizontallyScrollableScrollerWidth)
        {
            focusedElementRect->Width += paddingAdjustment.right;
        }

        if (focusedElementRect->Height + paddingAdjustment.bottom < innermostVerticallyScrollableScrollerHeight)
        {
            focusedElementRect->Height += paddingAdjustment.bottom;
        }
    }

    return S_OK;
}

// Get a caret position from the focused element. Position is relative to element.
_Check_return_ HRESULT
CBringIntoViewHandler::GetCaretPosition(
    _In_ CUIElement* focusedElement,
    _Out_ XPOINTF& pointCaret)
{
    CTextBoxBase* textBoxBaseNoRef = nullptr;
    CTextBoxView* textBoxViewNoRef = nullptr;
    CUIElement* caretElementNoRef = nullptr;
    XPOINTF pointCaretLocal = {0.0f, 0.0f};
    XRECTF rectCaret = {0.0f, 0.0f, 0.0f, 0.0f};

    IFCEXPECT_RETURN(focusedElement);
    pointCaret.x = pointCaret.y = 0.0f;

    // Deixar esse if??
    if (focusedElement->OfTypeByIndex<KnownTypeIndex::TextBox>() ||
        focusedElement->OfTypeByIndex<KnownTypeIndex::PasswordBox>() ||
        focusedElement->OfTypeByIndex<KnownTypeIndex::RichEditBox>())
    {
        textBoxBaseNoRef = static_cast<CTextBoxBase*>(focusedElement);
        if (textBoxBaseNoRef)
        {
            textBoxViewNoRef = textBoxBaseNoRef->GetView();
            ASSERT(textBoxViewNoRef);

            // Get the caret rect relative from the text viewport
            rectCaret = textBoxViewNoRef->GetCaretRect();

            pointCaret.x = rectCaret.X;
            pointCaret.y = rectCaret.Y;
            // Get the caret position from the text view.
            caretElementNoRef = textBoxViewNoRef->GetCaretElementNoRef();
            if (caretElementNoRef)
            {
                xref_ptr<CGeneralTransform> transform;
                IFC_RETURN(caretElementNoRef->TransformToVisual(focusedElement, &transform));
                if (transform)
                {
                    IFC_RETURN(transform->TransformPoints(&pointCaretLocal, &pointCaretLocal, 1));

                    pointCaret.x = pointCaretLocal.x;
                    pointCaret.y = pointCaretLocal.y;
                }
            }
        }
    }

    return S_OK;
}

// Get a opened popup from the current focused element.
_Check_return_ HRESULT
CBringIntoViewHandler::GetOpenedPopup(
    _In_ CUIElement* focusedElement,
    _Out_ CPopup** openedPopup)
{
    HRESULT hr = S_OK;
    CPopupRoot* popupRoot = nullptr;
    CPopup** openedPopups = nullptr;
    XINT32 countOpenedPopups = 0;
    bool bFoundPopup = false;

    IFCPTR(focusedElement);
    IFCPTR(openedPopup);

    *openedPopup = nullptr;

    IFC(VisualTree::GetPopupRootForElementNoRef(focusedElement, &popupRoot));
    if (popupRoot)
    {
        IFC(popupRoot->GetOpenPopups(&countOpenedPopups, &openedPopups));

        for (XINT32 i=0; i<countOpenedPopups; i++)
        {
            CPopup* popup = openedPopups[i];
            if (popup && !bFoundPopup)
            {
                CUIElement* child = popup->m_pChild;
                if (child)
                {
                    CDependencyObject* parent = focusedElement->GetParentInternal(false);
                    while (parent)
                    {
                        if (parent == child)
                        {
                            bFoundPopup = true;
                            *openedPopup = popup;
                            popup = nullptr;
                            break;
                        }
                        parent = parent->GetParentInternal(false);
                    }
                }
            }
            ReleaseInterface(popup);
        }
    }

Cleanup:
    delete[] openedPopups;
    RRETURN(hr);
}

void CBringIntoViewHandler::DipsToPhysicalPixels(
    _In_ CDependencyObject* dependencyObject,
    _In_ const XRECTF& rectIn,
    _Out_ XRECTF* rectOut)
{
    const float scale = RootScale::GetRasterizationScaleForElement(dependencyObject);

    rectOut->X = rectIn.X * scale;
    rectOut->Y = rectIn.Y * scale;
    rectOut->Width = rectIn.Width * scale;
    rectOut->Height = rectIn.Height * scale;
}

// Convert DIPs to Physical Pixels
void CBringIntoViewHandler::DipsToPhysicalPixels(
    _In_ CDependencyObject* dependencyObject,
    _In_ const XTHICKNESS& rectIn,
    _Out_ XTHICKNESS* rectOut)
{
    const float scale = RootScale::GetRasterizationScaleForElement(dependencyObject);

    rectOut->left = rectIn.left * scale;
    rectOut->right = rectIn.right * scale;
    rectOut->top = rectIn.top * scale;
    rectOut->bottom = rectIn.bottom * scale;
}

// Convert Physical Pixels to DIPs
void CBringIntoViewHandler::PhysicalPixelsToDips(
    _In_ CDependencyObject* dependencyObject,
    _In_ const XRECTF& rectIn,
    _Out_ XRECTF* rectOut)
{
    const float scale = RootScale::GetRasterizationScaleForElement(dependencyObject);

    rectOut->X = rectIn.X / scale;
    rectOut->Y = rectIn.Y / scale;
    rectOut->Width = rectIn.Width / scale;
    rectOut->Height = rectIn.Height / scale;
}

// Convert DIPs to Physical Pixels
XFLOAT CBringIntoViewHandler::DipsToPhysicalPixels(_In_ CDependencyObject* dependencyObject, _In_ XFLOAT dips)
{
    const float scale = RootScale::GetRasterizationScaleForElement(dependencyObject);
    return (dips * scale);
}

// Convert physical pixels to DIPs
XFLOAT CBringIntoViewHandler::PhysicalPixelsToDips(_In_ CDependencyObject* dependencyObject, _In_ XFLOAT physicalPixels)
{
    const float scale = RootScale::GetRasterizationScaleForElement(dependencyObject);
    return (physicalPixels / scale);
}

// Transform global coordinates to local, relative to element
_Check_return_ HRESULT
CBringIntoViewHandler::TransformGlobalToLocal(
    _In_ CUIElement* element,
    _In_ const XRECTF& rectGlobal,
    _Out_ XRECTF* rectLocal)
{
    // First convert to Dips
    XRECTF rectLocalTmp;
    PhysicalPixelsToDips(element, rectGlobal, &rectLocalTmp);

    // Calculate local rect based on reverse transform to root
    xref_ptr<CGeneralTransform> transform;
    IFC_RETURN(element->TransformToVisual(nullptr, &transform));
    xref_ptr<CGeneralTransform> inverse(GetInverseTransform(transform));
    if (inverse != nullptr)
    {
        IFC_RETURN(inverse->TransformRect(rectLocalTmp, rectLocal));
    }
    else
    {
        EmptyRectF(rectLocal); // In case GetInverseTransform returns null, just return empty rect to have default BringIntoView behavior on element
    }

    return S_OK;
}

// Computes the padding to apply to the rectangle brought into view based on the delta between the CoreWindow bounds and the ApplicationView visible bounds.
_Check_return_ HRESULT
CBringIntoViewHandler::GetVisibleBoundsAdjustment(
    _In_ CUIElement* const rootElement,
    _In_ XRECTF_RB rectInnerScrollViewerViewportBounds,
    _Out_ XRECTF_RB* rectVisibleBoundsAdjustment)
{
    rectVisibleBoundsAdjustment->left = rectVisibleBoundsAdjustment->top = rectVisibleBoundsAdjustment->right = rectVisibleBoundsAdjustment->bottom = 0.0f;

    wf::Rect visibleContentBounds = {};
    IFC_RETURN(FxCallbacks::DXamlCore_GetVisibleContentBoundsForElement(rootElement, &visibleContentBounds));

    // Account for the delta between the CoreWindow bounds and the ApplicationView visible bounds, so that the rectangle brought
    // into view remains within the visible bounds.
    XRECTF contentRootBounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    IFC_RETURN(FxCallbacks::Window_GetContentRootBounds(rootElement, &contentRootBounds));

    // visibleContentBounds and contentRootBounds are expressed in logical pixels. Convert rectInnerScrollViewerViewportBounds
    // into logical pixels as well.
    rectInnerScrollViewerViewportBounds.left = PhysicalPixelsToDips(rootElement, rectInnerScrollViewerViewportBounds.left);
    rectInnerScrollViewerViewportBounds.top = PhysicalPixelsToDips(rootElement, rectInnerScrollViewerViewportBounds.top);
    rectInnerScrollViewerViewportBounds.right = PhysicalPixelsToDips(rootElement, rectInnerScrollViewerViewportBounds.right);
    rectInnerScrollViewerViewportBounds.bottom = PhysicalPixelsToDips(rootElement, rectInnerScrollViewerViewportBounds.bottom);

    const float left = visibleContentBounds.X - contentRootBounds.X - rectInnerScrollViewerViewportBounds.left;
    if (left > 0.0f)
    {
        rectVisibleBoundsAdjustment->left = MIN(left, visibleContentBounds.X - contentRootBounds.X);
    }

    const float right = contentRootBounds.X + rectInnerScrollViewerViewportBounds.right - visibleContentBounds.X - visibleContentBounds.Width;
    if (right > 0.0f)
    {
        rectVisibleBoundsAdjustment->right = MIN(right, visibleContentBounds.X - contentRootBounds.X);
    }

    const float top = visibleContentBounds.Y - contentRootBounds.Y - rectInnerScrollViewerViewportBounds.top;
    if (top > 0.0f)
    {
        rectVisibleBoundsAdjustment->top = MIN(top, visibleContentBounds.Y - contentRootBounds.Y);
    }

    const float bottom = contentRootBounds.Y + rectInnerScrollViewerViewportBounds.bottom - visibleContentBounds.Y - visibleContentBounds.Height;
    if (bottom > 0.0f)
    {
        rectVisibleBoundsAdjustment->bottom = MIN(bottom, visibleContentBounds.Y - contentRootBounds.Y);
    }

    // Convert the padding to physical pixels so that ExtraPixelsForBringIntoView for instance gets scaled to 32px
    // if the global scale factor is 1.6.
    rectVisibleBoundsAdjustment->left = DipsToPhysicalPixels(rootElement, rectVisibleBoundsAdjustment->left);
    rectVisibleBoundsAdjustment->top = DipsToPhysicalPixels(rootElement, rectVisibleBoundsAdjustment->top);
    rectVisibleBoundsAdjustment->right = DipsToPhysicalPixels(rootElement, rectVisibleBoundsAdjustment->right);
    rectVisibleBoundsAdjustment->bottom = DipsToPhysicalPixels(rootElement, rectVisibleBoundsAdjustment->bottom);

    return S_OK;
}

_Check_return_ HRESULT
CBringIntoViewHandler::GetScrollerGlobalBounds(_In_ CUIElement* scroller, _Out_ XRECTF_RB* globalBoundsRB)
{
    const XRECTF localBounds{ 0.0f, 0.0f, scroller->GetActualWidth(), scroller->GetActualHeight() };

    xref_ptr<CGeneralTransform> globalTransform;
    IFC_RETURN(scroller->TransformToVisual(nullptr, &globalTransform));
    XRECTF globalBounds = {};
    IFC_RETURN(globalTransform->TransformRect(localBounds, &globalBounds));
    *globalBoundsRB =
    {
        globalBounds.X,
        globalBounds.Y,
        globalBounds.X + globalBounds.Width,
        globalBounds.Y + globalBounds.Height
    };

    return S_OK;
}

