// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    General IDirectManipulationContainer implementation that can be used
//    by any non-Core control such as the ScrollViewer control. It uses
//    'reverse-PInvokes' to interact with the custom control.
//    This is the default implementation used by any UIElement when its
//    m_fIsDirectManipulationContainer flag is set to True.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::Create
//
//  Synopsis:
//      Creates an instance of the CUIDMContainer class
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::Create(
    _Outptr_ CUIDMContainer **ppDMContainer,
    _In_ CCoreServices* pCoreServices,
    _In_ CUIElement* pUIElement)
{
    HRESULT hr = S_OK;
    CUIDMContainer* pUIDMContainer = NULL;

    IFCPTR(ppDMContainer);
    IFCPTR(pCoreServices);
    IFCPTR(pUIElement);

    pUIDMContainer = new CUIDMContainer(pCoreServices, pUIElement);

    *ppDMContainer = pUIDMContainer;
    pUIDMContainer = NULL;

Cleanup:
    delete pUIDMContainer;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::SetManipulationHandler
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::SetManipulationHandler(
    _In_opt_ IDirectManipulationContainerHandler* pHandler)
{
    IFCEXPECT_ASSERT_RETURN(m_pCoreServices);

    if (m_pDMContainerHandler)
    {
        m_pDMContainerHandler->Release();
        m_pDMContainerHandler = NULL;
    }

    if (pHandler)
    {
        pHandler->AddRef();
        m_pDMContainerHandler = pHandler;
    }

    IFC_RETURN(FxCallbacks::UIElement_SetManipulationHandler(
        m_pUIElement,
        m_pDMContainerHandler));

    m_pUIElement->SetIsDirectManipulationContainer(m_pDMContainerHandler != NULL);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::SetManipulationHandlerWantsNotifications
//
//  Synopsis:
//    Used to tell the container if the manipulation handler wants to be
//    aware of manipulation characteristic changes even though no manipulation
//    is in progress.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::SetManipulationHandlerWantsNotifications(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fWantsNotifications)
{
    IFCPTR_RETURN(pManipulatedElement);

    IFCEXPECT_ASSERT_RETURN(m_pCoreServices);

    IFC_RETURN(FxCallbacks::UIElement_SetManipulationHandlerWantsNotifications(
        m_pUIElement,
        pManipulatedElement,
        fWantsNotifications));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::GetCanManipulateElements
//
//  Synopsis:
//    Returns True when the UIElement can potentially have manipulatable
//    inner elements.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::GetCanManipulateElements(
    _Out_ bool* pfCanManipulateElementsByTouch,
    _Out_ bool* pfCanManipulateElementsNonTouch,
    _Out_ bool* pfCanManipulateElementsWithBringIntoViewport) const
{
    IFCPTR_RETURN(pfCanManipulateElementsByTouch);
    IFCPTR_RETURN(pfCanManipulateElementsNonTouch);
    IFCPTR_RETURN(pfCanManipulateElementsWithBringIntoViewport);

    IFCEXPECT_ASSERT_RETURN(m_pCoreServices);

    *pfCanManipulateElementsByTouch = FALSE;
    *pfCanManipulateElementsNonTouch = FALSE;
    *pfCanManipulateElementsWithBringIntoViewport = FALSE;

    IFC_RETURN(FxCallbacks::UIElement_GetCanManipulateElements(
        m_pUIElement,
        pfCanManipulateElementsByTouch,
        pfCanManipulateElementsNonTouch,
        pfCanManipulateElementsWithBringIntoViewport));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::SetPointedElement
//
//  Synopsis:
//    Called to set the dependency object that is touched when initiating
//    a new touch-based manipulation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::SetPointedElement(
    _In_ CDependencyObject* pPointedElement)
{
    IFCPTR_RETURN(pPointedElement);

    IFC_RETURN(FxCallbacks::UIElement_SetPointedElement(
        m_pUIElement,
        pPointedElement));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::GetManipulatedElement
//
//  Synopsis:
//    Used to retrieve the potential manipulated element for the given pointed and child elements.
//    The returned ppManipulatedElement must have been advertized through a
//    IDirectManipulationContainerHandler.NotifyManipulatedElement call.
//    Called when the user puts a finger down on the screen.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::GetManipulatedElement(
    _In_opt_ CDependencyObject* pPointedElement,
    _In_opt_ CUIElement* pChildElement,
    _Outptr_ CUIElement** ppManipulatedElement) const
{
    HRESULT hr = S_OK;
    CUIElement* pManipulatedElement = NULL;

    IFCPTR(ppManipulatedElement);
    *ppManipulatedElement = NULL;

    IFC(FxCallbacks::UIElement_GetManipulatedElement(
        m_pUIElement,
        pPointedElement,
        pChildElement,
        &pManipulatedElement));

    AddRefInterface(pManipulatedElement);
    *ppManipulatedElement = pManipulatedElement;
    pManipulatedElement = NULL;

Cleanup:
    ReleaseInterface(pManipulatedElement);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::GetManipulationViewport
//
//  Synopsis:
//    Called to retrieve information about the manipulated element's viewport
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::GetManipulationViewport(
    _In_ CUIElement* pManipulatedElement,
    _Out_opt_ XRECTF* pBounds,
    _Out_opt_ CMILMatrix* pInputTransform,
    _Out_opt_ XDMConfigurations* pTouchConfiguration,
    _Out_opt_ XDMConfigurations* pNonTouchConfiguration,
    _Out_opt_ XDMConfigurations* pBringIntoViewportConfiguration,
    _Out_opt_ XUINT8* pcConfigurations,
    _Outptr_result_buffer_maybenull_(*pcConfigurations) XDMConfigurations** ppConfigurations,
    _Out_opt_ XDMMotionTypes* pChainedMotionTypes,
    _Out_opt_ XDMOverpanMode* pHorizontalOverpanMode,
    _Out_opt_ XDMOverpanMode* pVerticalOverpanMode) const
{
    IFCPTR_RETURN(pManipulatedElement);

    if (pBounds)
    {
        pBounds->X = pBounds->Y = pBounds->Width = pBounds->Height = 0.0f;
    }
    if (pTouchConfiguration)
    {
        *pTouchConfiguration = XcpDMConfigurationNone;
    }
    if (pNonTouchConfiguration)
    {
        *pNonTouchConfiguration = XcpDMConfigurationNone;
    }
    if (pBringIntoViewportConfiguration)
    {
        *pBringIntoViewportConfiguration = XcpDMConfigurationNone;
    }
    if (pcConfigurations)
    {
        *pcConfigurations = 0;
    }
    if (ppConfigurations)
    {
        *ppConfigurations = NULL;
    }
    if (pChainedMotionTypes)
    {
        *pChainedMotionTypes = XcpDMMotionTypeNone;
    }
    if (pHorizontalOverpanMode)
    {
        *pHorizontalOverpanMode = XcpDMOverpanModeDefault;
    }
    if (pVerticalOverpanMode)
    {
        *pVerticalOverpanMode = XcpDMOverpanModeDefault;
    }

    IFC_RETURN(FxCallbacks::UIElement_GetManipulationViewport(
        m_pUIElement,
        pManipulatedElement,
        pBounds,
        pInputTransform,
        reinterpret_cast<XUINT32*>(pTouchConfiguration),
        reinterpret_cast<XUINT32*>(pNonTouchConfiguration),
        reinterpret_cast<XUINT32*>(pBringIntoViewportConfiguration),
        reinterpret_cast<XUINT32*>(pHorizontalOverpanMode),
        reinterpret_cast<XUINT32*>(pVerticalOverpanMode),
        pcConfigurations,
        reinterpret_cast<XUINT32**>(ppConfigurations),
        reinterpret_cast<XUINT32*>(pChainedMotionTypes)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::GetManipulationPrimaryContent
//
//  Synopsis:
//    Called to retrieve information about the manipulated element's primary content
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::GetManipulationPrimaryContent(
    _In_ CUIElement* pManipulatedElement,
    _Out_opt_ XSIZEF* pOffsets,
    _Out_opt_ XRECTF* pBounds,
    _Out_opt_ XDMAlignment* pHorizontalAligment,
    _Out_opt_ XDMAlignment* pVerticalAligment,
    _Out_opt_ XFLOAT* pMinZoomFactor,
    _Out_opt_ XFLOAT* pMaxZoomFactor,
    _Out_opt_ bool* pfIsHorizontalStretchAlignmentTreatedAsNear,
    _Out_opt_ bool* pfIsVerticalStretchAlignmentTreatedAsNear,
    _Out_opt_ bool* pfIsLayoutRefreshed) const
{
    IFCPTR_RETURN(pManipulatedElement);

    if (pOffsets)
    {
        pOffsets->width = pOffsets->height = 0.0f;
    }
    if (pBounds)
    {
        pBounds->X = pBounds->Y = pBounds->Width = pBounds->Height = 0.0f;
    }
    if (pHorizontalAligment)
    {
        *pHorizontalAligment = XcpDMAlignmentNear;
    }
    if (pVerticalAligment)
    {
        *pVerticalAligment = XcpDMAlignmentNear;
    }
    if (pMinZoomFactor)
    {
        *pMinZoomFactor = 0.0f;
    }
    if (pMaxZoomFactor)
    {
        *pMaxZoomFactor = 0.0f;
    }
    if (pfIsHorizontalStretchAlignmentTreatedAsNear != nullptr)
    {
        *pfIsHorizontalStretchAlignmentTreatedAsNear = FALSE;
    }
    if (pfIsVerticalStretchAlignmentTreatedAsNear != nullptr)
    {
        *pfIsVerticalStretchAlignmentTreatedAsNear = FALSE;
    }
    if (pfIsLayoutRefreshed)
    {
        *pfIsLayoutRefreshed = FALSE;
    }

    IFC_RETURN(FxCallbacks::UIElement_GetManipulationPrimaryContent(
        m_pUIElement,
        pManipulatedElement,
        pOffsets,
        pBounds,
        reinterpret_cast<XUINT32*>(pHorizontalAligment),
        reinterpret_cast<XUINT32*>(pVerticalAligment),
        pMinZoomFactor,
        pMaxZoomFactor,
        pfIsHorizontalStretchAlignmentTreatedAsNear,
        pfIsVerticalStretchAlignmentTreatedAsNear,
        pfIsLayoutRefreshed));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::GetManipulationSecondaryContent
//
//  Synopsis:
//    Called to retrieve information about a secondary content
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::GetManipulationSecondaryContent(
    _In_ CUIElement* pContentElement,
    _Out_ XSIZEF* pOffsets) const
{
    IFCPTR_RETURN(pContentElement);
    IFCPTR_RETURN(pOffsets);

    pOffsets->width = pOffsets->height = 0.0f;

    IFC_RETURN(FxCallbacks::UIElement_GetManipulationSecondaryContent(
        m_pUIElement,
        pContentElement,
        pOffsets));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::GetManipulationPrimaryContentTransform
//
//  Synopsis:
//    Called to retrieve information about the manipulated element's
//    primary content transform
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::GetManipulationPrimaryContentTransform(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fForInitialTransformationAdjustment,
    _In_ bool fForMargins,
    _Out_opt_ XFLOAT* pTranslationX,
    _Out_opt_ XFLOAT* pTranslationY,
    _Out_opt_ XFLOAT* pZoomFactor) const
{
    IFCPTR_RETURN(pManipulatedElement);

    if (pTranslationX)
    {
        *pTranslationX = 0.0f;
    }
    if (pTranslationY)
    {
        *pTranslationY = 0.0f;
    }
    if (pZoomFactor)
    {
        *pZoomFactor = 0.0f;
    }

    IFC_RETURN(FxCallbacks::UIElement_GetManipulationPrimaryContentTransform(
        m_pUIElement,
        pManipulatedElement,
        fInManipulation,
        fForInitialTransformationAdjustment,
        fForMargins,
        pTranslationX,
        pTranslationY,
        pZoomFactor));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::GetManipulationSecondaryContentTransform
//
//  Synopsis:
//    Called to retrieve a secondary content's transform
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::GetManipulationSecondaryContentTransform(
    _In_ CUIElement* pContentElement,
    _Out_ XFLOAT* pTranslationX,
    _Out_ XFLOAT* pTranslationY,
    _Out_ XFLOAT* pZoomFactor) const
{
    IFCPTR_RETURN(pContentElement);
    IFCPTR_RETURN(pTranslationX);
    *pTranslationX = 0.0f;
    IFCPTR_RETURN(pTranslationY);
    *pTranslationY = 0.0f;
    IFCPTR_RETURN(pZoomFactor);
    *pZoomFactor = 1.0f;

    IFC_RETURN(FxCallbacks::UIElement_GetManipulationSecondaryContentTransform(
        m_pUIElement,
        pContentElement,
        pTranslationX,
        pTranslationY,
        pZoomFactor));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::GetManipulationSnapPoints
//
//  Synopsis:
//    Called to retrieve information about the manipulated element's
//    primary content snap points
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::GetManipulationSnapPoints(
    _In_ CUIElement* pManipulatedElement,
    _In_ XDMMotionTypes motionType,
    _Out_ bool* pfAreSnapPointsOptional,
    _Out_ bool* pfAreSnapPointsSingle,
    _Out_ bool* pfAreSnapPointsRegular,
    _Out_ XFLOAT* pRegularOffset,
    _Out_ XFLOAT* pRegularInterval,
    _Out_ XUINT32* pcIrregularSnapPoints,
    _Outptr_result_buffer_(*pcIrregularSnapPoints) XFLOAT** ppIrregularSnapPoints,
    _Out_ XDMSnapCoordinate* pSnapCoordinate) const
{
    IFCPTR_RETURN(pManipulatedElement);

    if (pfAreSnapPointsOptional)
    {
        *pfAreSnapPointsOptional = FALSE;
    }
    if (pfAreSnapPointsSingle)
    {
        *pfAreSnapPointsSingle = FALSE;
    }
    if (pfAreSnapPointsRegular)
    {
        *pfAreSnapPointsRegular = FALSE;
    }
    if (pRegularOffset)
    {
        *pRegularOffset = 0.0f;
    }
    if (pRegularInterval)
    {
        *pRegularInterval = 0.0f;
    }
    if (pcIrregularSnapPoints)
    {
        *pcIrregularSnapPoints = 0;
    }
    if (ppIrregularSnapPoints)
    {
        *ppIrregularSnapPoints = NULL;
    }
    if (pSnapCoordinate)
    {
        *pSnapCoordinate = (motionType == XcpDMMotionTypeZoom) ? XcpDMSnapCoordinateOrigin : XcpDMSnapCoordinateBoundary;
    }

    IFC_RETURN(FxCallbacks::UIElement_GetManipulationSnapPoints(
        m_pUIElement,
        pManipulatedElement,
        motionType,
        pfAreSnapPointsOptional,
        pfAreSnapPointsSingle,
        pfAreSnapPointsRegular,
        pRegularOffset,
        pRegularInterval,
        pcIrregularSnapPoints,
        ppIrregularSnapPoints,
        reinterpret_cast<XUINT32*>(pSnapCoordinate)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::NotifyManipulatabilityAffectingPropertyChanged
//
//  Synopsis:
//    Used when the container needs to reevaluate the value returned by
//    GetCanManipulateElements because a characteristic affecting the
//    manipulability of its children has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::NotifyManipulatabilityAffectingPropertyChanged(
    _In_ bool fIsInLiveTree)
{
    IFCEXPECT_ASSERT_RETURN(m_pCoreServices);

    IFC_RETURN(FxCallbacks::UIElement_NotifyManipulatabilityAffectingPropertyChanged(
        m_pUIElement,
        fIsInLiveTree));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::NotifyContentAlignmentAffectingPropertyChanged
//
//  Synopsis:
//    Used when the container needs to reevaluate the alignment of the
//    provided manipulated element because an alignemnt characteristic
//    has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::NotifyContentAlignmentAffectingPropertyChanged(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fIsForHorizontalAlignment,
    _In_ bool fIsForStretchAlignment,
    _In_ bool fIsStretchAlignmentTreatedAsNear)
{
    IFCEXPECT_ASSERT_RETURN(m_pCoreServices);

    IFCPTR_RETURN(pManipulatedElement);

    IFC_RETURN(FxCallbacks::UIElement_NotifyContentAlignmentAffectingPropertyChanged(
        m_pUIElement,
        pManipulatedElement,
        fIsForHorizontalAlignment,
        fIsForStretchAlignment,
        fIsStretchAlignmentTreatedAsNear));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::NotifyManipulationProgress
//
//  Synopsis:
//    Called to notify the IDirectManipulationContainer implementation
//    of a manipulation's progress.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::NotifyManipulationProgress(
    _In_ CUIElement* pManipulatedElement,
    _In_ DirectManipulationState state,
    _In_ XFLOAT xCumulativeTranslation,
    _In_ XFLOAT yCumulativeTranslation,
    _In_ XFLOAT zCumulativeFactor,
    _In_ XFLOAT xInertiaEndTranslation,
    _In_ XFLOAT yInertiaEndTranslation,
    _In_ XFLOAT zInertiaEndFactor,
    _In_ XFLOAT xCenter,
    _In_ XFLOAT yCenter,
    _In_ bool fIsInertiaEndTransformValid,
    _In_ bool fIsInertial,
    _In_ bool fIsTouchConfigurationActivated,
    _In_ bool fIsBringIntoViewportConfigurationActivated)
{
    IFCEXPECT_ASSERT_RETURN(m_pCoreServices);

    IFCPTR_RETURN(pManipulatedElement);

    IFC_RETURN(FxCallbacks::UIElement_NotifyManipulationProgress(
        m_pUIElement,
        pManipulatedElement,
        static_cast<XUINT32>(state),
        xCumulativeTranslation,
        yCumulativeTranslation,
        zCumulativeFactor,
        xInertiaEndTranslation,
        yInertiaEndTranslation,
        zInertiaEndFactor,
        xCenter,
        yCenter,
        fIsInertiaEndTransformValid, 
        fIsInertial,
        fIsTouchConfigurationActivated,
        fIsBringIntoViewportConfigurationActivated));

    return S_OK;
}

_Check_return_ HRESULT
CUIDMContainer::NotifyManipulationStateChanged(
    _In_ DirectManipulationState state)
{
    IFCEXPECT_ASSERT_RETURN(m_pCoreServices);

    IFC_RETURN(FxCallbacks::UIElement_NotifyManipulationStateChanged(
        m_pUIElement,
        static_cast<XUINT32>(state)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIDMContainer::NotifyBringIntoViewportNeeded
//
//  Synopsis:
//    Called to notify the manipulation handler that it needs to
//    call IDirectManipulationContainerHandler::BringIntoViewport
//    either to synchronize the DManip primary content transform with XAML
//    when fTransformIsValid==False, or to jump to the provided transform
//    when fTransformIsValid==True.
//    When fTransformIsInertiaEnd==True, the call is made after cancelling
//    inertia and the transform provided was the expected end-of-inertia
//    transform.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIDMContainer::NotifyBringIntoViewportNeeded(
    _In_ CUIElement* pManipulatedElement,
    _In_ XFLOAT translationX,
    _In_ XFLOAT translationY,
    _In_ XFLOAT zoomFactor,
    _In_ bool fTransformIsValid,
    _In_ bool fTransformIsInertiaEnd)
{
    IFCEXPECT_ASSERT_RETURN(m_pCoreServices);

    IFCPTR_RETURN(pManipulatedElement);

    IFC_RETURN(FxCallbacks::UIElement_NotifyBringIntoViewportNeeded(
        m_pUIElement,
        pManipulatedElement,
        translationX,
        translationY,
        zoomFactor,
        fTransformIsValid,
        fTransformIsInertiaEnd));

    return S_OK;
}
