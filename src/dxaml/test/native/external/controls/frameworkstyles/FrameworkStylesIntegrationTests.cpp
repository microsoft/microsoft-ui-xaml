// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "FrameworkStylesIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>

#include <ControlHelper.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace FrameworkStyles {

    bool FrameworkStylesIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool FrameworkStylesIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool FrameworkStylesIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void FrameworkStylesIntegrationTests::ValidateNavBackBtnUIETree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 400),
            1.f,
            []()
            {
                xaml_controls::StackPanel^ rootPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"navigationbackbutton.xaml"));

                RunOnUIThread([&]()
                {
                    rootPanel->IsHitTestVisible = false;
                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, "BackButtonN2")), "PointerOver", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, "BackButtonN3")), "Pressed", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, "BackButtonN4")), "Disabled", true);

                    VisualStateManager::GoToState(safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, "BackButtonS2")), "PointerOver", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, "BackButtonS3")), "Pressed", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, "BackButtonS4")), "Disabled", true);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void FrameworkStylesIntegrationTests::ValidateTxtBlkBtnUIETree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                xaml_controls::StackPanel^ rootPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"textblockbutton.xaml"));

                RunOnUIThread([&]()
                {
                    rootPanel->IsHitTestVisible = false;
                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, "Button2")), "PointerOver", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, "Button3")), "Pressed", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, "Button4")), "Disabled", true);

                    VisualStateManager::GoToState(safe_cast<xaml_controls::RadioButton^>(TreeHelper::GetVisualChildByName(rootPanel, "RadioButton2")), "PointerOver", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::RadioButton^>(TreeHelper::GetVisualChildByName(rootPanel, "RadioButton3")), "Pressed", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::RadioButton^>(TreeHelper::GetVisualChildByName(rootPanel, "RadioButton4")), "Disabled", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::RadioButton^>(TreeHelper::GetVisualChildByName(rootPanel, "RadioButton6")), "PointerOver", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::RadioButton^>(TreeHelper::GetVisualChildByName(rootPanel, "RadioButton7")), "Pressed", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::RadioButton^>(TreeHelper::GetVisualChildByName(rootPanel, "RadioButton8")), "Disabled", true);

                    VisualStateManager::GoToState(safe_cast<xaml_primitives::ToggleButton^>(TreeHelper::GetVisualChildByName(rootPanel, "ToggleButton2")), "PointerOver", true);
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::ToggleButton^>(TreeHelper::GetVisualChildByName(rootPanel, "ToggleButton3")), "Pressed", true);
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::ToggleButton^>(TreeHelper::GetVisualChildByName(rootPanel, "ToggleButton4")), "Disabled", true);
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::ToggleButton^>(TreeHelper::GetVisualChildByName(rootPanel, "ToggleButton6")), "PointerOver", true);
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::ToggleButton^>(TreeHelper::GetVisualChildByName(rootPanel, "ToggleButton7")), "Pressed", true);
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::ToggleButton^>(TreeHelper::GetVisualChildByName(rootPanel, "ToggleButton8")), "Disabled", true);

                    VisualStateManager::GoToState(safe_cast<xaml_controls::CheckBox^>(TreeHelper::GetVisualChildByName(rootPanel, "CheckBox2")), "PointerOver", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::CheckBox^>(TreeHelper::GetVisualChildByName(rootPanel, "CheckBox3")), "Pressed", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::CheckBox^>(TreeHelper::GetVisualChildByName(rootPanel, "CheckBox4")), "Disabled", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::CheckBox^>(TreeHelper::GetVisualChildByName(rootPanel, "CheckBox6")), "PointerOver", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::CheckBox^>(TreeHelper::GetVisualChildByName(rootPanel, "CheckBox7")), "Pressed", true);
                    VisualStateManager::GoToState(safe_cast<xaml_controls::CheckBox^>(TreeHelper::GetVisualChildByName(rootPanel, "CheckBox8")), "Disabled", true);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void FrameworkStylesIntegrationTests::ValidateUseSystemFocusVisualsDefaults()
    {
        Microsoft::UI::Xaml::FocusVisualKind oldFocusVisualKind = Microsoft::UI::Xaml::FocusVisualKind::HighVisibility;

        TestCleanupWrapper cleanup([&]()
        {
            RunOnUIThread([&]()
            {
                Microsoft::UI::Xaml::Application::Current->FocusVisualKind = oldFocusVisualKind;
            });
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L" <StackPanel>"
                L"  <DatePicker x:Name='datePicker'/>"
                L"  <TimePicker x:Name='timePicker'/>"
                L"  <CalendarDatePicker x:Name='calendarDatePicker'/>"
                L"  <ComboBox x:Name='comboBox'/>"
                L"  <TextBox x:Name='textBox'/>"
                L"  <RichEditBox x:Name='richEditBox'/>"
                L"  <AutoSuggestBox x:Name='autoSuggestBox'/>"
                L" </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            oldFocusVisualKind = Microsoft::UI::Xaml::Application::Current->FocusVisualKind;

            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "datePicker", "FlyoutButton");
            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "timePicker", "FlyoutButton");
            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "calendarDatePicker");
            VerifyUseSystemFocusVisualsHelper(rootPanel, false, "comboBox");
            VerifyUseSystemFocusVisualsHelper(rootPanel, false, "textBox");
            VerifyUseSystemFocusVisualsHelper(rootPanel, false, "richEditBox");
            VerifyUseSystemFocusVisualsHelper(rootPanel, false, "autoSuggestBox", "TextBox");

            // Change FocusVisualKind and tickle a theme change to make the controls pick up the change.
            Microsoft::UI::Xaml::Application::Current->FocusVisualKind = FocusVisualKind::Reveal;
            rootPanel->RequestedTheme = Microsoft::UI::Xaml::ElementTheme::Dark;
            rootPanel->RequestedTheme = Microsoft::UI::Xaml::ElementTheme::Default;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "datePicker", "FlyoutButton");
            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "timePicker", "FlyoutButton");
            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "calendarDatePicker");
            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "comboBox");
            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "textBox");
            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "richEditBox");
            VerifyUseSystemFocusVisualsHelper(rootPanel, true, "autoSuggestBox", "TextBox");
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    //
    // Utilities
    //
    Platform::String^ FrameworkStylesIntegrationTests::GetResourcesPath()
    {
        return GetPackageFolder() + L"resources\\native\\controls\\frameworkstyles\\";
    }

    void FrameworkStylesIntegrationTests::VerifyUseSystemFocusVisualsHelper(
        xaml_controls::Grid^ root, bool expectedUseSystemFocusVisuals, Platform::String^ elementName, Platform::String^ innerChildToo)
    {
        LOG_OUTPUT(L"Checking UseSystemFocusVisuals = %d on %s", expectedUseSystemFocusVisuals, elementName->Data());
        auto control = safe_cast<xaml_controls::Control^>(root->FindName(elementName));
        VERIFY_IS_NOT_NULL(control);

        VERIFY_IS_TRUE(control->UseSystemFocusVisuals == expectedUseSystemFocusVisuals);

        if (innerChildToo)
        {
            LOG_OUTPUT(L"Also verifying on child %s", innerChildToo->Data());
            auto child = safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(control, innerChildToo));
            VERIFY_IS_NOT_NULL(child);

            VERIFY_IS_TRUE(child->UseSystemFocusVisuals == expectedUseSystemFocusVisuals);
        }
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::FrameworkStyles
