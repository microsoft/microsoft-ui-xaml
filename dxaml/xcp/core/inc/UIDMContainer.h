// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CUIDMContainer class declaration. This class can be used by any
//    non-Core control such as the ScrollViewer control. It uses 
//    'reverse-PInvokes' to interact with the custom control.

#pragma once

// Uncomment to get DirectManipulation debug traces
// #define DMCNTNR_DBG

class CUIElement;

#include <ComTemplates.h>
#include "DirectManipulationContainerHandler.h"
#include "DirectManipulationContainer.h"

class CUIDMContainer : public ctl::implements<IUnknown>
{
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CUIDMContainer **ppDMContainer, 
        _In_ CCoreServices* pCoreServices, 
        _In_ CUIElement* pUIElement);

    // Used to setup a callback mechanism.
    _Check_return_ HRESULT SetManipulationHandler(
        _In_opt_ IDirectManipulationContainerHandler* pHandler);

    // Used to tell the container if the manipulation handler wants to be
    // aware of manipulation characteristic changes even though no manipulation
    // is in progress.
    _Check_return_ HRESULT SetManipulationHandlerWantsNotifications(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fWantsNotifications);

    // Returns True when the UIElement can potentially have manipulated inner elements.
    _Check_return_ HRESULT GetCanManipulateElements(
        _Out_ bool* pfCanManipulateElementsByTouch,
        _Out_ bool* pfCanManipulateElementsNonTouch,
        _Out_ bool* pfCanManipulateElementsWithBringIntoViewport) const;

    // Called to set the dependency object that is touched when initiating a new touch-based manipulation.
    _Check_return_ HRESULT SetPointedElement(
        _In_ CDependencyObject* pPointedElement);

    // Used to retrieve the potential manipulated element for the given pointed and child elements.
    // The returned ppManipulatedElement must have been advertized through a 
    // IDirectManipulationContainerHandler.NotifyManipulatedElement call.
    // Called when the user puts a finger down on the screen.
    _Check_return_ HRESULT GetManipulatedElement(
        _In_opt_ CDependencyObject* pPointedElement,
        _In_opt_ CUIElement* pChildElement,
        _Outptr_ CUIElement** ppManipulatedElement) const;

    // Called to retrieve information about the manipulated element's viewport
    _Check_return_ HRESULT GetManipulationViewport(
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
        _Out_opt_ XDMOverpanMode* pVerticalOverpanMode) const;

    // Called to retrieve information about the manipulated element's primary content
    _Check_return_ HRESULT GetManipulationPrimaryContent(
        _In_ CUIElement* pManipulatedElement,
        _Out_opt_ XSIZEF* pOffsets,
        _Out_opt_ XRECTF* pBounds,
        _Out_opt_ XDMAlignment* pHorizontalAligment,
        _Out_opt_ XDMAlignment* pVerticalAligment,
        _Out_opt_ XFLOAT* pMinZoomFactor,
        _Out_opt_ XFLOAT* pMaxZoomFactor,
        _Out_opt_ bool* pfIsHorizontalStretchAlignmentTreatedAsNear,
        _Out_opt_ bool* pfIsVerticalStretchAlignmentTreatedAsNear,
        _Out_opt_ bool* pfIsLayoutRefreshed) const;

    // Called to retrieve information about a secondary content
    _Check_return_ HRESULT GetManipulationSecondaryContent(
        _In_ CUIElement* pContentElement,
        _Out_ XSIZEF* pOffsets) const;

    // Called to retrieve information about the manipulated element's primary content transform
    _Check_return_ HRESULT GetManipulationPrimaryContentTransform(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fForInitialTransformationAdjustment,
        _In_ bool fForMargins,
        _Out_opt_ XFLOAT* pTranslationX,
        _Out_opt_ XFLOAT* pTranslationY,
        _Out_opt_ XFLOAT* pZoomFactor) const;

    // Called to retrieve a secondary content's transform
    _Check_return_ HRESULT GetManipulationSecondaryContentTransform(
        _In_ CUIElement* pContentElement,
        _Out_ XFLOAT* pTranslationX,
        _Out_ XFLOAT* pTranslationY,
        _Out_ XFLOAT* pZoomFactor) const;
        
    // Called to retrieve information about the manipulated element's primary content snap points
    _Check_return_ HRESULT GetManipulationSnapPoints(
        _In_ CUIElement* pManipulatedElement,
        _In_ XDMMotionTypes motionType,
        _Out_ bool* pfAreSnapPointsOptional,
        _Out_ bool* pfAreSnapPointsSingle,
        _Out_ bool* pfAreSnapPointsRegular,
        _Out_ XFLOAT* pRegularOffset,
        _Out_ XFLOAT* pRegularInterval,
        _Out_ XUINT32* pcIrregularSnapPoints,
        _Outptr_result_buffer_(*pcIrregularSnapPoints) XFLOAT** ppIrregularSnapPoints,
        _Out_ XDMSnapCoordinate* pSnapCoordinate) const;
        
    // Called to notify of a characteristic change that may affect the
    // manipulability of inner elements.
    _Check_return_ HRESULT NotifyManipulatabilityAffectingPropertyChanged(
        _In_ bool fIsInLiveTree);
    
    // Called to notify of a characteristic change that may affect the
    // alignment of the provided manipulated element.
    _Check_return_ HRESULT NotifyContentAlignmentAffectingPropertyChanged(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fIsForHorizontalAlignment,
        _In_ bool fIsForStretchAlignment,
        _In_ bool fIsStretchAlignmentTreatedAsNear);

    // Called to notify of a manipulation's progress.
    _Check_return_ HRESULT NotifyManipulationProgress(
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
        _In_ bool fIsBringIntoViewportConfigurationActivated);

    // Called to raise the DirectManipulationStarted/Completed events
    _Check_return_ HRESULT NotifyManipulationStateChanged(
        _In_ DirectManipulationState state);

    // Called to notify the manipulation handler that it needs to
    // call IDirectManipulationContainerHandler::BringIntoViewport
    // either to synchronize the DManip primary content transform with XAML when fTransformIsValid==False,
    // or to jump to the provided transform when fTransformIsValid==True.
    // When fTransformIsInertiaEnd==True, the call is made after cancelling inertia and the transform provided
    // was the expected end-of-inertia transform.
    _Check_return_ HRESULT NotifyBringIntoViewportNeeded(
        _In_ CUIElement* pManipulatedElement,
        _In_ XFLOAT translationX = 0.0f,
        _In_ XFLOAT translationY = 0.0f,
        _In_ XFLOAT zoomFactor = 0.0f,
        _In_ bool fTransformIsValid = false,
        _In_ bool fTransformIsInertiaEnd = false);

private:    
    // Constructor
    CUIDMContainer(_In_ CCoreServices* pCoreServices, _In_ CUIElement* pUIElement)
        : m_pCoreServices(pCoreServices)
        , m_pUIElement(pUIElement)
        , m_pDMContainerHandler(NULL)
    {
#ifdef DMCNTNR_DBG
        IGNOREHR(gps->DebugOutputSzNoEndl(L"DMC[0x%p]:   CUIDMContainer - constructor.\r\n", this));
#endif // DMCNTNR_DBG

        XCP_STRONG(&m_pCoreServices);
        ASSERT(pCoreServices);
        pCoreServices->AddRef();

        ASSERT(pUIElement);
        pUIElement->AddRef();
    }

protected:
    // Destructor
    ~CUIDMContainer() override
    {
#ifdef DMCNTNR_DBG
        IGNOREHR(gps->DebugOutputSzNoEndl(L"DMC[0x%p]:   ~CUIDMContainer - destructor.\r\n", this));
#endif // DMCNTNR_DBG

        if (m_pCoreServices)
        {
            m_pCoreServices->Release();
            m_pCoreServices = NULL;
        }

        if (m_pUIElement)
        {
            m_pUIElement->Release();
            m_pUIElement = NULL;
        }

        if (m_pDMContainerHandler)
        {
            m_pDMContainerHandler->Release();
            m_pDMContainerHandler = NULL;
        }
    }

private:    
    CCoreServices* m_pCoreServices;
    CUIElement* m_pUIElement; // CUIElement object associated to this implementation
    IDirectManipulationContainerHandler* m_pDMContainerHandler;
};

