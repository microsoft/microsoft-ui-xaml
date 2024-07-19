// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComboBoxItem.g.h"
#include "ComboBoxItemAutomationPeer.g.h"
#include "ComboBox.g.h"
#include "PointerRoutedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ComboBoxItem::ComboBoxItem()
    : m_bIsPressed(FALSE)
    , m_shouldPerformActions(FALSE)
{
}

// Called when the user release the pointer or left mouse button over the ComboBoxItem.
IFACEMETHODIMP 
ComboBoxItem::OnPointerReleased(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    GestureModes gestureFollowing = GestureModes::None;
    ctl::ComPtr<Selector> spParent;
    
    IFC(ComboBoxItemGenerated::OnPointerReleased(pArgs));
    IFCPTR(pArgs);
    
    IFC(pArgs->get_Handled(&isHandled));

    if (isHandled)
    {
        goto Cleanup;
    }

    m_shouldPerformActions = m_bIsPressed;

    IFC(GetParentSelector(&spParent));

    if (spParent && m_bIsPressed)
    {
        m_bIsPressed = FALSE;
        IFC(ChangeVisualState(true));
    }
    IFC(static_cast<PointerRoutedEventArgs*>(pArgs)->get_GestureFollowing(&gestureFollowing));
    if (gestureFollowing == GestureModes::RightTapped)
    {
        goto Cleanup;
    }

    // Note that we are intentionally NOT handling the args
    // if we do not fall through here because basically we are no_opting in that case.
    IFC(pArgs->put_Handled(TRUE));
    IFC(PerformPointerUpAction());
    
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxItem::OnRightTappedUnhandled(
    _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFC(ComboBoxItemGenerated::OnRightTappedUnhandled(pArgs));
    IFC(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        goto Cleanup;
    }
    IFC(PerformPointerUpAction());

Cleanup:
    RRETURN(hr);
}

// Perform the primary action related to pointer up.
_Check_return_
HRESULT
ComboBoxItem::PerformPointerUpAction()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Selector> spParent;

    IFC(GetParentSelector(&spParent));
    // m_shouldPerformActions will _never_ be true if m_bIsPressed
    // is FALSE by the time we set it. So we do not check m_bIsPressed
    // here.
    if (spParent && m_shouldPerformActions)
    {
        ctl::ComPtr<IComboBox> spCbNoRef;
        BOOLEAN bFocused = FALSE;

        IFC(spParent->OnSelectorItemClicked(this, &bFocused));
        IFC(ChangeVisualState(true));

        spCbNoRef = spParent.AsOrNull<IComboBox>();
        if (spCbNoRef)
        {
            ComboBox* cbNoRef = spCbNoRef.Cast<ComboBox>();
            IFC(cbNoRef->put_IsDropDownOpen(FALSE));
        }
    }

Cleanup:
    m_shouldPerformActions = FALSE;
    RRETURN(hr);
}

// Loaded event handler.
_Check_return_ 
 HRESULT 
 ComboBoxItem::OnLoaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ChangeVisualState(false));

Cleanup:
    RRETURN(hr);
}

// Apply a template to the ComboBoxItem.
IFACEMETHODIMP 
ComboBoxItem::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IFrameworkElement> spContentPresenterAsFE;

    IFC(ComboBoxItemGenerated::OnApplyTemplate());

    IFC(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"ContentPresenter"), spContentPresenterAsFE.ReleaseAndGetAddressOf()));
    if (spContentPresenterAsFE)
    {
        BOOLEAN returnValue;

        // Waiting for the ContentPresenter's template to be applied later as part of the normal 
        // measure pass will reset any template bound properties that are updated as a result of
        // the call below to ChangeVisualState(). This is because the ContentPresenter's
        // OnApplyTemplate calls RefreshTemplateBindings() on its parent, in this case the ComboBoxItem.
        // It is expected that changing the input mode visual state can result in updates to 
        // ComboBoxItem size which need to be taken into account when laying out the popup.
        IFC(spContentPresenterAsFE.Cast<FrameworkElement>()->InvokeApplyTemplate(&returnValue));
    }

    // Sync the logical and visual states of the control
    IFC(ChangeVisualState(false));

Cleanup:
    RRETURN(hr);
}

// Called when the user presses the left mouse button over the ComboBoxItem.
IFACEMETHODIMP 
ComboBoxItem::OnPointerPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bHandled = FALSE;
    ctl::ComPtr<Selector> spParentSelector;
    BOOLEAN bIsLeftButtonPressed = FALSE;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

    IFC(ComboBoxItemGenerated::OnPointerPressed(pArgs));
    m_bIsPressed = FALSE;
    IFCPTR(pArgs);
    IFC(pArgs->get_Handled(&bHandled));
    if (!bHandled)
    {
        IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_Properties(&spPointerProperties));
        IFCPTR(spPointerProperties);
        IFC(spPointerProperties->get_IsLeftButtonPressed(&bIsLeftButtonPressed));

        if (bIsLeftButtonPressed) 
        {
            IFC(GetParentSelector(&spParentSelector));
            if (spParentSelector)
            {
                ctl::ComPtr<IComboBox> spCbNoRef;
                BOOLEAN bPopupOpen = TRUE;
                
                IFC(pArgs->put_Handled(TRUE));
                
                spCbNoRef = spParentSelector.AsOrNull<IComboBox>();
                if (spCbNoRef)
                {
                    ComboBox* cbNoRef = spCbNoRef.Cast<ComboBox>();
                    IFC(cbNoRef->get_IsDropDownOpen(&bPopupOpen));
                }

                if (bPopupOpen)
                {
                    m_bIsPressed = TRUE;
                    IFC(ChangeVisualState(true));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when the mouse or pointer enters the bounds of this element.
IFACEMETHODIMP 
ComboBoxItem::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;

    IFC(ComboBoxItemGenerated::OnPointerEntered(pArgs));

    // TODO: check whether we get MouseEnter on Disabled element.
    IFC(get_IsEnabled(&bIsEnabled));
    if (bIsEnabled)
    {
        IFC(put_IsPointerOver(TRUE));
        IFC(ChangeVisualState(true));
    }

Cleanup:
    RRETURN(hr);
}

// Called when the mouse or pointer leaves the bounds of this element.
IFACEMETHODIMP 
ComboBoxItem::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(ComboBoxItemGenerated::OnPointerExited(pArgs));
    //No PointerCapture so PointerExit, the item is no longer pressed.
    IFC(ClearVisualStateFlagsOnExit());
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);  
}

// PointerCaptureLost event handler.
IFACEMETHODIMP ComboBoxItem::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(ComboBoxItemGenerated::OnPointerCaptureLost(pArgs));
    //No PointerCapture so PointerExit, the item is no longer pressed.
    IFC(ClearVisualStateFlagsOnExit());
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);  
}


// Called when the IsEnabled property changes.
_Check_return_ 
HRESULT 
ComboBoxItem::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;

    IFC(ComboBoxItemGenerated::OnIsEnabledChanged(pArgs));
   
    IFC(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(ClearVisualStateFlagsOnExit());
    }
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

// Called when the control got focus.
IFACEMETHODIMP 
ComboBoxItem::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource; 
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;

    IFC(ComboBoxItemGenerated::OnGotFocus(pArgs));
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
ComboBoxItem::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;

    IFC(ComboBoxItemGenerated::OnLostFocus(pArgs));
    IFCPTR(pArgs);

    IFC(HasFocus(&hasFocus));
    IFC(pArgs->get_OriginalSource(&spOriginalSource));
    IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));
    IFC(FocusChanged(hasFocus, !!isOriginalSource));

Cleanup:
    RRETURN(hr);
}

// Create ListBoxItemAutomationPeer to represent the ComboBoxItem.
IFACEMETHODIMP ComboBoxItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IComboBoxItemAutomationPeer> spComboBoxItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IComboBoxItemAutomationPeerFactory> spComboBoxItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ComboBoxItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spComboBoxItemAPFactory));

    IFC(spComboBoxItemAPFactory.Cast<ComboBoxItemAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
        NULL, 
        &spInner, 
        &spComboBoxItemAutomationPeer));
    IFC(spComboBoxItemAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Called when we got or lost focus
_Check_return_ 
HRESULT 
ComboBoxItem::FocusChanged(
    _In_ BOOLEAN hasFocus,
    _In_ BOOLEAN self)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Selector> spParentSelector;

    // Let Selector know that this item was selected
    IFC(GetParentSelector(&spParentSelector));
    if (spParentSelector && self)
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
    RRETURN(hr);
}

// Change to the correct visual state for the ComboBoxItem.
_Check_return_ 
HRESULT 
ComboBoxItem::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsSelected = FALSE;
    BOOLEAN bIsPointerOver = FALSE;
    BOOLEAN bIsParentSelectionActive = FALSE;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<Selector> spParentSelector;
    ctl::ComPtr<IComboBox> spCbNoRef;
    BOOLEAN bIgnored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    BOOLEAN isDropDownOpen = FALSE;
    BOOLEAN isInlineMode = FALSE;
    DirectUI::InputDeviceType inputDeviceTypeUsedToOpenComboBox = DirectUI::InputDeviceType::None;

    IFC_RETURN(GetParentSelector(&spParentSelector));
    if (!spParentSelector)
    {
        return S_OK;
    }

    IFC_RETURN(get_IsEnabled(&bIsEnabled));
    IFC_RETURN(get_IsSelected(&bIsSelected));
    IFC_RETURN(get_IsPointerOver(&bIsPointerOver));
    IFC_RETURN(get_FocusState(&focusState));
    
    spCbNoRef = spParentSelector.AsOrNull<IComboBox>();
    if (spCbNoRef)
    {
        ComboBox* cbNoRef = spCbNoRef.Cast<ComboBox>();
        IFC_RETURN(cbNoRef->get_IsDropDownOpen(&isDropDownOpen));
        IFC_RETURN(cbNoRef->IsInlineMode(&isInlineMode));
        inputDeviceTypeUsedToOpenComboBox = cbNoRef->GetInputDeviceTypeUsedToOpen();

        // IsSelected override for searched items when ComboBox is Editable.
        if (cbNoRef->IsSearchResultIndexSet())
        {
            int resultIndex = cbNoRef->GetSearchResultIndex();

            int index = -1;
            IFC_RETURN(cbNoRef->IndexFromContainer(this, &index));

            if (index > -1 && index == resultIndex)
            {
                bIsSelected = true;
            }
            else
            {
                bIsSelected = false;
            }
        }
    }

    if (m_appearSelected.get() != nullptr)
    {
        bIsSelected = *m_appearSelected;
    }

    if (!bIsEnabled)
    {
        IFC_RETURN(get_Content(&spContent));
        // If our child is a control then we depend on it displaying a proper "disabled" state.  If it is not a control
        // (ie TextBlock, Border, etc) then we will use our visuals to show a disabled state.
        IFC_RETURN(GoToState(bUseTransitions, ctl::is<IControl>(spContent) ? L"Normal" : L"Disabled", &bIgnored));
    }
    else
    {
        // Selected Item States
        if (bIsSelected && isDropDownOpen)
        {
            if (m_bIsPressed)
            {
                IFC_RETURN(GoToState(bUseTransitions, L"SelectedPressed", &bIgnored));
            }
            else if (bIsPointerOver)
            {
                IFC_RETURN(GoToState(bUseTransitions, L"SelectedPointerOver", &bIgnored));
            }
            else
            {
                IFC_RETURN(spParentSelector->get_IsSelectionActive(&bIsParentSelectionActive));
                if (bIsParentSelectionActive)
                {
                    IFC_RETURN(GoToState(bUseTransitions, L"Selected", &bIgnored));
                }
                else
                {
                    IFC_RETURN(GoToState(bUseTransitions, L"SelectedUnfocused", &bIgnored));
                }
            }
        }
        else if (m_bIsPressed)
        {
            IFC_RETURN(GoToState(bUseTransitions, L"Pressed", &bIgnored));
        }
        else if (bIsPointerOver)
        {
            IFC_RETURN(GoToState(bUseTransitions, L"PointerOver", &bIgnored));
        }
        else
        {
            IFC_RETURN(GoToState(bUseTransitions, L"Normal", &bIgnored));
        }
    }

    // Apply the proper padding on the ContentPresenter according to the input mode.
    if (inputDeviceTypeUsedToOpenComboBox == DirectUI::InputDeviceType::Touch)
    {
        BOOLEAN usedTouchInputModeState = FALSE;

        IFC_RETURN(GoToState(bUseTransitions, L"TouchInputMode", &usedTouchInputModeState));

        // Ensure the input mode default state if the touch input mode state isn't applied.
        if (!usedTouchInputModeState)
        {
            IFC_RETURN(GoToState(bUseTransitions, L"InputModeDefault", &bIgnored));
        }
    }
    else if (inputDeviceTypeUsedToOpenComboBox == DirectUI::InputDeviceType::GamepadOrRemote)
    {
        BOOLEAN usedGameControllerInputModeState = FALSE;

        IFC_RETURN(GoToState(bUseTransitions, L"GameControllerInputMode", &usedGameControllerInputModeState));

        // Ensure the input mode default state if the game controller input mode state isn't applied.
        if (!usedGameControllerInputModeState)
        {
            IFC_RETURN(GoToState(bUseTransitions, L"InputModeDefault", &bIgnored));
        }
    }
    else
    {
        IFC_RETURN(GoToState(bUseTransitions, L"InputModeDefault", &bIgnored));
    }

    return S_OK;
}

// Prepares object's state
_Check_return_ 
HRESULT 
ComboBoxItem::Initialize()
{
    HRESULT hr = S_OK;  
    ctl::ComPtr<xaml::IRoutedEventHandler> spLoadedEventHandler;
    EventRegistrationToken eventRegistration; 

    IFC(ComboBoxItemGenerated::Initialize());
    m_bIsPressed = FALSE;

    spLoadedEventHandler.Attach( 
        new ClassMemberEventHandler<
            ComboBoxItem,
            xaml_controls::IComboBoxItem,
            xaml::IRoutedEventHandler,
            IInspectable,
            xaml::IRoutedEventArgs>(this, &ComboBoxItem::OnLoaded, true /* subscribingToSelf */));

    IFC(add_Loaded(spLoadedEventHandler.Get(), &eventRegistration));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxItem::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ComboBoxItemGenerated::OnPropertyChanged2(args));
          
    if (KnownPropertyIndex::UIElement_Visibility == args.m_pDP->GetIndex())
    {
        IFC(OnVisibilityChanged());
    }

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
ComboBoxItem::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        IFC(ClearVisualStateFlagsOnExit());
    }
    
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
ComboBoxItem::ClearVisualStateFlagsOnExit()
{
    HRESULT hr = S_OK;

    m_bIsPressed = FALSE;
    IFC(put_IsPointerOver(FALSE));

Cleanup:
    RRETURN(hr);  
}

_Check_return_ HRESULT ComboBoxItem::OverrideSelectedVisualState(
    _In_ bool appearSelected)
{
     m_appearSelected.reset(new bool(appearSelected));

    IFC_RETURN(ChangeVisualState(true));

    return S_OK;
}

_Check_return_ HRESULT ComboBoxItem::ClearSelectedVisualState()
{
    m_appearSelected.reset();

    IFC_RETURN(ChangeVisualState(true));

    return S_OK;
}
