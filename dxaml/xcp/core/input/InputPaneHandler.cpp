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
#include <BringIntoViewHandler.h>

class CFullWindowMediaRoot;

//------------------------------------------------------------------------
//
//  Method:   Showing
//
//  Synopsis:
//      Response InputPane's showing notification that update the ScrollViewer
//      content's height and make the focused element is located in the view.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputPaneHandler::Showing(
    _In_ XRECTF *pOccludedRectangle, // screen rectangle of the IHM, in physical pixels in Windows and in DIPs on Phone
    _In_ BOOL ensureFocusedElementInView)
{
    XRECTF occludedRectangleInPixels = {0.0f}; // In physical pixels

    IFCPTR_RETURN(pOccludedRectangle);

    // Showing event's SIP rect is zero in height(i.e. HoloLens), just return S_OK.
    if ((pOccludedRectangle->Y == 0.0f) && (pOccludedRectangle->Height == 0.0f))
    {
        return S_OK;
    }

    // FrameworkInputView provides pOccludedRectangle in DIPs, and FrameworkInputPane provides it in physical pixels.
    if (m_useVisualRelativePixels)
    {
        CBringIntoViewHandler::DipsToPhysicalPixels(m_pRootScrollViewer, *pOccludedRectangle, &occludedRectangleInPixels);
    }
    else
    {
        occludedRectangleInPixels = *pOccludedRectangle;
    }

    if (!ensureFocusedElementInView) // APP has already responded to SIP showing to ensure the focused element into the view
    {
        m_inputPaneShowingBringIntoViewNotHandled = true;
        IFC_RETURN(BringTextControlIntoView(FALSE /* forceIntoView */));

        // Update the cached m_rectInputPaneInPixels value always even though application is already
        // handled InputPane Showing event with set ensureFocusedElementInView flag as false.
        m_rectInputPaneInPixels = occludedRectangleInPixels;
    }
    else
    {
        ASSERT(m_pRootScrollViewer);

        m_rectInputPaneInPixels = occludedRectangleInPixels;

        m_bHandledFocusElementInView = true;
        XFLOAT rootSVOriginalHeight = m_pRootScrollViewer->GetRootScrollViewerOriginalHeight();

        if (m_inputPaneState == DirectUI::InputPaneState::InputPaneHidden)
        {
            CValue floatValue;
            XFLOAT floatIHMTop = 0.0f;
            XRECTF coreWindowRect = {0.0f, 0.0f, 0.0f, 0.0f};

            IFC_RETURN(FxCallbacks::Window_GetContentRootBounds(m_pRootScrollViewer, &coreWindowRect));

            // Update the ScrollViewer content height to exclude InputPane window

            floatIHMTop = CBringIntoViewHandler::PhysicalPixelsToDips(m_pRootScrollViewer, occludedRectangleInPixels.Y);
            IFCEXPECT_RETURN(floatIHMTop >= 0);

            m_ihmTopInDips = floatIHMTop;

            IFCEXPECT_RETURN(floatIHMTop >= coreWindowRect.Y);

            // New root SV's height must exclude the core window's top position since Jupiter hosted
            // window can be positioned in the host window (e.g. File Picker).
            // The root SV's height value will be compared with the original root SV's height to take a
            // minimum height.
            m_rootSVHeightInDips = floatIHMTop - coreWindowRect.Y;

            m_ihmHeightInDips = CBringIntoViewHandler::PhysicalPixelsToDips(m_pRootScrollViewer, occludedRectangleInPixels.Height);
            IFCEXPECT_RETURN(m_ihmHeightInDips >= 0);

            // Some of our tests use window sizes much smaller than screen size.
            // In such cases m_rootSVHeightInDips may be larger than the window's original
            // height. Setting root ScrollViewer's height to m_rootSVHeightInDips as is will cause
            // unexpected jumps and thus failing the test (esp. Graphics.Jupiter.SCBPBasictest).
            // So we put an upper bound on m_rootSVHeightInDips to window's original height.
            // Note that this should not be an issue in real world scenarios.
            ASSERT(rootSVOriginalHeight >= 0);
            m_rootSVHeightInDips = MIN(m_rootSVHeightInDips, rootSVOriginalHeight);

            floatValue.SetFloat(m_rootSVHeightInDips);
            IFC_RETURN(m_pRootScrollViewer->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, floatValue));
        }

        m_inputPaneState = DirectUI::InputPaneState::InputPaneShowing;

        // note: only notify of inputpane state change after the height has been set
        IFC_RETURN(FxCallbacks::UIElement_NotifyInputPaneStateChange(m_pRootScrollViewer, m_inputPaneState, m_rectInputPaneInPixels));

        // Ensure the current focused element is into view
        IFC_RETURN(EnsureFocusedElementBringIntoView(true /*isIHMShowing*/, true /*forceIntoView*/));

        // Update the layout at the last not to snap the reduced content size immediately.
        IFC_RETURN(m_pRootScrollViewer->UpdateLayout());
        IFC_RETURN(BringTextControlIntoView(TRUE /* forceIntoView */));

        auto contentRoot = VisualTree::GetContentRootForElement(m_pRootScrollViewer);
        CXamlIslandRoot* xamlIslandRoot = contentRoot->GetXamlIslandRootNoRef();
        // Dirty the RootVisual or IslandRoot as it may need to render the "exposure rect" that is created when the input pane doesn't span all the way across.
        if (xamlIslandRoot != nullptr)
        {
            CUIElement::NWSetContentDirty(xamlIslandRoot, DirtyFlags::Render);
        }
        else
        {
            CUIElement::NWSetContentDirty(m_pRootScrollViewer->GetContext()->GetMainRootVisual(), DirtyFlags::Render);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Hiding
//
//  Synopsis:
//      Response InputPane's Hiding notification that restore the ScrollViewer
//      content's original height.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputPaneHandler::Hiding(
    _In_ BOOL ensureFocusedElementInView)
{
    CValue floatValue;
    XRECTF rectInputPane = {0.0f, 0.0f, 0.0f, 0.0f};

    // Update the cached InputPane occulude rectangle with empty rect
    m_rectInputPaneInPixels = {};

    m_inputPaneState = DirectUI::InputPaneState::InputPaneHidden;
    m_inputPaneShowingBringIntoViewNotHandled = false;

    if (ensureFocusedElementInView && m_bHandledFocusElementInView)
    {
        XFLOAT rootSVOriginalHeight = m_pRootScrollViewer->GetRootScrollViewerOriginalHeight();

        floatValue.SetFloat(rootSVOriginalHeight);
        IFC_RETURN(m_pRootScrollViewer->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, floatValue));
        m_rootSVHeightInDips = rootSVOriginalHeight;

        // Reset ihmTop height in case of hiding IHM.
        m_ihmTopInDips = 0;

        // note: only notify of pane state change after height has been set
        IFC_RETURN(FxCallbacks::UIElement_NotifyInputPaneStateChange(m_pRootScrollViewer, DirectUI::InputPaneState::InputPaneHidden, rectInputPane));

        IFC_RETURN(FxCallbacks::UIElement_ApplyInputPaneTransition(m_pRootScrollViewer, true));

        // Dirty the RootVisual as it may need to clear the "exposure rect" that is created when the input pane doesn't span all the way across.
        CUIElement::NWSetContentDirty(m_pRootScrollViewer->GetContext()->GetMainRootVisual(), DirtyFlags::Render);
    }

    return S_OK;
}

_Check_return_ HRESULT
CInputPaneHandler::NotifyEditFocusRemoval()
{
    CFocusManager* pFocusManager = nullptr;
    CControl* pFocusedElement = nullptr;

    IFCEXPECT_RETURN(pFocusManager = VisualTree::GetFocusManagerForElement(m_pRootScrollViewer));
    pFocusedElement = do_pointer_cast<CControl>(pFocusManager->GetFocusedElementNoRef());

    if (CInputServices::IsTextEditableControl(pFocusedElement))
    {
        CTextBoxBase *pTextControl = do_pointer_cast<CTextBoxBase>(pFocusedElement);

        if (pTextControl)
        {
            IFC_RETURN(pTextControl->NotifyEditFocusLost());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CInputPaneHandler::NotifyEditControlInputPaneHiding()
{
    CFocusManager* pFocusManager = nullptr;
    CControl* pFocusedElement = nullptr;

    IFCEXPECT_RETURN(pFocusManager = VisualTree::GetFocusManagerForElement(m_pRootScrollViewer));
    pFocusedElement = do_pointer_cast<CControl>(pFocusManager->GetFocusedElementNoRef());

    if (CInputServices::IsTextEditableControl(pFocusedElement))
    {
        CTextBoxBase *pTextControl = do_pointer_cast<CTextBoxBase>(pFocusedElement);

        if (pTextControl)
        {
            IFC_RETURN(pTextControl->NotifyEditControlInputPaneHiding());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetPointerPosition
//
//  Synopsis:
//      Update the latest pointer position by InputManager.
//
//------------------------------------------------------------------------
void
CInputPaneHandler::SetPointerPosition(
    _In_ XPOINTF ptPosition)
{
    m_ptLastPointerPosition.x = ptPosition.x;
    m_ptLastPointerPosition.y = ptPosition.y;
}


//------------------------------------------------------------------------
//
//  Method:   EnsureFocusedElementBringIntoView
//
//  Synopsis:
//      Ensure the focused element is visible with IHM showing
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputPaneHandler::EnsureFocusedElementBringIntoView(
    _In_ bool isIHMShowing,
    _In_ bool forceIntoView,
    _In_ bool animateIfBringIntoView)
{
    IFC_RETURN(CBringIntoViewHandler::EnsureFocusedElementBringIntoView(
        nullptr /*focusedElement*/,
        m_pRootScrollViewer,
        m_rootSVHeightInDips,
        m_ihmHeightInDips,
        m_ihmTopInDips,
        m_inputPaneState,
        isIHMShowing,
        forceIntoView,
        animateIfBringIntoView));
    
    return S_OK;
}

// Apply bottom AppBar's height to the bring into view Rect if necessary
void CInputPaneHandler::AdjustBringIntoViewRecHeight(_In_ float topGlobal, _In_ float bottomGlobal, _Inout_ float &height)
{
    CBringIntoViewHandler::AdjustBringIntoViewRecHeight(
        m_pRootScrollViewer,
        m_rootSVHeightInDips,
        topGlobal,
        bottomGlobal,
        height
    );
}

//------------------------------------------------------------------------
//
//  Method:   GetInputPaneBounds
//
//  Synopsis:
//      Return bounds of input pane (IHM) in physical pixels.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputPaneHandler::GetInputPaneBounds(
    _Out_ XRECTF* pInputPaneBounds)
{
    IFCEXPECT_RETURN(pInputPaneBounds);

    *pInputPaneBounds = m_rectInputPaneInPixels;

    return S_OK;
}

// Retrieves the area exposed when the input pane doesn't completely cover the XAML tree
XRECTF CInputPaneHandler::GetInputPaneExposureRect()
{
    XRECTF exposureRect = {};

    XRECTF inputPaneRectDips;
    CBringIntoViewHandler::PhysicalPixelsToDips(m_pRootScrollViewer, m_rectInputPaneInPixels, &inputPaneRectDips);

    // This code finds the rect below the RootScrollViewer that isn't covered by the input pane.  This rect may be empty.
    // The input pane is assumed to only ever be "docked" to the left or right hand side of the window (or both).
    // For example this rect will look like the below when the input pane comes up on the left hand side of the window:
    //
    // |------------------------------------|
    // |                                    |
    // |          RootScrollViewer          |
    // |                                    |
    // |                                    |
    // |                                    |
    // |                                    |
    // |------------------------------------|---  RootScrollViewer stops here ----
    // |                 |                  |
    // |  input pane     |   exposure rect  |
    // |                 |                  |
    // |-------------------------------------
    //
    CRootVisual* rootVisual = m_pRootScrollViewer->GetContext()->GetMainRootVisual();

    if (inputPaneRectDips.Width < rootVisual->GetActualWidth() && inputPaneRectDips.Height > 0)
    {
        exposureRect.X = inputPaneRectDips.X == 0 ? inputPaneRectDips.Width : 0;
        exposureRect.Y = m_rootSVHeightInDips;
        exposureRect.Width = rootVisual->GetActualWidth() - inputPaneRectDips.Width;
        exposureRect.Height = inputPaneRectDips.Height;
    }

    return exposureRect;
}

// DEAD_CODE_REMOVAL
_Check_return_ HRESULT
CInputPaneHandler::BringTextControlIntoView(_In_ BOOL forceIntoView)
{
    return S_OK;
}

// Computes the padding to apply to the rectangle brought into view based on the delta between the CoreWindow bounds and the ApplicationView visible bounds.
_Check_return_ HRESULT
CInputPaneHandler::GetVisibleBoundsAdjustment(
    _In_ XRECTF_RB rectInnerScrollViewerViewportBounds,
    _Out_ XRECTF_RB* pRectVisibleBoundsAdjustment)
{
    return CBringIntoViewHandler::GetVisibleBoundsAdjustment(
        m_pRootScrollViewer,
        rectInnerScrollViewerViewportBounds,
        pRectVisibleBoundsAdjustment);
}
