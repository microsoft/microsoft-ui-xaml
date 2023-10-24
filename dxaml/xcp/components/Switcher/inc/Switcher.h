// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// This header is shared by test and product code

#include <roapi.h>
#include <windows.foundation.metadata.h>
#include <wrl/wrappers/corewrappers.h>
#include <array>

#ifndef IFCFAILFAST
#define IFCFAILFAST(x) FAIL_FAST_IF_FAILED(x)
#endif

enum class OSVersion
{
    WIN10_RS2 = 4,
    WIN10_RS3 = 5,
    WIN10_RS4 = 6,
    WIN10_RS5 = 7,
    WIN10_19H1 = 8,
    WIN10_VIBRANIUM = 10,
};

struct Switcher
{
    static OSVersion HighestContractSupported()
    {
        static OSVersion currentOSVersion = GetOSVersion();

        return currentOSVersion;
    }

    static bool IsContractSupported(OSVersion version)
    {
        return version <= HighestContractSupported();
    }

private:
    static bool IsAPIContractAvailable(
        _In_ wf::Metadata::IApiInformationStatics* apiInformationStatics, 
        const HSTRING contractName, 
        const OSVersion& version)
    {
        boolean isAPIContractAvailable = false;

        IFCFAILFAST(apiInformationStatics->IsApiContractPresentByMajor(
            contractName,
            static_cast<UINT16>(version),
            &isAPIContractAvailable));

        return !!isAPIContractAvailable;
    }

    struct RoInitializeHelper
    {
        void RoInitialize()
        {
            IFCFAILFAST(::RoInitialize(RO_INIT_SINGLETHREADED));
            m_calledRoInit = true;
        }

        ~RoInitializeHelper()
        {
            if (m_calledRoInit)
            {
                ::RoUninitialize();
            }
        }
        bool m_calledRoInit = false;
    };

    static OSVersion GetOSVersion()
    {
        RoInitializeHelper roInitHelper;

        wrl_wrappers::HStringReference contractName(L"Windows.Foundation.UniversalApiContract");
        Microsoft::WRL::ComPtr<wf::Metadata::IApiInformationStatics> apiInformationStatics;

        const HRESULT factoryHr = wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Metadata_ApiInformation).Get(),
            apiInformationStatics.GetAddressOf());
        if (factoryHr == CO_E_NOTINITIALIZED)
        {
            // We failed because RoInitialize has not been called.  Initialize it now and retry.
            roInitHelper.RoInitialize();
            IFCFAILFAST(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Metadata_ApiInformation).Get(),
                apiInformationStatics.GetAddressOf()));
        }
        else
        {
            IFCFAILFAST(factoryHr);
        }

        constexpr std::array<OSVersion, 6> supportedOSVersions{
            OSVersion::WIN10_VIBRANIUM,
            OSVersion::WIN10_19H1,
            OSVersion::WIN10_RS5,
            OSVersion::WIN10_RS4,
            OSVersion::WIN10_RS3,
            OSVersion::WIN10_RS2,
        };

        for (OSVersion version : supportedOSVersions)
        {
            if (IsAPIContractAvailable(apiInformationStatics.Get(), contractName.Get(), version))
            {
                return version;
            }
        }

        FAIL_FAST();
    }
};
