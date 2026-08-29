// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "FocusPressState.h"

namespace KeyPress {    
    class ButtonBase
    {
    public:

        static bool IsPress(_In_ wsy::VirtualKey key, _In_ bool acceptsReturn)
        {
            // Hitting the SPACE, ENTER, or GAMEPAD_A key is equivalent to pressing the pointer
            // button
            return (key == wsy::VirtualKey_Space ||
                   (key == wsy::VirtualKey_Enter && acceptsReturn) ||
                   (key == wsy::VirtualKey_GamepadA));
        }
        
        template<typename ControlClass>
        static HRESULT KeyDown(_In_ wsy::VirtualKey key, _Out_ BOOLEAN* pbHandled, bool bAcceptsReturn, _Inout_ ControlClass* control)
        {
            BOOLEAN bIsEnabled = false;
            xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;

            *pbHandled = FALSE;

            IFC_RETURN(control->get_IsEnabled(&bIsEnabled));
            IFC_RETURN(control->get_ClickMode(&nClickMode));

            // Key presses can be ignored when disabled or in xaml_controls::ClickMode.Hover
            if (bIsEnabled &&
                nClickMode != xaml_controls::ClickMode_Hover)
            {
                if (IsPress(key, bAcceptsReturn))
                {
                    // Ignore the SPACE/ENTER/GAMEPAD_A key if we already have the pointer captured
                    // or if it had been pressed previously.
                    if (!control->m_bIsPointerCaptured && !control->m_bIsSpaceOrEnterKeyDown && !control->m_bIsNavigationAcceptOrGamepadAKeyDown)
                    {
                        if (key == wsy::VirtualKey_Space || (key == wsy::VirtualKey_Enter && bAcceptsReturn))
                        {
                            control->m_bIsSpaceOrEnterKeyDown = true;
                        }
                        else if (key == wsy::VirtualKey_GamepadA)
                        {
                            control->m_bIsNavigationAcceptOrGamepadAKeyDown = true;
                        }

                        IFC_RETURN(control->put_IsPressed(TRUE))

                        if (nClickMode == xaml_controls::ClickMode_Press)
                        {
                            IFC_RETURN(control->OnClick());
                        }

                        *pbHandled = TRUE;

                        StartFocusPress(checked_cast<CUIElement>(control->GetHandle()));
                    }
                }
                // Any other keys pressed are irrelevant
                else if (control->m_bIsSpaceOrEnterKeyDown || control->m_bIsNavigationAcceptOrGamepadAKeyDown)
                {
                    IFC_RETURN(control->put_IsPressed(FALSE));

                    control->m_bIsSpaceOrEnterKeyDown = false;
                    control->m_bIsNavigationAcceptOrGamepadAKeyDown = false;
                }
            }

            return S_OK;
        }

        template<typename ControlClass>
        static HRESULT KeyUp(_In_ wsy::VirtualKey key, _Out_ BOOLEAN* pbHandled, bool bAcceptsReturn, _Inout_ ControlClass* control)
        {
            BOOLEAN bIsEnabled = false;
            xaml_controls::ClickMode nClickMode = xaml_controls::ClickMode_Release;

            *pbHandled = FALSE;

            IFC_RETURN(control->get_IsEnabled(&bIsEnabled));
            IFC_RETURN(control->get_ClickMode(&nClickMode));

            // Key presses can be ignored when disabled or in xaml_controls::ClickMode.Hover or if any
            // other key than SPACE, ENTER, or GAMEPAD_A was released.
            if (bIsEnabled &&
                nClickMode != xaml_controls::ClickMode_Hover &&
                IsPress(key, bAcceptsReturn))
            {
                if (key == wsy::VirtualKey_Space || (key == wsy::VirtualKey_Enter && bAcceptsReturn))
                {
                    control->m_bIsSpaceOrEnterKeyDown = false;
                }
                else if (key == wsy::VirtualKey_GamepadA)
                {
                    control->m_bIsNavigationAcceptOrGamepadAKeyDown = false;
                }

                // If the pointer isn't in use, raise the Click event if we're in the
                // correct click mode
                if (!control->m_bIsPointerLeftButtonDown)
                {
                    BOOLEAN bIsPressed = false;

                    IFC_RETURN(control->get_IsPressed(&bIsPressed));

                    if (bIsPressed && nClickMode == xaml_controls::ClickMode_Release)
                    {
                        IFC_RETURN(control->OnClick());
                    }

                    IFC_RETURN(control->put_IsPressed(FALSE));
                }

                *pbHandled = TRUE;

                EndFocusPress(checked_cast<CUIElement>(control->GetHandle()));
            }

            return S_OK;
        }
    };
}