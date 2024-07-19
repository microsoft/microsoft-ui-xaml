// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListBox.g.h"

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    // Represents a ListBox control.
    //
    PARTIAL_CLASS(ListBox)
    {
    public:

        // List of all items that are currently selected
        _Check_return_ HRESULT get_SelectedItemsImpl(
            _Outptr_ wfc::IVector<IInspectable*>** pValue);

        // Causes the object to scroll into view.
        // If it is not visible, it is aligned either at the top or bottom of the viewport.
        _Check_return_ HRESULT ScrollIntoViewImpl(
            _In_ IInspectable* item);

        // Selects all the items.
        _Check_return_ HRESULT SelectAllImpl();

        // Change to the correct visual state for the ListBox.
        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

    protected:
        ListBox();
        ~ListBox() override;


        // Prepares object's state
        _Check_return_ HRESULT Initialize() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Called when the IsEnabled property changes.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        // Property to control whether or not Selector will allow multiple items to be selected at once
        _Check_return_ HRESULT get_CanSelectMultiple(
            _Out_ BOOLEAN* pValue) override;

        // Determines if the specified item is (or is eligible to be) its own container.
        IFACEMETHOD(IsItemItsOwnContainerOverride)(
            _In_ IInspectable* item,
            _Out_ BOOLEAN* returnValue) override;

        // Creates or identifies the element that is used to display the given item.
        IFACEMETHOD(GetContainerForItemOverride)(
            _Outptr_ xaml::IDependencyObject** returnValue) override;

        // Called when the control got focus.
        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        // Called when the control lost focus.
        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        // Responds to the KeyDown event.
        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        // Handler for when a ListBoxItem is clicked
        // allows different behavior based on the current SelectionMode
        _Check_return_ HRESULT OnSelectorItemClicked(
            _In_ SelectorItem* pSelectorItem,
            _Out_ BOOLEAN* pFocused) override;

        // Additional processing that occures after Selector has changed the selected item.
        _Check_return_ HRESULT OnSelectionChanged(
            _In_ INT oldSelectedIndex,
            _In_ INT newSelectedIndex,
            _In_ IInspectable* pOldSelectedItem,
            _In_ IInspectable* pNewSelectedItem,
            _In_ BOOLEAN animateIfBringIntoView = FALSE,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None) override;

        // Handler for when the Items collection is changed. We should adjust _anchorIndex
        // if any item before that index were added or removed.
        _Check_return_ HRESULT OnItemsChanged(
            _In_ wfc::IVectorChangedEventArgs* e) override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;


    private:
        // Handler for selection mode changed event
        _Check_return_ HRESULT OnSelectionModeChanged(_In_ xaml_controls::SelectionMode newValue);

        // Called when the focus has changed.
        _Check_return_ HRESULT FocusChanged(
            _In_ BOOLEAN hasFocus,
            _In_ BOOLEAN self);


        // Select item(s) based on current selection mode
        _Check_return_ HRESULT HandleItemSelection(
            _In_ ListBoxItem* pListBoxItem,
            _In_ BOOLEAN isMouseSelection);

        // Called when the IsSelectionActive property has changed.
        _Check_return_ HRESULT OnIsSelectionActiveChanged();

        // Handles selection of single item, and only that item
        _Check_return_ HRESULT MakeSingleSelection(
            _In_ UINT index,
            _In_ BOOLEAN animateIfBringIntoView,
            _In_opt_ IInspectable* pSelectedItem,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None) override;

        _Check_return_ HRESULT MakeSingleSelection(
            _In_ ListBoxItem* pListBoxItem,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None);

        // Toggle IsSelected of item
        _Check_return_ HRESULT MakeToggleSelection(
            _In_ ListBoxItem* pListBoxItem);

        // Select a range of items beginning with current anchor
        _Check_return_ HRESULT MakeRangeSelection(
            _In_ UINT index,
            _In_ BOOLEAN clearOldSelection);

        _Check_return_ HRESULT MakeRangeSelection(
            _In_ ListBoxItem* pListBoxItem,
            _In_ BOOLEAN clearOldSelection);

        // Selects all the items.
        _Check_return_ HRESULT SelectAllItems();

        // Deselect all the items.
        _Check_return_ HRESULT DeselectAllItems();

        // Loaded event handler.
        _Check_return_ HRESULT OnLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

    private:
        // anchor index shouldn't be negative.
        UINT* m_pAnchorIndex;
    };
}
