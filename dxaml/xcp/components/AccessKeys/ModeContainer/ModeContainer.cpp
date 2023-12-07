// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModeContainer.h"


namespace AccessKeys
{
    AKModeContainer::AKModeContainer() :
        m_isActive(false),
        m_forceQuit(false),
        m_akModeChanged(false),
        m_canEnterAccessKeyMode(false),
        m_lockEnteringAccessKeyModeUntilAltUp(false),
        m_lockExitingAccessKeyModeOnAltUp(false) {}

    _Check_return_ HRESULT
        AKModeContainer::SetIsActive(_In_ bool newValue)
    {
        if (GetIsActive() != newValue)
        {
            AK_TRACE(L"AK> SetIsActive from %d to %d\n", m_isActive, newValue);
            m_isActive = newValue;
            IFC_RETURN(AKOnIsActiveChanged(m_focusManager, nullptr, nullptr));
        }
        return S_OK;
    }

    // Evaluate input message to see if we should activate access key navigation
    // Mode will change synchronously and fire IsActiveChanged event
    // Returns true if IsActive changed as a result of this input
    _Check_return_ HRESULT
        AKModeContainer::EvaluateAccessKeyMode(
            _In_  const bool isAltKey,
            _In_  const MessageMap msgID,
            _In_  const wsy::VirtualKey keyCode,
            _In_  const bool isMenuKey,
            _In_  const bool isNumpadInput,
            _Out_ bool* isValid)
    {
        (*isValid) = IsValidAccessKeyMessage(isAltKey, msgID, keyCode, isMenuKey, isNumpadInput);

        const bool isAltAKMessage = IsAltAccessKeyMessage(isAltKey, msgID, keyCode, isMenuKey);
        (*isValid) |= isAltAKMessage;

        m_akModeChanged = false;

        //If we have received a alt + keydown, this is recognized as a hotkey and should be processed
        if ((*isValid) && isMenuKey && !m_isActive)
        {
            m_akModeChanged = true;
        }
        else if (isAltKey)
        {
            const bool isKeyUp = msgID == MessageMap::XCP_KEYUP;

            if (isKeyUp)
            {
                //We don't want to activate ak mode when using hotkeys
                if ((*isValid) && !isMenuKey)
                {
                    if ((m_isActive == false && m_canEnterAccessKeyMode) || (m_isActive && !m_lockExitingAccessKeyModeOnAltUp))
                    {
                        m_akModeChanged = true;
                        m_canEnterAccessKeyMode = false;
                        IFC_RETURN(SetIsActive(!m_isActive));
                    }
                }
            }
        }

        return S_OK;
    }

    // This type of input is an access key key pressed down with alt.  Should treat this as a valid accesskey and invoke
    bool AKModeContainer::IsAltAccessKeyMessage(
        _In_  const bool isAltKey,
        _In_  const MessageMap msgID,
        _In_  const wsy::VirtualKey keyCode,
        _In_  const bool isMenuKey) const
    {
        // When Alt then A is pressed, the first KeyDown message will contain isMenuKey==true with subsequent ones having this field set to false.
        // Therefore, this blocks repeatedly navigating down an access key heirarchy without using alt +AK without lifting the key and repressing it
        return  (m_isActive && isMenuKey && keyCode >= wsy::VirtualKey::VirtualKey_Number0 && keyCode <= wsy::VirtualKey::VirtualKey_Z && msgID == XCP_KEYDOWN);
    }

    bool AKModeContainer::IsValidAccessKeyMessage(
        _In_  const bool isAltKey,
        _In_  const MessageMap msgID,
        _In_  const wsy::VirtualKey keyCode,
        _In_  const bool isMenuKey,
        _In_  const bool isNumpadInput) const
    {
        // All numeric access key messages are valid for both number and numpad keys when in Access Key mode.
        // When in hot-key mode, access keys are enabled only for number keys, but not numpad keys.
        // This follows the precedent set by Office's access keys, and helps disambiguate alt-numeric special
        // characters from access keys.
        return  (m_isActive && msgID == XCP_CHAR) ||
                (isAltKey && msgID == XCP_KEYUP) ||
                (keyCode == wsy::VirtualKey::VirtualKey_Escape && msgID == XCP_KEYDOWN && m_isActive) ||
                (!isAltKey && !m_isActive && isMenuKey && !isNumpadInput);
    }

    void AKModeContainer::UpdateAKModeStateChangeLockout(const InputMessage* const message)
    {
        if (IsNakedAltKeyDown(message) && m_lockEnteringAccessKeyModeUntilAltUp == false)
        {
            m_canEnterAccessKeyMode = true; // Can only enter AK Mode when this is set to true.
            m_lockExitingAccessKeyModeOnAltUp = false;
        }
        else if (!IsNakedAltKeyUp(message) || m_lockEnteringAccessKeyModeUntilAltUp)
        {
            // Any input after the alt down that is not an Alt up will cause can enter AccessKeyMode to toggle to false.  This will prevent entering AK mode
            // When using control+alt+delete, Alt+f4 etc.  Even if an alt key-up is received, without the corresponding key down no state change will occur.
            m_canEnterAccessKeyMode = false;

            // if Alt was held down on the release of a key, toggle the latch so we do not enter AKmode on the alt release.
            if ((message->m_modifierKeys & KEY_MODIFIER_ALT) == KEY_MODIFIER_ALT)
            {
                m_lockEnteringAccessKeyModeUntilAltUp = true;
            }
            else if (message->m_modifierKeys == 0)
            {
                // All modifiers released.  Reset the lock (This also handles the case for alt up)
                m_lockEnteringAccessKeyModeUntilAltUp = false;
                // In the case Alt+Ak is used to invoke an access key, we do not want the end of that message (Alt up) to potentially cause
                // an AK mode exit.  Note: this does not preclude the invoke causing an exit, only the Alt Up.
                m_lockExitingAccessKeyModeOnAltUp = true;
            }
        }
    }

    bool AKModeContainer::IsNakedAltKeyDown(const InputMessage* const message)
    {
        return message->m_msgID == XCP_KEYDOWN && IsLeftAltKey(message) && message->m_modifierKeys == KEY_MODIFIER_ALT;
    }

    bool AKModeContainer::IsNakedAltKeyUp(const InputMessage* const message)
    {
        return message->m_msgID == XCP_KEYUP && IsLeftAltKey(message) && message->m_modifierKeys == 0;
    }
}
