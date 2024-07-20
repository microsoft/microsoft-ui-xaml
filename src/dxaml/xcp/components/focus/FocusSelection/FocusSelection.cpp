// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <FocusSelection.h>

using namespace Focus;

DirectUI::FocusNavigationDirection FocusSelection::GetNavigationDirection(_In_ wsy::VirtualKey key)
{
    DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::None;

    switch (key)
    {
    case wsy::VirtualKey_GamepadDPadUp:
    case wsy::VirtualKey_GamepadLeftThumbstickUp:
    case wsy::VirtualKey_Up:
        direction = DirectUI::FocusNavigationDirection::Up;
        break;
    case wsy::VirtualKey_GamepadDPadDown:
    case wsy::VirtualKey_GamepadLeftThumbstickDown:
    case wsy::VirtualKey_Down:
        direction = DirectUI::FocusNavigationDirection::Down;
        break;
    case wsy::VirtualKey_GamepadDPadLeft:
    case wsy::VirtualKey_GamepadLeftThumbstickLeft:
    case wsy::VirtualKey_Left:
        direction = DirectUI::FocusNavigationDirection::Left;
        break;
    case wsy::VirtualKey_GamepadDPadRight:
    case wsy::VirtualKey_GamepadLeftThumbstickRight:
    case wsy::VirtualKey_Right:
        direction = DirectUI::FocusNavigationDirection::Right;
        break;
    }

    return direction;
}

DirectUI::FocusNavigationDirection FocusSelection::GetNavigationDirectionForKeyboardArrow(_In_ wsy::VirtualKey key)
{
    if (key == wsy::VirtualKey_Up) { return DirectUI::FocusNavigationDirection::Up; }
    else if (key == wsy::VirtualKey_Down) { return DirectUI::FocusNavigationDirection::Down; }
    else if (key == wsy::VirtualKey_Left) { return DirectUI::FocusNavigationDirection::Left; }
    else if (key == wsy::VirtualKey_Right) { return DirectUI::FocusNavigationDirection::Right; }

    return DirectUI::FocusNavigationDirection::None;
}