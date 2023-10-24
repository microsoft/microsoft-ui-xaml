// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputManager.h"

#include "KeyboardInputProcessor.h"
#include "KeyboardEventArgs.h"

#include "corep.h"

#include "XboxUtility.h"
#include "KeyboardUtility.h"
#include "RemapVirtualKey.h"

#include "FocusMovement.h"

#include "CaretBrowsingGlobal.h"
#include "FxCallbacks.h"

#include "RootVisual.h"
#include "CharacterReceivedEventArgs.h"

#include "KeyboardAcceleratorUtility.h"

#include "MUX-ETWEvents.h"
#include "CoreImports.h"

#include "ScrollContentControl.h"
#include "PALInputPaneInteraction.h"

#include "FocusObserver.h"

#include "DOPointerCast.h"
#include "TextBoxBase.h"

using namespace ContentRootInput;

KeyboardInputProcessor::KeyboardInputProcessor(_In_ CInputManager& inputManager)
    : m_inputManager(inputManager)
{
}

_Check_return_ HRESULT KeyboardInputProcessor::ProcessKeyboardInput(
    _In_ const wsy::VirtualKey virtualKey,
    _In_ const PhysicalKeyStatus& keyStatus,
    _In_ const MessageMap msgId,
    _In_opt_ const wrl_wrappers::HString* const deviceId, //TODO: deviceId is no longer used and can be removed.
    _In_ const bool isSecondaryMessage,
    _In_ const XHANDLE platformPacket,
    _Out_ bool* handled)
{
    xref_ptr<CKeyEventArgs> keyEventArgs;

    const bool hasCharacterCode = (msgId == XCP_CHAR || msgId == XCP_DEADCHAR) && virtualKey != 0;
    XUINT32 modifierKeys = 0;

    IFC_RETURN(gps->GetKeyboardModifiersState(&modifierKeys));

    bool accessKeyHandled = false;
    m_shouldAllowRequestFocusSound = true;

    auto cleanupGuard = wil::scope_exit([&]
    {
        m_shouldAllowRequestFocusSound = false;

        if (m_inputManager.m_coreServices.IsTSF3Enabled())
        {
            BOOLEAN handledShouldNotImpedeTextInput = FALSE;
            IFCFAILFAST(keyEventArgs->get_HandledShouldNotImpedeTextInput(&handledShouldNotImpedeTextInput));

            TextInputProducerHelper& textInputProducerHelper = m_inputManager.m_coreServices.GetInputServices()->GetTextInputProducerHelper();

            // Disable possible text insertion associated with this key if KeyDown is handled.
            // In order to implement keyboard accelerator effectively for content dialog,
            // each input key is marked handled though it's not actual keyboard accelerator.
            // In that case we will skip SetKeyDownHandled for all those keys
            // This is required on any InputService enabled build because characters are sent
            // to RichEdit directly without go through XAML.
            if (((m_fKeyDownHandled && !handledShouldNotImpedeTextInput) || accessKeyHandled) && textInputProducerHelper.IsValid())
            {
                textInputProducerHelper.SetKeyDownHandled();
            }
        }
    });

    const auto contentRoot = m_inputManager.GetContentRoot();

    // We only wait when injecting WM_KeyDown and WM_KeyUp
    // We do should not wait WM_CHAR events as we cannot inject them
    if (msgId == XCP_KEYDOWN || msgId == XCP_KEYUP)
    {
        m_inputManager.m_coreServices.SetKeyboardInputEvent();
    }

    const bool sipIsOpen = m_inputManager.m_inputPaneProcessor.IsSipOpen();
    // Set the last input device type.
    if (XboxUtility::IsGamepadNavigationInput(virtualKey))
    {
        m_inputManager.m_inputDeviceCache.SetLastInputDeviceType(DirectUI::InputDeviceType::GamepadOrRemote);
    }
    else if (!sipIsOpen || (sipIsOpen && m_inputManager.m_inputDeviceCache.GetLastInputDeviceType() != DirectUI::InputDeviceType::GamepadOrRemote))
    {
        m_inputManager.m_inputDeviceCache.SetLastInputDeviceType(DirectUI::InputDeviceType::Keyboard);
    }

    // Create the DO that represents the event args
    keyEventArgs.init(new CKeyEventArgs());

    // set the platformKeyCode value

    keyEventArgs->m_originalKeyCode = virtualKey;
    keyEventArgs->m_platformKeyCode = InputUtility::RemapVirtualKey(virtualKey);

    // set the modifier keys
    keyEventArgs->SetModifierKeys(modifierKeys);

    // Check for platform-specific edit key combinations.
    keyEventArgs->m_xEditKey = InputUtility::Keyboard::TranslateEditKey(modifierKeys, keyEventArgs->m_platformKeyCode);

    // Set the keyboard physical status
    keyEventArgs->m_physicalKeyStatus = keyStatus;

    if (deviceId != nullptr)
    {
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(deviceId->Get(), &keyEventArgs->m_deviceId));
    }

    // Call raise event AND reset handled to false if this is not ours
    *handled = TRUE;

    const bool focusNavigationKey = IsFocusNavigationKey(keyEventArgs->m_platformKeyCode);
    m_inputManager.m_inputDeviceCache.SetLastInputWasNonFocusNavigationKeyFromSIP(sipIsOpen && !focusNavigationKey);

    // Make an input method with the params the AK and input interceptor needs
    InputMessage inputMsg;
    inputMsg.m_msgID = msgId;
    inputMsg.m_physicalKeyStatus = keyStatus;
    inputMsg.m_modifierKeys = modifierKeys;
    inputMsg.m_platformKeyCode = virtualKey;

    //We want this message to first be processed by AccessKeys before it is processed by XAML. We do this because
    //once an accesskey is pressed, we've considered it handled.
    if (msgId == XCP_KEYUP || msgId == XCP_KEYDOWN)
    {
        IFC_RETURN(contentRoot->GetAKExport().TryProcessInputForAccessKey(&inputMsg, &accessKeyHandled));

        if (accessKeyHandled)
        {
            return S_OK;
        }
    }

    const auto focusManager = contentRoot->GetFocusManagerNoRef();
    if (!focusManager)
    {
        return S_OK;
    }
    // Get focused element here since input interceptor may change focused UI element.
    xref_ptr<CDependencyObject> pSource(focusManager->GetFocusedElementNoRef());

    pSource = GetKeyRoutedSource(pSource);
    // set the Source value
    IFC_RETURN(keyEventArgs->put_Source(pSource));

    switch (msgId)
    {
    case XCP_KEYUP:
    {
        IFC_RETURN(ProcessXCPKeyup(pSource, keyEventArgs, handled));
        break;
    }
    case XCP_KEYDOWN:
    {
        IFC_RETURN(ProcessXCPKeydown(pSource, keyEventArgs, msgId, hasCharacterCode, modifierKeys, isSecondaryMessage, platformPacket, handled));

        IFC_RETURN(m_inputManager.GetContextMenuProcessor().ProcessContextRequestOnKeyboardInput(pSource, virtualKey, modifierKeys));

        BOOLEAN handledShouldNotImpedeTextInput = FALSE;
        IFCFAILFAST(keyEventArgs->get_HandledShouldNotImpedeTextInput(&handledShouldNotImpedeTextInput));
        if (!*handled && !handledShouldNotImpedeTextInput && KeyboardAcceleratorUtility::IsKeyValidForAccelerators(virtualKey, modifierKeys))
        {
            wsy::VirtualKeyModifiers keyModifiers = wsy::VirtualKeyModifiers::VirtualKeyModifiers_None;
            IFCFAILFAST(CoreImports::Input_GetKeyboardModifiers(&keyModifiers));
            *handled = KeyboardAcceleratorUtility::ProcessGlobalAccelerators(virtualKey, keyModifiers, contentRoot->GetAllLiveKeyboardAccelerators());
        }

        break;
    }
    case XCP_CHAR:
    case XCP_DEADCHAR:
        if (hasCharacterCode && (!m_fKeyDownHandled || m_handledShouldNotImpedeTextInput))
        {
            IFC_RETURN(RaiseCharacterReceivedEvent(
                virtualKey,
                keyStatus,
                modifierKeys,
                msgId,
                m_handledShouldNotImpedeTextInput,
                handled));
        }
        break;
    default:
        ASSERT(FALSE);
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT KeyboardInputProcessor::ProcessKeyEvent(
    _In_ mui::IKeyEventArgs* args,
    _In_ UINT32 msg,
    _In_ const XHANDLE platformPacket,
    _Out_ bool* handled)
{
    wsy::VirtualKey virtualKey;
    wsy::VirtualKey keyToLog = wsy::VirtualKey_None;
    mui::PhysicalKeyStatus keyStatus;
    MessageMap msgId;

    IFC_RETURN(args->get_VirtualKey(&virtualKey));
    IFC_RETURN(args->get_KeyStatus(&keyStatus));

    ctl::ComPtr<mui::IKeyEventArgs> keyArgs(args);

    if (virtualKey == VK_PACKET)
    {
        virtualKey = static_cast<wsy::VirtualKey>(InputUtility::Keyboard::GetVirtualKeyFromPacketInput(keyStatus.ScanCode));
    }

    if (InputUtility::Keyboard::ShouldLogVirtualKeyForKeyEvent(virtualKey))
    {
        //We only want to log directional and focus changing virtual keys
        keyToLog = virtualKey;
    }

    PhysicalKeyStatus physicalKeyStatus;
    physicalKeyStatus.m_uiRepeatCount = keyStatus.RepeatCount;
    physicalKeyStatus.m_uiScanCode = keyStatus.ScanCode;
    physicalKeyStatus.m_bIsExtendedKey = !!keyStatus.IsExtendedKey;
    physicalKeyStatus.m_bIsMenuKeyDown = !!keyStatus.IsMenuKeyDown;
    physicalKeyStatus.m_bIsKeyReleased = !!keyStatus.IsKeyReleased;

    // TFS #: 7529456. Currently on phone and onecore, we do not correctly receive the WasKeyDown field on KeyUp. Hard code to true until this issue is resolved.
    if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
    {
        physicalKeyStatus.m_bWasKeyDown = 1;
    }
    else
    {
        physicalKeyStatus.m_bWasKeyDown = !!keyStatus.WasKeyDown;
    }

    auto traceGuard = wil::scope_exit([&] {
        if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
        {
            TraceKeyDownEnd(keyToLog, keyStatus.RepeatCount, m_cKeyDownCountBeforeSubmitFrame++);
        }
        else if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
        {
            TraceKeyUpEnd(keyToLog, keyStatus.RepeatCount);
        }
    });

    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
    {
        msgId = XCP_KEYDOWN;
        TraceKeyDownBegin(keyToLog, keyStatus.RepeatCount, m_cKeyDownCountBeforeSubmitFrame);
    }
    else if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
    {
        msgId = XCP_KEYUP;
        TraceKeyUpBegin(keyToLog, keyStatus.RepeatCount);
    }
    else if (msg == WM_CHAR) { msgId = XCP_CHAR; }
    else if (msg == WM_DEADCHAR) { msgId = XCP_DEADCHAR; }
    else
    {
        // We should not receive any key aside from the ones above
        XAML_FAIL_FAST();
    }

    const bool isSecondaryMessage = msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP;

    IFC_RETURN(ProcessKeyboardInput(
        virtualKey,
        physicalKeyStatus,
        msgId,
        nullptr,
        isSecondaryMessage,
        platformPacket,
        handled));

    return S_OK;
}

_Check_return_ HRESULT KeyboardInputProcessor::ProcessXCPKeyup(
    _In_ CDependencyObject* const source,
    _In_ CKeyEventArgs* const keyEventArgs,
    _Inout_ bool* handled)
{
    CDependencyObject *pRoutedSource = source;
    CControl *pSourceControl = do_pointer_cast<CControl>(pRoutedSource);
    const bool isAccept = XboxUtility::IsGamepadNavigationAccept(keyEventArgs->m_originalKeyCode);
    const bool isCancel = XboxUtility::IsGamepadNavigationCancel(keyEventArgs->m_originalKeyCode);
    const bool isGamepadNavigation = XboxUtility::IsGamepadNavigationInput(keyEventArgs->m_originalKeyCode);
    const auto contentRoot = m_inputManager.GetContentRoot();

    // Even though preview events do not affect engagement, we want to adjust the source for the Event args.
    // This is because the PreviewKeyUp and KeyUp events are meant to be a symmetric pair.
    if (isGamepadNavigation && !isAccept && pSourceControl && pSourceControl->IsFocusEngagementEnabled() && !pSourceControl->IsFocusEngaged())
    {
        pRoutedSource = source->GetParentInternal();
    }

    xref_ptr<CDependencyObject> pOldFocusedElement(contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef());
    const bool handledFlagCoerced = m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_PreviewKeyUp),
        pRoutedSource,
        keyEventArgs,
        TRUE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */,
        nullptr /* coerceToHandledAtElement */,
        RoutingStrategy::Tunnelling);
    ASSERT(!handledFlagCoerced);

    CDependencyObject *pFocusedElement = contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();

    //If focus was changed during a preview event, update the key event args accordingly.
    if (pFocusedElement != pOldFocusedElement)
    {
        pRoutedSource = GetKeyRoutedSource(pFocusedElement);
        IFC_RETURN(keyEventArgs->put_Source(pRoutedSource));
        pSourceControl = do_pointer_cast<CControl>(pRoutedSource);
    }

    if (isGamepadNavigation)
    {
        if (pSourceControl && isAccept && pSourceControl->IsFocusEngagementEnabled() && !pSourceControl->IsFocusEngaged() && !keyEventArgs->m_bHandled)
        {
            IFC_RETURN(pSourceControl->SetValueByKnownIndex(KnownPropertyIndex::Control_IsFocusEngaged, TRUE));
            m_fKeyDownHandled = TRUE;
            keyEventArgs->m_bHandled = TRUE;
        }
        else if (pSourceControl && pSourceControl->IsFocusEngagementEnabled() && !pSourceControl->IsFocusEngaged())
        {
            pRoutedSource = source->GetParentInternal();
            m_fKeyDownHandled = FALSE;
        }
        else
        {
            m_fKeyDownHandled = FALSE;
        }
    }
    else
    {
        m_fKeyDownHandled = FALSE;
    }
    
    m_handledShouldNotImpedeTextInput = FALSE;


    // This is a synchronous callout to application code that allows
    // the application to re-enter XAML. The application could
    // change state and release objects, so protect against
    // reentrancy by ensuring that objects are alive and state is
    // re-validated after return.
    const bool wasKeyUpCoercedToHandled = m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_KeyUp),
        pRoutedSource,
        keyEventArgs,
        TRUE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */,
        contentRoot->GetFocusManagerNoRef()->GetEngagedControlNoRef() /* coerceToHandledAtElement */);

    // If b-keydown or b-keyup have not been handled
    if (isCancel && (!keyEventArgs->m_bHandled || (m_wasKeyDownCoercedToHandled && wasKeyUpCoercedToHandled)))
    {
        CControl *pEngagedControl = contentRoot->GetFocusManagerNoRef()->GetEngagedControlNoRef();

        const bool isNavigatedToByEngagingControl = contentRoot->GetFocusManagerNoRef()->NavigatedToByEngagingControl(source);

        if (isNavigatedToByEngagingControl)
        {
            IFC_RETURN(pEngagedControl->RemoveFocusEngagement());
            m_fKeyDownHandled = TRUE;
            keyEventArgs->m_bHandled = TRUE;

            // Give focus to the disengaged control if it isn't the same as the currently focused control
            if (pEngagedControl != do_pointer_cast<CControl>(pRoutedSource))
            {
                //If an error is propagated to the Input Manager here, we are in an invalid state.
                const Focus::FocusMovementResult result = contentRoot->GetFocusManagerNoRef()->SetFocusedElement(Focus::FocusMovement(pEngagedControl, DirectUI::FocusNavigationDirection::None, DirectUI::FocusState::Keyboard));
                IFCFAILFAST(result.GetHResult());
            }
        }
    }

    *handled = keyEventArgs->m_bHandled == TRUE;

    return S_OK;
}

_Check_return_ HRESULT KeyboardInputProcessor::ProcessXCPKeydown(
    _In_ CDependencyObject* const source,
    _In_ CKeyEventArgs* const keyEventArgs,
    _In_ const MessageMap msgId,
    _In_ const bool hasCharacterCode,
    _In_ const XUINT32 modifierKeys,
    _In_ const bool isSecondaryMessage,
    _In_ const XHANDLE platformPacket,
    _Inout_ bool* handled)
{
    CDependencyObject *pRoutedSource = source;
    CControl *pSourceControl = do_pointer_cast<CControl>(source);
    bool processTabKey = false;
    InputMessage directManipMessage;
    const auto contentRoot = m_inputManager.GetContentRoot();

    // We need to ensure that the CurrentMsgForDirectManipulationProcessing inputmessage is cleared out if something
    // fails while raising the key down.
    auto cleanupGuard = wil::scope_exit([&]
    {
        m_inputManager.SetCurrentMsgForDirectManipulationProcessing(nullptr);
    });

    // If the key is F7, we may need to toggle Caret Browsing mode.
    if (keyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_F7 &&
        !GetIsCaretBrowsingF7Disabled())
    {
        if (GetCaretBrowsingDialogNotPopAgain())
        {
            //toggling around when detect F7, if user don't want the dialog.
            if (GetCaretBrowsingModeEnable())
            {
                SetCaretBrowsingModeEnable(false);
            }
            else
            {
                SetCaretBrowsingModeEnable(true);
            }
        }
        else
        {
            //pop the dialog if the not popagain checkbox was not checked
            FxCallbacks::PopCaretBrowsingDialog(VisualTree::GetForElementNoRef(source)->GetOrCreateXamlRootNoRef());
        }
    }

    const bool isGamepadNavigationInput = XboxUtility::IsGamepadNavigationInput(keyEventArgs->m_originalKeyCode);

    // This is a synchronous callout to application code that allows
    // the application to re-enter XAML. The application could
    // change state and release objects, so protect against
    // reentrancy by ensuring that objects are alive and state is
    // re-validated after return.

    // Only forward a selected subset of keystrokes to DirectManipulation
    // If Auto Focus is Enabled, we don't want DM to handle Focus altering keys
    // these will change to actual XBOX VKs once they're available

    // We should only forward key input to Direct Manipulation if:
    // in UWP, CoreWindow activation mode is Activated in Foreground;
    // in desktop, XamlIslandRoot has focus;
    // We are in this activation mode for all non-COmponent UI scenarios.
    // Keyboard Accelerators across components allows us to get input when Activated but not in the foreground, and we want to avoid
    // sending input to Direct Manipulation here to prevent unintended consequences.

    bool isActivated = contentRoot->GetFocusManagerNoRef()->GetFocusObserverNoRef()->IsActivated();
    auto xamlIslandRoot = contentRoot->GetXamlIslandRootNoRef();
    if (xamlIslandRoot)
    {
        ctl::ComPtr<IInspectable> spInsp;
        ctl::ComPtr<xaml_hosting::IFocusController> spFocusController;
        IFC_RETURN(xamlIslandRoot->get_FocusController(&spInsp));
        IFC_RETURN(spInsp.As(&spFocusController));

        BOOLEAN active = FALSE;
        spFocusController->get_HasFocus(&active);
        if (active)
        {
            isActivated = true;
        }
    }

    if (isActivated && InputUtility::Keyboard::ShouldForwardToDirectManipulation(keyEventArgs->m_platformKeyCode, keyEventArgs->m_originalKeyCode))
    {
        // Used by the ProcessInputMessageWithDirectManipulation method potentially invoked by the DM container,
        // synchronously during the RaiseRoutedEvent call.
        directManipMessage.m_hPlatformPacket = platformPacket;
        directManipMessage.m_msgID = msgId;
        directManipMessage.m_bIsSecondaryMessage = isSecondaryMessage;

        m_inputManager.SetCurrentMsgForDirectManipulationProcessing(&directManipMessage);
    }

    if (isGamepadNavigationInput && pSourceControl && pSourceControl->IsFocusEngagementEnabled() && !pSourceControl->IsFocusEngaged())
    {
        pRoutedSource = source->GetParentInternal();
    }

    xref_ptr<CDependencyObject> pOldFocusedElement(contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef());
    //Raise preview event
    const bool handledFlagCoerced = m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_PreviewKeyDown),
        pRoutedSource,
        keyEventArgs,
        TRUE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */,
        nullptr /*coerceToHandledAtElement*/,
        RoutingStrategy::Tunnelling);
    ASSERT(!handledFlagCoerced);

    //If focus was changed during a preview event, update the key event args accordingly.
    CDependencyObject *pFocusedElement = contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();
    if (pFocusedElement != pOldFocusedElement)
    {
        pRoutedSource = GetKeyRoutedSource(pFocusedElement);
        IFC_RETURN(keyEventArgs->put_Source(pRoutedSource));
        pSourceControl = do_pointer_cast<CControl>(pRoutedSource);
    }

    if (isGamepadNavigationInput && pSourceControl && pSourceControl->IsFocusEngagementEnabled() && !pSourceControl->IsFocusEngaged())
    {
        pRoutedSource = source->GetParentInternal();
    }

    m_wasKeyDownCoercedToHandled =
        m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_KeyDown),
        pRoutedSource,
        keyEventArgs,
        TRUE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */,
        contentRoot->GetFocusManagerNoRef()->GetEngagedControlNoRef() /* coerceToHandledAtElement */);

    if (!keyEventArgs->m_bHandled && modifierKeys == 0)
    {
        // if the keydown is not handled by child element of text floatie, it will be used to dismiss floatie
        CUIElement *element = do_pointer_cast<CUIElement>(pFocusedElement);
        if (element && FxCallbacks::TextControlFlyout_IsElementChildOfTransientOpenedFlyout(element))
        {
            IFC_RETURN(FxCallbacks::TextControlFlyout_DismissAllFlyoutsForOwner(element));
            // setting handled to true to suppress text input
            keyEventArgs->m_bHandled = TRUE;
        }
    }

    m_inputManager.SetCurrentMsgForDirectManipulationProcessing(nullptr);

    *handled = keyEventArgs->m_bHandled == TRUE;

    if (keyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Tab && !(keyEventArgs->m_bHandled))
    {
        if (!(modifierKeys & KEY_MODIFIER_CTRL) && !(modifierKeys & KEY_MODIFIER_ALT))
        {
            processTabKey = true;  // Process Tab Navigation if Ctrl and Alt keys isn't pressed.
        }
    }

    if (processTabKey)
    {
        // Raise Internal TabProcessing event directly to the hidden root to handle the default tab processing
        // if no client code has marked it handled.
        m_inputManager.m_coreServices.GetEventManager()->Raise(
            EventHandle(KnownEventIndex::UIElement_TabProcessing),
            TRUE,
            static_cast<CDependencyObject*>(contentRoot->GetVisualTreeNoRef()->GetRootElementNoRef()),
            keyEventArgs,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);

        *handled = keyEventArgs->m_bHandled == TRUE;
    }

    // Due to the way events are raised, it is possible to lose activation
    // before the event args are marked as handled. However, we don't
    // want to leave m_fKeyDownHandled marked as true if we are not
    // active. This is because in scenarios (such as type-to-search) when
    // the window becomes active again we don't want to skip the XCP_CHAR
    // below as it results in a dropped character.
    m_fKeyDownHandled = m_inputManager.m_activationManager.IsActive() ? keyEventArgs->m_bHandled : false;
    
    // for TFS to handle keyboard input, handledShouldNotImpedeTextInput gets set in event args even when key is marked handled
    // For Keyboard processor to use this value for XCP_CHAR, m_handledShouldNotImpedeTextInput stores this value and 
    // it gets cleared up during KEYUP event
    BOOLEAN handledShouldNotImpedeTextInput = FALSE;
    IFCFAILFAST(keyEventArgs->get_HandledShouldNotImpedeTextInput(&handledShouldNotImpedeTextInput));
    m_handledShouldNotImpedeTextInput = m_inputManager.m_activationManager.IsActive() ? !!handledShouldNotImpedeTextInput : false;

    if (!m_fKeyDownHandled || m_wasKeyDownCoercedToHandled)
    {
        DirectUI::FocusNavigationDirection navigationDirection = DirectUI::FocusNavigationDirection::None;
        // Use original key code here because platform key code has already had Dpad and left thumbstick directions mapped to the arrow keys
        // The methods in XBox utility expect Xbox VK's to come in for them to work correctly.
        if (XboxUtility::IsGamepadNavigationDown(keyEventArgs->m_originalKeyCode))
        {
            navigationDirection = DirectUI::FocusNavigationDirection::Down;
        }
        else if (XboxUtility::IsGamepadNavigationUp(keyEventArgs->m_originalKeyCode))
        {
            navigationDirection = DirectUI::FocusNavigationDirection::Up;
        }
        else if (XboxUtility::IsGamepadNavigationLeft(keyEventArgs->m_originalKeyCode))
        {
            navigationDirection = DirectUI::FocusNavigationDirection::Left;
        }
        else if (XboxUtility::IsGamepadNavigationRight(keyEventArgs->m_originalKeyCode))
        {
            navigationDirection = DirectUI::FocusNavigationDirection::Right;
        }

        if (navigationDirection != DirectUI::FocusNavigationDirection::None && m_noCandidateDirectionPerTick != navigationDirection)
        {
            const bool focusMoved = contentRoot->GetFocusManagerNoRef()->FindAndSetNextFocus(navigationDirection);
            if (!focusMoved)
            {
                contentRoot->GetFocusManagerNoRef()->RaiseNoFocusCandidateFoundEvent(navigationDirection);
                // Temporarily halt expensive focus-walks in the same direction until the next tick, to speed up the case when the
                // gamepad is spamming thumbstick input and we've hit the end of an list that's still loading.
                // If we receive multiple directions that cause no focus candidates to be found during the same tick,
                // we will use the LAST direction received to determine whether we should halt the focus-walks.
                m_noCandidateDirectionPerTick = navigationDirection;
            }
        }
    }

    if (hasCharacterCode && !m_fKeyDownHandled)
    {
        IFC_RETURN(RaiseCharacterReceivedEvent(
            keyEventArgs->m_originalKeyCode,
            keyEventArgs->m_physicalKeyStatus,
            modifierKeys,
            msgId,
            false,
            handled));
    }

    return S_OK;
}

_Check_return_ HRESULT KeyboardInputProcessor::RaiseCharacterReceivedEvent(
    _In_ const wsy::VirtualKey virtualKey,
    _In_ const PhysicalKeyStatus& keyStatus,
    _In_ const XUINT32 modifierKeys,
    _In_ const MessageMap msgId,
    _In_ const bool handledShouldNotImpedeTextInput,
    _Out_ bool* handled) const
{
    const auto contentRoot = m_inputManager.GetContentRoot();

    //Special handling of AccessKeys via WM_CHAR messages here for cases on Desktop when there is no CoreWindow
    //For all other cases, we will process this by handling CoreWindow's CharacterReceived Event
    const bool shouldProcess =
        contentRoot->GetType() == CContentRoot::Type::CoreWindow &&
        (m_inputManager.m_coreServices.GetInputServices()->GetCoreWindow() == nullptr);

    if (shouldProcess)
    {
        // Make an input method with the params the AK needs
        // TODO: decouple InputMessage from AccessKeys
        InputMessage inputMsg;
        inputMsg.m_physicalKeyStatus = keyStatus;
        inputMsg.m_modifierKeys = modifierKeys;
        inputMsg.m_platformKeyCode = virtualKey;
        inputMsg.m_msgID = msgId;

        bool keyHandled = false;
        IFC_RETURN(contentRoot->GetAKExport().TryProcessInputForAccessKey(&inputMsg, &keyHandled));

        if (keyHandled)
        {
            return S_OK;
        }
    }
    else if (contentRoot->GetAKExport().IsActive())
    {
        return S_OK;
    }

    if (virtualKey)
    {
        CDependencyObject* source = contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();
        xref_ptr<CCharacterReceivedRoutedEventArgs> characterReceivedEventArgs;
        characterReceivedEventArgs.init(new CCharacterReceivedRoutedEventArgs());

        // Set the source as the public visual root if the focus isn't set or focus is on the root ScrollViewer.
        if (source == nullptr || source == contentRoot->GetVisualTreeNoRef()->GetRootScrollViewer())
        {
            source = contentRoot->GetVisualTreeNoRef()->GetPublicRootVisual();
        }

        IFC_RETURN(characterReceivedEventArgs->put_Source(source));

        characterReceivedEventArgs->m_platformKeyCode = virtualKey;
        characterReceivedEventArgs->m_physicalKeyStatus = keyStatus;
        characterReceivedEventArgs->m_msgID = msgId;

        // for allowing WM_CHAR/XCP_CHAR handled message with handledShouldNotImpedeTextInput for TextBox set as true
        // to be passed to TextServiceFramework (TSF), this code will bypass raising CharacterReceived event (and avoid bubbling it up)
        // and directly call OnCharacterReceived event handler for TextBoxBase so that it passes the event to TSF for processing
        // Only applicable to TSF1 which is used in Xaml Island and Desktop Window
        CTextBoxBase* textBoxBase = nullptr;

        if (handledShouldNotImpedeTextInput &&
            characterReceivedEventArgs->m_bHandled &&
            !m_inputManager.m_coreServices.IsTSF3Enabled() && 
            (textBoxBase = do_pointer_cast<CTextBoxBase>(source)))
        {
            IFC_RETURN(textBoxBase->InjectCharaterReceivedTSF(characterReceivedEventArgs));
        }
        else
        {
            // This is a synchronous callout to application code that allows
            // the application to re-enter XAML. The application could
            // change state and release objects, so protect against
            // reentrancy by ensuring that objects are alive and state is
            // re-validated after return.
            m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
                EventHandle(KnownEventIndex::UIElement_CharacterReceived),
                source,
                characterReceivedEventArgs,
                TRUE /* bIgnoreVisibility */,
                TRUE /* fRaiseSync */,
                TRUE /* fInputEvent */);
        }
    }

    return S_OK;
}

bool KeyboardInputProcessor::IsFocusNavigationKey(const wsy::VirtualKey virtualKeyCode)
{
    return (virtualKeyCode == wsy::VirtualKey::VirtualKey_Left ||
            virtualKeyCode == wsy::VirtualKey::VirtualKey_Right ||
            virtualKeyCode == wsy::VirtualKey::VirtualKey_Down ||
            virtualKeyCode == wsy::VirtualKey::VirtualKey_Up ||
            virtualKeyCode == wsy::VirtualKey::VirtualKey_Tab);
}

CDependencyObject* KeyboardInputProcessor::GetKeyRoutedSource(_In_ CDependencyObject* const source)
{
    CDependencyObject *pRoutedSource = source;
    VisualTree* visualTree = m_inputManager.GetContentRoot()->GetVisualTreeNoRef();

    // Set the source as the public visual root if the focus isn't set or focus is on the root ScrollViewer.
    if (pRoutedSource == nullptr || pRoutedSource == visualTree->GetRootScrollViewer())
    {
        pRoutedSource = visualTree->GetPublicRootVisual();
        // if window chrome the is top most element on a visual tree, return its content as 
        // the real root element of the visual tree
        // TODO: Task 38535248
        if (pRoutedSource->OfTypeByIndex<KnownTypeIndex::WindowChrome>())
        {
            CContentControl* pWindowChrome = do_pointer_cast<CContentControl>(pRoutedSource);
            CValue windowContent;
            pWindowChrome->Content(pWindowChrome, 0, nullptr, nullptr, &windowContent);
            pRoutedSource = windowContent.AsObject();
        }
    }

    return pRoutedSource;
}

/* A focus sound should only be requested during processing of a user initiated interaction
   like a key press or mouse/touch interaction */
bool KeyboardInputProcessor::ShouldRequestFocusSound() const
{
    return m_shouldAllowRequestFocusSound;
}

void KeyboardInputProcessor::SetShouldAllRequestFocusSound(_In_ bool value)
{
    m_shouldAllowRequestFocusSound = value;
}

void KeyboardInputProcessor::SetKeyDownHandled(_In_ bool value)
{
    m_fKeyDownHandled = value;
}

void KeyboardInputProcessor::SetCandidateDirectionPerTick(_In_ DirectUI::FocusNavigationDirection direction)
{
    m_noCandidateDirectionPerTick = direction;
}
