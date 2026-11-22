// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace ElementSound {

// These tests don't depend on an audio endpoint being present: they verify the ElementSoundPlayer API contract
// (state, volume, spatial audio mode, sound validation and interaction-sound requests), which is the same with and
// without audio hardware.
class ElementSoundPlayerTests : public WEX::TestClass<ElementSoundPlayerTests>
{
public:
    BEGIN_TEST_CLASS(ElementSoundPlayerTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(StateVolumeAndSpatialAudioModeRoundTrip)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies the ElementSoundPlayer defaults and that State, Volume and SpatialAudioMode round-trip, including while sounds are enabled.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VolumeOutOfRangeIsRejected)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that ElementSoundPlayer.Volume rejects values outside [0, 1] and keeps the previous value.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PlayValidatesSoundKind)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that ElementSoundPlayer.Play accepts every ElementSoundKind and rejects other values, with sounds off and on.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ButtonInvokeWithSpatialAudio)
        TEST_METHOD_PROPERTY(L"Description", L"Invokes buttons with sounds and spatial audio on, with the default and the Off ElementSoundMode.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InteractionSoundsForTextElementsAndFlyouts)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies effective sound modes and requests spatial interaction sounds for hyperlinks, flyouts with and without content and restricted elements.")
    END_TEST_METHOD()
};

} } } } } }
