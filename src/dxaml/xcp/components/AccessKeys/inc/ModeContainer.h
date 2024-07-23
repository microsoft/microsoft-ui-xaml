// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "paltypes.h"
#include "AKCommon.h"

class CFocusManager;

namespace AccessKeys
{
    // Global callback to be defined by caller
    _Check_return_ HRESULT AKOnIsActiveChanged(_In_opt_ CFocusManager* focusManager, _In_opt_ IInspectable* sender, _In_opt_ IInspectable* args);

    class AKModeContainer
    {
    public:
        AKModeContainer();
        _Check_return_ HRESULT SetIsActive(_In_ bool newValue);

        void SetFocusManager(_In_opt_ CFocusManager* focusManager)
        {
            m_focusManager = focusManager;
        }

        // Evaluate input message to see if we should activate access key navigation
        // Mode will change synchronously and fire IsActiveChanged event
        _Check_return_ HRESULT EvaluateAccessKeyMode(_In_ const InputMessage* const inputMessage, _Out_ bool* shouldEvaluate)
        {
            *shouldEvaluate = false;
            m_forceQuit = false;

            UpdateAKModeStateChangeLockout(inputMessage);

            if (inputMessage->m_msgID != XCP_CHAR && (((inputMessage->m_modifierKeys & KEY_MODIFIER_CTRL) == KEY_MODIFIER_CTRL) ||
                (IsUnicodeKeypadInput(inputMessage)) ||
                (IsFunctionKey(inputMessage))))
            {
                return S_OK;
            }

            if (InputShouldCauseAKModeExit(inputMessage))
            {
                if (m_isActive)
                {
                    m_akModeChanged = true;
                    m_forceQuit = true;
                    *shouldEvaluate = true; //We need to fire Hide events on the currently shown elements
                    IFC_RETURN(SetIsActive(false));
                }
            }
            else
            {
                IFC_RETURN(EvaluateAccessKeyMode(
                    IsLeftAltKey(inputMessage),
                    inputMessage->m_msgID, //Message type, like a Keydown/Up, character, etc
                    inputMessage->m_platformKeyCode, //The message Key.  Represents a Vkey unless message type is CHAR at which point it's an actual character.
                    IsMenuKeyDown(inputMessage),
                    IsNumpadInput(inputMessage),
                    shouldEvaluate));
            }

            return S_OK;
        }

        bool GetIsActive() const { return m_isActive; }
        bool HasAKModeChanged() const { return m_akModeChanged; }
        bool ShouldForciblyExitAKMode() const { return m_forceQuit; }

    private:
        bool m_isActive;
        bool m_akModeChanged;
        bool m_forceQuit;
        bool m_canEnterAccessKeyMode;  // Only want to enter AK mode when an alt key was pressed and released without other key input.  This bool toggles to false whenever input that should disallow AK mode is entered.
        bool m_lockEnteringAccessKeyModeUntilAltUp;
        bool m_lockExitingAccessKeyModeOnAltUp;

        CFocusManager* m_focusManager = nullptr;

        _Check_return_ HRESULT EvaluateAccessKeyMode(
            _In_ const bool isAltKey,
            _In_ const MessageMap msgID,
            _In_ const  wsy::VirtualKey keyCode,
            _In_ const bool isMenuKey,
            _In_ const bool isNumpadInput,
            _Out_ bool* shouldEvaluate);

        bool IsValidAccessKeyMessage(_In_  const bool isAltKey,
            _In_  const MessageMap msgID,
            _In_  const  wsy::VirtualKey keyCode,
            _In_  const bool isMenuKey,
            _In_  const bool isNumpadInput) const;

        bool IsAltAccessKeyMessage(_In_  const bool isAltKey,
            _In_  const MessageMap msgID,
            _In_  const  wsy::VirtualKey keyCode,
            _In_  const bool isMenuKey) const;

        static bool InputShouldCauseAKModeExit(_In_ const InputMessage* const message)
        {
            return  message->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Up ||
                    message->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Down ||
                    message->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Left ||
                    message->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Right ||
                    message->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Tab ||
                    message->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Space ||
                    message->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Enter;
        }

        bool IsUnicodeKeypadInput(const InputMessage* const message) const
        {
            return  m_isActive &&
                (message->m_modifierKeys & KEY_MODIFIER_ALT) &&
                IsNumpadInput(message);
        }

        static bool IsNumpadInput(const InputMessage* const message)
        {
            return message->m_platformKeyCode >= wsy::VirtualKey::VirtualKey_NumberPad0 && message->m_platformKeyCode <=wsy::VirtualKey::VirtualKey_NumberPad9;
        }

        static bool IsFunctionKey(const InputMessage* const message)
        {
            return ((message->m_platformKeyCode >= wsy::VirtualKey::VirtualKey_F1 && message->m_platformKeyCode <= wsy::VirtualKey::VirtualKey_F12));
        }

        void UpdateAKModeStateChangeLockout(const InputMessage* const message);
        bool IsNakedAltKeyDown(const InputMessage* const message);
        bool IsNakedAltKeyUp(const InputMessage* const message);
    };
}
