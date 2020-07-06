// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

class FocusHelper
{
public:
    static bool constexpr IsGamepadNavigationDirection(winrt::VirtualKey key)
    {
        return key == winrt::VirtualKey::GamepadLeftThumbstickDown
            || key == winrt::VirtualKey::GamepadDPadDown
            || key == winrt::VirtualKey::GamepadLeftThumbstickUp
            || key == winrt::VirtualKey::GamepadDPadUp
            || key == winrt::VirtualKey::GamepadLeftThumbstickRight
            || key == winrt::VirtualKey::GamepadDPadRight
            || key == winrt::VirtualKey::GamepadLeftThumbstickLeft
            || key == winrt::VirtualKey::GamepadDPadLeft;
    }

    static bool constexpr IsGamepadPageNavigationDirection(winrt::VirtualKey key)
    {
        return
            key == winrt::VirtualKey::GamepadLeftShoulder ||
            key == winrt::VirtualKey::GamepadRightShoulder ||
            key == winrt::VirtualKey::GamepadLeftTrigger ||
            key == winrt::VirtualKey::GamepadRightTrigger;
    }

    static winrt::FocusNavigationDirection constexpr GetNavigationDirection(winrt::VirtualKey key)
    {
        winrt::FocusNavigationDirection direction = winrt::FocusNavigationDirection::None;

        switch (key)
        {
        case winrt::VirtualKey::GamepadDPadUp:
        case winrt::VirtualKey::GamepadLeftThumbstickUp:
        case winrt::VirtualKey::Up:
            direction = winrt::FocusNavigationDirection::Up;
            break;
        case winrt::VirtualKey::GamepadDPadDown:
        case winrt::VirtualKey::GamepadLeftThumbstickDown:
        case winrt::VirtualKey::Down:
            direction = winrt::FocusNavigationDirection::Down;
            break;
        case winrt::VirtualKey::GamepadDPadLeft:
        case winrt::VirtualKey::GamepadLeftThumbstickLeft:
        case winrt::VirtualKey::Left:
            direction = winrt::FocusNavigationDirection::Left;
            break;
        case winrt::VirtualKey::GamepadDPadRight:
        case winrt::VirtualKey::GamepadLeftThumbstickRight:
        case winrt::VirtualKey::Right:
            direction = winrt::FocusNavigationDirection::Right;
            break;
        }

        return direction;
    }

    static winrt::FocusNavigationDirection constexpr GetPageNavigationDirection(winrt::VirtualKey key)
    {
        winrt::FocusNavigationDirection direction = winrt::FocusNavigationDirection::None;

        switch (key)
        {
        case winrt::VirtualKey::GamepadLeftTrigger:
            direction = winrt::FocusNavigationDirection::Up;
            break;
        case winrt::VirtualKey::GamepadRightTrigger:
            direction = winrt::FocusNavigationDirection::Down;
            break;
        case winrt::VirtualKey::GamepadLeftShoulder:
            direction = winrt::FocusNavigationDirection::Left;
            break;
        case winrt::VirtualKey::GamepadRightShoulder:
            direction = winrt::FocusNavigationDirection::Right;
            break;
        }

        return direction;
    }

    static winrt::FocusNavigationDirection constexpr GetOppositeDirection(winrt::FocusNavigationDirection direction)
    {
        winrt::FocusNavigationDirection oppositeDirection = winrt::FocusNavigationDirection::None;
        switch (direction)
        {
        case winrt::FocusNavigationDirection::Down:
            oppositeDirection = winrt::FocusNavigationDirection::Up;
            break;
        case winrt::FocusNavigationDirection::Up:
            oppositeDirection = winrt::FocusNavigationDirection::Down;
            break;
        case winrt::FocusNavigationDirection::Left:
            oppositeDirection = winrt::FocusNavigationDirection::Right;
            break;
        case winrt::FocusNavigationDirection::Right:
            oppositeDirection = winrt::FocusNavigationDirection::Left;
            break;
        case winrt::FocusNavigationDirection::Next:
            oppositeDirection = winrt::FocusNavigationDirection::Previous;
            break;
        case winrt::FocusNavigationDirection::Previous:
            oppositeDirection = winrt::FocusNavigationDirection::Next;
            break;
        }
        return oppositeDirection;
    }

    static winrt::UIElement GetUIElementForFocusCandidate(const winrt::DependencyObject& dobj)
    {
        auto uielement = dobj.try_as<winrt::UIElement>();
        auto parent = dobj;
        while (uielement == nullptr && parent != nullptr)
        {
            parent = winrt::VisualTreeHelper::GetParent(dobj);
            if (parent)
            {
                uielement = dobj.try_as<winrt::UIElement>();
            }
        }

        return uielement;
    }
};
