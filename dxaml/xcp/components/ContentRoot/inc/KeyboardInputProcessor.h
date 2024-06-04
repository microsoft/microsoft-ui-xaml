// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <paltypes.h>
#include "enumdefs.g.h"

class CCoreServices;
class CContentRoot;
class CKeyEventArgs;
class CDependencyObject;

class CInputManager;

namespace ContentRootInput
{
    class KeyboardInputProcessor
    {
    public:
        KeyboardInputProcessor(_In_ CInputManager& inputManager);

        _Check_return_ HRESULT ProcessKeyboardInput(
            _In_ const wsy::VirtualKey virtualKey,
            _In_ const PhysicalKeyStatus& keyStatus,
            _In_ const MessageMap msgId,
            _In_opt_ const wrl_wrappers::HString* const deviceId,
            _In_ const bool isSecondaryMessage,
            _In_ const XHANDLE platformPacket,
            _Out_ bool* handled);

        _Check_return_ HRESULT ProcessKeyEvent(
            _In_ mui::IKeyEventArgs* args,
            _In_ UINT32 msg,
            _In_ const XHANDLE platformPacket,
            _Out_ bool* handled);

        bool ShouldRequestFocusSound() const;

        void SetShouldAllRequestFocusSound(_In_ bool value);
        void SetKeyDownHandled(_In_ bool value);
        void SetCandidateDirectionPerTick(_In_ DirectUI::FocusNavigationDirection direction);

        // Tracking the number of KeyDowns processed before a frame is submitted
        int m_cKeyDownCountBeforeSubmitFrame = 0;

    private:
        _Check_return_ HRESULT ProcessXCPKeyup(
                _In_ CDependencyObject* const source,
                _In_ CKeyEventArgs* const keyEventArgs,
                _Inout_ bool* handled);

        _Check_return_ HRESULT ProcessXCPKeydown(
            _In_ CDependencyObject* const source,
            _In_ CKeyEventArgs* const keyEventArgs,
            _In_ const MessageMap msgId,
            _In_ const bool hasCharacterCode,
            _In_ const XUINT32 modifierKeys,
            _In_ const bool isSecondaryMessage,
            _In_ const XHANDLE platformPacket,
            _Inout_ bool* handled);

        _Check_return_ HRESULT RaiseCharacterReceivedEvent(
            _In_ const wsy::VirtualKey virtualKey,
            _In_ const PhysicalKeyStatus& keyStatus,
            _In_ const XUINT32 modifierKeys,
            _In_ const MessageMap msgId,
            _In_ const bool handledShouldNotImpedeTextInput,
            _Out_ bool* handled) const;

        static bool IsFocusNavigationKey(const wsy::VirtualKey virtualKeyCode);

        CDependencyObject* GetKeyRoutedSource(_In_ CDependencyObject* const source);

        CInputManager& m_inputManager;

        bool m_shouldAllowRequestFocusSound = false;
        bool m_fKeyDownHandled = false;
        bool m_handledShouldNotImpedeTextInput = false;
        bool m_wasKeyDownCoercedToHandled = false;

        DirectUI::FocusNavigationDirection m_noCandidateDirectionPerTick = DirectUI::FocusNavigationDirection::None;
    };
};