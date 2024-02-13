// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScrollViewer.g.h"
#include "ListViewBase.g.h"
#include "ListViewBaseItem.g.h"
#include "GroupItem.g.h"
#include "SemanticZoom.g.h"
#include "ItemClickEventArgs.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "ItemCollection.g.h"
#include "Panel.g.h"
#include "ListViewBaseHeaderItem.g.h"
#include "IKeyboardHeaderNavigationPanel.g.h"
#include "IItemLookupPanel.g.h"
#include "ItemsPresenter.g.h"
#include "IKeyboardNavigationPanel.g.h"
#include "KeyboardNavigation.h"
#include "ItemIndexRangeHelper.h"
#include <DependencyLocator.h>
#include <RuntimeEnabledFeatures.h>
#include "ModernCollectionBasePanel.g.h"
#include "OrientedVirtualizingPanel.g.h"
#include "XboxUtility.h"
#include "FocusManager.g.h"
#include "VisualTreeHelper.h"
#include "FocusPressState.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace RuntimeFeatureBehavior;

// Uncomment to output ListViewBase debugging information
//#define LVB_DEBUG

// Handler for when a GroupItem receives focus.
_Check_return_ HRESULT ListViewBase::GroupItemFocused(_In_ GroupItem* pItem)
{
    INT focusedGroupIndex{ -1 };

    IFC_RETURN(pItem->GetGroupIndex(&focusedGroupIndex));
    SetFocusedGroupIndex(focusedGroupIndex);
    SetLastFocusedGroupIndex(focusedGroupIndex);

    return S_OK;
}

// Handler for when a GroupItem loses focus.
_Check_return_ HRESULT ListViewBase::GroupItemUnfocused(_In_ GroupItem* pItem)
{
    INT groupIndex = -1;

    IFC_RETURN(pItem->GetGroupIndex(&groupIndex));

    if (GetFocusedGroupIndex() == groupIndex)
    {
        SetFocusedGroupIndex(-1);
    }

    return S_OK;
}

// Handler called by ListViewBaseHeaderItem when it receives focus.
_Check_return_ HRESULT ListViewBase::GroupHeaderItemFocused(
    _In_ ListViewBaseHeaderItem* pItem)
{
    INT focusedGroupIndex{ -1 };

    IFC_RETURN(IndexFromHeader(pItem, FALSE /*excludeHiddenEmptyGroups*/, &focusedGroupIndex));
    SetFocusedGroupIndex(focusedGroupIndex);

    if (focusedGroupIndex >= 0)
    {
        BOOLEAN groupHasElements = FALSE;

        SetLastFocusedGroupIndex(focusedGroupIndex);
        m_lastFocusedElementType = ElementType::GroupHeader;
        m_wasItemOrGroupHeaderFocused = true;

        IFC_RETURN(GroupHasElements(
            GetLastFocusedGroupIndex(),
            &groupHasElements));

        if (groupHasElements)
        {
            UINT lastFocusedIndex;

            // Make sure the last focused index falls within range applicable for this group.
            IFC_RETURN(ValidateItemIndexForGroup(
                GetLastFocusedIndex(),
                GetLastFocusedGroupIndex(),
                TRUE,
                FALSE,
                &lastFocusedIndex));
            SetLastFocusedIndex(lastFocusedIndex);
        }
    }

    return S_OK;
}

// Handler called by ListViewBaseHeaderItem when it loses focus.
_Check_return_ HRESULT ListViewBase::GroupHeaderItemUnfocused(
    _In_ ListViewBaseHeaderItem* pItem)
{
    INT groupIndex = -1;

    IFC_RETURN(IndexFromHeader(pItem, FALSE /*excludeHiddenEmptyGroups*/, &groupIndex));

    if (GetFocusedGroupIndex() == groupIndex)
    {
        SetFocusedGroupIndex(-1);
    }

    return S_OK;
}

// Handler for when a SelectorItem received focus
_Check_return_ HRESULT ListViewBase::ItemFocused(_In_ SelectorItem* pSelectorItem)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: ItemFocused.", ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this));
#endif

    BOOLEAN shouldCustomizeTabNavigation = FALSE;

    IFC_RETURN(ListViewBaseGenerated::ItemFocused(pSelectorItem));

    m_lastFocusedElementType = ElementType::Item;
    m_wasItemOrGroupHeaderFocused = true;

    IFC_RETURN(ShouldCustomizeTabNavigation(&shouldCustomizeTabNavigation));

    if (shouldCustomizeTabNavigation)
    {
        BOOLEAN isGrouping = FALSE;

        IFC_RETURN(get_IsGrouping(&isGrouping));

        if (isGrouping)
        {
            ctl::ComPtr<xaml_data::ICollectionViewGroup> spGroup;
            INT lastFocusedGroupIndex{ -1 };

            IFC_RETURN(GetGroupFromItem(
                GetFocusedIndex(),
                &spGroup,
                nullptr,
                &lastFocusedGroupIndex,
                nullptr));
            SetLastFocusedGroupIndex(lastFocusedGroupIndex);
        }

        // Sometimes when focus moves from a header to an item (disconnected)
        // the header does not get the GroupHeaderItemUnfocused call. In order to
        // workaround that, if an item gets focus, we set the focused group index to -1.
        SetFocusedGroupIndex(-1);
    }

    // When SelectionMode is 'Single', SingleSelectionFollowsFocus is True, TabNavigation is 'Cycle' or 'Local',
    // the focused item needs to be selected.
    xaml_input::KeyboardNavigationMode navigationMode;

    IFC_RETURN(get_TabNavigation(&navigationMode));

    if (navigationMode != xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Once)
    {
        BOOLEAN singleSelectionFollowsFocus = FALSE;

        IFC_RETURN(get_SingleSelectionFollowsFocus(&singleSelectionFollowsFocus));

        if (singleSelectionFollowsFocus)
        {
            xaml_controls::ListViewSelectionMode selectionMode;

            IFC_RETURN(get_SelectionMode(&selectionMode));

            if (selectionMode == xaml_controls::ListViewSelectionMode_Single)
            {
                IFC_RETURN(MakeSingleSelection(GetLastFocusedIndex(), FALSE /*animateIfBringIntoView*/, nullptr /*pSelectedItem*/));
            }
        }
    }

    return S_OK;
}

// Automation Helper for ItemClick
_Check_return_ HRESULT ListViewBase::AutomationItemClick(_In_ ListViewBaseItem* pItem)
{
    IFCPTR_RETURN(pItem);
    BOOLEAN pIsHandled = FALSE;

    // We only use FocusState_Pointer here as one of the primary scenario is touch narrator which is mocking a tap.
    // FocusState_Keyboard is not really required here.
    IFC_RETURN(OnItemPrimaryInteractionGesture(pItem, FALSE /*isKeyboardInput*/, &pIsHandled));

    return S_OK;
}

// Automation Helper for HeaderItemClick
_Check_return_ HRESULT ListViewBase::AutomationHeaderItemClick(_In_ ListViewBaseHeaderItem* headerItem)
{
    BOOLEAN isHandled = FALSE;

    IFC_RETURN(ToggleSemanticZoomActiveView(ctl::as_iinspectable(headerItem), &isHandled));

    return S_OK;
}

// Handles the primary interaction gesture for a ListViewBaseItem (Tap, Click, or
// Space) and updates selection or raises the ItemClick event.
_Check_return_ HRESULT ListViewBase::OnItemPrimaryInteractionGesture(
    _In_ ListViewBaseItem* pItem,
    _In_ BOOLEAN isKeyboardInput,
    _Out_ BOOLEAN* pIsHandled)
{
    ctl::ComPtr<ISemanticZoom> spSemanticZoom;
    BOOLEAN isZoomedInView = FALSE;

    auto guard = wil::scope_exit([this]()
    {
        m_spContainerBeingClicked = nullptr;
    });

    IFCPTR_RETURN(pItem);
    IFCPTR_RETURN(pIsHandled);
    *pIsHandled = FALSE;

    // Get the SemanticZoom owner
    IFC_RETURN(get_SemanticZoomOwner(&spSemanticZoom));

    IFC_RETURN(get_IsZoomedInView(&isZoomedInView));

    // store the container as a suggestion so that any call to ContainerFromItem that is called from user code
    // during any of the below callouts, will be able to be more intelligent.
    ASSERT(m_spContainerBeingClicked.Get() == nullptr);
    m_spContainerBeingClicked = pItem;

    // If we're not the ZoomedOutView of a SemanticZoom,
    // do a regular primary interaction.
    if (!spSemanticZoom || isZoomedInView)
    {
        xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;
        BOOLEAN isItemClickEnabled = FALSE;
        bool shouldSetFocus = true;

        IFC_RETURN(get_SelectionMode(&mode));

        // We interpret the primary interaction gesture as an ItemClick if it's
        // enabled or as a selection otherwise.
        IFC_RETURN(get_IsItemClickEnabled(&isItemClickEnabled));
        if (isItemClickEnabled)
        {
            // Focus the item before ItemClick
            IFC_RETURN(FocusItem(isKeyboardInput ? xaml::FocusState_Keyboard : xaml::FocusState_Pointer, pItem));

            // After setting focus to the item, we don't want to set it again.
            // When we call out to user-code in OnItemClick, user-code could move focus (directly or indirectly). We don't want to
            // stomp on their change.
            shouldSetFocus = false;

            // Raise the ItemClick event
            IFC_RETURN(OnItemClick(pItem));
            *pIsHandled = TRUE;
        }

        if (mode != xaml_controls::ListViewSelectionMode_None)
        {
            // Update the selection accordingly
            IFC_RETURN(OnSelectItemPrimary(pItem, isKeyboardInput, shouldSetFocus, pIsHandled));
        }

        if (!*pIsHandled && shouldSetFocus)
        {
            // Selection is disabled. Just focus the item.
            IFC_RETURN(FocusItem(isKeyboardInput ? xaml::FocusState_Keyboard : xaml::FocusState_Pointer, pItem));
            *pIsHandled = TRUE;
        }
    }
    // If we're in a SemanticZoom, the primary interaction gesture in the ZoomedOutView
    // should be used to jump back into the ZoomedInView
    else
    {
        ctl::ComPtr<SemanticZoom> spSemanticZoomConcrete;

        if (spSemanticZoom)
        {
            INT focusedIndex{ -1 };

            // Make the item the target of our jump
            IFC_RETURN(FocusItem(isKeyboardInput ? xaml::FocusState_Keyboard : xaml::FocusState_Pointer, pItem));
            IFC_RETURN(IndexFromContainer(pItem, &focusedIndex));
            SetFocusedIndex(focusedIndex);

            // Jump to this item
            spSemanticZoomConcrete = spSemanticZoom.Cast<SemanticZoom>();
            IFC_RETURN(spSemanticZoomConcrete->ToggleActiveViewWithFocusState(
                isKeyboardInput ? xaml::FocusState_Keyboard : xaml::FocusState_Pointer));
            *pIsHandled = TRUE;
        }
    }

    ElementSoundPlayerService* soundPlayerService = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
    IFC_RETURN(soundPlayerService->RequestInteractionSoundForElement(xaml::ElementSoundKind_Invoke, this));

    if (isKeyboardInput)
    {
        KeyPress::StartFocusPress(checked_cast<CUIElement>(GetHandle()));
    }

    return S_OK;
}

// Handles the secondary interaction gesture for a ListViewBaseItem
// (Ctrl+Click or Ctrl+Space) and updates selection accordingly.
_Check_return_ HRESULT ListViewBase::OnItemSecondaryInteractionGesture(
    _In_ ListViewBaseItem* pItem,
    _In_ BOOLEAN isKeyboardInput,
    _Out_ BOOLEAN* pIsHandled)
{
    xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;

    auto guard = wil::scope_exit([this]()
    {
        m_spContainerBeingClicked = nullptr;
    });

    IFCPTR_RETURN(pItem);
    IFCPTR_RETURN(pIsHandled);
    *pIsHandled = FALSE;

    // store the container as a suggestion so that any call to ContainerFromItem that is called from user code
    // during any of the below callouts, will be able to be more intelligent.
    ASSERT(m_spContainerBeingClicked.Get() == nullptr);
    m_spContainerBeingClicked = pItem;

    BOOLEAN isItemClickEnabled = FALSE;
    IFC_RETURN(get_IsItemClickEnabled(&isItemClickEnabled));

    if (isItemClickEnabled)
    {
        // Raise the ItemClick event
        IFC_RETURN(OnItemClick(pItem));
        *pIsHandled = TRUE;
    }

    // We ignore all secondary input gestures if the SelectionMode is None.
    IFC_RETURN(get_SelectionMode(&mode));
    if (mode != xaml_controls::ListViewSelectionMode_None)
    {
        // Update the selection
        IFC_RETURN(OnSelectItemSecondary(pItem, isKeyboardInput, pIsHandled));
    }

    ElementSoundPlayerService* soundPlayerService = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
    IFC_RETURN(soundPlayerService->RequestInteractionSoundForElement(xaml::ElementSoundKind_Invoke, this));

    return S_OK;
}

// Selects item(s) based on the current SelectionMode, given a primary interaction gesture.
_Check_return_ HRESULT ListViewBase::OnSelectItemPrimary(
    _In_ ListViewBaseItem* pItem,
    bool isKeyboardInput,
    bool shouldFocusItem,
    _Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;
    xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;

    IFCPTR(pItem);
    IFCPTR(pIsHandled);

    if (shouldFocusItem)
    {
        // Focus the item
        IFC(FocusItem(isKeyboardInput ? xaml::FocusState_Keyboard : xaml::FocusState_Pointer, pItem));
    }

    IFC(get_SelectionMode(&mode));
    switch(mode)
    {
        // There's nothing to select when SelectionMode is None
        // case xaml_controls::ListViewSelectionMode_None:
        //     break;
        case xaml_controls::ListViewSelectionMode_Single:
        {
            IFC(MakeSingleSelection(pItem));
            *pIsHandled = TRUE;
            break;
        }
        case xaml_controls::ListViewSelectionMode_Multiple:
        {
            wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
            IFC(GetKeyboardModifiers(&modifiers));

            if (IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift))
            {
                // Create a range selection from the current anchor to the
                // current item, keeping all items outside this change as is.
                IFC(MakeRangeSelection(pItem, /*clearOldSelection*/ FALSE));
                *pIsHandled = TRUE;
            }
            else
            {
                // Add or remove the current item to the selection depending on
                // whether it was already selected or not.
                IFC(MakeToggleSelection(pItem));
                *pIsHandled = TRUE;
            }
            break;
        }
        case xaml_controls::ListViewSelectionMode_Extended:
        {
            wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
            IFC(GetKeyboardModifiers(&modifiers));
            if (IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift))
            {
                // Create a range selection from the current anchor to the
                // current item, unselecting all items outside of this range.
                IFC(MakeRangeSelection(pItem, /*clearOldSelection*/ TRUE));
                *pIsHandled = TRUE;
            }
            else
            {
                IFC(MakeSingleSelection(pItem));
                *pIsHandled = TRUE;
            }

            break;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Selects item(s) based on the current SelectionMode, given a secondary interaction gesture.
_Check_return_ HRESULT ListViewBase::OnSelectItemSecondary(
    _In_ ListViewBaseItem* pItem,
    _In_ BOOLEAN isKeyboardInput,
    _Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;
    xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;

    IFCPTR(pItem);
    IFCPTR(pIsHandled);

    // Focus the item
    IFC(FocusItem(isKeyboardInput ? xaml::FocusState_Keyboard : xaml::FocusState_Pointer, pItem));

    IFC(get_SelectionMode(&mode));
    switch(mode)
    {
        // There's nothing to select when SelectionMode is None
        // case xaml_controls::ListViewSelectionMode_None:
        //     break;
        case xaml_controls::ListViewSelectionMode_Single:
        {
            IFC(MakeToggleSelection(pItem));
            *pIsHandled = TRUE;
            break;
        }
        case xaml_controls::ListViewSelectionMode_Multiple:
        {
            wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
            IFC(GetKeyboardModifiers(&modifiers));

            if (IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift))
            {
                // Multiple selection will now support Ctrl+Shift+Click to create a
                // range selection from the current anchor to the current item,
                // leaving all previously selected items selected.
                IFC(MakeRangeSelection(pItem, /*clearOldSelection*/ FALSE));
                *pIsHandled = TRUE;
            }
            else
            {
                IFC(MakeToggleSelection(pItem));
                *pIsHandled = TRUE;
            }
            break;
        }
        case xaml_controls::ListViewSelectionMode_Extended:
        {
            wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
            IFC(GetKeyboardModifiers(&modifiers));
            if (IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift))
            {
                // Extended selection supports Ctrl+Shift+Click to create a
                // range selection from the current anchor to the current item,
                // leaving all previously selected items selected.
                IFC(MakeRangeSelection(pItem, /*clearOldSelection*/ FALSE));
                *pIsHandled = TRUE;
            }
            else
            {
                // Toggle the selection of the current item (effectively adding
                // or removing it to the current selection)
                IFC(MakeToggleSelection(pItem));
                *pIsHandled = TRUE;
            }
            break;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Raises the ItemClick event.
_Check_return_ HRESULT ListViewBase::OnItemClick(
    _In_ ListViewBaseItem* pContainer)
{
    ctl::ComPtr<ItemClickEventArgs> spArgs;
    ctl::ComPtr<IInspectable> spItemData;

    UIElement::VirtualizationInformation* pVirtualizationInformation = nullptr;

    IFCPTR_RETURN(pContainer);

    // Create the args
    IFC_RETURN(ctl::make<ItemClickEventArgs>(&spArgs));
    IFC_RETURN(spArgs->put_OriginalSource(ctl::as_iinspectable(this)));

    // we assign content to the container, but that is not necessarily
    // the correct item, nor are the new panels guaranteeing that
    // they set content
    pVirtualizationInformation = static_cast<UIElement*>(pContainer)->GetVirtualizationInformation();
    if (pVirtualizationInformation)
    {
        BOOLEAN isItemAContainer = FALSE;

        spItemData = pVirtualizationInformation->GetItem();

        // Do a quick check to see if the item is its own container before
        // proceeding.
        IFC_RETURN(IsItemItsOwnContainer(spItemData.Get(), &isItemAContainer));

        if (isItemAContainer)
        {
            // Item was a container. We need to get its content.
            ctl::ComPtr<IContentControl> spTempItemAsICC = spItemData.AsOrNull<IContentControl>();
            ASSERT(spTempItemAsICC);

            if (spTempItemAsICC)
            {
                // Get the data from the container.
                // This is safe, because if you are your own container, you have been defined with content in it.
                // The CCC work does not truly apply, nor do the placeholders.
                IFC_RETURN(spTempItemAsICC->get_Content(&spItemData));
            }
        }
    }
    else
    {
        // fallback to old behavior
        IFC_RETURN(pContainer->get_Content(&spItemData));
    }

    IFC_RETURN(spArgs->put_ClickedItem(spItemData.Get()));

    // Raise the event
    ItemClickEventSourceType* pEventSource = nullptr;
    IFC_RETURN(GetItemClickEventSourceNoRef(&pEventSource));
    IFC_RETURN(pEventSource->Raise(
        ctl::as_iinspectable(this),
        spArgs.Cast<xaml_controls::IItemClickEventArgs>()));

    return S_OK;
}

// Tells our KeyDown handler that the key down args came from an item so we should handle them.
// Otherwise we should pass them to the scroll view.
void ListViewBase::SetHandleKeyDownArgsFromItem(_In_ bool fFromItem)
{
    m_bKeyDownArgsFromItem = fFromItem;
}

// Makes use of ModernCollectionBasePane's get_FirstCacheGroupIndexBase/get_LastCacheGroupIndexBase, 
// ModernCollectionBasePane's get_FirstCacheIndexBase/get_LastCacheIndexBase, OrientedVirtualizingPanel's GetFirstCacheIndex/GetLastCacheIndex
// to retrieve the index range of realized items or groups. Falls back to the entire collection range. 
_Check_return_ HRESULT ListViewBase::GetCacheStartAndEnd(
    bool isForGroupIndexes,
    _Out_ int* cacheIndexStart,
    _Out_ int* cacheIndexEnd)
{
    *cacheIndexStart = -1;
    *cacheIndexEnd = -1;

    ctl::ComPtr<IPanel> itemsPanel;

    IFC_RETURN(get_ItemsHost(&itemsPanel));

    if (!itemsPanel)
    {
        return S_OK;
    }

    ctl::ComPtr<IModernCollectionBasePanel> modernPanel = itemsPanel.AsOrNull<IModernCollectionBasePanel>();

    if (modernPanel)
    {
        if (isForGroupIndexes)
        {
            IFC_RETURN(modernPanel.Cast<ModernCollectionBasePanel>()->get_FirstCacheGroupIndexBase(cacheIndexStart));
            IFC_RETURN(modernPanel.Cast<ModernCollectionBasePanel>()->get_LastCacheGroupIndexBase(cacheIndexEnd));
        }
        else
        {
            IFC_RETURN(modernPanel.Cast<ModernCollectionBasePanel>()->get_FirstCacheIndexBase(cacheIndexStart));
            IFC_RETURN(modernPanel.Cast<ModernCollectionBasePanel>()->get_LastCacheIndexBase(cacheIndexEnd));
        }
    }
    else
    {
        ctl::ComPtr<IOrientedVirtualizingPanel> orientedVirtualizingPanel;
        BOOLEAN isGrouping = FALSE;

        IFC_RETURN(get_IsGrouping(&isGrouping));

        ASSERT(!(isForGroupIndexes && !isGrouping));

        if (!isGrouping)
        {
            orientedVirtualizingPanel = itemsPanel.AsOrNull<IOrientedVirtualizingPanel>();
        }

        if (orientedVirtualizingPanel)
        {
            *cacheIndexStart = orientedVirtualizingPanel.Cast<OrientedVirtualizingPanel>()->GetFirstCacheIndex();
            *cacheIndexEnd = orientedVirtualizingPanel.Cast<OrientedVirtualizingPanel>()->GetLastCacheIndex();
        }
        else if (isForGroupIndexes)
        {
            // This is encountered in the grouped VirtualizingStackPanel case, when the TabNavigation is 'Local' or 'Cycle' 
            // and the group count is expected to be small.
            int groupCount = 0;

            IFC_RETURN(GetGroupCount(&groupCount));

            *cacheIndexStart = 0;
            *cacheIndexEnd = groupCount - 1;
        }
        else
        {
            unsigned int itemCount = 0;

            IFC_RETURN(GetItemCount(itemCount));

            if (itemCount > 0)
            {
                // Non-virtualizing StackPanel or grouped VirtualizingStackPanel for instance.
                // In the grouped VirtualizingStackPanel case, this is encountered when the TabNavigation is 'Local' or 'Cycle' 
                // and the total item count is expected to be small.
                *cacheIndexStart = 0;
                *cacheIndexEnd = itemCount - 1;
            }
        }
    }

    return S_OK;
}

// Handles when a key is pressed down on the ListViewBase.
IFACEMETHODIMP ListViewBase::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs) noexcept
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: OnKeyDown.", ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this));
#endif

    BOOLEAN isHandled = FALSE;
    BOOLEAN animateIfBringIntoView = FALSE;
    wsy::VirtualKey originalKey = wsy::VirtualKey_None;
    wsy::VirtualKey key = wsy::VirtualKey_None;
    wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
    xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;
    INT newFocusedIndex = -1;
    auto newFocusedType = xaml_controls::ElementType_ItemContainer;
    ctl::ComPtr<IInspectable> spOriginalSource;
    xaml_input::FocusNavigationDirection gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None;
    const bool handleEvent = m_bKeyDownArgsFromItem;
    bool is2DPageNavigation = false;
    float verticalViewportPadding = 0.0f;
    bool wasGroupInitiallyFocused = HasFocusedGroup();

    // reset the m_bKeyDownArgsFromItem variable
    SetHandleKeyDownArgsFromItem(false /* fFromItem */);

    // Ignore already handled events
    IFCPTR_RETURN(pArgs);
    IFC_RETURN(ListViewBaseGenerated::OnKeyDown(pArgs));
    IFC_RETURN(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        return S_OK;
    }

    if (IsInExclusiveInteraction())
    {
        // During an exclusive interaction (drag/drop), disable all keyboard interaction.
        // Handle the event, since other controls don't have the necessary context around drag/drop interactions.
        IFC_RETURN(pArgs->put_Handled(TRUE));
        return S_OK;
    }

    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_Key(&key));

    // If gamepad keys, we want to handle keys that come from items, headers, listview etc.
    // For Keyboard home/end/page keys, if the key came from something other than an item, we scroll the ScrollViewer.
    if (!handleEvent && !XboxUtility::IsGamepadNavigationDirection(originalKey) && !XboxUtility::IsGamepadPageNavigationDirection(originalKey))
    {
        if (key == wsy::VirtualKey_End ||
            key == wsy::VirtualKey_Home ||
            key == wsy::VirtualKey_PageUp ||
            key == wsy::VirtualKey_PageDown)
        {
            IFC_RETURN(ElementScrollViewerScrollInDirection(key));
            IFC_RETURN(pArgs->put_Handled(TRUE));
        }

        return S_OK;
    }

    // TODO: Ignore keyboard input if input has already been "captured" by
    // another device.

    // Most keyboard input (except navigation if we're in a mode where
    // navigation moves focus rather than selection) is ignored if the
    // SelectionMode is None.
    IFC_RETURN(get_SelectionMode(&mode));

    switch (originalKey)
    {
        // TODO: Should we move Enter/Space down to ListViewBaseItem instead so that
        // the interaction processing split between ListView and ListViewBaseItem is
        // symmetrical for keyboard and pointer input?
        case wsy::VirtualKey_Enter:
        {
            IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalSource(&spOriginalSource));

            if (spOriginalSource && IsListViewBaseItem(ctl::iinspectable_cast(spOriginalSource.Get())))
            {
                // Enter is considered a primary input gesture
                IFC_RETURN(OnItemPrimaryInteractionGesture(spOriginalSource.AsOrNull<ListViewBaseItem>().Get(), TRUE, &isHandled));
            }
            break;
        }

        case wsy::VirtualKey_Space:
        {
            // Ignore Alt+Space key presses which bring up the current window's context menu.
            IFC_RETURN(GetKeyboardModifiers(&modifiers));

            if (!IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Menu))
            {
                IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalSource(&spOriginalSource));

                if (spOriginalSource && IsListViewBaseItem(ctl::iinspectable_cast(spOriginalSource.Get())))
                {
                    const bool shouldInvokeSecondary = IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Control);

                    if (shouldInvokeSecondary)
                    {
                        // Space is considered a secondary input gesture
                        IFC_RETURN(OnItemSecondaryInteractionGesture(spOriginalSource.AsOrNull<ListViewBaseItem>().Get(), TRUE /* isKeyboardInput */, &isHandled));
                    }
                    else
                    {
                        IFC_RETURN(OnItemPrimaryInteractionGesture(spOriginalSource.AsOrNull<ListViewBaseItem>().Get(), TRUE /* isKeyboardInput */, &isHandled));
                    }
                }
            }
            break;
        }

        case wsy::VirtualKey_GamepadDPadUp:
        case wsy::VirtualKey_GamepadLeftThumbstickUp:
        case wsy::VirtualKey_NavigationUp:
            gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Up;
            animateIfBringIntoView = TRUE;
            break;
        case wsy::VirtualKey_GamepadDPadDown:
        case wsy::VirtualKey_GamepadLeftThumbstickDown:
        case wsy::VirtualKey_NavigationDown:
            gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down;
            animateIfBringIntoView = TRUE;
            break;
        case wsy::VirtualKey_GamepadDPadLeft:
        case wsy::VirtualKey_GamepadLeftThumbstickLeft:
        case wsy::VirtualKey_NavigationLeft:
            gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left;
            animateIfBringIntoView = TRUE;
            break;
        case wsy::VirtualKey_GamepadDPadRight:
        case wsy::VirtualKey_GamepadLeftThumbstickRight:
        case wsy::VirtualKey_NavigationRight:
            gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right;
            animateIfBringIntoView = TRUE;
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
            bool canNavigate = true;
            IFC_RETURN(CanPerformKeyboardNavigation(key, &canNavigate));

            if (!canNavigate)
            {
                break;
            }

#ifdef DBG
            static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
            // Turn on animated navigations for regular keyboard keys in debug builds when the EnableAnimatedListViewBaseScrolling feature is enabled for convenient testing purposes.
            animateIfBringIntoView = animateIfBringIntoView || runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::EnableAnimatedListViewBaseScrolling);
#endif
            if (animateIfBringIntoView && m_tpScrollViewer && m_tpScrollViewer.Cast<ScrollViewer>()->IsInManipulation())
            {
                IFC_RETURN(m_tpScrollViewer.Cast<ScrollViewer>()->StopInertialManipulation());
            }

            {
                ctl::ComPtr<IPanel> spItemsPanel;

                // Map gamepad trigger/shoulder keys to page navigation
                if (key == wsy::VirtualKey_GamepadLeftTrigger || key == wsy::VirtualKey_GamepadLeftShoulder)
                {
                    key = wsy::VirtualKey_PageUp;
                }

                if (key == wsy::VirtualKey_GamepadRightTrigger || key == wsy::VirtualKey_GamepadRightShoulder)
                {
                    key = wsy::VirtualKey_PageDown;
                }

                IFC_RETURN(OnKeyboardNavigation(key, originalKey, animateIfBringIntoView, &newFocusedIndex, &newFocusedType));

                // perform 2D page navigation if we are using a modern panel and we got a paging key (trigger/bumper)
                if (SUCCEEDED(get_ItemsHost(&spItemsPanel)) && ctl::is<IModernCollectionBasePanel>(spItemsPanel) &&
                    XboxUtility::IsGamepadPageNavigationDirection(originalKey))
                {
                    // if we are doing gamepad paging, the above call to OnKeyboardNavigation ensures that the
                    // next page is realized, now we can let 2D focus figure out the candidate for us.
                    ctl::ComPtr<IModernCollectionBasePanel> spItemsHostPanelModernCollection;
                    IFC_RETURN(spItemsPanel.As<IModernCollectionBasePanel>(&spItemsHostPanelModernCollection));
                    // pad the viewport by the amount the header is sticky.
                    IFC_RETURN(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->GetVerticalViewportPadding(&verticalViewportPadding));

                    // update layout is required here because the modern panel has already scrolled by a page
                    // to realize the content and set its window back to original but not run layout yet.
                    IFC_RETURN(UpdateLayout());
                    gamepadDirection = XboxUtility::GetPageNavigationDirection(originalKey);
                    animateIfBringIntoView = TRUE;
                    newFocusedIndex = -1;
                    newFocusedType = xaml_controls::ElementType_ItemContainer;
                    is2DPageNavigation = true;
                }
            }
            break;
        }
        case wsy::VirtualKey_A:
        {
            // Select all items on Ctrl+A
            if (mode != xaml_controls::ListViewSelectionMode_Single &&
                mode != xaml_controls::ListViewSelectionMode_None)
            {
                IFC_RETURN(GetKeyboardModifiers(&modifiers));
                if (modifiers ==  wsy::VirtualKeyModifiers_Control)
                {
                    UINT itemCount = 0;
                    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

                    IFC_RETURN(get_Items(&spItems));
                    IFC_RETURN(spItems.Cast<ItemCollection>()->get_Size(&itemCount));

                    if (itemCount != 0)
                    {
                        UINT itemCountSelected = 0;
                        ctl::ComPtr<xaml_data::ISelectionInfo> spDataSourceAsSelectionInfo;

                        // call the Selector::GetDataSourceAsSelectionInfo
                        IFC_RETURN(GetDataSourceAsSelectionInfo(&spDataSourceAsSelectionInfo));

                        // if the DataSource implemented SelectionInfo, we get the selected ranges and merge them if needed
                        // in the end, if all items were selected, we should end up with one range with its lenght being the itemcount
                        if (spDataSourceAsSelectionInfo)
                        {
                            unsigned int selectedRangesCount = 0;
                            ctl::ComPtr<wfc::IVectorView<xaml_data::ItemIndexRange*>> spSelectedRanges;

                            IFC_RETURN(spDataSourceAsSelectionInfo->GetSelectedRanges(&spSelectedRanges));

                            IFC_RETURN(spSelectedRanges->get_Size(&selectedRangesCount));

                            if (selectedRangesCount > 0)
                            {
                                std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;
                                DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;

                                for (unsigned int i = 0; i < selectedRangesCount; ++i)
                                {
                                    int firstIndex = -1;
                                    unsigned int length = 0;
                                    ctl::ComPtr<xaml_data::IItemIndexRange> spRange;

                                    IFC_RETURN(spSelectedRanges->GetAt(i, &spRange));
                                    IFC_RETURN(spRange->get_FirstIndex(&firstIndex));
                                    IFC_RETURN(spRange->get_Length(&length));

                                    rangeSelection.SelectRange(firstIndex, length, addedRanges);
                                }

                                if (rangeSelection.size() == static_cast<unsigned int>(1))
                                {
                                    itemCountSelected = (*rangeSelection.begin()).length;
                                }
                            }
                        }
                        else
                        {
                            ctl::ComPtr<wfc::IVector<IInspectable*>> spSelectedItems;

                            IFC_RETURN(get_SelectedItems(&spSelectedItems));
                            IFC_RETURN(spSelectedItems->get_Size(&itemCountSelected));
                        }

                        if (itemCount == itemCountSelected)
                        {
                            IFC_RETURN(DeselectAllImpl());
                        }
                        else
                        {
                            IFC_RETURN(SelectAll());
                        }
                        isHandled = TRUE;
                    }
                }
            }
            break;
        }
        // TODO: Support Ctrl+C, Ctrl+X, Ctrl+Z
    }

    // If it was a game pad navigation key, use auto focus to figure out the next focused index
    if (gamepadDirection != xaml_input::FocusNavigationDirection::FocusNavigationDirection_None)
    {
        // ask auto focus for next item, scoping for items only under the listview
        // check if that element is a ListViewBaseItem - if it is, then find its index and
        // use that as newFocusedIndex
        ctl::ComPtr<xaml_input::IFocusManagerStaticsPrivate> spFocusManager;
        ctl::ComPtr<IInspectable> spCandidate;
        ctl::ComPtr<ListViewBase> spListViewBase(this);
        ctl::ComPtr<IInspectable> spListViewAsI;
        ctl::ComPtr<ListViewBaseItem> spCurrentAsLVBI;
        BOOLEAN isCandidateAlreadyFocused = FALSE;

        IFC_RETURN(ctl::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
            &spFocusManager));

        if (is2DPageNavigation)
        {
            if (m_tpScrollViewer)
            {
                ctl::ComPtr<ScrollViewer> scrollViewer = m_tpScrollViewer.Cast<ScrollViewer>();

                IFC_RETURN(scrollViewer->GetGamepadNavigationCandidate(
                    XboxUtility::GetPageNavigationDirection(originalKey),
                    true /* isPageNavigation */,
                    2 /* numPagesLookAhead */,
                    verticalViewportPadding /* verticalViewportPadding */,
                    &spCandidate));

                // if the candidate we get is the currently focused element, we do not
                // want to set focus/scroll again and handle the key. We should let the key
                // bubble up so that the next control can handle it.
                ctl::ComPtr<FrameworkElement> spCandidateAsFE = spCandidate.AsOrNull<FrameworkElement>();
                if (spCandidateAsFE)
                {
                    IFC_RETURN(spCandidateAsFE->HasFocus(&isCandidateAlreadyFocused));
                    if (isCandidateAlreadyFocused)
                    {
                        spCandidate = nullptr;
                    }
                }
            }
        }
        else
        {
            IFC_RETURN(spListViewBase.As(&spListViewAsI));
            IFC_RETURN(spFocusManager->FindNextFocusWithSearchRootIgnoreEngagement(gamepadDirection,
                spListViewAsI.Get(), /* searchRoot */
                spCandidate.GetAddressOf()));

            // If no candidate was found and the currently focused element
            // is outside the visible range of items. Bring back focus to
            // something within the visible range. This can happen when flipping
            // through at a fast clip and the focused element pans out of the visible range.
            if (!spCandidate)
            {
                ctl::ComPtr<DependencyObject> currentFocusedElement;
                IFC_RETURN(GetFocusedElement(&currentFocusedElement));
                if (currentFocusedElement)
                {
                    if (auto currentFocusAsLVBI = currentFocusedElement.AsOrNull<ListViewBaseItem>())
                    {
                        ctl::ComPtr<IPanel> itemsHost;
                        IFC_RETURN(get_ItemsHost(&itemsHost));
                        ctl::ComPtr<ModernCollectionBasePanel> panel = itemsHost.AsOrNull<ModernCollectionBasePanel>();
                        if (panel)
                        {
                            int focusIndex = -1;
                            IFC_RETURN(IndexFromContainer(currentFocusAsLVBI.Get(), &focusIndex));

                            int firstVisibleIndex = -1;
                            int lastVisibleIndex = -1;
                            IFC_RETURN(panel->get_FirstVisibleIndexBase(&firstVisibleIndex));
                            IFC_RETURN(panel->get_LastVisibleIndexBase(&lastVisibleIndex));

                            if (focusIndex < firstVisibleIndex || focusIndex > lastVisibleIndex)
                            {
                                int targetIndex = -1;
                                if (gamepadDirection == xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left ||
                                    gamepadDirection == xaml_input::FocusNavigationDirection::FocusNavigationDirection_Up)
                                {
                                    targetIndex = firstVisibleIndex;
                                }
                                else if (gamepadDirection == xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down ||
                                         gamepadDirection == xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right)
                                {
                                    targetIndex = lastVisibleIndex;
                                }

                                if (targetIndex > 0)
                                {
                                    ctl::ComPtr<xaml::IDependencyObject> container;
                                    IFC_RETURN(ContainerFromIndex(targetIndex, &container));
                                    spCandidate = container.Cast<IInspectable>();
                                }
                            }
                        }
                    }
                }
            }
        }

        bool focusMoved = false;
        if (spCandidate)
        {
            ctl::ComPtr<ListViewBaseItem> spCandidateAsLVBI = spCandidate.AsOrNull<ListViewBaseItem>();
            spCurrentAsLVBI = spCandidateAsLVBI;
            // Is the candidate list item this listview's item. we could have nested listviews.
            const bool isCandidateNotOwnListItem = !spCandidateAsLVBI || (spCandidateAsLVBI->GetParentListView() != this);

            // candidate is a list item but it is not this listview's list item (or)
            // candidate is not a list item or header item
            if (isCandidateNotOwnListItem || (!spCandidateAsLVBI && !spCandidate.AsOrNull<ListViewBaseHeaderItem>()))
            {
                ctl::ComPtr<IInspectable> header;
                ctl::ComPtr<IInspectable> footer;

                // we got a candidate but it is not a ListViewBaseItem or a ListViewBaseHeaderItem
                // walk up the tree until the ListViewBase to see if the candidate is within an item
                // weird for auto focus to give something inside focus - but this is to catch that case.
                ctl::ComPtr<IDependencyObject> spCandidateAsDO = spCandidate.AsOrNull<IDependencyObject>();
                ctl::ComPtr<IDependencyObject> spCurrentItem = spCandidateAsDO;
                IFC_RETURN(get_Header(&header));
                IFC_RETURN(get_Footer(&footer));
                while (spCurrentItem)
                {
                    ctl::ComPtr<IInspectable> spCurrentItemAsI = spCurrentItem.Cast<IInspectable>();
                    if (ctl::are_equal(spCurrentItemAsI.Get(), header.Get()) || ctl::are_equal(spCurrentItemAsI.Get(), footer.Get()))
                    {
                        // if we reached the header or footer, set focus on the item
                        // since the ScrollViewer within the ListViewBase will not handle scrolling, ListViewBase
                        // will need to do this explicitly.
                        ctl::ComPtr<IUIElement> spCandidateAsUI = spCandidate.AsOrNull<UIElement>();
                        if (!spCandidateAsUI)
                        {
                            ctl::ComPtr<IDependencyObject> spParent;
                            IFC_RETURN(VisualTreeHelper::GetParentStatic(spCandidateAsDO.Get(), &spParent));
                            spCandidateAsUI = spParent.AsOrNull<UIElement>();
                        }

                        ASSERT(spCandidateAsUI);

                        if (spCandidateAsUI && m_tpScrollViewer)
                        {
                            bool overlapsViewport = FALSE;
                            BOOLEAN focusUpdated = FALSE;

                            ctl::ComPtr<IControl> svAsControl;

                            IFC_RETURN(m_tpScrollViewer.As(&svAsControl));
                            IFC_RETURN(svAsControl.Cast<Control>()->DoesElementOverlapViewport(spCandidateAsUI.Get(), overlapsViewport));

                            if (overlapsViewport)
                            {
                                IFC_RETURN(SetFocusedElement(spCandidateAsDO.Cast<DependencyObject>(), xaml::FocusState_Keyboard, TRUE /* animateBringIntoView */, &focusUpdated));
                                isHandled = focusUpdated;
                            }
                        }

                        break;
                    }

                    spCurrentAsLVBI = spCurrentItem.AsOrNull<ListViewBaseItem>();
                    bool isCurrentOwnListItem = false;
                    if (spCurrentAsLVBI)
                    {
                        ctl::ComPtr<ListViewBase> spParentListView = spCurrentAsLVBI->GetParentListView();
                        isCurrentOwnListItem = spParentListView && (spParentListView.Get() == this);

                        if (!isCurrentOwnListItem && spParentListView)
                        {
                            // we are going through a listview who is the parent of this item but it is not
                            // this listview. Make the parent listview bring it into view and set the correct
                            // focused item.
                            INT index;
                            IFC_RETURN(spParentListView->IndexFromContainer(spCurrentAsLVBI.Get(), &index));
                            if (index > -1)
                            {
                                IFC_RETURN(spParentListView->SetFocusedItem(index,
                                    TRUE /* shouldScrollIntoView*/,
                                    TRUE /* forcefocus*/,
                                    xaml::FocusState_Keyboard,
                                    TRUE /* animateIfBringIntoView */));
                                focusMoved = true;
                            }
                        }
                    }

                    if (isCurrentOwnListItem ||
                        spCurrentItem.AsOrNull<ListViewBaseHeaderItem>() ||
                        ctl::are_equal(spCurrentItem.Get(), this))
                    {
                        // we reached this listview's item (or) a GroupHeader (or) this listview.
                        break;
                    }

                    ctl::ComPtr<IDependencyObject> spNextItem;
                    IFC_RETURN(VisualTreeHelper::GetParentStatic(spCurrentItem.Get(), &spNextItem));
                    spCurrentItem = spNextItem;
                }
            }
        }

        if (spCurrentAsLVBI)
        {
            BOOLEAN hasFocus = FALSE;
            spCurrentAsLVBI->HasFocus(&hasFocus);

            if (!hasFocus || focusMoved)
            {
                // if the new candidate item is the currently focused item, it means we likely
                // have focus within the item and moved it to another element within the item. In that case don't handle
                // the key. This will let auto focus move focus within the item and keep
                // the currently focused list item the same. If the candidate item is not the currently
                // focused item, handle the key and set focus on the new candidate.
                IFC_RETURN(IndexFromContainer(spCurrentAsLVBI.Get(), &newFocusedIndex));
            }
        }

        if (newFocusedIndex != -1)
        {
            // we have a ListViewItem we want to set focus on
            if (!spCandidate.AsOrNull<ListViewBaseItem>())
            {
                // we are going to handle and set focus. If the candidate is not a listviewitem, then
                // lets set focus on the candidate explicitly so that focus does not go within the listviewitem
                // to the default element (1st focusable candidate)
                ctl::ComPtr<DependencyObject> spCandidateAsDO;
                BOOLEAN focusUpdated = FALSE;

                IFC_RETURN(spCandidate.As(&spCandidateAsDO));
                IFC_RETURN(SetFocusedElement(spCandidateAsDO.Get(),
                    xaml::FocusState_Keyboard,
                    true /* animateIfBringIntoView */,
                    &focusUpdated));
            }
        }
        else if (!isHandled && m_tpScrollViewer && !isCandidateAlreadyFocused)
        {
            // we did not find an item to scroll to, fallback to ScrollViewer's logic
            // to pick the next item and move to it. This will make sure that the ScrollViewer scrolls
            // or moves focus based on the focusable content in that direction if it is scrollable.
            if (spCandidate)
            {
                IFC_RETURN(m_tpScrollViewer.Cast<ScrollViewer>()->ProcessGamepadNavigation(key, originalKey, isHandled));
            }
            else
            {
                double viewportScrollPercent = is2DPageNavigation ? 1.0f : 0.5f;

                //If the Candidate returned was null, the list is likely blank, we need to process this key for Scroll
                IFC_RETURN(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollForFocusNavigation(key, gamepadDirection, viewportScrollPercent, &isHandled));
            }
        }
    }

    if (newFocusedIndex != -1)
    {
        // If a new item received focus, update the selection accordingly
        if (newFocusedType == xaml_controls::ElementType_ItemContainer)
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
            UINT itemCount = 0;

            m_itemIndexHintForHeaderNavigation = -1;

            isHandled = TRUE;

            // Coerce the new index under the upper bound of the ItemCollection
            IFC_RETURN(get_Items(&spItems));
            IFC_RETURN(spItems.Cast<ItemCollection>()->get_Size(&itemCount));
            newFocusedIndex = __min(newFocusedIndex, static_cast<INT>(itemCount) - 1);

            // TODO: We'll need to update the following behavior which moves
            // selection to be compliant with input gestures that only move focus
            // (we need to resolve the desired behavior of Shift+Arrows when
            // keyboard navigation only moves focus)
            if (newFocusedIndex >= 0)
            {
                IFC_RETURN(GetKeyboardModifiers(&modifiers));

                if (mode == xaml_controls::ListViewSelectionMode_Extended &&
                    IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift))
                {
                    // If the KeyboardModifier is the Shift key and SelectionMode is
                    // Extended, we need to reselect the range and set focus to the
                    // focused index (including bringing it into view if necessary).
                    // We clear the old selection unless the Control modifier is
                    // also pressed.
                    IFC_RETURN(MakeRangeSelection(
                        newFocusedIndex,
                        !IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Control)));
                }
                else if (mode == xaml_controls::ListViewSelectionMode_Multiple)
                {
                    // Holding shift key will do a range selection or deselection (depending on anchor item)
                    if (IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift))
                    {
                        IFC_RETURN(MakeRangeSelection(newFocusedIndex, /*clearOldSelection*/ FALSE));
                    }

                    IFC_RETURN(SetAnchorIndex(newFocusedIndex));
                }
                else if (IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Control))
                {
                    // If the KeyboardModifier is the Ctrl key, we just want to move the focus to the focused index
                    // (and bring it into view if necessary)
                }
                else
                {
                    // For all other cases, we select just this item and move focus
                    // to the focused index (and bring it into view if necessary)
                    if (mode != xaml_controls::ListViewSelectionMode_None)
                    {
                        BOOLEAN singleSelectionFollowsFocus = FALSE;
                        IFC_RETURN(get_SingleSelectionFollowsFocus(&singleSelectionFollowsFocus));

                        // If mode is single selection, select only if SingleSelectionFollowsFocus is true
                        if ((mode == xaml_controls::ListViewSelectionMode_Single && !!singleSelectionFollowsFocus) ||
                            mode != xaml_controls::ListViewSelectionMode_Single)
                        {
                            IFC_RETURN(MakeSingleSelection(newFocusedIndex, animateIfBringIntoView, NULL, gamepadDirection));
                        }
                    }
                }

                IFC_RETURN(SetFocusedItem(newFocusedIndex, TRUE /*shouldScrollIntoView*/, FALSE /*forceFocus*/, xaml::FocusState_Keyboard, animateIfBringIntoView, gamepadDirection));
            }
        }
        else
        {
            // Focus the group headers.
            ctl::ComPtr<xaml::IDependencyObject> groupHeader;
            IFC_RETURN(HeaderFromIndex(newFocusedIndex, &groupHeader));
            if (groupHeader)
            {
                IFC_RETURN(DependencyObject::SetFocusedElement(groupHeader.Cast<DependencyObject>(), xaml::FocusState_Keyboard, animateIfBringIntoView, &isHandled));
                if (isHandled && !wasGroupInitiallyFocused)
                {
                    // Only update the item index hint if we transitioned from an item to a group header.
                    // We need to preserve it while moving between group headers, which could happen
                    // if we are displaying group headers for empty groups.
                    m_itemIndexHintForHeaderNavigation = GetLastFocusedIndex();
                }
            }
            else
            {
                // If we got here, we should have a group header.
                ASSERT(FALSE);
            }
        }
    }

    if (isHandled)
    {
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    return S_OK;
}

// Handles when a key is released on the ListViewBase.
IFACEMETHODIMP ListViewBase::OnKeyUp(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: OnKeyUp.", ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this));
#endif

    HRESULT hr = S_OK;

    IFC(ListViewBaseGenerated::OnKeyUp(pArgs));
    if (IsInExclusiveInteraction())
    {
        // During an exclusive interaction (drag/drop), disable all keyboard interaction.
        // Handle the event, since other controls don't have the necessary context around drag/drop interactions.
        IFC(pArgs->put_Handled(TRUE));
    }
    else
    {
        KeyPress::EndFocusPress(checked_cast<CUIElement>(GetHandle()));
    }

Cleanup:
    RRETURN(hr);
}

// Handles a key press on a group header.
_Check_return_ HRESULT ListViewBase::OnGroupHeaderKeyDown(
    _In_opt_ IInspectable* pItem,
    _In_ wsy::VirtualKey originalKey,
    _In_ wsy::VirtualKey key,
    _Out_ BOOLEAN* pHandled)
{
    // Ignore already handled events
    if (IsInExclusiveInteraction())
    {
        // During an exclusive interaction (drag/drop), disable all keyboard interaction.
        // Handle the event, since other controls don't have the necessary context around drag/drop interactions.
        *pHandled = TRUE;
    }
    else
    {
        switch (key)
        {
            // Enter, Space (gamepad 'A' maps to space)
            case wsy::VirtualKey_Enter:
            case wsy::VirtualKey_Space:
                IFC_RETURN(ToggleSemanticZoomActiveView(pItem, pHandled));
                break;
        }
    }

    return S_OK;
}

// Handles a key press on a group header.
_Check_return_ HRESULT ListViewBase::OnGroupHeaderKeyUp(
    _In_opt_ IInspectable* pItem,
    _In_ wsy::VirtualKey originalKey,
    _In_ wsy::VirtualKey key,
    _Out_ BOOLEAN* pHandled)
{
    // Ignore already handled events
    if (IsInExclusiveInteraction())
    {
        // During an exclusive interaction (drag/drop), disable all  interaction.
        // Handle the event, since other controls don't have the necessary context around drag/drop interactions.
        *pHandled = TRUE;
    }
    else
    {
        switch (key)
        {
        // Enter, Space (gamepad 'A' maps to space)
        case wsy::VirtualKey_Enter:
        case wsy::VirtualKey_Space:
            IFC_RETURN(ToggleSemanticZoomActiveView(pItem, pHandled));
            break;
        }
    }

    return S_OK;
}

// Handles the tap gesture for a ListViewHeaderBaseItem
_Check_return_ HRESULT ListViewBase::OnHeaderItemTap(
    _In_opt_ IInspectable* pItem,
    _Out_ BOOLEAN* pIsHandled)
{
    IFC_RETURN(ToggleSemanticZoomActiveView(pItem, pIsHandled));

    return S_OK;
}

// Toggles the SemanticZoom active view
// Can be called either through KeyDown (Enter) or Tap
_Check_return_ HRESULT ListViewBase::ToggleSemanticZoomActiveView(
    _In_opt_ IInspectable* pItem,
    _Out_ BOOLEAN* pHandled)
{
    BOOLEAN canChangeViews = TRUE;
    ctl::ComPtr<ISemanticZoom> spSemanticZoom;

    IFCPTR_RETURN(pHandled);
    *pHandled = FALSE;

    // Get the SemanticZoom owner
    IFC_RETURN(get_SemanticZoomOwner(&spSemanticZoom));

    if (spSemanticZoom)
    {
        // If we cannot change views, doing a view change will
        // result in an IFCEXPECT - When doing keydown or Tap we
        // do not want to crash the app, so we just don't change views
        // in that case.
        IFC_RETURN(spSemanticZoom->get_CanChangeViews(&canChangeViews));
        if (canChangeViews)
        {
            BOOLEAN isZoomedInView = FALSE;
            IFC_RETURN(get_IsZoomedInView(&isZoomedInView));
            if (spSemanticZoom && isZoomedInView)
            {
                ctl::ComPtr<SemanticZoom> spSemanticZoomConcrete;

                SetPtrValue(m_tpSZRequestingItem, pItem);

                // Call ToggleActiveViewFromHeaderItem
                spSemanticZoomConcrete = spSemanticZoom.Cast<SemanticZoom>();
                IFC_RETURN(spSemanticZoomConcrete->ToggleActiveViewFromHeaderItem(pHandled));
            }
        }
    }

    return S_OK;
}

// Handles keyboard navigation on a group header.
_Check_return_ HRESULT ListViewBase::OnGroupKeyboardNavigation(
    _In_ wsy::VirtualKey key,
    _In_ wsy::VirtualKey originalKey,
    _In_ BOOLEAN animateIfBringIntoView,
    _Out_ BOOLEAN* pHandled)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spOuterPanel;
    xaml_controls::KeyNavigationAction action = xaml_controls::KeyNavigationAction_Up;
    bool isValidKey = false;
    ctl::ComPtr<IKeyboardHeaderNavigationPanel> spKeyboardHeaderNavigationPanel;
    INT groupCount = -1;

    *pHandled = FALSE;

    IFC(TranslateKeyToKeyNavigationAction(key, &action, &isValidKey));
    ASSERT(isValidKey);

    IFC(GetGroupCount(&groupCount));

    IFC(get_ItemsHost(&spOuterPanel));

    spKeyboardHeaderNavigationPanel = spOuterPanel.AsOrNull<IKeyboardHeaderNavigationPanel>();

    if (spKeyboardHeaderNavigationPanel)
    {
        UINT currentGroupIndex = static_cast<UINT>(GetFocusedGroupIndex());
        UINT newGroupIndex = 0;
        BOOLEAN isActionHandled = FALSE;
        ctl::ComPtr<IDependencyObject> spHeaderDO;

        for (INT groupsSeen = 0;
             groupsSeen < groupCount;
             ++groupsSeen)
        {
            IFC(spKeyboardHeaderNavigationPanel->GetTargetHeaderIndexFromNavigationAction(
                currentGroupIndex,
                action,
                &newGroupIndex,
                &isActionHandled));

            if (isActionHandled)
            {
                if (currentGroupIndex != newGroupIndex)
                {
                    // Handled, new index
                    BOOLEAN isFocusable = FALSE;

                    IFC(Selector::ScrollIntoView(
                        newGroupIndex,
                        TRUE  /*isGroupItemIndex*/,
                        FALSE /*isHeader*/,
                        FALSE /*isFooter*/,
                        FALSE /*isFromPublicAPI*/,
                        TRUE  /*ensureContainerRealized*/,
                        animateIfBringIntoView,
                        xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default,
                        0.0 /*offset*/,
                        GetFocusedGroupIndex() /*currentGroupIndex*/));

                    IFC(HeaderFromIndex(newGroupIndex, &spHeaderDO));

                    IFC(ItemsControl::IsFocusableHelper(
                        ctl::iinspectable_cast(spHeaderDO.Get()),
                        isFocusable));

                    if (isFocusable)
                    {
                        // This one is focusable.
                        break;
                    }
                    else
                    {
                        currentGroupIndex = newGroupIndex;
                        isActionHandled = FALSE;
                        spHeaderDO = nullptr;
                    }
                }
                else
                {
                    // Handled, same index -- maybe scrolling?  Leave.
                    break;
                }
            }
            else
            {
                // Not handled -- no need to continue.
                break;
            }
        }

        if (isActionHandled &&
            GetFocusedGroupIndex() != newGroupIndex)
        {
            ctl::ComPtr<xaml::IUIElement> spHeaderAsUIElement = spHeaderDO.AsOrNull<xaml::IUIElement>();
            if (spHeaderAsUIElement)
            {
                BOOLEAN isHandled = FALSE;

                IFC(spHeaderAsUIElement->Focus(xaml::FocusState_Keyboard, &isHandled));

                // Set focused group indices.  This will be set again in GroupHeaderItemFocused.
                SetFocusedGroupIndex(newGroupIndex);
                SetLastFocusedGroupIndex(newGroupIndex);
            }
        }

        *pHandled = isActionHandled;
    }
    else
    {
        // Keep asking the panel for groups to go to, until we find a group that has a
        // focusable header (or we run out of groups).
        do
        {
            INT newGroupIndex = -1;
            auto newElementType = xaml_controls::ElementType_GroupHeader;
            BOOLEAN useFallback = FALSE;
            BOOLEAN isHandled = FALSE;

            // Get the index of the destination group.
            IFC(QueryPanelForItemNavigation(
                    spOuterPanel.Get(),
                    action,
                    GetLastFocusedGroupIndex(), /*sourceFocusedIndex*/
                    xaml_controls::ElementType_GroupHeader,
                    [this](UINT groupIndex, xaml_controls::ElementType /*type*/, BOOLEAN* pIsFocusable) -> HRESULT
                    {
                        RRETURN(ListViewBase::IsGroupItemFocusable(this, groupIndex, pIsFocusable));
                    },
                    !XboxUtility::IsGamepadNavigationDirection(originalKey), /*allowWrap*/
                    -1, /*itemIndexHintForHeaderNavigation*/
                    &newGroupIndex,
                    &newElementType,
                    &isHandled,
                    &useFallback));

            ASSERT(newElementType == xaml_controls::ElementType_GroupHeader);

            if (!isHandled && useFallback)
            {
                // We've tried to use the outer panel for nav, but that has failed and
                // thus we need to do inter-group navigation ourselves.
                IFC(QueryGroupFallbackForGroupNavigation(
                    key,
                    GetLastFocusedGroupIndex(),
                    &newGroupIndex,
                    &isHandled));
            }

            if (isHandled)
            {
                ctl::ComPtr<xaml::IDependencyObject> spGroupItemAsIDO;
                ctl::ComPtr<IGroupItem> spGroupItem;

    #if DBG
                ASSERT(newGroupIndex < groupCount);
    #endif

                IFC(Selector::ScrollIntoView(
                    newGroupIndex,
                    TRUE  /*isGroupItemIndex*/,
                    FALSE /*isHeader*/,
                    FALSE /*isFooter*/,
                    FALSE /*isFromPublicAPI*/,
                    TRUE  /*ensureContainerRealized*/,
                    animateIfBringIntoView,
                    xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default,
                    0.0 /*offset*/,
                    GetFocusedGroupIndex() /*currentGroupIndex*/));
                IFC(HeaderFromIndex(newGroupIndex, &spGroupItemAsIDO));
                spGroupItem = spGroupItemAsIDO.AsOrNull<IGroupItem>();

                if (spGroupItem)
                {
                    IFC(spGroupItem.Cast<GroupItem>()->FocusHeader(xaml::FocusState_Keyboard, pHandled));
                }

                if (!spGroupItem || newGroupIndex == GetLastFocusedGroupIndex())
                {
                    // Safety check, shouldn't happen. If this is hit, we should break out because either the panel or the ICG is acting wonky.
                    // Note the key will still be handled if focus was taken above. Determining whether or not a key is handled
                    // is really up to the panel itself (and whether or not our header is focusable).

                    // The following 'impossible' conditions will trip us up:
                    // * We still dont' have a container after scrolling into view.
                    // * We get a non-IGroupItem as a group container.
                    // * Our panel gives us our current group index, but claims that it's given us
                    //   somewhere new to go (indicated by isHandled).

    #if DBG
                    ASSERT(spGroupItemAsIDO, L"Expected a container to be available after ScrollIntoView.");
                    ASSERT(spGroupItem, L"Expected an IGroupItem from ContainerFromGroupIndex.");
                    ASSERT(newGroupIndex != GetLastFocusedGroupIndex(), L"Panel handled key, but gave us an index that is the same as the current index.");
    #endif
                    break;
                }

                SetLastFocusedGroupIndex(newGroupIndex);
            }
            else
            {
                // Nobody handled the key. Nothing to do. Let other controls have the key.
                // We're querying outer panel, but it decided to do nothing.
                // As such, we need to call ElementScrollViewerScrollInDirection so we can scroll
                // any header into view..
                IFC(ElementScrollViewerScrollInDirection(key));
                break;
            }
        } while (!*pHandled);
    }

Cleanup:
    RRETURN(hr);
}

// Handles keyboard navigation for the ListViewBase.  By default this will refer to
// the ItemsHost Panel to provide layout specific knowledge of keyboard
// navigation, but also includes generic StackPanel-like behavior if ListViewBase is
// being used with an unsophisticated Panel implementation.
_Check_return_ HRESULT ListViewBase::OnKeyboardNavigation(
    _In_ wsy::VirtualKey key,
    _In_ wsy::VirtualKey originalKey,
    _In_ BOOLEAN animateIfBringIntoView,
    _Out_ INT* pNewFocusedIndex,
    _Out_ xaml_controls::ElementType* pNewFocusedType)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: OnKeyboardNavigation. key=%d, originalKey=%d, animateIfBringIntoView=%d", 
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, key, originalKey, animateIfBringIntoView));
#endif

    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    ctl::ComPtr<IPanel> spTargetPanel;
    ctl::ComPtr<IPanel> spItemsHost;
    INT sourceIndexWithinGroup = -1;
    INT sourceGroupIndex = -1;
    INT newFocusedIndex = -1;
    auto newFocusedType = xaml_controls::ElementType_ItemContainer;
    BOOLEAN considerNestedPanels = FALSE;
    BOOLEAN isQueryingOuterPanel = FALSE;
    BOOLEAN doForceUseOuterPanel = FALSE;
    BOOLEAN allowPanelWrap = TRUE;
    xaml_controls::Orientation outerLogicalOrientation = xaml_controls::Orientation_Vertical;
    xaml_controls::KeyNavigationAction action = xaml_controls::KeyNavigationAction_Up;
    ctl::ComPtr<IGroupItem> spSourceGroupItem;
    ctl::ComPtr<IItemContainerMapping> spMapping;
    ctl::ComPtr<IGroupHeaderMapping> spGroupMapping;
    BOOLEAN isNavDirectionParallelToInnerOrientation = FALSE;
    BOOLEAN isGrouping = FALSE;

    bool isValidKey = false;

    IFCPTR(pNewFocusedIndex);
    IFCPTR(pNewFocusedType);

    *pNewFocusedIndex = -1;

    *pNewFocusedType = xaml_controls::ElementType_ItemContainer;

    IFC(get_ItemsHost(&spItemsHost));

    if (!spItemsHost)
    {
        goto Cleanup;
    }

    IFC(get_IsGrouping(&isGrouping));
    if (isGrouping)
    {
        // Always use outer if panel is a IModernCollectionBasePanel. It handles everything. Otherwise,
        // deal with nested panels here.
        if (!ctl::is<IModernCollectionBasePanel>(spItemsHost))
        {
            considerNestedPanels = TRUE;
        }
    }

    IFC(TranslateKeyToKeyNavigationAction(key, &action, &isValidKey));
    ASSERT(isValidKey);

    IFC(GetItemContainerMapping(&spMapping));
    IFC(GetGroupHeaderMapping(&spGroupMapping));

    IFC(ShouldNavDirectionIgnoreGroups(action, &doForceUseOuterPanel));
    IFC(GetPanelOrientations(spItemsHost.Get(), NULL /*pPhysicalOrientation*/, &outerLogicalOrientation));

    if (!considerNestedPanels || doForceUseOuterPanel)
    {
        // If we are not grouping, nav within the main panel.
        isQueryingOuterPanel = TRUE;
        spTargetPanel = spItemsHost;
    }
    else
    {
        // Otherwise, nav within the group's panel, but only if this isn't
        // a global navigation key (Home, End, etc).
        ctl::ComPtr<ICollectionViewGroup> spCVGroup;
        ctl::ComPtr<IItemsControl> spGroupItemsControl;

        isQueryingOuterPanel = TRUE; // Default to outer panel in case we don't find an inner panel.

        IFC(GetGroupFromItem(GetLastFocusedIndex(), &spCVGroup, &spSourceGroupItem, &sourceGroupIndex, &sourceIndexWithinGroup));

        if (spSourceGroupItem)
        {
            IFC(spSourceGroupItem.Cast<GroupItem>()->GetTemplatedItemsControl(&spGroupItemsControl));
            if (spGroupItemsControl)
            {
                IFC(spGroupItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spTargetPanel));
                isQueryingOuterPanel = FALSE;
            }
        }
    }

    if (!isQueryingOuterPanel)
    {
        // Only allow wrapping for VSPs and SPs as the outer panel, if they match in orientation with the inner panel.
        allowPanelWrap = FALSE;
        if (ctl::is<IStackPanel>(spItemsHost.Get()) || ctl::is<IVirtualizingStackPanel>(spItemsHost.Get()))
        {
            xaml_controls::Orientation innerLogicalOrientation = xaml_controls::Orientation_Vertical;

            IFC(GetPanelOrientations(spTargetPanel.Get(), NULL /*pPhysicalOrientation*/, &innerLogicalOrientation));
            isNavDirectionParallelToInnerOrientation = (innerLogicalOrientation == xaml_controls::Orientation_Vertical) ?
                (action == xaml_controls::KeyNavigationAction_Up) || (action == xaml_controls::KeyNavigationAction_Down) :
                (action == xaml_controls::KeyNavigationAction_Left) || (action == xaml_controls::KeyNavigationAction_Right);
            allowPanelWrap = (innerLogicalOrientation != outerLogicalOrientation);
        }
    }

    if (spTargetPanel)
    {
        BOOLEAN useFallback = FALSE;
        if (isQueryingOuterPanel && considerNestedPanels)
        {
            // If we're grouping, but we're still querying the outer panel, this means that either we're processing a key where
            // ShouldNavDirectionIgnoreGroups returns true, or we couldn't find a group panel. In that case, we need to use
            // the fallback logic, since the outer panel does not have the item-specific context it needs (at most, it could
            // give us which group to go to, but not which specific item to go to). The fallback code is panel-agnostic
            // and will handle this case properly.
            useFallback = TRUE;
            isHandled = FALSE;
        }
        else
        {
            BOOLEAN isFocusOnHeaderOrFooter = FALSE;
            const bool isGamepadPageNavigation = XboxUtility::IsGamepadPageNavigationDirection(originalKey);

            ctl::ComPtr<IItemsPresenter> itemsPresenter;
            IFC(get_ItemsPresenter(&itemsPresenter));
            if (itemsPresenter)
            {
                ctl::ComPtr<ContentControl> container;
                IFC(itemsPresenter.Cast<ItemsPresenter>()->get_HeaderContainer(&container));
                if (container)
                {
                    IFC(container.Cast<ContentControl>()->HasFocus(&isFocusOnHeaderOrFooter));
                }

                if (!isFocusOnHeaderOrFooter)
                {
                    IFC(itemsPresenter.Cast<ItemsPresenter>()->get_FooterContainer(&container));
                    if (container)
                    {
                        IFC(container.Cast<ContentControl>()->HasFocus(&isFocusOnHeaderOrFooter));
                    }
                }
            }

            // when using gamepad or if focus is on header or footer, don't ask the panel,
            // use ScrollViewer's logic instead
            if (!isGamepadPageNavigation && !isFocusOnHeaderOrFooter)
            {
                const UINT lastFocusedIndex = GetLastFocusedIndex();
                UINT panelStartIndexInCollection = (isQueryingOuterPanel) ? 0 : lastFocusedIndex - sourceIndexWithinGroup;
                UINT startIndex = (isQueryingOuterPanel) ? lastFocusedIndex : sourceIndexWithinGroup;
                auto startType = xaml_controls::ElementType_ItemContainer;

                // Only allow focusing of the group header if the items panel is an IModernCollectionBasePanel, which
                // we can test for by validating that considerNestedPanels is false.
                if (!considerNestedPanels && HasFocusedGroup())
                {
                    startIndex = GetFocusedGroupIndex();
                    startType = xaml_controls::ElementType_GroupHeader;
                }

                // First, ask the panel what to do with our key.
                IFC(QueryPanelForItemNavigation(
                    spTargetPanel.Get(),
                    action,
                    startIndex,
                    startType,
                    [&panelStartIndexInCollection, &spMapping, &spGroupMapping](UINT index, xaml_controls::ElementType type, BOOLEAN* pIsFocusable) -> HRESULT
                    {
                        HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
                        *pIsFocusable = FALSE;
                        if (type == xaml_controls::ElementType_GroupHeader)
                        {
                            // Since group headers are treated the same as items, use the same selector logic for
                            // determining whether a header is selectable/focusable.
                            ctl::ComPtr<IDependencyObject> groupItemDO;

                            // Here the index is just the GroupIndex and we don't need to translate its values depending
                            // on which panel we're querying, like we do for items.
                            IFC(spGroupMapping->HeaderFromIndex(index, &groupItemDO));
                            IFC(ItemsControl::IsFocusableHelper(ctl::iinspectable_cast(groupItemDO.Get()), *pIsFocusable));
                        }
                        else
                        {
                            IFC(ListViewBase::IsItemFocusable(spMapping.Get(), panelStartIndexInCollection + index, pIsFocusable));
                        }

                    Cleanup:
                        return S_OK;
                    },
                    allowPanelWrap && !XboxUtility::IsGamepadNavigationDirection(originalKey),
                    m_itemIndexHintForHeaderNavigation,
                    &newFocusedIndex,
                    &newFocusedType,
                    &isHandled,
                    &useFallback));
            }
        }

        if (!isQueryingOuterPanel && isHandled)
        {
            // If the new focus element is an item rather than a group, then translate its focused index
            // (which is relative to group panel) to collection index.
            if (newFocusedType == xaml_controls::ElementType_ItemContainer)
            {
                INT indexDelta = newFocusedIndex - sourceIndexWithinGroup;
                newFocusedIndex = GetLastFocusedIndex() + indexDelta;
            }
        }

        if (isQueryingOuterPanel && !isHandled && !useFallback)
        {
            // We're querying the outer panel, but it decided to do nothing.
            // if it was a gamepad key,
            //  - we delegate to the ScrollViewer to move focus within the header/footer
            // if it is not a gamepad key,
            //  - we need to call ElementScrollViewerScrollInDirection so we can scroll
            //    any header into view. QuerySelectorFallbackForItemNavigation does this for us,
            //    so we're set for that case.
            if (!XboxUtility::IsGamepadPageNavigationDirection(originalKey))
            {
                IFC(ElementScrollViewerScrollInDirection(key, animateIfBringIntoView));
            }
        }

        // Panel wants us to deal with the navigation key, as it doesn't implement its own key nav for this case.
        if (useFallback)
        {
            if (!considerNestedPanels || doForceUseOuterPanel)
            {
                // We're safe, just fall back.
                IFC(QuerySelectorFallbackForItemNavigation(key, GetLastFocusedIndex(), &newFocusedIndex, &isHandled));
            }
            else
            {
                // Selector doesn't support group keyboard navigation. This must be handled in tricky ways.
                // Invoke our own group-aware fallback.
                UINT groupLeftIndex = 0;
                UINT groupSize = 0;
                BOOLEAN foundGroup = FALSE;

                ASSERT(considerNestedPanels);

                IFC(GetGroupInformation(
                    sourceGroupIndex,
                    &groupLeftIndex,
                    &groupSize,
                    &foundGroup));

                ASSERT(foundGroup);

                if (foundGroup)
                {
                    IFC(QueryGroupFallbackForItemNavigation(
                        spTargetPanel.Get(),
                        key,
                        GetLastFocusedIndex(),
                        groupLeftIndex,
                        groupLeftIndex + groupSize,
                        &newFocusedIndex,
                        &isHandled));
                }
            }
        }
    }

    if (isHandled)
    {
        // We're done.
        *pNewFocusedIndex = newFocusedIndex;
        *pNewFocusedType = newFocusedType;
    }
    else if (allowPanelWrap && !isQueryingOuterPanel && isNavDirectionParallelToInnerOrientation)
    {
        // Special case. We're grouping, outer is VSP or SP, inner and outer orientations don't match.
        // We're also at a group boundary, and the navigation action would result in wrapping
        // in the inner panel (if there were more items - remember, we're at an edge).
        const BOOLEAN doIncreaseIndex = (action == xaml_controls::KeyNavigationAction_Right) || (action == xaml_controls::KeyNavigationAction_Down);
        const UINT lastFocusedIndex = GetLastFocusedIndex();

        if (doIncreaseIndex)
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
            UINT itemCount = 0;

            // Coerce the new index under the upper bound of the ItemCollection
            IFC(get_Items(&spItems));
            IFC(spItems.Cast<ItemCollection>()->get_Size(&itemCount));
            if (lastFocusedIndex <= itemCount - 2)
            {
                *pNewFocusedIndex = lastFocusedIndex + 1;
            }
        }
        else if (lastFocusedIndex > 0)
        {
            *pNewFocusedIndex = lastFocusedIndex - 1;
        }
    }
    else if (!isQueryingOuterPanel)
    {
        // Our group's panel didn't think it was time to handle a key press (reached an edge).
        // We need to do navigation between groups now.
        BOOLEAN foundSelectableItem = FALSE;
        INT groupCount = -1;
        IFC(GetGroupCount(&groupCount));

        // Keep asking the relevant panels for groups to go to, until we find a group that has a
        // selectable item (or we run out of groups).
        do
        {
            INT newGroupIndex = -1;
            auto newElementType = xaml_controls::ElementType_GroupHeader;
            BOOLEAN useFallback = FALSE;

            // Get the index of the destination group.
            IFC(QueryPanelForItemNavigation(
                spItemsHost.Get(),
                action,
                sourceGroupIndex, /*sourceFocusedIndex*/
                xaml_controls::ElementType_GroupHeader,
                [this](UINT groupIndex, xaml_controls::ElementType type, BOOLEAN* pIsFocusable) -> HRESULT
                {
                    RRETURN(ListViewBase::IsGroupItemFocusable(this, groupIndex, pIsFocusable));
                },
                !XboxUtility::IsGamepadNavigationDirection(originalKey), /*allowWrap*/
                -1, /*itemIndexHintForHeaderNavigation*/
                &newGroupIndex,
                &newElementType,
                &isHandled,
                &useFallback));

            // If the navigation was handled, then the item type we get back is expected to be a container.
            ASSERT(!isHandled || newElementType == xaml_controls::ElementType_ItemContainer);

            if (!isHandled)
            {
                if (useFallback)
                {
                    // At this point, our group panel has not handled the navigation at all.
                    // We've tried to use the outer panel for nav, but that has failed and
                    // thus we need to do inter-group navigation ourselves.
                    IFC(QueryGroupFallbackForGroupNavigation(
                        key,
                        sourceGroupIndex,
                        &newGroupIndex,
                        &isHandled));
                }
            }

            if (isHandled)
            {
                //Figure out which item to go to within new group.

                INT targetItemIndexInCollection = -1;
                BOOLEAN wentForward = newGroupIndex > sourceGroupIndex;
                BOOLEAN useFallback2 = TRUE;
                INT fallbackSearchStartIndex = -1; /* default to beginning/end of group.*/

                // We only support smart navigation between groups if the outer panel NOT a VariableSizedWrapGrid. See design notes attached to Win8: 398951 for details.
                if (!ctl::is<IVariableSizedWrapGrid>(spItemsHost.Get()))
                {
                    BOOLEAN foundItem = FALSE;
                    INT closestIndex = -1;
                    // Smart navigation.
                    IFC(GetClosestIndexToItemInGroupPanel(
                        GetLastFocusedIndex(),
                        newGroupIndex,
                        action,
                        spSourceGroupItem.Get(),
                        &closestIndex,
                        &foundItem,
                        &useFallback2));

                    foundSelectableItem = foundItem && !useFallback2;
                    if (foundItem && useFallback2)
                    {
                        // Panel found the item, but it wasn't Focusable. We need to use the fallback, starting
                        // from the (unfocusable) index found.
                        fallbackSearchStartIndex = closestIndex;
                    }
                    else if (foundSelectableItem)
                    {
                        // No need to do anything special. We'll use this value if !useFallback2.
                        targetItemIndexInCollection = closestIndex;
                    }
                }

                if (useFallback2)
                {
                    // Fallback navigation - go to first or last item, depending on action.
                    // If GetClosestIndexToItemInGroupPanel found an unfocusable item, start the search
                    // from that item.
                    IFC(FindFirstSelectableItemInGroup(
                        newGroupIndex,
                        wentForward,
                        fallbackSearchStartIndex,
                        &targetItemIndexInCollection,
                        &foundSelectableItem));
                }

                if (foundSelectableItem)
                {
                    *pNewFocusedIndex = targetItemIndexInCollection;
                    SetFocusedGroupIndex(newGroupIndex);
                    SetLastFocusedGroupIndex(newGroupIndex);
                }
                else
                {
                    // Continue with the next group, if possible.
                    sourceGroupIndex = newGroupIndex;
                    if (
                        (wentForward && sourceGroupIndex >= groupCount) ||
                        (!wentForward && sourceGroupIndex <= 0) )
                    {
                        break;
                    }
                }
            }
            else
            {
                // Nobody handled the key. Nothing to do. Let other controls have the key.
                // We're querying outer panel, but it decided to do nothing.
                if (!XboxUtility::IsGamepadPageNavigationDirection(originalKey))
                {
                    // we need to call ElementScrollViewerScrollInDirection so we can scroll
                    // any header into view..
                    IFC(ElementScrollViewerScrollInDirection(key));
                }
                break;
            }
        } while (!foundSelectableItem);
    }

Cleanup:
    RRETURN(hr);
}

// Given a source item and target group and a navigation action,
// find which item in the target group is closest to the source item.
// "Closest" means: closest to the point returned by
// GetNavigationSourcePoint(source item).
// Also indicates whether or not an item was found,
// and whether or not to default to the fallback implementation
// (FindFirstSelectableItemInGroup).
_Check_return_ HRESULT ListViewBase::GetClosestIndexToItemInGroupPanel(
    _In_ UINT sourceIndexInCollection,
    _In_ UINT targetGroupIndex,
    _In_ xaml_controls::KeyNavigationAction action,
    _In_ IGroupItem* pSourceGroupItem,
    _Out_ INT* pClosestIndex,
    _Out_ BOOLEAN* pFoundItem,
    _Out_ BOOLEAN* pUseFallback)
{
    HRESULT hr = S_OK;

    UINT targetGroupStartIndex = 0;
    UINT targetGroupSize = 0;
    BOOLEAN isGroupValid = FALSE;

    *pUseFallback = TRUE;
    *pFoundItem = FALSE;
    *pClosestIndex = -1;

    IFC(GetGroupInformation(
        targetGroupIndex,
        &targetGroupStartIndex,
        &targetGroupSize,
        &isGroupValid));
    IFCEXPECT(isGroupValid);

    if (targetGroupSize > 0)
    {
        ctl::ComPtr<ICollectionViewGroup> spTargetCVGroup;
        ctl::ComPtr<IGroupItem> spTargetGroupItem;
        ctl::ComPtr<IItemsControl> spTargetGroupItemsControl;
        ctl::ComPtr<IPanel> spTargetGroupPanel;

        IFC(GetGroupFromItem(targetGroupStartIndex, &spTargetCVGroup, &spTargetGroupItem, NULL, NULL));

        // Try to get the Panel.
        if (spTargetGroupItem)
        {
            IFC(spTargetGroupItem.Cast<GroupItem>()->GetTemplatedItemsControl(&spTargetGroupItemsControl));
            if (spTargetGroupItemsControl)
            {
                IFC(spTargetGroupItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spTargetGroupPanel));
            }
        }

        if (spTargetGroupPanel)
        {
            // Get the source item.
            ctl::ComPtr<xaml::IDependencyObject> spSourceContainer;
            ctl::ComPtr<IFrameworkElement> spSourceContainerAsFE;
            ctl::ComPtr<IItemContainerMapping> spMapping;

            IFC(GetItemContainerMapping(&spMapping));
            IFC(ContainerFromIndex(sourceIndexInCollection, &spSourceContainer));
            spSourceContainerAsFE = spSourceContainer.AsOrNull<IFrameworkElement>();
            if (spSourceContainerAsFE)
            {
                wf::Point sourcePointRelativeToSourceItem = {0,0};
                wf::Point sourcePointRelativeToTargetPanel = {0,0};
                ctl::ComPtr<IGeneralTransform> spItemToTargetPanel;
                ctl::ComPtr<IItemLookupPanel> spTargetGroupPanelAsILP;
                xaml_primitives::ElementInfo targetElementInfoInTargetPanel = {-1, FALSE};
                INT targetIndexInCollection = -1;
                BOOLEAN isTargetFocusable = FALSE;

                // Get the source point relative to the source item.
                IFC(GetNavigationSourcePoint(spSourceContainerAsFE.Cast<FrameworkElement>(), action, &sourcePointRelativeToSourceItem));

                // We could just transform directly to the target panel here. Unfortunately, consider the case of
                // the outer panel limiting the size of our source GroupItem - say, in the case of a WrapGrid.
                // Then, our source panel could extend underneath our target panel! This will usually cause our GetClosestElementInfo
                // call below to return odd (but technically correct) indexes, since we could query for a point
                // that's in the middle of our target panel. So, we must clip our source point to the bounds
                // of the source GroupItem.
                IFC(static_cast<GroupItem*>(pSourceGroupItem)->ClipPointToElementBounds(
                    spSourceContainer.Cast<FrameworkElement>(),
                    &sourcePointRelativeToSourceItem));

                // Get the source point relative to the target panel.
                IFC(spSourceContainerAsFE.Cast<FrameworkElement>()->TransformToVisual(spTargetGroupPanel.Cast<Panel>(), &spItemToTargetPanel));
                IFC(spItemToTargetPanel->TransformPoint(sourcePointRelativeToSourceItem, &sourcePointRelativeToTargetPanel));

                // Calculate the closest index in the target panel, using IItemLookupPanel or the fallback method.
                spTargetGroupPanelAsILP = spTargetGroupPanel.AsOrNull<IItemLookupPanel>();
                if (spTargetGroupPanelAsILP)
                {
                    IFC(spTargetGroupPanelAsILP->GetClosestElementInfo(sourcePointRelativeToTargetPanel, &targetElementInfoInTargetPanel));
                }
                else
                {
                    // fallback to a naive and slow (full iteration) implementation of all the
                    // arranged children
                    XPOINTF p = {sourcePointRelativeToTargetPanel.X, sourcePointRelativeToTargetPanel.Y};
                    IFC(CoreImports::CPanel_PanelGetClosestIndexSlow(static_cast<CPanel*>(spTargetGroupPanel.Cast<Panel>()->GetHandle()), p, &targetElementInfoInTargetPanel.m_childIndex));
                }

                // We have a target index in the new group's panel. Convert it to an index in the full collection.
                targetIndexInCollection = targetElementInfoInTargetPanel.m_childIndex + targetGroupStartIndex;

                // See if the target is selectable.
                IFC(ListViewBase::IsItemFocusable(spMapping.Get(), targetIndexInCollection, &isTargetFocusable))

                if (isTargetFocusable)
                {
                    *pUseFallback = FALSE;
                }
                else
                {
                    // The closest item was unselectable. We need to use the fallback.
                    // Ideally we'd have some sort of GetClosestSelectableIndex, but
                    // we decided to punt that for now.
                    *pUseFallback = TRUE;
                }

                // We found the item either way.
                *pClosestIndex = targetIndexInCollection;
                *pFoundItem = TRUE;
            }
        }
    }
    else
    {
        // Group was empty.
        *pUseFallback = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

// Given an element and a navigation action, return the point
// that should be considered the "source" of the navigation.
_Check_return_ HRESULT ListViewBase::GetNavigationSourcePoint (
    _In_ FrameworkElement* pElementNavigatedFrom,
    _In_ xaml_controls::KeyNavigationAction action,
    _Out_ wf::Point* pPoint)
{
    HRESULT hr = S_OK;
    DOUBLE sourceHeight = 0;
    DOUBLE sourceWidth = 0;

    IFC(pElementNavigatedFrom->get_ActualHeight(&sourceHeight));
    IFC(pElementNavigatedFrom->get_ActualWidth(&sourceWidth));

    switch (action)
    {
        case xaml_controls::KeyNavigationAction_Right:
        {
            pPoint->X = static_cast<FLOAT>(sourceWidth);
            pPoint->Y = static_cast<FLOAT>(sourceHeight / 2.0f);
            break;
        }
        case xaml_controls::KeyNavigationAction_Left:
        {
            pPoint->X = 0;
            pPoint->Y = static_cast<FLOAT>(sourceHeight / 2.0f);
            break;
        }
        case xaml_controls::KeyNavigationAction_Down:
        {
            pPoint->X = static_cast<FLOAT>(sourceWidth / 2.0f);
            pPoint->Y = static_cast<FLOAT>(sourceHeight);
            break;
        }
        case xaml_controls::KeyNavigationAction_Up:
        {
            pPoint->X = static_cast<FLOAT>(sourceWidth / 2.0f);
            pPoint->Y = 0;
            break;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Gets the first selectable item within the given group.
// Can start from the end or the beginning of the group.
_Check_return_ HRESULT ListViewBase::FindFirstSelectableItemInGroup(
    _In_ UINT groupIndex,
    _In_ BOOLEAN fromBeginning,
    _In_ INT startIndexInCollection,
    _Out_ INT* pFirstSelectableItemIndexInCollection,
    _Out_ BOOLEAN* pFoundSelectableItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IItemContainerMapping> spMapping;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    UINT nCount = 0;
    UINT groupStartIndex = 0;
    UINT groupSize = 0;
    BOOLEAN isGroupValid = FALSE;
    BOOLEAN isFocusable = FALSE;
    INT startIndex = -1;

    *pFoundSelectableItem = FALSE;
    *pFirstSelectableItemIndexInCollection = -1;

    IFC(GetGroupInformation(
        groupIndex,
        &groupStartIndex,
        &groupSize,
        &isGroupValid));
    IFCEXPECT(isGroupValid);

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    if (startIndexInCollection >= 0)
    {
        startIndex = startIndexInCollection;
    }
    else
    {
        startIndex = groupStartIndex;
        if (!fromBeginning)
        {
            startIndex += groupSize - 1;
        }
    }

    for (INT currentItemIndex = startIndex;
        currentItemIndex < (INT)nCount && currentItemIndex >= static_cast<INT>(groupStartIndex) && currentItemIndex < (INT)(groupStartIndex+groupSize) && currentItemIndex >= 0;
        currentItemIndex += (fromBeginning? 1 : -1) )
    {
        ctl::ComPtr<IInspectable> spItem = nullptr;
        IFC(spItems.Cast<ItemCollection>()->GetAt(currentItemIndex, &spItem));
        IFC(ItemsControl::IsFocusableHelper(spItem.Get(), isFocusable));
        if (isFocusable)
        {
            if (!spMapping)
            {
                IFC(GetItemContainerMapping(&spMapping));
            }

            IFC(ListViewBase::IsItemFocusable(spMapping.Get(), currentItemIndex, &isFocusable));
            if (isFocusable)
            {
                *pFoundSelectableItem = TRUE;
                *pFirstSelectableItemIndexInCollection = currentItemIndex;
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Fetches group details, given a group index.
_Check_return_ HRESULT ListViewBase::GetGroupInformation(
    _In_ UINT groupIndex,
    _Out_ UINT* pGroupStartIndex,
    _Out_ UINT* pGroupSize,
    _Out_ BOOLEAN* pIsValid)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICollectionView> spCollectionView;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCollectionGroups;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spCollectionGroupsAsV;
    UINT groupCount = 0;

    *pGroupStartIndex = 0;
    *pGroupSize = 0;
    *pIsValid = FALSE;

    IFC(get_CollectionView(&spCollectionView));
    IFC(spCollectionView->get_CollectionGroups(&spCollectionGroups));

    IFC(spCollectionGroups.As<wfc::IVector<IInspectable*>>(&spCollectionGroupsAsV));
    IFC(spCollectionGroupsAsV->get_Size(&groupCount));

    if (groupCount > groupIndex)
    {
        UINT i = 0;

        for (i = 0; i <= groupIndex && i < groupCount; i++)
        {
            ctl::ComPtr<IInspectable> spCurrent;
            ctl::ComPtr<ICollectionViewGroup> spCurrentGroup;
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCurrentGroupItems;
            ctl::ComPtr<wfc::IVector<IInspectable*>> spCurrentGroupItemsAsV;

            IFC(spCollectionGroupsAsV->GetAt(i, &spCurrent));
            IFC(spCurrent.As<ICollectionViewGroup>(&spCurrentGroup));
            IFC(spCurrentGroup->get_GroupItems(&spCurrentGroupItems));
            IFC(spCurrentGroupItems.As<wfc::IVector<IInspectable*>>(&spCurrentGroupItemsAsV));
            IFC(spCurrentGroupItemsAsV->get_Size(pGroupSize));

            if (i != groupIndex)
            {
                *pGroupStartIndex += *pGroupSize;
            }
        }
        *pIsValid = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

// Given a key, returns whether or not we must handle the nav action at the outer panel.
// This is true for keys such as home or end - they should ignore groups.
_Check_return_ HRESULT ListViewBase::ShouldNavDirectionIgnoreGroups(
    xaml_controls::KeyNavigationAction action,
    _Out_ BOOLEAN* pIsGlobal)
{
    *pIsGlobal =
        (action == xaml_controls::KeyNavigationAction_Next) ||
        (action == xaml_controls::KeyNavigationAction_Previous) ||
        (action == xaml_controls::KeyNavigationAction_First) ||
        (action == xaml_controls::KeyNavigationAction_Last);

    RRETURN(S_OK);
}

// Given a key, returns a focus navigation action.
_Check_return_ HRESULT ListViewBase::TranslateKeyToKeyNavigationAction(
    _In_ wsy::VirtualKey key,
    _Out_ xaml_controls::KeyNavigationAction* pNavAction,
    _Out_ bool* pIsValidKey)
{
    HRESULT hr = S_OK;
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;

    *pNavAction = xaml_controls::KeyNavigationAction_Up;
    *pIsValidKey = false;
    IFC(get_FlowDirection(&flowDirection));

    KeyboardNavigation::TranslateKeyToKeyNavigationAction(flowDirection, key, pNavAction, pIsValidKey);

Cleanup:
    RRETURN(hr);
}

// Asks the given Panel what to do with a navigation action,
// given a starting index. Returns the destination index,
// whether or not the Panel handled the key, and whether
// or not the panel needs us to fall back to the default
// Selector behavior.
_Check_return_ HRESULT ListViewBase::QueryPanelForItemNavigation(
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
    _Out_ BOOLEAN* pUseFallback)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: QueryPanelForItemNavigation.", ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this));
#endif

    BOOLEAN isHandled = FALSE;
    ctl::ComPtr<IKeyboardNavigationPanel> spKeyNavPanel;

    *pNewFocusedIndex = -1;
    *pIsHandled = FALSE;
    *pUseFallback = FALSE;

    if (SUCCEEDED(ctl::do_query_interface(spKeyNavPanel, pTargetPanel)))
    {
        UINT newFocusedIndexUint = 0;
        auto newFocusedType = xaml_controls::ElementType_ItemContainer;
        BOOLEAN actionValidForSourceIndex = FALSE;
        BOOLEAN supportsAction = FALSE;

        IFC_RETURN(spKeyNavPanel->SupportsKeyNavigationAction(
                action,
                &supportsAction));

        if (supportsAction)
        {
            bool queryPanel = true;
            do
            {
                if (queryPanel)
                {
                    IFC_RETURN(spKeyNavPanel->GetTargetIndexFromNavigationAction(
                        sourceFocusedIndex,
                        sourceFocusedType,
                        action,
                        allowWrap,
                        itemIndexHintForHeaderNavigation,
                        &newFocusedIndexUint,
                        &newFocusedType,
                        &actionValidForSourceIndex));
                }

                isHandled = actionValidForSourceIndex;
                if (actionValidForSourceIndex)
                {
                    BOOLEAN isEligible = FALSE;
                    IFC_RETURN(verificationFunction(newFocusedIndexUint, newFocusedType, &isEligible));
                    if (isEligible)
                    {
                        *pNewFocusedIndex = newFocusedIndexUint;
                        *pNewFocusedType = newFocusedType;
                        break;
                    }
                    else if (sourceFocusedIndex == newFocusedIndexUint && sourceFocusedType == newFocusedType)
                    {
                        // Safety.
                        break;
                    }
                    else
                    {
                        sourceFocusedType = newFocusedType;

                        if (action == xaml_controls::KeyNavigationAction_First)
                        {
                            // We wanted the first item, but the one we got from the panel was not eligible.
                            // stop querying the panel and walk through the indicies to get to the next eligible item
                            INT itemCount;
                            IFC_RETURN(GetItemsCount(&itemCount));
                            if (static_cast<INT>(newFocusedIndexUint) < itemCount -1)
                            {
                                ++newFocusedIndexUint;
                                queryPanel = false;
                            }
                            else
                            {
                                // reached the end
                                break;
                            }
                        }
                        else if (action == xaml_controls::KeyNavigationAction_Last)
                        {
                            // We wanted the last item, but the one we got from the panel was not eligible.
                            // stop querying the panel and walk through the indicies to get to the previous eligible item
                            if (newFocusedIndexUint > 0)
                            {
                                --newFocusedIndexUint;
                                queryPanel = false;
                            }
                            else
                            {
                                // reached the beginning.
                                break;
                            }
                        }
                        else
                        {
                            // Proceed in the same direction.
                            sourceFocusedIndex = newFocusedIndexUint;
                        }
                    }
                }
                else
                {
                    // We ran out of items.
                    break;
                }
            } while (true);
        }
        else
        {
            // Panel doesn't support the key. Fall back.
            *pUseFallback = TRUE;
        }
    }
    else
    {
        *pUseFallback = TRUE;
    }

    *pIsHandled = isHandled;
    return S_OK;
}


// Asks the default Selector key navigation logic what to do
// with the given navigation key. Never call this if we're
// grouping.
// This should really be combined with QueryPanelForItemNavigation,
// but Selector's logic is too convoluted to justify messing with it.
// Better to encapsulate Selector's inability to handle group navigation
// into this function and QueryGroupFallbackForItemNavigation.
_Check_return_ HRESULT ListViewBase::QuerySelectorFallbackForItemNavigation(
    _In_ wsy::VirtualKey key,
    _In_ INT sourceFocusedIndex,
    _Out_ INT* pNewFocusedIndex,
    _Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;

    // Store sourceFocusedIndex in newFocusedIndex. HandleNavigationKey uses this
    // as an _Inout_ parameter.
    INT newFocusedIndex = sourceFocusedIndex;

    *pIsHandled = FALSE;

    // Forward along to Selector's generic keyboard navigation handling.
    IFC(HandleNavigationKey(key, /*scrollViewport*/ TRUE, newFocusedIndex));
    *pNewFocusedIndex = newFocusedIndex;

    *pIsHandled = (newFocusedIndex != sourceFocusedIndex);

Cleanup:
    RRETURN(hr);
}

// Panels may decide to use the default Selector logic for keyboard
// navigation (see QueryPanelForItemNavigation's pUseFallback). However,
// the usual Selector logic cannot handle grouping. Instead, we re-implement
// a grouping-aware version here. This particular method attempts to navigate
// BETWEEN groups. QueryGroupFallbackForItemNavigation handles the in-group
// navigation.
_Check_return_ HRESULT ListViewBase::QueryGroupFallbackForGroupNavigation(
    _In_ wsy::VirtualKey key,
    _In_ INT sourceGroupIndex,
    _Out_ INT* pNewGroupIndex,
    _Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;
    BOOLEAN bInvertForRTL = FALSE;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
    BOOLEAN isVertical = FALSE;
    INT newGroupIndex = sourceGroupIndex;
    xaml::FlowDirection direction = xaml::FlowDirection_LeftToRight;
    INT groupCount = 0;
    INT indexDeltaPerStep = 0;

    // At this point, we know we're grouping, but the inner panels have not handled the event (probably
    // because they reached the edge of the items).
    // So, we need to do a group-aware implementation of Selector's HandleKeyNavigation - but this tiem,
    // the implementation deals with groups instead of items, since we're crossing groups.

    *pNewGroupIndex = -1;
    *pIsHandled = FALSE;

    IFC(GetGroupCount(&groupCount));

    IFC(get_FlowDirection(&direction));
    bInvertForRTL = (direction == xaml::FlowDirection_RightToLeft);

    IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
    isVertical = (physicalOrientation == xaml_controls::Orientation_Vertical);

    switch (key)
    {
        case wsy::VirtualKey_Left:

            if (!isVertical)
            {
                if (bInvertForRTL)
                {
                    indexDeltaPerStep = 1;
                }
                else
                {
                    indexDeltaPerStep = -1;
                }
            }
            break;
        case wsy::VirtualKey_Up:
            if (isVertical)
            {
                indexDeltaPerStep = -1;
            }
            break;
        case wsy::VirtualKey_Right:
            if (!isVertical)
            {
                if (bInvertForRTL)
                {
                    indexDeltaPerStep = -1;
                }
                else
                {
                    indexDeltaPerStep = 1;
                }
            }
            break;
        case wsy::VirtualKey_Down:
            if (isVertical)
            {
                indexDeltaPerStep = 1;
            }
            break;
    }

    if (indexDeltaPerStep != 0 &&
        ((sourceGroupIndex + indexDeltaPerStep) >= 0) &&
        ((sourceGroupIndex + indexDeltaPerStep) < groupCount)
        )
    {
        BOOLEAN isTargetFocusable = FALSE;

        do
        {
            newGroupIndex += indexDeltaPerStep;
            IFC(ListViewBase::IsGroupItemFocusable(this, newGroupIndex, &isTargetFocusable));
        } 
        while (!isTargetFocusable && newGroupIndex > 0 && newGroupIndex < groupCount - 1);

        if (isTargetFocusable)
        {
            *pIsHandled = TRUE;
            *pNewGroupIndex = newGroupIndex;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Gets the number of groups we have.
_Check_return_ HRESULT ListViewBase::GetGroupCount(
    _Out_ INT* pGroupCount)
{
    HRESULT hr = S_OK;
    UINT groupCount = 0;

    ctl::ComPtr<ICollectionView> spCollectionView;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCollectionGroups;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spCollectionGroupsAsV;

    IFC(get_CollectionView(&spCollectionView));
    IFC(spCollectionView->get_CollectionGroups(&spCollectionGroups));

    IFC(spCollectionGroups.As<wfc::IVector<IInspectable*>>(&spCollectionGroupsAsV));
    IFC(spCollectionGroupsAsV->get_Size(&groupCount));

    *pGroupCount = (INT)groupCount;

Cleanup:
    RRETURN(hr);
}

// Panels may decide to use the default Selector logic for keyboard
// navigation (see QueryPanelForItemNavigation's pUseFallback). However,
// the usual Selector logic cannot handle grouping. Instead, we re-implement
// a grouping-aware version here. This particular method attempts to navigate
// WITHIN a group - it returns pIsHandled = FALSE if it hits a group boundary.
// See QueryGroupFallbackForGroupNavigation.
_Check_return_ HRESULT ListViewBase::QueryGroupFallbackForItemNavigation(
    _In_ xaml_controls::IPanel* pGroupPanel,
    _In_ wsy::VirtualKey key,
    _In_ INT sourceIndexInCollection,
    _In_ INT groupLeftEdgeIndex,
    _In_ INT groupRightEdgeIndex,
    _Out_ INT* pNewFocusedIndexInCollection,
    _Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;
    BOOLEAN bInvertForRTL = FALSE;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
    BOOLEAN isVertical = FALSE;
    xaml::FlowDirection direction = xaml::FlowDirection_LeftToRight;
    UINT nCount = 0;
    INT newFocusedIndexInCollection = sourceIndexInCollection;

    // At this point, we know we're grouping, and we've given the group's panel a chance to handle the key nav.
    // Turns out, the panel is either not a KeyNavPanel, or the panel explicitly decided to let us deal with it.
    // So, we need to do a group-aware implementation of Selector's HandleKeyNavigation.
    // We shouldn't handle the event if we can't navigate (say, we're trying to go outside the bounds of the group).
    // Other logic will handle that.

    *pNewFocusedIndexInCollection = FALSE;

    nCount = groupRightEdgeIndex - groupLeftEdgeIndex;

    IFC(get_FlowDirection(&direction));
    bInvertForRTL = (direction == xaml::FlowDirection_RightToLeft);

    IFC(GetPanelOrientations(pGroupPanel, &physicalOrientation, NULL /*pLogicalOrientation*/));
    isVertical = (physicalOrientation == xaml_controls::Orientation_Vertical);

    switch (key)
    {
        case wsy::VirtualKey_Left:

            if (!isVertical)
            {
                if (bInvertForRTL)
                {
                    IFC(SelectNext(newFocusedIndexInCollection));
                }
                else
                {
                    IFC(SelectPrev(newFocusedIndexInCollection));
                }
            }
            break;
        case wsy::VirtualKey_Up:
            if (isVertical)
            {
                IFC(SelectPrev(newFocusedIndexInCollection));
            }
            break;
        case wsy::VirtualKey_Right:
            if (!isVertical)
            {
                if (bInvertForRTL)
                {
                    IFC(SelectPrev(newFocusedIndexInCollection));
                }
                else
                {
                    IFC(SelectNext(newFocusedIndexInCollection));
                }
            }
            break;
        case wsy::VirtualKey_Down:
            if (isVertical)
            {
                IFC(SelectNext(newFocusedIndexInCollection));
            }
            break;
        default:
            // Don't handle these keys. They should have already been handled
            // by prior code.
            ASSERT(FALSE);
            break;
    }

    // Don't nav to outside of group.
    if (newFocusedIndexInCollection >= groupLeftEdgeIndex && newFocusedIndexInCollection < groupRightEdgeIndex)
    {
        *pIsHandled = (newFocusedIndexInCollection != sourceIndexInCollection);
        *pNewFocusedIndexInCollection = newFocusedIndexInCollection;
    }

Cleanup:
    RRETURN(hr);
}


// Called when the ListView receives focus.
IFACEMETHODIMP ListViewBase::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    bool isOriginalSource = false;
    CControl *pControl = do_pointer_cast<CControl>(GetHandle());

    IFCPTR(pArgs);
    IFC(ListViewBaseGenerated::OnGotFocus(pArgs));

    if (!pControl->IsFocusEngagementEnabled() || pControl->IsFocusEngaged())
    {
        IFC(pArgs->get_OriginalSource(&spOriginalSource));
        IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));

        if (isOriginalSource)
        {
            BOOLEAN hasFocus = FALSE;
            IFC(HasFocus(&hasFocus));
            IFC(OnFocusChanged(hasFocus));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when the ListView loses focus.
IFACEMETHODIMP ListViewBase::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    bool isOriginalSource = false;


    IFCPTR(pArgs);
    IFC(ListViewBaseGenerated::OnLostFocus(pArgs));

    IFC(pArgs->get_OriginalSource(&spOriginalSource));
    IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));
    if (isOriginalSource)
    {
        BOOLEAN hasFocus = FALSE;
        IFC(HasFocus(&hasFocus));
        IFC(OnFocusChanged(hasFocus));
    }

Cleanup:
    RRETURN(hr);
}

// Updates IsSelectionActive when the ListView receives or loses focus and
// optionally refocuses the last focused child ListViewBaseItem.
_Check_return_ HRESULT ListViewBase::OnFocusChanged(
    _In_ BOOLEAN hasFocus)
{
    HRESULT hr = S_OK;

    IFC(put_IsSelectionActive(hasFocus));

    if (hasFocus)
    {
        INT targetFocusedItem = -1;

        if (GetFocusedGroupIndex() >= 0)
        {
            // Tabbing from group header to items.
            UINT groupStartIndex = 0;
            UINT groupSize = 0;
            BOOLEAN isValid = FALSE;
            IFC(GetGroupInformation(GetFocusedGroupIndex(), &groupStartIndex, &groupSize, &isValid));
            if (isValid)
            {
                targetFocusedItem = static_cast<INT>(groupStartIndex);
            }
        }
        else if (GetFocusedIndex() >= 0)
        {
            targetFocusedItem = GetFocusedIndex();
        }
        else
        {
            targetFocusedItem = static_cast<INT>(GetLastFocusedIndex());
        }

        if (targetFocusedItem >= 0)
        {
            IFC(SetFocusedItem(targetFocusedItem, /* shouldScrollIntoView */ FALSE));
        }
    }
    else
    {
        SetFocusedGroupIndex(-1);
    }

Cleanup:
    RRETURN(hr);
}

// Handles changes to GroupItems focus (as a whole collection of groups). If a group
// header has focus, forward focus to the last focused header.
_Check_return_ HRESULT ListViewBase::OnGroupFocusChanged(
    _In_ BOOLEAN hasFocus,
    _In_ BOOLEAN headerHasFocus,
    _In_ xaml::FocusState howFocusChanged)
{
    HRESULT hr = S_OK;

    if (hasFocus)
    {
        SetFocusedGroupIndex(GetFocusedGroupIndex() >= 0 ? GetFocusedGroupIndex() : GetLastFocusedGroupIndex());
        SetLastFocusedGroupIndex(GetFocusedGroupIndex());

        // Redirect focus if the header has focus.
        if (headerHasFocus)
        {
            ctl::ComPtr<xaml::IDependencyObject> spGroupItemAsIDO;
            ctl::ComPtr<IGroupItem> spGroupItem;
            IFC(HeaderFromIndex(GetFocusedGroupIndex(), &spGroupItemAsIDO));
            spGroupItem = spGroupItemAsIDO.AsOrNull<IGroupItem>();
            if (spGroupItem)
            {
                BOOLEAN didFocus = FALSE;
                IFC(spGroupItem.Cast<GroupItem>()->FocusHeader(howFocusChanged, &didFocus));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Focus a ListViewBaseItem.
_Check_return_ HRESULT ListViewBase::FocusItem(
    _In_ xaml::FocusState focusState,
    _In_ ListViewBaseItem* pItem)
{
    HRESULT hr = S_OK;
    INT index = -1;
    BOOLEAN focused = FALSE;

    IFCPTR(pItem);

    // TODO: Cleanup the mechanics around SetFocusedItem because it's currently
    // kind of complicated.  We've also got several methods with similar names
    // that do kind of the same things in different ways.  I'm waiting until we
    // get to InteractionManager before deciding how to split things up.
    //
    // This FocusItem(item) does the following:
    //  1. This method
    //    a. Gets and optionally stores the to-be-focused item's index as last
    //    b. Calls Focus(...) on the item
    //  3. The item handles OnGotFocus and calls its FocusChanged
    //  4. The item's FocusChanged will invoke Selector.ItemFocused
    //  5. Selector.ItemFocused stores the currently focused index
    //  6. (in parallel with 3) Any unfocused item will call
    //     Selector.ItemUnfocused
    //  7. If the caller of this SetFocusedItem also changes selection, then
    //     Selector.OnSelectionChanged will call SetFocusedItem as well.  The
    //     danger of just skipping 1a is that we may be called via ItemClick
    //     scenarios where no additional selection will take place.4
    //
    // The other SetFocusedItem(index, scroll) does the following:
    //  1. Optionally stores the to-be-focused item's index as last
    //  2. Calls Selector.SetSelectedItem which will
    //    a. Ensure the bounds of the index
    //    b. Optionally scroll the item into view (if it's desired and it can)
    //    c. Optionally lookup the container from the index and focus it
    //  3. The item handles OnGotFocus and calls its FocusChanged
    //  4. The item's FocusChanged will invoke Selector.ItemFocused
    //  5. Selector.ItemFocused stores the currently focused index
    //  6. (in parallel with 3) Any unfocused item will call
    //     Selector.ItemUnfocused

    // Get the index of the item
    IFC(IndexFromContainer(pItem, &index));

#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: FocusItem. focusState=%d, index=%d", 
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, focusState, index));
#endif

    // Store the index of the last focused item so we can jump back to this item
    // if ListViewBase loses and regains focus
    if (index >= 0)
    {
        SetLastFocusedIndex(static_cast<UINT>(index));
    }

    // Focus the item
    IFC(pItem->FocusSelfOrChild(focusState, FALSE /*animateIfBringIntoView*/, &focused));

Cleanup:
    RRETURN(hr);
}

// Focus a ListViewBaseItem at the given index and optionally scroll it into view.
_Check_return_ HRESULT ListViewBase::SetFocusedItem(
    _In_ INT index,
    _In_ BOOLEAN shouldScrollIntoView,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    InputActivationBehavior inputActivationBehavior)
{
    HRESULT hr = S_OK;

    // Store the index of the last focused item so we can jump back to this item
    // if ListViewBase loses and regains focus
    if (index >= 0)
    {
        SetLastFocusedIndex(static_cast<UINT>(index));
    }

    const BOOLEAN effectiveShouldScrollIntoView = shouldScrollIntoView && !m_isInOnSelectionModeChanged;

    // Focus the item by calling Selector.SetFocusedItem
    IFC(ListViewBaseGenerated::SetFocusedItem(index, effectiveShouldScrollIntoView, animateIfBringIntoView, focusNavigationDirection, inputActivationBehavior));

Cleanup:
    RRETURN(hr);
}

// Set the ListViewBaseItem  at index to be focused using an explicit FocusState
// The focus is only set if forceFocus is TRUE or the Selector already has focus.
// ScrollIntoView is always called if shouldScrollIntoView is set (regardless of focus).
_Check_return_ HRESULT ListViewBase::SetFocusedItem(
    _In_ INT index,
    _In_ BOOLEAN shouldScrollIntoView,
    _In_ BOOLEAN forceFocus,
    _In_ xaml::FocusState focusState,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    InputActivationBehavior inputActivationBehavior)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: SetFocusedItem. index=%d, shouldScrollIntoView=%d, forceFocus=%d, focusState=%d, animateIfBringIntoView=%d, focusNavigationDirection=%d", 
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, index, shouldScrollIntoView, forceFocus, focusState, animateIfBringIntoView, focusNavigationDirection));
#endif

    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spItemsHostPanel;
    ctl::ComPtr<IModernCollectionBasePanel> spItemsHostPanelModernCollection;

    // Store the index of the last focused item so we can jump back to this item
    // if ListViewBase loses and regains focus
    if (index >= 0)
    {
        SetLastFocusedIndex(static_cast<UINT>(index));
    }

    BOOLEAN effectiveShouldScrollIntoView = shouldScrollIntoView && !m_isInOnSelectionModeChanged;

    // If i need to scroll intoview, check if the host panel
    // is in a state where we can force scroll into view.
    if (effectiveShouldScrollIntoView)
    {
        IFC(get_ItemsHost(&spItemsHostPanel));
        spItemsHostPanelModernCollection = spItemsHostPanel.AsOrNull<IModernCollectionBasePanel>();
        if (spItemsHostPanelModernCollection)
        {
            effectiveShouldScrollIntoView = spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->CanScrollIntoView();
        }
    }

    // Focus the item by calling Selector.SetFocusedItem
    IFC(ListViewBaseGenerated::SetFocusedItem(index, effectiveShouldScrollIntoView, forceFocus, focusState, animateIfBringIntoView, focusNavigationDirection, inputActivationBehavior));

Cleanup:
    RRETURN(hr);
}

// Handles changes to the IsSelectionActive property by updating the visual
// states of all the currently selected ListViewBaseItems.
_Check_return_ HRESULT ListViewBase::OnIsSelectionActiveChanged()
{
    HRESULT hr = S_OK;
    UINT selectedCount = 0;

    IFC(m_selection.GetNumItemsSelected(selectedCount));
    if (selectedCount > 0)
    {
        for (UINT i = 0; i < selectedCount; i++)
        {
            UINT itemIndex = 0;
            ctl::ComPtr<xaml::IDependencyObject> spContainer;

            IFC(m_selection.GetIndexAt(i, itemIndex));
            IFC(ContainerFromIndex(itemIndex, &spContainer));
            // If the container at given Index is not relaised, it will return NULL
            // If it is realised then only VisualStates needs to be updated
            if (spContainer)
            {
                ctl::ComPtr<ISelectorItem> spItem;
                spItem = spContainer.AsOrNull<ISelectorItem>();
                if (spItem && IsListViewBaseItem(ctl::iinspectable_cast(spItem.Get())))
                {
                    IFC(spItem.Cast<ListViewBaseItem>()->ChangeVisualState(TRUE));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::SetHoldingItem(
    _In_ ListViewBaseItem* pItem)
{
    IFCPTR_RETURN(pItem);
    if (m_tpHoldingItem.Get() != pItem)
    {
        IFC_RETURN(ClearHoldingState());
        SetPtrValue(m_tpHoldingItem, pItem);
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::ClearHoldingItem(
    _In_ ListViewBaseItem* pItem)
{
    IFCPTR_RETURN(pItem);
    if (m_tpHoldingItem.Get() == pItem)
    {
        m_tpHoldingItem.Clear();
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::ClearHoldingState()
{
    if (m_tpHoldingItem)
    {
        ctl::ComPtr<ListViewBaseItem> holdingItem = m_tpHoldingItem.Cast<ListViewBaseItem>();
        IFC_RETURN(holdingItem->ClearHoldingState());
        IFC_RETURN(ClearHoldingItem(holdingItem.Get()));
        SetIsHolding(false);
    }

    return S_OK;
}

// Returns TRUE if the ListViewBase is currently engaged in an exclusive interaction
// and should thus ignore all new interactions.
BOOLEAN ListViewBase::IsInExclusiveInteraction()
{
    return IsInDragDrop();
}

// Checks whether the group item returned for the given index
// by pGenerator is focusable.
_Check_return_ HRESULT ListViewBase::IsGroupItemFocusable(
    _In_ DirectUI::IGroupHeaderMapping* pMapping,
    _In_ UINT groupIndex,
    _Out_ BOOLEAN* pIsFocusable)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spGroupItemAsIDO;
    ctl::ComPtr<IControl> spGroupItemAsControl;
    BOOLEAN isFocusable = FALSE;

    *pIsFocusable = FALSE;

    IFC(pMapping->HeaderFromIndex(groupIndex, &spGroupItemAsIDO));

    spGroupItemAsControl = spGroupItemAsIDO.AsOrNull<IControl>();

    if (spGroupItemAsControl)
    {
        IFC(spGroupItemAsControl->get_IsEnabled(&isFocusable));
    }
    else
    {
        // Non-Controls focusable by default (see ItemsControl::IsFocusableHelper).
        isFocusable = TRUE;
    }

    if (isFocusable)
    {
        ctl::ComPtr<IUIElement> spGroupItemAsUIE;

        spGroupItemAsUIE = spGroupItemAsIDO.AsOrNull<IUIElement>();
        if (spGroupItemAsUIE)
        {
            xaml::Visibility visibility = xaml::Visibility_Collapsed;
            IFC(spGroupItemAsUIE->get_Visibility(&visibility));
            isFocusable = visibility != xaml::Visibility_Collapsed;
        }
    }

    *pIsFocusable = isFocusable;

Cleanup:
    RRETURN(hr);
}

// Checks whether the item container returned for the given index
// by pGenerator is Selectable.
_Check_return_ HRESULT ListViewBase::IsItemFocusable(
    _In_ IItemContainerMapping* pMapping,
    _In_ UINT itemIndex,
    _Out_ BOOLEAN* pIsFocusable)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spContainerAsIDO;
    ctl::ComPtr<IInspectable> spContainerAsI;
    BOOLEAN isFocusable = FALSE;

    *pIsFocusable = FALSE;

    IFC(pMapping->ContainerFromIndex(itemIndex, &spContainerAsIDO));

    spContainerAsI = spContainerAsIDO.AsOrNull<IInspectable>();
    IFC(ItemsControl::IsFocusableHelper(spContainerAsI.Get(), isFocusable));
    *pIsFocusable = isFocusable;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::CanPerformKeyboardNavigation(
    _In_ wsy::VirtualKey key,
    _Out_ bool* canNavigate)
{
    *canNavigate = true;

    ctl::ComPtr<IPanel> itemsHost;
    IFC_RETURN(get_ItemsHost(&itemsHost));

    if (!itemsHost)
    {
        *canNavigate = false;
    }
    else
    {
        xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
        IFC_RETURN(GetItemsHostOrientations(&physicalOrientation, nullptr /* pLogicalOrientation */));

        // Triggers should navigate only for vertical lists, and Shoulders should navigate only for horizontal lists
        if ((key == wsy::VirtualKey_GamepadLeftTrigger || key == wsy::VirtualKey_GamepadRightTrigger) &&
            physicalOrientation != xaml_controls::Orientation_Vertical)
        {
            *canNavigate = false;
        }

        if ((key == wsy::VirtualKey_GamepadLeftShoulder || key == wsy::VirtualKey_GamepadRightShoulder) &&
            physicalOrientation != xaml_controls::Orientation_Horizontal)
        {
            *canNavigate = false;
        }
    }

    return S_OK;
}
