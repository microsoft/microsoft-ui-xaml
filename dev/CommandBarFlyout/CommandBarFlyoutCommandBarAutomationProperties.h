// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CommandBarFlyoutCommandBarAutomationProperties.g.h"
#include "CommandBarFlyoutCommandBarAutomationProperties.properties.h"

class CommandBarFlyoutCommandBarAutomationProperties :
    public CommandBarFlyoutCommandBarAutomationPropertiesProperties
{
public:
    static void OnControlTypePropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);
};

class CommandBarFlyoutCommandBarAutomationPropertiesFactory :
    public winrt::factory_implementation::CommandBarFlyoutCommandBarAutomationPropertiesT<CommandBarFlyoutCommandBarAutomationPropertiesFactory, CommandBarFlyoutCommandBarAutomationProperties>
{
};
