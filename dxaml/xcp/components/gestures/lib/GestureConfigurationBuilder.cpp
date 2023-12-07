// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "GestureConfigurationBuilder.h"
#include <InteractionContext.h>

bool HasManipulationMode(_In_ DirectUI::ManipulationModes modeValue, _In_ DirectUI::ManipulationModes modeToCheck)
{
    return (modeValue & modeToCheck) == modeToCheck;
}

wui::GestureSettings GestureConfig::GestureConfigurationBuilder(
    _In_ bool tapEnabled,
    _In_ bool doubleTapEnabled,
    _In_ bool rightTapEnabled,
    _In_ bool holdEnabled,
    _In_ DirectUI::ManipulationModes manipulationMode)
{
    wui::GestureSettings flags = wui::GestureSettings::GestureSettings_None;

    if (manipulationMode != DirectUI::ManipulationModes::None)
    {
        if (manipulationMode == DirectUI::ManipulationModes::All)
        {
            flags |= wui::GestureSettings::GestureSettings_ManipulationTranslateX | wui::GestureSettings::GestureSettings_ManipulationTranslateY | wui::GestureSettings::GestureSettings_ManipulationRotate
                | wui::GestureSettings::GestureSettings_ManipulationScale | wui::GestureSettings::GestureSettings_ManipulationTranslateInertia | wui::GestureSettings::GestureSettings_ManipulationRotateInertia
                | wui::GestureSettings::GestureSettings_ManipulationScaleInertia;
        }
        else
        {
            if (HasManipulationMode(manipulationMode, DirectUI::ManipulationModes::TranslateX))
            { flags |= wui::GestureSettings::GestureSettings_ManipulationTranslateX; }

            if (HasManipulationMode(manipulationMode, DirectUI::ManipulationModes::TranslateY))
            { flags |= wui::GestureSettings::GestureSettings_ManipulationTranslateY; }

            if (HasManipulationMode(manipulationMode, DirectUI::ManipulationModes::TranslateRailsX))
            { flags |= wui::GestureSettings::GestureSettings_ManipulationTranslateRailsX; }

            if (HasManipulationMode(manipulationMode, DirectUI::ManipulationModes::TranslateRailsY))
            { flags |= wui::GestureSettings::GestureSettings_ManipulationTranslateRailsY; }

            if (HasManipulationMode(manipulationMode, DirectUI::ManipulationModes::Rotate))
            { flags |= wui::GestureSettings::GestureSettings_ManipulationRotate; }

            if (HasManipulationMode(manipulationMode, DirectUI::ManipulationModes::Scale))
            { flags |= wui::GestureSettings::GestureSettings_ManipulationScale; }

            if (HasManipulationMode(manipulationMode, DirectUI::ManipulationModes::TranslateInertia))
            { flags |= wui::GestureSettings::GestureSettings_ManipulationTranslateInertia; }

            if (HasManipulationMode(manipulationMode, DirectUI::ManipulationModes::RotateInertia))
            { flags |= wui::GestureSettings::GestureSettings_ManipulationRotateInertia; }

            if (HasManipulationMode(manipulationMode, DirectUI::ManipulationModes::ScaleInertia))
            { flags |= wui::GestureSettings::GestureSettings_ManipulationScaleInertia; }
        }
    }

    if (tapEnabled) { flags |= wui::GestureSettings::GestureSettings_Tap; }
    if (doubleTapEnabled) { flags |= wui::GestureSettings::GestureSettings_DoubleTap; }
    if (rightTapEnabled) { flags |= wui::GestureSettings::GestureSettings_RightTap; }
    if (holdEnabled) { flags |= wui::GestureSettings::GestureSettings_Hold; }

    return flags;
}

INTERACTION_CONFIGURATION_FLAGS GestureConfig::GestureSettingsToInteractionConfigManipulation(_In_ wui::GestureSettings settings)
{
    INTERACTION_CONFIGURATION_FLAGS flags = INTERACTION_CONFIGURATION_FLAG_NONE;

    if (settings & wui::GestureSettings::GestureSettings_ManipulationTranslateX) { flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_X; }
    if (settings & wui::GestureSettings::GestureSettings_ManipulationTranslateY) { flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y; }
    if (settings & wui::GestureSettings::GestureSettings_ManipulationRotate) { flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_ROTATION; }
    if (settings & wui::GestureSettings::GestureSettings_ManipulationScale) { flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING; }
    if (settings & wui::GestureSettings::GestureSettings_ManipulationTranslateInertia) { flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_INERTIA; }
    if (settings & wui::GestureSettings::GestureSettings_ManipulationRotateInertia) { flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_ROTATION_INERTIA; }
    if (settings & wui::GestureSettings::GestureSettings_ManipulationScaleInertia) { flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING_INERTIA; }
    if (settings & wui::GestureSettings::GestureSettings_ManipulationTranslateRailsX) { flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_RAILS_X; }
    if (settings & wui::GestureSettings::GestureSettings_ManipulationTranslateRailsY) { flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_RAILS_Y; }

    if (flags != INTERACTION_CONFIGURATION_FLAG_NONE)
    {
        flags |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION;
    }

    return flags;
}

INTERACTION_CONFIGURATION_FLAGS GestureConfig::GestureSettingsToInteractionConfigTap(_In_ wui::GestureSettings settings)
{
    INTERACTION_CONFIGURATION_FLAGS flags = INTERACTION_CONFIGURATION_FLAG_NONE;

    if (settings & wui::GestureSettings::GestureSettings_Tap) { flags |= INTERACTION_CONFIGURATION_FLAG_TAP; }
    if (settings & wui::GestureSettings::GestureSettings_DoubleTap) { flags |= INTERACTION_CONFIGURATION_FLAG_TAP_DOUBLE; }

    return flags;
}

INTERACTION_CONFIGURATION_FLAGS GestureConfig::GestureSettingsToInteractionConfigRightTap(_In_ wui::GestureSettings settings)
{
    INTERACTION_CONFIGURATION_FLAGS flags = INTERACTION_CONFIGURATION_FLAG_NONE;

    if (settings & wui::GestureSettings::GestureSettings_RightTap) { flags |= INTERACTION_CONFIGURATION_FLAG_SECONDARY_TAP; }

    return flags;
}

INTERACTION_CONFIGURATION_FLAGS GestureConfig::GestureSettingsToInteractionConfigHold(_In_ wui::GestureSettings settings)
{
    INTERACTION_CONFIGURATION_FLAGS flags = INTERACTION_CONFIGURATION_FLAG_NONE;

    if (settings & wui::GestureSettings::GestureSettings_Hold) { flags |= INTERACTION_CONFIGURATION_FLAG_HOLD; }

    return flags;
}

int GestureConfig::GestureConfigurationBuilder(
    _In_ wui::GestureSettings flags,
    _In_ bool tapEnabled,
    _In_ bool doubleTapEnabled,
    _In_ bool rightTapEnabled,
    _In_ bool holdEnabled,
    _In_ DirectUI::ManipulationModes manipulationMode,
    _Inout_ INTERACTION_CONTEXT_CONFIGURATION* configurationICM)
{
    int uiConfigIndex = 0;
    wui::GestureSettings settings = GestureConfigurationBuilder(tapEnabled, doubleTapEnabled, rightTapEnabled, holdEnabled, manipulationMode);

    const INTERACTION_ID interactionIds[MAX_CONFIGURATION_COUNT] = {
        INTERACTION_ID_MANIPULATION,
        INTERACTION_ID_TAP,
        INTERACTION_ID_SECONDARY_TAP,
        INTERACTION_ID_HOLD
    };

    for (UINT32 i = 0; i < MAX_CONFIGURATION_COUNT; i++)
    {
        // set default value for each entry
        configurationICM[i].interactionId = INTERACTION_ID_NONE;
        configurationICM[i].enable = INTERACTION_CONFIGURATION_FLAG_NONE;

        switch (interactionIds[i])
        {
        case INTERACTION_ID_MANIPULATION:
            configurationICM[uiConfigIndex].enable = GestureSettingsToInteractionConfigManipulation(settings);
            break;

        case INTERACTION_ID_TAP:
            configurationICM[uiConfigIndex].enable = GestureSettingsToInteractionConfigTap(settings);
            break;

        case INTERACTION_ID_SECONDARY_TAP:
            configurationICM[uiConfigIndex].enable = GestureSettingsToInteractionConfigRightTap(settings);
            break;

        case INTERACTION_ID_HOLD:
            configurationICM[uiConfigIndex].enable = GestureSettingsToInteractionConfigHold(settings);
            break;
        }

        if (configurationICM[uiConfigIndex].enable != INTERACTION_CONFIGURATION_FLAG_NONE)
        {
            configurationICM[uiConfigIndex].interactionId = interactionIds[i];
            uiConfigIndex++;
        }
    }

    return uiConfigIndex;
}