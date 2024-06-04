// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "CommandBarFlyoutCommandBarAutomationProperties.h"

// Forward changes to CommandBarFlyoutCommandBarAutomationProperties.ControlType on to
// AutomationProperties.ControlType.  We need to do this indirection since we can't
// access types in XAML that we're interacting with via a known interface GUID.
/* static */ void CommandBarFlyoutCommandBarAutomationProperties::OnControlTypePropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto senderAsUIE = sender.try_as<winrt::UIElement>())
    {
        winrt::AutomationProperties::SetAutomationControlType(senderAsUIE, unbox_value<winrt::AutomationControlType>(args.NewValue()));
    }
}
