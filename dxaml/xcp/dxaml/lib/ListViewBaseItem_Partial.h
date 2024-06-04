// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListViewBaseItem.g.h"
#include "ListViewBase.g.h"

namespace DirectUI
{
    // Forward declaration.
    class DragDropVisual;
    class IsEnabledChangedEventArgs;

    // Represents a selectable item in a ListView or GridView.  ListViewBaseItem's
    // usage is tied to classes that derive from ListViewBase.  It defers much
    // of the processing of its input events to its parent ListViewBase.

    PARTIAL_CLASS(ListViewBaseItem)
    {
        private:
            // Latest position the mouse left button was pushed. Used to initiate a mouse drag and drop.
            wf::Point m_lastMouseLeftButtonDownPosition;

            // Latest position the user first made contact with the screen.
            wf::Point m_lastTouchDownPosition;

            // The Timeline within the NoSelectionHint VisualState. We register a listener on
            // Completed in order to call UpdateTopmost as needed.
            TrackerPtr<xaml_animation::ITimeline> m_tpNoSelectionHintTimeline;

            // Event for Completed on m_tpNoSelectionHintTimeline.
            ctl::EventPtr<TimelineCompletedEventCallback> m_epNoSelectionHintTimelineCompletedHandler;

            // The Timeline within the NoReorderHint VisualState. We register a listener on
            // Completed in order to call UpdateTopmost as needed.
            TrackerPtr<xaml_animation::ITimeline> m_tpNoReorderHintTimeline;

            // Event for Completed on m_tpNoReorderHintTimeline.
            ctl::EventPtr<TimelineCompletedEventCallback> m_epNoReorderHintTimelineCompletedHandler;

            // The ContentContainer template part.
            TrackerPtr<IUIElement> m_tpContentContainer;

            // The CheckboxContainer template part (phone-only).
            TrackerPtr<IUIElement> m_tpCheckboxContainer;

            // The DragDropVisual used by drag and drop.
            // Handed off to ListViewBase when a drag begins.
            ctl::ComPtr<DirectUI::DragDropVisual> m_spDragDropVisual;

            // The visual to show on holding if the ListViewItem can be dragged
            TrackerPtr<xaml::IUIElement> m_tpHoldingVisual;

            // The pArgs received at the latest OnPointerPressed event.
            TrackerPtr<xaml_input::IPointerRoutedEventArgs> m_tpLastTouchPressedArgs;

            // Unfortunately, we can't rely on the Tapped event to do interaction because of incompatibility with other controls
            // (for instance, tapping a button inside a ListViewBaseItem would result in selection changing, because Button doesn't
            // handle Tapped but instead uses PointerReleased).
            // This is the list of pointer IDs waiting for a PointerReleased to raise an interaction gesture (selection, etc).
            std::list<UINT> m_pendingTapPointerIDs;

            // Timer and variables to help control the Item's Pressed state using Touch
            enum class TouchTimerState
            {
                NoTimer,
                PressedTimer,
                ReleasedTimer
            };

            TouchTimerState m_touchTimerState;
            TrackerPtr<xaml::IDispatcherTimer> m_tpTouchTimer;
            ctl::EventPtr<DispatcherTimerTickEventCallback> m_epTouchTimerEvent;


            // Whether or not to display our pointer over visuals.
            BOOLEAN m_shouldEnterPointerOver;

            // Whether or not the left mouse button is pressed over this ListViewBaseItem.
            BOOLEAN m_isLeftButtonPressed;

            // Whether or not the right mouse button is pressed over this ListViewBaseItem.
            BOOLEAN m_isRightButtonPressed;

            // Whether or not we should be in the Pressed VisualState due to touch interactions.
            BOOLEAN m_inPressedForTouch;

            // Whether or not we should be in the CheckboxPressed VisualState.
            BOOLEAN m_inCheckboxPressedForTouch;

            // Whether or not the user is Holding over this ListViewBaseItem.
            BOOLEAN m_isHolding;

            // UIAutomation Property: whether or not this element is currently grabbed (i.e. being dragged).
            BOOLEAN m_fDragIsGrabbed;

            // If this is TRUE, this ListViewBaseItem is in a selection hint visual state.
            BOOLEAN m_inSelectionHintState;

            // If this is TRUE, this ListViewBaseItem is in a reorder hint visual state.
            BOOLEAN m_inReorderHintState;

            // TRUE if this ListViewBaseItem is above its siblings in ListViewBase.
            // Manipulated by MakeTopmost/ClearTopmost.
            BOOLEAN m_isTopmost;

            // If this is true, start a drag when the mouse moves far enough away from
            // m_lastMouseLeftButtonDownPosition (while the left button is held down).
            // "Far enough away" is defined via system metrics SM_CXDRAG and SM_CYDRAG.
            BOOLEAN m_isCheckingForMouseDrag;

            // Set when we're handing off a drag gesture to our ListViewBase.
            // Used to prevent the release of the CrossSlideViewport on CaptureLost,
            // since we need that viewport even after the swipe has completed (it prevents
            // a pan from occurring). However, we want to release the viewport in other
            // CaptureLost invocations.
            BOOLEAN m_isStartingDrag;

            // This boolean is set to true after we take a capture of the drag visual
            BOOLEAN m_dragVisualCaptured;

            // This boolean is used to indicate whether the Enter, Space or GamepadA key is down (pressed)
            // We will use this value to show the pressed visual state when one of those key down
            BOOLEAN m_isKeyboardPressed;

            // This boolean is used to indicate whether the Item should go into the DragOver visual
            BOOLEAN m_isDragOver;

            static const wf::Point s_center;
            static const double s_holdingVisualOpacity;
            static const double s_holdingVisualScale;

        public:
            // Initializes a new instance of the ListViewBaseItem class.
            ListViewBaseItem();

            // Destroys an instance of the ListViewBaseItem class.
            ~ListViewBaseItem() override;

        protected:
            // Initialize the ListViewBaseItem by setting up its OnLoaded handler.
            _Check_return_ HRESULT Initialize()
                override;

            // Handles when a key is pressed down on the ListViewBaseItem.
            IFACEMETHOD(OnKeyDown)(
                _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
                override;

            // Handles when a key is released on the ListViewBaseItem.
            IFACEMETHOD(OnKeyUp)(
                _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
                override;

            // Called when the user presses a pointer down over the
            // ListViewBaseItem.
            IFACEMETHOD(OnPointerPressed)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            IFACEMETHOD(OnPointerCanceled)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Called when the user holds a pointer down over the
            // ListViewBaseItem (Holding gesture).
            IFACEMETHOD(OnHolding)(_In_ xaml_input::IHoldingRoutedEventArgs* e);

            // Called when the user releases a pointer over the
            // ListViewBaseItem.
            IFACEMETHOD(OnPointerReleased)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Called when a pointer enters a ListViewBaseItem.
            IFACEMETHOD(OnPointerEntered)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Called when a pointer leaves a ListViewBaseItem.
            IFACEMETHOD(OnPointerExited)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Called when a pointer moves within a ListViewBaseItem.
            IFACEMETHOD(OnPointerMoved)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Called when the ListViewBaseItem or its children lose pointer capture.
            IFACEMETHOD(OnPointerCaptureLost)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Called when a pointer makes a right-tap gesture on a ListViewBaseItem.
            IFACEMETHOD(OnRightTapped)(
                _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
                override;

            _Check_return_ HRESULT OnContextRequestedImpl(
                _In_ xaml_input::IContextRequestedEventArgs * pArgs)
                override;

            // Called when the value of the IsEnabled property changes.
            _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

            // Called when the ListViewBaseItem receives focus.
            IFACEMETHOD(OnGotFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs)
                override;

            // Called when the ListViewBaseItem loses focus.
            IFACEMETHOD(OnLostFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs)
                override;

            // Called when the NoSelectionHint VisualState's Storyboard completes.
            _Check_return_ HRESULT OnNoSelectionHintStoryboardCompleted(
                _In_ IInspectable* pSender,
                _In_ IInspectable* pArgs);

            // Called when the NoReorderHint VisualState's Storyboard completes.
            _Check_return_ HRESULT OnNoReorderHintStoryboardCompleted(
                _In_ IInspectable* pSender,
                _In_ IInspectable* pArgs);

            _Check_return_ HRESULT GetLogicalParentForAPProtected(
                _Outptr_ DependencyObject** ppLogicalParentForAP) override;

            // Called when the user drags over this ListViewBaseItem
            IFACEMETHOD(OnDragOver)(
                _In_ xaml::IDragEventArgs* args)
                override;

            // Called when the user drags out of this ListViewBaseItem
            IFACEMETHOD(OnDragLeave)(
                _In_ xaml::IDragEventArgs* args)
                override;

        public:
            // Apply a template to the ListViewBaseItem.
            IFACEMETHOD(OnApplyTemplate)() override;

            _Check_return_ HRESULT OnTouchDragStarted(
                _In_ ixp::IPointerPoint* pointerPoint,
                _In_ xaml_input::IPointer* pointer) override;

            // Change to the correct visual state for the ListViewBaseItem.
            _Check_return_ HRESULT ChangeVisualStateWithContext(
                _In_ VisualStateManagerBatchContext *pContext,
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions) override;

            // determines if this element should be transitioned using the passed in transition
            IFACEMETHOD(GetCurrentTransitionContext)(
                _In_ INT layoutTickId,
                _Out_ ThemeTransitionContext* pReturnValue)
                override;

            // determines if mutations are going fast
            IFACEMETHOD(IsCollectionMutatingFast)(
                _Out_ BOOLEAN* returnValue)
                override;

            IFACEMETHOD(GetDropOffsetToRoot)(
                _Out_ wf::Point* pReturnValue)
                override;

            // Clears flags related to ongoing pointer input in response to a manipulation starting
            // or container recycle.
            // Updates the VisualState as appropriate.
            _Check_return_ HRESULT ClearPointerState();

            // Clears all state related to user interaction. Used to prepare a ListViewBaseItem
            // for a new data item.
            _Check_return_ HRESULT ClearInteractionState();

            _Check_return_ HRESULT ClearHoldingState();

            // Clears the previously cached arguments
            void ClearLastTouchPressedArgs()
            {
                m_tpLastTouchPressedArgs.Clear();
            }

            // Sets the value to display as the dragged item count.
            virtual _Check_return_ HRESULT SetDragItemsCountDisplay(
                _In_ UINT dragItemsCount);

            // Get the parent ListViewBase of this item.  Note that we retrieve
            // the parent via GetParentSelector whose relationship is setup in
            // PrepareContainer and torn down in ClearContainer so it's only
            // valid within that range.  This will obviously be NULL if the
            // parent ItemsControl does not implement IListViewBase.
            ListViewBase* GetParentListView();

            // Since ListViewBaseItem doesn't have a public GUID we can not QI for the interface. Instead we QI for the two know
            // derived classes.
            static _Check_return_ HRESULT QueryFor(_In_ IUnknown* pThis, _Outptr_ ListViewBaseItem** ppListViewBaseItem);

            // UIAutomation Property: Set whether or not the current item is the primary drag item.
            _Check_return_ HRESULT SetDragIsGrabbed(_In_ BOOLEAN newValue);

            // UIAutomation Property: Return whether or not the current item is the primary drag item.
            _Check_return_ HRESULT GetDragIsGrabbed(_Out_ BOOLEAN* value);

            // Returns chrome object associated with item (if any).
            _Check_return_ HRESULT GetItemChrome(
                _Out_ CListViewBaseItemChrome** ppItemChrome);

            // Returns a drag and drop visual for this item.
            // If necessary, this method will create one.
            _Check_return_ HRESULT GetOrCreateDragDropVisual(
                _Outptr_ DirectUI::DragDropVisual** ppVisual);

            // Clears the drag and drop visual.
            void ClearDragDropVisual();

            // Sets the state of the dragVisualCaptured boolean
            _Check_return_ HRESULT SetDragVisualCaptured(_In_ BOOLEAN dragVisualCaptured);

            // Returns TRUE if the dragPoint is in the topHalf of the item
            _Check_return_ HRESULT IsInBottomHalfForExternalReorder(
                _In_ wf::Point dragPoint,
                _In_ xaml_controls::Orientation panelOrientation,
                _Out_ bool* pResult);

        private:

            // Called when the ListViewBaseItem has been constructed and added to
            // the visual tree.
            _Check_return_ HRESULT OnLoaded(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Update our parent ListView when focus changes so it can manage
            // the currently focused item.
            _Check_return_ HRESULT FocusChanged(
                _In_ BOOLEAN hasFocus,
                _In_ BOOLEAN notifyParent);

            // Creates m_spDragDropVisual, if necessary.
            _Check_return_ HRESULT EnsureDragDropVisual(
                _In_ wf::Point visualOffset,
                _In_ bool useSystemDefaultVisual);

            // Creates m_tpHoldingVisual to show that the item can be dragged
            _Check_return_ HRESULT CreateHoldingVisual();

            // Destroys the holding visual if it exists
            _Check_return_ HRESULT DestroyHoldingVisual();

            // Applies position, opacity, and scale transforms to the holding visual
            _Check_return_ HRESULT TransformHoldingVisual(
                _In_ xaml_input::IHoldingRoutedEventArgs* pArgs);

            // Notify our parent ListViewBase (if any) that the user has requested
            // a drag operation with the specified input pointer. Returns whether or
            // not our previous gesture should continue.
            _Check_return_ HRESULT TryStartDrag(
                _In_ xaml_input::IPointer* pointer,
                _In_ ixp::IPointerPoint* pointerPoint,
                _In_ wf::Point dragStartPoint,
                _Out_opt_ BOOLEAN* pShouldPreviousGestureContinue,
                _Out_ BOOLEAN* pDidStartDrag);


            // Obtains the drag visual element for this item.
            _Check_return_ HRESULT GetDragVisual(
                _Outptr_ IUIElement** ppDragVisual);

            // This group of members implements the mouse drag and drop gesture (click and drag
            // outside of a rectangle).

            // Begin tracking the mouse cursor in order to fire a drag start if the pointer
            // moves a certain distance away from m_lastMouseLeftButtonDownPosition.
            _Check_return_ HRESULT BeginCheckingForMouseDrag(
                _In_ xaml_input::IPointer* pPointer);

            // Stop tracking the mouse cursor.
            _Check_return_ HRESULT StopCheckingForMouseDrag(
                _In_ xaml_input::IPointer* pPointer);

            // Return TRUE if we're tracking the mouse and newMousePosition is outside the drag
            // rectangle centered at m_lastMouseLeftButtonDownPosition (see IsOutsideDragRectangle).
            bool ShouldStartMouseDrag(
                _In_ wf::Point newMousePosition);

            // Returns TRUE if testPoint is outside of the rectangle
            // defined by the SM_CXDRAG and SM_CYDRAG system metrics and
            // dragRectangleCenter.
            bool IsOutsideDragRectangle(
                _In_ wf::Point testPoint,
                _In_ wf::Point dragRectangleCenter);

            // Determinese if the RoutedEventArgs originated within
            // the checkbox template part by walking up the visual tree
            // of the RoutedEventArg's OriginalSource element until either
            // the Checkbox or Content container is encountered.
            // Supports a phone-specific scenario.
            _Check_return_ HRESULT IsOverCheckbox(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs,
                _Out_ BOOLEAN* pIsInCheckboxContainer) const;

            // Registers event handlers on the Storyboard within the NoSelectionHint
            // and NoReorderHint VisualStates.
            _Check_return_ HRESULT RegisterNoHintVisualStateHandlers();

            // Unregisters all event handlers on the Storyboard within the NoSelectionHint
            // and NoReorderHint VisualStates.
            _Check_return_ HRESULT UnregisterNoHintVisualStateHandlers();

            // Raises this ListViewBaseItem above its siblings.
            _Check_return_ HRESULT MakeTopmost();

            // Restores this ListViewBaseItem to its default Z-Order.
            _Check_return_ HRESULT ClearTopmost();

            // Sets this ListViewBaseItem to the appropriate Z-Order,
            // based on whether it is in a Hint state.
            _Check_return_ HRESULT UpdateTopmost();

            // Registers a handler to be called when the ListViewBaseItem completes a transition to the named VisualState.
            _Check_return_ HRESULT AttachTransitionCompleted(
                _In_reads_(nStateNameLength + 1) _Null_terminated_ const WCHAR* pszStateName,
                _In_ XUINT32 nStateNameLength,
                _In_ std::function<HRESULT(IInspectable*, IInspectable*)> callableHandler,
                _Outptr_result_maybenull_ xaml_animation::ITimeline** ppAttachedTo,
                _Out_ ctl::EventPtr<TimelineCompletedEventCallback>* pCompletedEvent);

            // Clear flags relating to the visual state.  Called when IsEnabled is set to FALSE
            // or when Visibility is set to Hidden or Collapsed.
            _Check_return_ HRESULT ClearStateFlags();

            // Helper method to call the parent ListViewBase's OnKeyboardReorder.
            _Check_return_ HRESULT OnKeyboardReorder(
                _In_ ListViewBase::KeyboardReorderDirection direction);

            // Call our ListViewBase's Primary or Secondary interaction gesture, depending on whether or not the
            // control key is pressed.
            _Check_return_ HRESULT DoTapInteraction(_Out_ BOOLEAN* pIsHandled);

            // If the given pointer ID has a pending tap interaction, do the action and remove the pointer id from the pending
            // interaction list.
            _Check_return_ HRESULT DoPendingTapInteractionForPointerId(_In_ UINT pointerId, _Out_ BOOLEAN* pIsHandled);

            // Register the given pointer ID as having a pending tap action. The action will be performed on the next PointerReleased
            // for the pointer, unless it is canceled i.e. by PointerExited.
            _Check_return_ HRESULT EnqueuePendingTapInteractionForPointerId(_In_ UINT pointerId);

            // Cancel a pending tap action for the given pointer ID.
            _Check_return_ HRESULT CancelPendingTapInteractionForPointerId(_In_ UINT pointerId);

            // Helper which will try to go to pszPrimaryState if bTryPrimaryState is true.
            // If that cannot happen, it will attempt to go to pszFallbackState.
            _Check_return_ HRESULT GoToStateWithFallback(
                _In_ bool useTransitions,
                _In_ BOOLEAN tryPrimaryState,
                _In_z_ WCHAR* pszPrimaryState,
                _In_z_ WCHAR* pszFallbackState,
                _Out_ BOOLEAN* used);

            _Check_return_ HRESULT ChangeVisualStateWithContextNewStyle(
                _In_ VisualStateManagerBatchContext *pContext,
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions);

            _Check_return_ HRESULT SetUpAutomaticDragTrigger(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            // Sets the state of the isKeyboardPressed boolean
            _Check_return_ HRESULT SetIsKeyboardPressed(_In_ BOOLEAN isKeyboardPressed);

            _Check_return_ HRESULT StartTouchTimer(
                _In_ TouchTimerState touchTimerState);

            _Check_return_ HRESULT EnsureTouchTimer();

            _Check_return_ HRESULT StopTouchTimer();

            // Handler for the Tick event on m_tpTouchTimer
            _Check_return_ HRESULT TouchTimerTickHandler();

            // Sets the state of the isDragOver boolean
            _Check_return_ HRESULT SetIsDragOver(_In_ BOOLEAN isDragOver);

            // Returns TRUE if the item should play the DragOver visual state
            // this is true if AllowDrop is true
            // and if the dragPoint is in the center area of the item
            // 20/60/20 for LVI
            // 30/40/30 for GVI
            _Check_return_ HRESULT IsDragOver(
                _In_ wf::Point dragPoint,
                _In_ xaml_controls::Orientation panelOrientation,
                _Out_ bool* pResult);

        protected:
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // Update the visual states when the Visibility property is changed.
            _Check_return_ HRESULT OnVisibilityChanged() override;

            // Called when the element leaves the tree. Clears the value of the IsDirectManipulationCrossSlideContainer
            // property, used to support Swipe gesture recognition.
            _Check_return_ HRESULT LeaveImpl(
                _In_ bool bLive,
                _In_ bool bSkipNameRegistration,
                _In_ bool bCoercedIsEnabled,
                _In_ bool bVisualTreeBeingReset) override;

            bool ShouldAutomaticDragHelperHandleInputEvents() override
            {
                // ListViewBaseItem handles the pointer/holding events for automatic drag trigger
                return false;
            }

            // Starts a UIElement's Drag and Drop if CanDrag==True
            _Check_return_ HRESULT StartDragIfEnabled(_In_ ixp::IPointerPoint* pointerPoint);
    };
}
