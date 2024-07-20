// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      ListViewBase displays a rich, interactive collection of items.

#pragma once

#include "ListViewBase.g.h"
#include <DataTemplate.g.h>
#include "LiveReorderHelper.h"
#include "DirectManipulationStateChangeHandler.h"

namespace DirectUI
{
    // Forward declaration.
    class DragDropVisual;
    class ListViewBaseHeaderItem;
    class GroupItem;
    class ConnectedAnimation;

    extern __declspec(selectany) const WCHAR ListViewBaseName[] = L"Microsoft.UI.Xaml.ListViewBase.ListViewBaseName";

    // ListViewBase displays a a rich, interactive collection of items.
    PARTIAL_CLASS(ListViewBase)
        , private DirectManipulationStateChangeHandler
    {
        // Give ListViewBaseItem friend access so it can provide interaction
        // notifications.
        friend class ListViewBaseItem;

        // Grant friend access to LoadMoreItemsOperation so it can call
        // LoadItemsAsyncInternal when it is requested to Start.
        friend class LoadMoreItemsOperation;

        private:
            // Enum of the actions we can take when our scroll host's offset changes.
            enum ScrollHostOffsetChangeAction
            {
                // No action.
                ScrollHostOffsetChangeAction_None = 0,
                // Automatic triggering of incremental loading.
                ScrollHostOffsetChangeAction_IncrementalEdgeTrigger = 1,
            };

            // Enum used for identifying tab-stoppable elements for navigation in LVB with modern panel.
            // Other is used for a control outside of LVB. It's also used for serialization.
            enum ElementType
            {
                Other,
                Header,
                Footer,
                Item,
                GroupHeader,

                // This type is used when we want to consider Items & Group Headers together
                // when determining tab stop candidates.
                ItemOrGroupHeader,
            };

            // Represents a pan velocity. Used for edge scrolling.
            struct PanVelocity
            {
                XFLOAT HorizontalVelocity;
                XFLOAT VerticalVelocity;

                PanVelocity()
                {
                    Clear();
                }

                static PanVelocity Stationary()
                {
                    PanVelocity retval;
                    return retval;
                }

                BOOLEAN IsStationary()
                {
                    return (HorizontalVelocity == 0) && (VerticalVelocity == 0);
                }

                void Clear()
                {
                    HorizontalVelocity = VerticalVelocity = 0;
                }
            };

            // Represents a direction for keyboard reordering.
            enum KeyboardReorderDirection
            {
                KeyboardReorderDirection_ToLowerIndex,
                KeyboardReorderDirection_ToHigherIndex
            };

            // Represents the index used as an anchor during extended selection
            // mode.  The anchor is used specifically so range selection will
            // respect the user's original selected item.  For example, click an
            // item in the middle of a directory in file explorer, press
            // Shift+Down several times and note how the selection grows down,
            // but then press Shift+Up even more times and you'll see selection
            // actually flip about the anchor item and start growing upwards.
            UINT* m_pAnchorIndex;

            // The input pointer used for the current drag and drop operation,
            // if any.  During a drag, will be used to ignore input from devices
            // not involved in the drag (i.e., mouse input will be ignored when
            // touch-dragging).
            TrackerPtr<xaml_input::IPointer> m_tpDragPointer;

            // If there is a drag and drop in progress, this is the number of
            // items being dragged. Outside of a drag, the value is undefined.
            UINT m_dragItemsCount;

            // A reference to the item the user is physically dragging (as
            // opposed to items in the selection, but not being directly
            // dragged).  If there is no drag and drop in progress, this field
            // is set to NULL.
            TrackerPtr<xaml_primitives::ISelectorItem> m_tpPrimaryDraggedContainer;

            // The DragDropVisual used by the current drag/drop operation.
            // This DragDropVisual will be drawing m_tpPrimaryDraggedContainer's
            // drag content (see ListViewBaseItem::GetDragVisual).
            TrackerPtr<IInspectable> m_tpDragDropVisual;

            // Set to TRUE while we're in ReorderItemsTo. This prevents us
            // from clearing drag/drop-related fields in our ItemsCollection
            // change handler.
            BOOLEAN m_isInItemReorderAfterDrop;

            // TRUE if there is a drag and drop started by this ListViewBase and
            // the user is dragging over this ListViewBase. NOT VALID outside of
            // a drag and drop (could be TRUE or FALSE).
            BOOLEAN m_isDraggingOverSelf;

            // TRUE if any item in the ListView is in press and hold state
            bool m_isHolding;

            // The event handler that we have attached to determine when the ScrollViewer's
            // offsets have changed. This is only set when incremental loading or prefetching
            // are enabled.
            TrackerPtr<IDPChangedEventHandler> m_tpOffsetChangedHandler;

            // The item that should be in a DragOver state
            TrackerPtr<xaml_primitives::ISelectorItem> m_tpDragOverItem;

            // The current item being interacted with through pointer events. We need to know this so we
            // can forward ManipulationStarted notifications to the item, so it can update its state.
            TrackerPtr<xaml_primitives::ISelectorItem> m_tpHoldingItem;

            // Set before raising ItemClick event, cleared afterwards
            ctl::ComPtr<xaml::IDependencyObject> m_spContainerBeingClicked;

            // The last location of the top-left corner of the drag visual. Only valid during a drag and drop
            // initiated via this ListViewBase. Relative to root visual.
            wf::Point m_lastDragPosition;

            // The last point the user performed a reorder drop will be stored here.
            // Relative to root visual.
            wf::Point m_lastReorderPosition;

            // The action we will take when the scroll host's offset changes.
            ScrollHostOffsetChangeAction m_scrollHostOffsetChangeAction;

            xaml::FocusState m_semanticZoomCompletedFocusState;

            // Edge scrolling begins after a delay. This is the velocity we will take after
            // the delay expires.
            PanVelocity m_pendingAutoPanVelocity;

            // Our current automatic scrolling speed. See ScrollWithVelocity.
            PanVelocity m_currentAutoPanVelocity;

            // Timer that handles the delay between hitting an edge and edge scrolling
            // kicking in.
            TrackerPtr<xaml::IDispatcherTimer> m_tpStartEdgeScrollTimer;

            // True when items are getting fetched from AcynLoadOperation for Increment Loading
            BOOLEAN m_isLoadAsyncInProgress;

            XDOUBLE m_maxContentWidth;
            XDOUBLE m_maxContentHeight;

            // UIAutomation Property: Text that describes what would happen if the currently dragged item were dropped over this target.
            wrl_wrappers::HString m_strDropTargetDropEffect;

            // UIAutomation Property: Array of grabbed items
            TrackerPtr<wfc::IVector<xaml_automation::Provider::IRawElementProviderSimple*>> m_tpDragGrabbedItems;

            // Last point we got from a DragOver event. Only valid during a drag/drop.
            wf::Point m_lastDragOverPoint;

            // Currently focused group index.
            // Can be negative. (-1) means nothing focused.
            INT m_focusedGroupIndex;

            // The index of the last focused group.
            INT m_lastFocusedGroupIndex;

            // The index of the first available item for focus (only valid during a call to ProcessTabStopInternal).
            int m_firstAvailableItemForFocus;

            // The index of the first available group header for focus (only valid during a call to ProcessTabStopInternal).
            int m_firstAvailableGroupHeaderForFocus;

            // The item index to use as a reference when keyboarding navigating
            // off of a group header.
            int m_itemIndexHintForHeaderNavigation;

            ElementType m_lastFocusedElementType;

            // Tells our KeyDown handler whether the key down args came from an item.
            BOOLEAN m_bKeyDownArgsFromItem;

            // On the phone
            // If the SemanticZoom switch has been triggered by a group tap, we want to keep it to
            // Position the jumpList correctly
            TrackerPtr<IInspectable> m_tpSZRequestingItem;

            //
            // Deferred scrolling.
            //

            // If asked to scroll too soon, we delay the scroll until the template is applied.
            // This can happen with semantic zoom and list UI deserialization.
            TrackerPtr<IInspectable> m_tpDeferredScrollToItem;
            xaml_controls::ScrollIntoViewAlignment m_deferredAlignment;

            std::function<HRESULT(IInspectable*)> m_deferredScrollCommand;

            enum class DeferPoint
            {
                // Defer until OnItemsHostAvailable call
                ItemsHostAvailable,
                // Defer until OnLoaded
                ListViewBaseLoaded
            };

            // The point to defer until - ItemsHostAvailable or ListViewBaseLoaded
            DeferPoint m_deferredPoint;

            // The deferred element is a footer. We need to keep track of this to
            // load the footer. The footer is delay loaded by default.
            bool m_isDeferredElementFooter;

            // Schedules a deferred scroll command.
            void ScheduleDeferredScrollCommand(
                _In_ std::function<HRESULT(IInspectable*)>&& command,
                _In_ IInspectable* pScrollToItem,
                _In_ DeferPoint deferUntil);
            // Executes a deferred scroll command (if any) and clears it.
            _Check_return_ HRESULT ExecuteDeferredScrollCommand();

            // Delay from PointerPressed to conversion form candidate to reorderable.
            static const UINT s_reorderConfirmationDelayInMsec;

            // If pointer travels distance smaller than this, it is considered 'noise' and will not reset
            // the conversion timer.
            static const FLOAT s_reorderConfirmationThresholdInDips;

            std::wstring m_itemSelectionGestureId;

            // LiveReorder
            struct
            {
                int draggedItemIndex;
                int draggedOverIndex;
                int itemsCount;
            } m_liveReorderIndices;
            DirectUI::Components::LiveReorderHelper::MovedItems m_movedItems;
            TrackerPtr<xaml::IDispatcherTimer> m_tpLiveReorderTimer;
            ctl::EventPtr<DispatcherTimerTickEventCallback> m_epLiveReorderTimerEvent;
            wadt::DataPackageOperation m_dragAcceptedOperation;

            // Event registrations on the ScrollViewer.
            ctl::EventPtr<ViewChangingEventCallback> m_epScrollViewerViewChangingHandler;

        #pragma region ListViewBase_Partial.cpp members
        // ---------------------------------------------------------------------
        // ListViewBase_Partial.cpp members
        // ---------------------------------------------------------------------
        protected:
            // Initializes a new instance of the ListViewBase class.
            ListViewBase();

            // Destroys an instance of the ListViewBase class.
            ~ListViewBase() override;

            // initialize state
            _Check_return_ HRESULT Initialize() override;

            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // Called when a ListViewBaseItem is getting a new data item to display.
            IFACEMETHOD(PrepareContainerForItemOverride)(
                _In_ xaml::IDependencyObject* element,
                _In_ IInspectable* item)
                override;

            // Called when a ListViewBaseItem is getting recycled
            // Deferred
            IFACEMETHOD(ClearContainerForItemOverride)(
                _In_ xaml::IDependencyObject* element,
                _In_ IInspectable* item)
                override;

            // Called when the user releases a pointer over the
            // ListViewBase.
            IFACEMETHOD(OnPointerReleased)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Called when a pointer moves within a ListViewBase.
            IFACEMETHOD(OnPointerMoved)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Called when the ListViewBase or its children lose pointer capture.
            IFACEMETHOD(OnPointerCaptureLost)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Invoked when ItemsHost is available
            _Check_return_ HRESULT OnItemsHostAvailable() override;

            // GetFocusedGroupIndex and SetFocusedGroupIndex are consistently used instead of 
            // m_focusedGroupIndex to make it easier to track when this field is read & written.
            INT GetFocusedGroupIndex() const
            {
                return m_focusedGroupIndex;
            }

            void SetFocusedGroupIndex(INT focusedGroupIndex)
            {
                if (m_focusedGroupIndex != focusedGroupIndex)
                {
                    m_focusedGroupIndex = focusedGroupIndex;
                }
            }

            // GetLastFocusedGroupIndex and SetLastFocusedGroupIndex are consistently used instead of 
            // m_lastFocusedGroupIndex to make it easier to track when this field is read & written.
            INT GetLastFocusedGroupIndex() const
            {
                return m_lastFocusedGroupIndex;
            }

            void SetLastFocusedGroupIndex(INT lastFocusedGroupIndex)
            {
                if (m_lastFocusedGroupIndex != lastFocusedGroupIndex)
                {
                    m_lastFocusedGroupIndex = lastFocusedGroupIndex;
                }
            }

        public:
            _Check_return_ HRESULT IsScrollable(_Out_ bool* scrollable);

            // Returns true if the ListView is both source and target
            // of the current Drag and Drop operation, and has reordering enabled
            _Check_return_ HRESULT IsCurrentlyReordering(_Out_ bool* isReordering);

            // Used to handle back button presses on phone
            _Check_return_ HRESULT OnBackButtonPressedImpl(_Out_ BOOLEAN* pHandled);

            // Causes the object to scroll into view.  If it is not visible, it
            // is aligned either at the top or bottom of the viewport.
            _Check_return_ HRESULT ScrollIntoViewImpl(
                _In_ IInspectable* item);

            // Causes the object to scroll into view.
            // Item will be always alligned based on alignment mode.
            _Check_return_ HRESULT ScrollIntoViewWithAlignmentImpl(
                _In_ IInspectable* item,
                _In_ xaml_controls::ScrollIntoViewAlignment alignment);

            // Overload of ScrollIntoViewWithAlignment which supports an animation mode.
            _Check_return_ HRESULT ScrollIntoViewWithOptionalAnimationImpl(
                _In_ IInspectable* item,
                _In_ xaml_controls::ScrollIntoViewAlignment alignment,
                _In_ BOOLEAN disableAnimation);

            // Apply a template to the ListViewBase.
            IFACEMETHOD(OnApplyTemplate)()
                override;

            // ArrangeOverride
            IFACEMETHOD(ArrangeOverride)(
                _In_ wf::Size arrangeSize,
                _Out_ wf::Size* pReturnValue)
                override;

            // MeasureOverride
            IFACEMETHOD(MeasureOverride)(
                _In_ wf::Size availableSize,
                _Out_ wf::Size* pDesired)
                override;

            // DirectManipulationStateChangeHandler implementation
            _Check_return_ HRESULT NotifyStateChange(
                _In_ DMManipulationState state,
                _In_ FLOAT xCumulativeTranslation,
                _In_ FLOAT yCumulativeTranslation,
                _In_ FLOAT zCumulativeFactor,
                _In_ FLOAT xCenter,
                _In_ FLOAT yCenter,
                _In_ BOOLEAN isInertial,
                _In_ BOOLEAN isTouchConfigurationActivated,
                _In_ BOOLEAN isBringIntoViewportConfigurationActivated) override;

            // ItemClick for Invoke UIA pattern
            _Check_return_ HRESULT AutomationItemClick(_In_ ListViewBaseItem* pItem);

            // HeaderItemClick for Invoke UIA pattern
            _Check_return_ HRESULT AutomationHeaderItemClick(_In_ ListViewBaseHeaderItem* headerItem);

            // return string representation of Header property
            _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

            // FocusManager GetFirst/LastFocusableElementOverride
            _Check_return_ HRESULT GetFirstFocusableElementOverride(
                _Outptr_ DependencyObject** ppFirstFocusable) override;

            _Check_return_ HRESULT GetLastFocusableElementOverride(
                _Outptr_ DependencyObject** ppLastFocusable) override;

            // Returns next focusable element in a specified direction.
            // Since Get{First,Last}FocusableElementOverride is called in different context after refactoring change 601761
            // this method needs to be called from Panel::GetFirstFocusableElementOverride in order to retain the same call order as in Win8.
            _Check_return_ HRESULT GetFocusableElement(
                const bool isBackward,
                _Outptr_ DependencyObject** ppFocusable);

            #pragma region IGeneratorHost overrides

            // For certain situations (such as drag/drop), the host may hang onto a container for an extended
            // period of time. That particular container shouldn't ever be recycled as long as it's being used.
            // This method asks whether or not the given container is eligible for recycling.
            _Check_return_ IFACEMETHOD(CanRecycleContainer)(
                _In_ xaml::IDependencyObject* pContainer,
                _Out_ BOOLEAN* pCanRecycleContainer) override;

            // During lookups of duplicate or null values, there might be a container that
            // the host can provide the ICG.
            _Check_return_ IFACEMETHOD(SuggestContainerForContainerFromItemLookup)(
                _Outptr_ xaml::IDependencyObject** ppContainer) override;

            // implemented in ListViewBase_Partial_ContainerPhase.cpp
            // There is no SetupContainerContentChangingAfterClear, this is handled in ClearContainer directly.
            _Check_return_ IFACEMETHOD(SetupContainerContentChangingAfterPrepare)(
                _In_ xaml::IDependencyObject* container,
                _In_ IInspectable* item,
                _In_ INT itemIndex,
                _In_ wf::Size measureSize) override;

            _Check_return_ IFACEMETHOD(RegisterWorkFromArgs)(
                _In_ xaml_controls::IContainerContentChangingEventArgs* pArgs) override;

            _Check_return_ IFACEMETHOD(RegisterWorkForContainer)(
                _In_ xaml::IUIElement* pContainer) override;

            // Sets up a link from group header to parent LVB used to relay calls.  Used only for modern panels.
            _Check_return_ IFACEMETHOD(PrepareGroupContainer)(
                _In_ xaml::IDependencyObject* pContainer,
                _In_ xaml_data::ICollectionViewGroup* pGroup) override;

            // Clears a link from group header to parent LVB used to relay calls.  Used only for modern panels.
            _Check_return_ IFACEMETHOD(ClearGroupContainerForGroup)(
                _In_ xaml::IDependencyObject* pContainer,
                _In_opt_ xaml_data::ICollectionViewGroup* pItem) override;

            _Check_return_ IFACEMETHOD(ShouldRaiseChoosingItemContainer)(
                _Out_ BOOLEAN* pShouldRaiseChoosingItemContainer) override;

            _Check_return_ IFACEMETHOD(RaiseChoosingItemContainer)(
                _In_ xaml_controls::IChoosingItemContainerEventArgs* pArgs) override;

            _Check_return_ IFACEMETHOD(ShouldRaiseChoosingGroupHeaderContainer)(
                _Out_ BOOLEAN* pShouldRaiseChoosingGroupHeaderContainer) override;

            _Check_return_ IFACEMETHOD(RaiseChoosingGroupHeaderContainer)(
                _In_ xaml_controls::IChoosingGroupHeaderContainerEventArgs* pArgs) override;

            _Check_return_ IFACEMETHOD(RaiseContainerContentChangingOnRecycle)(
                _In_ xaml::IUIElement* container,
                _In_ IInspectable* item) override;

            bool IsDraggableOrPannableImpl() override;

            _Check_return_ HRESULT PrepareConnectedAnimationImpl(_In_ HSTRING key, _In_ IInspectable* pItem, _In_ HSTRING elementName, _Outptr_ xaml_animation::IConnectedAnimation** ppReturnValue);
            _Check_return_ HRESULT TryStartConnectedAnimationAsyncImpl(_In_ xaml_animation::IConnectedAnimation* pAnimation, _In_ IInspectable* pItem, _In_ HSTRING elementName, _Outptr_ wf::IAsyncOperation<bool>** pReturnValue);

            #pragma endregion

            // if we have pending phasing work, register for buildtrees when
            // we enter back into the tree (if we are not in there already)
            _Check_return_ HRESULT RegisterIfWorkPending();

        protected:
            // guaranteed to be called when we call prepare (even if base is not)
            // listview sets content only when the ccc event is raised
            _Check_return_ HRESULT PreProcessContentPreparation(
                _In_ xaml::IDependencyObject* pContainer,
                _In_opt_ IInspectable* pItem) override;

        private:
            // Adds or removes a property changed handler to the ScrollHost.
            _Check_return_ HRESULT EnsureScrollHostOffsetChangeAction();

            // Adds a property changed handler to the ScrollHost.
            // Used to call OnScrollHostOffsetsChanged for Data Virtualization.
            _Check_return_ HRESULT AttachScrollViewerPropertyChanged();

            // Removes the property changed handler we attached in AttachScrollViewerPropertyChanged
            // from the ScrollHost. Used for Data Virtualization.
            _Check_return_ HRESULT DetachScrollViewerPropertyChanged();

            // ScrollHost property changed handler callback
            _Check_return_ HRESULT OnScrollHostPropertyChanged(
                _In_ xaml::IDependencyObject* pSender,
                _In_ const CDependencyProperty* pDP);

            // Called when the ScrollHost's scrolling offsets changes.
            _Check_return_ HRESULT OnScrollHostOffsetsChanged();

            // In case of DataVirtualization, if number of of items on screen are less than availableSize,
            // this will trigger to LoadMoreItems
            _Check_return_ HRESULT LoadMoreItemsIfNeeded(
                _In_ wf::Size finalSize);

            // Shared implementation for GetFirstFocusableElementOverride and GetLastFocusableElementOverride.
            _Check_return_ HRESULT GetFocusableElementForModernPanel(
                _In_ BOOLEAN isBackward,
                _Outptr_ DependencyObject** ppFocusable);

        private:

            // we keep a list of containers we have called clear on. Each time we call prepare, we remove it from this list
            // at the end of measure, we know to only execute code on containers that are left in this list.
            // if the below looks like a hack: it is. We need to keep these two vectors in sync.
            TrackerPtr<TrackerCollection<xaml::UIElement*>> m_toBeClearedContainers;
            TrackerPtr<TrackerCollection<IInspectable*>> m_toBeClearedItems;

            // Flags indicating whether items/groups should be tab stops.
            BOOLEAN m_itemsAreTabStops;
            BOOLEAN m_groupsAreTabStops;

            // flag indicating that an item or group header was focused before. If this is false
            // that means that items/group headers never got focus so far.
            bool m_wasItemOrGroupHeaderFocused;

            bool m_focusableElementForModernPanelReentrancyGuard;

            _Check_return_ HRESULT UpdateTabStopFlags();

            BOOLEAN m_allowDrop;

            // This flag is set to true while in ListViewBase::OnSelectionModeChanged.
            // Selection might change when changing selection mode. In this case, we don't
            // want to bring the new selected item into view.
            // A typical case for this is when the ItemsSource is a CollectionViewSource and we
            // are switching from Multiple to Single selection mode. In this case, UpdateCVSynchronizationState
            // will get called and we will select the collection view's current item, but we should not bring it
            // into view.
            // Additional note: this is really only an issue if this control has focus when the selection mode changes.
            // Which is the case on phone when the user is using the (phone) AppBar since the latter don't take focus.
            BOOLEAN m_isInOnSelectionModeChanged;

            ctl::EventPtr<UIElementGettingFocusEventCallback> m_gettingFocusHandler;

            // Helper for getting next focusable control outside of LVB.
            _Check_return_ HRESULT GetNextFocusablePeer(
                const bool isBackward,
                _Outptr_ IDependencyObject** ppOutsidePeer);

            // Ensure itemIndex is within valid range for group with index of groupIndex.  If alwaysSet is TRUE, then
            // set pNewItemIndex to the first item index (atBeginning = TRUE) or the last.
            _Check_return_ HRESULT ValidateItemIndexForGroup(
                _In_ UINT itemIndex,
                _In_ UINT groupIndex,
                _In_ BOOLEAN atBeginning,
                _In_ BOOLEAN forceSet,
                _Out_ UINT* pNewItemIndex);

            // Test if there are any elements in group.
            _Check_return_ HRESULT GroupHasElements(
                _In_ UINT groupIndex,
                _Out_ BOOLEAN* pGroupHasElements);

            // Test if there are empty group headers at the beginning or at the end.
            _Check_return_ HRESULT HasBoundaryEmptyGroup(
                _In_ BOOLEAN atBeginning,
                _Out_ BOOLEAN* pHasBoundaryEmptyGroup);

            // Walk up visual tree from pChild towards this LVB, looking for the last element which is an
            // item, group header item, header or footer.
            _Check_return_ HRESULT IdentifyParentElement(
                _In_opt_ IDependencyObject* pChild,
                _Out_ ElementType* pParentElementType,
                _Outptr_ IDependencyObject** pParent);

            // Given a starting element type transition states until the next tab stop is found.
            _Check_return_ HRESULT GetNextTabStopForElementType(
                _In_ ElementType elementType,
                const bool isBackward,
                _Outptr_ IDependencyObject** ppNextElement);

            // Get next tab stop for currently focused element type.
            _Check_return_ HRESULT GetNextTabStop(
                _In_ ElementType elementType,
               const bool isBackward,
                _Outptr_ IDependencyObject** ppNextElement);

            // The Panel stores the focus candidate (header or item). For example
            // if there is a scroll into view, the panel would keep the scrolled item
            // as the candidate.
            _Check_return_ HRESULT GetFocusCandidateFromPanel(
                _Outptr_ xaml::IDependencyObject** ppFocusCandidate);

            // State machine for determining next tab stop.
            // Given the current state information it tests if the transition to candidate state is a valid one
            // and if so, it returns a container for the next tab stop.  If the candidate is not accepted, it
            // advances the state.
            // initialElementType - type of element currenly having focus
            // candidateElementType - state being tested
            // isBackward - direction
            // pNewElementType - new state transition
            // ppNextElement - next tab stop
            _Check_return_ HRESULT TryNextElementType(
                _In_ ElementType initialElementType,
                _In_ ElementType candidateElementType,
                const bool isBackward,
                _Out_ ElementType* pNewElementType,
                _Outptr_ IDependencyObject** ppNextElement);

            // Determines the next tab stop given currently focused element and default focus candidate.
            _Check_return_ HRESULT ProcessTabStopInternal(
                _In_opt_ DependencyObject* pFocusedElement,
                _In_opt_ DependencyObject* pCandidateElement,
                const bool isBackward,
                const bool didCycleFocusAtRootVisualScope,
                _Out_ BOOLEAN* pHandled,
                _Outptr_ DependencyObject** ppNewTabStop);

            // For Gamepad page navigation, this will return false for: 1) trigger keys on horizontal list 2) shoulder keys for vertical list
            _Check_return_ HRESULT CanPerformKeyboardNavigation(
                _In_ wsy::VirtualKey key,
                _Out_ bool* canNavigate);

            _Check_return_ HRESULT FindNextTabStopCandidateFromItem(
                int referenceItemIndex,
                const bool isBackwards,
                const bool supportGroupHeaders,
                _Out_ int* focusCandidateIndex,
                _Out_ bool* isCandidateAGroupHeader);

            _Check_return_ HRESULT GetCacheStartAndEnd(
                bool isForGroupIndexes,
                _Out_ int* cacheIndexStart,
                _Out_ int* cacheIndexEnd);

            _Check_return_ HRESULT OnGettingFocus(_In_ xaml::IUIElement* sender, _In_ xaml_input::IGettingFocusEventArgs* args);

            // Get the next state from current one in the specified direction.
            // The order for isBackward = FALSE is: Other -> Header-> Item -> GroupHeader -> Footer -> Other
            static ElementType GetElementTypeTransition(
                _In_ ElementType current,
                const bool isBackward);

            static inline BOOLEAN IsListViewBaseItem(
                _In_ IInspectable* pItem)
            {
                return ctl::is<xaml_controls::IListViewItem>(pItem) ||
                       ctl::is<xaml_controls::IGridViewItem>(pItem);
            }

            static inline BOOLEAN IsListViewBaseHeaderItem(
                _In_ IInspectable* pItem)
            {
                return ctl::is<xaml_controls::IListViewHeaderItem>(pItem) ||
                       ctl::is<xaml_controls::IGridViewHeaderItem>(pItem);
            }

        #pragma endregion

        #pragma region ListViewBase_Partial_Selection.cpp members
        // ---------------------------------------------------------------------
        // ListViewBase_Partial_Selection.cpp members
        // ---------------------------------------------------------------------
        public:
            // Gets a collection containing the currently selected items in the
            // ListView.
            _Check_return_ HRESULT get_SelectedItemsImpl(
                _Outptr_ wfc::IVector<IInspectable*>** pValue);

            // Gets a collection containing the currently selected ranges in the
            // ListView.
            _Check_return_ HRESULT get_SelectedRangesImpl(
                _Outptr_ wfc::IVectorView<xaml_data::ItemIndexRange*>** ppValue);

            // Selects all the items in the ListView.
            _Check_return_ HRESULT SelectAllImpl();

            // Selects a range of items in the ListView.
            _Check_return_ HRESULT SelectRangeImpl(
                _In_ xaml_data::IItemIndexRange* pItemIndexRange);

            // Deselects a range of items in the ListView.
            _Check_return_ HRESULT DeselectRangeImpl(
                _In_ xaml_data::IItemIndexRange* pItemIndexRange);

            // Deselect all the items in the ListView.
            _Check_return_ HRESULT DeselectAllImpl();

            _Check_return_ HRESULT GetDropOffsetToRoot(
                _Out_ wf::Point* returnValue)
                override;

            _Check_return_ HRESULT UpdateClip();

        protected:
            // Gets a value indicating whether the ListView can select multiple
            // items at once.
            _Check_return_ HRESULT get_CanSelectMultiple(
                _Out_ BOOLEAN* pValue)
                override;

            _Check_return_ HRESULT IsSelectionPatternApplicable(
                _Out_ bool* selectionPatternApplicable)
                override;

            // Additional processing that occures after Selector has changed the
            // selected item.
            _Check_return_ HRESULT OnSelectionChanged(
                _In_ INT oldSelectedIndex,
                _In_ INT newSelectedIndex,
                _In_ IInspectable* pOldSelectedItem,
                _In_ IInspectable* pNewSelectedItem,
                _In_ BOOLEAN animateIfBringIntoView = FALSE,
                _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None)
                override;

            // Handles changes to the Items collection.  We should adjust m_pAnchorIndex if
            // any item before that index was added or removed. Additionally, we should
            // cancel a drag/drop if the item we're dragging is removed, and we should cancel
            // any swipe in progress.
            _Check_return_ HRESULT OnItemsChanged(
                _In_ wfc::IVectorChangedEventArgs* pArgs)
                override;

            _Check_return_ HRESULT FocusImpl(
                _In_ xaml::FocusState value,
                _Out_ BOOLEAN* returnValue) override;

        private:
            // Handles changes to the SelectionMode property.
            // TODO: Make this a protected virtual method?
            _Check_return_ HRESULT OnSelectionModeChanged(
                _In_ xaml_controls::ListViewSelectionMode oldMode,
                _In_ xaml_controls::ListViewSelectionMode newMode);

            // Handles selection of single item, and only that item
            _Check_return_ HRESULT MakeSingleSelection(
                _In_ UINT index,
                _In_ BOOLEAN animateIfBringIntoView,
                _In_opt_ IInspectable* pSelectedItem,
                _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None) override;

            _Check_return_ HRESULT MakeSingleSelection(
                _In_ ListViewBaseItem* pItem,
                _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None);

            // Toggle IsSelected of item
            _Check_return_ HRESULT MakeToggleSelection(
                _In_ ListViewBaseItem* pItem);

            // Select a range of items beginning with current anchor.
            _Check_return_ HRESULT MakeRangeSelection(
                _In_ UINT index,
                _In_ BOOLEAN clearOldSelection);

            // Select a range of items beginning
            _Check_return_ HRESULT MakeRangeSelection(
                _In_ ListViewBaseItem* pItem,
                _In_ BOOLEAN clearOldSelection);

            // Set the extended anchor index
            _Check_return_ HRESULT SetAnchorIndex(
                _In_ UINT index)
            {
                HRESULT hr = S_OK;

                if (!m_pAnchorIndex)
                {
                    m_pAnchorIndex = new UINT(index);
                }
                else
                {
                    *m_pAnchorIndex = index;
                }

                RRETURN(hr);//RRETURN_REMOVAL
            }

            // Release the extended selection anchor index.
            void ReleaseAnchorIndex()
            {
                delete m_pAnchorIndex;
                m_pAnchorIndex = NULL;
            }

            // Stores a handle to the items source as an ISelectionInfo if possible
            _Check_return_ HRESULT InitializeDataSourceSelectionInfo();

        #pragma endregion

        #pragma region ListViewBase_Partial_Interaction.cpp members
        // ---------------------------------------------------------------------
        // ListViewBase_Partial_Interaction.cpp members
        // ---------------------------------------------------------------------
        public:
            // Handler for when a GroupItem receives focus.
            _Check_return_ HRESULT GroupItemFocused(_In_ GroupItem* pItem);

            // Handler for when a GroupItem loses focus.
            _Check_return_ HRESULT GroupItemUnfocused(_In_ GroupItem* pItem);

            // Handler called by ListViewBaseHeaderItem when it receives focus.
            _Check_return_ HRESULT GroupHeaderItemFocused(
                _In_ ListViewBaseHeaderItem* pItem);

            // Handler called by ListViewBaseHeaderItem when it loses focus.
            _Check_return_ HRESULT GroupHeaderItemUnfocused(
                _In_ ListViewBaseHeaderItem* pItem);

            // Handles changes to GroupItems focus (as a whole collection of groups). If a group
            // header has focus, forward focus to the last focused header.
            _Check_return_ HRESULT OnGroupFocusChanged(
                _In_ BOOLEAN hasFocus, _In_ BOOLEAN headerHasFocus, _In_ xaml::FocusState howFocusChanged);

            // Returns TRUE if there is a currently focused GroupItem.
            BOOLEAN HasFocusedGroup()
            {
                return GetFocusedGroupIndex() >= 0;
            }

            // Tells our KeyDown handler that the key down args came from an item so we should handle them.
            // Otherwise we should pass them to the scroll view.
            void SetHandleKeyDownArgsFromItem(_In_ bool fFromItem);

            // Handles a key press on a group header.
            _Check_return_ HRESULT OnGroupHeaderKeyDown(
                _In_opt_ IInspectable* pItem,
                _In_ wsy::VirtualKey originalKey,
                _In_ wsy::VirtualKey key,
                _Out_ BOOLEAN* pHandled);

            // Handles a key up on a group header.
            _Check_return_ HRESULT OnGroupHeaderKeyUp(
                _In_opt_ IInspectable* pItem,
                _In_ wsy::VirtualKey originalKey,
                _In_ wsy::VirtualKey key,
                _Out_ BOOLEAN* pHandled);

            // Handles the tap gesture for a ListViewHeaderBaseItem
            _Check_return_ HRESULT OnHeaderItemTap(_In_opt_ IInspectable* pItem, _Out_ BOOLEAN* pIsHandled);

        protected:
            // Handles the primary interaction gesture for a ListViewBaseItem (Tap,
            // Click, or Space) and updates selection or raises the ItemClick
            // event.
            _Check_return_ HRESULT OnItemPrimaryInteractionGesture(
                _In_ ListViewBaseItem* pItem,
                _In_ BOOLEAN isKeyboardInput,
                _Out_ BOOLEAN* pIsHandled);

            // Handles the secondary interaction gesture for a ListViewBaseItem
            // (Ctrl+Click or Ctrl+Space) and updates selection
            // accordingly.
            _Check_return_ HRESULT OnItemSecondaryInteractionGesture(
                _In_ ListViewBaseItem* pItem,
                _In_ BOOLEAN isKeyboardInput,
                _Out_ BOOLEAN* pIsHandled);

            // Selects item(s) based on the current SelectionMode, given a primary interaction
            // gesture.
            _Check_return_ HRESULT OnSelectItemPrimary(
                _In_ ListViewBaseItem* pItem,
                bool isKeyboardInput,
                bool shouldFocusItem,
                _Out_ BOOLEAN* pIsHandled);

            // Selects item(s) based on the current SelectionMode, given a secondary interaction
            // gesture.
            _Check_return_ HRESULT OnSelectItemSecondary(
                _In_ ListViewBaseItem* pItem,
                _In_ BOOLEAN isKeyboardInput,
                _Out_ BOOLEAN* pIsHandled);

            // Raises the ItemClick event.
            _Check_return_ HRESULT OnItemClick(
                _In_ ListViewBaseItem* pItem);

            // Handles when a key is pressed down on the ListViewBase.
            IFACEMETHOD(OnKeyDown)(
                _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
                noexcept override;

            // Handles when a key is released on the ListViewBase.
            IFACEMETHOD(OnKeyUp)(
                _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
                override;

            // Handles keyboard navigation for the ListViewBase.  By default this
            // will refer to the ItemsHost Panel to provide layout specific
            // knowledge of keyboard navigation, but also includes generic
            // StackPanel-like behavior if ListViewBase is being used with an
            // unsophisticated Panel implementation.
            _Check_return_ HRESULT OnKeyboardNavigation(
                _In_ wsy::VirtualKey key,
                _In_ wsy::VirtualKey originalKey,
                _In_ BOOLEAN animateIfBringIntoView,
                _Out_ INT* pNewFocusedIndex,
                _Out_ xaml_controls::ElementType* pNewFocusedType);

            // Handles keyboard navigation on a group header.
            _Check_return_ HRESULT OnGroupKeyboardNavigation(
                _In_ wsy::VirtualKey key,
                _In_ wsy::VirtualKey originalKey,
                _In_ BOOLEAN animateIfBringIntoView,
                _Out_ BOOLEAN* pHandled);

            // Toggles the SemanticZoom active view
            _Check_return_ HRESULT ToggleSemanticZoomActiveView(
                _In_opt_ IInspectable* pItem,
                _Out_ BOOLEAN* pHandled);

            // Asks the given Panel what to do with a navigation action,
            // given a starting index. Returns the destination index,
            // whether or not the Panel handled the key, and whether
            // or not the panel needs us to fall back to the default
            // Selector behavior. The given verification function
            // determines whether or not a given item is an eligible
            // target for navigation.
            _Check_return_ HRESULT QueryPanelForItemNavigation(
                _In_ xaml_controls::IPanel* pTargetPanel,
                _In_ xaml_controls::KeyNavigationAction action,
                _In_ INT sourceFocusedIndex,
                _In_ xaml_controls::ElementType sourceFocusedType,
                _In_ std::function<HRESULT (UINT, xaml_controls::ElementType, BOOLEAN*)> verificationFunction,
                _In_ BOOLEAN allowWrap,
                _In_ UINT itemIndexHintForHeaderNavigation,
                _Out_ INT* pNewFocusedIndex,
                _Out_ xaml_controls::ElementType* pNewFocusedType,
                _Out_ BOOLEAN* pIsHandled,
                _Out_ BOOLEAN* pUseFallback);

            // Asks the default Selector key navigation logic what to do
            // with the given navigation key. Never call this if we're
            // grouping.
            // This should really be combined with QueryPanelForItemNavigation,
            // but Selector's logic is too convoluted to justify messing with it.
            // Better to encapsulate Selector's inability to handle group navigation
            // into this function and QueryGroupFallbackForItemNavigation.
            _Check_return_ HRESULT QuerySelectorFallbackForItemNavigation(
                _In_ wsy::VirtualKey key,
                _In_ INT sourceFocusedIndex, // Relative to entire source.
                _Out_ INT* pNewFocusedIndex,
                _Out_ BOOLEAN* pIsHandled);

            // Panels may decide to use the default Selector logic for keyboard
            // navigation (see QueryPanelForItemNavigation's pUseFallback). However,
            // the usual Selector logic cannot handle grouping. Instead, we re-implement
            // a grouping-aware version here. This particular method attempts to navigate
            // WITHIN a group - it returns pIsHandled = FALSE if it hits a group boundary.
            // See QueryGroupFallbackForGroupNavigation.
            _Check_return_ HRESULT QueryGroupFallbackForItemNavigation(
                _In_ xaml_controls::IPanel* pGroupPanel,
                _In_ wsy::VirtualKey key,
                _In_ INT sourceIndexInCollection,
                _In_ INT groupLeftEdgeIndex,
                _In_ INT groupRightEdgeIndex,
                _Out_ INT* pNewFocusedIndexInCollection,
                _Out_ BOOLEAN* pIsHandled);

            // Panels may decide to use the default Selector logic for keyboard
            // navigation (see QueryPanelForItemNavigation's pUseFallback). However,
            // the usual Selector logic cannot handle grouping. Instead, we re-implement
            // a grouping-aware version here. This particular method attempts to navigate
            // BETWEEN groups. QueryGroupFallbackForItemNavigation handles the in-group
            // navigation.
            _Check_return_ HRESULT QueryGroupFallbackForGroupNavigation(
                _In_ wsy::VirtualKey key,
                _In_ INT sourceGroupIndex,
                _Out_ INT* pNewGroupIndex,
                _Out_ BOOLEAN* pIsHandled);

            // Given a key, returns the appropriate navigation action.
            _Check_return_ HRESULT TranslateKeyToKeyNavigationAction(
                _In_ wsy::VirtualKey key,
                _Out_ xaml_controls::KeyNavigationAction* pNavAction,
                _Out_ bool* pIsValidKey);

            // Given a key, returns whether or not we must handle the nav direction at the outer panel.
            // This is true for keys such as home or end - they should ignore groups.
            _Check_return_ HRESULT ShouldNavDirectionIgnoreGroups(
                xaml_controls::KeyNavigationAction action,
                _Out_ BOOLEAN* pIsGlobal);

            // Fetches group details, given a group index.
            _Check_return_ HRESULT GetGroupInformation(
                _In_ UINT groupIndex,
                _Out_ UINT* pGroupStartIndex,
                _Out_ UINT* pGroupSize,
                _Out_ BOOLEAN* pIsValid);

            // Gets the number of groups we have.
            _Check_return_ HRESULT GetGroupCount(
                _Out_ INT* pGroupCount);

            // Gets the first selectable item within the given group.
            // Can start from the end or the beginning of the group.
            _Check_return_ HRESULT FindFirstSelectableItemInGroup(
                _In_ UINT groupIndex,
                _In_ BOOLEAN fromBeginning,
                _In_ INT startIndexInCollection, /* if -1, defaults to start of group if fromBeginning is set, from end of group otherwise */
                _Out_ INT* pFirstSelectableItemIndexInCollection,
                _Out_ BOOLEAN* pFoundSelectableItem);

            // Given a source item and target group and a keynav action,
            // find which item in the target group is closest to the source item.
            // "Closest" means: closest to the point returned by
            // GetNavigationSourcePoint(source item).
            // Also indicates whether or not an item was found,
            // and whether or not to default to the fallback implementation
            // (FindFirstSelectableItemInGroup).
            _Check_return_ HRESULT GetClosestIndexToItemInGroupPanel(
                _In_ UINT sourceIndexInCollection,
                _In_ UINT targetGroupIndex,
                _In_ xaml_controls::KeyNavigationAction action,
                _In_ xaml_controls::IGroupItem* pSourceGroupItem,
                _Out_ INT* pClosestIndex,
                _Out_ BOOLEAN* pFoundItem,
                _Out_ BOOLEAN* pUseFallback);

            // Given an element and a navigation direction, return the point
            // that should be considered the "source" of the navigation.
            _Check_return_ HRESULT GetNavigationSourcePoint (
                _In_ FrameworkElement* pElementNavigatedFrom,
                _In_ xaml_controls::KeyNavigationAction action,
                _Out_ wf::Point* pPoint);

            // Called when the ListView receives focus.
            IFACEMETHOD(OnGotFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs)
                override;

            // Called when the ListView loses focus.
            IFACEMETHOD(OnLostFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs)
                override;

            // Updates IsSelectionActive when the ListView receives or loses
            // focus and optionally refocuses the last focused child
            // ListViewBaseItem.
            _Check_return_ HRESULT OnFocusChanged(
                _In_ BOOLEAN hasFocus);

            // Focus a ListViewBaseItem.
            _Check_return_ HRESULT FocusItem(
                _In_ xaml::FocusState focusState,
                _In_ ListViewBaseItem* pItem);

            // Focus a ListViewBaseItem at the given index and optionally scroll it
            // into view.
            _Check_return_ HRESULT SetFocusedItem(
                _In_ INT index,
                _In_ BOOLEAN shouldScrollIntoView,
                _In_ BOOLEAN animateIfBringIntoView = FALSE,
                _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None,
                InputActivationBehavior inputActivationBehavior = InputActivationBehavior::RequestActivation) // default to request activation to match legacy behavior
                override;

            // Set the ListViewBaseItem at index to be focused using an explicit FocusState
            // The focus is only set if forceFocus is TRUE or the Selector already has focus.
            // ScrollIntoView is always called if shouldScrollIntoView is set (regardless of focus).
            _Check_return_ HRESULT SetFocusedItem(
                _In_ INT index,
                _In_ BOOLEAN shouldScrollIntoView,
                _In_ BOOLEAN forceFocus,
                _In_ xaml::FocusState focusState,
                _In_ BOOLEAN animateIfBringIntoView = FALSE,
                _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None,
                InputActivationBehavior inputActivationBehavior = InputActivationBehavior::RequestActivation) // default to request activation to match legacy behavior
                override;

            // Returns TRUE if the ListViewBase is currently engaged in an exclusive interaction
            // and should thus ignore all new interactions.
            BOOLEAN IsInExclusiveInteraction();

            // UIElement override for getting next tab stop on path from focused element to root.
            _Check_return_ HRESULT ProcessTabStopOverride(
                _In_opt_ DependencyObject* pFocusedElement,
                _In_opt_ DependencyObject* pCandidateTabStopElement,
                const bool isBackward,
                const bool didCycleFocusAtRootVisualScope,
                _Outptr_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsTabStopOverridden) override;

            // UIElement override for getting next tab stop on path from focus candidate element to root.
            _Check_return_ HRESULT ProcessCandidateTabStopOverride(
                _In_opt_ DependencyObject* pFocusedElement,
                _In_ DependencyObject* pCandidateTabStopElement,
                _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
                const bool isBackward,
                _Outptr_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsCandidateTabStopOverridden) override;

            _Check_return_ HRESULT ItemFocused(
                _In_ SelectorItem* pSelectorItem) override;

        private:
            // Handles changes to the IsSelectionActive property by updating the
            // visual states of all the currently selected ListViewBaseItems.
            _Check_return_ HRESULT OnIsSelectionActiveChanged();

            _Check_return_ HRESULT SetHoldingItem(
                _In_ ListViewBaseItem* pItem);

            _Check_return_ HRESULT ClearHoldingItem(
                _In_ ListViewBaseItem* pItem);

            _Check_return_ HRESULT ClearHoldingState();

            // Checks whether the group item returned for the given index
            // by pGenerator is focusable.
            static _Check_return_ HRESULT IsGroupItemFocusable(
                _In_ DirectUI::IGroupHeaderMapping* pMapping,
                _In_ UINT groupIndex,
                _Out_ BOOLEAN* pIsFocusable);

            // Checks whether the item container returned for the given index
            // by pGenerator is focusable.
            static _Check_return_ HRESULT IsItemFocusable(
                _In_ xaml_controls::IItemContainerMapping* pMapping,
                _In_ UINT itemIndex,
                _Out_ BOOLEAN* pIsFocusable);

        #pragma endregion

        #pragma region ListViewBase_Partial_DataVirtualization.cpp members
        // ---------------------------------------------------------------------
        // ListViewBase_Partial_DataVirtualization.cpp members
        // ---------------------------------------------------------------------
        public:
            // Create an IAsyncOperation to load an additional DataFetchSize
            // number of items.
            _Check_return_ HRESULT LoadMoreItemsAsyncImpl(
                _Outptr_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>** returnValue);

        private:
            // Internal method for starting an incremental load, used for both
            // manual and automatic incremental loads.
            _Check_return_ HRESULT LoadMoreItemsAsyncInternal(
                _In_opt_ LoadMoreItemsOperation* pChainedAsyncOperation,
                _Outptr_ wf::IAsyncInfo** ppAsyncInfo);

            // Called when the ScrollHost's scrolling offsets change.
            // Triggers a LoadMoreItems call if necessary.
            _Check_return_ HRESULT ProcessDataVirtualizationScrollOffsets();

            // overridden from ItemsControl (IGeneratorHost)
            _Check_return_ IFACEMETHOD(VirtualizationFinished)() override;

            // Stores a handle to the items source as an IItemsRangeInfo if possible
            _Check_return_ HRESULT InitializeDataSourceItemsRangeInfo();

            // Used to inform the data source of the items it is tracking
            // in this function, we collect the data
            _Check_return_ HRESULT InvokeDataSourceRangesChanged();

            // Used to inform the data source of the items it is tracking
            // in this function, we check if the new data is the same as the last passed ones
            // we invoke if the data is different
            _Check_return_ HRESULT InvokeDataSourceRangesChangedHelper(
                _In_ xaml_data::IItemIndexRange* visibleRange,
                _In_ TrackerCollection<xaml_data::ItemIndexRange*>* trackedItems);

            TrackerPtr<xaml_data::IItemsRangeInfo> m_tpDataSourceAsItemsRangeInfo;
            TrackerPtr<xaml_data::IItemIndexRange> m_tpLastPassedVisibleRange;
            TrackerPtr<TrackerCollection<xaml_data::ItemIndexRange*>> m_tpLastPassedTrackedRanges;

        #pragma endregion

        #pragma region ListViewBase_Partial_Reorder.cpp members
        // ---------------------------------------------------------------------
        // ListViewBase_Partial_Reorder.cpp members
        // ---------------------------------------------------------------------

        public:
            // Sets pResult to TRUE if
            // 1) a drag and drop operation is in progress, and
            // 2a) the given item is the item the user is physically dragging
            //    (as opposed to just part of the selection), or
            // 2b) the given item is the owning GroupItem for the item the user is physically dragging.
            // Sets pResult to FALSE otherwise.
            _Check_return_ HRESULT IsContainerDragDropOwner(
                _In_ xaml::IUIElement* pContainer,
                _Out_ BOOLEAN* pResult);

            // Returns TRUE if a drag and drop is in progress.
            BOOLEAN IsInDragDrop();

            // Gets the value of m_isHolding
            bool GetIsHolding()
            {
                return m_isHolding;
            }

            // Sets the value of m_isHolding
            void SetIsHolding(_In_ bool isHolding)
            {
                m_isHolding = isHolding;
            }

            // If a drag and drop operation is in progress, returns the number of items being dragged.
            // Otherwise, returns 0.
            UINT DragItemsCount();

            _Check_return_ HRESULT UpdateDragPositionOnCapturedTarget(
                _In_ DragDropMessageType type,
                _In_ wf::Point dragPoint);

            // Returns whether the passed item is the one being dragged over
            BOOLEAN IsDragOverItem(
                _In_ ListViewBaseItem* pItem);

            // Returns whether the ListViewBase was the source of the drag
            _Check_return_ HRESULT IsDragSourceImpl(
                _Out_ BOOLEAN* pReturnValue);

        protected:
            // Raises the DragItemsStarting event populates the DataPackage and returns whether or not
            // the drag was canceled by the handler.
            _Check_return_ HRESULT OnDragItemsStarting(
                _In_ wfc::IVector<IInspectable*>* pDraggedItems,
                _In_ wadt::IDataPackage* pData,
                _Out_ BOOLEAN* pWasCanceled);

            // If there isn't a drag and drop in progress
            // 1) fire DragItemsStart to serialize the current selection and to determine if the drag should be canceled.
            // 2) If the drag isn't canceled, kick off the drag operation.
            //  pointer -          The pointer used to start the new drag.
            //  pOriginatingItem -  The item the user performed a drag gesture on.
            //  dragStartPoint   -  the point at which the drag should start.
            //                      This should line up with the user's current Pointer location.
            // onDragStarting    -  Called when we've decided to actually start a drag, meaning that DragItemsStarting
            //                      was fired and did not request a cancel (or drag/drop is disabled but reordering is enabled).
            //                      NOTE! This doesn't mean *pWasCanceled is guaranteed to become false! We can still fail in other ways.
            // pWasCanceled      -  Set if the drag was canceled or failed in some way.
            _Check_return_ HRESULT OnDragGesture(
                _In_ xaml_input::IPointer* pointer,
                _In_ ixp::IPointerPoint* pointerPoint,
                _In_ ListViewBaseItem* pOriginatingItem,
                _In_ DragDropVisual* pDragVisual,
                _In_ wf::Point dragStartPoint,
                _In_ std::function<HRESULT ()> onDragStarting,
                _Out_ BOOLEAN* pWasCanceled);

            // Called when the user moves the drag location.
            // If the current drag modality isn't the same as the provided modality,
            // do nothing.
            //  pointer    - The input pointer used to indicate the drop.
            //  dragPoint   - The point the user moved to. Relative to this ListViewBase.
            //  pIsHandled  - Set to FALSE if the gesture was ignored due to input modality mismatch, set to TRUE otherwise.
            _Check_return_ HRESULT OnDragMoveGesture(
                _In_ xaml_input::IPointer* pointer,
                _In_ wf::Point dragPoint,
                _Out_ BOOLEAN *pIsHandled);

            // Called when the user indicates the dragged item should be dropped.
            // If the current drag modality isn't the same as the provided modality,
            // do nothing.
            //  pointer    - The input pointer used to indicate the drop.
            //  pIsHandled  - Set to FALSE if the gesture was ignored due to input modality mismatch, set to TRUE otherwise.
            _Check_return_ HRESULT OnDropGesture(
                _In_opt_ xaml_input::IPointer* pointer,
                _In_ const wf::Point& dropPoint,
                _Out_ BOOLEAN *pIsHandled);

            // Cancels the current drag/drop operation.
            _Check_return_ HRESULT CancelDrag();

            // Moves the currently dragged items to the given point.
            // Depends on ItemsHost being an IItemLookupPanel.
            //  dropPoint   - The point where reordered items should be inserted, relative to this ListViewBase.
            _Check_return_ HRESULT OnReorderDrop(
                _In_ wf::Point dropPoint);

            // Called when the user drags into this ListViewBase.
            IFACEMETHOD(OnDragEnter)(
                _In_ xaml::IDragEventArgs* pArgs)
                override;

            // Called when the user drags out of this ListViewBase.
            IFACEMETHOD(OnDragLeave)(
                _In_ xaml::IDragEventArgs* pArgs)
                override;

            // Called when the user drags over this ListViewBase.
            IFACEMETHOD(OnDragOver)(
                _In_ xaml::IDragEventArgs* pArgs)
                override;

            // Called when the user drops onto this ListViewBase.
            IFACEMETHOD(OnDrop)(
                _In_ xaml::IDragEventArgs* pArgs)
                override;

        private:
            // Event source type for DragItemsStartingEvent.
            typedef
                CEventSource<
                    xaml_controls::IDragItemsStartingEventHandler,
                    IInspectable,
                    xaml_controls::IDragItemsStartingEventArgs>
                DragItemsStartingEventSource;

            // Event source type for DragItemsCompleted
            typedef
                CEventSource<
                    wf::ITypedEventHandler<xaml_controls::ListViewBase*, xaml_controls::DragItemsCompletedEventArgs*>,
                    xaml_controls::IListViewBase,
                    xaml_controls::IDragItemsCompletedEventArgs>
                DragItemsCompletedEventSource;

            // Processes a drag over event at the given position (relative to the panel).
            _Check_return_ HRESULT ProcessDragOverAt(
                _In_ wf::Point dragPoint,
                _Out_opt_ BOOLEAN* pHandled);

            // Builds one or both of:
            // - a vector containing the data items that should be dragged. This is the vector that will be passed as the Items property in the DragItemsStartingEvent args.
            // - a list containing the current indexes of said items. Not that this isn't ref-counted - it's a std::list.
            _Check_return_ HRESULT GetDraggedItems(
                _In_ ListViewBaseItem* pOriginatingItem,
                _Outptr_opt_ wfc::IVector<IInspectable*>** ppItems,
                _Out_opt_ std::vector<UINT>* pItemIndices);

            // Reorder the given list of items (given as a list of indexes) to the given index.
            // Items are inserted in their original order.
            // Optionally, pTrackedIndex can be specified. It should hold the index of an item you're
            // interested in tracking. When the function returns successfully, pTrackedIndex will be updated
            // with the new index location of the item originally at *pTrackedIndex.
            _Check_return_ HRESULT ReorderItemsTo(
                _In_ std::vector<UINT>* pReorderItemIndexes,
                _In_ INT insertIndex,
                _Inout_opt_ INT* pTrackedIndex1,
                _Inout_opt_ INT* pTrackedIndex2);

            // Returns TRUE if reordering is enabled. This means reordering is active,
            // AllowDrop is TRUE and we're dragging over ourselves.
            _Check_return_ HRESULT DropCausesReorder(
                _Out_ BOOLEAN* pResult);

            // Returns TRUE if reordering is active, meaning:
            // * (CanReorderItems is TRUE OR Phone_ListViewReorderEnabled)
            // * AND (IsNotGroupingAndHostIsLookupPanel is TRUE)
            _Check_return_ HRESULT IsItemReorderingActive(
                _Out_ BOOLEAN* pResult);

            // Returns TRUE if:
            // * IsGrouping is FALSE
            // * ItemsHost is an IItemLookupPanel
            _Check_return_ HRESULT IsNotGroupingAndHostIsLookupPanel(
                _Out_ BOOLEAN* pResult);

            // Complete a drop gesture.
            _Check_return_ HRESULT CompleteDrop();

            _Check_return_ HRESULT ClearPrimaryDragContainer();

            // Only one Pointer can drive drag and drop operations at a time.
            // This method returns TRUE if there is no current drag and drop operation,
            // or the given IPointer is the pointer currently in charge of a drag
            // and drop operation.
            _Check_return_ HRESULT ShouldAcceptDragInput(
                _In_ xaml_input::IPointer* pointer,
                _Out_ BOOLEAN* shouldAccept);

            // Returns TRUE if the given IPointers refer to the same physical device.
            static _Check_return_ HRESULT AreSamePointer(
                _In_ xaml_input::IPointer* pPointerA,
                _In_ xaml_input::IPointer* pPointerB,
                _Out_ BOOLEAN* pAreSame);

            // Creates a blank IDataPackage for use in drag and drop.
            _Check_return_ HRESULT GetBlankDataPackage(
                _Outptr_ wadt::IDataPackage** ppData);

            // Returns TRUE if the current interaction mode requires drag and drop.
            // This is the case if CanDragItems == TRUE, or CanReorderItems == TRUE
            // unless we're grouping.
            _Check_return_ HRESULT GetIsDragEnabled(
                _Out_ BOOLEAN *pDragEnabled);

            // Returns the edge scrolling velocity we should be performing.
            // dragPoint - The point over which the user is dragging, relative to the ListViewBase.
            _Check_return_ HRESULT ComputeEdgeScrollVelocity(
                _In_ wf::Point dragPoint,
                _Out_ PanVelocity* pVelocity);

            // Computes the speed of an edge scroll, given a distance from the edge.
            XFLOAT ComputeEdgeScrollVelocityFromEdgeDistance(
                _In_ DOUBLE distanceFromEdge);

            // Implements the delay-start, but instant-update behavior for edge scrolling.
            // If the given velocity is zero, immediately stop edge scrolling.
            // If there isn't a currently running edge scroll, start a timer. When that timer
            // completes, call ScrollWithVelocity with the given arguments.
            // If there is a currently running edge scroll, change the velocity immediately.
            _Check_return_ HRESULT SetPendingAutoPanVelocity(
                _In_ PanVelocity velocity);

            // Scroll our ScrollViewer with the given velocity.
            _Check_return_ HRESULT ScrollWithVelocity(
                _In_ PanVelocity velocity);

            // Instantiate the edge scroll timer, if necessary, and start it.
            _Check_return_ HRESULT EnsureStartEdgeScrollTimer();

            // Stop and releas the edge scroll timer.
            _Check_return_ HRESULT DestroyStartEdgeScrollTimer();

            // The edge scroll timer calls this function when it's time
            // to do our pending edge scroll action.
            _Check_return_ HRESULT StartEdgeScrollTimerTick(
                _In_opt_ IInspectable* pUnused1,
                _In_opt_ IInspectable* pUnused2);

            // React to a drag visual being created. The given visual is set to
            // m_tpDragDropVisual. Will result in error if there is already a drag
            // in progress.
            _Check_return_ HRESULT NotifyDragVisualCreated(
                _In_ DirectUI::DragDropVisual* pDragVisual);

            // Release m_tpDragDropVisual.
            _Check_return_ HRESULT NotifyDragVisualReleased();

            // Perform a keyboard reorder in the specified direction,
            // originating from the given focused item.
            _Check_return_ HRESULT OnKeyboardReorder(
                _In_ KeyboardReorderDirection direction,
                _In_ ListViewBaseItem* pFocusedItem);

            // Registers or unregisters back key listener on phone based on whether LV.ReorderMode is enabled or not
            _Check_return_ HRESULT ToggleBackKeyListenerOnPhone(
                _In_ BOOLEAN bIsZoomedOutViewActive);

            // Loaded event handler.
            _Check_return_ HRESULT OnLoaded(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Unloaded event handler.
            _Check_return_ HRESULT OnUnloaded(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            _Check_return_ HRESULT OnDragCompleted(
                _In_ wf::IAsyncOperation<wadt::DataPackageOperation>* pAsyncOp,
                _In_ wf::AsyncStatus status,
                _In_ wfc::IVector<IInspectable*>* pItems);

            // Returns TRUE if live reordering is enabled
            // This means CanReorderItems is true and AllowDrop is true
            _Check_return_ HRESULT ShouldPlayLiveReorder(
                _Out_ BOOLEAN* pResult);

            _Check_return_ HRESULT StartLiveReorderTimer();

            _Check_return_ HRESULT EnsureLiveReorderTimer();

            _Check_return_ HRESULT StopLiveReorderTimer();

            // Handler for the Tick event on m_tpLiveReorderTimer
            _Check_return_ HRESULT LiveReorderTimerTickHandler();

            _Check_return_ HRESULT GetNewMovedItemsForLiveReorder(
                _Inout_ std::vector<DirectUI::Components::LiveReorderHelper::MovedItem>& newItems);

            _Check_return_ HRESULT AddNewItemForLiveReorder(
                _In_ const int sourceIndex,
                _In_ const int targetIndex,
                _In_ DirectUI::LayoutInformation* pLayoutInformation,
                _Inout_ std::vector<DirectUI::Components::LiveReorderHelper::MovedItem>& newItems);

            _Check_return_ HRESULT ResetAllItemsForLiveReorder();

            _Check_return_ HRESULT MoveItemsForLiveReorder(
                _In_ bool areNewItems,
                _Inout_ const std::vector<DirectUI::Components::LiveReorderHelper::MovedItem>& newItemsToMove);

            _Check_return_ HRESULT GetLayoutSlot(
                _In_ int itemIndex,
                _In_ DirectUI::LayoutInformation* pLayoutInformation,
                _Out_ wf::Rect* pLayoutSlot);

            _Check_return_ HRESULT GetItemsCount(
                _Out_ int* pItemsCount);

            bool IsInLiveReorder() const
            {
                return m_liveReorderIndices.draggedItemIndex != -1;
            }

            int GetInsertionIndexForLiveReorder() const
            {
                const int draggedIndex = m_liveReorderIndices.draggedItemIndex;
                int insertIndex = m_liveReorderIndices.draggedOverIndex;

                if (draggedIndex < insertIndex)
                {
                    ++insertIndex;
                }

                // make sure we don't go out of range
                if (insertIndex > m_liveReorderIndices.itemsCount)
                {
                    insertIndex = m_liveReorderIndices.itemsCount;
                }

                return insertIndex;
            }

            _Check_return_ HRESULT GetPanelOrientation(
                _Out_ xaml_controls::Orientation* panelOrientation);

            _Check_return_ HRESULT SetDragOverItem(
                _In_ ListViewBaseItem* dragOverItem);

            _Check_return_ IFACEMETHOD(OverrideContainerArrangeBounds)(
                _In_ INT index,
                _In_ wf::Rect suggestedBounds,
                _Out_ wf::Rect* newBounds) override;
        #pragma endregion

        #pragma region ListViewBase_Partial_SemanticZoomInformation.cpp members
        // ---------------------------------------------------------------------
        // ListViewBase_Partial_SemanticZoomInformation.cpp members
        // ---------------------------------------------------------------------
        public:
            // Prepare the view for a zoom transition.
            _Check_return_ HRESULT InitializeViewChangeImpl();

            // Cleanup the view after a zoom transition.
            _Check_return_ HRESULT CompleteViewChangeImpl();

            // Forces content to scroll until the coordinate space of the
            // SemanticZoomItem is visible.
            _Check_return_ HRESULT MakeVisibleImpl(
                _In_ xaml_controls::ISemanticZoomLocation* pItem) noexcept;

            // When this ListViewBase is the active view and we're changing to
            // the other view, optionally provide the source and destination
            // items.
            _Check_return_ HRESULT StartViewChangeFromImpl(
                _In_ xaml_controls::ISemanticZoomLocation* pSource,
                _In_ xaml_controls::ISemanticZoomLocation* pDestination) noexcept;

            // When this ListViewBase is the inactive view and we're changing to
            // it, optionally provide the source and destination items.
            _Check_return_ HRESULT StartViewChangeToImpl(
                _In_ xaml_controls::ISemanticZoomLocation* pSource,
                _In_ xaml_controls::ISemanticZoomLocation* pDestination);

            // Complete the change to the other view when this ListViewBase was
            // the active view.
            _Check_return_ HRESULT CompleteViewChangeFromImpl(
                _In_ xaml_controls::ISemanticZoomLocation* pSource,
                _In_ xaml_controls::ISemanticZoomLocation* pDestination);

            // Complete the change to make this ListViewBase the active view.
            _Check_return_ HRESULT CompleteViewChangeToImpl(
                _In_ xaml_controls::ISemanticZoomLocation* pSource,
                _In_ xaml_controls::ISemanticZoomLocation* pDestination);

        private:
            // Determines whether or not a SemanticZoom associated with this
            // ListViewBase has grouped data as its ZoomedInView.
            _Check_return_ HRESULT get_CanSemanticZoomGroup(
                _Out_ BOOLEAN* pCanGroup);

            // Find the first item
            _Check_return_ HRESULT FindFirstItem(
                _In_ UINT groupIndex,
                _Out_ INT* pItemIndex,
                _Outptr_result_maybenull_ IInspectable** ppItem);

            _Check_return_ HRESULT OnLoadAsyncCompleted(
                wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>* getOperation, wf::AsyncStatus status);

        #pragma endregion

        #pragma region ListViewBase_Partial_UIAProperties.cpp members
            // ---------------------------------------------------------------------
            // ListViewBase_Partial_UIAProperties.cpp members
            // ---------------------------------------------------------------------
        public:
            // Return a string that represents the action that would happen if you were to drop the currently dragged item.
            _Check_return_ HRESULT GetDropTargetDropEffect(_Out_ HSTRING* phsValue);
            _Check_return_ HRESULT GetDropTargetDropEffects(_Out_ UINT* returnValueCount, _Out_ HSTRING** phsArray);
            _Check_return_ HRESULT GetDragGrabbedItems(_Out_ UINT* cRepsArray,
                _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** pRepsArray);
        private:
            // Build a string that represents the action that would happen if you were to drop the currently dragged item.
            _Check_return_ HRESULT UpdateDropTargetDropEffect(_In_ BOOLEAN forceUpdate, _In_ BOOLEAN isLeaving);
            _Check_return_ HRESULT UpdateDropTargetDropEffect(_In_ BOOLEAN forceUpdate, _In_ BOOLEAN isLeaving, _In_opt_ UIElement* keyboardReorderedContainer);
            _Check_return_ HRESULT UpdateDragGrabbedItems(_In_ BOOLEAN forceUpdate);
            _Check_return_ HRESULT CreateCopyOfDragGrabbedItems(
                _Out_ UINT* cDragGrabbedItems,
                _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** pppDragGrabbedItems);

            _Check_return_ HRESULT GetAutomationString(
                _In_ xaml::IDependencyObject* item,
                _Outptr_result_maybenull_ WCHAR** automationString);

        #pragma endregion

        #pragma region ListViewBase_Partial_ContainerPhase.cpp members
        public:
            IFACEMETHOD(get_ShowsScrollingPlaceholders)(_Out_ BOOLEAN* pValue) override;

            _Check_return_ HRESULT IsBuildTreeSuspendedImpl(_Out_ BOOLEAN* pReturnValue);

            // perform async work. In the case of a listview that is:
            // 1. raise the ContainerContentChanging event
            // 2. process ClearContainer calls that we had deferred
            _Check_return_ HRESULT BuildTreeImpl(_Out_ BOOLEAN* workLeft) noexcept;

            // shuts down all the work we still have to do
            _Check_return_ HRESULT ShutDownDeferredWorkImpl();

            // sets the duration since last render that we want to execute deferred work for
            _Check_return_ HRESULT SetDesiredContainerUpdateDurationImpl(_In_ wf::TimeSpan duration);

            _Check_return_ HRESULT GetRelativeScrollPositionImpl(
                _In_ xaml_controls::IListViewItemToKeyHandler* itemToKeyHandler,
                _Out_ HSTRING* returnValue);

            _Check_return_ HRESULT SetRelativeScrollPositionAsyncImpl(
                _In_ HSTRING relativeScrollPosition,
                _In_ xaml_controls::IListViewKeyToItemHandler* keyToItemHandler,
                _Outptr_ wf::IAsyncAction** returnValue);

        protected:
            // returns whether we are deferring the prepare for a container
            _Check_return_ HRESULT IsPrepareContainerForItemDeferred(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pDefer) override;

        private:

            // the lowest phase container we have in the queue. -1 means nothing is in the queue
            INT64 m_lowestPhaseInQueue;

            // budget that we have to do other work, measured since the last tick
            // once we get stl 11, we should convert to std::chrono
            UINT m_budget;

            // the last known count of containers in the incremental visualization queue. this value is used when logging
            // our telemetry event in the case where BuildTreeImpl cannot perform work on the busy uiThread.
            UINT m_lastIncrementalVisualizationContainerCount;

            // Locally cached value, so we can avoid more expensive property system lookups.
            BOOLEAN m_showsScrollingPlaceholdersLocal;

            // In some situations (typically when closing JumpList), we do not want scrolling PlaceHolders
            BOOLEAN m_disableScrollingPlaceholders;

        #pragma endregion

        #pragma region ListViewBase_Partial_Serialization.cpp members

        private:

            enum ListSerializationFormatVersions
            {
                ListSerializationFormatVersions_PhoneBlue = 1
            };

            struct ListSerializationData
            {
                ListSerializationFormatVersions version;
                ElementType elementType;
                DOUBLE viewportOffset;
                wrl_wrappers::HString serializationKey;
            };

            _Check_return_ HRESULT CreateSerializationBuffer(
                _In_ const ListSerializationData& serializationData,
                _Outptr_ wsts::IBuffer** returnValue) const;

            _Check_return_ HRESULT ParseSerializationBuffer(
                _In_ wsts::IBuffer* pBuffer,
                _Out_ ListSerializationData* pSerializationData,
                _Out_ BOOLEAN* pSucceeded) const;

            _Check_return_ HRESULT EncodeBuffer(
                _In_ wsts::IBuffer* pBuffer,
                _Out_ HSTRING* pBase64String);

            _Check_return_ HRESULT DecodeBuffer(
                _In_ HSTRING base64String,
                _Outptr_ wsts::IBuffer** ppBuffer);

            std::function<HRESULT(IInspectable*)> GetDeserializationScrollCommand(
                _In_ BOOLEAN isHeader,
                _In_ BOOLEAN isFooter,
                _In_ DOUBLE offset);

        #pragma endregion
    };
}
