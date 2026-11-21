// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // Pure adapter policy over synthetic DXGI_ADAPTER_DESC1 values (D2DDeviceFactory.h).
    class HardwareAdapterSelectionTests
    {
        BEGIN_TEST_CLASS(HardwareAdapterSelectionTests)
        END_TEST_CLASS()

        TEST_METHOD(RejectsSoftwareFlag);
        TEST_METHOD(RejectsBasicRenderDriverWithoutSoftwareFlag);
        TEST_METHOD(RejectsHyperVAndRemoteDisplayAdapters);
        TEST_METHOD(AcceptsNvidiaAmdAndIntel);
        TEST_METHOD(ChoosesFirstQualifyingInPreferenceOrder);
        TEST_METHOD(ChoosesNothingWhenNoneQualifies);
        TEST_METHOD(OverridePinsTheNamedAdapter);
        TEST_METHOD(OverrideNeverSubstitutesAnotherAdapter);
        TEST_METHOD(ParsesLuidOverride);
        TEST_METHOD(RejectsMalformedLuidOverride);
        TEST_METHOD(PacksLuidLikeDeviceInfo);
    };

    // CreateD3DDeviceForMode against this machine's real adapters. On a machine without a hardware GPU (e.g. a Hyper-V
    // VM) the hardware path must report DXGI_ERROR_NOT_FOUND, and these tests pass by checking exactly that.
    class HardwareDeviceFactoryTests
    {
        BEGIN_TEST_CLASS(HardwareDeviceFactoryTests)
        END_TEST_CLASS()

        TEST_METHOD(HardwareDeviceMatchesMachineEnumeration);
        TEST_METHOD(HardwareDeviceIsNeverWarpOrBasicRender);
        TEST_METHOD(MicrosoftAdaptersDoNotProvideHardwareRendering);
        TEST_METHOD(OverrideToUnknownOrRejectedAdapterIsNotFound);
        TEST_METHOD(OverrideToQualifyingAdapterIsHonored);
        TEST_METHOD(MalformedOverrideFails);
        TEST_METHOD(WarpPathStillWorks);
    };
}
