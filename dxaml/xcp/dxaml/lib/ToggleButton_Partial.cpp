// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToggleButton.g.h"
#include "ToggleButtonAutomationPeer.g.h"
#include "KeyboardNavigation.h"

using namespace DirectUI;

// Initializes a new instance of the RadioButton class.
ToggleButton::ToggleButton()
{
    _skipCreateAutomationPeer = FALSE;
}

// Destructor
ToggleButton::~ToggleButton()
{
}

// Prepares object's state
_Check_return_ 
HRESULT
ToggleButton::Initialize()
{
    IFC_RETURN(ToggleButtonGenerated::Initialize());

    // Handle the ENTER key by default.
    SetAcceptsReturn(true);

    return S_OK;
}

_Check_return_ HRESULT ToggleButton::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    BOOLEAN bAutomationListener = FALSE;

    IFC(ToggleButtonGenerated::OnPropertyChanged2(args));
    
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::ToggleButton_IsChecked)
    {
        IFC(OnIsCheckedChanged());

        IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
        if (bAutomationListener)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
            ctl::ComPtr<xaml_automation_peers::IToggleButtonAutomationPeer> spToggleButtonAutomationPeer;
            
            IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
            if(spAutomationPeer && !ctl::is<xaml_automation_peers::IRadioButtonAutomationPeer>(spAutomationPeer.Get()))
            {
                ctl::ComPtr<IInspectable> spOldValue;
                ctl::ComPtr<IInspectable> spNewValue;

                IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
                IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));

                IFC(spAutomationPeer.As(&spToggleButtonAutomationPeer));
                IFC(spToggleButtonAutomationPeer.Cast<ToggleButtonAutomationPeer>()->RaiseToggleStatePropertyChangedEvent(spOldValue.Get(), spNewValue.Get()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the ToggleButton.
_Check_return_ HRESULT ToggleButton::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsPressed = FALSE;
    BOOLEAN bIsPointerOver = FALSE;
    ctl::ComPtr<wf::IReference<bool>> spIsCheckedReference;
    BOOLEAN bIsChecked = FALSE;
    BOOLEAN bIgnored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_IsPressed(&bIsPressed));
    IFC(get_IsPointerOver(&bIsPointerOver));
    IFC(get_FocusState(&focusState));

    IFC(this->get_IsChecked(&spIsCheckedReference));
    if (spIsCheckedReference)
    {
        IFC(spIsCheckedReference->get_Value(&bIsChecked));
    }

    // Update the CommonStates group
    if (!spIsCheckedReference)
    {
        if (!bIsEnabled)
        {
            IFC(GoToState(bUseTransitions, L"IndeterminateDisabled", &bIgnored));
        }
        else if (bIsPressed)
        {
            IFC(GoToState(bUseTransitions, L"IndeterminatePressed", &bIgnored));
        }
        else if (bIsPointerOver)
        {
            IFC(GoToState(bUseTransitions, L"IndeterminatePointerOver", &bIgnored));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"Indeterminate", &bIgnored));
        }
    }
    else if (bIsChecked)
    {
        if (!bIsEnabled)
        {
            IFC(GoToState(bUseTransitions, L"CheckedDisabled", &bIgnored));
        }
        else if (bIsPressed)
        {
            IFC(GoToState(bUseTransitions, L"CheckedPressed", &bIgnored));
        }
        else if (bIsPointerOver)
        {
            IFC(GoToState(bUseTransitions, L"CheckedPointerOver", &bIgnored));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"Checked", &bIgnored));
        }
    }
    else
    {
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
    }

    // Update the Focus group
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

// Toggles the Check state and raises the Click routed event.
_Check_return_ HRESULT ToggleButton::OnClick()
{
    HRESULT hr = S_OK;
    IFC(OnToggleProtected());
    IFC(ToggleButtonGenerated::OnClick());

Cleanup:
    RRETURN(hr);
}

// Raises the Checked routed event.
_Check_return_ HRESULT ToggleButton::OnChecked()
{
    HRESULT hr = S_OK;
    CheckedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<RoutedEventArgs> spArgs;
    IFC(UpdateVisualState());

    // Create the args
    IFC(ctl::make<RoutedEventArgs>(&spArgs));
    IFC(spArgs->put_OriginalSource(ctl::as_iinspectable(this)));
    
    // Raise the event
    IFC(GetCheckedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

// Raises the Unchecked routed event
_Check_return_ HRESULT ToggleButton::OnUnchecked()
{
    HRESULT hr = S_OK;
    UncheckedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<RoutedEventArgs> spArgs;
    IFC(UpdateVisualState());

    // Create the args
    IFC(ctl::make<RoutedEventArgs>(&spArgs));
    IFC(spArgs->put_OriginalSource(ctl::as_iinspectable(this)));
    
    // Raise the event
    IFC(GetUncheckedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

// Raises the Indeterminate routed event
_Check_return_ HRESULT ToggleButton::OnIndeterminate()
{
    HRESULT hr = S_OK;
    IndeterminateEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<RoutedEventArgs> spArgs;
    IFC(UpdateVisualState());

    // Create the args
    IFC(ctl::make<RoutedEventArgs>(&spArgs));
    IFC(spArgs->put_OriginalSource(ctl::as_iinspectable(this)));
    
    // Raise the event
    IFC(GetIndeterminateEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

// Raises the Click routed event.
_Check_return_ HRESULT ToggleButton::OnToggleImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN bIsThreeState = FALSE;
    BOOLEAN bIsChecked = FALSE;
    ctl::ComPtr<wf::IReference<bool>> spIsCheckedReference;
    ctl::ComPtr<IInspectable> spNewValue;
    ctl::ComPtr<wf::IReference<bool>> spNewValueReference;
    SuspendFailFastOnStowedException suspender; // http://osgvsowi/6487245 - suspend fail fast until it is fixed

    IFC(get_IsChecked(&spIsCheckedReference));
    if (spIsCheckedReference)
    {
        IFC(spIsCheckedReference->get_Value(&bIsChecked));
    }
    
    if (!spIsCheckedReference)
    {
        // Indeterminate
        // Set to Unchecked
        IFC(PropertyValue::CreateFromBoolean(FALSE, &spNewValue));
        IFC(spNewValue.As(&spNewValueReference));
        IFC(put_IsChecked(spNewValueReference.Get()));
    }
    else if (bIsChecked)
    {
        // Checked
        IFC(get_IsThreeState(&bIsThreeState));
        if (bIsThreeState)
        {
            // Set to Indeterminate
            IFC(put_IsChecked(NULL));
        }
        else
        {
            // Set to Unchecked
            IFC(PropertyValue::CreateFromBoolean(FALSE, &spNewValue));
            IFC(spNewValue.As(&spNewValueReference));
            IFC(put_IsChecked(spNewValueReference.Get()));
        }
    }
    else
    {
        // Unchecked
        // Set to Checked
        IFC(PropertyValue::CreateFromBoolean(TRUE, &spNewValue));
        IFC(spNewValue.As(&spNewValueReference));
        IFC(put_IsChecked(spNewValueReference.Get()));
    }
    
Cleanup:
    RRETURN(hr);
}

// Handle the IsChecked status change, resulting in updated VSM states and raising events
_Check_return_ HRESULT ToggleButton::OnIsCheckedChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IReference<bool>> spIsCheckedReference;
    BOOLEAN bIsChecked = FALSE;

    IFC(get_IsChecked(&spIsCheckedReference));
    if (spIsCheckedReference)
    {
        IFC(spIsCheckedReference->get_Value(&bIsChecked));
    }
    
    if (!spIsCheckedReference)
    {
        // Indeterminate
        IFC(OnIndeterminate());
    }
    else if (bIsChecked)
    {
        // Checked
        IFC(OnChecked());
    }
    else
    {
        // Unchecked
        IFC(OnUnchecked());
    }
    
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToggleButton::AutomationToggleButtonOnToggle()
{
    HRESULT hr = S_OK;

    // OnToggle through UIAutomation
    IFC(OnClick());

Cleanup:
    RRETURN(hr);
}

// Create ToggleButtonAutomationPeer to represent the ToggleButton.
IFACEMETHODIMP ToggleButton::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    
    *ppAutomationPeer = NULL;
    if(!_skipCreateAutomationPeer)
    {    
        ctl::ComPtr<xaml_automation_peers::IToggleButtonAutomationPeer> spToggleButtonAutomationPeer;
        ctl::ComPtr<xaml_automation_peers::IToggleButtonAutomationPeerFactory> spToggleButtonAPFactory;
        ctl::ComPtr<IActivationFactory> spActivationFactory;
        ctl::ComPtr<IInspectable> spInner;
        
        spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ToggleButtonAutomationPeerFactory>::CreateActivationFactory());
        IFC(spActivationFactory.As(&spToggleButtonAPFactory));

        IFC(spToggleButtonAPFactory.Cast<ToggleButtonAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
            NULL, 
            &spInner, 
            &spToggleButtonAutomationPeer));
        IFC(spToggleButtonAutomationPeer.CopyTo(ppAutomationPeer));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToggleButton::SetSkipAutomationPeerCreation()
{
    _skipCreateAutomationPeer = TRUE;
    return S_OK;
}
