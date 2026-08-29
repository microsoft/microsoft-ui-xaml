// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListBox.g.h"
#include "ListBoxAutomationPeer.g.h"
#include "ListBoxItem.g.h"
#include "ItemCollection.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;

ListBox::ListBox()
    : m_pAnchorIndex(NULL)
{
}

ListBox::~ListBox()
{
    delete m_pAnchorIndex;
    m_pAnchorIndex = NULL;
}

_Check_return_ HRESULT ListBox::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ListBoxGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ListBox_SelectionMode:
            IFC(OnSelectionModeChanged(static_cast<xaml_controls::SelectionMode>(args.m_pNewValue->AsEnum())));
            break;
        case KnownPropertyIndex::Selector_IsSelectionActive:
            IFC(OnIsSelectionActiveChanged());
            break;
    }

Cleanup:
    RRETURN(hr);
}

// Handler for selection mode changed event.
_Check_return_ HRESULT ListBox::OnSelectionModeChanged(_In_ xaml_controls::SelectionMode newValue)
{
    HRESULT hr = S_OK;

    if (xaml_controls::SelectionMode_Single == newValue)
    {
        INT selectedIndex = -1;
        IFC(get_SelectedIndex(&selectedIndex));

        if (selectedIndex > -1)
        {
            IFC(MakeSingleSelection(selectedIndex, FALSE /*animateIfBringIntoView*/, NULL));
        }
    }

    // Update the selection mode
    IFC(UpdateCVSynchronizationState());

Cleanup:
    RRETURN(hr);
}

// Called when the IsEnabled property changes.
_Check_return_
HRESULT
ListBox::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(UpdateVisualState(TRUE));

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the ListBox.
_Check_return_
HRESULT
ListBox::ChangeVisualState(
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIgnore = FALSE;
    BOOLEAN bHasFocus = FALSE;

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(HasFocus(&bHasFocus));

    if (!bIsEnabled)
    {
        IFC(GoToState(bUseTransitions, L"Disabled", &bIgnore));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Normal", &bIgnore));
    }

    if(bHasFocus)
    {
        IFC(GoToState(bUseTransitions, L"Focused", &bIgnore));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unfocused", &bIgnore));
    }

Cleanup:
    RRETURN(hr);
}

// List of all items that are currently selected
_Check_return_ HRESULT ListBox::get_SelectedItemsImpl(
    _Outptr_ wfc::IVector<IInspectable*>** pValue)
{
    RRETURN(get_SelectedItemsInternal(pValue));
}

// Property to control whether or not Selector will allow multiple items to be selected at once
_Check_return_
HRESULT
ListBox::get_CanSelectMultiple(
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::SelectionMode mode =
        xaml_controls::SelectionMode_Single;

    IFC(get_SelectionMode(&mode));
    *pValue = xaml_controls::SelectionMode_Single != mode;

Cleanup:
    RRETURN(hr);
}

// Determines if the specified item is (or is eligible to be) its own container.
IFACEMETHODIMP
ListBox::IsItemItsOwnContainerOverride(
    _In_ IInspectable* item,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    IFCEXPECT(returnValue);
    *returnValue = !!ctl::value_is<xaml_controls::IListBoxItem>(item);

Cleanup:
    RRETURN(hr);
}

// Creates or identifies the element that is used to display the given item.
IFACEMETHODIMP
ListBox::GetContainerForItemOverride(
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListBoxItem> spListBoxItem;

    IFCEXPECT(returnValue);
    IFC(ctl::make<ListBoxItem>(&spListBoxItem));

    IFC(spListBoxItem.MoveTo(returnValue));

Cleanup:
    RRETURN(hr);
}

// Called when the control got focus.
IFACEMETHODIMP
ListBox::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;
    CControl *pControl = do_pointer_cast<CControl>(GetHandle());
    IFC(ListBoxGenerated::OnGotFocus(pArgs));
    if (!pControl->IsFocusEngagementEnabled() || pControl->IsFocusEngaged())
    {
        IFCPTR(pArgs);
        IFC(pArgs->get_OriginalSource(&spOriginalSource));

        IFC(HasFocus(&hasFocus));
        IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));

        IFC(FocusChanged(hasFocus, !!isOriginalSource));
    }
Cleanup:
    RRETURN(hr);
}

// Called when the control lost focus.
IFACEMETHODIMP
ListBox::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;

    IFC(ListBoxGenerated::OnLostFocus(pArgs));

    IFCPTR(pArgs);
    IFC(pArgs->get_OriginalSource(&spOriginalSource));

    IFC(HasFocus(&hasFocus));
    IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));
    IFC(FocusChanged(hasFocus, !!isOriginalSource));

Cleanup:
    RRETURN(hr);
}

// Called when the focus has changed.
_Check_return_
HRESULT
ListBox::FocusChanged(
    _In_ BOOLEAN hasFocus,
    _In_ BOOLEAN self)
{
    HRESULT hr = S_OK;

    if(self)
    {
        IFC(put_IsSelectionActive(hasFocus));

        if (hasFocus)
        {
            IFC(SetFocusedItem(GetFocusedIndex() < 0 ? 0 : GetFocusedIndex(), TRUE));
        }
    }

    IFC(UpdateVisualState(TRUE));

Cleanup:
    RRETURN(hr);
}

// Responds to the KeyDown event.
IFACEMETHODIMP
ListBox::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    ctl::ComPtr<IInspectable> spOriginalSource;
    ctl::ComPtr<xaml_controls::IListBoxItem> spListBoxItem;
    BOOLEAN handled = FALSE;
    wsy::VirtualKey key = wsy::VirtualKey_None;
    wsy::VirtualKeyModifiers nModifierKeys;
    xaml_controls::SelectionMode mode =
        xaml_controls::SelectionMode_Single;
    UINT nCount = 0;
    INT newFocusedIndex = -1;

    IFC(ListBoxGenerated::OnKeyDown(pArgs));
    IFC(pArgs->get_Handled(&handled));
    if (handled)
    {
        goto Cleanup;
    }

    IFC(pArgs->get_Key(&key));

    switch (key)
    {
        case wsy::VirtualKey_Enter:
            {
                // TODO: Query for accepts return attached property when bug#99090 is fixed
                BOOLEAN acceptsReturn = TRUE;
                if (!acceptsReturn)
                {
                    break;
                }
            }
        case wsy::VirtualKey_Space:
            IFC(CoreImports::Input_GetKeyboardModifiers(&nModifierKeys));
            if (wsy::VirtualKeyModifiers_Menu != (nModifierKeys &
                (wsy::VirtualKeyModifiers_Menu | wsy::VirtualKeyModifiers_Menu)))
            {
                // KeyRoutedEventArgs.OriginalSource (used by WPF) isn't available in Silverlight; use FocusManager.GetFocusedElement instead
                IFC(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalSource(&spOriginalSource));
                spListBoxItem = spOriginalSource.AsOrNull<xaml_controls::IListBoxItem>();
                if (spListBoxItem)
                {
                    IFC(HandleItemSelection(spListBoxItem.Cast<ListBoxItem>(), FALSE));
                    handled = TRUE;
                }
            }
            break;
        case wsy::VirtualKey_Home:
        case wsy::VirtualKey_End:
        case wsy::VirtualKey_PageUp:
        case wsy::VirtualKey_GamepadLeftTrigger:
        case wsy::VirtualKey_GamepadLeftShoulder:
        case wsy::VirtualKey_PageDown:
        case wsy::VirtualKey_GamepadRightTrigger:
        case wsy::VirtualKey_GamepadRightShoulder:
        case wsy::VirtualKey_Left:
        case wsy::VirtualKey_Up:
        case wsy::VirtualKey_Right:
        case wsy::VirtualKey_Down:
            {
                newFocusedIndex = GetFocusedIndex();
                IFC(HandleNavigationKey(key, /*scrollViewport*/ TRUE, newFocusedIndex));
            }
            break;
        case wsy::VirtualKey_A:
            IFC(get_SelectionMode(&mode));
            if (mode != xaml_controls::SelectionMode_Single)
            {
                IFC(CoreImports::Input_GetKeyboardModifiers(&nModifierKeys));
                if (wsy::VirtualKeyModifiers_Control ==
                    (nModifierKeys & wsy::VirtualKeyModifiers_Control))
                {
                    UINT itemCount = 0;
                    UINT itemCountSelected = 0;
                    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems2;
                    ctl::ComPtr<wfc::IVector<IInspectable*>> spSelectedItems;

                    IFC(get_Items(&spItems2));
                    IFC(get_SelectedItems(&spSelectedItems));

                    IFC(spItems2.Cast<ItemCollection>()->get_Size(&itemCount));
                    IFC(spSelectedItems->get_Size(&itemCountSelected));

                    if (itemCount != 0)
                    {
                        if (itemCount == itemCountSelected)
                        {
                            IFC(DeselectAllItems());
                        }
                        else
                        {
                            IFC(SelectAllItems());
                        }
                        handled = TRUE;
                    }
                }
            }
            break;
        default:
            ASSERT(!handled);
            break;
    }

    if (newFocusedIndex != -1 && newFocusedIndex != GetFocusedIndex())
    {
        handled = TRUE;
        IFC(get_Items(&spItems));
        IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

        newFocusedIndex = static_cast<INT>(MIN(newFocusedIndex, static_cast<INT>(nCount) - 1));
        if (0 <= newFocusedIndex)
        {
            IFC(get_SelectionMode(&mode));
            IFC(CoreImports::Input_GetKeyboardModifiers(&nModifierKeys));

            if (mode == xaml_controls::SelectionMode_Extended &&
                (wsy::VirtualKeyModifiers_Shift ==
                    (nModifierKeys & wsy::VirtualKeyModifiers_Shift)))
            {
                // if keyboard Modifier is Shift key and mode is Extended we need to reselect range
                // and set focus to focused index and bring it into the view if necessary.
                IFC(MakeRangeSelection(
                    newFocusedIndex,
                    /*clearOldSelection*/ !(wsy::VirtualKeyModifiers_Control ==
                    (nModifierKeys & wsy::VirtualKeyModifiers_Control))));
            }
            else if ((wsy::VirtualKeyModifiers_Control ==
                     (nModifierKeys & wsy::VirtualKeyModifiers_Control)) ||
                     mode == xaml_controls::SelectionMode_Multiple)
            {
                // if keyboard Modifier is Ctrl key or mode is Multiple we just want to move the focus
                // to focused index and bring it into the view if necessary.
            }
            else
            {
                BOOLEAN singleSelectionFollowsFocus = FALSE;
                IFC(get_SingleSelectionFollowsFocus(&singleSelectionFollowsFocus));

                // If mode is single selection, select only if DoesSingleSelectionFollowsFocus is true
                if ((mode == xaml_controls::SelectionMode_Single && !!singleSelectionFollowsFocus) ||
                    mode != xaml_controls::SelectionMode_Single)
                {
                    // for all other cases we should select just this item and move focus
                    // to focused index and bring it into the view if necessary.
                    IFC(MakeSingleSelection(newFocusedIndex, FALSE /*animateIfBringIntoView*/, NULL));
                }
            }

            IFC(SetFocusedItem(newFocusedIndex, TRUE /*shouldScrollIntoView*/, FALSE /*forceFocus*/, xaml::FocusState_Keyboard));
        }
    }

    if (handled)
    {
        IFC(pArgs->put_Handled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// Handler for when a ListBoxItem is clicked
// allows different behavior based on the current SelectionMode
_Check_return_ HRESULT ListBox::OnSelectorItemClicked(
    _In_ SelectorItem* pSelectorItem,
    _Out_ BOOLEAN* pFocused)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IListBoxItem> spListBoxItem;
    IFCPTR(pSelectorItem);

    spListBoxItem = ctl::query_interface_cast<xaml_controls::IListBoxItem>(pSelectorItem);

    if (spListBoxItem)
    {
        BOOLEAN alreadyHasFocus = FALSE;

        IFC(spListBoxItem.Cast<ListBoxItem>()->HasFocus(&alreadyHasFocus));

        IFC(spListBoxItem.Cast<ListBoxItem>()->Focus(xaml::FocusState_Pointer, pFocused));

        if (*pFocused)
        {
            IFC(HandleItemSelection(spListBoxItem.Cast<ListBoxItem>(), TRUE));
        }

        IFC(spListBoxItem.Cast<ListBoxItem>()->HasFocus(pFocused));
        if (*pFocused && !alreadyHasFocus)
        {
            spListBoxItem.Cast<ListBoxItem>()->SetExpectingGotFocusEventFromParentInteraction(TRUE);
        }
    }
    else
    {
        IFC(ListBoxGenerated::OnSelectorItemClicked(pSelectorItem, pFocused));
    }

Cleanup:
    RRETURN(hr);
}

// Select item(s) based on current selection mode
_Check_return_
HRESULT
ListBox::HandleItemSelection(
    _In_ ListBoxItem* pListBoxItem,
    _In_ BOOLEAN isMouseSelection)
{
    xaml_controls::SelectionMode mode =
        xaml_controls::SelectionMode_Single;

    wsy::VirtualKeyModifiers nModifierKeys;
    IFC_RETURN(get_SelectionMode(&mode));

    switch(mode)
    {
        case xaml_controls::SelectionMode_Single:
            IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&nModifierKeys));
            if (wsy::VirtualKeyModifiers_Control ==
                (nModifierKeys & wsy::VirtualKeyModifiers_Control))
            {
                IFC_RETURN(MakeToggleSelection(pListBoxItem));
            }
            else
            {
                IFC_RETURN(MakeSingleSelection(pListBoxItem));
            }
            break;

        case xaml_controls::SelectionMode_Multiple:
            IFC_RETURN(MakeToggleSelection(pListBoxItem));
            break;

        case xaml_controls::SelectionMode_Extended:
            IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&nModifierKeys));
            if ((wsy::VirtualKeyModifiers_Shift ==
                (nModifierKeys & wsy::VirtualKeyModifiers_Shift)) &&
                (wsy::VirtualKeyModifiers_Control ==
                (nModifierKeys & wsy::VirtualKeyModifiers_Control)))
            {
                if (isMouseSelection)
                {
                    IFC_RETURN(MakeRangeSelection(pListBoxItem, /*clearOldSelection*/ FALSE));
                }
            }
            else if (wsy::VirtualKeyModifiers_Shift ==
                (nModifierKeys & wsy::VirtualKeyModifiers_Shift))
            {
                IFC_RETURN(MakeRangeSelection(pListBoxItem, /*clearOldSelection*/ TRUE));
            }
            else if (wsy::VirtualKeyModifiers_Control ==
                (nModifierKeys & wsy::VirtualKeyModifiers_Control))
            {
                IFC_RETURN(MakeToggleSelection(pListBoxItem));
            }
            else
            {
                IFC_RETURN(MakeSingleSelection(pListBoxItem));
            }
            break;
    }

    ElementSoundPlayerService* soundPlayerService = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
    IFC_RETURN(soundPlayerService->RequestInteractionSoundForElement(xaml::ElementSoundKind_Invoke, this));

    return S_OK;
}

// Called when the IsSelectionActive property has changed.
_Check_return_
HRESULT
ListBox::OnIsSelectionActiveChanged()
{
    HRESULT hr = S_OK;
    UINT nSelectedCount = 0;

    IFC(m_selection.GetNumItemsSelected(nSelectedCount));
    if (nSelectedCount > 0)
    {
        for (UINT i = 0; i < nSelectedCount; ++i)
        {
            ctl::ComPtr<xaml_controls::IListBoxItem> spListBoxItem;
            ctl::ComPtr<xaml::IDependencyObject> spContainer;
            UINT itemIndex = 0;

            IFC(m_selection.GetIndexAt(i, itemIndex));
            IFC(ContainerFromIndex(itemIndex, &spContainer));
            spListBoxItem = spContainer.AsOrNull<xaml_controls::IListBoxItem>();
            if (spListBoxItem)
            {
                IFC(spListBoxItem.Cast<ListBoxItem>()->ChangeVisualState(true));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Additional processing that occures after Selector has changed the selected item.
_Check_return_
HRESULT
ListBox::OnSelectionChanged(
    _In_ INT oldSelectedIndex,
    _In_ INT newSelectedIndex,
    _In_ IInspectable* pOldSelectedItem,
    _In_ IInspectable* pNewSelectedItem,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;
    BOOLEAN shouldCallBase = FALSE;

    shouldCallBase = ((newSelectedIndex != -1) && (oldSelectedIndex == -1));
    if (!shouldCallBase && (oldSelectedIndex != newSelectedIndex))
    {
        xaml_controls::SelectionMode mode =
            xaml_controls::SelectionMode_Single;

        IFC(get_SelectionMode(&mode));
        shouldCallBase = (xaml_controls::SelectionMode_Single == mode);
    }

    if (shouldCallBase)
    {
        IFC(ListBoxGenerated::OnSelectionChanged(oldSelectedIndex, newSelectedIndex, pOldSelectedItem, pNewSelectedItem, animateIfBringIntoView, focusNavigationDirection));
    }

Cleanup:
    RRETURN(hr);
}

// Handler for when the Items collection is changed. We should adjust _anchorIndex
// if any item before that index were added or removed.
_Check_return_ HRESULT
ListBox::OnItemsChanged(
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action =
        wfc::CollectionChange_Reset;
    UINT index = 0;

    IFC(ListBoxGenerated::OnItemsChanged(e));

    if (!m_pAnchorIndex)
    {
        goto Cleanup;
    }

    IFC(e->get_CollectionChange(&action));

    switch (action)
    {
        case wfc::CollectionChange_Reset:
            delete m_pAnchorIndex;
            m_pAnchorIndex = NULL;
            break;

        case wfc::CollectionChange_ItemInserted:
            IFC(e->get_Index(&index));
            if (index <= *m_pAnchorIndex)
            {
                (*m_pAnchorIndex)++;
            }
            break;

        case wfc::CollectionChange_ItemRemoved:
            IFC(e->get_Index(&index));
            if (index == *m_pAnchorIndex)
            {
                delete m_pAnchorIndex;
                m_pAnchorIndex = NULL;
            }
            else if (index < *m_pAnchorIndex)
            {
                (*m_pAnchorIndex)--;
            }
            break;

        case wfc::CollectionChange_ItemChanged:
            IFC(e->get_Index(&index));
            if (index == *m_pAnchorIndex)
            {
                delete m_pAnchorIndex;
                m_pAnchorIndex = NULL;
            }
            break;
    }

Cleanup:
    RRETURN(hr);
}

// Create ListBoxAutomationPeer to represent the ListBox.
IFACEMETHODIMP ListBox::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IListBoxAutomationPeer> spListBoxAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IListBoxAutomationPeerFactory> spListBoxAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ListBoxAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spListBoxAPFactory));

    IFC(spListBoxAPFactory.Cast<ListBoxAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spListBoxAutomationPeer));
    IFC(spListBoxAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}


// Causes the object to scroll into view.
// If it is not visible, it is aligned either at the top or bottom of the viewport.
_Check_return_ HRESULT ListBox::ScrollIntoViewImpl(
    _In_ IInspectable* item)
{
    RRETURN(ScrollIntoViewInternal(item, /*isHeader*/FALSE, /*isFooter*/FALSE, /*isFromPublicAPI*/TRUE, xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));
}

// Handles selection of single item, and only that item
_Check_return_
HRESULT
ListBox::MakeSingleSelection(
    _In_ UINT index,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_opt_ IInspectable* pSelectedItem,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;

    xaml_controls::SelectionMode mode =
        xaml_controls::SelectionMode_Single;

    IFC(get_SelectionMode(&mode));

    if (xaml_controls::SelectionMode_Extended == mode)
    {
        delete m_pAnchorIndex;
        m_pAnchorIndex = new UINT(index);
    }

    IFC(Selector::MakeSingleSelection(index, animateIfBringIntoView, pSelectedItem, focusNavigationDirection));

Cleanup:
    return hr;
}

// Handles selection of single item, and only that item
_Check_return_
HRESULT
ListBox::MakeSingleSelection(
    _In_ ListBoxItem* pListBoxItem,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spSelectedItem;
    INT index = -1;

    IFC(IndexFromContainer(pListBoxItem, &index));
    IFC(ItemFromContainer(pListBoxItem, &spSelectedItem));

    IFCEXPECT(index >= 0);
    IFC(MakeSingleSelection(index, FALSE /*animateIfBringIntoView*/, spSelectedItem.Get(), focusNavigationDirection));

Cleanup:
    RRETURN(hr);
}

// Toggle IsSelected of item
_Check_return_
HRESULT
ListBox::MakeToggleSelection(
    _In_ ListBoxItem* pListBoxItem)
{
    HRESULT hr = S_OK;
    xaml_controls::SelectionMode mode =
        xaml_controls::SelectionMode_Single;
    BOOLEAN isSelected = FALSE;

    IFC(get_SelectionMode(&mode));

    if (xaml_controls::SelectionMode_Extended == mode)
    {
        INT index = -1;
        IFC(IndexFromContainer(pListBoxItem, &index));
        IFCEXPECT(index >= 0);

        delete m_pAnchorIndex;
        m_pAnchorIndex = new UINT(index);
    }

    IFC(pListBoxItem->get_IsSelected(&isSelected));
    isSelected = !isSelected;
    IFC(pListBoxItem->put_IsSelected(isSelected));

Cleanup:
    RRETURN(hr);
}

// Select a range of items beginning with current anchor
_Check_return_
HRESULT
ListBox::MakeRangeSelection(
    _In_ UINT index,
    _In_ BOOLEAN clearOldSelection)
{
    HRESULT hr = S_OK;
    UINT anchorIndex = 0;
    UINT newIndex = index;

#if DBG
    xaml_controls::SelectionMode mode =
        xaml_controls::SelectionMode_Single;

    IFC(get_SelectionMode(&mode));

    ASSERT(xaml_controls::SelectionMode_Single != mode, L"Invalid selection mode in range selection");
#endif

    if (m_pAnchorIndex)
    {
        anchorIndex = *m_pAnchorIndex;
    }

    if (anchorIndex > index)
    {
        newIndex = anchorIndex;
        anchorIndex = index;
    }

    // call the Selector::SelectRangeInternal
    IFC(SelectRangeInternal(anchorIndex /* firstIndex */, newIndex - anchorIndex + 1 /* length */, clearOldSelection /* clearOldSelection */));

Cleanup:
    RRETURN(hr);
}

// Select a range of items beginning with current anchor
_Check_return_
HRESULT
ListBox::MakeRangeSelection(
    _In_ ListBoxItem* pListBoxItem,
    _In_ BOOLEAN clearOldSelection)
{
    HRESULT hr = S_OK;
    INT index = -1;

    IFC(IndexFromContainer(pListBoxItem, &index));

    IFCEXPECT(index >= 0);
    IFC(MakeRangeSelection(index, clearOldSelection));

Cleanup:
    RRETURN(hr);
}

// Selects all the items.
_Check_return_ HRESULT ListBox::SelectAllImpl()
{
    HRESULT hr = S_OK;
    xaml_controls::SelectionMode mode =
        xaml_controls::SelectionMode_Single;

    IFC(get_SelectionMode(&mode));
    if(mode == xaml_controls::SelectionMode_Single)
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_NOT_SUPPORTED, ERROR_INVALID_MULTIPLE_SELECT));
    }
    IFC(SelectAllItems());

Cleanup:
    RRETURN(hr);
}

// Selects all the items.
_Check_return_
HRESULT
ListBox::SelectAllItems()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    UINT nCount = 0;

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    if (nCount > 0)
    {
        // call the Selector::SelectRangeInternal
        IFC(SelectRangeInternal(0 /* firstIndex */, nCount /* length */, FALSE /* clearOldSelection */));
    }

Cleanup:
    RRETURN(hr);
}

// Deselect all the items.
_Check_return_
HRESULT
ListBox::DeselectAllItems()
{
    HRESULT hr = S_OK;

    IFC(ClearSelection());

Cleanup:
    RRETURN(hr);
}

// Prepares object's state
_Check_return_ HRESULT ListBox::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLoadedEventHandler;
    EventRegistrationToken eventRegistration;

    IFC(ListBoxGenerated::Initialize());

    spLoadedEventHandler.Attach(
        new ClassMemberEventHandler<
            ListBox,
            xaml_controls::IListBox,
            xaml::IRoutedEventHandler,
            IInspectable,
            xaml::IRoutedEventArgs>(this, &ListBox::OnLoaded, true /* subscribingToSelf */ ));

    IFC(add_Loaded(spLoadedEventHandler.Get(), &eventRegistration));

Cleanup:
    RRETURN(hr);
}

// Loaded event handler.
_Check_return_ HRESULT ListBox::OnLoaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(UpdateVisualState(FALSE));

Cleanup:
    RRETURN(hr);
}
