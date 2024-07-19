// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace KeyPress {         
    class Slider
    {
    public:
        template<typename ControlClass>
        static HRESULT KeyDown(_In_ wsy::VirtualKey key, _In_ ControlClass* control, _Out_ BOOLEAN* pbHandled)
        {
            *pbHandled = false;

            BOOLEAN shouldInvert = false;
            BOOLEAN shouldReverse = false;
            xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
            xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

            double delta = 0;

            IFC_RETURN(control->get_Orientation(&orientation));
            IFC_RETURN(control->get_FlowDirection(&flowDirection));
            IFC_RETURN(control->get_IsDirectionReversed(&shouldReverse));

            shouldInvert = (flowDirection == xaml::FlowDirection_RightToLeft) ^ shouldReverse;
            
            if (key == wsy::VirtualKey_Left ||
                (orientation == xaml_controls::Orientation_Horizontal &&
                 (key == wsy::VirtualKey_GamepadDPadLeft ||
                  key == wsy::VirtualKey_GamepadLeftThumbstickLeft)))
            {
                control->Step(true, shouldInvert);
                *pbHandled = true;
            }
            else if (key == wsy::VirtualKey_Right ||
                     (orientation == xaml_controls::Orientation_Horizontal &&
                      (key == wsy::VirtualKey_GamepadDPadRight ||
                       key == wsy::VirtualKey_GamepadLeftThumbstickRight)))
            {
                control->Step(true, !shouldInvert);
                *pbHandled = true;
            }
            else if (key == wsy::VirtualKey_Up ||
                     (orientation == xaml_controls::Orientation_Vertical &&
                      (key == wsy::VirtualKey_GamepadDPadUp ||
                       key == wsy::VirtualKey_GamepadLeftThumbstickUp)))
            {
                control->Step(true, !shouldReverse);
                *pbHandled = true;
            }
            else if (key == wsy::VirtualKey_Down ||
                     (orientation == xaml_controls::Orientation_Vertical &&
                      (key == wsy::VirtualKey_GamepadDPadDown ||
                       key == wsy::VirtualKey_GamepadLeftThumbstickDown)))
            {
                control->Step(true, shouldReverse);
                *pbHandled = true;
            }
            else if (key == wsy::VirtualKey_Home || key == wsy::VirtualKey_GamepadLeftShoulder)
            {
                IFC_RETURN(control->get_Minimum(&delta));
                IFC_RETURN(control->put_Value(delta));
                *pbHandled = true;
            }
            else if (key == wsy::VirtualKey_End || key == wsy::VirtualKey_GamepadRightShoulder)
            {
                IFC_RETURN(control->get_Maximum(&delta));
                IFC_RETURN(control->put_Value(delta));
                *pbHandled = true;
            }

            return S_OK;
        }
    };
}