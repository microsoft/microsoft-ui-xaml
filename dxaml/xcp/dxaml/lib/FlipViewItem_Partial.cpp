// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FlipViewItem.g.h"
#include "FlipViewItemAutomationPeer.g.h"
#include "Selector.g.h"
#include "ItemCollection.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "XboxUtility.h"
#include "ElementSoundPlayerService_Partial.h"

// Work around disruptive max/min macros
#undef max
#undef min

using namespace DirectUI;

FlipViewItem::FlipViewItem()
    :m_isPressed(FALSE)
{
}

FlipViewItem::~FlipViewItem()
{
}

// Create FlipViewItemAutomationPeer to represent the FlipViewItem.
IFACEMETHODIMP FlipViewItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IFlipViewItemAutomationPeer> spFlipViewItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IFlipViewItemAutomationPeerFactory> spFlipViewItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::FlipViewItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spFlipViewItemAPFactory));

    IFC(spFlipViewItemAPFactory.Cast<FlipViewItemAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spFlipViewItemAutomationPeer));
    IFC(spFlipViewItemAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Called when the control got focus.
IFACEMETHODIMP
FlipViewItem::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;

    IFC(FlipViewItemGenerated::OnGotFocus(pArgs));

    IFC(HasFocus(&hasFocus));
    IFC(pArgs->get_OriginalSource(&spOriginalSource));
    IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));

    IFC(FocusChanged(hasFocus, !!isOriginalSource));

Cleanup:
    RRETURN(hr);
}

// LostFocus event handler.
IFACEMETHODIMP
FlipViewItem::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;

    IFC(FlipViewItemGenerated::OnLostFocus(pArgs));

    IFC(HasFocus(&hasFocus));
    IFC(pArgs->get_OriginalSource(&spOriginalSource));
    IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));

    IFC(FocusChanged(hasFocus, !!isOriginalSource));

Cleanup:
    RRETURN(hr);
}

// Called when we got or lost focus
_Check_return_
HRESULT
FlipViewItem::FocusChanged(
    _In_ BOOLEAN hasFocus,
    _In_ BOOLEAN isOriginalSource)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Selector> spParentSelector;

    // Let parent selector know that this item was selected
    IFC(GetParentSelector(&spParentSelector));

    // FocusManager gets the correct focusable item from the control(see
    // Selector::GetFirst/LastFocusableElement), so additional work to
    // set focus to the last focused item is not necessary
    if (spParentSelector)
    {
        if (hasFocus)
        {
            IFC(spParentSelector->ItemFocused(this));
        }
        else
        {
            IFC(spParentSelector->ItemUnfocused(this));
        }
    }

    IFC(ChangeVisualState(TRUE));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
FlipViewItem::OnKeyDown(
_In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    BOOLEAN handled = FALSE;
    ctl::ComPtr<Selector> spParentSelector;

    IFCPTR_RETURN(pArgs);
    IFC_RETURN(FlipViewItemGenerated::OnKeyDown(pArgs));

    IFC_RETURN(pArgs->get_Handled(&handled));
    if (handled)
    {
        return S_OK;
    }

    // Let parent selector know that this item was selected
    IFC_RETURN(GetParentSelector(&spParentSelector));

    if (spParentSelector)
    {
        wsy::VirtualKey key = wsy::VirtualKey_None;

        IFC_RETURN(pArgs->get_Key(&key));

        if (Selector::IsNavigationKey(key) || XboxUtility::IsGamepadPageNavigationDirection(key))
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
            ctl::ComPtr<KeyRoutedEventArgs> spArgsWithOriginalKey;
            wsy::VirtualKey originalKey = wsy::VirtualKey_None;
            xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
            UINT itemsCount = 0;
            INT oldSelectedIndex = 0;
            INT newSelectedIndex = 0;
            INT minIndex = 0;

            spArgsWithOriginalKey = ctl::query_interface_cast<KeyRoutedEventArgs>(pArgs);

            if (spArgsWithOriginalKey)
            {
                IFC_RETURN(spArgsWithOriginalKey->get_OriginalKey(&originalKey));
            }

            IFC_RETURN(spParentSelector->get_SelectedIndex(&oldSelectedIndex));
            newSelectedIndex = oldSelectedIndex;
            IFC_RETURN(spParentSelector->get_Items(&spItems));
            IFC_RETURN(spItems.Cast<ItemCollection>()->get_Size(&itemsCount));
            IFC_RETURN(spParentSelector->GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
            const bool isVertical = (physicalOrientation == xaml_controls::Orientation_Vertical);

            if (isVertical)
            {
                // If the FlipView is Vertical, don't do navigate/mark handled the Right/Left/Shoulder keys
                bool isVerticalNoOp = XboxUtility::IsGamepadNavigationRight(originalKey) ||
                                      XboxUtility::IsGamepadNavigationLeft(originalKey) ||
                                      key == wsy::VirtualKey_GamepadLeftShoulder ||
                                      key == wsy::VirtualKey_GamepadRightShoulder;

                if (isVerticalNoOp)
                {
                    return S_OK;
                }
            }
            else
            {
                // If the FlipView is Horizontal, don't do navigate/mark handled the Up/Down/Trigger keys
                bool isHorizontalNoOp = XboxUtility::IsGamepadNavigationUp(originalKey) ||
                                        XboxUtility::IsGamepadNavigationDown(originalKey) ||
                                        key == wsy::VirtualKey_GamepadLeftTrigger ||
                                        key == wsy::VirtualKey_GamepadRightTrigger;

                if (isHorizontalNoOp)
                {
                    return S_OK;
                }
            }

            if (key == wsy::VirtualKey_PageUp)
            {
                newSelectedIndex--;
            }
            else if (key == wsy::VirtualKey_PageDown)
            {
                newSelectedIndex++;
            }
            else
            {
                IFC_RETURN(spParentSelector->HandleNavigationKey(key, FALSE /*scrollViewport*/, newSelectedIndex));
            }

            // We don't want selected index to go to -1 (unselected) if we have items.
            minIndex = itemsCount > 0 ? 0 : -1;
            newSelectedIndex = std::min(newSelectedIndex, static_cast<INT>(itemsCount-1));
            newSelectedIndex = std::max(newSelectedIndex, minIndex);
            IFC_RETURN(spParentSelector->put_SelectedIndex(newSelectedIndex));

            if (newSelectedIndex != oldSelectedIndex)
            {
                auto soundToPlay = newSelectedIndex > oldSelectedIndex ? xaml::ElementSoundKind_MoveNext : xaml::ElementSoundKind_MovePrevious;
                IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(soundToPlay, this));
            }

            //Focus state needs to be keyboard when using navigation keys
            IFC_RETURN(spParentSelector->SetFocusedItem(newSelectedIndex, FALSE, TRUE, xaml::FocusState_Keyboard));

            IFC_RETURN(pArgs->put_Handled(TRUE));
        }
    }

    return S_OK;
}


IFACEMETHODIMP FlipViewItem::OnPointerCanceled(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(FlipViewItemGenerated::OnPointerCanceled(pArgs));

Cleanup:
    m_isPressed = FALSE;
    RRETURN(hr);
}

IFACEMETHODIMP FlipViewItem::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(FlipViewItemGenerated::OnPointerCaptureLost(pArgs));

Cleanup:
    m_isPressed = FALSE;
    RRETURN(hr);
}

// PointerPressed event handler.
IFACEMETHODIMP FlipViewItem::OnPointerPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;
    ctl::ComPtr<Selector> spParentSelector;

    IFC(FlipViewItemGenerated::OnPointerPressed(pArgs));

    IFC(pArgs->get_Handled(&handled));
    if (handled)
    {
        goto Cleanup;
    }

    m_isPressed = TRUE;

Cleanup:
    RRETURN(hr);
}

// PointerReleased event handler.
IFACEMETHODIMP FlipViewItem::OnPointerReleased(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;

    IFC(FlipViewItemGenerated::OnPointerReleased(pArgs));

    IFC(pArgs->get_Handled(&handled));

    if (m_isPressed && !handled)
    {
        ctl::ComPtr<Selector> spParentSelector;

        IFC(pArgs->put_Handled(TRUE));
        IFC(GetParentSelector(&spParentSelector));

        if (spParentSelector)
        {
            INT nSelectedIndex = 0;

            IFC(spParentSelector->get_SelectedIndex(&nSelectedIndex));

            //When pointer is released the current item needs to have its focus changed from whatever it is to pointer.
            //This is accomplished by using SetFocusItem with ForceFocus set to TRUE.
            IFC(spParentSelector->SetFocusedItem(nSelectedIndex, FALSE, TRUE, xaml::FocusState_Pointer));
        }
    }

Cleanup:
    m_isPressed = FALSE;
    RRETURN(hr);
}

_Check_return_ HRESULT FlipViewItem::SetParentSelector(
    _In_opt_ Selector* pParentSelector)
{
    IFC_RETURN(FlipViewItemGenerated::SetParentSelector(pParentSelector));
    IFC_RETURN(put_FocusTargetDescendant(pParentSelector));

    return S_OK;
}