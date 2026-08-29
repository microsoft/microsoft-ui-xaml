// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Interface:  IDirectManipulationContainerHandler interface
//
//  Synopsis:
//    Interface implemented by CUIDMContainerHandler to handle 
//    'reverse-PInvokes' from non-Core controls that support 
//    DirectManipulation, like the ScrollViewer.

#pragma once

struct IDirectManipulationContainerHandler : public IObject
{
    // Called when the container's ability to manipulate 
    // elements has changed
    virtual _Check_return_ HRESULT NotifyCanManipulateElements(
        _In_ bool fCanManipulateElementsByTouch,
        _In_ bool fCanManipulateElementsNonTouch,
        _In_ bool fCanManipulateElementsWithBringIntoViewport) = 0;

    // Called when:
    //  - originally, when IDirectManipulationContainer.put_Handler is called, in order to declare the existing manipulated elements.
    //  - afterwards, whenever the list of manipulated elements has changed.
    // pOldManipulatableElement == NULL && pNewManipulatableElement != NULL ==> a new manipulated element is available
    // pOldManipulatableElement != NULL && pNewManipulatableElement == NULL ==> an old manipulated element is gone
    // pOldManipulatableElement != NULL && pNewManipulatableElement != NULL ==> an old manipulated element was replaced with another one
    virtual _Check_return_ HRESULT NotifyManipulatableElementChanged(
        _In_opt_ CUIElement* pOldManipulatableElement, 
        _In_opt_ CUIElement* pNewManipulatableElement) = 0;

    // Called when a secondary content is added for the specified manipulatable element.
    virtual _Check_return_ HRESULT NotifySecondaryContentAdded(
        _In_opt_ CUIElement* pManipulatableElement,
        _In_ CUIElement* pContentElement,
        _In_ XDMContentType contentType) = 0;

    // Called when a secondary content is removed for the specified manipulatable element.
    virtual _Check_return_ HRESULT NotifySecondaryContentRemoved(
        _In_opt_ CUIElement* pManipulatableElement,
        _In_ CUIElement* pContentElement) = 0;

    // Called when one or more viewport characteristic has changed.
    virtual _Check_return_ HRESULT NotifyViewportChanged(
        _In_ CUIElement* pManipulatedElement, 
        _In_ bool fInManipulation,
        _In_ bool fBoundsChanged,
        _In_ bool fTouchConfigurationChanged,
        _In_ bool fNonTouchConfigurationChanged,
        _In_ bool fConfigurationsChanged,
        _In_ bool fChainedMotionTypesChanged,
        _In_ bool fHorizontalOverpanModeChanged,
        _In_ bool fVerticalOverpanModeChanged,
        _Out_ bool* pfConfigurationsUpdated) = 0;

    // Called when one or more primary content characteristics have changed.
    virtual _Check_return_ HRESULT NotifyPrimaryContentChanged(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fLayoutRefreshed,
        _In_ bool fBoundsChanged,
        _In_ bool fHorizontalAlignmentChanged,
        _In_ bool fVerticalAlignmentChanged,
        _In_ bool fZoomFactorBoundaryChanged) = 0;

    // Called when one or more primary content transform characteristics have changed.
    virtual _Check_return_ HRESULT NotifyPrimaryContentTransformChanged(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fTranslationXChanged,
        _In_ bool fTranslationYChanged,
        _In_ bool fZoomFactorChanged) = 0;

    // Called when the snap points for the provided motion type have changed.
    virtual _Check_return_ HRESULT NotifySnapPointsChanged(
        _In_ CUIElement* pManipulatedElement,
        _In_ XDMMotionTypes motionType) = 0;

    // Called when the DM container needs access to the latest primary content
    // output transform.
    virtual _Check_return_ HRESULT GetPrimaryContentTransform(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fForBringIntoViewport,
        _Out_ XFLOAT& translationX,
        _Out_ XFLOAT& translationY,
        _Out_ XFLOAT& uncompressedZoomFactor,
        _Out_ XFLOAT& zoomFactorX,
        _Out_ XFLOAT& zoomFactorY) = 0;

    // Called when the DM container wants to bring the specified bounds of
    // the manipulated element into the viewport.
    // When fAnimate is True, a DM animation is used. When fSkipDuringTouchContact is True,
    // the operation is skipped when there is at least one contact point and no DManip 
    // manipulation was recognized. When fSkipAnimationWhileRunning is True, the fAnimate
    // flag is reset when the viewport status is Running.
    // When fApplyAsManip is False, the view change does not trigger a series of NotifyStateChange calls.
    // When fTransformIsValid is True, the translateX, translateY and zoomFactor parameters can be used.
    // When fIsForMakeVisible is True, the bring-into-viewport request is triggered by a MakeVisible call.
    virtual _Check_return_ HRESULT BringIntoViewport(
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
        _Out_ bool* pfHandled) = 0;
        
    // Called when the DM container wants to initiate a constant-velocity pan.
    virtual _Check_return_ HRESULT SetConstantVelocities(
        _In_ CUIElement* pManipulatedElement,
        _In_ XFLOAT panXVelocity,
        _In_ XFLOAT panYVelocity) = 0;
        
    // Called when the DM container wants the handler to process the current
    // input message, by forwarding it to DirectManipulation.
    // The handler must set the bHandled flag to True if the message was handled.
    virtual _Check_return_ HRESULT ProcessInputMessage(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool ignoreFlowDirection,
        _Out_ bool& fHandled) = 0;

    // Stops the viewport associated with this DManip container handler and
    // provided manipulated element if it's in inertia phase.
    virtual _Check_return_ HRESULT StopInertialViewport(
        _In_ CUIElement* pManipulatedElement,
        _Out_ bool* pHandled) = 0;
};
