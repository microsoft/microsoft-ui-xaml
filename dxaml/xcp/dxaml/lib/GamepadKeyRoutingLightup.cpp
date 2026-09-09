// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This source file is intended for light-up code, requiring compilation with a UAP
// contract later than 7 (i.e., Windows build 17763, the downlevel limit for WinAppSDK).
// To permit that, we compile without a PCH which otherwise locks in those limits,
// and undefine the UAP contract version.  See also wrtdxamlfoundation.vcxproj.

// Undefine UAP contract version to enable latest for lightup code
#undef WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION
#include "precomp.h"    // This is a direct (non-PCH) compile
#include "GamepadKeyRoutingLightup.h"

// Windows.UI.Input.GamepadKeyRoutingConfiguration was introduced in UniversalApiContract 19
// (Windows 11, version 24H2). Gate on the contract version declared by the SDK headers rather than
// the value forced by the build (see WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION in
// dxaml\Xaml.Cpp.Targets), which the #undef above clears.
#define GAMEPAD_KEY_ROUTING_CONTRACT_VERSION 0x130000   // UniversalApiContract 19.0

#if WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= GAMEPAD_KEY_ROUTING_CONTRACT_VERSION
#include <windows.foundation.metadata.h>
#include <windows.ui.input.h>
#endif

_Check_return_ HRESULT DirectUI::EnableGamepadKeyRouting()
{
#if WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= GAMEPAD_KEY_ROUTING_CONTRACT_VERSION
    // Compiling against contract 19 only guarantees the type is declared, not that the OS we are
    // running on registers it.  Without this check the activation factory lookup below would fail on
    // every downlevel machine.
    ctl::ComPtr<wf::Metadata::IApiInformationStatics> apiInformationStatics;
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Metadata_ApiInformation).Get(),
        &apiInformationStatics));

    boolean isContractPresent = false;
    IFC_RETURN(apiInformationStatics->IsApiContractPresentByMajor(
        wrl_wrappers::HStringReference(L"Windows.Foundation.UniversalApiContract").Get(),
        19,
        &isContractPresent));

    if (!isContractPresent)
    {
        return S_OK;
    }

    boolean isTypePresent = false;
    IFC_RETURN(apiInformationStatics->IsTypePresent(
        wrl_wrappers::HStringReference(
            RuntimeClass_Windows_UI_Input_GamepadKeyRoutingConfiguration).Get(),
        &isTypePresent));

    if (!isTypePresent)
    {
        return S_OK;
    }

    ctl::ComPtr<wui::IGamepadKeyRoutingConfigurationStatics> gamepadKeyRoutingStatics;
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_Input_GamepadKeyRoutingConfiguration).Get(),
        &gamepadKeyRoutingStatics));

    // The API can be present but unusable on a particular device.
    boolean isSupported = false;
    IFC_RETURN(gamepadKeyRoutingStatics->IsSupported(&isSupported));

    if (!isSupported)
    {
        return S_OK;
    }

    // The OS reports back whether it applied the setting.  A refusal is not an error: gamepad
    // navigation is an enhancement, and the app remains fully usable without it.
    boolean wasApplied = false;
    IFC_RETURN(gamepadKeyRoutingStatics->TrySetKeyRoutingEnabled(true, &wasApplied));
#endif

    return S_OK;
}
