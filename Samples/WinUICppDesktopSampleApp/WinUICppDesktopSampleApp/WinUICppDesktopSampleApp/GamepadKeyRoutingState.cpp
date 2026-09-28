// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "GamepadKeyRoutingState.h"

// GamepadKeyRoutingConfiguration ships in UniversalApiContract 19, which this app's C++/WinRT
// projection does not know about: samples intentionally build against the 17763 contract metadata
// (see eng\lightup.targets). The Windows SDK ABI headers are not subject to that pin and do
// declare the type, so this talks to the ABI interface directly. Those headers also introduce a
// global ::Windows namespace that collides with the using-directives in MainWindow.cpp, which is
// why this lives in its own translation unit.
#undef WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION
#include <roapi.h>
#include <windows.foundation.metadata.h>
#include <windows.ui.input.h>

namespace
{
    winrt::hstring RuntimeClassName(wchar_t const* name)
    {
        return winrt::hstring{ name };
    }
}

namespace WinUICppDesktopSampleApp
{
    std::wstring GetGamepadKeyRoutingState()
    {
        namespace wf_metadata = ABI::Windows::Foundation::Metadata;
        namespace wui = ABI::Windows::UI::Input;

        auto const apiInformationName = RuntimeClassName(L"Windows.Foundation.Metadata.ApiInformation");
        winrt::com_ptr<wf_metadata::IApiInformationStatics> apiInformation;
        if (FAILED(RoGetActivationFactory(
                static_cast<HSTRING>(winrt::get_abi(apiInformationName)),
                __uuidof(wf_metadata::IApiInformationStatics),
                apiInformation.put_void())))
        {
            return L"Unavailable";
        }

        auto const contractName = RuntimeClassName(L"Windows.Foundation.UniversalApiContract");
        boolean isContractPresent = false;
        if (FAILED(apiInformation->IsApiContractPresentByMajor(
                static_cast<HSTRING>(winrt::get_abi(contractName)),
                19,
                &isContractPresent)) ||
            !isContractPresent)
        {
            return L"Unavailable";
        }

        auto const configurationName = RuntimeClassName(L"Windows.UI.Input.GamepadKeyRoutingConfiguration");
        winrt::com_ptr<wui::IGamepadKeyRoutingConfigurationStatics> gamepadKeyRouting;
        if (FAILED(RoGetActivationFactory(
                static_cast<HSTRING>(winrt::get_abi(configurationName)),
                __uuidof(wui::IGamepadKeyRoutingConfigurationStatics),
                gamepadKeyRouting.put_void())))
        {
            return L"Unavailable";
        }

        // IsSupported() reports the same OS velocity feature that gates the routing request
        // itself, so a false here means the machine cannot route gamepad input at all. That is
        // not a product failure, and the test treats it as inconclusive.
        boolean isSupported = false;
        if (FAILED(gamepadKeyRouting->IsSupported(&isSupported)) || !isSupported)
        {
            return L"Unavailable";
        }

        boolean isEnabled = false;
        if (FAILED(gamepadKeyRouting->get_IsKeyRoutingEnabled(&isEnabled)))
        {
            return L"Unavailable";
        }

        return isEnabled ? L"Enabled" : L"Disabled";
    }
}
