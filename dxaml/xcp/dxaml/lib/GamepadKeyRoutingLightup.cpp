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

#include <windows.foundation.metadata.h>
#include <windows.ui.input.h>

// GamepadKeyRoutingConfiguration requires UniversalApiContract 19 (Windows 11, version 24H2).
static_assert(
    WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0x130000,
    "The Windows SDK in use predates UniversalApiContract 19 and does not declare "
    "Windows.UI.Input.GamepadKeyRoutingConfiguration.  Update the SDK version this repo builds against.");

_Check_return_ HRESULT DirectUI::EnableGamepadKeyRouting()
{
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

    ctl::ComPtr<wui::IGamepadKeyRoutingConfigurationStatics> gamepadKeyRoutingStatics;
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_Input_GamepadKeyRoutingConfiguration).Get(),
        &gamepadKeyRoutingStatics));

    // A refusal is not an error: gamepad navigation is an enhancement, and the app remains fully
    // usable without it.
    boolean wasApplied = false;
    IFC_RETURN(gamepadKeyRoutingStatics->TrySetKeyRoutingEnabled(true, &wasApplied));

    return S_OK;
}
