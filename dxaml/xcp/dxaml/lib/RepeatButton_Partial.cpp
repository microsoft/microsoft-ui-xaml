// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RepeatButton.g.h"
#include "RepeatButtonAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

RepeatButton::RepeatButton()
    : m_keyboardCausingRepeat(FALSE)
    , m_pointerCausingRepeat(FALSE)
    , m_ignoreTouchInput(FALSE)
{
}

RepeatButton::~RepeatButton()
{ }

// Prepares object's state
_Check_return_ 
HRESULT 
RepeatButton::Initialize()
{
    HRESULT hr = S_OK;
    IFC(RepeatButtonGenerated::Initialize());

    IFC(put_ClickMode(xaml_controls::ClickMode_Press));

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the repeat button.
_Check_return_ HRESULT RepeatButton::ChangeVisualState(
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

    // TODO: Declare string constants for each state.
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
_Check_return_ HRESULT RepeatButton::OnClick()
{
    HRESULT hr = S_OK;
    BOOLEAN bAutomationListener = FALSE;

    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_InvokePatternOnInvoked, &bAutomationListener));
    if (bAutomationListener)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
        
        IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
        if(spAutomationPeer)
        {
            IFC(spAutomationPeer->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_InvokePatternOnInvoked));
        }
    }

    IFC(RepeatButtonGenerated::OnClick());

Cleanup:
    RRETURN(hr);
}

// Handle the custom property changed event and call the
// OnPropertyChanged2 methods. 
_Check_return_ 
HRESULT 
RepeatButton::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(RepeatButtonGenerated::OnPropertyChanged2(args));
    
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::RepeatButton_Delay:
            {
                ctl::ComPtr<IInspectable> spNewValue;
                IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
                IFC(OnDelayPropertyChanged(spNewValue.Get()));
            }
            break;

        case KnownPropertyIndex::RepeatButton_Interval:
            {
                ctl::ComPtr<IInspectable> spNewValue;
                IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
                IFC(OnIntervalPropertyChanged(spNewValue.Get())); 
            }
            break;
    }

Cleanup:
    RRETURN(hr);
}

// Called when the Delay value changed.
_Check_return_ HRESULT RepeatButton::OnDelayPropertyChanged(_In_ IInspectable* pNewDelay)
{
    HRESULT hr = S_OK;
    INT newDelay;

    IFC(ctl::do_get_value(newDelay, pNewDelay));
    if (newDelay < 0)
    {
        // TODO: return special error code. SL for instance does:
        // throw new ArgumentException(Resx.GetString(Resx.RepeatButton_DelayPropertyCannotBeNegative), DelayProperty.ToString());
        IFC(E_FAIL);
    }

Cleanup:
    RRETURN(hr);
}
            
// Called when the Interval value changed.
_Check_return_ HRESULT RepeatButton::OnIntervalPropertyChanged(_In_ IInspectable* pNewInterval)
{
    HRESULT hr = S_OK;
    INT newInterval = 0;

    IFC(ctl::do_get_value(newInterval, pNewInterval));
    if (newInterval <= 0)
    {
        // TODO: return special error code. SL for instance does:
        // throw new ArgumentException(Resx.GetString(Resx.RepeatButton_IntervalMustBePositive), IntervalProperty.ToString());
        IFC(E_FAIL);
    }

Cleanup:
    RRETURN(hr);
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT RepeatButton::OnIsEnabledChanged(
    _In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(RepeatButtonGenerated::OnIsEnabledChanged(pArgs));

    m_keyboardCausingRepeat = false;
    m_pointerCausingRepeat = false;
    IFC(UpdateRepeatState());

Cleanup:
    RRETURN(hr);
}

// KeyDown event handler.
IFACEMETHODIMP RepeatButton::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    wsy::VirtualKey nKey = wsy::VirtualKey_None;
    xaml_controls::ClickMode nClickMode;

    IFCPTR(pArgs);
    IFC(pArgs->get_Key(&nKey));
    IFC(get_ClickMode(&nClickMode));
    
    if ((nKey == wsy::VirtualKey_Space) && 
        (nClickMode != xaml_controls::ClickMode_Hover))
    {
        m_keyboardCausingRepeat = true;
        IFC(UpdateRepeatState());
    }

    IFC(RepeatButtonGenerated::OnKeyDown(pArgs));

Cleanup:
    RRETURN(hr);
}

// KeyUp event handler.
IFACEMETHODIMP RepeatButton::OnKeyUp(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    wsy::VirtualKey nKey = wsy::VirtualKey_None;
    xaml_controls::ClickMode nClickMode;

    IFCPTR(pArgs);
    IFC(pArgs->get_Key(&nKey));
    IFC(get_ClickMode(&nClickMode));

    IFC(RepeatButtonGenerated::OnKeyUp(pArgs));

    if ((nKey == wsy::VirtualKey_Space) && 
        (nClickMode != xaml_controls::ClickMode_Hover))
    {
        m_keyboardCausingRepeat = false;
        IFC(UpdateRepeatState());
    }
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// LostFocus event handler.
IFACEMETHODIMP RepeatButton::OnLostFocus(
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    xaml_controls::ClickMode nClickMode;

    IFCPTR(pArgs);
    IFC(RepeatButtonGenerated::OnLostFocus(pArgs));
    IFC(get_ClickMode(&nClickMode));

    if (nClickMode != xaml_controls::ClickMode_Hover)
    {
        m_keyboardCausingRepeat = false;
        m_pointerCausingRepeat = false;
        IFC(UpdateRepeatState());
    }

Cleanup:
    RRETURN(hr);
}

// Sets a value indicating whether the RepeatButton reacts to touch input or not.
_Check_return_ HRESULT RepeatButton::put_IgnoreTouchInput(
    _In_ BOOLEAN value)
{
    m_ignoreTouchInput = value;
    RRETURN(S_OK);
}

_Check_return_ HRESULT RepeatButton::ShouldIgnoreInput(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs, 
    _Out_ BOOLEAN* pIgnoreInput)
{
    HRESULT hr = S_OK;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

    IFCPTR(pArgs);
    IFCPTR(pIgnoreInput);
    *pIgnoreInput = FALSE;

    if (m_ignoreTouchInput)
    {
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        
        IFC(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));
        if (nPointerDeviceType == mui::PointerDeviceType_Touch)
        {
            *pIgnoreInput = TRUE;
        }
    }
Cleanup:
    RRETURN(hr);
}

// PointerEnter event handler.
IFACEMETHODIMP RepeatButton::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN pIgnoreInput = FALSE;
    xaml_controls::ClickMode nClickMode;

    IFCPTR(pArgs);

    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }

    IFC(RepeatButtonGenerated::OnPointerEntered(pArgs));
    IFC(get_ClickMode(&nClickMode));

    if (nClickMode == xaml_controls::ClickMode_Hover)
    {
        m_pointerCausingRepeat = true;
    }           
    IFC(UpdateRepeatState());
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// PointerMoved event handler.
IFACEMETHODIMP RepeatButton::OnPointerMoved(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    /// The reason this function does not call the base 
    /// OnPointerMove is that the ButtonBase class sometimes 
    /// sets IsPressed to false based on mouse position. This 
    /// interferes with the RepeatButton functionality, which
    /// relies on IsPressed being True in Hover mode incorrectly.
    HRESULT hr = S_OK;

    IFCPTR(pArgs);

Cleanup:
    RRETURN(hr);
}

// PointerLeave event handler.
IFACEMETHODIMP RepeatButton::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN pIgnoreInput = FALSE;
    xaml_controls::ClickMode nClickMode;

    IFCPTR(pArgs);

    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }

    IFC(RepeatButtonGenerated::OnPointerExited(pArgs));
    IFC(get_ClickMode(&nClickMode));

    if (nClickMode == xaml_controls::ClickMode_Hover)     
    {
        m_pointerCausingRepeat = false;
        IFC(UpdateRepeatState());
    }
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// PointerPressed event handler.
IFACEMETHODIMP RepeatButton::OnPointerPressed(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
    BOOLEAN bHandled = FALSE;
    BOOLEAN pIgnoreInput = FALSE;
    BOOLEAN bIsLeftButtonPressed = FALSE;
    xaml_controls::ClickMode nClickMode;

    IFCPTR(pArgs);
    IFC(pArgs->get_Handled(&bHandled));
    if (bHandled)
    {
        goto Cleanup;
    }

    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }

    IFC(RepeatButtonGenerated::OnPointerPressed(pArgs));

    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_Properties(&spPointerProperties));
    IFCPTR(spPointerProperties);
    IFC(spPointerProperties->get_IsLeftButtonPressed(&bIsLeftButtonPressed));


    if(bIsLeftButtonPressed)
    {
        IFC(get_ClickMode(&nClickMode));

        if (nClickMode != xaml_controls::ClickMode_Hover)
        {
            m_pointerCausingRepeat = true;
            IFC(UpdateRepeatState());
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerReleased event handler.
IFACEMETHODIMP RepeatButton::OnPointerReleased(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN pIgnoreInput = FALSE;
    BOOLEAN bHandled = FALSE;
    xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;

    IFCPTR(pArgs);
    IFC(pArgs->get_Handled(&bHandled));
    if (bHandled)
    {
        goto Cleanup;
    }

    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }

    IFC(RepeatButtonGenerated::OnPointerReleased(pArgs));

    IFC(get_ClickMode(&nClickMode));

    if (nClickMode != xaml_controls::ClickMode_Hover)
    {
        m_pointerCausingRepeat = false;
        IFC(UpdateRepeatState());
    }
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Create RepeatButtonAutomationPeer to represent the RepeatButton.
IFACEMETHODIMP RepeatButton::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IRepeatButtonAutomationPeer> spRepeatButtonAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IRepeatButtonAutomationPeerFactory> spRepeatButtonAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::RepeatButtonAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spRepeatButtonAPFactory));

    IFC(spRepeatButtonAPFactory.Cast<RepeatButtonAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
        NULL, 
        &spInner, 
        &spRepeatButtonAutomationPeer));
    IFC(spRepeatButtonAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RepeatButton::StartTimer()
{
    HRESULT hr = S_OK;
    INT delay = 0;

    if (!m_tpTimer)
    {
        ctl::ComPtr<TimelineTimer> spTimer;

        IFC(ctl::make<TimelineTimer>(this, &spTimer));
        SetPtrValue(m_tpTimer, spTimer);
    }

    if (!m_tpTimer->get_IsEnabled())
    {
        IFC(get_Delay(&delay));
        IFC(m_tpTimer->put_Interval(delay));
        IFC(m_tpTimer->Start());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RepeatButton::StopTimer()
{
    HRESULT hr = S_OK;

    if (m_tpTimer && m_tpTimer->get_IsEnabled())
    {
        IFC(m_tpTimer->Stop());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RepeatButton::UpdateRepeatState()
{
    HRESULT hr = S_OK;

    BOOLEAN pointerOver = FALSE;
    IFC(get_IsPointerOver(&pointerOver));

    if ((m_pointerCausingRepeat && pointerOver) || m_keyboardCausingRepeat) 
    {
        IFC(StartTimer());
    }
    else
    {
        IFC(StopTimer());
    }           

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RepeatButton::TickCallback()
{
    HRESULT hr = S_OK;
    BOOLEAN bIsPressed = FALSE;
    BOOLEAN bIsPointerOver = FALSE;

    INT interval = 0;

    IFC(get_Interval(&interval));

    if (m_tpTimer->get_Interval() != interval)
    {
        IFC(m_tpTimer->put_Interval(interval));
    }

    IFC(get_IsPressed(&bIsPressed));
    IFC(get_IsPointerOver(&bIsPointerOver));
    if ((bIsPressed && bIsPointerOver) || (bIsPressed && m_keyboardCausingRepeat))
    {
        IFC(OnClick());
    }
    else
    {
        IFC(StopTimer());
    }

Cleanup:
    RRETURN(hr);
}
