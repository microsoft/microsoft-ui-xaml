// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    General IDirectManipulationContainerHandler implementation that can
//    be used for any non-Core control such as the ScrollViewer control.
//    It handles the 'reverse-PInvokes' used to interact with the InputManager.

#include "precomp.h"

// Uncomment to get DirectManipulation debug traces
// #define DMCH_DBG

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::CUIDMContainerHandler
//
//  Synopsis:
//      Constructor of the CUIDMContainerHandler class
//
//------------------------------------------------------------------------
CUIDMContainerHandler::CUIDMContainerHandler(_In_ CInputServices* inputServices, _In_ CUIElement* pUIElement)
    : m_inputServices(inputServices)
    , m_pUIElement(pUIElement)
{
#ifdef DMCH_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(L"DMCH[0x%p]:  CUIDMContainerHandler - constructor.\r\n", this));
#endif // DMCH_DBG

    ASSERT(inputServices);
    inputServices->AddRef();

    ASSERT(pUIElement);
    pUIElement->AddRef();
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::~CUIDMContainerHandler
//
//  Synopsis:
//      Destructor of the CUIDMContainerHandler class
//
//------------------------------------------------------------------------
CUIDMContainerHandler::~CUIDMContainerHandler()
{
#ifdef DMCH_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(L"DMCH[0x%p]:  ~CUIDMContainerHandler - destructor.\r\n", this));
#endif // DMCH_DBG

    if (m_inputServices)
    {
        VERIFYHR(m_inputServices->NotifyReleaseManipulationContainer(m_pUIElement));

        m_inputServices->Release();
        m_inputServices = NULL;
    }

    if (m_pUIElement)
    {
        m_pUIElement->Release();
        m_pUIElement = NULL;
    }
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::Create
//
//  Synopsis:
//      Creates an instance of the CUIDMContainerHandler class
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::Create(
    _Outptr_ CUIDMContainerHandler **ppDMContainerHandler,
    _In_ CInputServices* inputServices,
    _In_ CUIElement* pUIElement)
{
    HRESULT hr = S_OK;
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCPTR(ppDMContainerHandler);
    IFCPTR(inputServices);
    IFCPTR(pUIElement);

    pUIDMContainerHandler = new CUIDMContainerHandler(inputServices, pUIElement);

    *ppDMContainerHandler = pUIDMContainerHandler;
    pUIDMContainerHandler = NULL;

Cleanup:
    delete pUIDMContainerHandler;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::NotifyCanManipulateElements
//
//  Synopsis:
//    Called when the container's ability to manipulate
//    elements has changed
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::NotifyCanManipulateElements(
    _In_ bool fCanManipulateElementsByTouch,
    _In_ bool fCanManipulateElementsNonTouch,
    _In_ bool fCanManipulateElementsWithBringIntoViewport)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->NotifyCanManipulateElements(
        m_pUIElement, fCanManipulateElementsByTouch, fCanManipulateElementsNonTouch, fCanManipulateElementsWithBringIntoViewport));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::NotifyManipulatableElementChanged
//
//  Synopsis:
//    Called when:
//     - originally, when IDirectManipulationContainer.put_Handler is called in order to declare the existing manipulated elements.
//     - afterwards, whenever the list of manipulated elements has changed.
//    pOldManipulatableElement == NULL && pNewManipulatableElement != NULL ==> a new manipulated element is available
//    pOldManipulatableElement != NULL && pNewManipulatableElement == NULL ==> an old manipulated element is gone
//    pOldManipulatableElement != NULL && pNewManipulatableElement != NULL ==> an old manipulated element was replaced with another one
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::NotifyManipulatableElementChanged(
    _In_opt_ CUIElement* pOldManipulatableElement,
    _In_opt_ CUIElement* pNewManipulatableElement)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->NotifyManipulatableElementChanged(
        m_pUIElement, pOldManipulatableElement, pNewManipulatableElement));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::NotifySecondaryContentAdded
//
//  Synopsis:
//    Called when a secondary content is added for the specified
//    manipulatable element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::NotifySecondaryContentAdded(
    _In_opt_ CUIElement* pManipulatableElement,
    _In_ CUIElement* pContentElement,
    _In_ XDMContentType contentType)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->NotifySecondaryContentAdded(
        m_pUIElement, pManipulatableElement, pContentElement, contentType));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::NotifySecondaryContentRemoved
//
//  Synopsis:
//    Called when a secondary content is removed for the specified
//    manipulatable element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::NotifySecondaryContentRemoved(
    _In_opt_ CUIElement* pManipulatableElement,
    _In_ CUIElement* pContentElement)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->NotifySecondaryContentRemoved(
        m_pUIElement, pManipulatableElement, pContentElement));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::NotifyViewportChanged
//
//  Synopsis:
//    Called when one or more viewport characteristic has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::NotifyViewportChanged(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fBoundsChanged,
    _In_ bool fTouchConfigurationChanged,
    _In_ bool fNonTouchConfigurationChanged,
    _In_ bool fConfigurationsChanged,
    _In_ bool fChainedMotionTypesChanged,
    _In_ bool fHorizontalOverpanModeChanged,
    _In_ bool fVerticalOverpanModeChanged,
    _Out_ bool* pfConfigurationsUpdated)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFCPTR_RETURN(pfConfigurationsUpdated);
    *pfConfigurationsUpdated = FALSE;

    IFC_RETURN(m_inputServices->NotifyViewportChanged(
        m_pUIElement,
        pManipulatedElement,
        fInManipulation,
        fBoundsChanged,
        fTouchConfigurationChanged,
        fNonTouchConfigurationChanged,
        fConfigurationsChanged,
        fChainedMotionTypesChanged,
        fHorizontalOverpanModeChanged,
        fVerticalOverpanModeChanged,
        pfConfigurationsUpdated));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::NotifyPrimaryContentChanged
//
//  Synopsis:
//    Called when one or more primary content characteristics have changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::NotifyPrimaryContentChanged(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fLayoutRefreshed,
    _In_ bool fBoundsChanged,
    _In_ bool fHorizontalChanged,
    _In_ bool fVerticalChanged,
    _In_ bool fZoomFactorBoundaryChanged)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->NotifyPrimaryContentChanged(
        m_pUIElement,
        pManipulatedElement,
        fInManipulation,
        fLayoutRefreshed,
        fBoundsChanged,
        fHorizontalChanged,
        fVerticalChanged,
        fZoomFactorBoundaryChanged));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::NotifyPrimaryContentTransformChanged
//
//  Synopsis:
//    Called when one or more primary content transform characteristics
//    have changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::NotifyPrimaryContentTransformChanged(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fTranslationXChanged,
    _In_ bool fTranslationYChanged,
    _In_ bool fZoomFactorChanged)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->NotifyPrimaryContentTransformChanged(
        m_pUIElement, pManipulatedElement, fInManipulation, fTranslationXChanged, fTranslationYChanged, fZoomFactorChanged));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::NotifySnapPointsChanged
//
//  Synopsis:
//    Called when the snap points for the provided motion type have changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::NotifySnapPointsChanged(
    _In_ CUIElement* pManipulatedElement,
    _In_ XDMMotionTypes motionType)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->NotifySnapPointsChanged(
        m_pUIElement, pManipulatedElement, motionType));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::GetPrimaryContentTransform
//
//  Synopsis:
//    Called when the DM container needs access to the latest primary content
//    transform.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::GetPrimaryContentTransform(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fForBringIntoViewport,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& uncompressedZoomFactor,
    _Out_ XFLOAT& zoomFactorX,
    _Out_ XFLOAT& zoomFactorY)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->GetPrimaryContentTransform(
        m_pUIElement, pManipulatedElement, fForBringIntoViewport, translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::BringIntoViewport
//
//  Synopsis:
//    Called when the DM container wants to bring the specified bounds of
//    the manipulated element into the viewport. When fAnimate is True, a
//    DM animation is used. When fSkipDuringTouchContact is True, the
//    operation is skipped when there is at least one contact point and no
//    DManip manipulation was recognized. When fSkipAnimationWhileRunning
//    is True, the fAnimate flag is reset when the viewport status is Running.
//    When fApplyAsManip is False, the view change does not trigger a series
//    of NotifyStateChange calls.
//    When fTransformIsValid is True, the translateX, translateY and zoomFactor
//    parameters can be used.
//    When fIsForMakeVisible is True, the bring-into-viewport request is
//    triggered by a MakeVisible call.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::BringIntoViewport(
    _In_ CUIElement* pManipulatedElement,
    _In_ XRECTF& bounds,
    _In_ XFLOAT translateX,
    _In_ XFLOAT translateY,
    _In_ XFLOAT zoomFactor,
    _In_ bool fTransformIsValid,
    _In_ bool fSkipDuringTouchContact,
    _In_ bool fSkipAnimationWhileRunning,
    _In_ bool fAnimate,
    _In_ bool fApplyAsManip,
    _In_ bool fIsForMakeVisible,
    _Out_ bool* pfHandled)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->BringIntoViewport(
        m_pUIElement, pManipulatedElement, bounds, translateX, translateY, zoomFactor, fTransformIsValid, fSkipDuringTouchContact, fSkipAnimationWhileRunning, fAnimate, fApplyAsManip, fIsForMakeVisible, pfHandled));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_SetConstantVelocities
//
//  Synopsis:
//    Called when the DM container wants to initiate a constant-velocity pan.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::SetConstantVelocities(
    _In_ CUIElement* pManipulatedElement,
    _In_ XFLOAT panXVelocity,
    _In_ XFLOAT panYVelocity)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    IFC_RETURN(m_inputServices->SetConstantVelocities(
        m_pUIElement, pManipulatedElement, panXVelocity, panYVelocity));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_ProcessInputMessage
//
//  Synopsis:
//    Called when the DM container wants the handler to process the current
//    input message, by forwarding it to DirectManipulation.
//    The handler must set the fHandled flag to True if the message was handled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainerHandler::ProcessInputMessage(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool ignoreFlowDirection,
    _Out_ bool& fHandled)
{
    IFCEXPECT_ASSERT_RETURN(m_inputServices);
    IFCEXPECT_ASSERT_RETURN(m_pUIElement);

    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(m_pUIElement);

    IFC_RETURN(m_inputServices->ProcessInputMessageWithDirectManipulation(
        m_pUIElement, pManipulatedElement, ignoreFlowDirection, contentRoot, fHandled));

    return S_OK;
}

// Stops the viewport associated with this DManip container handler and
// provided manipulated element if it's in inertia phase.
_Check_return_ HRESULT
CUIDMContainerHandler::StopInertialViewport(
    _In_ CUIElement* pManipulatedElement,
    _Out_ bool* pHandled)
{
    ASSERT(m_inputServices);
    ASSERT(m_pUIElement);

    IFC_RETURN(m_inputServices->StopInertialViewport(
        m_pUIElement, pManipulatedElement, pHandled));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainerHandler::GetCoreServicesNoRef
//
//  Synopsis:
//    Returns the core services belonging to the CInputServices member.
//
//------------------------------------------------------------------------
CCoreServices* CUIDMContainerHandler::GetCoreServicesNoRef()
{
    return m_inputServices ? m_inputServices->GetCoreServicesNoRef() : NULL;
}
