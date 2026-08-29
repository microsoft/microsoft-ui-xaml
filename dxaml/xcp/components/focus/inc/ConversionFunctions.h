// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define XAML_SOURCE_FOCUS_NAVIGATION_REASON_NOT_SUPPORTED static_cast<xaml_hosting::XamlSourceFocusNavigationReason >(0xFFFF)
namespace DirectUI
{
    enum class FocusNavigationDirection : uint8_t;
    FocusNavigationDirection GetFocusNavigationDirectionFromReason(_In_ xaml_hosting::XamlSourceFocusNavigationReason reason);
    xaml_hosting::XamlSourceFocusNavigationReason GetFocusNavigationReasonFromDirection(_In_ FocusNavigationDirection direction);
    FocusNavigationDirection GetReverseDirection(_In_ FocusNavigationDirection direction);

    enum class InputDeviceType : uint8_t;
    InputDeviceType GetInputDeviceTypeFromDirection(_In_ FocusNavigationDirection direction);
}

