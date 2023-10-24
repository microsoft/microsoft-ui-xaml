// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <enumdefs.g.h>
#include <interactioncontext.h>

namespace GestureConfig {

    const int MAX_CONFIGURATION_COUNT = 4;

    wui::GestureSettings GestureConfigurationBuilder(
        _In_ bool bTapEnabled,
        _In_ bool bDoubleTapEnabled,
        _In_ bool bRightTapEnabled,
        _In_ bool bHoldEnabled,
        _In_ DirectUI::ManipulationModes manipulationMode);

    int GestureConfigurationBuilder(
        _In_ wui::GestureSettings flags,
        _In_ bool tapEnabled,
        _In_ bool doubleTapEnabled,
        _In_ bool rightTapEnabled,
        _In_ bool holdEnabled,
        _In_ DirectUI::ManipulationModes manipulationMode,
        _Inout_ INTERACTION_CONTEXT_CONFIGURATION* configurationICM);

    INTERACTION_CONFIGURATION_FLAGS GestureSettingsToInteractionConfigManipulation(_In_ wui::GestureSettings settings);
    INTERACTION_CONFIGURATION_FLAGS GestureSettingsToInteractionConfigTap(_In_ wui::GestureSettings settings);
    INTERACTION_CONFIGURATION_FLAGS GestureSettingsToInteractionConfigRightTap(_In_ wui::GestureSettings settings);
    INTERACTION_CONFIGURATION_FLAGS GestureSettingsToInteractionConfigHold(_In_ wui::GestureSettings settings);
}