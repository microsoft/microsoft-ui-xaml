// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a Selector abstract class.
//      Management of the selected items list is done through a Selection instance. If we need to modify the selected items list,
//      we request a SelectionChanger from the Selection (via BeginChange) and perform the needed operations. This is done to prevent
//      reentrancy (and infinite loops) arising from user event handlers changing the selection during selection processing.
//      The Selector class itself is primarily responsible for managing the UI side of things such as notifying SelectorItems
//      when their selection state changes. To this end, Selector provides a callback (in the form of SelectionChangeApplier)
//      to the Selection instance.

#pragma once

#include "Selection.h"
#include "Selector.g.h"

namespace DirectUI
{
    // forward declaration
    class SelectionChanger;
    class Selection;
    class SelectorItem;
    class PropertyPathListener;
    class ListViewBaseItem;

    // Represents a Selector abstract class.
    //
    PARTIAL_CLASS(Selector),
        private SelectionChangeApplier
    {
        // Give SelectorItem friend access so it can check whether it's a data
        // virtualized item.
        friend class SelectorItem;

        // Give ListViewBaseItem friend access so it can call ItemFocused.
        friend class ListViewBaseItem;

    public:
        // Invoked whenever application code or internal processes call
        // ApplyTemplate.
        IFACEMETHOD(OnApplyTemplate)() override;

        // Sets the ListBoxItem for item to be selected
        _Check_return_ HRESULT AutomationPeerAddToSelection(
            _In_ UINT index,
            _In_ IInspectable* pItem);

        // Sets the ListBoxItem for item to be selected
        _Check_return_ HRESULT AutomationPeerRemoveFromSelection(
            _In_ UINT index,
            _In_ IInspectable* pItem);

        // Checks to see if an item is selected
        _Check_return_ HRESULT AutomationPeerIsSelected(
            _In_ IInspectable* item,
            _Out_ BOOLEAN* isSelected);

        // Gets first element that should take focus after Tab.
        _Check_return_ HRESULT GetFirstFocusableElementOverride(
            _Outptr_result_maybenull_ DependencyObject** ppFirstFocusable) override;

        // Gets last element that should take focus after Tab.
        _Check_return_ HRESULT GetLastFocusableElementOverride(
            _Outptr_result_maybenull_ DependencyObject** ppLastFocusable) override;

        static BOOLEAN IsNavigationKey(
            _In_ wsy::VirtualKey key);

        _Check_return_ HRESULT DataSourceGetIsSelected(
            _In_ SelectorItem* pSelectorItem,
            _Out_ BOOLEAN* pIsSelected,
            _Out_ bool* pIsValueSet);

        // Customized methods.
        _Check_return_ HRESULT BeginInitImpl();
        _Check_return_ HRESULT EndInitImpl(_In_opt_ XamlServiceProviderContext*);

        // Get the ItemsHost's orientations. This is a stopgap while
        // StackPanel doesn't implement IOrientedPanel. See IOrientedPanel
        // for more information on what these two orientations mean.
        // If the panel is neither a StackPanel nor a IOrientedPanel,
        // both orientations are returned as Vertical.
        _Check_return_ HRESULT GetItemsHostOrientations(
            _Out_opt_ xaml_controls::Orientation* pPhysicalOrientation,
            _Out_opt_ xaml_controls::Orientation* pLogicalOrientation);

        _Check_return_ HRESULT IsIndexSelected(_In_ int index, _Out_ bool* result);

        void SetAllowCustomValues(bool allow);

    protected:
        // constructor/destructor
        Selector();
        ~Selector() override;

        // Prepares object's state
        _Check_return_ HRESULT Initialize() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Handles when a key is pressed down on the Selector.
        // We use this to forward keyboard zoom hotkeys to our ScrollViewer.
        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
            override;

        // List of all items that are currently selected
        _Check_return_ HRESULT get_SelectedItemsInternal(
            _Outptr_ wfc::IVector<IInspectable*>** pValue);

        // List of all ranges that are currently selected
        _Check_return_ HRESULT get_SelectedRangesInternal(
            _Outptr_ wfc::IVectorView<xaml_data::ItemIndexRange*>** pValue);

        // Whether or not this Selector allows multiple selection
        virtual _Check_return_ HRESULT get_CanSelectMultiple(_Out_ BOOLEAN* pValue);

        virtual _Check_return_ HRESULT OnSelectionChanged(
            _In_ xaml_controls::ISelectionChangedEventArgs* pSelectionChangedEventArgs);

        virtual _Check_return_ HRESULT OnSelectionChanged(
            _In_ INT oldSelectedIndex,
            _In_ INT newSelectedIndex,
            _In_ IInspectable* pOldSelectedItem,
            _In_ IInspectable* pNewSelectedItem,
            _In_ BOOLEAN animateIfBringIntoView = FALSE,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None);

        _Check_return_ HRESULT NotifyOfSourceChanged(
            _In_ wfc::IObservableVector<IInspectable*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e) noexcept override;

        _Check_return_ HRESULT OnItemsChanged(
            _In_ wfc::IVectorChangedEventArgs* e) override;

        IFACEMETHOD(PrepareContainerForItemOverride)(
            _In_ xaml::IDependencyObject* element,
            _In_ IInspectable* item) override;

        IFACEMETHOD(ClearContainerForItemOverride)(
            _In_ xaml::IDependencyObject* element,
            _In_ IInspectable* item) override;

        virtual _Check_return_ HRESULT OnSelectorItemClicked(
            _In_ SelectorItem* pSelectorItem,
            _Out_ BOOLEAN* pFocused);

        _Check_return_ HRESULT OnFocusEngaged(
            _In_ xaml::Controls::IControl* pSender,
            _In_ xaml::Controls::IFocusEngagedEventArgs* pArgs);

        // Called to detect whether we can scroll to the View or not.
        virtual _Check_return_ HRESULT CanScrollIntoView(_Out_ BOOLEAN& canScroll);

        // Helper function to select one item in Selector
        _Check_return_ HRESULT SelectOneItemInternal(
            _In_ int index,
            _In_ IInspectable* item,
            _In_ BOOLEAN clearOldSelection);

        // Helper function to select all items in Selector
        _Check_return_ HRESULT SelectAllInternal();

        // Helper function to select a range of items in Selector
        _Check_return_ HRESULT SelectRangeInternal(
            _In_ int firstIndex,
            _In_ unsigned int length,
            _In_ BOOLEAN clearOldSelection);

        // Helper function to deselect a range of items in Selector
        _Check_return_ HRESULT DeselectRangeInternal(
            _In_ int firstIndex,
            _In_ unsigned int length);

        // Helper function to clear all selection in Selector
        _Check_return_ HRESULT ClearSelection();

        // Requests a SelectionChanger from our Selection instance.
        // This is preferred over requesting the Selection directly.
        _Check_return_ HRESULT BeginChange(
            _Outptr_ SelectionChanger** pChanger);

        // Applies the changes in the given SelectionChanger to this
        // Selector, ensuring the state of public properties.
        _Check_return_ HRESULT EndChange(
            _In_ SelectionChanger* pChanger,
            _In_ BOOLEAN animateIfBringIntoView = FALSE,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None);

        // Ask the CollectionView if it is appropriate to select a single item,
        // then select just that item (in the context of the given SelectionChanger).
        _Check_return_ HRESULT SelectJustThisItem(
            _In_ const INT oldIndex,
            _In_ const INT newIndex,
            _In_ SelectionChanger* pChanger,
            _In_opt_ IInspectable* pSelectedItem,
            _Out_opt_ BOOLEAN* pShouldUndoChange);

        _Check_return_ HRESULT GetDataSourceAsSelectionInfo(
            _Outptr_ xaml_data::ISelectionInfo** ppDataSourceAsSelectionInfo);

        void SetDataSourceAsSelectionInfo(
            _In_ xaml_data::ISelectionInfo* const pDataSourceAsSelectionInfo);

        // Set the SelectorItem at index to be focused.
        // If HasFocus() returns true, we propagate previous focused item's FocusState to the new focused item.
        virtual _Check_return_ HRESULT SetFocusedItem(
            _In_ INT index,
            _In_ BOOLEAN shouldScrollIntoView,
            _In_ BOOLEAN animateIfBringIntoView = FALSE,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None,
            InputActivationBehavior inputActivationBehavior = InputActivationBehavior::RequestActivation); // default to request activation to match legacy behavior

        // Set the SelectorItem at index to be focused using an explicit FocusState
        // The focus is only set if forceFocus is TRUE or the Selector already has focus.
        // ScrollIntoView is always called if shouldScrollIntoView is set (regardless of focus).
        virtual _Check_return_ HRESULT SetFocusedItem(
            _In_ INT index,
            _In_ BOOLEAN shouldScrollIntoView,
            _In_ BOOLEAN forceFocus,
            _In_ xaml::FocusState focusState,
            _In_ BOOLEAN animateIfBringIntoView = FALSE,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None,
            InputActivationBehavior inputActivationBehavior = InputActivationBehavior::RequestActivation); // default to request activation to match legacy behavior

        // Handles selection of single item, and only that item
        virtual _Check_return_ HRESULT MakeSingleSelection(
                _In_ UINT index,
                _In_ BOOLEAN animateIfBringIntoView,
                _In_opt_ IInspectable* pSelectedItem,
                _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None);

        virtual _Check_return_ HRESULT IsSelectionPatternApplicable(
            _Out_ bool* selectionPatternApplicable);

        _Check_return_ HRESULT ScrollIntoViewInternal(
            _In_ IInspectable* item,
            _In_ BOOLEAN isHeader,
            _In_ BOOLEAN isFooter,
            _In_ BOOLEAN isFromPublicAPI,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment,
            _In_ DOUBLE offset = 0.0,
            _In_ BOOLEAN animateIfBringIntoView = FALSE);

        _Check_return_ HRESULT ScrollIntoView(
            _In_ UINT index,
            _In_ BOOLEAN isGroupItemIndex,
            _In_ BOOLEAN isHeader,
            _In_ BOOLEAN isFooter,
            _In_ BOOLEAN isFromPublicAPI,
            _In_ BOOLEAN ensureContainerRealized,
            _In_ BOOLEAN animateIfBringIntoView,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment,
            _In_ DOUBLE offset = 0.0,
            _In_ UINT currentGroupIndex = 0) override;

        _Check_return_ HRESULT ScrollRectToViewport(
            _In_ wf::Rect containerRect,
            _In_ wf::Rect scrollHostRect,
            _In_ BOOLEAN animateIfBringIntoView,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment,
            _In_ DOUBLE offset,
            _In_ xaml_controls::IItemsPresenter* pItemsPresenter);

        // Call m_pScrollViewer->ScrollInDirection if possible.
        _Check_return_ HRESULT ElementScrollViewerScrollInDirection(
            _In_ wsy::VirtualKey key,
            _In_ BOOLEAN animate = FALSE);

        _Check_return_ HRESULT HandleNavigationKey(
            _In_ wsy::VirtualKey key,
            _In_ BOOLEAN scrollViewport,
            _Inout_ INT& newFocusedIndex);

        _Check_return_ HRESULT HasItems(_Out_ BOOLEAN& bHasItems);

        // Get the geven panel's orientations. This is a stopgap while
        // StackPanel doesn't implement IOrientedPanel. See IOrientedPanel
        // for more information on what these two orientations mean.
        // If the panel is neither a StackPanel nor a IOrientedPanel,
        // both orientations are returned as Vertical.
        _Check_return_ HRESULT GetPanelOrientations(
            _In_ xaml_controls::IPanel* pPanel,
            _Out_opt_ xaml_controls::Orientation* pPhysicalOrientation,
            _Out_opt_ xaml_controls::Orientation* pLogicalOrientation);

        // Selects the next item in the list.
        _Check_return_ HRESULT SelectNext(_Inout_ INT& index);

        // Selects the previous item in the list.
        _Check_return_ HRESULT SelectPrev(_Inout_ INT& index);

        // Check whether the item at a given index is a data virtualized
        // placeholder, and update its visuals if that's the case.
        _Check_return_ HRESULT ShowPlaceholderIfVirtualized(
            _In_ UINT index);

        // Check whether the given ListView is a data virtualized placeholder,
        // and update its visuals if that's the case.
        _Check_return_ HRESULT ShowPlaceholderIfVirtualized(
            _In_ SelectorItem* pItem);

        // Prevent reentrancy even if m_selection.IsChangeActive() is false.
        void PreventSelectionReentrancy()
        {
            m_isSelectionReentrancyLocked = true;
        }

        // Undo the effects of PreventSelectionReentrancy to allow reentrancy
        // when m_selection.IsChangeActive() is false.
        void AllowSelectionReentrancy()
        {
            m_isSelectionReentrancyLocked = false;
        }

        // Updating selection causes reentrancy due to our event handler
        // layout. We prevent reentrancy into selection-related functions
        // if there is an active SelectionChanger or someone explicitly prevents
        // reentrancy via PreventSelectionReentrancy.
        BOOLEAN IsSelectionReentrancyAllowed()
        {
            return !m_isSelectionReentrancyLocked && !m_selection.IsChangeActive();
        }

        // Called to update the synchronization state due to changes on properties
        _Check_return_ HRESULT UpdateCVSynchronizationState();

        virtual _Check_return_ HRESULT IsInSingleSelectionMode(bool *pfInSingleSelectionMode)
        {
            *pfInSingleSelectionMode = FALSE;
            return S_OK;
        }

        _Check_return_ HRESULT OnItemsSourceChanged(
            _In_ IInspectable* pNewValue) override;

        // goes through the visible and cached items and updates their IsSelected property and their visual state
        _Check_return_ HRESULT UpdateVisibleAndCachedItemsSelectionAndVisualState(
            _In_ bool updateIsSelected);

    // SelectorItem's methods
    private:
        friend class SelectorItem;
        friend class ListViewItem;
        friend class ListBoxItem;
        friend class FlipViewItem;
        friend class SelectorAutomationPeer;
        friend class SelectorItemAutomationPeer;
        friend class ListBoxItemDataAutomationPeer;
        friend class ComboBoxItemDataAutomationPeer;
        friend class ListViewBaseItemDataAutomationPeer;
        friend class FlipViewItemDataAutomationPeer;
        friend class ComboBoxItem;
        friend class HeaderContentControlAutomationPeer;

        _Check_return_ HRESULT OnItemsHostChanged();

        _Check_return_ HRESULT SelectAllSelectedSelectorItems(
            _In_ SelectionChanger* pSelectionChanger);

        _Check_return_ HRESULT FindIndexOfItemWithValue(
            _In_ IInspectable* pValue,
            _Out_ INT &index,
            _Outptr_ IInspectable** pItemWithValue);

        _Check_return_ HRESULT CoerceSelectedValueToNull();

        /// Returns the selected value of an item using a path.
        _Check_return_ HRESULT GetSelectedValue(
            _In_ IInspectable* pItem,
            _Outptr_result_maybenull_ IInspectable** pSelectedValue);

        _Check_return_ HRESULT OnSelectedValueChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnSelectedValuePathChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnSelectedIndexChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnSelectedItemChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnIsSynchronizedWithCurrentItemChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        // Event handler for when SelectedItems changed
        _Check_return_ HRESULT OnSelectedItemsCollectionChanged(
            _In_ wfc::IObservableVector<IInspectable*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e);

        _Check_return_ HRESULT InvokeSelectionChanged(
            _In_opt_ wfc::IVector<IInspectable*>* pUnselectedItems,
            _In_opt_ wfc::IVector<IInspectable*>* pSelectedItems);

        // Raise SelectionChanged event
        _Check_return_ HRESULT RaiseSelectionChanged(
            _In_ xaml_controls::ISelectionChangedEventArgs* pSelectionChangedEventArgs);

        // Handles changing selection properties when a SelectorItem has IsSelected change
        _Check_return_ HRESULT NotifyListItemSelected(
            _In_ SelectorItem* pSelectorItem,
            _In_ BOOLEAN bIsSelected);

        // Sets the SelectorItem for item to be selected
        _Check_return_ HRESULT SetItemIsSelected(
            _In_ UINT index,
            _In_ BOOLEAN isSelected);

        // Updates m_pSelectedItemsImpl
        _Check_return_ HRESULT UpdateSelectedItems();

        // Updates m_pSelectedRangesImpl
        _Check_return_ HRESULT UpdateSelectedRanges();

        // Updates all properties that are publicly visible after a selection change
        _Check_return_ HRESULT UpdatePublicSelectionProperties(
            _In_ INT oldSelectedIndex,
            _In_ INT newSelectedIndex,
            _In_ IInspectable* pOldSelectedItem,
            _In_ IInspectable* pNewSelectedItem,
            _In_ BOOLEAN animateIfBringIntoView = FALSE,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None);

        // Given a direction, searches through list for next available item to select.
        _Check_return_ HRESULT SelectItemHelper(
            _Inout_ INT& startIndex,
            _In_ INT increment);

        // Indicate whether the specified item is currently visible.
        // Allow partial visibility is a flag that indicates whether an item that's partially visible should be
        // considered as on the page.
        _Check_return_ HRESULT IsOnCurrentPage(
            _In_ INT index,
            _In_ BOOLEAN isGroupItemIndex,
            _In_ BOOLEAN allowPartialVisibility,
            _Out_opt_ wf::Rect* pScrollHostRect,
            _Out_opt_ wf::Rect* pContainerRect,
            _Out_ BOOLEAN& isOnCurrentPage);

        // Indicate whether the specified uielement is currently visible.
        // Allow partial visibility is a flag that indicates whether an item that's partially visible should be
        // considered as on the page.
        _Check_return_ HRESULT IsOnCurrentPage(
            _In_ FrameworkElement* pContainer,
            _In_ BOOLEAN allowPartialVisibility,
            _Out_opt_ wf::Rect* pScrollHostRect,
            _Out_opt_ wf::Rect* pContainerRect,
            _Out_ BOOLEAN& isOnCurrentPage);

        // Get the first visible item.
        _Check_return_ HRESULT GetFirstItemOnCurrentPage(
            _Inout_ INT& startingIndex,
            _In_ BOOLEAN forward);

        // Move the focus forward/backward one page.
        _Check_return_ HRESULT NavigateByPage(
            _In_ BOOLEAN forward,
            _Out_ INT& newFocusedIndex);

        // CV synchronization methods
        _Check_return_ HRESULT DisconnectFromMonitoredCV();
        _Check_return_ HRESULT DisconnectFromMonitoredCVCore(_In_ IInspectable *pMonitoredCV);

        _Check_return_ HRESULT MonitorCV(_In_opt_ xaml_data::ICollectionView *pCV);

        _Check_return_ HRESULT UpdateIsSynchronized();

        _Check_return_ HRESULT OnCurrentChanged();

        _Check_return_ HRESULT UpdateCurrentItemInCollectionView(_In_opt_ IInspectable *pItem, _Out_ bool *pfDone);

        _Check_return_ HRESULT RaiseIsSelectedChangedAutomationEvent(xaml::IDependencyObject* pContainer, _In_ BOOLEAN isSelected);

        // Report the old selected index as the given index.
        _Check_return_ HRESULT ReportOldSelectedIndexAs(
            _In_ INT index);

        // Stop reporting the old selected index.
        void ClearOldIndexToReport();

        // SelectionChangeApplier implementation
        // Calls SetItemIsSelected(TRUE) on the given item.
        _Check_return_ HRESULT SelectIndex(
            _In_ UINT index) override;

        // Calls SetItemIsSelected(FALSE) on the given item.
        _Check_return_ HRESULT UnselectIndex(
            _In_ UINT index) override;

        // nullable int. keeps old selected index
        INT* m_pOldSelectedIndexToReport;
        int m_selectedIndexValueSetBeforeItemsAvailable{ -1 };

        _Check_return_ HRESULT SelectJustThisItemInternal(
            _In_ const INT oldIndex,
            _In_ const INT newIndex,
            _In_opt_ IInspectable* pSelectedItem,
            _In_ BOOLEAN animateIfBringIntoView,
            _Out_opt_ BOOLEAN* pShouldUndoChange,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None);

        // Used to inform the data source of a range selection/deselection
        // bool select indicates whether it's a select or deselect
        _Check_return_ HRESULT InvokeDataSourceRangeSelection(
            _In_ bool select,
            _In_ int firstIndex,
            _In_ unsigned int length);

        bool AreCustomValuesAllowed() const
        {
            return m_customValuesAllowed;
        }

        // helper function to invoke a DeselectAll for the data source without invoking a SelectionChangedEvent
        _Check_return_ HRESULT InvokeDataSourceClearSelection();

        // updates SelectedIndex, SelectedItem and SelectedValue after a selection using SelectionInfo occurs
        _Check_return_ HRESULT UpdatePublicSelectionPropertiesAfterDataSourceSelectionInfo();

        TrackerPtr<xaml_data::ISelectionInfo> m_tpDataSourceAsSelectionInfo;

        // Allows the insertion of custom values by not reverting values outside the item source.
        bool m_customValuesAllowed = false;

        // Can be negative. (-1) means nothing focused.
        INT m_focusedIndex;

        // Holds the last focused index just before focusing out of the selector.
        UINT m_lastFocusedIndex;

    protected:
        // Handler for when a SelectorItem received focus
        virtual _Check_return_ HRESULT ItemFocused(_In_ SelectorItem* pSelectorItem);

        // Handler for when a SelectorItem lost focus
        virtual _Check_return_ HRESULT ItemUnfocused(_In_ SelectorItem* pSelectorItem);

        bool IsInit()
        {
            return m_pInitializingData != NULL;
        }

        // GetFocusedIndex and SetFocusedIndex are consistently used instead of 
        // m_focusedIndex to make it easier to track when this field is read & written.
        INT GetFocusedIndex() const
        {
            return m_focusedIndex;
        }

        void SetFocusedIndex(INT focusedIndex)
        {
            if (m_focusedIndex != focusedIndex)
            {
                m_focusedIndex = focusedIndex;
            }
        }

        // GetLastFocusedIndex and SetLastFocusedIndex are consistently used instead of 
        // m_lastFocusedIndex to make it easier to track when this field is read & written.
        UINT GetLastFocusedIndex() const
        {
            return m_lastFocusedIndex;
        }

        void SetLastFocusedIndex(UINT lastFocusedIndex)
        {
            if (m_lastFocusedIndex != lastFocusedIndex)
            {
                m_lastFocusedIndex = lastFocusedIndex;
            }
        }

        // The Selection instance responsible for managing our selected items list and
        // providing the transactional API required for modifying that list.
        Selection m_selection;

        // If true, do not scroll Selector Item into view. We will use this flag to prevent Selector from scrolling
        // into view the item we will be animating in FlipView.
        bool m_skipScrollIntoView : 1;
        bool m_inCollectionChange : 1;
        bool m_bSelectionChangeCausedBySelectedValuePathPropertyChange : 1;
        bool m_bCoercingSelectedeValueToNull : 1;
        bool m_skipFocusSuggestion : 1;
        // If true, prevent reentrancy by short-circuiting selection-related functions.
        bool m_isSelectionReentrancyLocked : 1;
        bool m_fUpdatingCurrentItemInCollectionView : 1;
        bool m_fSynchronizeCurrentItem : 1;

        class InitializingData
        {
            friend class Selector;

            // Some data we need to track when in Initializing mode.  We don't want to keep it around all the time.
            INT m_nInitialIndex;
            IInspectable* m_pInitialItem;
            IInspectable* m_pInitialValue;

            InitializingData():
                m_nInitialIndex(-1),
                m_pInitialItem(NULL),
                m_pInitialValue(NULL)
            {
            }

            ~InitializingData()
            {
                ReleaseInterface(m_pInitialItem);
                ReleaseInterface(m_pInitialValue);
            }
        };

        TrackerPtr<wfc::IObservableVector<IInspectable*>> m_tpSelectedItemsImpl;
        EventRegistrationToken m_SelectedItemsVectorChangedToken;
        EventRegistrationToken m_focusEnagedToken;

        TrackerPtr<wfc::IVector<xaml_data::ItemIndexRange*>> m_tpSelectedRangesImpl;

        TrackerPtr<xaml_controls::IScrollViewer> m_tpScrollViewer;

        TrackerPtr<xaml_data::ICollectionView> m_tpMonitoredCV;
        ctl::EventPtr<CurrentChangedEventCallback> m_epCVCurrentChanged;
        InitializingData* m_pInitializingData;
        TrackerPtr<PropertyPathListener> m_tpSelectedValuePropertyPathListener;

        // The order at which the XAML properties are set on Selector should not matter.
        // SelectedItem might be set before ItemsSource is set, in which case the SelectedItem
        // value in m_itemPendingSelection until ItemsSource is set.
        TrackerPtr<IInspectable> m_itemPendingSelection;
    };
}
