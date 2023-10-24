// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace KeyPress {
    class ToggleSwitch
    {
    public:
        template<typename ControlClass>
        static HRESULT KeyDown(_In_ wsy::VirtualKey key, _In_ ControlClass* control, _Out_ BOOLEAN* pbHandled)
        {

            if (control->HandlesKey(key))
            {
                *pbHandled = true;
            }

            return S_OK;
        }

        template<typename ControlClass>
        static HRESULT KeyUp(_In_ wsy::VirtualKey key, _Inout_ ControlClass* control, _Out_ BOOLEAN* pbHandled)
        {
            bool shouldToggleOff = FALSE;
            bool shouldToggleOn = FALSE;
            BOOLEAN handlesKey = FALSE;
            BOOLEAN handledKeyDown = FALSE;
            BOOLEAN isOn = false;

            auto flowDirection = xaml::FlowDirection_LeftToRight;
            IFC_RETURN(control->get_FlowDirection(&flowDirection));
            const bool isLTR = (flowDirection == xaml::FlowDirection_LeftToRight);

            handlesKey = control->HandlesKey(key);
            if (handlesKey)
            {
                handledKeyDown = control->m_handledKeyDown;
                control->m_handledKeyDown = false;
            }

            if (key == wsy::VirtualKey_GamepadA)
            {
                IFC_RETURN(control->Toggle());
                *pbHandled = true;
            }

            if (handlesKey && handledKeyDown && (!*pbHandled && !control->m_isDragging))
            {
                IFC_RETURN(control->get_IsOn(&isOn));

                if ((key == wsy::VirtualKey_Left && isLTR)          // Left toggles us off in LTR
                    || (key == wsy::VirtualKey_Right && !isLTR)     // Right toggles us off in RTR
                    || key == wsy::VirtualKey_Down
                    || key == wsy::VirtualKey_Home)
                {
                    shouldToggleOff = true;
                }
                else if ((key == wsy::VirtualKey_Right && isLTR)    // Right toggles us on in LTR
                    || (key == wsy::VirtualKey_Left && !isLTR)      // Left toggles us off in RTL
                    || key == wsy::VirtualKey_Up
                    || key == wsy::VirtualKey_End)
                {
                    shouldToggleOn = true;
                }

                if ((!isOn && shouldToggleOn) || (isOn && shouldToggleOff) || key == wsy::VirtualKey_Space)
                {
                    IFC_RETURN(control->Toggle());
                    *pbHandled = true;
                }
            }

            return S_OK;
        }
    };
}