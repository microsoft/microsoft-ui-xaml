// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Button.g.h"
#include "ButtonAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Change to the correct visual state for the button.
_Check_return_ HRESULT Button::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsPressed = FALSE;
    BOOLEAN bIsPointerOver = FALSE;
    BOOLEAN bIgnored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_IsPressed(&bIsPressed));
    IFC(get_IsPointerOver(&bIsPointerOver));
    IFC(get_FocusState(&focusState));

    if (!bIsEnabled)
    {
        IFC(GoToState(bUseTransitions, L"Disabled", &bIgnored));
    }
    else if (bIsPressed)
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

// Raises the Click routed event.
_Check_return_ HRESULT Button::OnClick()
{
    HRESULT hr = S_OK;
    BOOLEAN bAutomationListener = FALSE;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;

    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_InvokePatternOnInvoked, &bAutomationListener));
    if (bAutomationListener)
    {
        IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
        if(spAutomationPeer)
        {
            IFC(spAutomationPeer->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_InvokePatternOnInvoked));
        }
    }

    IFC(ButtonGenerated::OnClick());

    // If this button has associated Flyout, open it now.
    IFC(OpenAssociatedFlyout());

Cleanup:
    RRETURN(hr);
}

// Create ButtonAutomationPeer to represent the button.
IFACEMETHODIMP Button::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IButtonAutomationPeer> spButtonAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IButtonAutomationPeerFactory> spButtonAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ButtonAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spButtonAPFactory));

    IFC(spButtonAPFactory.Cast<ButtonAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spButtonAutomationPeer));
    IFC(spButtonAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP Button::OnPointerCanceled(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    m_suppressFlyoutOpening = false;
    IFC_RETURN(ButtonBase::OnPointerCanceled(args));
    return S_OK;
}

IFACEMETHODIMP Button::OnPointerCaptureLost(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    m_suppressFlyoutOpening = false;
    IFC_RETURN(ButtonBase::OnPointerCaptureLost(args));
    return S_OK;
}

IFACEMETHODIMP Button::OnPointerExited(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    m_suppressFlyoutOpening = false;
    IFC_RETURN(ButtonBase::OnPointerExited(args));
    return S_OK;
}

// In case if Button has set Flyout property, get associated Flyout and open it next to this Button.
_Check_return_ HRESULT Button::OpenAssociatedFlyout()
{
    auto guard = wil::scope_exit([this]()
        {
            m_suppressFlyoutOpening = false;
        });

    if (!m_suppressFlyoutOpening)
    {
        ctl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;
        IFC_RETURN(get_Flyout(&spFlyoutBase));

        if (spFlyoutBase)
        {
            IFC_RETURN(spFlyoutBase->ShowAt(this));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Button::OnProcessKeyboardAcceleratorsImplLocal(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs)
{
    ctl::ComPtr<IFlyoutBase> spButtonFlyout;
    IFC_RETURN(get_Flyout(spButtonFlyout.ReleaseAndGetAddressOf()));
    if (spButtonFlyout == nullptr)
    {
        return S_OK;
    }

    IFC_RETURN(spButtonFlyout->TryInvokeKeyboardAccelerator(pArgs));

    return S_OK;
}

/* static */ _Check_return_ HRESULT Button::SuppressFlyoutOpening(_In_ CButton* button)
{
    ctl::ComPtr<DependencyObject> buttonPeer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(button, &buttonPeer));
    ASSERT(ctl::is<IButton>(buttonPeer));

    buttonPeer.Cast<Button>()->m_suppressFlyoutOpening = true;
    return S_OK;
}
