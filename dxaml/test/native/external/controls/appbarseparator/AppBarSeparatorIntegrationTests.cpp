// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AppBarSeparatorIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <ControlHelper.H>
#include <TestCleanupWrapper.h>
#include <PopupHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarSeparator {

    bool AppBarSeparatorIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool AppBarSeparatorIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AppBarSeparatorIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void AppBarSeparatorIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::AppBarSeparator>::CanInstantiate();
    }

    void AppBarSeparatorIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::AppBarSeparator>::CanEnterAndLeaveLiveTree();
    }

    void AppBarSeparatorIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            PopupHelper::AreWindowedPopupsEnabled() ? L"Windowed" : L"Unwindowed",
            wf::Size(500, 800),
            1.f,
            []()
            {
                // Set the content first before send Tab() key not to hang for injecting input.
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
                });
                TestServices::WindowHelper->WaitForIdle();

                // Inject a tab to ensure a consistent sizing of the overflow menu
                // since it's based on last input mode.
                TestServices::KeyboardHelper->Tab();

                xaml_controls::Grid^ rootPanel = nullptr;
                xaml_controls::CommandBar^ cmdBar = nullptr;

                xaml_controls::AppBarSeparator^ fullSizeAppBarSeparator;
                xaml_controls::AppBarSeparator^ compactAppBarSeparator;
                xaml_controls::AppBarSeparator^ overflowAppBarSeparator;

                RunOnUIThread([&]()
                {
                    cmdBar = ref new xaml_controls::CommandBar();
                    cmdBar->VerticalAlignment = xaml::VerticalAlignment::Top;
                    cmdBar->IsOpen = true;

                    fullSizeAppBarSeparator = ref new xaml_controls::AppBarSeparator();
                    cmdBar->PrimaryCommands->Append(fullSizeAppBarSeparator);

                    compactAppBarSeparator = ref new xaml_controls::AppBarSeparator();
                    cmdBar->PrimaryCommands->Append(compactAppBarSeparator);

                    overflowAppBarSeparator = ref new xaml_controls::AppBarSeparator();
                    cmdBar->SecondaryCommands->Append(overflowAppBarSeparator);

                    rootPanel = ref new xaml_controls::Grid();
                    rootPanel->Children->Append(cmdBar);

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(fullSizeAppBarSeparator, "FullSize", false);
                    VisualStateManager::GoToState(compactAppBarSeparator, "Compact", false);
                    VisualStateManager::GoToState(overflowAppBarSeparator, "Overflow", false);

                    // Disable the tooltip on the more button to addressed by instability
                    // caused by it showing up on slower running tests, such as when
                    // these tests are run as CHK bits.
                    auto moreButton = TreeHelper::GetVisualChildByName(cmdBar, L"MoreButton");
                    xaml_controls::ToolTipService::SetToolTip(moreButton, nullptr);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void AppBarSeparatorIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedAppBarSeparatorWidth = 500;
        double const expectedAppBarSeparatorHeight = 16;

        double const expectedAppBarSeparatorCompactWidth = 500;
        double const expectedAppBarSeparatorCompactHeight = 48;

        double const expectedAppBarSeparatorInOverflowWidth = 158;
        double const expectedAppBarSeparatorInOverflowHeight = 9;

        xaml_controls::AppBarSeparator^ appBarSeparator = nullptr;
        xaml_controls::AppBarSeparator^ appBarSeparatorCompact = nullptr;
        xaml_controls::AppBarSeparator^ appBarSeparatorInOverflow = nullptr;

        // Override the window size to make sure the CommandBar doesn't try to use full width
        // on phone.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 600));

        // Set the content first before send Tab() key not to hang for injecting input.
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Inject a tab to ensure a consistent sizing of the overflow menu
        // since it's based on last input mode.
        TestServices::KeyboardHelper->Tab();

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <AppBarSeparator x:Name="appBarSeparator"/>
                        <AppBarSeparator x:Name="appBarSeparatorCompact" IsCompact="True"/>
                        <CommandBar IsOpen="True">
                            <CommandBar.SecondaryCommands>
                                <AppBarSeparator x:Name="appBarSeparatorInOverflow"/>
                            </CommandBar.SecondaryCommands>
                        </CommandBar>
                    </StackPanel>)"));

            appBarSeparator = safe_cast<xaml_controls::AppBarSeparator^>(rootPanel->FindName(L"appBarSeparator"));
            appBarSeparatorCompact = safe_cast<xaml_controls::AppBarSeparator^>(rootPanel->FindName(L"appBarSeparatorCompact"));
            appBarSeparatorInOverflow = safe_cast<xaml_controls::AppBarSeparator^>(rootPanel->FindName(L"appBarSeparatorInOverflow"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedAppBarSeparatorWidth, appBarSeparator->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarSeparatorHeight, appBarSeparator->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarSeparatorCompactWidth, appBarSeparatorCompact->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarSeparatorCompactHeight, appBarSeparatorCompact->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarSeparatorInOverflowWidth, appBarSeparatorInOverflow->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarSeparatorInOverflowHeight, appBarSeparatorInOverflow->ActualHeight);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AppBarSeparator
