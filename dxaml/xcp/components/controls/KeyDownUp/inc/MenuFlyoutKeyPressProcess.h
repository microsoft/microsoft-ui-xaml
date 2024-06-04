// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace KeyPress {
    class MenuFlyoutPresenter
    {
    public:
        template<typename ControlClass>
        static HRESULT KeyDown(_In_ wsy::VirtualKey key, _In_ ControlClass* control, _Out_ BOOLEAN* pbHandled)
        {
            if (key == wsy::VirtualKey_Up ||
                key == wsy::VirtualKey_GamepadDPadUp ||
                key == wsy::VirtualKey_GamepadLeftThumbstickUp)
            {
                IFC_RETURN(control->HandleUpOrDownKey(false));
                *pbHandled = true;
            }
            else if (key == wsy::VirtualKey_Down ||
                     key == wsy::VirtualKey_GamepadDPadDown ||
                     key == wsy::VirtualKey_GamepadLeftThumbstickDown)
            {
                IFC_RETURN(control->HandleUpOrDownKey(true));
                *pbHandled = true;
            }
            else if (key == wsy::VirtualKey_Tab)
            {
                *pbHandled = true;
            }
            // Handle the left key to close the opened MenuFlyoutSubItem.
            // The right arrow key is directly handled from the MenuFlyoutSubItem
            // to open the sub menu item.
            else if (key == wsy::VirtualKey_Left ||
                     key == wsy::VirtualKey_Escape)
            {
                if (control->IsSubPresenter())
                {
                    IFC_RETURN(control->HandleKeyDownLeftOrEscape());
                    *pbHandled = true;
                }
                else
                {
                    // If this is the top-level menu, let Popup close it (on Escape key press)
                    *pbHandled = false;
                }
            }

            return S_OK;
        }
    };

    class MenuFlyout
    {
    public:
        template<typename ControlClass, typename ControlClassParentPresenter>
        static HRESULT KeyDown(_In_ wsy::VirtualKey key, _Inout_ ControlClass* control, _Out_ BOOLEAN* pbHandled)
        {
            // If SPACE/ENTER/NAVIGATION_ACCEPT/GAMEPAD_A is already down and a different key is now pressed,
            // then cancel the SPACE/ENTER/NAVIGATION_ACCEPT/GAMEPAD_A press.
            if (control->m_bIsSpaceOrEnterKeyDown || control->m_bIsNavigationAcceptOrGamepadAKeyDown)
            {
                if (key != wsy::VirtualKey_Space && key != wsy::VirtualKey_Enter && control->m_bIsSpaceOrEnterKeyDown)
                {
                    control->m_bIsSpaceOrEnterKeyDown = false;
                }
                if (control->m_bIsNavigationAcceptOrGamepadAKeyDown &&             // The key down flag is set
                    key != wsy::VirtualKey_GamepadA)                 // AND it's not the GamepadA key
                {
                    control->m_bIsNavigationAcceptOrGamepadAKeyDown = false;
                }

                control->m_bIsPressed = false;
                IFC_RETURN(control->UpdateVisualState());
            }

            if (key == wsy::VirtualKey_Up ||
                key == wsy::VirtualKey_GamepadDPadUp ||
                key == wsy::VirtualKey_GamepadLeftThumbstickUp)
            {
                ctl::ComPtr<ControlClassParentPresenter> spParentMenuFlyoutPresenter;
                IFC_RETURN(control->GetParentMenuFlyoutPresenter(&spParentMenuFlyoutPresenter));
                if (spParentMenuFlyoutPresenter)
                {
                    IFC_RETURN(spParentMenuFlyoutPresenter->HandleUpOrDownKey(false));
                    *pbHandled = true;
                }
            }
            else if (key == wsy::VirtualKey_Down ||
                     key == wsy::VirtualKey_GamepadDPadDown ||
                     key == wsy::VirtualKey_GamepadLeftThumbstickDown)
            {
                ctl::ComPtr<ControlClassParentPresenter> spParentMenuFlyoutPresenter;
                IFC_RETURN(control->GetParentMenuFlyoutPresenter(&spParentMenuFlyoutPresenter));
                if (spParentMenuFlyoutPresenter)
                {
                    IFC_RETURN(spParentMenuFlyoutPresenter->HandleUpOrDownKey(true));
                    *pbHandled = true;
                }
            }
            else if (key == wsy::VirtualKey_Space ||
                     key == wsy::VirtualKey_Enter ||
                     key == wsy::VirtualKey_GamepadA)
            {
                control->m_bIsPressed = true;

                if (key == wsy::VirtualKey_Space || key == wsy::VirtualKey_Enter)
                {
                    control->m_bIsSpaceOrEnterKeyDown = true;
                }
                else if (key == wsy::VirtualKey_GamepadA)
                {
                    control->m_bIsNavigationAcceptOrGamepadAKeyDown = true;
                }

                IFC_RETURN(control->UpdateVisualState());
                *pbHandled = true;
            }

            return S_OK;
        }

        template<typename ControlClass>
        static HRESULT KeyUp(_In_ wsy::VirtualKey key, _Inout_ ControlClass* control, _Out_ BOOLEAN* pbHandled)
        {
            if (key == wsy::VirtualKey_Space ||
                key == wsy::VirtualKey_Enter ||
                key == wsy::VirtualKey_GamepadA)
            {
                if (key == wsy::VirtualKey_Space || key == wsy::VirtualKey_Enter)
                {
                    control->m_bIsSpaceOrEnterKeyDown = false;
                }
                else if (key == wsy::VirtualKey_GamepadA)
                {
                    control->m_bIsNavigationAcceptOrGamepadAKeyDown = false;
                }

                if (control->m_bIsPressed && !control->m_bIsPointerLeftButtonDown)
                {
                    control->m_bIsPressed = FALSE;
                    IFC_RETURN(control->UpdateVisualState());
                    IFC_RETURN(control->Invoke());
                    *pbHandled = true;
                }
            }

            return S_OK;
        }
    };
}
