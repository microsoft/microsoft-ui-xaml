// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AppBarToggleButtonIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>
#include <generic\ToggleButtonTests.h>

#include <XamlTailored.h>
#include <CommandHelper.h>
#include <ControlHelper.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include "KeyboardInjectionOverride.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarToggleButton {

    bool AppBarToggleButtonIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool AppBarToggleButtonIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AppBarToggleButtonIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void AppBarToggleButtonIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::AppBarToggleButton>::CanInstantiate();
    }

    void AppBarToggleButtonIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::AppBarToggleButton>::CanEnterAndLeaveLiveTree();
    }

    void AppBarToggleButtonIntegrationTests::CanTap()
    {
        Generic::ButtonBaseTests<xaml_controls::AppBarToggleButton>::CanClickUsingTap();
    }

    void AppBarToggleButtonIntegrationTests::CanToggle()
    {
        Generic::ToggleButtonTests<xaml_controls::AppBarToggleButton>::CanToggle();
    }

    void AppBarToggleButtonIntegrationTests::CanSetAndGetLabelProperty()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto button = ref new xaml_controls::AppBarToggleButton();
            Platform::String^ label = "label";

            VERIFY_IS_TRUE(button->Label->IsEmpty());

            button->Label = label;

            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(label, button->Label) == 0);
        });
    }

    void AppBarToggleButtonIntegrationTests::CanSetAndGetIconProperty()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto button = ref new xaml_controls::AppBarToggleButton();
            VERIFY_IS_NULL(button->Icon);

            auto symbolIcon = ref new xaml_controls::SymbolIcon();
            button->Icon = symbolIcon;
            VERIFY_IS_TRUE(button->Icon->Equals(symbolIcon));

            auto fontIcon = ref new xaml_controls::FontIcon();
            button->Icon = fontIcon;
            VERIFY_IS_TRUE(button->Icon->Equals(fontIcon));

            auto pathIcon = ref new xaml_controls::PathIcon();
            button->Icon = pathIcon;
            VERIFY_IS_TRUE(button->Icon->Equals(pathIcon));

            auto bitmapIcon = ref new xaml_controls::BitmapIcon();
            button->Icon = bitmapIcon;
            VERIFY_IS_TRUE(button->Icon->Equals(bitmapIcon));
        });
    }

    void AppBarToggleButtonIntegrationTests::ValidateSettingKeyboardAcceleratorCreatesDefaultKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto button = ref new xaml_controls::AppBarToggleButton();

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            button->KeyboardAccelerators->Append(keyboardAccelerator);

            Platform::String^ expectedKeyboardAcceleratorText = ref new Platform::String(L"Ctrl+A");

            LOG_OUTPUT(L"Expected keyboard accelerator text: \"%s\"", expectedKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Actual keyboard accelerator text:   \"%s\"", button->KeyboardAcceleratorTextOverride->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedKeyboardAcceleratorText, button->KeyboardAcceleratorTextOverride) == 0);
        });
    }

    void AppBarToggleButtonIntegrationTests::ValidateSettingKeyboardAcceleratorDoesNotOverrideCustomKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto button = ref new xaml_controls::AppBarToggleButton();

            Platform::String^ customKeyboardAcceleratorText = ref new Platform::String(L"Custom keyboard accelerator text");
            button->KeyboardAcceleratorTextOverride = customKeyboardAcceleratorText;

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            button->KeyboardAccelerators->Append(keyboardAccelerator);

            LOG_OUTPUT(L"Expected keyboard accelerator text: \"%s\"", customKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Actual keyboard accelerator text:   \"%s\"", button->KeyboardAcceleratorTextOverride->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(customKeyboardAcceleratorText, button->KeyboardAcceleratorTextOverride) == 0);
        });
    }

    void AppBarToggleButtonIntegrationTests::ValidateSettingKeyboardAcceleratorCreatesDefaultToolTip()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::AppBarToggleButton^ button = nullptr;

        RunOnUIThread([&]
        {
            button = ref new xaml_controls::AppBarToggleButton();
            button->Label = ref new Platform::String(L"Select all");

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            button->KeyboardAcceleratorPlacementMode = xaml_input::KeyboardAcceleratorPlacementMode::Hidden;
            button->KeyboardAccelerators->Append(keyboardAccelerator);

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            Platform::String^ expectedToolTip = ref new Platform::String(L"Select all (Ctrl+A)");
            Platform::String^ toolTip = safe_cast<Platform::String^>(xaml_controls::ToolTipService::GetToolTip(button));

            LOG_OUTPUT(L"Expected tool tip text: \"%s\"", expectedToolTip->Data());
            LOG_OUTPUT(L"Actual tool tip text: \"%s\"", toolTip->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedToolTip, toolTip) == 0);
        });
    }

    void AppBarToggleButtonIntegrationTests::ValidateSettingKeyboardAcceleratorDoesNotOverrideCustomToolTip()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::AppBarToggleButton^ button = nullptr;
        Platform::String^ customToolTip = nullptr;

        RunOnUIThread([&]
        {
            button = ref new xaml_controls::AppBarToggleButton();
            button->Label = ref new Platform::String(L"Select all");

            customToolTip = ref new Platform::String(L"Custom tool tip");
            xaml_controls::ToolTipService::SetToolTip(button, customToolTip);

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            button->KeyboardAcceleratorPlacementMode = xaml_input::KeyboardAcceleratorPlacementMode::Hidden;
            button->KeyboardAccelerators->Append(keyboardAccelerator);

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            Platform::String^ toolTip = safe_cast<Platform::String^>(xaml_controls::ToolTipService::GetToolTip(button));

            LOG_OUTPUT(L"Expected tool tip text: \"%s\"", customToolTip->Data());
            LOG_OUTPUT(L"Actual tool tip text: \"%s\"", toolTip->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(customToolTip, toolTip) == 0);
        });
    }

    void AppBarToggleButtonIntegrationTests::ValidateSettingUICommandSetsProperties()
    {
        CommandHelper::ValidateSettingUICommandSetsProperties<xaml_controls::AppBarToggleButton>(
            xaml_controls::AppBarToggleButton::CommandProperty,
            xaml_controls::AppBarToggleButton::LabelProperty,
            xaml_controls::AppBarToggleButton::IconProperty);
    }

    void AppBarToggleButtonIntegrationTests::ValidateSettingUICommandDoesNotOverwriteProperties()
    {
        CommandHelper::ValidateSettingUICommandDoesNotOverwriteProperties<xaml_controls::AppBarToggleButton>(
            xaml_controls::AppBarToggleButton::CommandProperty,
            xaml_controls::AppBarToggleButton::LabelProperty,
            xaml_controls::AppBarToggleButton::IconProperty);
    }

    void AppBarToggleButtonIntegrationTests::ValidateUIElementTree()
    {
        test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        ControlHelper::ValidateUIElementTree(
            wf::Size(600, 800),
            1.f,
            []()
            {
                {
                    // Inject a tab to ensure a consistent sizing of the overflow menu
                    // since it's based on last input mode.
                    KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;
                    TestServices::KeyboardHelper->Tab();
                }
                xaml_controls::StackPanel^ rootPanel = nullptr;
                xaml_controls::CommandBar^ cmdBar = nullptr;
                xaml_controls::CommandBar^ cmdBarWithRightAlignedLabels = nullptr;

                xaml_controls::AppBarToggleButton^ restAppBarToggleButton = nullptr;
                xaml_controls::AppBarToggleButton^ restAppBarToggleButtonWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ pointerOverAppBarToggleButton = nullptr;
                xaml_controls::AppBarToggleButton^ pointerOverAppBarToggleButtonWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ pressedAppBarToggleButton = nullptr;
                xaml_controls::AppBarToggleButton^ pressedAppBarToggleButtonWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ disabledAppBarToggleButton = nullptr;
                xaml_controls::AppBarToggleButton^ disabledAppBarToggleButtonWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ rightLabelAppBarToggleButton = nullptr;
                xaml_controls::AppBarToggleButton^ collapsedLabelAppBarToggleButton = nullptr;
                xaml_controls::AppBarToggleButton^ restAppBarToggleButtonChecked = nullptr;
                xaml_controls::AppBarToggleButton^ restAppBarToggleButtonCheckedWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ pointerOverAppBarToggleButtonChecked = nullptr;
                xaml_controls::AppBarToggleButton^ pointerOverAppBarToggleButtonCheckedWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ pressedAppBarToggleButtonChecked = nullptr;
                xaml_controls::AppBarToggleButton^ pressedAppBarToggleButtonCheckedWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ disabledAppBarToggleButtonChecked = nullptr;
                xaml_controls::AppBarToggleButton^ disabledAppBarToggleButtonCheckedWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ rightLabelAppBarToggleButtonChecked = nullptr;
                xaml_controls::AppBarToggleButton^ collapsedLabelAppBarToggleButtonChecked = nullptr;

                xaml_controls::AppBarToggleButton^ restAppBarToggleButtonOverflow = nullptr;
                xaml_controls::AppBarToggleButton^ restAppBarToggleButtonOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ pointerOverAppBarToggleButtonOverflow = nullptr;
                xaml_controls::AppBarToggleButton^ pointerOverAppBarToggleButtonOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ pressedAppBarToggleButtonOverflow = nullptr;
                xaml_controls::AppBarToggleButton^ pressedAppBarToggleButtonOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ disabledAppBarToggleButtonOverflow = nullptr;
                xaml_controls::AppBarToggleButton^ disabledAppBarToggleButtonOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ restAppBarToggleButtonCheckedOverflow = nullptr;
                xaml_controls::AppBarToggleButton^ restAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ pointerOverAppBarToggleButtonCheckedOverflow = nullptr;
                xaml_controls::AppBarToggleButton^ pointerOverAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ pressedAppBarToggleButtonCheckedOverflow = nullptr;
                xaml_controls::AppBarToggleButton^ pressedAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarToggleButton^ disabledAppBarToggleButtonCheckedOverflow = nullptr;
                xaml_controls::AppBarToggleButton^ disabledAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator = nullptr;

                RunOnUIThread([&]()
                {
                    cmdBar = ref new xaml_controls::CommandBar();
                    cmdBar->VerticalAlignment = xaml::VerticalAlignment::Top;
                    cmdBar->IsOpen = true;
                    cmdBar->IsDynamicOverflowEnabled = false;

                    //Primary Commands
                    restAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
                    restAppBarToggleButton->Label = "Rest AppBarToggleButton";
                    restAppBarToggleButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->PrimaryCommands->Append(restAppBarToggleButton);

                    restAppBarToggleButtonWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    restAppBarToggleButtonWithKeyboardAccelerator->Label = "Rest AppBarToggleButton with keyboard accelerator";
                    restAppBarToggleButtonWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    restAppBarToggleButtonWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::A, ::Windows::System::VirtualKeyModifiers::None));
                    cmdBar->PrimaryCommands->Append(restAppBarToggleButtonWithKeyboardAccelerator);

                    pointerOverAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
                    pointerOverAppBarToggleButton->Label = "PointerOver AppBarToggleButton";
                    pointerOverAppBarToggleButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->PrimaryCommands->Append(pointerOverAppBarToggleButton);

                    pointerOverAppBarToggleButtonWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    pointerOverAppBarToggleButtonWithKeyboardAccelerator->Label = "PointerOver AppBarToggleButton with keyboard accelerator";
                    pointerOverAppBarToggleButtonWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pointerOverAppBarToggleButtonWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::S, ::Windows::System::VirtualKeyModifiers::Control));
                    cmdBar->PrimaryCommands->Append(pointerOverAppBarToggleButtonWithKeyboardAccelerator);

                    pressedAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
                    pressedAppBarToggleButton->Label = "Pressed AppBarToggleButton";
                    pressedAppBarToggleButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->PrimaryCommands->Append(pressedAppBarToggleButton);

                    pressedAppBarToggleButtonWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    pressedAppBarToggleButtonWithKeyboardAccelerator->Label = "Pressed AppBarToggleButton with keyboard accelerator";
                    pressedAppBarToggleButtonWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pressedAppBarToggleButtonWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::D, ::Windows::System::VirtualKeyModifiers::Shift));
                    cmdBar->PrimaryCommands->Append(pressedAppBarToggleButtonWithKeyboardAccelerator);

                    disabledAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
                    disabledAppBarToggleButton->Label = "Disabled AppBarToggleButton";
                    disabledAppBarToggleButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarToggleButton->IsEnabled = false;
                    cmdBar->PrimaryCommands->Append(disabledAppBarToggleButton);

                    disabledAppBarToggleButtonWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    disabledAppBarToggleButtonWithKeyboardAccelerator->Label = "Disabled AppBarToggleButton with keyboard accelerator";
                    disabledAppBarToggleButtonWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarToggleButtonWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::F, ::Windows::System::VirtualKeyModifiers::Menu | ::Windows::System::VirtualKeyModifiers::Windows));
                    cmdBar->PrimaryCommands->Append(disabledAppBarToggleButtonWithKeyboardAccelerator);

                    collapsedLabelAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
                    collapsedLabelAppBarToggleButton->Label = "Collapsed Label AppBarToggleButton";
                    collapsedLabelAppBarToggleButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    collapsedLabelAppBarToggleButton->LabelPosition = xaml_controls::CommandBarLabelPosition::Collapsed;
                    cmdBar->PrimaryCommands->Append(collapsedLabelAppBarToggleButton);

                    restAppBarToggleButtonChecked = ref new xaml_controls::AppBarToggleButton();
                    restAppBarToggleButtonChecked->Label = "Rest Checked AppBarToggleButton";
                    restAppBarToggleButtonChecked->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    restAppBarToggleButtonChecked->IsChecked = true;
                    cmdBar->PrimaryCommands->Append(restAppBarToggleButtonChecked);

                    restAppBarToggleButtonCheckedWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    restAppBarToggleButtonCheckedWithKeyboardAccelerator->Label = "Rest Checked AppBarToggleButton with keyboard accelerator";
                    restAppBarToggleButtonCheckedWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    restAppBarToggleButtonCheckedWithKeyboardAccelerator->IsChecked = true;
                    restAppBarToggleButtonCheckedWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::A, ::Windows::System::VirtualKeyModifiers::None));
                    cmdBar->PrimaryCommands->Append(restAppBarToggleButtonCheckedWithKeyboardAccelerator);

                    pointerOverAppBarToggleButtonChecked = ref new xaml_controls::AppBarToggleButton();
                    pointerOverAppBarToggleButtonChecked->Label = "PointerOver Checked AppBarToggleButton";
                    pointerOverAppBarToggleButtonChecked->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pointerOverAppBarToggleButtonChecked->IsChecked = true;
                    cmdBar->PrimaryCommands->Append(pointerOverAppBarToggleButtonChecked);

                    pointerOverAppBarToggleButtonCheckedWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    pointerOverAppBarToggleButtonCheckedWithKeyboardAccelerator->Label = "PointerOver Checked AppBarToggleButton with keyboard accelerator";
                    pointerOverAppBarToggleButtonCheckedWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pointerOverAppBarToggleButtonCheckedWithKeyboardAccelerator->IsChecked = true;
                    pointerOverAppBarToggleButtonCheckedWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::S, ::Windows::System::VirtualKeyModifiers::Control));
                    cmdBar->PrimaryCommands->Append(pointerOverAppBarToggleButtonCheckedWithKeyboardAccelerator);

                    pressedAppBarToggleButtonChecked = ref new xaml_controls::AppBarToggleButton();
                    pressedAppBarToggleButtonChecked->Label = "Pressed Checked AppBarToggleButton";
                    pressedAppBarToggleButtonChecked->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pressedAppBarToggleButtonChecked->IsChecked = true;
                    cmdBar->PrimaryCommands->Append(pressedAppBarToggleButtonChecked);

                    pressedAppBarToggleButtonCheckedWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    pressedAppBarToggleButtonCheckedWithKeyboardAccelerator->Label = "Pressed Checked AppBarToggleButton with keyboard accelerator";
                    pressedAppBarToggleButtonCheckedWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pressedAppBarToggleButtonCheckedWithKeyboardAccelerator->IsChecked = true;
                    pressedAppBarToggleButtonCheckedWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::D, ::Windows::System::VirtualKeyModifiers::Shift));
                    cmdBar->PrimaryCommands->Append(pressedAppBarToggleButtonCheckedWithKeyboardAccelerator);

                    disabledAppBarToggleButtonChecked = ref new xaml_controls::AppBarToggleButton();
                    disabledAppBarToggleButtonChecked->Label = "Disabled Checked AppBarToggleButton";
                    disabledAppBarToggleButtonChecked->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarToggleButtonChecked->IsChecked = true;
                    disabledAppBarToggleButtonChecked->IsEnabled = false;
                    cmdBar->PrimaryCommands->Append(disabledAppBarToggleButtonChecked);

                    disabledAppBarToggleButtonCheckedWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    disabledAppBarToggleButtonCheckedWithKeyboardAccelerator->Label = "Disabled Checked AppBarToggleButton with keyboard accelerator";
                    disabledAppBarToggleButtonCheckedWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarToggleButtonCheckedWithKeyboardAccelerator->IsChecked = true;
                    disabledAppBarToggleButtonCheckedWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::F, ::Windows::System::VirtualKeyModifiers::Menu | ::Windows::System::VirtualKeyModifiers::Windows));
                    cmdBar->PrimaryCommands->Append(disabledAppBarToggleButtonCheckedWithKeyboardAccelerator);

                    collapsedLabelAppBarToggleButtonChecked = ref new xaml_controls::AppBarToggleButton();
                    collapsedLabelAppBarToggleButtonChecked->Label = "Collapsed Label AppBarToggleButton";
                    collapsedLabelAppBarToggleButtonChecked->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    collapsedLabelAppBarToggleButtonChecked->LabelPosition = xaml_controls::CommandBarLabelPosition::Collapsed;
                    collapsedLabelAppBarToggleButtonChecked->IsChecked = true;
                    cmdBar->PrimaryCommands->Append(collapsedLabelAppBarToggleButtonChecked);

                    //Secondary Commands
                    restAppBarToggleButtonOverflow = ref new xaml_controls::AppBarToggleButton();
                    restAppBarToggleButtonOverflow->Label = "Rest Overflow AppBarToggleButton";
                    restAppBarToggleButtonOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->SecondaryCommands->Append(restAppBarToggleButtonOverflow);

                    restAppBarToggleButtonOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    restAppBarToggleButtonOverflowWithKeyboardAccelerator->Label = "Rest Overflow AppBarToggleButton with keyboard accelerator";
                    restAppBarToggleButtonOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    restAppBarToggleButtonOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::A, ::Windows::System::VirtualKeyModifiers::None));
                    cmdBar->SecondaryCommands->Append(restAppBarToggleButtonOverflowWithKeyboardAccelerator);

                    pointerOverAppBarToggleButtonOverflow = ref new xaml_controls::AppBarToggleButton();
                    pointerOverAppBarToggleButtonOverflow->Label = "PointerOver Overflow AppBarToggleButton";
                    pointerOverAppBarToggleButtonOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->SecondaryCommands->Append(pointerOverAppBarToggleButtonOverflow);

                    pointerOverAppBarToggleButtonOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    pointerOverAppBarToggleButtonOverflowWithKeyboardAccelerator->Label = "PointerOver Overflow AppBarToggleButton with keyboard accelerator";
                    pointerOverAppBarToggleButtonOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pointerOverAppBarToggleButtonOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::S, ::Windows::System::VirtualKeyModifiers::Control));
                    cmdBar->SecondaryCommands->Append(pointerOverAppBarToggleButtonOverflowWithKeyboardAccelerator);

                    pressedAppBarToggleButtonOverflow = ref new xaml_controls::AppBarToggleButton();
                    pressedAppBarToggleButtonOverflow->Label = "Pressed Overflow AppBarToggleButton";
                    pressedAppBarToggleButtonOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->SecondaryCommands->Append(pressedAppBarToggleButtonOverflow);

                    pressedAppBarToggleButtonOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    pressedAppBarToggleButtonOverflowWithKeyboardAccelerator->Label = "Pressed Overflow AppBarToggleButton with keyboard accelerator";
                    pressedAppBarToggleButtonOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pressedAppBarToggleButtonOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::D, ::Windows::System::VirtualKeyModifiers::Shift));
                    cmdBar->SecondaryCommands->Append(pressedAppBarToggleButtonOverflowWithKeyboardAccelerator);

                    disabledAppBarToggleButtonOverflow = ref new xaml_controls::AppBarToggleButton();
                    disabledAppBarToggleButtonOverflow->Label = "Disabled Overflow AppBarToggleButton";
                    disabledAppBarToggleButtonOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarToggleButtonOverflow->IsEnabled = false;
                    cmdBar->SecondaryCommands->Append(disabledAppBarToggleButtonOverflow);

                    disabledAppBarToggleButtonOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    disabledAppBarToggleButtonOverflowWithKeyboardAccelerator->Label = "Disabled Overflow AppBarToggleButton with keyboard accelerator";
                    disabledAppBarToggleButtonOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarToggleButtonOverflowWithKeyboardAccelerator->IsEnabled = false;
                    disabledAppBarToggleButtonOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::F, ::Windows::System::VirtualKeyModifiers::Menu | ::Windows::System::VirtualKeyModifiers::Windows));
                    cmdBar->SecondaryCommands->Append(disabledAppBarToggleButtonOverflowWithKeyboardAccelerator);

                    restAppBarToggleButtonCheckedOverflow = ref new xaml_controls::AppBarToggleButton();
                    restAppBarToggleButtonCheckedOverflow->Label = "Rest Overflow Checked AppBarToggleButton";
                    restAppBarToggleButtonCheckedOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    restAppBarToggleButtonCheckedOverflow->IsChecked = true;
                    cmdBar->SecondaryCommands->Append(restAppBarToggleButtonCheckedOverflow);

                    restAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    restAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->Label = "Rest Overflow Checked AppBarToggleButton with keyboard accelerator";
                    restAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    restAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->IsChecked = true;
                    restAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::A, ::Windows::System::VirtualKeyModifiers::None));
                    cmdBar->SecondaryCommands->Append(restAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator);

                    pointerOverAppBarToggleButtonCheckedOverflow = ref new xaml_controls::AppBarToggleButton();
                    pointerOverAppBarToggleButtonCheckedOverflow->Label = "PointerOver Overflow Checked AppBarToggleButton";
                    pointerOverAppBarToggleButtonCheckedOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pointerOverAppBarToggleButtonCheckedOverflow->IsChecked = true;
                    cmdBar->SecondaryCommands->Append(pointerOverAppBarToggleButtonCheckedOverflow);

                    pointerOverAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    pointerOverAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->Label = "PointerOver Overflow Checked AppBarToggleButton with keyboard accelerator";
                    pointerOverAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pointerOverAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->IsChecked = true;
                    pointerOverAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::S, ::Windows::System::VirtualKeyModifiers::Control));
                    cmdBar->SecondaryCommands->Append(pointerOverAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator);

                    pressedAppBarToggleButtonCheckedOverflow = ref new xaml_controls::AppBarToggleButton();
                    pressedAppBarToggleButtonCheckedOverflow->Label = "Pressed Overflow Checked AppBarToggleButton";
                    pressedAppBarToggleButtonCheckedOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pressedAppBarToggleButtonCheckedOverflow->IsChecked = true;
                    cmdBar->SecondaryCommands->Append(pressedAppBarToggleButtonCheckedOverflow);

                    pressedAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    pressedAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->Label = "Pressed Overflow Checked AppBarToggleButton with keyboard accelerator";
                    pressedAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pressedAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->IsChecked = true;
                    pressedAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::D, ::Windows::System::VirtualKeyModifiers::Shift));
                    cmdBar->SecondaryCommands->Append(pressedAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator);

                    disabledAppBarToggleButtonCheckedOverflow = ref new xaml_controls::AppBarToggleButton();
                    disabledAppBarToggleButtonCheckedOverflow->Label = "Disabled Overflow Checked AppBarToggleButton";
                    disabledAppBarToggleButtonCheckedOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarToggleButtonCheckedOverflow->IsChecked = true;
                    disabledAppBarToggleButtonCheckedOverflow->IsEnabled = false;
                    cmdBar->SecondaryCommands->Append(disabledAppBarToggleButtonCheckedOverflow);

                    disabledAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarToggleButton();
                    disabledAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->Label = "Disabled Overflow Checked AppBarToggleButton with keyboard accelerator";
                    disabledAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->IsChecked = true;
                    disabledAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->IsEnabled = false;
                    disabledAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::F, ::Windows::System::VirtualKeyModifiers::Menu | ::Windows::System::VirtualKeyModifiers::Windows));
                    cmdBar->SecondaryCommands->Append(disabledAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator);

                    cmdBarWithRightAlignedLabels = ref new xaml_controls::CommandBar();
                    cmdBarWithRightAlignedLabels->VerticalAlignment = xaml::VerticalAlignment::Top;
                    cmdBarWithRightAlignedLabels->IsOpen = true;
                    cmdBarWithRightAlignedLabels->IsDynamicOverflowEnabled = false;
                    cmdBarWithRightAlignedLabels->DefaultLabelPosition = xaml_controls::CommandBarDefaultLabelPosition::Right;

                    rightLabelAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
                    rightLabelAppBarToggleButton->Label = "Right Label AppBarToggleButton";
                    rightLabelAppBarToggleButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBarWithRightAlignedLabels->PrimaryCommands->Append(rightLabelAppBarToggleButton);

                    rightLabelAppBarToggleButtonChecked = ref new xaml_controls::AppBarToggleButton();
                    rightLabelAppBarToggleButtonChecked->Label = "Right Label AppBarToggleButton";
                    rightLabelAppBarToggleButtonChecked->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    rightLabelAppBarToggleButtonChecked->IsChecked = true;
                    cmdBarWithRightAlignedLabels->PrimaryCommands->Append(rightLabelAppBarToggleButtonChecked);

                    rootPanel = ref new xaml_controls::StackPanel();
                    rootPanel->Children->Append(cmdBarWithRightAlignedLabels);
                    rootPanel->Children->Append(cmdBar);

                    TestServices::WindowHelper->WindowContent = rootPanel;

                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // AppBarToggleButton states
                    VisualStateManager::GoToState(pointerOverAppBarToggleButton, "PointerOver", false);
                    VisualStateManager::GoToState(pointerOverAppBarToggleButtonWithKeyboardAccelerator, "PointerOver", false);
                    VisualStateManager::GoToState(pressedAppBarToggleButton, "Pressed", false);
                    VisualStateManager::GoToState(pressedAppBarToggleButtonWithKeyboardAccelerator, "Pressed", false);
                    VisualStateManager::GoToState(pointerOverAppBarToggleButtonChecked, "CheckedPointerOver", false);
                    VisualStateManager::GoToState(pointerOverAppBarToggleButtonCheckedWithKeyboardAccelerator, "CheckedPointerOver", false);
                    VisualStateManager::GoToState(pressedAppBarToggleButtonChecked, "CheckedPressed", false);
                    VisualStateManager::GoToState(pressedAppBarToggleButtonCheckedWithKeyboardAccelerator, "CheckedPressed", false);
                    VisualStateManager::GoToState(pointerOverAppBarToggleButtonOverflow, "PointerOver", false);
                    VisualStateManager::GoToState(pointerOverAppBarToggleButtonOverflowWithKeyboardAccelerator, "PointerOver", false);
                    VisualStateManager::GoToState(pressedAppBarToggleButtonOverflow, "Pressed", false);
                    VisualStateManager::GoToState(pressedAppBarToggleButtonOverflowWithKeyboardAccelerator, "Pressed", false);
                    VisualStateManager::GoToState(pointerOverAppBarToggleButtonCheckedOverflow, "CheckedPointerOver", false);
                    VisualStateManager::GoToState(pointerOverAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator, "CheckedPointerOver", false);
                    VisualStateManager::GoToState(pressedAppBarToggleButtonCheckedOverflow, "CheckedPressed", false);
                    VisualStateManager::GoToState(pressedAppBarToggleButtonCheckedOverflowWithKeyboardAccelerator, "CheckedPressed", false);

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

    void AppBarToggleButtonIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedAppBarToggleButtonWidth = 68;
        double const expectedAppBarToggleButtonHeight = 64;

        double const expectedAppBarToggleButtonWithMultiLineLabelWidth = 68;
        double const expectedAppBarToggleButtonWithMultiLineLabelHeight = 74;

        double const expectedAppBarToggleButtonCompactWidth = 68;
        double const expectedAppBarToggleButtonCompactHeight = 64;

        double const expectedAppBarToggleButtonRightLabelPositionWidth = 84;
        double const expectedAppBarToggleButtonRightLabelPositionHeight = 48;

        double const expectedAppBarToggleButtonCollapsedLabelPositionWidth = 68;
        double const expectedAppBarToggleButtonCollapsedLabelPositionHeight = 48;

        double const expectedAppBarToggleButtonInOverflowWidth = 158;
        double const expectedAppBarToggleButtonInOverflowHeight = 32;

        xaml_controls::AppBarToggleButton^ appBarToggleButton = nullptr;
        xaml_controls::AppBarToggleButton^ appBarToggleButtonWithMultiLineLabel = nullptr;
        xaml_controls::AppBarToggleButton^ appBarToggleButtonCompact = nullptr;
        xaml_controls::AppBarToggleButton^ appBarToggleButtonRightLabelPosition = nullptr;
        xaml_controls::AppBarToggleButton^ appBarToggleButtonCollapsedLabelPosition = nullptr;
        xaml_controls::AppBarToggleButton^ appBarToggleButtonInOverflow = nullptr;

        // Override the window size to make sure the CommandBar doesn't try to use full width
        // on phone.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 600));

        // Inject a tab to ensure a consistent sizing of the overflow menu
        // since it's based on last input mode.
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
        });
        TestServices::WindowHelper->WaitForIdle();

        {
            // Inject a tab to ensure a consistent sizing of the overflow menu
            // since it's based on last input mode.
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;
            TestServices::KeyboardHelper->Tab();
        }
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <AppBarToggleButton x:Name="appBarToggleButton" Label="button" Icon="Accept"/>
                        <AppBarToggleButton x:Name="appBarToggleButtonWithMultiLineLabel" Label="button button line" Icon="Accept"/>
                        <AppBarToggleButton x:Name="appBarToggleButtonCompact" Label="button" Icon="Accept" IsCompact="True"/>
                        <AppBarToggleButton x:Name="appBarToggleButtonCollapsedLabelPosition" Label="button" Icon="Accept" LabelPosition="Collapsed"/>
                        <CommandBar DefaultLabelPosition="Right" IsOpen="True">
                            <AppBarToggleButton x:Name="appBarToggleButtonRightLabelPosition" Label="button" Icon="Accept"/>
                            <CommandBar.SecondaryCommands>
                                <AppBarToggleButton x:Name="appBarToggleButtonInOverflow" Label="button" Icon="Accept"/>
                            </CommandBar.SecondaryCommands>
                        </CommandBar>
                    </StackPanel>)"));

            appBarToggleButton = safe_cast<xaml_controls::AppBarToggleButton^>(rootPanel->FindName(L"appBarToggleButton"));
            appBarToggleButtonWithMultiLineLabel = safe_cast<xaml_controls::AppBarToggleButton^>(rootPanel->FindName(L"appBarToggleButtonWithMultiLineLabel"));
            appBarToggleButtonCompact = safe_cast<xaml_controls::AppBarToggleButton^>(rootPanel->FindName(L"appBarToggleButtonCompact"));
            appBarToggleButtonRightLabelPosition = safe_cast<xaml_controls::AppBarToggleButton^>(rootPanel->FindName(L"appBarToggleButtonRightLabelPosition"));
            appBarToggleButtonCollapsedLabelPosition = safe_cast<xaml_controls::AppBarToggleButton^>(rootPanel->FindName(L"appBarToggleButtonCollapsedLabelPosition"));
            appBarToggleButtonInOverflow = safe_cast<xaml_controls::AppBarToggleButton^>(rootPanel->FindName(L"appBarToggleButtonInOverflow"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonWidth, appBarToggleButton->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonHeight, appBarToggleButton->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonWithMultiLineLabelWidth, appBarToggleButtonWithMultiLineLabel->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonWithMultiLineLabelHeight, appBarToggleButtonWithMultiLineLabel->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonCompactWidth, appBarToggleButtonCompact->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonCompactHeight, appBarToggleButtonCompact->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonRightLabelPositionWidth, appBarToggleButtonRightLabelPosition->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonRightLabelPositionHeight, appBarToggleButtonRightLabelPosition->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonCollapsedLabelPositionWidth, appBarToggleButtonCollapsedLabelPosition->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonCollapsedLabelPositionHeight, appBarToggleButtonCollapsedLabelPosition->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonInOverflowWidth, appBarToggleButtonInOverflow->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarToggleButtonInOverflowHeight, appBarToggleButtonInOverflow->ActualHeight);
        });
    }

    xaml_input::KeyboardAccelerator^ AppBarToggleButtonIntegrationTests::CreateKeyboardAccelerator(::Windows::System::VirtualKey key, ::Windows::System::VirtualKeyModifiers modifiers)
    {
        auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
        keyboardAccelerator->Key = key;
        keyboardAccelerator->Modifiers = modifiers;
        return keyboardAccelerator;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AppBarToggleButton
