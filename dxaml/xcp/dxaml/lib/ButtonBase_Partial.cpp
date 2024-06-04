// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ButtonBase.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "ButtonBaseKeyProcess.h"
#include "KeyboardNavigation.h"
#include "CommandingHelpers.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the ButtonBase class.
ButtonBase::ButtonBase()
: m_bIsPointerCaptured(FALSE)
, m_bIsSpaceOrEnterKeyDown(FALSE)
, m_bIsPointerLeftButtonDown(FALSE)
, m_bKeyboardNavigationAcceptsReturn(FALSE)
, m_shouldPerformActions(FALSE)
, m_handlesKeyboardInput(TRUE)
, m_bIsNavigationAcceptOrGamepadAKeyDown(FALSE)
{
    m_pointerPosition.X = m_pointerPosition.Y = 0;
}

// Destroys an instance of the ButtonBase class.
ButtonBase::~ButtonBase()
{
}


// Prepares object's state
_Check_return_
HRESULT
ButtonBase::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLoadedEventHandler;
    EventRegistrationToken eventRegistration;

    IFC(ButtonBaseGenerated::Initialize());

    // Allow the button to respond to the ENTER key and be focused
    SetAcceptsReturn(true);

    spLoadedEventHandler.Attach(
        new ClassMemberEventHandler<
        ButtonBase,
        xaml_primitives::IButtonBase,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs>(this, &ButtonBase::OnLoaded, true /* subscribingToSelf */));

    IFC(add_Loaded(spLoadedEventHandler.Get(), &eventRegistration));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ButtonBase::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOldValue;
    ctl::ComPtr<IInspectable> spNewValue;

    IFC(ButtonBaseGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ButtonBase_ClickMode:
        IFC(OnClickModeChanged(static_cast<xaml_controls::ClickMode>(args.m_pNewValue->AsEnum())));
        break;
    case KnownPropertyIndex::ButtonBase_IsPressed:
        IFC(OnIsPressedChanged());
        break;
    case KnownPropertyIndex::ButtonBase_Command:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnCommandChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    case KnownPropertyIndex::ButtonBase_CommandParameter:
        IFC(UpdateCanExecute());
        break;
    case KnownPropertyIndex::UIElement_Visibility:
        IFC(OnVisibilityChanged());
        break;
    }

Cleanup:
    RRETURN(hr);
}

// Validate the xaml_controls::ClickMode property when its
// value is changed.
_Check_return_ HRESULT ButtonBase::OnClickModeChanged(_In_ xaml_controls::ClickMode eNewClickMode)
{
    HRESULT hr = S_OK;

    IFCEXPECT(
        eNewClickMode == xaml_controls::ClickMode_Release ||
        eNewClickMode == xaml_controls::ClickMode_Press ||
        eNewClickMode == xaml_controls::ClickMode_Hover);

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the IsPressed property is changed.
_Check_return_ HRESULT ButtonBase::OnIsPressedChanged()
{
    HRESULT hr = S_OK;

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
ButtonBase::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        IFC(ClearStateFlags());
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT ButtonBase::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;

    IFC(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(ClearStateFlags());
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Called when the element enters the tree. Attaches event handler to Command.CanExecuteChanged.
_Check_return_ HRESULT
ButtonBase::EnterImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding)
{
    IFC_RETURN(ButtonBaseGenerated::EnterImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding));

    if (bLive && !m_epCanExecuteChangedHandler)
    {
        ctl::ComPtr<ICommand> spCommand;
        IFC_RETURN(get_Command(&spCommand));

        if (spCommand)
        {
            IFC_RETURN(m_epCanExecuteChangedHandler.AttachEventHandler(static_cast<IButtonBase*>(this), spCommand.Get(),
                [this](IInspectable *pSource, IInspectable *pArgs)
            {
                return UpdateCanExecute();
            }));
        }

        // In case we missed an update to CanExecute while the CanExecuteChanged handler was unhooked,
        // we need to update our value now.
        IFC_RETURN(UpdateCanExecute());
    }

    return S_OK;
}

// Called when the element leaves the tree. Detaches event handler from Command.CanExecuteChanged.
_Check_return_ HRESULT
ButtonBase::LeaveImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bVisualTreeBeingReset)
{
    IFC_RETURN(ButtonBaseGenerated::LeaveImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset));

    if (bLive && m_epCanExecuteChangedHandler)
    {
        ctl::ComPtr<ICommand> spCommand;
        IFC_RETURN(get_Command(&spCommand));

        if (spCommand)
        {
            IFC_RETURN(m_epCanExecuteChangedHandler.DetachEventHandler(spCommand.Get()));
        }
    }

    return S_OK;
}

// Clear flags relating to the visual state.  Called when IsEnabled is set to FALSE
// or when Visibility is set to Hidden or Collapsed.
_Check_return_ HRESULT
ButtonBase::ClearStateFlags()
{
    HRESULT hr = S_OK;

    SUSPEND_STATE_CHANGES();
    {
        IFC(put_IsPressed(FALSE));
        IFC(put_IsPointerOver(FALSE));
        m_bIsPointerCaptured = FALSE;
        m_bIsSpaceOrEnterKeyDown = FALSE;
        m_bIsPointerLeftButtonDown = FALSE;
        m_bIsNavigationAcceptOrGamepadAKeyDown = FALSE;
    }
    RESUME_STATE_CHANGES();

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   OnCommandChanged
//
//  Synopsis:
//      Called when ButtonBase.Command property changes.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT ButtonBase::OnCommandChanged(
    _In_  IInspectable* pOldValue,
    _In_ IInspectable* pNewValue
    )
{
    // Remove handler for CanExecuteChanged from the old value
    if (m_epCanExecuteChangedHandler)
    {
        IFC_RETURN(m_epCanExecuteChangedHandler.DetachEventHandler(pOldValue));
    }

    if (pOldValue)
    {
        ctl::ComPtr<ICommand> oldCommand;
        IFC_RETURN(ctl::do_query_interface(oldCommand, pOldValue));

        ctl::ComPtr<IXamlUICommand> oldCommandAsUICommand = oldCommand.AsOrNull<IXamlUICommand>();

        if (oldCommandAsUICommand)
        {
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::ContentControl_Content));
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::UIElement_KeyboardAccelerators));
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::UIElement_AccessKey));
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::AutomationProperties_HelpText));
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::ToolTipService_ToolTip));
        }
    }

    // Subscribe to the CanExecuteChanged event on the new value
    if (pNewValue)
    {
        ctl::ComPtr<ICommand> newCommand;

        IFC_RETURN(ctl::do_query_interface(newCommand, pNewValue));
        IFC_RETURN(m_epCanExecuteChangedHandler.AttachEventHandler(static_cast<IButtonBase*>(this), newCommand.Get(),
            [this](IInspectable *pSource, IInspectable *pArgs)
        {
            return UpdateCanExecute();
        }));

        ctl::ComPtr<IXamlUICommand> newCommandAsUICommand = newCommand.AsOrNull<IXamlUICommand>();

        if (newCommandAsUICommand)
        {
            IFC_RETURN(CommandingHelpers::BindToLabelPropertyIfUnset(newCommandAsUICommand.Get(), this, KnownPropertyIndex::ContentControl_Content));
            IFC_RETURN(CommandingHelpers::BindToKeyboardAcceleratorsIfUnset(newCommandAsUICommand.Get(), this));
            IFC_RETURN(CommandingHelpers::BindToAccessKeyIfUnset(newCommandAsUICommand.Get(), this));
            IFC_RETURN(CommandingHelpers::BindToDescriptionPropertiesIfUnset(newCommandAsUICommand.Get(), this));
        }
    }

    // Coerce the button enabled state with the CanExecute state of the command.
    IFC_RETURN(UpdateCanExecute());

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   UpdateCanExecute
//
//  Synopsis:
//      Coerces button enabled state with CanExecute state of the command.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ButtonBase::UpdateCanExecute()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICommand> spCommand;
    ctl::ComPtr<IInspectable> spCommandParameter;
    bool suppress;
    BOOLEAN canExecute = TRUE;

    IFC(get_Command(&spCommand));
    if (spCommand)
    {
        IFC(get_CommandParameter(&spCommandParameter));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(spCommand->CanExecute(spCommandParameter.Get(), &canExecute));
    }

    // If command is present and cannot be executed, disable the button.
    suppress = !canExecute;
    static_cast<CControl*>(GetHandle())->SuppressIsEnabled(!canExecute);

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   ExecuteCommand
//
//  Synopsis:
//      Executes ButtonBase.Command.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ButtonBase::ExecuteCommand()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICommand> spCommand;
    ctl::ComPtr<IInspectable> spCommandParameter;
    BOOLEAN canExecute;

    IFC(get_Command(&spCommand));
    if (spCommand)
    {
        IFC(get_CommandParameter(&spCommandParameter));
        
        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(spCommand->CanExecute(spCommandParameter.Get(), &canExecute));
        IFC(hr);

        if (canExecute)
        {
            // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
            IFC(spCommand->Execute(spCommandParameter.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Loaded event handler.
_Check_return_ HRESULT ButtonBase::OnLoaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(UpdateVisualState(FALSE));

Cleanup:
    RRETURN(hr);
}

// GotFocus event handler.
IFACEMETHODIMP ButtonBase::OnGotFocus(
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Release pointer capture if we already had it.
_Check_return_ HRESULT ButtonBase::ReleasePointerCaptureInternal(
    _In_ xaml_input::IPointer* pointer)
{
    HRESULT hr = S_OK;
    if (pointer == nullptr)
    {   //no pointer available so clear all captures.
        IFC(ReleasePointerCaptures());
    }
    else
    {
        IFC(ReleasePointerCapture(pointer));
    }

    m_bIsPointerCaptured = FALSE;

Cleanup:
    RRETURN(hr);
}

// LostFocus event handler.
IFACEMETHODIMP ButtonBase::OnLostFocus(
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;

    BOOLEAN hasFocus = FALSE;
    IFC(HasFocus(&hasFocus));

    if (!hasFocus)
    {
        SUSPEND_STATE_CHANGES();
        {
            IFC(get_ClickMode(&nClickMode));
            if (nClickMode != xaml_controls::ClickMode_Hover)
            {
                IFC(put_IsPressed(FALSE));
                IFC(ReleasePointerCaptureInternal(NULL));
                m_bIsSpaceOrEnterKeyDown = FALSE;
                m_bIsNavigationAcceptOrGamepadAKeyDown = FALSE;
            }
        }
        RESUME_STATE_CHANGES();
    }

Cleanup:
    RRETURN(hr);
}

// KeyDown event handler.
IFACEMETHODIMP ButtonBase::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bHandled = FALSE;
    wsy::VirtualKey nKey = wsy::VirtualKey_None;

    IFCPTR(pArgs);
    IFC(pArgs->get_Handled(&bHandled));
    if (bHandled || !m_handlesKeyboardInput)
    {
        goto Cleanup;
    }

    IFC(pArgs->get_Key(&nKey));
    IFC(OnKeyDownInternal(nKey, &bHandled));
    if (bHandled)
    {
        IFC(pArgs->put_Handled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// Handles the KeyDown event for ButtonBase.
//
// Note: ENTER previously did not use xaml_controls::ClickMode & was processed on first
// keydown. However keydown events are sent synchronously, so if the button was
// clicked using the ENTER key, the click handler could not execute code that
// caused reentrancy, like displaying a dialog. Such handlers are common. To
// solve this, ENTER was changed to use xaml_controls::ClickMode.  The default xaml_controls::ClickMode is
// xaml_controls::ClickMode.Release, and keyup events are sent asynchronously, so in the
// default case, click handlers can display dialogs or execute other code that
// causes reentrancy. If the xaml_controls::ClickMode is changed by the app to xaml_controls::ClickMode.Press,
// the click handler will not be able to execute code that causes reentrancy.
_Check_return_ HRESULT ButtonBase::OnKeyDownInternal(
    _In_ wsy::VirtualKey nKey,
    _Out_ BOOLEAN* pbHandled)
{
    HRESULT hr = S_OK;

    IFCPTR(pbHandled);

    IFC(KeyPress::ButtonBase::KeyDown<ButtonBase>(nKey, pbHandled, m_bKeyboardNavigationAcceptsReturn, this));

Cleanup:
    RRETURN(hr);
}

// KeyUp event handler.
IFACEMETHODIMP ButtonBase::OnKeyUp(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bHandled = FALSE;
    wsy::VirtualKey nKey = wsy::VirtualKey_None;

    IFCPTR(pArgs);
    IFC(pArgs->get_Handled(&bHandled));
    if (bHandled || !m_handlesKeyboardInput)
    {
        goto Cleanup;
    }

    IFC(pArgs->get_Key(&nKey));
    OnKeyUpInternal(nKey, &bHandled);
    if (bHandled)
    {
        IFC(pArgs->put_Handled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// Handle key up events.
void ButtonBase::OnKeyUpInternal(
    _In_ wsy::VirtualKey nKey,
    _Out_ BOOLEAN* pbHandled)
{
    KeyPress::ButtonBase::KeyUp<ButtonBase>(nKey, pbHandled, m_bKeyboardNavigationAcceptsReturn, this);
}

// PointerEnter event handler.
IFACEMETHODIMP ButtonBase::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;
    BOOLEAN bIsEnabled = FALSE;

    IFC(put_IsPointerOver(TRUE));

    SUSPEND_STATE_CHANGES();
    {
        IFC(get_ClickMode(&nClickMode));
        IFC(get_IsEnabled(&bIsEnabled));
        if (nClickMode == xaml_controls::ClickMode_Hover && bIsEnabled)
        {
            IFC(put_IsPressed(TRUE));
            IFC(OnClick());
        }
    }
    RESUME_STATE_CHANGES();

Cleanup:
    RRETURN(hr);
}

// PointerExited event handler.
IFACEMETHODIMP ButtonBase::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;
    BOOLEAN bIsEnabled = FALSE;

    IFC(put_IsPointerOver(FALSE));

    SUSPEND_STATE_CHANGES();
    {
        IFC(get_ClickMode(&nClickMode));
        IFC(get_IsEnabled(&bIsEnabled));
        if (nClickMode == xaml_controls::ClickMode_Hover && bIsEnabled)
        {
            IFC(put_IsPressed(FALSE));
        }
    }
    RESUME_STATE_CHANGES();

Cleanup:
    RRETURN(hr);
}

// Determine if the pointer is above the button based on its last known position.
_Check_return_ HRESULT ButtonBase::IsValidPointerPosition(
    _Out_ BOOLEAN* pbIsValid)
{
    HRESULT hr = S_OK;
    DOUBLE dActualWidth;
    DOUBLE dActualHeight;

    IFCPTR(pbIsValid);
    *pbIsValid = FALSE;

    IFC(get_ActualWidth(&dActualWidth));
    IFC(get_ActualHeight(&dActualHeight));

    // This method is used to check mouse position after a mouse down and a drag. If the mouse moves outside the bounds
    // of the button, then we treat it as not pressed anymore. The cached m_pointerPosition comes from ::Windows::UI::Input's
    // PointerPoint::get_Position, which uses a HimetricPoint that has subpixel precision, beyond even the plateau scale.
    // All other hit tests in Xaml use pixels or DIPs that have been rounded before Xaml receives the point. The HimetricPoint
    // can have precision issues after being converted to DIPs, so we allow a tolerance when doing the bounds comparison here.
    static const double tolerance = 0.05;

    *pbIsValid =
        -tolerance <= m_pointerPosition.X && m_pointerPosition.X <= dActualWidth + tolerance &&
        -tolerance <= m_pointerPosition.Y && m_pointerPosition.Y <= dActualHeight + tolerance;

Cleanup:
    RRETURN(hr);
}

// PointerMoved event handler.
IFACEMETHODIMP ButtonBase::OnPointerMoved(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;

    // Cache the last pointer position
    IFCPTR(pArgs);
    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
    IFC(spPointerPoint->get_Position(&m_pointerPosition));

    // Determine if the button is still pressed near the pointer's position
    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_ClickMode(&nClickMode));
    if (m_bIsPointerLeftButtonDown &&
        bIsEnabled &&
        nClickMode != xaml_controls::ClickMode_Hover &&
        m_bIsPointerCaptured &&
        !m_bIsSpaceOrEnterKeyDown && !m_bIsNavigationAcceptOrGamepadAKeyDown)
    {
        BOOLEAN bIsValid = FALSE;
        IFC(IsValidPointerPosition(&bIsValid));
        IFC(put_IsPressed(bIsValid));
    }

Cleanup:
    RRETURN(hr);
}

// Capture the pointer.
_Check_return_ HRESULT ButtonBase::CapturePointerInternal(
    _In_ xaml_input::IPointer* pPointer)
{
    HRESULT hr = S_OK;

    if (!m_bIsPointerCaptured)
    {
        IFC(CapturePointer(pPointer, &m_bIsPointerCaptured));
    }

Cleanup:
    RRETURN(hr);
}


// PointerPressed event handler.
IFACEMETHODIMP ButtonBase::OnPointerPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bHandled = FALSE;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsFocused = FALSE;
    BOOLEAN bIsLeftButtonPressed = FALSE;
    xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

    IFCPTR(pArgs);

    IFC(pArgs->get_Handled(&bHandled));
    if (bHandled)
    {
        goto Cleanup;
    }

    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_Properties(&spPointerProperties));
    IFCPTR(spPointerProperties);
    IFC(spPointerProperties->get_IsLeftButtonPressed(&bIsLeftButtonPressed));

    if (bIsLeftButtonPressed)
    {
        m_bIsPointerLeftButtonDown = TRUE;
        IFC(get_IsEnabled(&bIsEnabled));
        IFC(get_ClickMode(&nClickMode));
        if (!bIsEnabled || nClickMode == xaml_controls::ClickMode_Hover)
        {
            goto Cleanup;
        }

        IFC(pArgs->put_Handled(TRUE));

        SUSPEND_STATE_CHANGES();
        {
            IFC(Focus(xaml::FocusState_Pointer, &bIsFocused));

            IFC(pArgs->get_Pointer(&spPointer));
            IFC(CapturePointerInternal(spPointer.Get()));
            if (m_bIsPointerCaptured)
            {
                IFC(put_IsPressed(TRUE));
            }
        }
        RESUME_STATE_CHANGES();

        if (nClickMode == xaml_controls::ClickMode_Press)
        {
            IFC(OnClick());
        }

    }
Cleanup:
    RRETURN(hr);
}

// PointerReleased event handler.
IFACEMETHODIMP ButtonBase::OnPointerReleased(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    GestureModes gestureFollowing = GestureModes::None;
    BOOLEAN bHandled = FALSE;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsPressed = FALSE;
    xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;
    ctl::ComPtr<xaml_input::IPointer> spPointer;

    IFCPTR(pArgs);
    IFC(pArgs->get_Handled(&bHandled));
    if (bHandled)
    {
        goto Cleanup;
    }

    //this should be changed to Pointer down and only occur when pointer is down or mouse left button is down.
    m_bIsPointerLeftButtonDown = FALSE;

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_ClickMode(&nClickMode));
    if (!bIsEnabled || nClickMode == xaml_controls::ClickMode_Hover)
    {
        goto Cleanup;
    }

    IFC(get_IsPressed(&bIsPressed));

    // We can not put all our logic for this event inside an if block conditioning to this BOOLEAN, as we need to
    // release our pointer capture and set is pressed to false depending solely on m_bIsSpaceOrEnterKeyDown
    m_shouldPerformActions = (bIsPressed && !m_bIsSpaceOrEnterKeyDown && !m_bIsNavigationAcceptOrGamepadAKeyDown);

    m_tpPointerForPendingRightTapped.Clear();
    if (!m_bIsSpaceOrEnterKeyDown && !m_bIsNavigationAcceptOrGamepadAKeyDown)
    {
        ctl::ComPtr<xaml_input::IPointer> spPointerTemp;
        IFC(put_IsPressed(FALSE));
        IFC(pArgs->get_Pointer(&spPointerTemp));
        SetPtrValue(m_tpPointerForPendingRightTapped, spPointerTemp);
    }
    IFC(static_cast<PointerRoutedEventArgs*>(pArgs)->get_GestureFollowing(&gestureFollowing));
    if (gestureFollowing == GestureModes::RightTapped)
    {
        // This will be released OnRightTappedUnhandled or destructor.
        goto Cleanup;
    }

    // No right tap is pending. Note that we are intentionally NOT handling the args
    // if we do not fall through here because basically we are no_opting in that case.
    IFC(pArgs->put_Handled(TRUE));
    IFC(PerformPointerUpAction());
    if (!m_bIsSpaceOrEnterKeyDown && !m_bIsNavigationAcceptOrGamepadAKeyDown)
    {
        IFC(pArgs->get_Pointer(&spPointer));
        IFC(ReleasePointerCaptureInternal(spPointer.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ButtonBase::PerformPointerUpAction()
{
    HRESULT hr = S_OK;
    xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;

    IFC(get_ClickMode(&nClickMode));
    if (nClickMode == xaml_controls::ClickMode_Release &&
        m_shouldPerformActions)
    {
        IFC(OnClick());
    }

Cleanup:
    m_shouldPerformActions = FALSE;
    RRETURN(hr);
}

// PointerCaptureLost event handler.
IFACEMETHODIMP ButtonBase::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

    IFC(ButtonBaseGenerated::OnPointerCaptureLost(pArgs));

    IFC(pArgs->get_Pointer(&spPointer));
    IFC(ReleasePointerCaptureInternal(spPointer.Get()));

    // For touch, we can clear PointerOver when receiving PointerCaptureLost, which we get when the finger is lifted
    // or from cancellation, e.g. pinch-zoom gesture in ScrollViewer.
    // For mouse, we need to wait for PointerExited because the mouse may still be above the ButtonBase when
    // PointerCaptureLost is received from clicking.
    IFC(pArgs->GetCurrentPoint(nullptr, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));
    if (nPointerDeviceType == mui::PointerDeviceType_Touch)
    {
        IFC(put_IsPointerOver(FALSE));
    }

    SUSPEND_STATE_CHANGES();
    {
        IFC(put_IsPressed(FALSE));
    }
    RESUME_STATE_CHANGES();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ButtonBase::OnRightTappedUnhandled(
    _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFC(ButtonBaseGenerated::OnRightTappedUnhandled(pArgs));
    IFC(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        goto Cleanup;
    }

    IFC(PerformPointerUpAction());
    IFC(ReleasePointerCaptureInternal(m_tpPointerForPendingRightTapped.Get()));

Cleanup:
    m_tpPointerForPendingRightTapped.Clear();
    RRETURN(hr);
}

// Apply a template to the ButtonBase.
IFACEMETHODIMP ButtonBase::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    IFC(ButtonBaseGenerated::OnApplyTemplate());

    // Sync the logical and visual states of the control
    IFC(UpdateVisualState(FALSE));

Cleanup:
    RRETURN(hr);
}

// Raises the Click routed event.
_Check_return_ HRESULT ButtonBase::OnClick()
{
    // Request a play invoke sound for Click event. This call needs to be made before the click event is raised.
    // This is because RequestInteractionSoundForElement expects the element to be in the tree.
    // And some click handlers remove the button from the tree e.g. an accept/cancel button on a Popup or Flyout.
    IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Invoke, this));

    ClickEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<RoutedEventArgs> spArgs;

    // Create the args
    IFC_RETURN(ctl::make(&spArgs));
    IFC_RETURN(spArgs->put_OriginalSource(ctl::as_iinspectable(this)));

    // Raise the event
    IFC_RETURN(GetClickEventSourceNoRef(&pEventSource));
    IFC_RETURN(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

    // Execute command associated with the button
    IFC_RETURN(ExecuteCommand());

    return S_OK;
}

_Check_return_ HRESULT ButtonBase::ProgrammaticClick()
{
    return OnClick();
}

 void ButtonBase::SetAcceptsReturn(bool value)
 {
     m_bKeyboardNavigationAcceptsReturn = value;
 }