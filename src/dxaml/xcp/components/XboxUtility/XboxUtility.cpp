// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "paltypes.h"
#include "XboxUtility.h"

#include <FocusSelection.h>

namespace XboxUtility {

bool shouldReturnCachedIsOnXboxValue = false;

bool IsOnXbox()
{
    static bool isOnXboxResult = false;

    if (!shouldReturnCachedIsOnXboxValue)
    {
        ULONG platformId = 0;
        RtlGetDeviceFamilyInfoEnum(NULL, &platformId, NULL);
        shouldReturnCachedIsOnXboxValue = true;

        // Note: There are other _XBOX* defines in the SDK which may need to be
        //       evaluated (and documented) if this check is ever insufficient.
        isOnXboxResult = (platformId == DEVICEFAMILYINFOENUM_XBOX);
    }

    return isOnXboxResult;
}

void DeleteIsOnXboxCache()
{
    shouldReturnCachedIsOnXboxValue = false;
}

bool IsGamepadNavigationInput(_In_ wsy::VirtualKey key)
{
    return (int)key >= (int)wsy::VirtualKey_GamepadA && (int)key <= (int)wsy::VirtualKey_GamepadRightThumbstickLeft;
}

bool IsGamepadNavigationDirection(_In_ wsy::VirtualKey key)
{
    return
        IsGamepadNavigationRight(key) ||
        IsGamepadNavigationLeft(key) ||
        IsGamepadNavigationUp(key) ||
        IsGamepadNavigationDown(key);
}

bool IsGamepadPageNavigationDirection(_In_ wsy::VirtualKey key)
{
    return
        key == wsy::VirtualKey_GamepadLeftShoulder ||
        key == wsy::VirtualKey_GamepadRightShoulder ||
        key == wsy::VirtualKey_GamepadLeftTrigger ||
        key == wsy::VirtualKey_GamepadRightTrigger;
}

bool IsGamepadNavigationRight(_In_ wsy::VirtualKey key)
{
    return key == wsy::VirtualKey_GamepadLeftThumbstickRight || key == wsy::VirtualKey_GamepadDPadRight;
}

bool IsGamepadNavigationLeft(_In_ wsy::VirtualKey key)
{
    return key == wsy::VirtualKey_GamepadLeftThumbstickLeft || key == wsy::VirtualKey_GamepadDPadLeft;
}

bool IsGamepadNavigationUp(_In_ wsy::VirtualKey key)
{
    return key == wsy::VirtualKey_GamepadLeftThumbstickUp || key == wsy::VirtualKey_GamepadDPadUp;
}

bool IsGamepadNavigationDown(_In_ wsy::VirtualKey key)
{
    return key == wsy::VirtualKey_GamepadLeftThumbstickDown || key == wsy::VirtualKey_GamepadDPadDown;
}

bool IsGamepadNavigationAccept(_In_ wsy::VirtualKey key)
{
    return key == wsy::VirtualKey_GamepadA;
}

bool IsGamepadNavigationCancel(_In_ wsy::VirtualKey key)
{
    return key == wsy::VirtualKey_GamepadB;
}

xaml_input::FocusNavigationDirection GetPageNavigationDirection(_In_ wsy::VirtualKey key)
{
    xaml_input::FocusNavigationDirection gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None;
    switch (key)
    {
    case wsy::VirtualKey_GamepadLeftShoulder:
        gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left;
        break;
    case wsy::VirtualKey_GamepadRightShoulder:
        gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right;
        break;
    case wsy::VirtualKey_GamepadLeftTrigger:
        gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Up;
        break;
    case wsy::VirtualKey_GamepadRightTrigger:
        gamepadDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down;
        break;
    }

    return gamepadDirection;
}

xaml_input::FocusNavigationDirection GetNavigationDirection(_In_ wsy::VirtualKey key)
{
    if (IsGamepadNavigationDirection(key) == false) { return xaml_input::FocusNavigationDirection::FocusNavigationDirection_None; }

    return static_cast<xaml_input::FocusNavigationDirection>(Focus::FocusSelection::GetNavigationDirection(key));
}

} // namespace

