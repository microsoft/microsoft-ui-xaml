// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlLogging.h>

#include <GestureConfigurationBuilder.h>
#include <GestureConfigurationBuilderUnitTests.h>

#include <InteractionContext.h>

using namespace Microsoft::WRL;
using namespace WEX::TestExecution;
using namespace GestureConfig;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Input { namespace Gestures {

INTERACTION_CONFIGURATION_FLAGS AllManipulationForInteractionConfig()
{
    return INTERACTION_CONFIGURATION_FLAG_MANIPULATION | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_X | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_ROTATION
        | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_INERTIA | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_ROTATION_INERTIA
        | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING_INERTIA;
}

void GestureConfigurationBuilderUnitTests::VerifyGestureConfigurationBuilder()
{
    wui::GestureSettings flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::None);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_None); // No flags should be set

    flags = GestureConfigurationBuilder(true, false, false, false, DirectUI::ManipulationModes::None); // tap
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_Tap);

    flags = GestureConfigurationBuilder(false, true, false, false, DirectUI::ManipulationModes::None); // double tap
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_DoubleTap);

    flags = GestureConfigurationBuilder(false, false, true, false, DirectUI::ManipulationModes::None); // right tapped
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_RightTap);

    flags = GestureConfigurationBuilder(false, false, false, true, DirectUI::ManipulationModes::None); // hold
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_Hold);

    flags = GestureConfigurationBuilder(true, true, false, false, DirectUI::ManipulationModes::None); // tap and double tap
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_Tap | wui::GestureSettings::GestureSettings_DoubleTap);

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::All); // all manipulations
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationTranslateX | wui::GestureSettings::GestureSettings_ManipulationTranslateY
        | wui::GestureSettings::GestureSettings_ManipulationRotate | wui::GestureSettings::GestureSettings_ManipulationScale | wui::GestureSettings::GestureSettings_ManipulationTranslateInertia
        | wui::GestureSettings::GestureSettings_ManipulationRotateInertia | wui::GestureSettings::GestureSettings_ManipulationScaleInertia);
    VERIFY_ARE_EQUAL(wui::GestureSettings::GestureSettings_None, flags & (wui::GestureSettings::GestureSettings_ManipulationTranslateRailsX | wui::GestureSettings::GestureSettings_ManipulationTranslateRailsY)); // rails are not part of 'all' manipulations

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::TranslateX);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationTranslateX);

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::TranslateY);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationTranslateY);

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::TranslateRailsX);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationTranslateRailsX);

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::TranslateRailsY);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationTranslateRailsY);

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::Rotate);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationRotate);

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::Scale);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationScale);

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::TranslateInertia);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationTranslateInertia);

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::RotateInertia);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationRotateInertia);

    flags = GestureConfigurationBuilder(false, false, false, false, DirectUI::ManipulationModes::ScaleInertia);
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationScaleInertia);

    flags = GestureConfigurationBuilder(true, true, true, true, DirectUI::ManipulationModes::All); // all options (exluding rails)
    VERIFY_ARE_EQUAL(flags, wui::GestureSettings::GestureSettings_ManipulationTranslateX | wui::GestureSettings::GestureSettings_ManipulationTranslateY
        | wui::GestureSettings::GestureSettings_ManipulationRotate | wui::GestureSettings::GestureSettings_ManipulationScale | wui::GestureSettings::GestureSettings_ManipulationTranslateInertia
        | wui::GestureSettings::GestureSettings_ManipulationRotateInertia | wui::GestureSettings::GestureSettings_ManipulationScaleInertia
        | wui::GestureSettings::GestureSettings_Tap | wui::GestureSettings::GestureSettings_DoubleTap | wui::GestureSettings::GestureSettings_RightTap | wui::GestureSettings::GestureSettings_Hold);
}

void GestureConfigurationBuilderUnitTests::VerifyGestureConfigurationBuilderForInteractionConfig()
{
    INTERACTION_CONTEXT_CONFIGURATION configurationICM[MAX_CONFIGURATION_COUNT];

    int count = GestureConfigurationBuilder(wui::GestureSettings::GestureSettings_None, true, true, true, true, DirectUI::ManipulationModes::All, configurationICM);

    VERIFY_ARE_EQUAL(configurationICM[0].interactionId, INTERACTION_ID_MANIPULATION);
    VERIFY_IS_TRUE(configurationICM[0].enable == AllManipulationForInteractionConfig());

    VERIFY_ARE_EQUAL(configurationICM[1].interactionId, INTERACTION_ID_TAP);
    VERIFY_IS_TRUE(configurationICM[1].enable == (INTERACTION_CONFIGURATION_FLAG_TAP | INTERACTION_CONFIGURATION_FLAG_TAP_DOUBLE));

    VERIFY_ARE_EQUAL(configurationICM[2].interactionId, INTERACTION_ID_SECONDARY_TAP);
    VERIFY_IS_TRUE(configurationICM[2].enable == (INTERACTION_CONFIGURATION_FLAG_SECONDARY_TAP));

    VERIFY_ARE_EQUAL(configurationICM[3].interactionId, INTERACTION_ID_HOLD);
    VERIFY_IS_TRUE(configurationICM[3].enable == (INTERACTION_CONFIGURATION_FLAG_HOLD));

    VERIFY_ARE_EQUAL(count, 4);
}

void GestureConfigurationBuilderUnitTests::VerifyGestureConfigurationBuilderForInteractionConfigPartial()
{
    INTERACTION_CONTEXT_CONFIGURATION configurationICM[4];

    int count = GestureConfigurationBuilder(wui::GestureSettings::GestureSettings_None, true, false, false, true, DirectUI::ManipulationModes::None, configurationICM);

    VERIFY_ARE_EQUAL(configurationICM[0].interactionId, INTERACTION_ID_TAP);
    VERIFY_IS_TRUE(configurationICM[0].enable == (INTERACTION_CONFIGURATION_FLAG_TAP));

    VERIFY_ARE_EQUAL(configurationICM[1].interactionId, INTERACTION_ID_HOLD);
    VERIFY_IS_TRUE(configurationICM[1].enable == (INTERACTION_CONFIGURATION_FLAG_HOLD));

    VERIFY_ARE_EQUAL(configurationICM[2].interactionId, INTERACTION_ID_NONE);
    VERIFY_IS_TRUE(configurationICM[2].enable == INTERACTION_CONFIGURATION_FLAG_NONE);

    VERIFY_ARE_EQUAL(configurationICM[3].interactionId, INTERACTION_ID_NONE);
    VERIFY_IS_TRUE(configurationICM[3].enable == INTERACTION_CONFIGURATION_FLAG_NONE);

    VERIFY_ARE_EQUAL(count, 2);
}

void GestureConfigurationBuilderUnitTests::VerifyGestureSettingsToInteractionConfigManipulation()
{
    wui::GestureSettings settings = wui::GestureSettings::GestureSettings_ManipulationTranslateX | wui::GestureSettings::GestureSettings_ManipulationTranslateY | wui::GestureSettings::GestureSettings_ManipulationRotate
        | wui::GestureSettings::GestureSettings_ManipulationScale | wui::GestureSettings::GestureSettings_ManipulationTranslateInertia | wui::GestureSettings::GestureSettings_ManipulationRotateInertia | wui::GestureSettings::GestureSettings_ManipulationScaleInertia;

    VERIFY_IS_TRUE(GestureSettingsToInteractionConfigManipulation(settings) == AllManipulationForInteractionConfig());
}

void GestureConfigurationBuilderUnitTests::VerifyGestureSettingsToInteractionConfigTap()
{
    VERIFY_IS_TRUE(GestureSettingsToInteractionConfigTap(wui::GestureSettings::GestureSettings_Tap | wui::GestureSettings::GestureSettings_DoubleTap) == (INTERACTION_CONFIGURATION_FLAG_TAP | INTERACTION_CONFIGURATION_FLAG_TAP_DOUBLE));
}

void GestureConfigurationBuilderUnitTests::VerifyGestureSettingsToInteractionConfigRightTap()
{
    VERIFY_IS_TRUE(GestureSettingsToInteractionConfigRightTap(wui::GestureSettings::GestureSettings_RightTap) == INTERACTION_CONFIGURATION_FLAG_SECONDARY_TAP);
}

void GestureConfigurationBuilderUnitTests::VerifyGestureSettingsToInteractionConfigHold()
{
    VERIFY_IS_TRUE(GestureSettingsToInteractionConfigHold(wui::GestureSettings::GestureSettings_Hold) == INTERACTION_CONFIGURATION_FLAG_HOLD);
}

} } } } } }
