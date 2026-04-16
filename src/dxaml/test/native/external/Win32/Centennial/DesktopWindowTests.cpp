// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "DesktopWindowTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;

using namespace test_infra;

namespace Microsoft::UI::Xaml::Tests::DesktopWindow {

        bool DesktopWindowTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool DesktopWindowTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool DesktopWindowTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void DesktopWindowTests::ValidateDesktopWindowLifeTime()
        {
            TestCleanupWrapper cleanup;

            Button^ btn = nullptr;
            RunOnUIThread([&]()
                {
                    StackPanel^ mainStackPanel = ref new StackPanel();
                    btn = ref new Button();
                    btn->Width = 150;
                    btn->Height = 50;
                    btn->Content = "Button";
                    btn->HorizontalAlignment = HorizontalAlignment::Center;
                    mainStackPanel->Children->Append(btn);
                    void* rawWinPtr = nullptr; // don't care what is in address.

                    // This scope will make desktopWindow to get released. However, as now we have pegged window, window will be still alive
                    {
                        Window^ desktopWindow = ref new Window();
                        desktopWindow->Content = mainStackPanel;

                        // Get the address of window pointed by hat pointer before we release it
                        rawWinPtr = reinterpret_cast<void*>(safe_cast<Platform::Object^>(desktopWindow));
                        desktopWindow->Activate();
                        desktopWindow = nullptr; 
                    }

                    // Close the desktop window
                    Window^ hatWinPtr = reinterpret_cast<Window^>(rawWinPtr);
                    hatWinPtr->Close();
                });
            TestServices::WindowHelper->WaitForIdle();
        }

}

