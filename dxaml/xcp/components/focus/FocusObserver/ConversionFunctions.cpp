// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ConversionFunctions.h"

#include <EnumDefs.g.h>

using namespace Jupiter;

namespace DirectUI {

    FocusNavigationDirection GetFocusNavigationDirectionFromReason(_In_ xaml_hosting::XamlSourceFocusNavigationReason reason)
    {
        switch (reason)
        {
            case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_First:
                return FocusNavigationDirection::Next;
            case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Last:
                return FocusNavigationDirection::Previous;
            case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Left:
                return FocusNavigationDirection::Left;
            case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Right:
                return FocusNavigationDirection::Right;
            case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Up:
                return FocusNavigationDirection::Up;
            case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Down:
                return FocusNavigationDirection::Down;

            default:
                return FocusNavigationDirection::None;
        }
    }

    xaml_hosting::XamlSourceFocusNavigationReason GetFocusNavigationReasonFromDirection(_In_ FocusNavigationDirection direction)
    {
        switch (direction)
        {
            case FocusNavigationDirection::Next:
                return xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_First;
            case FocusNavigationDirection::Previous:
                return xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Last;
            case FocusNavigationDirection::None:
                return xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Programmatic;
            case FocusNavigationDirection::Left:
                return xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Left;
            case FocusNavigationDirection::Right:
                return xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Right;
            case FocusNavigationDirection::Up:
                return xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Up;
            case FocusNavigationDirection::Down:
                return xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Down;
            default:
                return XAML_SOURCE_FOCUS_NAVIGATION_REASON_NOT_SUPPORTED;
        }
    }

    InputDeviceType GetInputDeviceTypeFromDirection(_In_ FocusNavigationDirection direction)
    {
        switch (direction)
        {
            case FocusNavigationDirection::Next:
            case FocusNavigationDirection::Previous:
                return InputDeviceType::Keyboard;
            case FocusNavigationDirection::Left:
            case FocusNavigationDirection::Right:
            case FocusNavigationDirection::Up:
            case FocusNavigationDirection::Down:
                // Bug: 14219605: OnFocusNavigating should include last input device
                // In XAML is it possible to do XY focus navigation via the keyboard or the gamepad
                // Currently when a OnFocusNavigating is received XAML has no way of knowing which device
                // was used to do XY navigation.
                // XAML assumes a Gamepad device but this might be wrong
                // in the cases where Keyboard navigation was used.
                return InputDeviceType::GamepadOrRemote;
            default:
                return InputDeviceType::None;
        }
    }

    FocusNavigationDirection GetReverseDirection(_In_ FocusNavigationDirection direction)
    {
        switch (direction)
        {
        case FocusNavigationDirection::Left:
            return FocusNavigationDirection::Right;
        case FocusNavigationDirection::Right:
            return FocusNavigationDirection::Left;
        case FocusNavigationDirection::Up:
            return FocusNavigationDirection::Down;
        case FocusNavigationDirection::Down:
            return FocusNavigationDirection::Up;
        case FocusNavigationDirection::Next:
            return FocusNavigationDirection::Previous;
        case FocusNavigationDirection::Previous:
            return FocusNavigationDirection::Next;
        default:
            IFCFAILFAST(E_INVALIDARG);
            return FocusNavigationDirection::None;
        }
    }

}