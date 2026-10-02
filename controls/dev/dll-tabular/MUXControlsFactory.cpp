// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlMetadataProvider.h"
#include "MUXControlsFactory.h"
#include <mutex>

#ifdef MATERIALS_INCLUDED
#ifdef MUX_PRERELEASE
#include "RevealBrush.h"
#endif
#endif

#include "XamlControlsTabularXamlMetaDataProvider.g.cpp"

extern "C" void __stdcall DeinitializeTabular();

bool MUXControlsFactory::s_initialized{ false };

void MUXControlsFactory::EnsureInitialized()
{
    static std::once_flag processShutdownSubscription;
    std::call_once(processShutdownSubscription, []()
    {
        winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::WinUIProcessShutdownStarting(
            [](const auto&, const auto&)
            {
                DeinitializeTabular();
            });
    });

    if (!s_initialized)
    {
        // Need to register here the DPs of types which are not referenced in our XAML but whose attached properties are.
#ifdef MATERIALS_INCLUDED
#ifdef MUX_PRERELEASE
        RevealBrush::EnsureProperties();
#endif
#endif
        s_initialized = true;
    }
}

void MUXControlsFactory::VerifyInitialized()
{
    if (!s_initialized)
    {
        throw winrt::hresult_error(E_FAIL, L"ERROR: You must put an instance of " MUXCONTROLSROOT_NAMESPACE_STR ".Controls.Tabular.TabularControlsResources in your Application.Resources.MergedDictionaries.");
    }
}
