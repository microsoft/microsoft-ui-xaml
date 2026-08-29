// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AccessKeyIntegrationTests.h"

#include <XamlTailored.h>
#include <TreeHelper.h>
#include <TestCleanupWrapper.h>

#include "AccessKeyTestHelper.h"


using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

    //
    // Test Cases
    //

    void AccessKeyIntegrationTests::InvokeAccessKeysOnMultipleButtonsOfAppBar()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = nullptr;
        xaml_controls::Button^ button = nullptr;
        std::vector<xaml_primitives::ButtonBase^> buttons;

        // Setup our environment.
        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->TopAppBar = ref new xaml_controls::CommandBar();
            page->TopAppBar->IsOpen = true;
            page->TopAppBar->IsSticky = true;

            auto topStackPanel = ref new xaml_controls::StackPanel();
            topStackPanel->Orientation = xaml_controls::Orientation::Horizontal;

            auto topAppBarButton = ref new xaml_controls::AppBarButton();
            topAppBarButton->Content = "TopAppBarButton";
            topAppBarButton->ExitDisplayModeOnAccessKeyInvoked = false;
            topStackPanel->Children->Append(topAppBarButton);
            buttons.push_back(std::move(topAppBarButton));

            auto topAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
            topAppBarToggleButton->Content = "TopAppBarToggleButton";
            topAppBarToggleButton->ExitDisplayModeOnAccessKeyInvoked = false;
            topStackPanel->Children->Append(topAppBarToggleButton);
            buttons.push_back(std::move(topAppBarToggleButton));

            page->BottomAppBar = ref new xaml_controls::CommandBar();
            page->BottomAppBar->IsOpen = true;
            page->BottomAppBar->IsSticky = true;

            auto bottomStackPanel = ref new xaml_controls::StackPanel();
            bottomStackPanel->Orientation = xaml_controls::Orientation::Horizontal;

            auto bottomAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
            bottomAppBarToggleButton->Content = "BottomAppBarToggleButton";
            bottomAppBarToggleButton->ExitDisplayModeOnAccessKeyInvoked = false;
            bottomStackPanel->Children->Append(bottomAppBarToggleButton);
            buttons.push_back(std::move(bottomAppBarToggleButton));

            auto bottomAppBarButton = ref new xaml_controls::AppBarButton();
            bottomAppBarButton->Content = "BottomAppBarButton";
            bottomAppBarButton->ExitDisplayModeOnAccessKeyInvoked = false;
            bottomStackPanel->Children->Append(bottomAppBarButton);
            buttons.push_back(std::move(bottomAppBarButton));

            button = ref new xaml_controls::Button();
            button->Content = "BTN";
            button->ExitDisplayModeOnAccessKeyInvoked = false;

            page->Content = button;
            page->TopAppBar->Content = topStackPanel;
            page->BottomAppBar->Content = bottomStackPanel;

            TestServices::WindowHelper->WindowContent = page;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Start the test off with the button focused.
            button->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        AccessKeyTestHelper::InvokeAccessKeysOnMultipleButtons(buttons);

        RunOnUIThread([&]()
        {
            page->TopAppBar->IsOpen = false;
            page->BottomAppBar->IsOpen = false;
        });

        TestServices::WindowHelper->WaitForIdle();
    }

} } } } }
