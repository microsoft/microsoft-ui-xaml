// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a scrollable area that can contain other visible elements.

#pragma once

//#define DM_DEBUG
//#define ANCHORING_DEBUG

#ifdef ANCHORING_DEBUG
#define ANCHORING_DEBUG_TRACE(message, ...) \
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG, message, __VA_ARGS__));
#else
#define ANCHORING_DEBUG_TRACE(message, ...)
#endif

#include "ScrollViewer.g.h"
#include "IScrollOwner.h"
#include <DoubleUtil.h>

// Default physical amount to scroll with Up/Down/Left/Right key
#define ScrollViewerLineDelta (16.0f)

// This value comes from WHEEL_DELTA defined in WinUser.h. It represents the universal default mouse wheel delta.
#define ScrollViewerDefaultMouseWheelDelta (120)

// These macros compute how many integral pixels need to be scrolled based on the viewport size and mouse wheel delta.
// - First the maximum between 48 and 15% of the viewport size is picked.
// - Then that number is multiplied by (mouse wheel delta/120), 120 being the universal default value.
// - Finally if the resulting number is larger than the viewport size, then that viewport size is picked instead.
#define GetVerticalScrollWheelDelta(size, delta)   (DoubleUtil::Min(DoubleUtil::Floor(size.Height), DoubleUtil::Round(delta * DoubleUtil::Max(48.0, DoubleUtil::Round(size.Height * 0.15, 0)) / 120.0, 0)))
#define GetHorizontalScrollWheelDelta(size, delta) (DoubleUtil::Min(DoubleUtil::Floor(size.Width),  DoubleUtil::Round(delta * DoubleUtil::Max(48.0, DoubleUtil::Round(size.Width  * 0.15, 0)) / 120.0, 0)))

// Minimum value of MinZoomFactor, ZoomFactor and MaxZoomFactor
// ZoomFactor can be manipulated to a slightly smaller value, but
// will jump back to 0.1 when the manipulation completes.
#define ScrollViewerMinimumZoomFactor (0.1f)

// Tolerated rounding delta in pixels between requested scroll offset and
// effective value. Used to handle non-DM-driven scrolls.
#define ScrollViewerScrollRoundingTolerance (0.05f)

// Tolerated rounding delta in pixels between requested scroll offset and
// effective value for cases where IScrollInfo is implemented by a
// IManipulationDataProvider provider. Used to handle non-DM-driven scrolls.
#define ScrollViewerScrollRoundingToleranceForProvider (1.0f)

// Delta required between the current scroll offsets and target scroll offsets
// in order to warrant a call to BringIntoViewport instead of
// SetOffsetsWithExtents, SetHorizontalOffset, SetVerticalOffset.
#define ScrollViewerScrollRoundingToleranceForBringIntoViewport (0.001f)

// Tolerated rounding delta in between requested zoom factor and
// effective value. Used to handle non-DM-driven zooms.
#define ScrollViewerZoomExtentRoundingTolerance (0.001f)

// Tolerated rounding delta in between old and new zoom factor
// in DM delta handling.
#define ScrollViewerZoomRoundingTolerance (0.000001f)

// Delta required between the current zoom factor and target zoom factor
// in order to warrant a call to BringIntoViewport instead of ZoomToFactor.
#define ScrollViewerZoomRoundingToleranceForBringIntoViewport (0.00001f)

// When a snap point is within this tolerance of the ScrollViewer's extent
// minus its viewport we nudge the snap point back into place.
#define ScrollViewerSnapPointLocationTolerance (0.0001f)

// If a ScrollViewer is going to reflow around docked CoreInputView occlussions
// by shrinking its viewport, we want to at least guarantee that it will keep
// an appropriate size.
#define ScrollViewerMinHeightToReflowAroundOcclusions (32.0f)

namespace DirectUI
{
    class IsEnabledChangedEventArgs;
    class ScrollContentPresenter;
    class ScrollBar;
    class ScrollEventArgs;
    class DirectManipulationStateChangeHandler;
    interface IManipulationDataProvider;

    // Represents a scrollable area that can contain other visible elements.
    PARTIAL_CLASS(ScrollViewer),
        public IScrollOwner
    {
    private:
        friend class Selector;

        // Grant friend access to ListViewBase, CalendarView and SemanticZoom so they can set
        // m_templatedParentHandlesMouseButton.
        friend class ListViewBase;
        friend class SemanticZoom;
        friend class OrientedVirtualizingPanel;
        friend class CalendarView;

        // Grant friend access to TextBoxView so it can set
        // m_handleScrollInfoWheelEvent.
        // NOTE: This should be removed once there is a better mechanism
        // in place of m_handleScrollInfoWheelEvent.
        friend class TextBoxView;

        // Indicates whether the parent handles mouse buttons itself.
        BOOLEAN m_templatedParentHandlesMouseButton;

        // Indicates whether the parent handles scrolling itself.
        BOOLEAN m_templatedParentHandlesScrolling;

        // Apply our layout adjustments using a storyboard so that we don't stomp over template or user
        // provided values.  When we stop the storyboard, it will restore the previous values.
        TrackerPtr<xaml_animation::IStoryboard> m_trLayoutAdjustmentsForOcclusionsStoryboard;

        // Reference to the template root.
        TrackerPtr<xaml::IFrameworkElement> m_trElementRoot;

        // Reference to the horizontal ScrollBar child.
        TrackerPtr<xaml_primitives::IScrollBar> m_trElementHorizontalScrollBar;

        // Reference to the vertical ScrollBar child.
        TrackerPtr<xaml_primitives::IScrollBar> m_trElementVerticalScrollBar;

        // Reference to the ScrollBar separator child.
        TrackerPtr<xaml::IUIElement> m_tpElementScrollBarSeparator;

        // Manipulatable element exposed through IDirectManipulationContainerHandler
        TrackerPtr<xaml::IUIElement> m_trManipulatableElement;

        // The main scrollable region.
        ctl::WeakRefPtr m_wrScrollInfo;

        // The touched element set during touch-based manipulation initialization.
        ctl::WeakRefPtr m_wrPointedElement;

        // A flag indicating whether we're currently in the measure pass.
        BOOLEAN m_inMeasure;

        // Flags indicating whether the horizontal and vertical ScrollBars are
        // visible.
        xaml::Visibility m_scrollVisibilityX;
        xaml::Visibility m_scrollVisibilityY;

        // Cached copies of the HorizontalOffset and VerticalOffset properties.
        DOUBLE m_xOffset;
        DOUBLE m_yOffset;

        // Minimal values of the HorizontalOffset and VerticalOffset properties.
        DOUBLE m_xMinOffset;
        DOUBLE m_yMinOffset;

        // Pixel-based versions of m_xOffset and m_yOffset for DM support.
        DOUBLE m_xPixelOffset;
        DOUBLE m_yPixelOffset;

        // Pixel-based unbound versions of m_xOffset and m_yOffset.
        DOUBLE m_unboundHorizontalOffset;
        DOUBLE m_unboundVerticalOffset;

        // Cached copies of the ViewportHeight and ViewportWidth properties.
        DOUBLE m_xViewport;
        DOUBLE m_yViewport;

        // Pixel-based versions of m_xViewport and m_yViewport for DM support.
        DOUBLE m_xPixelViewport;
        DOUBLE m_yPixelViewport;

        // Cached copies of the ExtentHeight and ExtentWidth properties.
        DOUBLE m_xExtent;
        DOUBLE m_yExtent;

        // Pixel-based versions of m_xExtent and m_yExtent for DM support.
        DOUBLE m_xPixelExtent;
        DOUBLE m_yPixelExtent;

        // Used by internal controls to apply a pseudo-LayoutTransform
        // to the ScrollViewer.Content element.
        // This layout size is treated by the ScrollContentPresenter as the desiredsize of its child.
        wf::Size m_layoutSize;

        // Latest availableSize provided to MeasureOverride. Used by the inner ScrollContentPresenter
        // when its SizesContentToTemplatedParent property is set to True. That available size replaces
        // infinity as the available size for its Content.
        wf::Size m_latestAvailableSize{};

        // Event registration tokens for events attached to template parts so
        // the handlers can be removed if we apply a new template.
        EventRegistrationToken m_HorizontalScrollToken;
        EventRegistrationToken m_horizontalThumbDragStartedToken;
        EventRegistrationToken m_horizontalThumbDragCompletedToken;
        EventRegistrationToken m_VerticalScrollToken;
        EventRegistrationToken m_verticalThumbDragStartedToken;
        EventRegistrationToken m_verticalThumbDragCompletedToken;
        EventRegistrationToken m_verticalScrollbarPointerEnteredToken;
        EventRegistrationToken m_verticalScrollbarPointerExitedToken;
        EventRegistrationToken m_horizontalScrollbarPointerEnteredToken;
        EventRegistrationToken m_horizontalScrollbarPointerExitedToken;
        EventRegistrationToken m_coreInputViewOcclusionsChangedToken = {};

        // Whether we are in a state where we want to prevent the normal fade-out of the scrolling indicators.
        BOOLEAN m_keepIndicatorsShowing;

        // Whether a pointer is over the scrollbars. If the scrollbar does not exist then the value here will be false.
        BOOLEAN m_isPointerOverVerticalScrollbar;
        BOOLEAN m_isPointerOverHorizontalScrollbar;

        // Whether the first layout pass has completed and the control has been loaded.
        BOOLEAN m_isLoaded;

        // Whether we are currently dragging a thumb
        BOOLEAN m_isDraggingThumb;
        DOUBLE m_horizontalOffsetCached;
        DOUBLE m_verticalOffsetCached;

        // DirectManipulation-related storage

        // ScrollViewer content width and height requested during the latest DirectManipulation feedback processing
        DOUBLE m_contentWidthRequested;
        DOUBLE m_contentHeightRequested;

        // Horizontal and vertical offsets requested during the latest DirectManipulation feedback processing
        DOUBLE m_xPixelOffsetRequested;
        DOUBLE m_yPixelOffsetRequested;

        // These variables are used to keep track of which zoom properties have been updated/coerced, and when to
        // call the corresponding property changed methods.
        uint32_t m_cLevelsFromRootCallForZoom;
        FLOAT m_initialMaxZoomFactor;
        FLOAT m_initialZoomFactor;
        FLOAT m_requestedMaxZoomFactor;
        FLOAT m_requestedZoomFactor;
        FLOAT m_preDirectManipulationOffsetX;
        FLOAT m_preDirectManipulationOffsetY;
        FLOAT m_preDirectManipulationZoomFactor;

        // Value used when the ScrollViewer contains a IManipulationDataProvider while its non-virtualizing alignment is center or far,
        // and the panel in that direction is smaller than the viewport.
        FLOAT m_preDirectManipulationNonVirtualizedTranslationCorrection;

        // Value of the HorizontalScrollMode property when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        xaml_controls::ScrollMode m_currentHorizontalScrollMode;

        // Value of the VerticalScrollMode property when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        xaml_controls::ScrollMode m_currentVerticalScrollMode;

        // Value of the ZoomMode property when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        xaml_controls::ZoomMode m_currentZoomMode;

        // Value of the horizontal scrollbar visibility property when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        xaml_controls::ScrollBarVisibility m_currentHorizontalScrollBarVisibility;

        // Value of the vertical scrollbar visibility property when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        xaml_controls::ScrollBarVisibility m_currentVerticalScrollBarVisibility;

        // Value of the DManip horizontal alignment when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        DMAlignment m_currentHorizontalAlignment;

        // Value of the DManip vertical alignment when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        DMAlignment m_currentVerticalAlignment;

        // Value of the IsHorizontalRailEnabled property when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        BOOLEAN m_currentIsHorizontalRailEnabled;

        // Value of the IsVerticalRailEnabled property when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        BOOLEAN m_currentIsVerticalRailEnabled;

        // Value of the IsScrollInertiaEnabled property when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        BOOLEAN m_currentIsScrollInertiaEnabled;

        // Value of the IsZoomInertiaEnabled property when a DM manip starts. This will be the value used until the end
        // of that manipulation.
        BOOLEAN m_currentIsZoomInertiaEnabled;

        ctl::ComPtr<ValueTypeObservableCollection<FLOAT>> m_spZoomSnapPoints;
        EventRegistrationToken m_ZoomSnapPointsVectorChangedToken;

        // IScrollSnapPointsInfo implementation of the ScrollContentPresenter's content
        TrackerPtr<xaml_primitives::IScrollSnapPointsInfo> m_trScrollSnapPointsInfo;

        // Event handlers for the IPSPI HorizontalSnapPointsChanged/VerticalSnapPointsChanged events
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> m_spHorizontalSnapPointsChangedEventHandler;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> m_spVerticalSnapPointsChangedEventHandler;

        // Event token for the IPSPI HorizontalSnapPointsChanged/VerticalSnapPointsChanged events
        EventRegistrationToken m_HorizontalSnapPointsChangedToken;
        EventRegistrationToken m_VerticalSnapPointsChangedToken;

        // Callback object used by a single listener that wants to be aware of DM state changes
        DirectManipulationStateChangeHandler* m_pDMStateChangeHandler;

        HANDLE m_hManipulationHandler;

        // Last values returned by ScrollViewer::get_CanManipulateElements.
        // Used to determine if IDirectManipulationContainerHandler.NotifyCanManipulateElements
        // needs to be called when a DM-affecting characteristic changes.
        BOOLEAN m_canManipulateElementsByTouch;   // True means user can manipulated content with touch
        BOOLEAN m_canManipulateElementsNonTouch;  // True means user can move content with keyboard or mouse wheel
        BOOLEAN m_canManipulateElementsWithBringIntoViewport;      // True means developer can move content with programmatic call to BringIntoViewport
        BOOLEAN m_canManipulateElementsWithAsyncBringIntoViewport; // True means BringIntoViewport can perform asynchronous view changes

        // Last values returned by ScrollViewer::GetManipulationViewport.
        // Used to determine if IDirectManipulationContainerHandler.NotifyViewportChanged
        // needs to be called when a DM-affecting characteristic changes.
        DMConfigurations m_touchConfiguration;    // DM configuration used for touch-based manipulations
        DMConfigurations m_nonTouchConfiguration; // DM configuration used for keyboard/mouse wheel operations

        // Last alignment values returned by ScrollViewer::GetManipulationPrimaryContent.
        // Used to determine if ScrollViewer::OnContentAlignmentAffectingPropertyChanged()
        // must call OnPrimaryContentAffectingPropertyChanged or not.
        DMAlignment m_activeHorizontalAlignment;
        DMAlignment m_activeVerticalAlignment;

        // Temporary workaround for DManip bug 799346
        FLOAT m_overridingMinZoomFactor;
        FLOAT m_overridingMaxZoomFactor;

        // Stores the DManip viewport state as of the last notification.
        DMManipulationState m_dmanipState;

        // Set to True in the DMManipulationStarting notification processed in NotifyManipulationProgress.
        // Reset to False in the DMManipulationCompleted notification.
        BOOLEAN m_isInDirectManipulation;

        // Set to True when a call to StopInertialManipulation interrupted a manipulation in inertia phase.
        BOOLEAN m_isDirectManipulationStopped;

        // Set to True in the DMManipulationCompleted notification processed in NotifyManipulationProgress.
        // Reset to False in the DMManipulationStarting notification, or in PostDirectManipulationLayoutRefreshed when a layout refresh occurs.
        BOOLEAN m_isInDirectManipulationCompletion;

        // Set to True when a DirectManipulation-driven zoom factor change occurred during
        // the current manipulation.
        BOOLEAN m_isInDirectManipulationZoom;

        // Set to True when a synchronization between the XAML transform and DManip transform is in progress (i.e. XAML is pushing an updated
        // transform to DManip with a ZoomToRect call).
        BOOLEAN m_isInDirectManipulationSync;

        // Set to True when a ChangeView's BringIntoViewport call is in progress.
        BOOLEAN m_isInChangeViewBringIntoViewport;

        // Set to True when a ZoomToFactor call changed the zoom factor and DManip is being updated accordingly.
        BOOLEAN m_isInZoomFactorSync;

        BOOLEAN m_isInConstantVelocityPan;

        BOOLEAN m_isManipulationHandlerInterestedInNotifications;

        // Indicates whether the zoom factor change is triggered by a DM feedback or a programmatic call.
        BOOLEAN m_isDirectManipulationZoomFactorChange;

        // Set to True when the zoom factor is programmatically changed during a DM manipulation
        // Reset at the end of that manipulation
        BOOLEAN m_isDirectManipulationZoomFactorChangeIgnored;

        // Set to True when any unexpected horizontal or vertical
        // offset change must not be reported to the manipulation handler.
        BOOLEAN m_isOffsetChangeIgnored;

        // Set to True when a configurations-affecting property is changed during a manipulation.
        // In this case the new configurations need to to be pushed to the ManipulationHandler
        // once the ongoing manipulation completes.
        BOOLEAN m_areViewportConfigurationsInvalid;

        // Set to True during a manipulation when the manipulability of the elements might be FALSE
        // once the ongoing manipulation completes.
        BOOLEAN m_isCanManipulateElementsInvalid;

        // Set to True during a manipulation when at least one ScrollBar's IsIgnoringUserInput flag needs to be updated
        // once the ongoing manipulation completes.
        BOOLEAN m_isScrollBarIgnoringUserInputInvalid;

        // Whether the pointer left button pressed or not.
        BOOLEAN m_isPointerLeftButtonPressed;

        // In OnPointerReleased, we want to call Focus and Handle the event.
        // If a right-tap is pending on PointerReleased, we must defer these actions
        // to RightTappedUnhandled so context menus function properly.
        BOOLEAN m_shouldFocusOnRightTapUnhandled;

        // Whether the mouse scrolling indicators are currently showing.
        BOOLEAN m_showingMouseIndicators;

        // Whether we are in a state that should show mouse indicators for scrolling (as opposed to panning indicators).
        BOOLEAN m_preferMouseIndicators;

        // Whether we are blocking all indicators (during SeZo switch)
        BOOLEAN m_blockIndicators;

        // Set to True when the respective three fields m_targetHorizontalOffset/m_targetVerticalOffset/m_targetZoomFactor
        // contain up-to-date values.
        BOOLEAN m_isTargetHorizontalOffsetValid;
        BOOLEAN m_isTargetVerticalOffsetValid;
        BOOLEAN m_isTargetZoomFactorValid;

        // Values representing the transform about to be applied - used for the ViewChanging notifications
        DOUBLE m_targetHorizontalOffset;
        DOUBLE m_targetVerticalOffset;
        FLOAT m_targetZoomFactor;

        // Values representing the latest view requested by ChangeView. When no view change is pending, this trio is equal to (-1, -1, -1)
        // When a ViewChanging event is raised, the values go back to -1.
        DOUBLE m_targetChangeViewHorizontalOffset;
        DOUBLE m_targetChangeViewVerticalOffset;
        FLOAT m_targetChangeViewZoomFactor;

        // Values representing the end of inertia transform - used for the ViewChanging notifications
        DOUBLE m_inertiaEndHorizontalOffset;
        DOUBLE m_inertiaEndVerticalOffset;
        FLOAT m_inertiaEndZoomFactor;

        // Indicates whether the inertia end transform above contains valid values that can be exposed in the ViewChanging event
        BOOLEAN m_isInertiaEndTransformValid;

        // Indicates whether the ScrollViewer is operating in inertia mode. Any ViewChanging notification, not matter its trigger,
        // uses m_isInertial to populate its ScrollViewerViewChanging.IsInertial property.
        BOOLEAN m_isInertial;

        // Set to True when we are batching up HorizontalOffset, VerticalOffset & ZoomFactor change notifications
        // into a single ViewChanging event.
        BOOLEAN m_isViewChangingDelayed;

        // Set to True when we are attempting to batch up HorizontalOffset, VerticalOffset & ZoomFactor change notifications
        // into a single ViewChanged event.
        BOOLEAN m_isViewChangedDelayed;

        // Set to True when the latest delayed ViewChanged event uses the IsIntermediate flag.
        BOOLEAN m_isDelayedViewChangedIntermediate;

        // Set to True when any ViewChanged event raised in the HorizontalOffset, VerticalOffset and ZoomFactor property
        // change handlers needs to specify ScrollViewerViewChangedEventArgs.IsIntermediate==True.
        BOOLEAN m_isInIntermediateViewChangedMode;

        // Set to True when a ViewChanged event is raised while the control is in Intermediate mode (i.e. m_isInIntermediateViewChangedMode==True)
        // Determines whether ViewChanged needs to be raised with ScrollViewerViewChangedEventArgs.IsIntermediate==False when
        // m_isInIntermediateViewChangedMode switches back to False.
        BOOLEAN m_isViewChangedRaisedInIntermediateMode;

        // When strictly positive, it prevents the ViewChanging event from being raised. It is delayed and raised
        // when m_iViewChangingDelay reaches 0.
        INT m_iViewChangingDelay;

        // When strictly positive, it prevents the ViewChanged event from being raised. It is delayed and raised
        // when m_iViewChangedDelay reaches 0.
        INT m_iViewChangedDelay;

        // Whether or not the ScrollViewer should mark as Handled the next PointerWheelChanged message.
        // This is a TEMPORARY measure for internal IScrollInfo implementations that want to allow wheel
        // messages to keep routing.
        BOOLEAN m_handleScrollInfoWheelEvent;

        // We cannot invalidate the grandchild directly. So this property is
        // informing that we are invalidating the child so the grandchild can
        // use it.
        BOOLEAN m_inChildInvalidateMeasure;

        // A value indicating whether the ScrollViewer should ignore any input
        // that would be used by a SemanticZoom to navigate between views.
        BOOLEAN m_ignoreSemanticZoomNavigationInput;

        // Set to True when this ScrollViewer successfully marked a header as associated.
        BOOLEAN m_isTopLeftHeaderAssociated;
        BOOLEAN m_isTopHeaderAssociated;
        BOOLEAN m_isLeftHeaderAssociated;

        // Overpan mode for the horizontal direction.
        DMOverpanMode m_horizontalOverpanMode;

        // Overpan mode for the vertical direction.
        DMOverpanMode m_verticalOverpanMode;

        // Work around for ScrollViewer / ScrollContentPresenter bug Windows Blue 358691
        // Used only by Hub to force "Near" vertical alignment
        bool m_isNearVerticalAlignmentForced;

        // Indicates whether ScrollViewer should ignore mouse wheel scroll events (not zoom).
        bool m_arePointerWheelEventsIgnored = false;

        bool m_isRequestBringIntoViewIgnored = false;

        // Indicates whether the NoIndicator visual state has a Storyboard for which a completion event was hooked up.
        bool m_hasNoIndicatorStateStoryboardCompletedHandler = false;

        // Set to True when our layout engine treats a Stretch alignment as a Left/Top alignment.
        // This typically occurs for text controls.
        BOOLEAN m_isHorizontalStretchAlignmentTreatedAsNear;
        BOOLEAN m_isVerticalStretchAlignmentTreatedAsNear;

        // Fields used for scenarios of WPB bugs 261102 and 342668.
        UINT8 m_cForcePanXConfiguration; // GetManipulationConfigurations artificially includes PanX configuration when m_cForcePanXConfiguration > 0
        UINT8 m_cForcePanYConfiguration; // GetManipulationConfigurations artificially includes PanY configuration when m_cForcePanYConfiguration > 0

        // Event Handler for BringIntoViewRequestedEvent
        ctl::EventPtr<UIElementBringIntoViewRequestedEventCallback> m_bringIntoViewRequestedHandler;

    public:
        // A value indicating whether the ScrollViewer should ignore any input
        // whatsoever (so that it does not conflict with the sezo animation).
        BOOLEAN m_inSemanticZoomAnimation;

        // Allow to set focus on ScrollViewer itself. For example, Flyout inner ScrollViewer.
        BOOLEAN m_isFocusableOnFlyoutScrollViewer;

    protected:
        // Reference to the ScrollContentPresenter child.
        TrackerPtr<xaml_controls::IScrollContentPresenter> m_trElementScrollContentPresenter;

    protected:
        // Initializes a new instance of the ScrollViewer class.
        ScrollViewer();

        // Destroys an instance of the ScrollViewer class.
        ~ScrollViewer() override;

        // Supports the IScrollOwner interface.
        _Check_return_ HRESULT QueryInterfaceImpl(
            _In_ REFIID iid,
            _Outptr_ void** ppObject) override;

        // Prepares object's state
        _Check_return_ HRESULT Initialize() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Called when the Content property changes.
        IFACEMETHOD(OnContentChanged)(
            _In_ IInspectable* pOldContent,
            _In_ IInspectable* pNewContent)
            override;

        // IsEnabled property changed handler.
        _Check_return_ HRESULT OnIsEnabledChanged(
            _In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        // Change to the correct visual state for the ScrollViewer.
        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

        virtual _Check_return_ HRESULT UpdateInputPaneOffsetX()
        { RRETURN(S_OK); }
        virtual _Check_return_ HRESULT UpdateInputPaneOffsetY()
        { RRETURN(S_OK); }
        virtual void UpdateInputPaneTransition()
        { }
        virtual BOOLEAN IsRootScrollViewer()
        { return FALSE; }
        virtual BOOLEAN IsRootScrollViewerAllowImplicitStyle()
        { return FALSE; }
        virtual BOOLEAN IsInputPaneShow()
        { return FALSE; }

    public:
        _Check_return_ HRESULT OnTreeParentUpdated(_In_opt_ CDependencyObject *pNewParent, _In_ BOOLEAN isParentAlive) override;

        // Note: The Attached HorizontalScrollBarVisibility and
        // VerticalScrollBarVisibility properties are defined in the
        // ScrollViewerStatics class

        // Gets or sets a value that indicates whether we should
        // ignore bring into view requests or not.
        _Check_return_ HRESULT get_IsRequestBringIntoViewIgnoredImpl(
            _Out_ BOOLEAN* value);
        _Check_return_ HRESULT put_IsRequestBringIntoViewIgnoredImpl(
            _In_ BOOLEAN value);

        // Gets or sets the CanContentRenderOutsideBounds property.
        _Check_return_ HRESULT get_CanContentRenderOutsideBoundsImpl(
            _Out_ BOOLEAN* value);
        _Check_return_ HRESULT put_CanContentRenderOutsideBoundsImpl(
            _In_ BOOLEAN value);

        // Gets or sets a value that indicates whether a horizontal ScrollBar
        // should be displayed.
        _Check_return_ HRESULT get_HorizontalScrollBarVisibilityImpl(
            _Out_ xaml_controls::ScrollBarVisibility* pValue);
        _Check_return_ HRESULT put_HorizontalScrollBarVisibilityImpl(
            _In_ xaml_controls::ScrollBarVisibility value);

        // Gets or sets a value that indicates whether a vertical ScrollBar
        // should be displayed.
        _Check_return_ HRESULT get_VerticalScrollBarVisibilityImpl(
            _Out_ xaml_controls::ScrollBarVisibility* pValue);
        _Check_return_ HRESULT put_VerticalScrollBarVisibilityImpl(
            _In_ xaml_controls::ScrollBarVisibility value);

        // Gets or sets a value that indicates whether we should attempt to handle
        // scroll wheel events or ignore them and let them bubble up.
        _Check_return_ HRESULT get_ArePointerWheelEventsIgnoredImpl(
            _Out_ BOOLEAN* value);
        _Check_return_ HRESULT put_ArePointerWheelEventsIgnoredImpl(
            _In_ BOOLEAN value);

        // Gets or sets a value that indicates whether horizontal scroll chaining
        // must be turned on during a DirectManipulation operation.
        _Check_return_ HRESULT get_IsHorizontalScrollChainingEnabledImpl(
            _Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsHorizontalScrollChainingEnabledImpl(
            _In_ BOOLEAN value);

        // Gets or sets a value that indicates whether vertical scroll chaining
        // must be turned on during a DirectManipulation operation.
        _Check_return_ HRESULT get_IsVerticalScrollChainingEnabledImpl(
            _Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsVerticalScrollChainingEnabledImpl(
            _In_ BOOLEAN value);

        // Gets or sets a value that indicates whether zoom chaining must be
        // turned on during a DirectManipulation operation.
        _Check_return_ HRESULT get_IsZoomChainingEnabledImpl(
            _Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsZoomChainingEnabledImpl(
            _In_ BOOLEAN value);

        // Gets or sets a value that indicates whether horizontal railing must be
        // turned on during a DirectManipulation operation.
        _Check_return_ HRESULT get_IsHorizontalRailEnabledImpl(
            _Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsHorizontalRailEnabledImpl(
            _In_ BOOLEAN value);

        // Gets or sets a value that indicates whether vertical railing must be
        // turned on during a DirectManipulation operation.
        _Check_return_ HRESULT get_IsVerticalRailEnabledImpl(
            _Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsVerticalRailEnabledImpl(
            _In_ BOOLEAN value);

        // Gets or sets a value that indicates whether scroll inertia must be
        // turned on during a DirectManipulation operation.
        _Check_return_ HRESULT get_IsScrollInertiaEnabledImpl(
            _Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsScrollInertiaEnabledImpl(
            _In_ BOOLEAN value);

        // Gets or sets a value that indicates whether zoom inertia must be
        // turned on during a DirectManipulation operation.
        _Check_return_ HRESULT get_IsZoomInertiaEnabledImpl(
            _Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsZoomInertiaEnabledImpl(
            _In_ BOOLEAN value);

        // Gets or sets the horizontal scroll mode property.
        _Check_return_ HRESULT get_HorizontalScrollModeImpl(
            _Out_ xaml_controls::ScrollMode* pValue);
        _Check_return_ HRESULT put_HorizontalScrollModeImpl(
            _In_ xaml_controls::ScrollMode value);

        // Gets or sets the vertical scroll mode property.
        _Check_return_ HRESULT get_VerticalScrollModeImpl(
            _Out_ xaml_controls::ScrollMode* pValue);
        _Check_return_ HRESULT put_VerticalScrollModeImpl(
            _In_ xaml_controls::ScrollMode value);

        // Gets or sets the zoom mode property.
        _Check_return_ HRESULT get_ZoomModeImpl(
            _Out_ xaml_controls::ZoomMode* pValue);
        _Check_return_ HRESULT put_ZoomModeImpl(
            _In_ xaml_controls::ZoomMode value);

        // Gets or sets the IsDeferredScrollingEnabledproperty.
        _Check_return_ HRESULT get_IsDeferredScrollingEnabledImpl(
            _Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsDeferredScrollingEnabledImpl(
            _In_ BOOLEAN value);

        // Gets or sets the BringIntoViewOnFocusChange.
        _Check_return_ HRESULT get_BringIntoViewOnFocusChangeImpl(
            _Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_BringIntoViewOnFocusChangeImpl(
            _In_ BOOLEAN value);

        // Gets the value of the horizontal offset of the content.
        IFACEMETHOD(get_HorizontalOffset)(
            _Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_HorizontalOffset(
            _In_ DOUBLE value) override;

         // Gets the value of the minimal horizontal offset of the content.
        _Check_return_ HRESULT get_MinHorizontalOffset(
            _Out_ DOUBLE* pValue);
        _Check_return_ HRESULT put_MinHorizontalOffset(
            _In_ DOUBLE value);

        // Gets the value of the viewport width of the content.
        IFACEMETHOD(get_ViewportWidth)(
            _Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_ViewportWidth(
            _In_ DOUBLE value) override;

        // Gets the value of the scrollable width of the content.
        IFACEMETHOD(get_ScrollableWidth)(
            _Out_ DOUBLE* pValue) override;

        // Gets a value indicating ExtentWidth.
        IFACEMETHOD(get_ExtentWidth)(
            _Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_ExtentWidth(
            _In_ DOUBLE value) override;

        // Gets the value of the vertical offset of the content.
        IFACEMETHOD(get_VerticalOffset)(
            _Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_VerticalOffset(
            _In_ DOUBLE value) override;

        // Gets the value of the minimal vertical offset of the content.
        _Check_return_ HRESULT get_MinVerticalOffset(
            _Out_ DOUBLE* pValue);
        _Check_return_ HRESULT put_MinVerticalOffset(
            _In_ DOUBLE value);

        // Gets the value of the viewport height of the content.
        IFACEMETHOD(get_ViewportHeight)(
            _Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_ViewportHeight(
            _In_ DOUBLE value) override;

        // Gets the value of the scrollable height of the content.
        IFACEMETHOD(get_ScrollableHeight)(
            _Out_ DOUBLE* pValue) override;

        // Gets a value indicating the ExtentHeight.
        IFACEMETHOD(get_ExtentHeight)(
            _Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_ExtentHeight(
            _In_ DOUBLE value) override;

        // Indicates whether the parent handles scrolling itself.
        _Check_return_ HRESULT get_TemplatedParentHandlesScrolling(
            _Out_ BOOLEAN* pValue);
        void put_TemplatedParentHandlesScrolling(
            _In_ BOOLEAN value);

        // Privately exposed API for determining whether the ScrollViewer
        // is being controlled by an ongoing direct manipulation.
        _Check_return_ HRESULT get_IsInDirectManipulationImpl(
            _Out_ BOOLEAN* pValue);

        // Privately exposed API for determining whether the viewport is
        // actively being manipulated.
        _Check_return_ HRESULT get_IsInActiveDirectManipulationImpl(
            _Out_ BOOLEAN* value);

        // Apply a template to the ScrollViewer.
        IFACEMETHOD(OnApplyTemplate)() noexcept override;

        // Scrolls the view in the specified direction.
        _Check_return_ HRESULT ScrollInDirection(
            _In_ wsy::VirtualKey key,
            _In_ BOOLEAN animate = FALSE);

        // Public and deprecated version of ScrollToHorizontalOffsetInternal.
        // Scrolls the content within the ScrollViewer to the specified
        // horizontal offset position.
        _Check_return_ HRESULT ScrollToHorizontalOffsetImpl(
            _In_ DOUBLE offset) override;

        // Public and deprecated version of ScrollToVerticalOffsetInternal.
        // Scrolls the content within the ScrollViewer to the specified vertical
        // offset position.
        _Check_return_ HRESULT ScrollToVerticalOffsetImpl(
            _In_ DOUBLE offset) override;

        // Combines the abilities of ScrollToHorizontalOffset, ScrollToVerticalOffset
        // and ZoomToFactor. Attempts to animate to the target view and snap to
        // mandatory snap points.
        _Check_return_ HRESULT ChangeViewImpl(
            _In_opt_ wf::IReference<DOUBLE>* pHorizontalOffset,
            _In_opt_ wf::IReference<DOUBLE>* pVerticalOffset,
            _In_opt_ wf::IReference<FLOAT>* pZoomFactor,
            _Out_ BOOLEAN* pReturnValue);

        // Combines the abilities of ScrollToHorizontalOffset, ScrollToVerticalOffset
        // and ZoomToFactor, with the option of animating to the target view and snapping
        // to mandatory snap points.
        _Check_return_ HRESULT ChangeViewWithOptionalAnimationImpl(
            _In_opt_ wf::IReference<DOUBLE>* pHorizontalOffset,
            _In_opt_ wf::IReference<DOUBLE>* pVerticalOffset,
            _In_opt_ wf::IReference<FLOAT>* pZoomFactor,
            _In_ BOOLEAN disableAnimation,
            _Out_ BOOLEAN* pReturnValue);

        // This function is called by an IScrollInfo attached to this
        // ScrollViewer when any values of scrolling properties (Offset, Extent,
        // and ViewportSize) change.  The function schedules invalidation of
        // other elements like ScrollBars that are dependant on these
        // properties.
        _Check_return_ HRESULT InvalidateScrollInfoImpl() noexcept override;

        // Member of the IScrollOwner internal contract. Allows the interface consumer to notify this ScrollViewer
        // that an ArrangeOverride occurred after an IManipulationDataProvider::UpdateInManipulation(...) call.
        // Also called by PostDirectManipulationLayoutRefreshed() when m_isInDirectManipulationCompletion is True.
        _Check_return_ HRESULT NotifyLayoutRefreshed() override;

        // Member of the IScrollOwner internal contract. Allows the interface
        // consumer to notify this ScrollViewer of an attempt to change the
        // horizontal offset. Used for the ViewChanging notifications.
        _Check_return_ HRESULT NotifyHorizontalOffsetChanging(
            _In_ DOUBLE targetHorizontalOffset,
            _In_ DOUBLE targetVerticalOffset) override;

        // Member of the IScrollOwner internal contract. Allows the interface
        // consumer to notify this ScrollViewer of an attempt to change the
        // vertical offset. Used for the ViewChanging notifications.
        _Check_return_ HRESULT NotifyVerticalOffsetChanging(
            _In_ DOUBLE targetHorizontalOffset,
            _In_ DOUBLE targetVerticalOffset) override;

        // Sets reference to the IScrollInfo implementation
        _Check_return_ HRESULT put_ScrollInfo(
            _In_opt_ IScrollInfo* value) override;

        // Gets reference to the IScrollInfo implementation
        _Check_return_ HRESULT get_ScrollInfo(
            _Outptr_result_maybenull_ IScrollInfo** value) override;

        // Scroll content by one page to the left.
        _Check_return_ HRESULT PageLeft();

        // Scroll content by one line to the left.
        _Check_return_ HRESULT LineLeft();

        // Scroll content by one line to the right.
        _Check_return_ HRESULT LineRight();

        // Scroll content by one page to the right
        _Check_return_ HRESULT PageRight();

        // Scroll content by one page to the top.
        _Check_return_ HRESULT PageUp();

        // Scroll content by one line to the top.
        _Check_return_ HRESULT LineUp();

        // Scroll content by one line to the bottom.
        _Check_return_ HRESULT LineDown();

        // Scroll content by one page to the bottom.
        _Check_return_ HRESULT PageDown();

        // Scroll content to the beginning.
        _Check_return_ HRESULT PageHome();

        // Scroll content to the end.
        _Check_return_ HRESULT PageEnd();

        // Measure the content.
        IFACEMETHOD(MeasureOverride)(
            // Measurement constraints, a control cannot return a size
            // larger than the constraint.
            _In_ wf::Size pAvailableSize,
            // The desired size of the control.
            _Out_ wf::Size* pDesired) noexcept override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size finalSize,
            _Out_ wf::Size* returnValue) noexcept override;

        // GotFocus event handler.
        IFACEMETHOD(OnGotFocus)(
              _In_ xaml::IRoutedEventArgs* pArgs) override;

        // KeyDown event handler.
        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        // PointerWheelChanged event handler.
        IFACEMETHOD(OnPointerWheelChanged)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerPressed event handler.
        IFACEMETHOD(OnPointerPressed)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerReleased event handler.
        IFACEMETHOD(OnPointerReleased)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // RightTappedUnhandled event handler.
        // Private event.
        _Check_return_ HRESULT OnRightTappedUnhandled(
            _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;

        // PointerEnter event handler.
        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerMoved event handler.
        IFACEMETHOD(OnPointerMoved)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerExited event handler.
        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // DirectManipulation-related overrides

        _Check_return_ HRESULT get_ZoomSnapPointsImpl(
            _Outptr_ wfc::IVector<FLOAT>** pValue);

        // Override of the MaxZoomFactor property setter needed for coercion
        IFACEMETHOD(put_MaxZoomFactor)(_In_ FLOAT value) override;

        // Override of the ZoomFactor property setter needed for coercion
        _Check_return_ HRESULT put_ZoomFactor(_In_ FLOAT value) override;

        _Check_return_ HRESULT get_ZoomFactorImpl(FLOAT* pValue) override
        {
            RRETURN(get_ZoomFactor(pValue));
        }

        bool IsDraggableOrPannableImpl() override;

        // Updates the zoom factor value. Equivalent of ScrollToHorizontalOffset
        // and ScrollToVerticalOffset for the ZoomFactor dependency property.
        _Check_return_ HRESULT ZoomToFactorImpl(_In_ FLOAT factor);

        // Returns True when the ScrollViewer can potentially manipulate its content
        _Check_return_ HRESULT get_CanManipulateElements(
            _Out_ BOOLEAN* pCanManipulateElementsByTouch,
            _Out_ BOOLEAN* pCanManipulateElementsNonTouch,
            _Out_ BOOLEAN* pCanManipulateElementsWithBringIntoViewport) override;

        // Called to set a manipulation handler used in control-to-InputManager notifications
        _Check_return_ HRESULT put_ManipulationHandler(
            _In_opt_ HANDLE hManipulationHandler) override;

        // Used to tell the container if the manipulation handler wants to be
        // aware of manipulation characteristic changes even though no manipulation
        // is in progress.
        _Check_return_ HRESULT SetManipulationHandlerWantsNotifications(
            _In_ UIElement* pManipulatedElement,
            _In_ BOOLEAN wantsNotifications) override;

        // Caches the dependency object that is touched during the initation of a touch-based manipulation.
        _Check_return_ HRESULT SetPointedElement(
            _In_ DependencyObject* pPointedElement) override;

        // Returns the manipulated element for a given pointed element
        _Check_return_ HRESULT GetManipulatedElement(
            _In_opt_ DependencyObject* pPointedElement,
            _In_opt_ UIElement* pChildElement,
            _Out_ UIElement** ppManipulatedElement) override;

        // Returns information about a manipulated element's viewport
        _Check_return_ HRESULT GetManipulationViewport(
            _In_ UIElement* pManipulatedElement,
            _Out_opt_ XRECTF* pBounds,
            _Out_opt_ CMILMatrix* pInputTransform,
            _Out_opt_ DMConfigurations* pTouchConfiguration,
            _Out_opt_ DMConfigurations* pNonTouchConfiguration,
            _Out_opt_ DMConfigurations* pBringIntoViewportConfiguration,
            _Out_opt_ DMOverpanMode* pHorizontalOverpanMode,
            _Out_opt_ DMOverpanMode* pVerticalOverpanMode,
            _Out_opt_ UINT8* pcConfigurations,
            _Outptr_result_buffer_maybenull_(*pcConfigurations) DMConfigurations** ppConfigurations,
            _Out_opt_ DMMotionTypes* pChainedMotionTypes) override;

        // Returns information about a manipulated element's primary content
        _Check_return_ HRESULT GetManipulationPrimaryContent(
            _In_ UIElement* pManipulatedElement,
            _Out_opt_ XSIZEF* pOffsets,
            _Out_opt_ XRECTF* pBounds,
            _Out_opt_ DMAlignment* pHorizontalAligment,
            _Out_opt_ DMAlignment* pVerticalAligment,
            _Out_opt_ FLOAT* pMinZoomFactor,
            _Out_opt_ FLOAT* pMaxZoomFactor,
            _Out_opt_ BOOLEAN* pIsHorizontalStretchAlignmentTreatedAsNear,
            _Out_opt_ BOOLEAN* pIsVerticalStretchAlignmentTreatedAsNear,
            _Out_opt_ BOOLEAN* pIsLayoutRefreshed) override;

        // Returns information about a secondary content
        _Check_return_ HRESULT GetManipulationSecondaryContent(
            _In_ UIElement* pContentElement,
            _Out_ XSIZEF* pOffsets) override;

        // Returns information about a manipulated element's primary content transform
        _Check_return_ HRESULT GetManipulationPrimaryContentTransform(
            _In_ UIElement* pManipulatedElement,
            _In_ BOOLEAN inManipulation,
            _In_ BOOLEAN forInitialTransformationAdjustment,
            _In_ BOOLEAN forMargins,
            _Out_opt_ FLOAT* pTranslationX,
            _Out_opt_ FLOAT* pTranslationY,
            _Out_opt_ FLOAT* pZoomFactor) override;

        // Returns information about a secondary content's transform
        _Check_return_ HRESULT GetManipulationSecondaryContentTransform(
            _In_ UIElement* pContentElement,
            _Out_ FLOAT* pTranslationX,
            _Out_ FLOAT* pTranslationY,
            _Out_ FLOAT* pZoomFactor) override;

        // Returns the snap points for the provided manipulated element and motion type.
        _Check_return_ HRESULT GetManipulationSnapPoints(
            _In_ UIElement* pManipulatedElement,   // Manipulated element for which the snap points are requested
            _In_ DMMotionTypes motionType,         // Motion type for which the snap points are requested
            _Out_ BOOLEAN* pAreSnapPointsOptional, // Set to True when returned snap points are optional
            _Out_ BOOLEAN* pAreSnapPointsSingle,   // Set to True when returned snap points are single (i.e. breaking inertia)
            _Out_ BOOLEAN* pAreSnapPointsRegular,  // Set to True when returned snap points are equidistant
            _Out_ FLOAT* pRegularOffset,           // Offset of regular snap points
            _Out_ FLOAT* pRegularInterval,         // Interval of regular snap points
            _Out_ UINT32* pcIrregularSnapPoints,   // Number of irregular snap points
            _Outptr_result_buffer_(*pcIrregularSnapPoints) FLOAT** ppIrregularSnapPoints,   // Array of irregular snap points
            _Out_ DMSnapCoordinate* pSnapCoordinate) override; // coordinate system of the snap points

        // Called to notify of a characteristic change that may affect the
        // manipulability of inner elements.
        _Check_return_ HRESULT NotifyManipulatabilityAffectingPropertyChanged(
            _In_ BOOLEAN isInLiveTree) override;

        // Called to notify of a characteristic change that may affect the
        // alignment of the provided manipulated element.
        _Check_return_ HRESULT NotifyContentAlignmentAffectingPropertyChanged(
            _In_ UIElement* pManipulatedElement,
            _In_ BOOLEAN isForHorizontalAlignment,
            _In_ BOOLEAN isForStretchAlignment,
            _In_ BOOLEAN isStretchAlignmentTreatedAsNear) override;

        // Processes the DirectManipulation feedback for the provided
        // manipulated element
        _Check_return_ HRESULT NotifyManipulationProgress(
            _In_ UIElement* pManipulatedElement,
            _In_ DMManipulationState state,
            _In_ FLOAT xCumulativeTranslation,
            _In_ FLOAT yCumulativeTranslation,
            _In_ FLOAT zCumulativeFactor,
            _In_ FLOAT xInertiaEndTranslation,
            _In_ FLOAT yInertiaEndTranslation,
            _In_ FLOAT zInertiaEndFactor,
            _In_ FLOAT xCenter,
            _In_ FLOAT yCenter,
            _In_ BOOLEAN isInertiaEndTransformValid,
            _In_ BOOLEAN isInertial,
            _In_ BOOLEAN isTouchConfigurationActivated,
            _In_ BOOLEAN isBringIntoViewportConfigurationActivated) override;

        // Called to raise the DirectManipulationStarting/Started/Completed events
        _Check_return_ HRESULT NotifyManipulationStateChanged(
            _In_ DMManipulationState state) override;

        // Called to determine if our manipulation data is stale and we need
        // to bring into view.
        _Check_return_ HRESULT IsBringIntoViewportNeeded(_Out_ bool * bringIntoViewportNeeded);

        // Called to notify this manipulation handler that it needs to
        // call IDirectManipulationContainerHandler::BringIntoViewport
        // either to synchronize the DManip primary content transform with XAML when transformIsValid==False,
        // or to jump to the provided transform when transformIsValid==True.
        // When transformIsInertiaEnd==True, the call is made after cancelling inertia and the transform provided
        // was the expected end-of-inertia transform.
        _Check_return_ HRESULT NotifyBringIntoViewportNeeded(
            _In_ UIElement* pManipulatedElement,
            _In_ FLOAT translationX,
            _In_ FLOAT translationY,
            _In_ FLOAT zoomFactor,
            _In_ BOOLEAN transformIsValid,
            _In_ BOOLEAN transformIsInertiaEnd) override;

        // Retrieves the DirectManipulation element and property associated with this element
        // and the target property that we want to animate.
        _Check_return_ HRESULT GetDManipElementAndProperty(
            _In_ KnownPropertyIndex targetProperty,
            _Outptr_ CDependencyObject** ppDManipElement,
            _Out_ XUINT32 *pDManipProperty) override;

        // Retrieves the DirectManipulation element associated with this element.
        _Check_return_ HRESULT GetDManipElement(
            _Outptr_ CDependencyObject** ppDManipElement) override;

        // Returns the current view from the manipulation handler.
        _Check_return_ HRESULT GetDManipView(
            _Out_ DOUBLE* pHorizontalOffset,
            _Out_ DOUBLE* pVerticalOffset,
            _Out_ FLOAT* pZoomFactor);

        // Returns the target view for the current manipulation.
        _Check_return_ HRESULT GetTargetView(
            _Out_ DOUBLE* pTargetHorizontalOffset,
            _Out_ DOUBLE* pTargetVerticalOffset,
            _Out_ FLOAT* pTargetZoomFactor);

        // Stops the current manipulation if it's in inertia phase.
        _Check_return_ HRESULT StopInertialManipulation();

        // Called internally to scroll the content within the ScrollViewer to the specified horizontal offset position.
        _Check_return_ HRESULT ScrollToHorizontalOffsetInternal(
            _In_ DOUBLE offset);

        // Called internally to scroll the content within the ScrollViewer to the specified vertical offset position.
        _Check_return_ HRESULT ScrollToVerticalOffsetInternal(
            _In_ DOUBLE offset);

        // Combines the abilities of ScrollToHorizontalOffset, ScrollToVerticalOffset
        // and ZoomToFactor, with the option of animating to the target view and snapping
        // to mandatory snap points.
        _Check_return_ HRESULT ChangeViewInternal(
            _In_opt_ wf::IReference<DOUBLE>* pHorizontalOffset,
            _In_opt_ wf::IReference<DOUBLE>* pVerticalOffset,
            _In_opt_ wf::IReference<FLOAT>* pZoomFactor,
            _In_opt_ FLOAT* pOldZoomFactor,
            _In_ BOOLEAN forceChangeToCurrentView,
            _In_ BOOLEAN adjustWithMandatorySnapPoints,
            _In_ BOOLEAN skipDuringTouchContact,
            _In_ BOOLEAN skipAnimationWhileRunning,
            _In_ BOOLEAN disableAnimation,
            _In_ BOOLEAN applyAsManip,
            _In_ BOOLEAN transformIsInertiaEnd,
            _In_ BOOLEAN isForMakeVisible,
            _Out_ BOOLEAN* pReturnValue) noexcept;

        // Public version of BringIntoViewportInternal that first checks if the ScrollViewer can
        // manipulate its content with a bring-into-viewport call.
        // Brings the specified bounds of the content into the viewport using DirectManipulation.
        // Uses an animation when possible.
        _Check_return_ HRESULT BringIntoViewport(
            _In_ XRECTF& bounds,
            _In_ BOOLEAN skipDuringTouchContact,
            _In_ BOOLEAN skipAnimationWhileRunning,
            _In_ BOOLEAN animate,
            _Out_ BOOLEAN* pHandled);

        // Temporary workaround for DManip bug 799346
        _Check_return_ HRESULT SetDirectManipulationOverridingZoomBoundaries();
        _Check_return_ HRESULT ResetDirectManipulationOverridingZoomBoundaries();

        // Called by internal controls to apply a pseudo-LayoutTransform
        // to the ScrollViewer.Content element.
        _Check_return_ HRESULT SetLayoutSize(
            _In_ wf::Size layoutSize);

        // Called at the end of a DM manipulation when the first layout occurs
        // after receiving the DMManipulationCompleted notification.
        _Check_return_ HRESULT PostDirectManipulationLayoutRefreshed();

        // Register this instance as being under control of a semantic zoom.
        _Check_return_ HRESULT RegisterAsSemanticZoomHost();

        // Starts a constant-velocity pan on this ScrollViewer with the given X and Y
        // velocities, in pixels/second. If both parameters are 0, the pan is stopped.
        // NOTE: Currently, it is not supported to call this method with a pan already
        // in progress. This means modifying the velocities is not currently possible
        // without first completely stopping the pan (by calling this method with both
        // parameters = 0).
        _Check_return_ HRESULT SetConstantVelocities(_In_ XFLOAT dx, _In_ XFLOAT dy);

        // Called by HorizontalSnapPointsChangedHandler and
        // VerticalSnapPointsChangedHandler when snap points changed
        _Check_return_ HRESULT OnSnapPointsChanged(_In_ DMMotionTypes motionType);

        _Check_return_ HRESULT get_ElementHorizontalScrollBar(
                _Outptr_ xaml::IUIElement** ppElement);

        _Check_return_ HRESULT get_ElementVerticalScrollBar(
            _Outptr_ xaml::IUIElement** ppElement);

        BOOLEAN IsManipulationHandlerReady() const
        {
            return m_hManipulationHandler != NULL;
        }

        BOOLEAN IsInDirectManipulation() const
        {
            return m_isInDirectManipulation;
        }

        BOOLEAN IsInManipulation() const
        {
            return m_isInDirectManipulation || m_isInConstantVelocityPan;
        }

        // A direct manipulation in inertia phase can be interrupted via a call to StopInertialManipulation().
        // At that point the m_isDirectManipulationStopped flag is set to True and IsInUnstoppedManipulation()
        // starts to return False instead of True. Once the state change to DMManipulationCompleted is processed,
        // m_isInDirectManipulation is set to False.
        BOOLEAN IsInUnstoppedManipulation() const
        {
            return (m_isInDirectManipulation && !m_isDirectManipulationStopped) || m_isInConstantVelocityPan;
        }

        BOOLEAN IsInDirectManipulationCompletion() const
        {
            return m_isInDirectManipulationCompletion;
        }

        BOOLEAN IsInDirectManipulationZoom() const
        {
            return m_isInDirectManipulationZoom;
        }

        // Returns true if currently in DM zooming
        _Check_return_ HRESULT IsInDirectManipulationZoom(
            _Out_ BOOLEAN& bIsInDirectManipulationZoom) override
        {
            bIsInDirectManipulationZoom = IsInDirectManipulationZoom();
            RRETURN(S_OK);
        }

        _Check_return_ HRESULT IsInChildInvalidateMeasure(
            _Out_ BOOLEAN& bIsInChildInvalidateMeasure) override
        {
            bIsInChildInvalidateMeasure = m_inChildInvalidateMeasure;
            RRETURN(S_OK);
        }

        FLOAT GetPreDirectManipulationOffsetX()
        {
            return m_preDirectManipulationOffsetX;
        }

        FLOAT GetPreDirectManipulationOffsetY()
        {
            return m_preDirectManipulationOffsetY;
        }

        FLOAT GetPreDirectManipulationZoomFactor()
        {
            return m_preDirectManipulationZoomFactor;
        }

        DOUBLE GetPixelHorizontalOffset()
        {
            return m_xPixelOffset;
        }

        DOUBLE GetPixelVerticalOffset()
        {
            return m_yPixelOffset;
        }

        DOUBLE GetUnboundHorizontalOffset()
        {
            return m_unboundHorizontalOffset;
        }

        DOUBLE GetUnboundVerticalOffset()
        {
            return m_unboundVerticalOffset;
        }

        wf::Size GetLayoutSize()
        {
            return m_layoutSize;
        }

        // Used by the inner ScrollContentPresenter when its SizesContentToTemplatedParent property is set to True.
        const wf::Size& GetLatestAvailableSize() const
        {
            return m_latestAvailableSize;
        }

        // Called by a control interested in knowning DirectManipulation state changes.
        // Only one listener can declare itself at once.
        _Check_return_ HRESULT SetDirectManipulationStateChangeHandler(_In_opt_ DirectManipulationStateChangeHandler* pDMStateChangeHandler);

        // Called when this DM container wants the DM handler to process the current
        // pure inertia input message, by forwarding it to DirectManipulation.
        _Check_return_ HRESULT ProcessPureInertiaInputMessage(
            _In_ ZoomDirection zoomDirection) override;

        // Called when this DM container wants the DM handler to process the current
        // pure inertia input message, by forwarding it to DirectManipulation.
        // The handler must set the isHandled flag to True if the message was handled.
        // Unfortunately, callers to this method must determine whether or not DM will treat
        // the current input message as a pure inertia manipulation.
        // PLEASE NOTE: You won't find an input message as a parameter to this function.
        // The implementation just calls ProcessInputMessage on the manipulation handler
        // (in most cases).
        _Check_return_ HRESULT ProcessPureInertiaInputMessage(
            _In_ ZoomDirection zoomDirection,
            _Out_ BOOLEAN* pIsHandled);

        //Obtains the zoom action (if any) DM will attempt if given the provided key combination.
        static ZoomDirection GetKeyboardMessageZoomAction(_In_ wsy::VirtualKeyModifiers keyModifiers, _In_ wsy::VirtualKey key);

        // allows SeZo to remove the indicators from view temporarily during view change
        _Check_return_ HRESULT BlockIndicatorsFromShowing();
        _Check_return_ HRESULT ResetBlockIndicatorsFromShowing();

        _Check_return_ HRESULT IsThumbDragging(_Out_ BOOLEAN* pThumbDragging);

        // Called internally by the inner ScrollContentPresenter when it is about to become the parent of a header.
        _Check_return_ HRESULT NotifyHeaderParenting(
            _In_ IUIElement* pHeader,
            _In_ BOOLEAN isTopHeader,
            _In_ BOOLEAN isLeftHeader);

        // Called internally by the inner ScrollContentPresenter when it became the parent of a header.
        _Check_return_ HRESULT NotifyHeaderParented(
            _In_ IUIElement* pHeader,
            _In_ BOOLEAN isTopHeader,
            _In_ BOOLEAN isLeftHeader);

        // Called internally by the inner ScrollContentPresenter when a header is no longer parented.
        _Check_return_ HRESULT NotifyHeaderUnparented(
            _In_ IUIElement* pHeader,
            _In_ BOOLEAN isTopHeader,
            _In_ BOOLEAN isLeftHeader);

        // Returns the combined size of the headers:
        // - horizontal size is max(TopLeftHeader's width, LeftHeader's width)
        // - vertical size is max(TopLeftHeader's height, TopHeader's height)
        _Check_return_ HRESULT GetHeadersSize(
            _Out_ XSIZEF* pSize);

        // Returns the horizontal and vertical ratios between the ScrollViewer effective viewport
        // and its actual size. That viewport is potentially reduced by the presence of headers.
        // Ratios returned depend on the quadrant owning the provided child.
        _Check_return_ HRESULT GetViewportRatios(
            _In_opt_ DependencyObject* pChild,
            _Out_ XSIZEF* pRatios);

        // Determine if content can be scrolled.
        // It can, if for either dimension scrolling is enabled AND content size is greater than size of viewport.
        _Check_return_ HRESULT IsContentScrollable(
            _In_ bool ignoreScrollMode,
            _In_ bool ignoreScrollBarVisibility,
            _Out_ bool* isContentHorizontallyScrollable,
            _Out_ bool* isContentVerticallyScrollable);

        // Sticky Headers
        // Functions needed for Sticky Headers, are called under apiset from ModernCollectionBasePanel
        _Check_return_ HRESULT get_HeaderHeight(_Out_ DOUBLE *pHeaderHeight);

        // Workaround for Windows Phone Blue bug 273985.
        // TODO: Threshold task 946804 - Re-evaluate the need for workaround in Pivot ctrl when DManip-on-DComp is on
        _Check_return_ HRESULT SetIsNearVerticalAlignmentForcedImpl(_In_ BOOLEAN enabled)
        {
            m_isNearVerticalAlignmentForced = !!enabled;
            return S_OK;
        }

        // Set explicit DMOverpanMode flags to configure overpan modes for horizontal and vertical directions.
        _Check_return_ HRESULT SetOverpanModes(_In_ DMOverpanMode horizontalOverpanMode, _In_ DMOverpanMode verticalOverpanMode);

        // Prevents overpan effect so that panning will hard-stop at the boundaries of the scrollable region.
        _Check_return_ HRESULT DisableOverpanImpl();

        // Reenables overpan.
        _Check_return_ HRESULT EnableOverpanImpl();

#ifdef DM_DEBUG
        BOOLEAN IsRootScrollViewerDbg() { return IsRootScrollViewer(); }
#endif // DM_DEBUG

        // Enters the mode where the child's actual size is used for
        // the extent exposed through IScrollInfo.
        void StartUseOfActualSizeAsExtent(_In_ bool isHorizontal);

        // Leaves the mode where the child's actual size is used for
        // the extent exposed through IScrollInfo.
        void StopUseOfActualSizeAsExtent(_In_ bool isHorizontal);

        // Retrieves the UIElement for the ScrollViewer content if any
        _Check_return_ HRESULT GetContentUIElement(_Outptr_result_maybenull_ xaml::IUIElement** ppContentUIElement);

        _Check_return_ HRESULT ProcessGamepadNavigation(
            _In_ wsy::VirtualKey key,
            _In_ wsy::VirtualKey originalKey,
            _Out_ BOOLEAN& isHandled);

        _Check_return_ HRESULT GetGamepadNavigationCandidate(
            _In_ xaml_input::FocusNavigationDirection direction,
            _In_ bool isPageNavigation,
            _In_ int numPagesLookAhead,
            _In_ float verticalViewportPadding,
            _Outptr_ IInspectable** ppCandidate);

        _Check_return_ HRESULT ScrollForFocusNavigation(
            _In_ wsy::VirtualKey key,
            _In_ xaml_input::FocusNavigationDirection direction,
            _In_ double viewportScrollPercent,
            _Out_ BOOLEAN* isHandled);

        // IScrollAnchorProvider
        _Check_return_ HRESULT get_CurrentAnchorImpl(_Outptr_result_maybenull_ xaml::IUIElement** value);

        _Check_return_ HRESULT RegisterAnchorCandidateImpl(_In_ xaml::IUIElement* element);
        _Check_return_ HRESULT UnregisterAnchorCandidateImpl(_In_ xaml::IUIElement* element);

        // Callback to know when the ScrollContentPresenter is measured. If the content size changes we need to run
        // arrange again so that we do the anchoring work. Since the ScrollContentPresenter's desired size itself might
        // not be changing, ScrollViewer will not get invalidated. We need to do that ourselves if we are anchoring.
        _Check_return_ HRESULT OnScrollContentPresenterMeasured();

    protected:

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        // Handles the vertical ScrollBar.Scroll event and updates the UI.
        _Check_return_ HRESULT HandleVerticalScroll(
            _In_ xaml_primitives::ScrollEventType scrollEventType,
            _In_ DOUBLE offset = 0.0);

        // Handles the horizontal ScrollBar.Scroll event and updates the UI.
        _Check_return_ HRESULT HandleHorizontalScroll(
            _In_ xaml_primitives::ScrollEventType scrollEventType,
            _In_ DOUBLE offset = 0.0);

    private:

        _Check_return_ HRESULT GetScrollViewerGlobalBoundsWithoutHeaders(
            _Out_ XRECTF_RB* scrollViewerGlobalBoundsWithoutHeaders);

        // Gets the viewport after removing top/left headers and accounting
        // for tv-safe area
        _Check_return_ HRESULT GetUnobstructedScrollViewerGlobalBounds(
            _Out_ wf::Rect* unobstructedViewportGlobalBounds);

        // Removes the potential duplicate DMConfigurationPanInertia flag. The duplication occurs when both horizontal and vertical
        // panning are allowed, and pan inertia is turned on.
        static DMConfigurations RemoveDuplicatePanInertia(
            _In_ BOOLEAN isScrollInertiaEnabled,
            _In_ DMConfigurations panXConfiguration,
            _In_ DMConfigurations panYConfiguration)
        {
            if (isScrollInertiaEnabled && panXConfiguration != DMConfigurationNone && panYConfiguration != DMConfigurationNone)
            {
                return (DMConfigurations) (panXConfiguration + panYConfiguration - DMConfigurationPanInertia);
            }
            return (DMConfigurations) (panXConfiguration + panYConfiguration);
        }

        static bool IsConscious() noexcept
        {
            return DXamlCore::ShouldUseDynamicScrollbars();
        }

        // Helper method to get the effective DMOverpanMode for the given orientation.
        // Returns the stored DMOverpanMode if the extent is smaller than the viewport;
        // returns the default DMOverpanMode otherwise (which is necessary to work around
        // a DM limitation we have when the extent is smaller than the viewport).
        _Check_return_ HRESULT GetEffectiveDMOverpanMode(_In_ BOOLEAN isForHorizontal, _Out_ DMOverpanMode* pDMOverpanMode);

        // Gets a value indicating whether the current ScrollContentPresenter is the IScrollInfo implementer.
        // Uses the ScrollContentPresenter::IsScrollClient(...) method.
        _Check_return_ HRESULT IsScrollContentPresenterScrollClient(
            _Out_ BOOLEAN& isScrollContentPresenterScrollClient);

        // Helper that updates the values of properties during InvalidateScrollInfo.
        _Check_return_ HRESULT InvalidateScrollInfo_TryUpdateValues(
            _In_ IScrollInfo* pScrollInfo,
            _Out_ BOOLEAN& changed,
            _Out_ BOOLEAN& updateOffsetX,
            _Out_ BOOLEAN& updateOffsetY,
            _Out_ BOOLEAN& updatePixelViewportX,
            _Out_ BOOLEAN& updatePixelViewportY,
            _Out_ BOOLEAN& updatePixelExtentX,
            _Out_ BOOLEAN& updatePixelExtentY) noexcept;

        // Brings the specified bounds of the content into the viewport using DirectManipulation.
        // If animate is True, an DM animation is used.
        _Check_return_ HRESULT BringIntoViewportInternal(
            _In_ XRECTF& bounds,
            _In_ FLOAT translateX,
            _In_ FLOAT translateY,
            _In_ FLOAT zoomFactor,
            _In_ BOOLEAN transformIsValid,
            _In_ BOOLEAN skipDuringTouchContact,
            _In_ BOOLEAN skipAnimationWhileRunning,
            _In_ BOOLEAN animate,
            _In_ BOOLEAN applyAsManip,
            _In_ BOOLEAN isForMakeVisible,
            _Out_ BOOLEAN* pHandled);

        // Bring a child element into view.
        _Check_return_ HRESULT MakeVisible(
            // Child element to bring into view
            _In_ UIElement* element,
            // Target rectangle dimensions. If empty, bring the child element's
            // RenderSize dimensions into view.
            wf::Rect targetRect,
            // Pass on forceIntoView from sender to ancestor ScrollViewer
            BOOLEAN forceIntoView,
            // When set to True, the DManip ZoomToRect method is invoked.
            BOOLEAN useAnimation,
            // Forwarded to the BringIntoView method to indicate whether its own
            // MakeVisible calls should be skipped during an ongoing manipulation or not.
            BOOLEAN skipDuringManipulation,
            DOUBLE horizontalAlignmentRatio = DoubleUtil::NaN,
            DOUBLE verticalAlignmentRatio = DoubleUtil::NaN,
            DOUBLE offsetX = 0.0,
            DOUBLE offsetY = 0.0);

        // OnBringIntoViewRequested is called from the event handler ScrollViewer
        // registers for the event.  The default implementation checks to make
        // sure the visual is a child of the scroll viewer, and then delegates
        // to a method there
        _Check_return_ HRESULT OnBringIntoViewRequested(
            _In_ IUIElement* pSender,
            _In_ xaml::IBringIntoViewRequestedEventArgs* pArgs);

        // Updates the provided IScrollInfo's CanHorizontallyScroll & CanVerticallyScroll characteristics based
        // on the scrollbars visibility, and resets the offset(s) when scrolling in not enabled.
        _Check_return_ HRESULT UpdateCanScroll(
            _In_ IScrollInfo* pScrollInfo);

        // Called when the HorizontalScrollBarVisibility or
        // VerticalScrollBarVisibility property has changed.
        _Check_return_ HRESULT OnScrollBarVisibilityChanged();

        // Handle the horizontal ScrollBar's Scroll event.
        _Check_return_ HRESULT OnHorizontalScrollBarScroll(
            _In_ IInspectable* pSender,
            _In_ xaml_primitives::IScrollEventArgs* pArgs);

        // Handle the vertical ScrollBar's Scroll event.
        _Check_return_ HRESULT OnVerticalScrollBarScroll(
            _In_ IInspectable* pSender,
            _In_ xaml_primitives::IScrollEventArgs* pArgs);

        // Handle DragStarted on the ScrollBar's Thumb.
        _Check_return_ HRESULT OnScrollBarThumbDragStarted(
            _In_ IInspectable* pSender,
            _In_ xaml_primitives::IDragStartedEventArgs* pArgs);

        // Handle DragCompleted on the ScrollBar's Thumb.
        _Check_return_ HRESULT OnScrollBarThumbDragCompleted(
            _In_ IInspectable* pSender,
            _In_ xaml_primitives::IDragCompletedEventArgs* pArgs);

        // Handle PointerEntered on the vertical scrollbar
        _Check_return_ HRESULT OnVerticalScrollbarPointerEntered(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Handle PointerExited on the vertical scrollbar
        _Check_return_ HRESULT OnVerticalScrollbarPointerExited(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Handle PointerEntered on the horizontal scrollbar
        _Check_return_ HRESULT OnHorizontalScrollbarPointerEntered(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Handle PointerExited on the horizontal scrollbar
        _Check_return_ HRESULT OnHorizontalScrollbarPointerExited(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Get the visual child of the ScrollViewer.
        _Check_return_ HRESULT GetVisualChild(
            _Outptr_ xaml::IUIElement** ppChild);

        // DirectManipulation-related private methods

        // Event handler called when TopLeftHeader, LeftHeader or TopHeader changed
        _Check_return_ HRESULT OnHeaderPropertyChanged(
            _In_ BOOLEAN isTopHeader, _In_ BOOLEAN isLeftHeader,
            _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue);

        // Event handler called when ZoomSnapPoints changed
        _Check_return_ HRESULT OnZoomSnapPointsCollectionChanged(
            _In_ wfc::IObservableVector<FLOAT>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e);

        // Validates the MinZoomFactor property when its value is changed.
        _Check_return_ HRESULT OnMinZoomFactorPropertyChanged(
            _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue);

        // Validates the MaxZoomFactor property when its value is changed.
        _Check_return_ HRESULT OnMaxZoomFactorPropertyChanged(
            _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue);

        // Called when the HorizontalOffset dependency property changed.
        _Check_return_ HRESULT OnHorizontalOffsetPropertyChanged(
            _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue);

        // Called when the VerticalOffset dependency property changed.
        _Check_return_ HRESULT OnVerticalOffsetPropertyChanged(
            _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue);

        // Validates the ZoomFactor property when its value is changed.
        _Check_return_ HRESULT OnZoomFactorPropertyChanged(
            _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue);

        // Called when the MinZoomFactor or MaxZoomFactor value changed.
        _Check_return_ HRESULT OnZoomFactorBoundaryChanged(
            _In_ BOOLEAN isForLowerBound, _In_ FLOAT oldZoomFactorBoundary, _In_ FLOAT newZoomFactorBoundary);

        // Called when the ZoomFactor value changed.
        _Check_return_ HRESULT OnZoomFactorChanged(_In_ FLOAT oldZoomFactor, _In_ FLOAT newZoomFactor);

        // Ensures the MaxZoomFactor is greater than or equal to the MinZoomFactor.
        _Check_return_ HRESULT CoerceMaxZoomFactor();

        // Ensures the ZoomFactor falls between the MinZoomFactor and MaxZoomFactor values.
        _Check_return_ HRESULT CoerceZoomFactor();

        // Common method called by NotifyHorizontalOffsetChanging and NotifyVerticalOffsetChanging,
        // in order to invoke RaiseViewChanging with the correct zoom factor.
        _Check_return_ HRESULT NotifyOffsetChanging(
            _In_ DOUBLE targetHorizontalOffset,
            _In_ DOUBLE targetVerticalOffset);

        // Called internally when a zoom factor change is processed in order to invoke RaiseViewChanging
        // with the correct offsets.
        _Check_return_ HRESULT NotifyZoomFactorChanging(
            _In_ FLOAT targetZoomFactor);

        // Returns a float if the IPropertyValue contains a FLOAT or DOUBLE.
        _Check_return_ HRESULT GetFloatValue(
            _In_ IInspectable *pValue, _Out_ FLOAT& floatValue);

        // Extracts a float from a IPropertyValue and checks if it's valid.
        _Check_return_ HRESULT IsValidFloatValue(
            _In_ IInspectable *pValue, _Out_ FLOAT& floatValue, _Out_ BOOLEAN* pbIsValidFloatValue);

        // Gets the horizontal extent in pixels even for logical scrolling scenarios.
        _Check_return_ HRESULT ComputePixelExtentWidth(
            _In_ bool ignoreZoomFactor,
            _In_opt_ IManipulationDataProvider* pProvider,
            _Out_ DOUBLE* pValue);

        // Gets the horizontal extent in pixels even for logical scrolling scenarios.
        // Overload that attempts to determine the potential inner horizontal IManipulationDataProvider.
        _Check_return_ HRESULT ComputePixelExtentWidth(
            _Out_ DOUBLE* pValue);

        // Gets the vertical extent in pixels even for logical scrolling scenarios.
        _Check_return_ HRESULT ComputePixelExtentHeight(
            _In_ bool ignoreZoomFactor,
            _In_opt_ IManipulationDataProvider* pProvider,
            _Out_ DOUBLE* pValue);

        // Gets the vertical extent in pixels even for logical scrolling scenarios.
        // Overload that attempts to determine the potential inner vertical IManipulationDataProvider.
        _Check_return_ HRESULT ComputePixelExtentHeight(
            _Out_ DOUBLE* pValue);

        // Gets the viewport width in pixels even for logical scrolling scenarios.
        _Check_return_ HRESULT ComputePixelViewportWidth(
            _In_opt_ IManipulationDataProvider* pProvider,
            _In_ BOOLEAN isProviderSet,
            _Out_ DOUBLE* pValue);

        // Gets the viewport height in pixels even for logical scrolling scenarios.
        _Check_return_ HRESULT ComputePixelViewportHeight(
            _In_opt_ IManipulationDataProvider* pProvider,
            _In_ BOOLEAN isProviderSet,
            _Out_ DOUBLE* pValue);

        // Gets the value of the scrollable width of the content in pixels even for logical scrolling scenarios.
        _Check_return_ HRESULT ComputePixelScrollableWidth(
            _In_ IManipulationDataProvider* pProvider,
            _Out_ DOUBLE& pixelScrollableWidth);

        // Gets the value of the scrollable height of the content in pixels even for logical scrolling scenarios.
        _Check_return_ HRESULT ComputePixelScrollableHeight(
            _In_ IManipulationDataProvider* pProvider,
            _Out_ DOUBLE& pixelScrollableHeight);

        // Computes the required horizontal alignment to provide to DirectManipulation
        _Check_return_ HRESULT ComputeHorizontalAlignment(
            _In_ BOOLEAN canUseCachedProperties,
            _Out_ DMAlignment& horizontalAlignment);

        // Computes the required vertical alignment to provide to DirectManipulation
        _Check_return_ HRESULT ComputeVerticalAlignment(
            _In_ BOOLEAN canUseCachedProperties,
            _Out_ DMAlignment& verticalAlignment);

        // Scrolls by the number of provided pixels, even for logical scrolling scenarios.
        _Check_return_ HRESULT ScrollByPixelDelta(
            _In_ BOOLEAN isForHorizontalOrientation,
            _In_ DOUBLE newPixelOffset,
            _In_ DOUBLE pixelDelta,
            _In_ BOOLEAN isDManipInput);

        // Returns the IScrollInfo implementation as a UIElement
        _Check_return_ HRESULT GetScrollInfoAsElement(
            _Outptr_ xaml::IUIElement** ppElement);

        // Returns the potential inner IManipulationDataProvider regardless of orientation
        _Check_return_ HRESULT GetInnerManipulationDataProvider(
            _Outptr_result_maybenull_ IManipulationDataProvider** ppProvider);

        // Returns the potential inner IManipulationDataProvider if it's oriented according
        // to the provided orientation.
        _Check_return_ HRESULT GetInnerManipulationDataProvider(
            _In_ BOOLEAN isForHorizontalOrientation,
            _Outptr_result_maybenull_ IManipulationDataProvider** ppProvider);

        // Returns True when the inner panel is a CarouselPanel
        _Check_return_ HRESULT IsPanelACarouselPanel(
            _In_ BOOLEAN isForHorizontalOrientation,
            _Out_ BOOLEAN& isCarouselPanel);

        // Computes the effect of the left margin and horizontal alignment when
        // the content is smaller than the viewport.
        _Check_return_ HRESULT ComputeTranslationXCorrection(
            _In_ BOOLEAN inManipulation,
            _In_ BOOLEAN forInitialTransformationAdjustment,
            _In_ BOOLEAN adjustDimensions,
            _In_opt_ IManipulationDataProvider* pProvider,
            _In_ DOUBLE leftMargin,
            _In_ DOUBLE extent,
            _In_ FLOAT zoomFactor,
            _Out_ FLOAT* pTranslationX);

        // Computes the effect of the top margin and vertical alignment when
        // the content is smaller than the viewport.
        _Check_return_ HRESULT ComputeTranslationYCorrection(
            _In_ BOOLEAN inManipulation,
            _In_ BOOLEAN forInitialTransformationAdjustment,
            _In_ BOOLEAN adjustDimensions,
            _In_opt_ IManipulationDataProvider* pProvider,
            _In_ DOUBLE topMargin,
            _In_ DOUBLE extent,
            _In_ FLOAT zoomFactor,
            _Out_ FLOAT* pTranslationY);

        // Retrieves the effective IsHorizontalRailEnabled value: m_currentIsHorizontalRailEnabled or
        // get_IsHorizontalRailEnabled depending on whether there is an active manip or not.
        _Check_return_ HRESULT GetEffectiveIsHorizontalRailEnabled(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ BOOLEAN& isHorizontalRailEnabled);

        // Retrieves the effective IsVerticalRailEnabled value: m_currentIsVerticalRailEnabled or
        // get_IsVerticalRailEnabled depending on whether there is an active manip or not.
        _Check_return_ HRESULT GetEffectiveIsVerticalRailEnabled(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ BOOLEAN& isVerticalRailEnabled);

        // Retrieves the effective IsScrollInertiaEnabled value: m_currentIsScrollInertiaEnabled or
        // get_IsScrollInertiaEnabled depending on whether there is an active manip or not.
        _Check_return_ HRESULT GetEffectiveIsScrollInertiaEnabled(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ BOOLEAN& isScrollInertiaEnabled);

        // Retrieves the effective IsZoomInertiaEnabled value: m_currentIsZoomInertiaEnabled or
        // get_IsZoomInertiaEnabled depending on whether there is an active manip or not.
        _Check_return_ HRESULT GetEffectiveIsZoomInertiaEnabled(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ BOOLEAN& isZoomInertiaEnabled);

        // Retrieves the effective horizontal scrollbar visibility: m_currentHorizontalScrollBarVisibility or
        // get_HorizontalScrollBarVisibility depending on whether there is an active manip or not.
        _Check_return_ HRESULT GetEffectiveHorizontalScrollBarVisibility(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ xaml_controls::ScrollBarVisibility& hsbv);

        // Retrieves the effective vertical scrollbar visibility: m_currentVerticalScrollBarVisibility or
        // get_VerticalScrollBarVisibility depending on whether there is an active manip or not.
        _Check_return_ HRESULT GetEffectiveVerticalScrollBarVisibility(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ xaml_controls::ScrollBarVisibility& vsbv);

        // Retrieves the horizontal alignment used at the beginning of the ongoing manipulation if any
        // when canUseCachedProperty is True, or the current alignment otherwise.
        _Check_return_ HRESULT GetEffectiveHorizontalAlignment(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ DMAlignment& horizontalAlignment);

        // Retrieves the vertical alignment used at the beginning of the ongoing manipulation if any
        // when canUseCachedProperty is True, or the current alignment otherwise.
        _Check_return_ HRESULT GetEffectiveVerticalAlignment(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ DMAlignment& verticalAlignment);

        // Retrieves the effective horizontal scroll mode: m_currentHorizontalScrollMode or get_HorizontalScrollMode depending
        // on whether there is an active manip or not.
        _Check_return_ HRESULT GetEffectiveHorizontalScrollMode(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ xaml_controls::ScrollMode& horizontalScrollMode);

        // Retrieves the effective vertical scroll mode: m_currentVerticalScrollMode or get_VerticalScrollMode depending
        // on whether there is an active manip or not.
        _Check_return_ HRESULT GetEffectiveVerticalScrollMode(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ xaml_controls::ScrollMode& verticalScrollMode);

        // Retrieves the effective zoom mode: m_currentZoomMode or get_ZoomMode depending on whether there is an active manip or not.
        _Check_return_ HRESULT GetEffectiveZoomMode(
            _In_ BOOLEAN canUseCachedProperty,
            _Out_ xaml_controls::ZoomMode& zoomMode);

        // Retrieves the left and top margins of the provided element.
        _Check_return_ HRESULT GetTopLeftMargins(_In_ UIElement* pElement, _Out_ DOUBLE& topMargin, _Out_ DOUBLE& leftMargin);

        // Determines if a change in the content's extent is expected based on
        // DirectManipulation feedback.
        _Check_return_ HRESULT AreExtentChangesExpected(_Out_ BOOLEAN& areExtentChangesExpected);
        _Check_return_ HRESULT IsExtentXChangeExpected(_Out_ BOOLEAN& isExtentXChangeExpected);
        _Check_return_ HRESULT IsExtentYChangeExpected(_Out_ BOOLEAN& isExtentYChangeExpected);

        // Called when the input manager may need to be told that a manipulatable element has changed.
        _Check_return_ HRESULT NotifyManipulatableElementChanged();

        // Returns the DMConfigurations values to provide to DM based on the DM-related properties and physical characteristics of the ScrollViewer
        _Check_return_ HRESULT GetManipulationConfigurations(
            _In_opt_ BOOLEAN* pIsInLiveTree,
            _In_ BOOLEAN canUseCachedProperties,
            _Out_opt_ BOOLEAN* pCanManipulateElementsByTouch,
            _Out_opt_ BOOLEAN* pCanManipulateElementsWithAsyncBringIntoViewport,
            _Out_opt_ DMConfigurations* pTouchConfiguration,
            _Out_opt_ DMConfigurations* pNonTouchConfiguration,
            _Out_opt_ DMConfigurations* pBringIntoViewportConfiguration,
            _Out_opt_ UINT8* pcConfigurations,
            _Outptr_result_buffer_maybenull_(*pcConfigurations) DMConfigurations** ppConfigurations);

        // Returns the DMConfigurations value to provide to DM for keyboard/mouse wheel operations.
        // Configuration is based on the DM-related and scrollbar-related properties of the ScrollViewer.
        _Check_return_ HRESULT GetNonTouchManipulationConfiguration(
            _In_ BOOLEAN canUseCachedProperties,
            _Out_ DMConfigurations* pNonTouchConfiguration);

        // Returns the DMConfigurations value to use during a BringIntoViewport manipulation.
        _Check_return_ HRESULT GetBringIntoViewportConfiguration(
            _In_ BOOLEAN canUseCachedProperties,
            _Out_ DMConfigurations* pBringIntoViewportConfiguration);

        // Removes all the configurations in the array equal to DMConfigurationNone, potentially resulting in a compacted array.
        _Check_return_ HRESULT CompactManipulationConfigurations(
            _In_reads_(cConfigurations) DMConfigurations* pConfigurations,
            _Inout_ UINT8& cConfigurations);

        // Returns the scroll snap points and their characteristics for the provided orientation.
        _Check_return_ HRESULT GetScrollSnapPoints(
            _In_ BOOLEAN isForHorizontalSnapPoints,// True to retrieve the horizontal scroll snap points
            _In_ FLOAT zoomFactor,
            _In_ FLOAT staticZoomFactor,
            _In_ DOUBLE extentDim,                 // Set to -1 when the extent was not pre-computed
            _In_ BOOLEAN updateSnapPointsChangeSubscription,
            _Out_ BOOLEAN* pAreSnapPointsOptional, // Set to True when returned snap points are optional
            _Out_ BOOLEAN* pAreSnapPointsSingle,   // Set to True when returned snap points are single (i.e. breaking inertia)
            _Out_ BOOLEAN* pAreSnapPointsRegular,  // Set to True when returned snap points are equidistant
            _Out_ FLOAT* pRegularOffset,           // Offset of regular snap points
            _Out_ FLOAT* pRegularInterval,         // Interval of regular snap points
            _Out_ UINT32* pcIrregularSnapPoints,   // Number of irregular snap points
            _Outptr_result_buffer_(*pcIrregularSnapPoints) FLOAT** ppIrregularSnapPoints, // Array of irregular snap points
            _Out_ DMSnapCoordinate* pSnapCoordinate); // coordinate system of the snap points

        // Converts the provided vector of floats into an array of floats to be
        // handed off to the core by GetManipulationMotionSnapPoints.
        _Check_return_ HRESULT CopyMotionSnapPointsFromVector(
            _In_ BOOLEAN isForZoomSnapPoints,
            _In_opt_ wfc::IVectorView<FLOAT>* pSnapPointsVector,
            _In_ xaml_primitives::SnapPointsAlignment snapPointsAlignment, // Near/Center/Far alignment of the snap points
            _In_ DOUBLE viewportDim,                                 // Dimension of the viewport
            _In_ DOUBLE extentDim,                                   // Dimension of the extent
            _In_ FLOAT zoomFactor,                                   // Zoom factor to apply
            _In_ FLOAT staticZoomFactor,                             // Static zoom factor to apply
            _Out_ UINT32* pcSnapPoints,                              // Number of snap points
            _Outptr_result_buffer_(*pcSnapPoints) FLOAT** ppSnapPoints); // Array of snap points

        // Ensures the target horizontal offset for a ChangeView request does
        // not exceed the maximum offset given the target zoom factor, viewport
        // width, extent width and optionally the mandatory scroll snap points.
        _Check_return_ HRESULT AdjustTargetHorizontalOffset(
            _In_ BOOLEAN disableAnimation,
            _In_ BOOLEAN adjustWithMandatorySnapPoints,
            _In_ FLOAT targetZoomFactor,
            _In_ DOUBLE minHorizontalOffset,
            _In_ DOUBLE currentHorizontalOffset,
            _In_ DOUBLE viewportPixelWidth,
            _Inout_ DOUBLE* pTargetHorizontalOffset,
            _Out_ DOUBLE* pCurrentUnzoomedPixelExtentWidth,
            _Out_ DOUBLE* pMaxHorizontalOffset,
            _Out_ DOUBLE* pTargetExtentWidth);

        // Ensures the target vertical offset for a ChangeView request does
        // not exceed the maximum offset given the target zoom factor, viewport
        // height, extent height and optionally the mandatory scroll snap points.
        _Check_return_ HRESULT AdjustTargetVerticalOffset(
            _In_ BOOLEAN disableAnimation,
            _In_ BOOLEAN adjustWithMandatorySnapPoints,
            _In_ FLOAT targetZoomFactor,
            _In_ DOUBLE minVerticalOffset,
            _In_ DOUBLE currentVerticalOffset,
            _In_ DOUBLE viewportPixelHeight,
            _Inout_ DOUBLE* pTargetVerticalOffset,
            _Out_ DOUBLE* pCurrentUnzoomedPixelExtentHeight,
            _Out_ DOUBLE* pMaxVerticalOffset,
            _Out_ DOUBLE* pTargetExtentHeight);

        // Adjusts the provided logical targetOffset based on potential mandatory near-aligned scroll snap points.
        _Check_return_ HRESULT AdjustLogicalOffsetWithMandatorySnapPoints(
            _In_ BOOLEAN isForHorizontalOffset,
            _Inout_ DOUBLE* pTargetOffset);

        // Adjusts the provided targetOffset based on potential mandatory scroll snap points.
        // The provided minOffset and maxOffset guarantee that the adjustment remains within
        // the allowed boundaries.
        _Check_return_ HRESULT AdjustOffsetWithMandatorySnapPoints(
            _In_ BOOLEAN isForHorizontalOffset,
            _In_ DOUBLE minOffset,
            _In_ DOUBLE maxOffset,
            _In_ DOUBLE currentOffset,
            _In_ DOUBLE targetExtentDim,
            _In_ DOUBLE viewportDim,
            _In_ FLOAT targetZoomFactor,
            _Inout_ DOUBLE* pTargetOffset);

        // Adjusts the provided targetZoomFactor zoom factor based on potential mandatory
        // zoom snap points. The provided minZoomFactor and maxZoomFactor values guarantee
        // that the adjustment remains within the allowed boundaries.
        _Check_return_ HRESULT AdjustZoomFactorWithMandatorySnapPoints(
            _In_ FLOAT minZoomFactor,
            _In_ FLOAT maxZoomFactor,
            _Inout_ FLOAT* pTargetZoomFactor);

        // Adjusts the provided target HorizontalOffset, target VerticalOffset and target
        // ZoomFactor based on potential mandatory scroll and zoom snap points.
        // The provided min & max offsets and factor guarantee that the adjustment remains
        // within the allowed boundaries.
        _Check_return_ HRESULT AdjustViewWithMandatorySnapPoints(
            _In_ DOUBLE minHorizontalOffset,
            _In_ DOUBLE maxHorizontalOffset,
            _In_ DOUBLE currentHorizontalOffset,
            _In_ DOUBLE minVerticalOffset,
            _In_ DOUBLE maxVerticalOffset,
            _In_ DOUBLE currentVerticalOffset,
            _In_ FLOAT minZoomFactor,
            _In_ FLOAT maxZoomFactor,
            _In_ DOUBLE viewportPixelWidth,
            _In_ DOUBLE viewportPixelHeight,
            _Inout_ DOUBLE* pCurrentUnzoomedPixelExtentWidth,
            _Inout_ DOUBLE* pCurrentUnzoomedPixelExtentHeight,
            _Inout_ DOUBLE* pTargetHorizontalOffset,
            _Inout_ DOUBLE* pTargetVerticalOffset,
            _Inout_ FLOAT* pTargetZoomFactor);

        // Updates the ScrollBar's IsIgnoringUserInput flag based on the scroll mode setting.
        // Delays the update when there is an ongoing manipulation.
        // The horizontal or vertical ScrollBar is affected depending on the param.
        _Check_return_ HRESULT RefreshScrollBarIsIgnoringUserInput(
            _In_ BOOLEAN isForHorizontalOrientation);

        // Checks if the ScrollContentPresenter's content implements IScrollSnapPointsInfo.
        _Check_return_ HRESULT RefreshScrollSnapPointsInfo();

        // Hooks up the snap points change event
        _Check_return_ HRESULT HookScrollSnapPointsInfoEvents(_In_ BOOLEAN isForHorizontalSnapPoints);

        // Unhooks the snap points change event
        _Check_return_ HRESULT UnhookScrollSnapPointsInfoEvents(_In_ BOOLEAN isForHorizontalSnapPoints);

        // Called when a property that affects the snap points changed.
        _Check_return_ HRESULT OnSnapPointsAffectingPropertyChanged(
            _In_ DMMotionTypes motionType,
            _In_ BOOLEAN updateSnapPointsChangeSubscription);

        // Called when a characteristic changes that might affect the result of get_CanManipulateElements.
        // isAffectingTouchConfiguration is True when the change might affect the active DirectManipulation configuration.
        _Check_return_ HRESULT OnManipulatabilityAffectingPropertyChanged(
            _In_opt_ BOOLEAN* pIsInLiveTree,
            _In_ BOOLEAN isCachedPropertyChanged,
            _In_ BOOLEAN isContentChanged,
            _In_ BOOLEAN isAffectingConfigurations,
            _In_ BOOLEAN isAffectingTouchConfiguration);

        // Called when a property that might change the required horizontal or vertical
        // DirectManipulation alignment has changed. For instance when a pixel-based
        // viewport or extent dimension has changed.
        _Check_return_ HRESULT OnContentAlignmentAffectingPropertyChanged(
            _In_ BOOLEAN isForHorizontalAlignment,
            _In_ BOOLEAN isForVerticalAlignment);

        // Called when a characteristic changes that affects the DM viewport configurations.
        _Check_return_ HRESULT OnViewportConfigurationsAffectingPropertyChanged();

        // Called when a property that changes responsiveness to occlusions changes.
        _Check_return_ HRESULT OnReduceViewportForCoreInputViewOcclusionsChanged();

        _Check_return_ HRESULT ReflowAroundCoreInputViewOcclusions();

        _Check_return_ HRESULT OnCanContentRenderOutsideBoundsChanged(
            _In_ IInspectable* newValue);

        // Called when the Content property changed.
        _Check_return_ HRESULT OnContentPropertyChanged();

        // Called when a viewport-affecting property changed.
        _Check_return_ HRESULT OnViewportAffectingPropertyChanged(
            _In_ BOOLEAN boundsChanged,
            _In_ BOOLEAN touchConfigurationChanged,
            _In_ BOOLEAN nonTouchConfigurationChanged,
            _In_ BOOLEAN configurationsChanged,
            _In_ BOOLEAN chainedMotionTypesChanged,
            _In_ BOOLEAN horizontalOverpanModeChanged,
            _In_ BOOLEAN verticalOverpanModeChanged,
            _Out_opt_ BOOLEAN* pAreConfigurationsUpdated);

        // Called when a primary content-affecting property changed.
        _Check_return_ HRESULT OnPrimaryContentAffectingPropertyChanged(
            _In_ BOOLEAN boundsChanged,
            _In_ BOOLEAN horizontalAlignmentChanged,
            _In_ BOOLEAN verticalAlignmentChanged,
            _In_ BOOLEAN zoomFactorBoundaryChanged);

        // Called when a primary content characteristic has changed.
        _Check_return_ HRESULT OnPrimaryContentChanged(
            _In_ BOOLEAN layoutRefreshed,
            _In_ BOOLEAN boundsChanged,
            _In_ BOOLEAN horizontalAlignmentChanged,
            _In_ BOOLEAN verticalAlignmentChanged,
            _In_ BOOLEAN zoomFactorBoundaryChanged);

        // Called when the input manager needs to push a new primary content
        // transform to DirectManipulation.
        _Check_return_ HRESULT OnPrimaryContentTransformChanged(
            _In_ BOOLEAN translationXChanged,
            _In_ BOOLEAN translationYChanged,
            _In_ BOOLEAN zoomFactorChanged);

        // Called by the InputManager when a manipulation may start.
        _Check_return_ HRESULT HandleManipulationStarting(
            _In_ UIElement* pManipulatedElement,
            _In_ BOOLEAN wasInDirectManipulation);

        // Called by the InputManager when a manipulation progresses.
        _Check_return_ HRESULT HandleManipulationDelta(
            _In_ UIElement* pManipulatedElement,
            _In_ FLOAT xInertiaEndTranslation,
            _In_ FLOAT yInertiaEndTranslation,
            _In_ FLOAT zInertiaEndFactor,
            _In_ BOOLEAN isInertiaEndTransformValid,
            _In_ BOOLEAN isBringIntoViewportConfigurationActivated,
            _In_ BOOLEAN isLastDelta);

        // Called when a DirectManipulation manipulation raises its last delta notification.
        // Calls BringIntoViewport if the final layout is not valid for the ScrollViewer.
        // Sets isBringIntoViewportCalled to True only when a call to BringIntoViewport was made.
        _Check_return_ HRESULT HandleManipulationLastDelta(
            _In_ UIElement* pManipulatedElement,
            _In_ DOUBLE xNewOffset,
            _In_ DOUBLE yNewOffset,
            _Out_ BOOLEAN& isBringIntoViewportCalled);

        // Called by the InputManager when a manipulation has completed.
        _Check_return_ HRESULT HandleManipulationCompleted(
            _In_ UIElement* pManipulatedElement,
            _In_ BOOLEAN wasInDirectManipulationZoom,
            _In_ FLOAT xCumulativeTranslation,
            _In_ FLOAT yCumulativeTranslation);

        // Synchronizes the ScrollData's m_ComputedOffset and m_Offset fields.
        _Check_return_ HRESULT SynchronizeScrollOffsets();
        _Check_return_ HRESULT SynchronizeScrollOffsetsAfterThumbDeferring();

        // Show the appropriate scrolling indicators.
        _Check_return_ HRESULT ShowIndicators();

        // Handler for when the NoIndicator state's storyboard completes animating.
        _Check_return_ HRESULT NoIndicatorStateStoryboardCompleted(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2);

        // Handler for when the TouchIndicator or MouseIndicator state's storyboard completes animating.
        _Check_return_ HRESULT IndicatorStateStoryboardCompleted(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2);

        // Raises or delays the ViewChanging event with the provided target transform and IsInertial flag.
        // The event is delayed when m_iViewChangingDelay is strictly positive. In that case the event is
        // raised later when m_iViewChangingDelay reaches 0 again.
        _Check_return_ HRESULT RaiseViewChanging(
            _In_ DOUBLE targetHorizontalOffset,
            _In_ DOUBLE targetVerticalOffset,
            _In_ FLOAT targetZoomFactor);

        // Raises the ViewChanged event with the provided IsIntermediate value unless m_iViewChangedDelay is
        // strictly positive. In that case the event is delayed and raised when m_iViewChangedDelay reaches 0 again.
        _Check_return_ HRESULT RaiseViewChanged(_In_ BOOLEAN isIntermediate);

        // Raise the DirectManipulationStarted/Completed events.
        _Check_return_ HRESULT RaiseDirectManipulationStarted();
        _Check_return_ HRESULT RaiseDirectManipulationCompleted();

        // Increments m_iViewChangingDelay to delay any potential attempt at raising the ViewChanging event.
        void DelayViewChanging();

        // Increments m_iViewChangedDelay to delay any potential attempt at raising the ViewChanged event.
        void DelayViewChanged();

        // Decrements m_iViewChangingDelay and checks if a ViewChanging notification was delayed and can now be raised.
        // DelayViewChanging and FlushViewChanging need to go in pairs.
        _Check_return_ HRESULT FlushViewChanging(_In_ HRESULT hr);

        // Decrements m_iViewChangedDelay and checks if a ViewChanged notofication was delayed and can now be raised.
        // DelayViewChanged and FlushViewChanged need to go in pairs.
        _Check_return_ HRESULT FlushViewChanged(_In_ HRESULT hr);

        // Called internally to update the zoom factor property without batching the ViewChanged notifications.
        _Check_return_ HRESULT ZoomToFactorInternal(_In_ FLOAT value, _In_ BOOLEAN delayAndFlushViewChanged, _Out_opt_ bool* pZoomChanged);

        // Called at the beginning of an operation that may cause several ViewChanged events, like a DM manip.
        _Check_return_ HRESULT EnterIntermediateViewChangedMode();

        // Called at the end of an operation that may have caused several ViewChanged events, like a DM manip.
        _Check_return_ HRESULT LeaveIntermediateViewChangedMode(_In_ BOOLEAN raiseFinalViewChanged);

        // Indicates whether we're at our highest zoom factor (as defined by MaxZoomFactor).
        _Check_return_ HRESULT get_IsAtMaxZoom(_Out_ BOOLEAN& isAtMaxZoom);

        // Indicates whether we're at our lowest zoom factor (as defined by MinZoomFactor).
        _Check_return_ HRESULT get_IsAtMinZoom(_Out_ BOOLEAN& isAtMinZoom);

        // Called when this DM container wants the DM handler to process the current
        // input message, by forwarding it to DirectManipulation.
        // The handler must set the isHandled flag to True if the message was handled.
        _Check_return_ HRESULT ProcessInputMessage(_In_ bool ignoreFlowDirection, _Out_ BOOLEAN& isHandled);

        // Releases and unhooks template parts and their events
        _Check_return_ HRESULT UnhookTemplate();

        // Loaded event handler.
        _Check_return_ HRESULT OnLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        // Unloaded event handler.
        _Check_return_ HRESULT OnUnloaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        // Check whether message should be forwarded after a key press.
        _Check_return_ HRESULT ShouldContinueRoutingKeyDownEvent(
            _In_ wsy::VirtualKey key,
            _Out_ BOOLEAN& continueRouting);

        // Check if the ScrollViewer can scroll in the focus navigation
        // direction
        _Check_return_ HRESULT CanScrollForFocusNavigation(
            _In_ wsy::VirtualKey key,
            _In_  xaml_input::FocusNavigationDirection direction,
            _Out_ bool* canScroll);

        // Check whether this ScrollViewer should clear focus on pointer released or right tapped.
        _Check_return_ HRESULT GetShouldClearFocus(_Out_ BOOLEAN *pShouldClearFocus);

        // Pushes the potential header elements to the inner ScrollContentPresenter.
        _Check_return_ HRESULT SetScrollContentPresenterHeaders();

        // Pushes any parented headers to the manipulation container.
        _Check_return_ HRESULT NotifyHeadersParented();

        // isAssociated==True: Associate the header so it cannot become the child of an element.
        // isAssociated==False: Un-associate the header so it can become the child of the ScrollContentPresenter.
        _Check_return_ HRESULT UpdateHeaderAssociatedStatus(
            _In_ UIElement* pObject,
            _In_ BOOLEAN isTopHeader,
            _In_ BOOLEAN isLeftHeader,
            _In_ BOOLEAN associate);

        // Sets the m_wrPointedElement field which represents the dependency object
        // touched at the beginning of a touch-based manipulation.
        _Check_return_ HRESULT put_PointedElement(
            _In_opt_ IDependencyObject* pValue);

        // Returns the reference to the touched dependency object set at the beginning of a touch-based manipulation.
        _Check_return_ HRESULT get_PointedElement(
            _Outptr_result_maybenull_ IDependencyObject** ppValue);

        // Returns a rounded down dimension for the viewport since DManip only accepts integer viewport values.
        // A rounded down value is used to avoid bugs with unreachable mandatory scroll snap points.
        DOUBLE AdjustPixelViewportDim(
            _In_ DOUBLE pixelViewportDim);

        // Returns a rounded up dimension for the content since DManip only accepts integer content values.
        // A rounded up value is used to avoid bugs with unreachable mandatory scroll snap points.
        DOUBLE AdjustPixelContentDim(
            _In_ DOUBLE pixelContentDim);

        // Returns true iff both horizontal and vertical scrollbars are collapsed. Used to skip scroll bar animations.
        bool AreBothScrollBarsCollapsed();

        // Returns true iff both horizontal and vertical scrollbars are visible.
        bool AreBothScrollBarsVisible();

        // Scroll Anchoring related methods

        _Check_return_ HRESULT RaiseAnchorRequested();

        _Check_return_ HRESULT IsAnchoring(
            _Out_ bool* isAnchoringElementHorizontally,
            _Out_ bool* isAnchoringElementVertically,
            _Out_opt_ bool* isAnchoringFarEdgeHorizontally = nullptr,
            _Out_opt_ bool* isAnchoringFarEdgeVertically = nullptr);

        _Check_return_ HRESULT ComputeViewportAnchorPoint(
            const XRECTF& zoomedViewport,
            _Out_ double* viewportAnchorPointHorizontalOffset,
            _Out_ double* viewportAnchorPointVerticalOffset);

        _Check_return_ HRESULT ComputeElementAnchorPoint(
            bool isForPreArrange,
            _Out_ double* elementAnchorPointHorizontalOffset,
            _Out_ double* elementAnchorPointVerticalOffset);

        _Check_return_ HRESULT ComputeAnchorPoint(
            const wf::Rect& anchorBounds,
            _Out_ double* anchorPointX,
            _Out_ double* anchorPointY);

        _Check_return_ HRESULT ComputeViewportToElementAnchorPointsDistance(
            const XRECTF& zoomedViewport,
            bool isForPreArrange,
            _Out_ wf::Size* distance);

        _Check_return_ HRESULT ClearAnchorCandidates();

        void ResetAnchorElement();

        _Check_return_ HRESULT EnsureAnchorElementSelection(const XRECTF& zoomedViewport);

        _Check_return_ HRESULT ProcessAnchorCandidate(
            _In_ xaml::IUIElement* anchorCandidate,
            _In_ xaml::IUIElement* child,
            wf::Rect viewportAnchorBounds,
            double viewportAnchorPointHorizontalOffset,
            double viewportAnchorPointVerticalOffset,
            _Inout_ double* bestAnchorCandidateDistance,
            _Inout_ xaml::IUIElement** bestAnchorCandidate,
            _Inout_ wf::Rect* bestAnchorCandidateBounds);

        _Check_return_ HRESULT GetDescendantBounds(
            _In_ xaml::IUIElement* child,
            _In_ xaml::IUIElement* descendant,
            _Out_ wf::Rect*);

        _Check_return_ HRESULT GetDescendantBounds(
            _In_ xaml::IUIElement* child,
            _In_ xaml::IUIElement* descendant,
            const wf::Rect& descendantRect,
            _Out_ wf::Rect* descendantBounds);

        _Check_return_ HRESULT IsElementValidAnchor(_In_ xaml::IUIElement* element, _Out_ bool* result);
        _Check_return_ HRESULT IsElementValidAnchor(_In_ xaml::IUIElement* element, _In_ xaml::IUIElement* child, _Out_ bool* isValid);

        bool DoRectsIntersect(
            const wf::Rect& rect1,
            const wf::Rect& rect2);

        _Check_return_ HRESULT PerformPositionAdjustment(bool isHorizontalDimension, float unzoomedAdjustment, const XRECTF& zoomedViewport);

        _Check_return_ HRESULT get_ZoomedHorizontalOffsetWithPendingShifts(_Out_ float* offset);
        _Check_return_ HRESULT get_ZoomedVerticalOffsetWithPendingShifts(_Out_ float* offset);

        _Check_return_ HRESULT EnsureAnchorCandidateVector();

        // We cache these values at the end of ArrangeOverride since the one's tracked by ScrollViewer's extent
        // and viewport properties get set at weird times and is unreliable to use for tracking shifts.
        double m_unzoomedExtentWidth{ 0.0 };
        double m_unzoomedExtentHeight{ 0.0 };
        double m_viewportWidth{ 0.0 };
        double m_viewportHeight{ 0.0 };

        TrackerPtr<wfc::IVector<xaml::UIElement*>> m_anchorCandidates;
        TrackerPtr<wfc::IVector<xaml::UIElement*>> m_anchorCandidatesForArgs;

        bool m_useCandidatesFromArgs{ false };
        bool m_isAnchorElementDirty{ true }; // False when m_anchorElement is up-to-date, True otherwise.

        TrackerPtr<xaml::IUIElement> m_anchorElement;
        wf::Rect m_anchorElementBounds{};

        TrackerPtr<xaml_controls::IAnchorRequestedEventArgs> m_anchorRequestedEventArgs;

        // A ScrollViewer.ChangeView operation, even if not animated, is not synchronous.
        // In other words, right after the call, ScrollViewer.[Vertical|Horizontal]Offset and
        // TransformToVisual are not going to reflect the new viewport. We need to keep
        // track of the pending viewport shift until the ChangeView operation completes
        // asynchronously.
        double m_pendingViewportShiftX{};
        double m_pendingViewportShiftY{};
    };

    // Event handler class for responding to the IScrollSnapPointsInfo.HorizontalSnapPointsChanged event
    template<class T>
    class HorizontalSnapPointsChangedHandler : public ctl::implements<wf::IEventHandler<IInspectable*>>
    {
    public:
        HorizontalSnapPointsChangedHandler(_In_ T* pClass): m_pClass(pClass)
        {}

    public:
        STDMETHODIMP Invoke(_In_ IInspectable* pSender, _In_ IInspectable* pEventArgs) override
        {
            RRETURN(m_pClass->OnSnapPointsChanged(DMMotionTypePanX));
        }

    private:
        T* m_pClass;
    };

    // Event handler class for responding to the IScrollSnapPointsInfo.VerticalSnapPointsChanged event
    template<class T>
    class VerticalSnapPointsChangedHandler : public ctl::implements<wf::IEventHandler<IInspectable*>>
    {
    public:
        VerticalSnapPointsChangedHandler(_In_ T* pClass): m_pClass(pClass)
        {}

    public:
        STDMETHODIMP Invoke(_In_ IInspectable* pSender, _In_ IInspectable* pEventArgs) override
        {
            RRETURN(m_pClass->OnSnapPointsChanged(DMMotionTypePanY));
        }

    private:
        T* m_pClass;
    };
}
