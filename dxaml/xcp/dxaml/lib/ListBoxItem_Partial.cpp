// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListBoxItem.g.h"
#include "ListBoxItemAutomationPeer.g.h"
#include "Selector.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ListBoxItem::ListBoxItem()
    : m_bIsPressed(FALSE)
    , m_expectingGotFocusEventFromParentInteraction(FALSE)
    , m_shouldPerformActions(FALSE)
{
}

ListBoxItem::~ListBoxItem()
{
}

// Prepares object's state
_Check_return_
HRESULT
ListBoxItem::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLoadedEventHandler;
    EventRegistrationToken eventRegistration;

    IFC(ListBoxItemGenerated::Initialize());

    m_bIsPressed = FALSE;

    spLoadedEventHandler.Attach(
        new ClassMemberEventHandler<
            ListBoxItem,
            xaml_controls::IListBoxItem,
            xaml::IRoutedEventHandler,
            IInspectable,
            xaml::IRoutedEventArgs>(this, &ListBoxItem::OnLoaded, true /* subscribingToSelf */));

    IFC(add_Loaded(spLoadedEventHandler.Get(), &eventRegistration));

Cleanup:
    RRETURN(hr);
}

// Loaded event handler.
_Check_return_
 HRESULT
 ListBoxItem::OnLoaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ChangeVisualState(false));

Cleanup:
    RRETURN(hr);
}

// Apply a template to the ListBoxItem.
IFACEMETHODIMP
ListBoxItem::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    IFC(ListBoxItemGenerated::OnApplyTemplate());

    // Sync the logical and visual states of the control
    IFC(ChangeVisualState(false));

Cleanup:
    RRETURN(hr);
}

// PointerPressed event handler.
IFACEMETHODIMP
ListBoxItem::OnPointerPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bHandled = FALSE;
    BOOLEAN bIsLeftButtonPressed = FALSE;

    IFC(ListBoxItemGenerated::OnPointerPressed(pArgs));

    IFCPTR(pArgs);
    IFC(pArgs->get_Handled(&bHandled));
    if (!bHandled)
    {
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

        IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_Properties(&spPointerProperties));
        IFCPTR(spPointerProperties);
        IFC(spPointerProperties->get_IsLeftButtonPressed(&bIsLeftButtonPressed));

        if (bIsLeftButtonPressed)
        {
            m_bIsPressed = TRUE;
            IFC(pArgs->put_Handled(TRUE));
            IFC(ChangeVisualState(false));
        }
    }


Cleanup:
    RRETURN(hr);
}

// PointerReleased event handler.
IFACEMETHODIMP
ListBoxItem::OnPointerReleased(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Selector> spParentSelector;
    GestureModes gestureFollowing = GestureModes::None;
    BOOLEAN bIsEnabled = FALSE;

    IFC(ListBoxItemGenerated::OnPointerReleased(pArgs));

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(GetParentSelector(&spParentSelector));

    // If this three conditions do not hold, in our pre-quirk behavior we do nothing. So we should
    // keep consistent with this.
    m_shouldPerformActions = m_bIsPressed && bIsEnabled && spParentSelector;

    if (m_shouldPerformActions)
    {
        m_bIsPressed = FALSE;
        IFC(ChangeVisualState(true));
        IFC(static_cast<PointerRoutedEventArgs*>(pArgs)->get_GestureFollowing(&gestureFollowing));
        if (gestureFollowing == GestureModes::RightTapped)
        {
            goto Cleanup;
        }

        // Note that we are intentionally NOT handling the args
        // if we do not fall through here because basically we are no_opting in that case.
        IFC(pArgs->put_Handled(TRUE));
        IFC(PerformPointerUpAction());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListBoxItem::OnRightTappedUnhandled(
    _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFC(ListBoxItemGenerated::OnRightTappedUnhandled(pArgs));
    IFC(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        goto Cleanup;
    }
    IFC(PerformPointerUpAction());

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ListBoxItem::PerformPointerUpAction()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Selector> spParentSelector;

    // TODO: check whether we get PointerEntered on Disabled element.

    // We do not need or want to check if we are enabled again, because we already worked
    // that BOOLEAN into m_shouldPerformActions at PointerReleased.
    if (m_shouldPerformActions)
    {
        BOOLEAN bFocused = FALSE;
        IFC(GetParentSelector(&spParentSelector));
        // We can not assume to have a non-null parent selector, as this method may have been
        // called after right tapped and anyhing may happen between pointer released and this call.
        if (spParentSelector)
        {
            IFC(spParentSelector->OnSelectorItemClicked(this, &bFocused));

            ElementSoundPlayerService* soundPlayerService = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
            IFC(soundPlayerService->RequestInteractionSoundForElement(xaml::ElementSoundKind_Invoke, this));
        }
    }

Cleanup:
    m_shouldPerformActions = FALSE;
    RRETURN(hr);
}

// PointerEnter event handler.
IFACEMETHODIMP
ListBoxItem::OnPointerEntered(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;

    IFC(ListBoxItemGenerated::OnPointerEntered(pArgs));

    // TODO: check whether we get PointerEntered on Disabled element.
    IFC(get_IsEnabled(&bIsEnabled));

    if (bIsEnabled)
    {
        IFC(put_IsPointerOver(TRUE));
        IFC(ChangeVisualState(true));
    }

Cleanup:
    RRETURN(hr);
}

// PointerExited event handler.
IFACEMETHODIMP
ListBoxItem::OnPointerExited(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(ListBoxItemGenerated::OnPointerExited(pArgs));
    //No PointerCapture so PointerExit, the item is no longer pressed.
    m_bIsPressed = FALSE;

    IFC(put_IsPointerOver(FALSE));
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

// PointerCaptureLost event handler.
IFACEMETHODIMP ListBoxItem::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(ListBoxItemGenerated::OnPointerCaptureLost(pArgs));
    //No PointerCapture so PointerExit, the item is no longer pressed.
    m_bIsPressed = FALSE;

    IFC(put_IsPointerOver(FALSE));
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

// Called when the IsEnabled property changes.
_Check_return_
HRESULT
ListBoxItem::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;

    IFC(ListBoxItemGenerated::OnIsEnabledChanged(pArgs));

    IFC(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(put_IsPointerOver(FALSE));
        m_bIsPressed = FALSE;
    }
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

// Called when the control got focus.
IFACEMETHODIMP
ListBoxItem::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;

    IFC(ListBoxItemGenerated::OnGotFocus(pArgs));
    IFCPTR(pArgs);

    IFC(HasFocus(&hasFocus));
    IFC(pArgs->get_OriginalSource(&spOriginalSource));
    IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));

    IFC(FocusChanged(hasFocus, !!isOriginalSource));

Cleanup:
    RRETURN(hr);
}

// LostFocus event handler.
IFACEMETHODIMP
ListBoxItem::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;

    IFC(ListBoxItemGenerated::OnLostFocus(pArgs));
    IFCPTR(pArgs);

    IFC(HasFocus(&hasFocus));
    IFC(pArgs->get_OriginalSource(&spOriginalSource));
    IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));

    IFC(FocusChanged(hasFocus, !!isOriginalSource));

Cleanup:
    RRETURN(hr);
}

// Create ListBoxItemAutomationPeer to represent the ListBoxItem.
IFACEMETHODIMP ListBoxItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IListBoxItemAutomationPeer> spListBoxItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IListBoxItemAutomationPeerFactory> spListBoxItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ListBoxItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spListBoxItemAPFactory));

    IFC(spListBoxItemAPFactory.Cast<ListBoxItemAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spListBoxItemAutomationPeer));
    IFC(spListBoxItemAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Called when we got or lost focus
_Check_return_
HRESULT
ListBoxItem::FocusChanged(
    _In_ BOOLEAN hasFocus,
    _In_ BOOLEAN self)
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

    IFC(ChangeVisualState(true));

Cleanup:
    m_expectingGotFocusEventFromParentInteraction = FALSE;
    RRETURN(hr);
}

// Change to the correct visual state for the ListBoxItem.
_Check_return_
HRESULT
ListBoxItem::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<Selector> spParentSelector;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsSelected = FALSE;
    BOOLEAN bIsPointerOver = FALSE;
    BOOLEAN bIsParentSelectionActive = FALSE;
    BOOLEAN bIgnored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(GetParentSelector(&spParentSelector));
    if (!spParentSelector)
    {
        goto Cleanup;
    }

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_IsSelected(&bIsSelected));
    IFC(get_IsPointerOver(&bIsPointerOver));
    IFC(get_FocusState(&focusState));

    if (!bIsEnabled)
    {
        IFC(get_Content(&spContent));
        // If our child is a control then we depend on it displaying a proper "disabled" state.  If it is not a control
        // (ie TextBlock, Border, etc) then we will use our visuals to show a disabled state.
        IFC(GoToState(bUseTransitions, ctl::is<IControl>(spContent) ? L"Normal" : L"Disabled", &bIgnored));
    }
    else if (m_bIsPressed)
    {
        IFC(GoToState(bUseTransitions, L"Pressed", &bIgnored));
    }
    else if (bIsPointerOver)
    {
        IFC(GoToState(bUseTransitions, L"PointerOver", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Normal", &bIgnored));
    }

    // Change to the correct state in the Selection group
    if (bIsSelected)
    {
        if (!bIsEnabled)
        {
            IFC(GoToState(bUseTransitions, L"SelectedDisabled", &bIgnored));
        }
        else if (m_bIsPressed)
        {
            IFC(GoToState(bUseTransitions, L"SelectedPressed", &bIgnored));
        }
        else if (bIsPointerOver)
        {
            IFC(GoToState(bUseTransitions, L"SelectedPointerOver", &bIgnored));
        }
        else
        {
            IFC(spParentSelector->get_IsSelectionActive(&bIsParentSelectionActive));
            if (bIsParentSelectionActive)
            {
                IFC(GoToState(bUseTransitions, L"Selected", &bIgnored));
            }
            else
            {
                IFC(GoToState(bUseTransitions, L"SelectedUnfocused", &bIgnored));
            }
        }
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unselected", &bIgnored));
    }

    if (xaml::FocusState_Unfocused != focusState && bIsEnabled)
    {
        if (xaml::FocusState_Pointer == focusState)
        {
            IFC(GoToState(bUseTransitions, L"PointerFocused", &bIgnored));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"Focused", &bIgnored));
        }
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unfocused", &bIgnored));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListBoxItem::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ListBoxItemGenerated::OnPropertyChanged2(args));

    if (KnownPropertyIndex::UIElement_Visibility == args.m_pDP->GetIndex())
    {
        IFC(OnVisibilityChanged());
    }

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
ListBoxItem::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        m_bIsPressed = FALSE;
        IFC(put_IsPointerOver(FALSE));
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

