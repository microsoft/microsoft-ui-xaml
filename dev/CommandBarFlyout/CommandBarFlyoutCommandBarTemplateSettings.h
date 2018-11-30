// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CommandBarFlyoutCommandBarTemplateSettings.g.h"
#include "CommandBarFlyoutCommandBarTemplateSettings.properties.h"

class CommandBarFlyoutCommandBarTemplateSettings :
    public winrt::implementation::CommandBarFlyoutCommandBarTemplateSettingsT<CommandBarFlyoutCommandBarTemplateSettings>,
    public CommandBarFlyoutCommandBarTemplateSettingsProperties
{
};

// Not actually a factory since this class is not activatable,
// but XamlMetadataProviderGenerated.cpp needs this type to exist
// to properly plug into the DP ensure/clear pipeline.
struct CommandBarFlyoutCommandBarTemplateSettingsFactory
{
    static void ClearProperties() { CommandBarFlyoutCommandBarTemplateSettings::ClearProperties(); }
    static void EnsureProperties() { CommandBarFlyoutCommandBarTemplateSettings::EnsureProperties(); }
};