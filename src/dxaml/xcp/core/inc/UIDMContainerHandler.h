// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CUIDMContainerHandler class declaration. This class can be used by
//    any non-Core control such as the ScrollViewer control. It handles
//    'reverse-PInvokes' from the controls to interact with the InputManager.
//    This is the default implementation of IDirectManipulationContainerHandler
//    used by any UIElement when its m_fIsDirectManipulationContainer
//    flag is set to True.

#pragma once

class CUIElement;
class CInputServices;

#include "DirectManipulationContainerHandler.h"

class CUIDMContainerHandler final : public CXcpObjectBase<IDirectManipulationContainerHandler>
{
    // ------------------------------------------------------------------------
    // CUIDMContainerHandler Public Methods
    // ------------------------------------------------------------------------
public:
    FORWARD_ADDREF_RELEASE(CXcpObjectBase<IDirectManipulationContainerHandler>);

    static _Check_return_ HRESULT Create(
        _Outptr_ CUIDMContainerHandler **ppDMContainerHandler,
        _In_ CInputServices* inputServices,
        _In_ CUIElement* pUIElement);

    // IDirectManipulationContainerHandler implementation

    // Called when the container's ability to manipulate elements has changed
    _Check_return_ HRESULT NotifyCanManipulateElements(
        _In_ bool fCanManipulateElementsByTouch,
        _In_ bool fCanManipulateElementsNonTouch,
        _In_ bool fCanManipulateElementsWithBringIntoViewport) override;

    // Called when:
    //  - originally, when IDirectManipulationContainer.put_Handler is called, in order to declare the existing manipulated elements.
    //  - afterwards, whenever the list of manipulated elements has changed.
    // pOldManipulatableElement == NULL && pNewManipulatableElement != NULL ==> a new manipulated element is available
    // pOldManipulatableElement != NULL && pNewManipulatableElement == NULL ==> an old manipulated element is gone
    // pOldManipulatableElement != NULL && pNewManipulatableElement != NULL ==> an old manipulated element was replaced with another one
    _Check_return_ HRESULT NotifyManipulatableElementChanged(
        _In_opt_ CUIElement* pOldManipulatableElement,
        _In_opt_ CUIElement* pNewManipulatableElement) override;

    // Called when a secondary content is added for the specified manipulatable element.
    _Check_return_ HRESULT NotifySecondaryContentAdded(
        _In_opt_ CUIElement* pManipulatableElement,
        _In_ CUIElement* pContentElement,
        _In_ XDMContentType contentType) override;

    // Called when a secondary content is removed for the specified manipulatable element.
    _Check_return_ HRESULT NotifySecondaryContentRemoved(
        _In_opt_ CUIElement* pManipulatableElement,
        _In_ CUIElement* pContentElement) override;

    // Called when one or more viewport characteristics have changed.
    _Check_return_ HRESULT NotifyViewportChanged(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fBoundsChanged,
        _In_ bool fTouchConfigurationChanged,
        _In_ bool fNonTouchConfigurationChanged,
        _In_ bool fConfigurationsChanged,
        _In_ bool fChainedMotionTypesChanged,
        _In_ bool fHorizontalOverpanModeChanged,
        _In_ bool fVerticalOverpanModeChanged,
        _Out_ bool* pfConfigurationsUpdated) override;

    // Called when one or more primary content characteristics have changed.
    _Check_return_ HRESULT NotifyPrimaryContentChanged(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fLayoutRefreshed,
        _In_ bool fBoundsChanged,
        _In_ bool fHorizontalChanged,
        _In_ bool fVerticalChanged,
        _In_ bool fZoomFactorBoundaryChanged) override;

    // Called when one or more primary content characteristics have changed.
    _Check_return_ HRESULT NotifyPrimaryContentTransformChanged(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fTranslationXChanged,
        _In_ bool fTranslationYChanged,
        _In_ bool fZoomFactorChanged) override;

    // Called when the snap points for the provided motion type have changed.
    _Check_return_ HRESULT NotifySnapPointsChanged(
        _In_ CUIElement* pManipulatedElement,
        _In_ XDMMotionTypes motionType) override;

    // Called when the DM container needs access to the latest primary content
    // output transform.
    _Check_return_ HRESULT GetPrimaryContentTransform(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fForBringIntoViewport,
        _Out_ XFLOAT& translationX,
        _Out_ XFLOAT& translationY,
        _Out_ XFLOAT& uncompressedZoomFactor,
        _Out_ XFLOAT& zoomFactorX,
        _Out_ XFLOAT& zoomFactorY) override;

    // Brings the specified bounds of the content into the viewport.
    // When fAnimate is True, a DM animation is used. When fSkipDuringTouchContact is True,
    // the operation is skipped when there is at least one contact point and no DManip
    // manipulation was recognized. When fSkipAnimationWhileRunning is True, the fAnimate
    // flag is reset when the viewport status is Running.
    // When fApplyAsManip is False, the view change does not trigger a series of NotifyStateChange calls.
    // When fTransformIsValid is True, the translateX, translateY and zoomFactor parameters can be used.
    // When fIsForMakeVisible is True, the bring-into-viewport request is triggered by a MakeVisible call.
    _Check_return_ HRESULT BringIntoViewport(
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
        _Out_ bool* pfHandled) override;

    // Called when the DM container wants to initiate a constant-velocity pan.
    _Check_return_ HRESULT SetConstantVelocities(
        _In_ CUIElement* pManipulatedElement,
        _In_ XFLOAT panXVelocity,
        _In_ XFLOAT panYVelocity) override;

    // Called when the DM container wants the handler to process the current
    // input message, by forwarding it to DirectManipulation.
    // The handler must set the fHandled flag to True if the message was handled.
    _Check_return_ HRESULT ProcessInputMessage(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool ignoreFlowDirection,
        _Out_ bool& fHandled) override;

    // Stops the viewport associated with this DManip container handler and
    // provided manipulated element if it's in inertia phase.
    _Check_return_ HRESULT StopInertialViewport(
        _In_ CUIElement* pManipulatedElement,
        _Out_ bool* pHandled) override;

public:
    CCoreServices* GetCoreServicesNoRef();

private:
    // Constructor
    CUIDMContainerHandler(_In_ CInputServices* inputServices, _In_ CUIElement* pUIElement);

    // Desctructor
    ~CUIDMContainerHandler() override;

private:
    CInputServices* m_inputServices;
    CUIElement* m_pUIElement; // CUIElement object associated to this IDirectManipulationContainerHandler implementation
};

