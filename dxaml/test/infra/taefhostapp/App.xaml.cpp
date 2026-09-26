// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <WexTestClass.h>
#include <RuntimeParameters.h>

using namespace TaefHostApp;

App::App()
{
    RequiresPointerMode = Microsoft::UI::Xaml::ApplicationRequiresPointerMode::WhenRequested;
    InitializeComponent();
}

void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^ e)
{
    // Full-suite switcher opt-in: when the test pass requests SwitcherMode, engage the system composition
    // engine for THIS packaged host process BEFORE the window is activated (before any compositor is
    // created). This packaged host has package identity, so the LAF unlocks and CompositionEngine activates
    // -- unlike the unpackaged elevated ModuleSetup fixture, where the same call fails REGDB_E_CLASSNOTREG.
    // One engagement here covers every RunAs=UAP test hosted in this process. Strict no-op for normal passes
    // (SwitcherMode absent). Mirrors the engagement in ModuleCleanup.cpp / SwitcherTests::ClassSetup.
    WEX::Common::String switcherMode;
    if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"SwitcherMode", switcherMode))
        && (switcherMode.CompareNoCase(L"true") == 0 || switcherMode == L"1"))
    {
        try
        {
            WEX::Common::String lafToken;
            if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"SwitcherLafToken", lafToken))
                && !lafToken.IsEmpty())
            {
                Windows::ApplicationModel::LimitedAccessFeatures::TryUnlockFeature(
                    ref new Platform::String(L"com.microsoft.windows.composition.engine"),
                    ref new Platform::String(static_cast<const wchar_t*>(lafToken)),
                    ref new Platform::String(
                        L"8wekyb3d8bbwe has registered their use of "
                        L"com.microsoft.windows.composition.engine with Microsoft and agrees to the terms of use."));
            }

            Microsoft::UI::Composition::CompositionEngine::TrySetProcessEngine(
                Microsoft::UI::Composition::CompositionEngineType::System);
        }
        catch (Platform::Exception^)
        {
            // Non-fatal: if the switcher can't engage, the switcher tests report it; normal passes are unaffected.
        }
    }

    Microsoft::UI::Xaml::Window::Current->Activate();
    Microsoft::VisualStudio::TestPlatform::TestExecutor::WinRTCore::UnitTestClient::Run(e->Arguments);
}
