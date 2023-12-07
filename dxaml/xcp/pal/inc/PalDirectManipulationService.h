// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Interface:  IPALDirectManipulationService
//  Synopsis:
//    Interface implemented by CDirectManipulationService and consumed by
//    the InputManager, via the CUIDMContainer class.

#ifndef __PAL__DIRECTMANIPULATION__SERVICE
#define __PAL__DIRECTMANIPULATION__SERVICE

struct IXcpDirectManipulationViewportEventHandler;
struct IPALDirectManipulationCompositorService;
interface IDirectManipulationViewport;
class DMDeferredRelease;

struct IPALDirectManipulationService : public IObject
{
    virtual _Check_return_ HRESULT EnsureDirectManipulationManager(_In_ XHANDLE hWindow, _In_ bool fIsForCrossSlideViewports) = 0;
    virtual _Check_return_ HRESULT RegisterViewportEventHandler(_In_opt_ IXcpDirectManipulationViewportEventHandler* pEventHandler) = 0;
    virtual _Check_return_ HRESULT ActivateDirectManipulationManager() = 0;
    virtual _Check_return_ HRESULT DeactivateDirectManipulationManager() = 0;
    virtual _Check_return_ HRESULT EnsureHwnd(HWND hwnd) = 0;

    virtual _Check_return_ HRESULT UnregisterViewport(_In_ IObject* pViewport) = 0;

    virtual _Check_return_ HRESULT ProcessInput(_In_ IObject* pViewport, _In_ XHANDLE hMsg, _In_ MessageMap msgID, _In_ bool fIsSecondaryMessage, _In_ bool fInvertForRightToLeft, _In_ XDMConfigurations activatedConfiguration, _Out_ bool& fHandled) = 0;

    virtual _Check_return_ HRESULT SetContact(_In_ IObject* pViewport, _In_ XUINT32 pointerId, _Out_ bool* pfContactFailure) = 0;
    virtual _Check_return_ HRESULT ReleaseContact(_In_ IObject* pViewport, _In_ XUINT32 pointerId) = 0;
    virtual _Check_return_ HRESULT ReleaseAllContacts(_In_ IObject* pViewport) = 0;

    virtual _Check_return_ HRESULT EnableViewport(_In_ IObject* pViewport, _Out_ bool& fCausedRunningStatus) = 0;
    virtual _Check_return_ HRESULT DisableViewport(_In_ IObject* pViewport) = 0;
    virtual _Check_return_ HRESULT StopViewport(_In_ IObject* pViewport) = 0;

    virtual _Check_return_ HRESULT AddViewportConfiguration(_In_ IObject* pViewport, _In_ bool fIsCrossSlideViewport, _In_ bool fIsDragDrop, _In_ XDMConfigurations configuration) = 0;
    virtual _Check_return_ HRESULT RemoveViewportConfiguration(_In_ IObject* pViewport, _In_ XDMConfigurations configuration) = 0;
    virtual _Check_return_ HRESULT ActivateViewportConfiguration(_In_ IObject* pViewport, _In_ XDMConfigurations configuration, _Out_ bool* activationFailed) = 0;

    virtual _Check_return_ HRESULT ApplyOverpanModes(_In_ IObject* pViewport, _In_ XDMOverpanMode horizontalOverpanMode, _In_ XDMOverpanMode verticalOverpanMode, _In_ XFLOAT zoomScale, _In_ bool fIsStartingNewManipulation, _Out_ bool* pfAreOverpanModesChanged) = 0;

    virtual _Check_return_ HRESULT AddSecondaryContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_ XDMContentType contentType) = 0;
    virtual _Check_return_ HRESULT AddSecondaryContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_ XUINT32 cDefinitions, _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions, _In_ XFLOAT offsetX, _In_ XFLOAT offsetY) = 0;
    virtual _Check_return_ HRESULT RemoveSecondaryContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_opt_ DMDeferredRelease* pDMDeferredRelease = nullptr) = 0;

    virtual _Check_return_ HRESULT AddSecondaryClipContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_ XUINT32 cDefinitions, _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions, _In_ XFLOAT offsetX, _In_ XFLOAT offsetY) = 0;
    virtual _Check_return_ HRESULT RemoveSecondaryClipContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_opt_ DMDeferredRelease* pDMDeferredRelease = nullptr) = 0;

    virtual _Check_return_ HRESULT SetViewportChaining(_In_ IObject* pViewport, _In_ XDMMotionTypes motionTypes) = 0;

    virtual _Check_return_ HRESULT SetViewportInputTransform(_In_ IObject* pViewport, _In_ CMILMatrix* pInputTransform) = 0;

    virtual _Check_return_ HRESULT GetViewportStatus(_In_ IObject* pViewport, _Out_ XDMViewportStatus& status) = 0;

    virtual _Check_return_ HRESULT GetViewportCenterPoint(_In_ IObject* pViewport, _Out_ XFLOAT& centerX, _Out_ XFLOAT& centerY) = 0;

    virtual _Check_return_ HRESULT GetContentInertiaEndTransform(_In_ IObject* pViewport, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& zoomFactor) = 0;

    virtual _Check_return_ HRESULT BringIntoViewport(_In_ IObject* pViewport, _In_ XRECTF& bounds, _In_ bool fAnimate) = 0;

    virtual _Check_return_ HRESULT GetViewportBounds(_In_ IObject* pViewport, _Out_ XRECTF& clientBounds) = 0;
    virtual _Check_return_ HRESULT SetViewportBounds(_In_ IObject* pViewport, _In_ const XRECTF& clientBounds) = 0;

    virtual _Check_return_ HRESULT GetContentBounds(_In_ IObject* pViewport, _Out_ XRECTF& bounds) = 0;
    virtual _Check_return_ HRESULT SetContentBounds(_In_ IObject* pViewport, _In_ XRECTF& bounds) = 0;

    virtual _Check_return_ HRESULT GetPrimaryContentTransform(_In_ IObject* pViewport, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& uncompressedZoomFactor, _Out_ XFLOAT& zoomFactorX, _Out_ XFLOAT& zoomFactorY) = 0;
    virtual _Check_return_ HRESULT GetSecondaryContentTransform(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_ XDMContentType contentType, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& uncompressedZoomFactor, _Out_ XFLOAT& zoomFactorX, _Out_ XFLOAT& zoomFactorY) = 0;
    virtual _Check_return_ HRESULT GetSecondaryClipContentTransform(_In_ IObject* pViewport, _In_ IObject* pSecondaryClipContent, _In_ XDMContentType contentType, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& uncompressedZoomFactor, _Out_ XFLOAT& zoomFactorX, _Out_ XFLOAT& zoomFactorY) = 0;

    virtual _Check_return_ HRESULT SetContentAlignment(_In_ IObject* pViewport, _In_ XDMAlignment alignment, _In_ bool fIsHorizontal) = 0;

    virtual _Check_return_ HRESULT SetPrimaryContentSnapPoints(_In_ IObject* pViewport, _In_ XDMMotionTypes motionType, _In_ XFLOAT offset, _In_ XFLOAT interval) = 0;
    virtual _Check_return_ HRESULT SetPrimaryContentSnapPoints(_In_ IObject* pViewport, _In_ XDMMotionTypes motionType, _In_ XUINT32 cSnapPoints, _In_reads_opt_(cSnapPoints) XFLOAT* pSnapPoints) = 0;
    virtual _Check_return_ HRESULT SetPrimaryContentSnapPointsType(_In_ IObject* pViewport, _In_ XDMMotionTypes motionType, _In_ bool fAreSnapPointsOptional, _In_ bool fAreSnapPointsSingle) = 0;
    virtual _Check_return_ HRESULT SetPrimaryContentSnapPointsCoordinate(_In_ IObject* pViewport, _In_ XDMMotionTypes motionType, _In_ XDMSnapCoordinate coordinate, _In_ XFLOAT origin) = 0;

    virtual _Check_return_ HRESULT SetPrimaryContentZoomBoundaries(_In_ IObject* pViewport, _In_ XFLOAT minZoomFactor, _In_ XFLOAT maxZoomFactor) = 0;
    virtual _Check_return_ HRESULT SetPrimaryContentTransform(_In_ IObject* pViewport, _In_ float translationX, _In_ float translationY, _In_ float zoomFactor) = 0;

    virtual _Check_return_ HRESULT GetCompositorService(_Outptr_ IPALDirectManipulationCompositorService** ppCompositorService) = 0;
    virtual _Check_return_ HRESULT GetCompositorViewport(_In_ IObject* pViewport, _Outptr_ IObject** ppCompositorViewport) = 0;
    virtual _Check_return_ HRESULT GetDirectManipulationViewport(_In_ IObject* pViewport, _Outptr_ IDirectManipulationViewport** ppDirectManipulationViewport) = 0;
    virtual _Check_return_ HRESULT GetCompositorPrimaryContent(_In_ IObject* pViewport, _Outptr_ IObject** ppCompositorContent) = 0;
    virtual _Check_return_ HRESULT GetCompositorSecondaryContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _Outptr_ IObject** ppCompositorSecondaryContent) = 0;
    virtual _Check_return_ HRESULT GetCompositorSecondaryClipContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryClipContent, _Outptr_ IObject** ppCompositorSecondaryClipContent) = 0;
    virtual _Check_return_ HRESULT ResetCompositor() = 0;

    virtual _Check_return_ HRESULT EnsureSharedContentTransform(_In_ ixp::ICompositor* compositor, _In_ IObject* pContent, _In_ XDMContentType contentType, _Outptr_result_nullonfailure_ IUnknown** sharedPrimaryTransform, _Outptr_result_maybenull_ IUnknown** sharedSecondaryTransform) = 0;
    virtual _Check_return_ HRESULT ReleaseSharedContentTransform(_In_ IObject* pContent, _In_ XDMContentType contentType) = 0;
    virtual _Check_return_ HRESULT RemoveSharedContentTransformMapping(_In_ IObject* pContent) = 0;

    // Begin an auto scroll. It will continue until StopAutoScroll is called.
    // motionType can be XcpDMMotionTypePanX or XcpDMMotionTypePanY
    virtual _Check_return_ HRESULT ActivateAutoScroll(
        _In_ IObject* pViewport,
        _In_ XDMMotionTypes motionType,
        _In_ bool autoScrollForward) = 0;

    // Stops the ongoing auto scroll if any.
    virtual _Check_return_ HRESULT StopAutoScroll(
        _In_ IObject* pViewport,
        _In_ XDMMotionTypes motionType) = 0;

    // We only set up a drag drop viewport for the first UIElement that has CanDrag=True up the tree from the contact DO.
    // This is called to skip setting up the viewport for other UIElement with CanDrag=True.
    virtual bool GetHasDragDropViewport() = 0;

    virtual _Check_return_ HRESULT GetDragDropViewport(_Out_ IObject** ppViewport) = 0;

    virtual _Check_return_ HRESULT CreateViewportInteraction(
        _In_ IUnknown* compositor,
        _In_ IObject* viewport,
        _Out_ IUnknown** interaction) = 0;

    virtual _Check_return_ HRESULT RemoveViewportInteraction(
        _In_ IObject* viewport) = 0;
};

#endif //__PAL__DIRECTMANIPULATION__SERVICE
