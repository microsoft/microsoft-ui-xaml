// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CDirectManipulationService class implementation.
//    Exercises the DirectManipulation APIs. One CDirectManipulationService
//    instance is associated with one IDirectManipulationContainer
//    implementation and holds one DM manager.

#pragma once

#include <Microsoft.DirectManipulation.h>
#include <Microsoft.UI.Input.Partner.h>
#include "XcpDirectManipulationViewportEventHandler.h"
#include "XcpAutoLock.h"

class WrappingHelper;
class DirectManipulationServiceSharedState;

class CDirectManipulationService final :
    CXcpObjectBase<IPALDirectManipulationService>, // Interface used to interact with the input manager
    IPALDirectManipulationCompositorService        // Interface used to interact with the compositor
    {
    private:
        // Enumeration used internally for tracking auto-scroll progress.
        enum AutoScrollStatus
        {
            AutoScrollStopped,
            AutoScrollStopping,
            AutoScrollActive
        };

        // Forward declaration.
        class ViewportOverpanReflexes;

        // Interface implementations
    public:
        FORWARD_ADDREF_RELEASE(CXcpObjectBase<IPALDirectManipulationService>);

        // IPALDirectManipulationService interface

        // Creates a DirectManipulation manager for the IslandInputSite if it was not created already.
        _Check_return_ HRESULT EnsureDirectManipulationManager(_In_ IUnknown* pIslandInputSite, _In_ bool fIsForCrossSlideViewports) override;

        // Provides an IXcpDirectManipulationViewportEventHandler implementation that this service can use to provide DM feedback to
        // the input manager.
        _Check_return_ HRESULT RegisterViewportEventHandler(_In_opt_ IXcpDirectManipulationViewportEventHandler* pEventHandler) override;

        // Activates the DirectManipulation manager.
        _Check_return_ HRESULT ActivateDirectManipulationManager() override;

        // Deactivates the DirectManipulation manager.
        _Check_return_ HRESULT DeactivateDirectManipulationManager() override;

        // Ensure we're associated with the correct IslandInputSite for a particular UIElement.
        // UIElements can switch between IslandInputSites if ScrollViewers move between islands or windowed popups.
        _Check_return_ HRESULT EnsureElementIslandInputSite(_In_ IUnknown* pIslandInputSite) override;

        // Removes the viewport from our internal m_mapViewports storage,
        // unhooks the two event listeners and releases the viewport DM interface.
        _Check_return_ HRESULT UnregisterViewport(_In_ IObject* pViewport) override;

        // Forwards a keyboard or mouse input message to DirectManipulation for processing.
        _Check_return_ HRESULT ProcessInput(
            _In_ IObject* pViewport,
            _In_ XHANDLE hMsg,
            _In_ MessageMap msgID,
            _In_ bool fIsSecondaryMessage,
            _In_ bool fInvertForRightToLeft,
            _In_ XDMConfigurations activatedConfiguration,
            _Out_ bool& fHandled) override;

        // Declares a contact ID to DirectManipulation that can potentially start a manipulation.
        _Check_return_ HRESULT SetContact(_In_ IObject* pViewport, _In_ XUINT32 pointerId, _Out_ bool* pfContactFailure) override;

        // Tells DirectManipulation to no longer track the provided contact ID.
        _Check_return_ HRESULT ReleaseContact(_In_ IObject* pViewport, _In_ XUINT32 pointerId) override;

        // Tells DirectManipulation to stop tracking all contact IDs associated with the provided viewport.
        _Check_return_ HRESULT ReleaseAllContacts(_In_ IObject* pViewport) override;

        // Enables the provided viewport if its current status is Building or Disabled.
        _Check_return_ HRESULT EnableViewport(_In_ IObject* pViewport, _Out_ bool& fCausedRunningStatus) override;

        // Disables the provided viewport if its current status is not Building or Disabled.
        _Check_return_ HRESULT DisableViewport(_In_ IObject* pViewport) override;

        // Interrupts the active manipulation for the provided viewport, triggering a status change to Ready.
        _Check_return_ HRESULT StopViewport(_In_ IObject* pViewport) override;

        // Adds a possible configuration to the provided viewport. This config may be used in a future manipulation.
        // Creates an associated DM viewport if needed and adds the duo to the viewport map.
        _Check_return_ HRESULT AddViewportConfiguration(_In_ IObject* pViewport, _In_ bool fIsCrossSlideViewport, _In_ bool fIsDragDrop, _In_ XDMConfigurations configuration) override;

        // Removes an existing configuration for the provided viewport. This config may no longer be used in the future manipulation.
        _Check_return_ HRESULT RemoveViewportConfiguration(_In_ IObject* pViewport, _In_ XDMConfigurations configuration) override;

        // Activates an existing configuration for the provided viewport. The active configuration can be changed during a manipulation.
        _Check_return_ HRESULT ActivateViewportConfiguration(_In_ IObject* pViewport, _In_ XDMConfigurations configuration, _Out_ bool* activationFailed) override;

        // Sets up DM curves as necessary to achieve overpan compression based on the horizontal/vertical overpan modes.
        _Check_return_ HRESULT ApplyOverpanModes(_In_ IObject* pViewport, _In_ XDMOverpanMode horizontalOverpanMode, _In_ XDMOverpanMode verticalOverpanMode, _In_ XFLOAT zoomScale, _In_ bool fIsStartingNewManipulation, _Out_ bool* pfAreOverpanModesChanged) override;

        // Adds a secondary content to the viewport.
        _Check_return_ HRESULT AddSecondaryContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_ XUINT32 cDefinitions, _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions, _In_ XFLOAT offsetX, _In_ XFLOAT offsetY) override;

        // Removes the provided secondary content from the viewport.
        _Check_return_ HRESULT RemoveSecondaryContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_opt_ DMDeferredRelease* pDMDeferredRelease) override;

        // Adds a secondary clip content to the viewport.
        _Check_return_ HRESULT AddSecondaryContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_ XDMContentType contentType) override;

        // Adds a secondary clip content to the viewport.
        _Check_return_ HRESULT AddSecondaryClipContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_ XUINT32 cDefinitions, _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions, _In_ XFLOAT offsetX, _In_ XFLOAT offsetY) override;

        // Removes the provided clip secondary content from the viewport.
        _Check_return_ HRESULT RemoveSecondaryClipContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_opt_ DMDeferredRelease* pDMDeferredRelease = nullptr) override;

        // Pushes a chaining setting to DirectManipulation for a given viewport.
        _Check_return_ HRESULT SetViewportChaining(_In_ IObject* pViewport, _In_ XDMMotionTypes motionTypes) override;

        // Declares an input matrix to DirectManipulation for the provided viewport.
        // This matrix is used in case the viewport has a render transform.
        _Check_return_ HRESULT SetViewportInputTransform(_In_ IObject* pViewport, _In_ CMILMatrix* pInputTransform) override;

        // Accesses the status for the provided viewport. Returns a PAL version of the status.
        _Check_return_ HRESULT GetViewportStatus(_In_ IObject* pViewport, _Out_ XDMViewportStatus& status) override;

        // Accesses the center point of the provided viewport. Returns coordinates in relation to the top left corner of the primary content.
        _Check_return_ HRESULT GetViewportCenterPoint(_In_ IObject* pViewport, _Out_ XFLOAT& centerX, _Out_ XFLOAT& centerY) override;

        // Retrieves the primary content's inertia end transform.
        _Check_return_ HRESULT GetContentInertiaEndTransform(_In_ IObject* pViewport, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& zoomFactor) override;

        // Invokes the DirectManipulation ZoomToRect method to scroll/zoom a section of the primary viewport content into view.
        // The move is instantaneous when fAnimate is False.
        _Check_return_ HRESULT BringIntoViewport(_In_ IObject* pViewport, _In_ XRECTF& bounds, _In_ bool fAnimate) override;

        // Returns the bounds of the viewport in client coordinates.
        _Check_return_ HRESULT GetViewportBounds(_In_ IObject* pViewport, _Out_ XRECTF& clientBounds) override;

        // Pushes new bounds for the provided viewport to DirectManipulation. The provided bounds are in client coordinates.
        _Check_return_ HRESULT SetViewportBounds(_In_ IObject* pViewport, _In_ const XRECTF& clientBounds) override;

        // Returns the bounds of the primary content for the provided viewport. The bounds are in relation to the viewport.
        _Check_return_ HRESULT GetContentBounds(_In_ IObject* pViewport, _Out_ XRECTF& bounds) override;

        // Pushes new primary content bounds for the provided viewport to  DirectManipulation. The provided bounds are in relation to the viewport.
        _Check_return_ HRESULT SetContentBounds(_In_ IObject* pViewport, _In_ XRECTF& bounds) override;

        // Accesses the transformation matrix for the primary content of the provided viewport.
        _Check_return_ HRESULT GetPrimaryContentTransform(_In_ IObject* pViewport, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& uncompressedZoomFactor, _Out_ XFLOAT& zoomFactorX, _Out_ XFLOAT& zoomFactorY) override;

        // Accesses the transformation matrix for the secondary content of the provided viewport & content.
        _Check_return_ HRESULT GetSecondaryContentTransform(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _In_ XDMContentType contentType, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& uncompressedZoomFactor, _Out_ XFLOAT& zoomFactorX, _Out_ XFLOAT& zoomFactorY) override;

        // Accesses the transformation matrix for the secondary clip ontent of the provided viewport & content.
        _Check_return_ HRESULT GetSecondaryClipContentTransform(_In_ IObject* pViewport, _In_ IObject* pSecondaryClipContent, _In_ XDMContentType contentType, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& uncompressedZoomFactor, _Out_ XFLOAT& zoomFactorX, _Out_ XFLOAT& zoomFactorY) override;

        // Pushes either horizontal or vertical alignment for the primary content of the provided viewport to DirectManipulation.
        _Check_return_ HRESULT SetContentAlignment(_In_ IObject* pViewport, _In_ XDMAlignment alignment, _In_ bool fIsHorizontal) override;

        // Pushes regular snap points to DirectManipulation for the provided viewport's primary content and motion type.
        _Check_return_ HRESULT SetPrimaryContentSnapPoints(_In_ IObject* pViewport, _In_ XDMMotionTypes motionType, _In_ XFLOAT offset, _In_ XFLOAT interval) override;

        // Pushes irregular snap points to DirectManipulation for the provided viewport's primary content and motion type.
        _Check_return_ HRESULT SetPrimaryContentSnapPoints(_In_ IObject* pViewport, _In_ XDMMotionTypes motionType, _In_ XUINT32 cSnapPoints, _In_reads_opt_(cSnapPoints) XFLOAT* pSnapPoints) override;

        // Pushes the snap points type to DirectManipulation for the provided viewport and motion type.
        _Check_return_ HRESULT SetPrimaryContentSnapPointsType(_In_ IObject* pViewport, _In_ XDMMotionTypes motionType, _In_ bool fAreSnapPointsOptional, _In_ bool fAreSnapPointsSingle) override;

        // Pushes the snap points coordinate to DirectManipulation for the provided viewport and motion type.
        _Check_return_ HRESULT SetPrimaryContentSnapPointsCoordinate(_In_ IObject* pViewport, _In_ XDMMotionTypes motionType, _In_ XDMSnapCoordinate coordinate, _In_ XFLOAT origin) override;

        // Pushes zoom boundaries for the provided viewport's primary content to DirectManipulation.
        _Check_return_ HRESULT SetPrimaryContentZoomBoundaries(_In_ IObject* pViewport, _In_ XFLOAT minZoomFactor, _In_ XFLOAT maxZoomFactor) override;

        // Pushes translation and zoom transformation values for the viewport's primary content to DirectManipulation.
        _Check_return_ HRESULT SetPrimaryContentTransform(_In_ IObject* pViewport, _In_ float translationX, _In_ float translationY, _In_ float zoomFactor) override;

        // Returns an IPALDirectManipulationCompositorService implementation that the compositor can use to interact with DirectManipulation.
        _Check_return_ HRESULT GetCompositorService(_Outptr_ IPALDirectManipulationCompositorService** ppCompositorService) override;

        // Returns a PAL-friendly ref-counted object representing the provided viewport. That object is used by the compositor in its usage of
        // IPALDirectManipulationCompositorService.
        _Check_return_ HRESULT GetCompositorViewport(_In_ IObject* pViewport, _Outptr_ IObject** ppCompositorViewport) override;

        // Returns the IDirectManipulationViewport for this viewport.
        _Check_return_ HRESULT GetDirectManipulationViewport(_In_ IObject* pViewport, _Outptr_ IDirectManipulationViewport** ppDirectManipulationViewport) override;

        // Returns a PAL-friendly ref-counted object representing the primary content of the provided viewport. That object is used by the compositor in its
        // usage of IPALDirectManipulationCompositorService.
        _Check_return_ HRESULT GetCompositorPrimaryContent(_In_ IObject* pViewport, _Outptr_ IObject** ppCompositorContent) override;

        // Returns a PAL-friendly ref-counted object representing the secondary content of the provided viewport & content. That object is used by the compositor in its
        // usage of IPALDirectManipulationCompositorService.
        _Check_return_ HRESULT GetCompositorSecondaryContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryContent, _Outptr_ IObject** ppCompositorSecondaryContent) override;

        // Returns a PAL-friendly ref-counted object representing the secondary clip content of the provided viewport & content. That object is used by the compositor in its
        // usage of IPALDirectManipulationCompositorService.
        _Check_return_ HRESULT GetCompositorSecondaryClipContent(_In_ IObject* pViewport, _In_ IObject* pSecondaryClipContent, _Outptr_ IObject** ppCompositorSecondaryClipContent) override;

        _Check_return_ HRESULT ResetCompositor() override;

        _Check_return_ HRESULT EnsureSharedContentTransform(_In_ ixp::ICompositor* compositor, _In_ IObject* pContent, _In_ XDMContentType contentType, _Outptr_result_nullonfailure_ IUnknown** sharedPrimaryTransform, _Outptr_result_maybenull_ IUnknown** sharedSecondaryTransform) override;

        _Check_return_ HRESULT ReleaseSharedContentTransform(_In_ IObject* pContent, _In_ XDMContentType contentType) override;
        _Check_return_ HRESULT RemoveSharedContentTransformMapping(_In_ IObject* pContent) override;

        // Begin an auto-scroll. It will continue until StopAutoScroll is called.
        // motionType can be XcpDMMotionTypePanX or XcpDMMotionTypePanY
        _Check_return_ HRESULT ActivateAutoScroll(
            _In_ IObject* pViewport,
            _In_ XDMMotionTypes motionType,
            _In_ bool autoScrollForward) override;

        // Stops the ongoing auto-scroll if any.
        _Check_return_ HRESULT StopAutoScroll(
            _In_ IObject* pViewport,
            _In_ XDMMotionTypes motionType) override;

        // IPALDirectManipulationCompositorService interface

        // Returns the content's transform info given its PAL-friendly handle.
        // deltaCompositionTime is the lapse of time in milliseconds between the time this call is made and the time the resulting transform is shown on screen
        _Check_return_ HRESULT GetCompositorContentTransform(_In_ IObject* pCompositorContent, _In_ XDMContentType contentType, _Out_ bool& fIsInertial, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& uncompressedZoomFactor, _Out_ XFLOAT& zoomFactorX, _Out_ XFLOAT& zoomFactorY) override;

        // Even if no compositor node exists for a DM content, DM needs to be ticked in inertia mode. UpdateCompositorContentTransform is called for that purpose
        _Check_return_ HRESULT UpdateCompositorContentTransform(_In_ IObject* pCompositorContent, _In_ XUINT32 deltaCompositionTime) override;

        // Return a unique key value associated with the underlying DM viewport.
        _Check_return_ HRESULT GetCompositorViewportKey(_In_ IObject* pCompositorViewport, _Out_ XHANDLE* pKey) override;

        // Return the status of the underlying DM viewport.
        _Check_return_ HRESULT GetCompositorViewportStatus(_In_ IObject* pCompositorViewport, _Out_ XDMViewportStatus* pStatus) override;

        // We only set up a drag drop viewport for the first UIElement that has CanDrag=True up the tree from the contact DO.
        // This is called to skip setting up the viewport for other UIElement with CanDrag=True.
        bool GetHasDragDropViewport() override { return m_spDragDropViewport != nullptr; }

        _Check_return_ HRESULT GetDragDropViewport(_Outptr_result_maybenull_ IObject** ppViewport) override;

// Public Methods
public:
    static _Check_return_ HRESULT Create(
        std::shared_ptr<DirectManipulationServiceSharedState> sharedState,
        _Outptr_ CDirectManipulationService **ppDMService);

    // Called by CDirectManipulationViewportEventHandler when a viewport's status changed.
    // Forwards the viewport status update to the IXcpDirectManipulationViewportEventHandler implementation.
    _Check_return_ HRESULT NotifyViewportStatusUpdate(
        _In_ IDirectManipulationViewport* pDMViewport,
        _In_ DIRECTMANIPULATION_STATUS oldStatus,
        _In_ DIRECTMANIPULATION_STATUS currentStatus);

    // Forwards the viewport status update to the IXcpDirectManipulationViewportEventHandler implementation.
    _Check_return_ HRESULT NotifyViewportStatusUpdate(
        _In_ IObject* pViewport,
        _In_ XDMViewportStatus oldStatus,
        _In_ XDMViewportStatus currentStatus);

    // Called by CDirectManipulationViewportEventHandler when a viewport's interaction type changed.
    // Forwards the viewport interaction type update to the IXcpDirectManipulationViewportEventHandler implementation.
    _Check_return_ HRESULT NotifyViewportInteractionTypeUpdate(
        _In_ IDirectManipulationViewport* pDMViewport,
        _In_ DIRECTMANIPULATION_INTERACTION_TYPE newInteractionType);

    // Forwards the viewport interaction type update to the IXcpDirectManipulationViewportEventHandler implementation.
    _Check_return_ HRESULT NotifyViewportInteractionTypeUpdate(
        _In_ IObject* pViewport,
        _In_ XDMViewportInteractionType newInteractionType);

    // Called by CDirectManipulationViewportEventHandler when drag drop status changes on the viewport.
    _Check_return_ HRESULT NotifyViewportDraggingStatusChange(
        _In_ IDirectManipulationViewport* pDMViewport,
        _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS current,
        _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS previous);

    // Called by CDirectManipulationCompositor::GetNextFrameInfo when new transform values are provided to the compositor.
    XUINT32 GetDeltaCompositionTime()
    {
        return m_deltaCompositionTime;
    }

    // Creates a lifted IExpDirectManipulationManager. Fails fast.
    static wrl::ComPtr<IExpDirectManipulationManager> CreateDirectManipulationManager();

// Private Methods
private:
    // Constructor
    CDirectManipulationService(std::shared_ptr<DirectManipulationServiceSharedState> sharedState)
        : m_sharedState(std::move(sharedState))
        , m_fManagerActive(FALSE)
        , m_islandInputSite(nullptr)
        , m_activeHwnd(nullptr)
        , m_pViewportEventHandler(NULL)
        , m_pDMManager(NULL)
        , m_pDMUpdateManager(NULL)
        , m_pDMFrameInfoProvider(NULL)
        , m_pDMCompositor(NULL)
        , m_deltaCompositionTime(0)
        , m_pUIThreadViewportEventHandler(NULL)
        , m_pMapSecondaryContents(NULL)
        , m_pMapSecondaryClipContents(NULL)
        , m_currentManipulationStatus((DIRECTMANIPULATION_STATUS)XcpDMViewportBuilding)
        , m_pWrappingHelper(NULL)
        , m_autoScrollBehaviorCookie(0)
        , m_dragDropBehaviorCookie(0)
        , m_autoScrollStatus(AutoScrollStopped)
        , m_cAutoScrollActivations(0)
        , m_isPrimarySharedTransformForOverpanReflexes(false)
        , m_isTopHeaderSharedTransformForOverpanReflexes(false)
        , m_isLeftHeaderSharedTransformForOverpanReflexes(false)
    {
    }

    // Destructor
    ~CDirectManipulationService() override;

    // Creates an IDirectManipulationViewportEventHandler event handler
    // dedicated for the UI thread IXcpDirectManipulationViewportEventHandler
    // listener.
    _Check_return_ HRESULT CreateViewportEventHandler();

    // Gets the IDirectManipulationFrameInfoProvider (which provides the GetNextFrameInfo times to DirectManipulation).
    _Check_return_ HRESULT EnsureFrameInfoProvider();

    // Adds a viewport to the internal m_mapViewports storage, addrefs the key and value.
    _Check_return_ HRESULT AddViewport(_In_ IObject* pViewport, _In_ IDirectManipulationViewport* pDMViewport);

    // Creates a IDirectManipulationViewport implementation, the event handlers if needed, and attaches them to the new viewport.
    _Check_return_ HRESULT CreateViewport(_Outptr_ IDirectManipulationViewport** ppDMViewport);

    // Creates a new cross-slide viewport
    _Check_return_ HRESULT CreateCrossSlideViewport(_In_ bool fIsDragDrop, _Outptr_ IDirectManipulationViewport** ppDMViewport);

    // Attach IDirectManipulationDragDropBehavior to viewport.
    _Check_return_ HRESULT AttachDragDropBehavior(_In_ IDirectManipulationViewport* pDMViewport);

    // Determines if a viewport handle was previously registered.
    _Check_return_ HRESULT IsViewportHandleRegistered(_In_ IObject* pViewport, _Out_ bool* pfViewportRegistered);

    // Retrieves the IDirectManipulationViewport instance associated to the provided viewport handle.
    _Check_return_ HRESULT GetDMViewportFromHandle(_In_ IObject* pViewport, _Out_ IDirectManipulationViewport** ppDMViewport);

    // Retrieves the viewport handle associated to the provided IDirectManipulationViewport implementation, if it exists.
    _Check_return_ HRESULT GetHandleFromDMViewport(_In_ IDirectManipulationViewport* pDMViewport, _Out_ IObject** ppViewport);

    // Returns the current status of the provided viewport.
    _Check_return_ HRESULT GetViewportStatus(_In_ IDirectManipulationViewport* pDMViewport, _Out_ XDMViewportStatus& status);

    // Converts from a PAL configuration enum value to a Windows DM configuration enum value.
    DIRECTMANIPULATION_CONFIGURATION GetDMConfigurations(_In_ XDMConfigurations configuration);

    // Converts from a PAL snap coordinate enum value to a Windows DM snap coordinate enum value.
    DIRECTMANIPULATION_SNAPPOINT_COORDINATE GetDMSnapCoordinate(_In_ XDMSnapCoordinate coordinate);

    // Returns True when the message must only result in a horizontal pan.
    bool IsWindowsMessageForHorizontalPan(_In_ MessageMap msgID, _In_ WPARAM wParam);

    // Returns True when the message must only result in a vertical pan.
    bool IsWindowsMessageForVerticalPan(_In_ MessageMap msgID, _In_ WPARAM wParam);

    // Returns True when the message must only result in a horizontal or
    // vertical pan depending on the DM config and Ctrl key.
    bool IsWindowsMessageForPan(_In_ MessageMap msgID, _In_ WPARAM wParam);

    // Converts a PAL message ID to a Window message ID
    UINT GetWindowsMessageFromMessageMap(_In_ MessageMap msgID, _In_ bool fIsSecondaryMessage, _Out_ bool& fIsKeyboardInput);

    // Updates the wParam member of a Window message to work around a DM behavior inconsistency.
    WPARAM GetWindowsMessageWParam(_In_ MessageMap msgID, _In_ WPARAM wParam, _In_ bool fInvertForRightToLeft);

    // Returns the closest LONG to the provided XFLOAT.
    LONG GetRoundedLong(_In_ XFLOAT value);

    // Returns the rounded down LONG for the provided XFLOAT, or an exact match if no rounding is necessary.
    LONG GetRoundedDownLong(_In_ XFLOAT value);

    // Returns the rounded up LONG for the provided XFLOAT, or an exact match if no rounding is necessary.
    LONG GetRoundedUpLong(_In_ XFLOAT value);

    DIRECTMANIPULATION_MOTION_TYPES GetDMMotionTypes(_In_ XDMMotionTypes type)
    {
        return static_cast<DIRECTMANIPULATION_MOTION_TYPES>(type);
    }

    XDMViewportStatus GetViewportStatus(_In_ DIRECTMANIPULATION_STATUS status)
    {
        return static_cast<XDMViewportStatus>(status);
    }

    XDMViewportInteractionType GetViewportInteractionType(_In_ DIRECTMANIPULATION_INTERACTION_TYPE interactionType)
    {
        return static_cast<XDMViewportInteractionType>(interactionType);
    }

    // Converts from a PAL alignment enum value to a Windows DM horizontal alignment enum value.
    // Values are identical for now.
    DIRECTMANIPULATION_HORIZONTALALIGNMENT GetDMHorizontalAlignment(_In_ XDMAlignment alignment)
    {
        return static_cast<DIRECTMANIPULATION_HORIZONTALALIGNMENT>(alignment);
    }

    // Converts from a PAL alignment enum value to a Windows DM vertical alignment enum value.
    // Values are identical for now.
    DIRECTMANIPULATION_VERTICALALIGNMENT GetDMVerticalAlignment(_In_ XDMAlignment alignment)
    {
        return static_cast<DIRECTMANIPULATION_VERTICALALIGNMENT>(alignment);
    }

    // Returns True when the provided content type is positioned based on overpan reflexes.
    bool IsContentTypeAffectedByOverpanReflexes(_In_ XDMContentType contentType);

    // Determines, for the given content type, if we should create a shared DManip transform that
    // takes overpan reflexes into account
    bool ShouldUseOverpanReflexesForContentType(_In_ XDMContentType contentType);

    // Determines, for a given type, if we should delete a shared DManip transform that takes overpan reflexes into account.
    // For a primary content for instance, when m_isOverpanReflexesUseValidForPrimary is True,
    // m_mapViewportOverpanReflexes.Count() > 0 indicates that a shared DManip transform for overpan reflexes was used.
    // When m_isOverpanReflexesUseValidForPrimary is False, m_mapViewportOverpanReflexes.Count() > 0 indicates
    // that no reflexes were used, but new ones were just created.
    bool DidUseOverpanReflexesForContentType(_In_ XDMContentType contentType);

    // Helper function to create DManip shared transform for the given DManip content
    _Check_return_ HRESULT CreateSharedContentTransformForContent(
        _In_ ixp::ICompositor* pDCompDevice,
        _In_ IObject* pContent,
        _Outptr_result_nullonfailure_ IUnknown** ppSharedTransform);

    // Release the DManip shared transform for the given DManip content
    _Check_return_ HRESULT ReleaseSharedContentTransformForContent(_In_ IObject* pContent);

    // Helper function to create DManip shared transform containing overpan reflexes
    _Check_return_ HRESULT CreateSharedContentTransformsForOverpanReflex(
        _In_ ixp::ICompositor* compositor,
        _In_ XDMContentType contentType,
        _Outptr_result_nullonfailure_ IUnknown** ppPrimarySharedTransform,
        _Outptr_result_nullonfailure_ IUnknown** ppSecondarySharedTransform);

    // Release the DManip shared transform for overpan reflexes
    _Check_return_ HRESULT ReleaseSharedContentTransformForOverpanReflex(_In_ XDMContentType contentType);

    // Sets up secondary content according to its content type.
    _Check_return_ HRESULT SetupSecondaryContent(
        _In_ IDirectManipulationContent* pDMSecondaryContent,
        _In_ XDMContentType contentType);

    // Sets up secondary content according to its content type.
    _Check_return_ HRESULT SetupSecondaryContent(
        _In_ IDirectManipulationContent* pDMSecondaryContent,
        _In_ XUINT32 cDefinitions,
        _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions,
        _In_ XFLOAT offsetX,
        _In_ XFLOAT offsetY);

    DIRECTMANIPULATION_MOTION_TYPES GetMotionTypeFromDMProperty(_In_ XDMProperty dmProperty);

    // Reevaluates overpan curves and calls ApplyCurves() to apply the curves to DM.
    // Call this method whenever curve parameters (content bounds, viewport bounds, overpan modes)
    // are changed or when the curves are first applied.
    //
    // NOTE: ApplyCurves() is called individually for each curve, so lock any call to
    // this method if the curve updates need to be processed atomically (i.e. when first applied).
    _Check_return_ HRESULT RefreshOverpanCurves(
        _In_ IObject* pViewport);

    _Check_return_ HRESULT CreateParametricBehavior(
        _In_ IDirectManipulationManager2* pManager2,
        _In_ IDirectManipulationViewport2* pDMViewport2,
        _In_ ViewportOverpanReflexes* pReflexes);

    _Check_return_ HRESULT CreateParametricReflex(
        _In_ IDirectManipulationManager2* pManager2,
        _In_ IDirectManipulationViewport2* pDMViewport2,
        _Outptr_result_nullonfailure_ IDirectManipulationContent** ppReflex);

    // Applies the scale and centerpoint correction curves to the primary reflex.
    _Check_return_ HRESULT ApplyPrimaryReflexCurves(
        _In_ IDirectManipulationContent* pReflex,
        _In_ XDMContentType contentType,
        _In_ XDMOverpanMode horizontalOverpanMode,
        _In_ XDMOverpanMode verticalOverpanMode,
        _In_ XFLOAT centerpointOffset,
        _In_ RECT viewportBounds,
        _In_ RECT contentBounds);

    // Applies the overpan suppresion curves to the secondary reflex.
    _Check_return_ HRESULT ApplySecondaryReflexCurves(
        _In_ IDirectManipulationContent* pReflex,
        _In_ XDMContentType contentType,
        _In_ XDMOverpanMode horizontalOverpanMode,
        _In_ XDMOverpanMode verticalOverpanMode,
        _In_ RECT viewportBounds,
        _In_ RECT contentBounds);

    // Turns off built-in DM overpan behavior, a necessary first step before adding custom overpan behavior.
    _Check_return_ HRESULT ApplyOverpanDefaultOverrideCurves(
        _In_ IExpDirectManipulationParametricMotionBehavior* pDefaultOverrideBehavior,
        _In_ DWORD targetPropertyType,
        _In_ DWORD sourcePropertyType);

    // Prevents content from overpanning.
    // This is necessary when the default DM overpan behavior is turned off; otherwise, the
    // content will be allowed to freely pan to infinity with no notion of pannable limits.
    // Part of the secondary reflex.
    _Check_return_ HRESULT ApplyOverpanTranslationSuppressionCurves(
        _In_ IExpDirectManipulationParametricMotionBehavior* pOverpanTranslationSuppressionBehavior,
        _In_ XFLOAT viewportExtent,
        _In_ XFLOAT contentExtent,
        _In_ XFLOAT contentOffset,
        _In_ XDMOverpanMode overpanMode,
        _In_ DWORD targetPropertyType,
        _In_ DWORD sourcePropertyType);

    _Check_return_ HRESULT ApplyConstantCurve(
        _In_ IExpDirectManipulationParametricMotionBehavior* pBehavior,
        _In_ XFLOAT constantValue,
        _In_ DWORD targetPropertyType,
        _In_ DWORD sourcePropertyType);

    _Check_return_ HRESULT ApplyLinearCurve(
        _In_ IExpDirectManipulationParametricMotionBehavior* pBehavior,
        _In_ XFLOAT slope,
        _In_ DWORD targetPropertyType,
        _In_ DWORD sourcePropertyType);

    _Check_return_ HRESULT GetDMTransform(
        _In_ IDirectManipulationContent* pDMContent,
        _In_ IObject* pViewport,
        _In_ XDMContentType contentType,
        _Out_ XFLOAT& translationX,
        _Out_ XFLOAT& translationY,
        _Out_ XFLOAT& uncompressedZoomFactor,
        _Out_ XFLOAT& zoomFactorX,
        _Out_ XFLOAT& zoomFactorY);

    _Check_return_ HRESULT GetDMTransformFromOverpanReflexes(
        _In_ IDirectManipulationContent* pDMContent,
        _In_ ViewportOverpanReflexes* pReflexes,
        _In_ XDMContentType contentType,
        _Out_writes_(6) XFLOAT* pMatrix);

    _Check_return_ HRESULT CleanupOverpanReflexData(
        _In_ IObject* pViewport,
        _In_ IDirectManipulationViewport2* pDMViewport2);

    _Check_return_ HRESULT RemoveOverpanBehavior(
        _In_ IDirectManipulationViewport2* pDMViewport2,
        _In_ ViewportOverpanReflexes* pReflexes);

    _Check_return_ HRESULT CreateViewportInteraction(
        _In_ IUnknown* compositor,
        _In_ IObject* viewport,
        _Out_ IUnknown** interaction) override;

    _Check_return_ HRESULT RemoveViewportInteraction(
        _In_ IObject* viewport);

    wrl::ComPtr<ixp::IPointerPoint> GetPointerPointFromPointerId(_In_ XUINT32 pointerId);

// Private Data
private:
    std::shared_ptr<DirectManipulationServiceSharedState> m_sharedState;

    // Set to True when the DM manager is currently activated
    bool m_fManagerActive;

    // Island input site associated with this service
    wrl::ComPtr<ixp::IIslandInputSitePartner> m_islandInputSite;

    // Cached for teardown scenarios where the m_islandInputSite's hwnd has already been cleared out
    // Potentially removable after this IXP bug is resolved:
    // https://task.ms/47915632 - Should the InputSiteHwnd cache its internal hwnd on Destroy so that it can return something to callers of get_UnderlyingInputHwnd instead of failing
    HWND m_activeHwnd;

    // Listener used to provide DM feedback to an input manager
    IXcpDirectManipulationViewportEventHandler* m_pViewportEventHandler;

    // DM manager owned by this service
    IExpDirectManipulationManager* m_pDMManager;

    // Update manager used to retrieve latest content transforms
    IDirectManipulationUpdateManager* m_pDMUpdateManager;

    // DM frame information implementation used to provide timing information
    IDirectManipulationFrameInfoProvider* m_pDMFrameInfoProvider;

    // DM Compositor, responsible for managing DManip's DComp resources and sending updates to DComp
    IDirectManipulationCompositor* m_pDMCompositor;

    // Map between content objects and their associated DComp shared transform
    struct SharedTransformParts
    {
        xref_ptr<IUnknown> spPrimarySharedTransform;
        xref_ptr<IUnknown> spSecondarySharedTransform;
    };
    typedef std::map<xref_ptr<IObject>, SharedTransformParts> DCompTransformsMap;
    DCompTransformsMap m_DCompTransformsMap;

    // Event listener dedicated to the UI thread
    IDirectManipulationViewportEventHandler* m_pUIThreadViewportEventHandler;

    // Map between viewport handles (CDMViewport objects) and DM viewport COM objects
    xchainedmap<XHANDLE, IDirectManipulationViewport*> m_mapViewports;

    // Map between viewport handles (CDMViewport objects) and DM secondary content COM objects
    xchainedmap<XHANDLE, xchainedmap<XHANDLE, IDirectManipulationContent*>*>* m_pMapSecondaryContents;

    // Map between viewport handles (CDMViewport objects) and DM secondary clip content COM objects
    xchainedmap<XHANDLE, xchainedmap<XHANDLE, IDirectManipulationContent*>*>* m_pMapSecondaryClipContents;

    // Event cookies representing the UI thread (input manager) event listeners
    xchainedmap<XHANDLE, XDWORD> m_mapUIThreadViewportEventHandlerCookies;

    // Map between viewport handles (CDMViewport objects) and their associated reflexes
    xchainedmap<XHANDLE, ViewportOverpanReflexes*> m_mapViewportOverpanReflexes;

    // Flags that get updated when DManip transforms get created for three content types affected by the
    // requirement of overpan reflexes. A flag is set to True when a transform for overpan reflexes is created
    // so that the correct kind can be released by CDirectManipulationService::ReleaseSharedContentTransform later on.
    bool m_isPrimarySharedTransformForOverpanReflexes;
    bool m_isTopHeaderSharedTransformForOverpanReflexes;
    bool m_isLeftHeaderSharedTransformForOverpanReflexes;

    // Lapse of time in milliseconds between the time the compositor calls UpdateCompositorContentTransform
    // and the time the resulting transform is shown on screen
    XUINT32 m_deltaCompositionTime;

    // Cached copy of the current DM status.
    DIRECTMANIPULATION_STATUS m_currentManipulationStatus;

    // wrapping helper, a reference to the one created in InputManager.
    WrappingHelper* m_pWrappingHelper;

    // Auto-scrolling fields
    Microsoft::WRL::ComPtr<IDirectManipulationAutoScrollBehavior> m_spAutoScrollBehavior;
    DWORD m_autoScrollBehaviorCookie;
    // Tracks the successive phases of an auto-scroll operation.
    AutoScrollStatus m_autoScrollStatus;
    // Tracks the number of ActivateAutoScroll(...) calls so that the correct AutoScrollStopped or AutoScrollActive status can be picked when auto-scroll inertia ends.
    UINT16 m_cAutoScrollActivations;

    // Drag Drop fields
    Microsoft::WRL::ComPtr<IDirectManipulationDragDropBehavior> m_spDragDropBehavior;
    Microsoft::WRL::ComPtr<IDirectManipulationViewport> m_spDragDropViewport;
    DWORD m_dragDropBehaviorCookie;

    class ViewportOverpanReflexes
    {
    public:
        ViewportOverpanReflexes()
            : m_viewportBehaviorCookie(0)
            , m_zoomScale(0.0f)
            , m_horizontalOverpanMode(XcpDMOverpanModeDefault)
            , m_verticalOverpanMode(XcpDMOverpanModeDefault)
            , m_fIsBehaviorRefreshNeeded(FALSE)
        {
        }

    public:
        DWORD m_viewportBehaviorCookie;

        // Represents the zoom scale factor of the root, i.e. RootScale::GetRasterizationScaleForRoot of the manipulated element
        // Passed in from the core layer when the overpan data is created.
        XFLOAT m_zoomScale;

        // Flag that, when set, indicates overpan modes have changed and this change has not yet been processed
        // by creating a new viewport behavior.  If going between default and non-default overpan modes, we need
        // to create and attach a new viewport behavior, but this cannot always be done synchronously when overpan
        // modes are changed because attaching a viewport behavior while in manipulation can cause DM deadlock (WPB 275883).
        bool m_fIsBehaviorRefreshNeeded;

        XDMOverpanMode m_horizontalOverpanMode;
        XDMOverpanMode m_verticalOverpanMode;

        Microsoft::WRL::ComPtr<IExpDirectManipulationParametricMotionBehavior> m_spViewportBehavior;
        Microsoft::WRL::ComPtr<IDirectManipulationContent> m_spContentPrimaryReflex;
        Microsoft::WRL::ComPtr<IDirectManipulationContent> m_spContentSecondaryReflex;
        Microsoft::WRL::ComPtr<IDirectManipulationContent> m_spLeftHeaderPrimaryReflex;
        Microsoft::WRL::ComPtr<IDirectManipulationContent> m_spLeftHeaderSecondaryReflex;
        Microsoft::WRL::ComPtr<IDirectManipulationContent> m_spTopHeaderPrimaryReflex;
        Microsoft::WRL::ComPtr<IDirectManipulationContent> m_spTopHeaderSecondaryReflex;
    };

    // Defines the range of interpolation of scale as a function of translation:
    //  As translation varies from 0px to g_fMaxOverpanDistance, the scale varies from 1 to g_fScaleOverpanValue.
    //  Larger values of g_fMaxOverpanDistance would result in higher-resistance scale overpan curves.
    static const float s_maxOverpanDistance;
    static const float s_scaleOverpanValue;

    // Defines the floating-point adjustment required so that the curve does not overlap.
    // At higher floating point values for the content widths along with higher scale values, the curves may result in overlap and jumps.
    // Allowing g_fMinOverpanDistance as a small adjustment avoids this issue. This is not needed if the content does not support scale.
    static const float s_minOverpanDistance;

    // Defines the scale factor used to calculate the center-point offset from either end of
    // the viewport (left/right or top/bottom) during the scale overpan curve.
    // This scale factor is multiplied by the device height to determine the center-point offset.
    //
    // During left/top overpan, the center-point offset is offset left from the 0.0f left/top.
    // During right/bottom overpan, the center-point offset is offset right from _rcViewport.right/bottom values.
    // Larger values of center-point offset result in lighter resistance in the scale curve:
    // That is, at larger values of the center-point offset, the scale overpan results in larger translation offsets.
    static const float s_centerPointScaleFactor;

    static const float s_curveSuppressionValueForZoom; // 1.0f
    static const float s_curveSuppressionValueForTranslate; // 0.0f
    static const float s_linearCurvePassThroughSlope; // 1.0f

    // Define the valid offset range for the right/bottom curves.
    // Clamping them at 0 prevents negative values and prevents these curves from overlapping
    // with the left/top curves (curve overlap can cause discrete jumps e.g. when zooming in!).
    static float s_range[];

    // Lock for synchronizing changes to the DM overpan behavior and reflexes.
    // A finer-grained per-viewport lock would allow great flexibility, but
    // since collisions are expected to be rare this per-DMService instance
    // lock should suffice.
    // DManipOnDComp_Staging:  This lock can be removed when DManip-on-DComp is finished
    XcpAutoCriticalSection m_overpanReflexesLock;

    wrl::ComPtr<ixp::IExpPointerPointStatics> m_pointerPointStatics;

#ifdef DBG
    bool m_wasInInertiaStatusDbg{ false };
#endif
};
