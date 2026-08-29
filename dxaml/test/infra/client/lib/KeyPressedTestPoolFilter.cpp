// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "KeyPressedTestPoolFilter.h"
#include "XamlTailored.h"
#include <microsoft.ui.input.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace WEX::Common;
using namespace WEX::Logging;

namespace Private::Infrastructure {
    bool KeyPressedTestPoolFilter::CheckKeyState(
        _In_  wsy::VirtualKey virtualKey, 
        _In_  wuc::CoreVirtualKeyStates keyStateToBeChecked
        )
    {
        wrl::ComPtr<ixp::IInputKeyboardSourceStatics> keyboardInputStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputKeyboardSource).Get(),
            &keyboardInputStatics));

        wuc::CoreVirtualKeyStates virtualKeyState;
        LogThrow_IfFailed(keyboardInputStatics->GetKeyStateForCurrentThread(virtualKey, &virtualKeyState));

        return virtualKeyState & keyStateToBeChecked;
    }

    bool  KeyPressedTestPoolFilter::IsDirty()
    {
        bool keyDown = false;

        RunOnUIThread([&]()
        {
            //Looping through all VirtualKeys, checking if they are in a keydown states
            for (int key = wsy::VirtualKey::VirtualKey_LeftButton; key <= wsy::VirtualKey::VirtualKey_RightMenu; key++)
            {
                wsy::VirtualKey virtualKey = static_cast<wsy::VirtualKey>(key);

                //A key should not be left in the Down state.
                //It should not matter if a key is in a locked state -- this state is just a toggle
                //for each key, and as such does not affect XAML.
                if (CheckKeyState(
                        virtualKey, 
                        wuc::CoreVirtualKeyStates::CoreVirtualKeyStates_Down))
                {
                    Log::Error(WEX::Common::String().Format(
                        L"Key %d is still pressed down.",
                    virtualKey));
                    keyDown = true;
                }
            }

            if (CheckKeyState(
                    wsy::VirtualKey::VirtualKey_NumberKeyLock, 
                    wuc::CoreVirtualKeyStates::CoreVirtualKeyStates_Locked))
            {
                Log::Error(L"Numlock is still enabled.");
                keyDown = true;
            }

            if (CheckKeyState(
                    wsy::VirtualKey::VirtualKey_CapitalLock, 
                    wuc::CoreVirtualKeyStates::CoreVirtualKeyStates_Locked))
            {
                Log::Error(L"Capslock is still enabled.");
                keyDown = true;
            }

            if (CheckKeyState(
                wsy::VirtualKey::VirtualKey_Scroll, 
                wuc::CoreVirtualKeyStates::CoreVirtualKeyStates_Locked))
            {
                Log::Error(L"ScrollLock is still enabled.");
                keyDown = true;
            }
        });

        return keyDown;
    }

}
