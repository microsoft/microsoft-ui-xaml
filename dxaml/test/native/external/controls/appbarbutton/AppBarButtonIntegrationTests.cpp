// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AppBarButtonIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>

#include <XamlTailored.h>
#include <CommandHelper.h>
#include <ControlHelper.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include "FocusTestHelper.h"

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarButton {

    bool AppBarButtonIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool AppBarButtonIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AppBarButtonIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void AppBarButtonIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::AppBarButton>::CanInstantiate();
    }

    void AppBarButtonIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::AppBarButton>::CanEnterAndLeaveLiveTree();
    }

    void AppBarButtonIntegrationTests::CanTap()
    {
        Generic::ButtonBaseTests<xaml_controls::AppBarButton>::CanClickUsingTap();
    }

    void AppBarButtonIntegrationTests::CanSetAndGetLabelProperty()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto button = ref new xaml_controls::AppBarButton();
            Platform::String^ label = "label";

            VERIFY_IS_TRUE(button->Label->IsEmpty());

            button->Label = label;

            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(label, button->Label) == 0);
        });
    }

    void AppBarButtonIntegrationTests::CanSetAndGetIconProperty()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto button = ref new xaml_controls::AppBarButton();
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

    void AppBarButtonIntegrationTests::CanUseThemeAnimations()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::AppBarButton^ button = nullptr;
        xaml_animation::Storyboard^ sb = nullptr;
        xaml_animation::FadeInThemeAnimation^ animation = nullptr;

        RunOnUIThread([&]
        {
            button = ref new xaml_controls::AppBarButton();

            sb = ref new xaml_animation::Storyboard();

            TimeSpan ts;
            ts.Duration = 0;
            sb->Duration = DurationHelper::FromTimeSpan(ts);

            animation = ref new xaml_animation::FadeInThemeAnimation();
            xaml_animation::Storyboard::SetTarget(animation, button);

            sb->Children->Append(animation);

            TestServices::WindowHelper->WindowContent = button;

            sb->Begin();
        });

        TestServices::WindowHelper->WaitForIdle();

    }

    void AppBarButtonIntegrationTests::ValidateSettingKeyboardAcceleratorCreatesDefaultKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto button = ref new xaml_controls::AppBarButton();

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

    void AppBarButtonIntegrationTests::ValidateSettingKeyboardAcceleratorDoesNotOverrideCustomKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto button = ref new xaml_controls::AppBarButton();

            Platform::String^ customKeyboardAcceleratorText = ref new Platform::String(L"Custom keyboard accelerator text");
            button->KeyboardAcceleratorTextOverride = customKeyboardAcceleratorText;

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            button->KeyboardAccelerators->Append(keyboardAccelerator);

            LOG_OUTPUT(L"Expected keyboard accelerator text: \"%s\"", customKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Actual keyboard accelerator text: \"%s\"", button->KeyboardAcceleratorTextOverride->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(customKeyboardAcceleratorText, button->KeyboardAcceleratorTextOverride) == 0);
        });
    }

    void AppBarButtonIntegrationTests::ValidateSettingKeyboardAcceleratorCreatesDefaultToolTip()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::AppBarButton^ button = nullptr;

        RunOnUIThread([&]
        {
            button = ref new xaml_controls::AppBarButton();
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

    void AppBarButtonIntegrationTests::ValidateSettingKeyboardAcceleratorDoesNotOverrideCustomToolTip()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::AppBarButton^ button = nullptr;
        Platform::String^ customToolTip = nullptr;

        RunOnUIThread([&]
        {
            button = ref new xaml_controls::AppBarButton();
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

    void AppBarButtonIntegrationTests::ValidateSettingUICommandSetsProperties()
    {
        CommandHelper::ValidateSettingUICommandSetsProperties<xaml_controls::AppBarButton>(
            xaml_controls::AppBarButton::CommandProperty,
            xaml_controls::AppBarButton::LabelProperty,
            xaml_controls::AppBarButton::IconProperty);
    }

    void AppBarButtonIntegrationTests::ValidateSettingUICommandDoesNotOverwriteProperties()
    {
        CommandHelper::ValidateSettingUICommandDoesNotOverwriteProperties<xaml_controls::AppBarButton>(
            xaml_controls::AppBarButton::CommandProperty,
            xaml_controls::AppBarButton::LabelProperty,
            xaml_controls::AppBarButton::IconProperty);
    }

    void AppBarButtonIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                // Inject a tab to ensure a consistent sizing of the overflow menu
                // since it's based on last input mode.
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
                });
                TestServices::KeyboardHelper->Tab();

                xaml_controls::StackPanel^ rootPanel = nullptr;
                xaml_controls::CommandBar^ cmdBar = nullptr;
                xaml_controls::CommandBar^ cmdBarWithRightAlignedLabels = nullptr;

                xaml_controls::AppBarButton^ restAppBarButton = nullptr;
                xaml_controls::AppBarButton^ restAppBarButtonWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarButton^ pointerOverAppBarButton = nullptr;
                xaml_controls::AppBarButton^ pointerOverAppBarButtonWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarButton^ pressedAppBarButton = nullptr;
                xaml_controls::AppBarButton^ pressedAppBarButtonWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarButton^ disabledAppBarButton = nullptr;
                xaml_controls::AppBarButton^ disabledAppBarButtonWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarButton^ rightLabelAppBarButton = nullptr;
                xaml_controls::AppBarButton^ collapsedLabelAppBarButton = nullptr;
                xaml_controls::AppBarButton^ restAppBarButtonOverflow = nullptr;
                xaml_controls::AppBarButton^ restAppBarButtonOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarButton^ pointerOverAppBarButtonOverflow = nullptr;
                xaml_controls::AppBarButton^ pointerOverAppBarButtonOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarButton^ pressedAppBarButtonOverflow = nullptr;
                xaml_controls::AppBarButton^ pressedAppBarButtonOverflowWithKeyboardAccelerator = nullptr;
                xaml_controls::AppBarButton^ disabledAppBarButtonOverflow = nullptr;
                xaml_controls::AppBarButton^ disabledAppBarButtonOverflowWithKeyboardAccelerator = nullptr;

                RunOnUIThread([&]()
                {
                    cmdBar = ref new xaml_controls::CommandBar();
                    cmdBar->VerticalAlignment = xaml::VerticalAlignment::Top;
                    cmdBar->IsOpen = true;
                    cmdBar->IsDynamicOverflowEnabled = false;

                    //Primary Commands
                    restAppBarButton = ref new xaml_controls::AppBarButton();
                    restAppBarButton->Label = "Rest AppBarButton";
                    restAppBarButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->PrimaryCommands->Append(restAppBarButton);

                    restAppBarButtonWithKeyboardAccelerator = ref new xaml_controls::AppBarButton();
                    restAppBarButtonWithKeyboardAccelerator->Label = "Rest AppBarButton with keyboard accelerator";
                    restAppBarButtonWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    restAppBarButtonWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::A, ::Windows::System::VirtualKeyModifiers::None));
                    cmdBar->PrimaryCommands->Append(restAppBarButtonWithKeyboardAccelerator);

                    pointerOverAppBarButton = ref new xaml_controls::AppBarButton();
                    pointerOverAppBarButton->Label = "PointerOver AppBarButton";
                    pointerOverAppBarButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->PrimaryCommands->Append(pointerOverAppBarButton);

                    pointerOverAppBarButtonWithKeyboardAccelerator = ref new xaml_controls::AppBarButton();
                    pointerOverAppBarButtonWithKeyboardAccelerator->Label = "PointerOver AppBarButton with keyboard accelerator";
                    pointerOverAppBarButtonWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pointerOverAppBarButtonWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::S, ::Windows::System::VirtualKeyModifiers::Control));
                    cmdBar->PrimaryCommands->Append(pointerOverAppBarButtonWithKeyboardAccelerator);

                    pressedAppBarButton = ref new xaml_controls::AppBarButton();
                    pressedAppBarButton->Label = "Pressed AppBarButton";
                    pressedAppBarButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->PrimaryCommands->Append(pressedAppBarButton);

                    pressedAppBarButtonWithKeyboardAccelerator = ref new xaml_controls::AppBarButton();
                    pressedAppBarButtonWithKeyboardAccelerator->Label = "Pressed AppBarButton with keyboard accelerator";
                    pressedAppBarButtonWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pressedAppBarButtonWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::D, ::Windows::System::VirtualKeyModifiers::Shift));
                    cmdBar->PrimaryCommands->Append(pressedAppBarButtonWithKeyboardAccelerator);

                    disabledAppBarButton = ref new xaml_controls::AppBarButton();
                    disabledAppBarButton->Label = "Disabled AppBarButton";
                    disabledAppBarButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarButton->IsEnabled = false;
                    cmdBar->PrimaryCommands->Append(disabledAppBarButton);

                    disabledAppBarButtonWithKeyboardAccelerator = ref new xaml_controls::AppBarButton();
                    disabledAppBarButtonWithKeyboardAccelerator->Label = "Disabled AppBarButton with keyboard accelerator";
                    disabledAppBarButtonWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarButton->IsEnabled = false;
                    disabledAppBarButtonWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::F, ::Windows::System::VirtualKeyModifiers::Menu | ::Windows::System::VirtualKeyModifiers::Windows));
                    cmdBar->PrimaryCommands->Append(disabledAppBarButtonWithKeyboardAccelerator);

                    collapsedLabelAppBarButton = ref new xaml_controls::AppBarButton();
                    collapsedLabelAppBarButton->Label = "Collapsed Label AppBarButton";
                    collapsedLabelAppBarButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    collapsedLabelAppBarButton->LabelPosition = xaml_controls::CommandBarLabelPosition::Collapsed;
                    cmdBar->PrimaryCommands->Append(collapsedLabelAppBarButton);

                    //Secondary Commands
                    restAppBarButtonOverflow = ref new xaml_controls::AppBarButton();
                    restAppBarButtonOverflow->Label = "Rest Overflow AppBarButton";
                    restAppBarButtonOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->SecondaryCommands->Append(restAppBarButtonOverflow);

                    restAppBarButtonOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarButton();
                    restAppBarButtonOverflowWithKeyboardAccelerator->Label = "Rest Overflow AppBarButton with keyboard accelerator";
                    restAppBarButtonOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    restAppBarButtonOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::A, ::Windows::System::VirtualKeyModifiers::None));
                    cmdBar->SecondaryCommands->Append(restAppBarButtonOverflowWithKeyboardAccelerator);

                    pointerOverAppBarButtonOverflow = ref new xaml_controls::AppBarButton();
                    pointerOverAppBarButtonOverflow->Label = "PointerOver Overflow AppBarButton";
                    pointerOverAppBarButtonOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->SecondaryCommands->Append(pointerOverAppBarButtonOverflow);

                    pointerOverAppBarButtonOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarButton();
                    pointerOverAppBarButtonOverflowWithKeyboardAccelerator->Label = "PointerOver Overflow AppBarButton with keyboard accelerator";
                    pointerOverAppBarButtonOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pointerOverAppBarButtonOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::S, ::Windows::System::VirtualKeyModifiers::Control));
                    cmdBar->SecondaryCommands->Append(pointerOverAppBarButtonOverflowWithKeyboardAccelerator);

                    pressedAppBarButtonOverflow = ref new xaml_controls::AppBarButton();
                    pressedAppBarButtonOverflow->Label = "Pressed Overflow AppBarButton";
                    pressedAppBarButtonOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBar->SecondaryCommands->Append(pressedAppBarButtonOverflow);

                    pressedAppBarButtonOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarButton();
                    pressedAppBarButtonOverflowWithKeyboardAccelerator->Label = "Pressed Overflow AppBarButton with keyboard accelerator";
                    pressedAppBarButtonOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    pressedAppBarButtonOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::D, ::Windows::System::VirtualKeyModifiers::Shift));
                    cmdBar->SecondaryCommands->Append(pressedAppBarButtonOverflowWithKeyboardAccelerator);

                    disabledAppBarButtonOverflow = ref new xaml_controls::AppBarButton();
                    disabledAppBarButtonOverflow->Label = "Disabled Overflow AppBarButton";
                    disabledAppBarButtonOverflow->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarButtonOverflow->IsEnabled = false;
                    cmdBar->SecondaryCommands->Append(disabledAppBarButtonOverflow);

                    disabledAppBarButtonOverflowWithKeyboardAccelerator = ref new xaml_controls::AppBarButton();
                    disabledAppBarButtonOverflowWithKeyboardAccelerator->Label = "Disabled Overflow AppBarButton with keyboard accelerator";
                    disabledAppBarButtonOverflowWithKeyboardAccelerator->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    disabledAppBarButtonOverflowWithKeyboardAccelerator->IsEnabled = false;
                    disabledAppBarButtonOverflowWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::F, ::Windows::System::VirtualKeyModifiers::Menu | ::Windows::System::VirtualKeyModifiers::Windows));
                    cmdBar->SecondaryCommands->Append(disabledAppBarButtonOverflowWithKeyboardAccelerator);

                    cmdBarWithRightAlignedLabels = ref new xaml_controls::CommandBar();
                    cmdBarWithRightAlignedLabels->VerticalAlignment = xaml::VerticalAlignment::Top;
                    cmdBarWithRightAlignedLabels->IsOpen = true;
                    cmdBarWithRightAlignedLabels->IsDynamicOverflowEnabled = false;
                    cmdBarWithRightAlignedLabels->DefaultLabelPosition = xaml_controls::CommandBarDefaultLabelPosition::Right;

                    rightLabelAppBarButton = ref new xaml_controls::AppBarButton();
                    rightLabelAppBarButton->Label = "Right Label AppBarButton";
                    rightLabelAppBarButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
                    cmdBarWithRightAlignedLabels->PrimaryCommands->Append(rightLabelAppBarButton);

                    rootPanel = ref new xaml_controls::StackPanel();
                    rootPanel->Children->Append(cmdBarWithRightAlignedLabels);
                    rootPanel->Children->Append(cmdBar);

                    // Prevent any interference from the mouse:
                    rootPanel->IsHitTestVisible = false;

                    TestServices::WindowHelper->WindowContent = rootPanel;

                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // AppBarButton states
                    VisualStateManager::GoToState(pointerOverAppBarButton, "PointerOver", false);
                    VisualStateManager::GoToState(pointerOverAppBarButtonWithKeyboardAccelerator, "PointerOver", false);
                    VisualStateManager::GoToState(pressedAppBarButton, "Pressed", false);
                    VisualStateManager::GoToState(pressedAppBarButtonWithKeyboardAccelerator, "Pressed", false);
                    VisualStateManager::GoToState(pointerOverAppBarButtonOverflow, "PointerOver", false);
                    VisualStateManager::GoToState(pointerOverAppBarButtonOverflowWithKeyboardAccelerator, "PointerOver", false);
                    VisualStateManager::GoToState(pressedAppBarButtonOverflow, "Pressed", false);
                    VisualStateManager::GoToState(pressedAppBarButtonOverflowWithKeyboardAccelerator, "Pressed", false);

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

    // Displays a CommandBar that progressively gets narrower and puts its AppBarButton instances
    // into the SecondaryCommands collection. Makes sure the AppBarButton instances' Width and Height
    // are NaN for each CommandBar width.
    void AppBarButtonIntegrationTests::ValidateUIElementTreeWithIcons()
    {
        TestCleanupWrapper cleanup;

        const int appBarButtonCount = 3;
        const int openingIterations = 3;

        xaml_controls::Grid^ grid = nullptr;
        xaml_controls::CommandBar^ cmdBar = nullptr;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->DefaultLabelPosition = xaml_controls::CommandBarDefaultLabelPosition::Right;
            cmdBar->IsEnabled = false;

            for (int i = 0; i < appBarButtonCount; i++)
            {
                xaml_controls::AppBarButton^ appBarButton = ref new xaml_controls::AppBarButton();

                appBarButton->Label = "Button" + i;
                appBarButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend + (xaml_controls::Symbol) i);

                cmdBar->PrimaryCommands->Append(appBarButton);
            }

            grid = ref new xaml_controls::Grid();
            auto row = ref new xaml_controls::RowDefinition();
            row->Height = xaml::GridLengthHelper::Auto;
            grid->RowDefinitions->Append(row);

            grid->Children->Append(cmdBar);

            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        for (int iteration = 1; iteration <= openingIterations; iteration++)
        {
            RunOnUIThread([&]()
            {
                cmdBar->IsOpen = true;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                for (unsigned int i = 0; i < cmdBar->PrimaryCommands->Size; i++)
                {
                    xaml_controls::AppBarButton^ appBarButton = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(i));
                    VERIFY_IS_NOT_NULL(appBarButton);
                    VERIFY_IS_TRUE(!!_isnan(appBarButton->Width), L"AppBarButton.Width should be NaN");
                    VERIFY_IS_TRUE(!!_isnan(appBarButton->Height), L"AppBarButton.Height should be NaN");
                }

                for (unsigned int i = 0; i < cmdBar->SecondaryCommands->Size; i++)
                {
                    xaml_controls::AppBarButton^ appBarButton = safe_cast<xaml_controls::AppBarButton^>(cmdBar->SecondaryCommands->GetAt(i));
                    VERIFY_IS_NOT_NULL(appBarButton);
                    VERIFY_IS_TRUE(!!_isnan(appBarButton->Width), L"AppBarButton.Width should be NaN");
                    VERIFY_IS_TRUE(!!_isnan(appBarButton->Height), L"AppBarButton.Height should be NaN");
                }
            });

            RunOnUIThread([&]()
            {
                cmdBar->IsOpen = false;
            });

            TestServices::WindowHelper->WaitForIdle();

            if (iteration < openingIterations)
            {
                size.Width -= 125;
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                TestServices::WindowHelper->WaitForIdle();
            }
        }
    }

    void AppBarButtonIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedAppBarButtonWidth = 68;
        double const expectedAppBarButtonHeight = 64;

        double const expectedAppBarButtonWithMultiLineLabelWidth = 68;
        double const expectedAppBarButtonWithMultiLineLabelHeight = 74;

        double const expectedAppBarButtonCompactWidth = 68;
        double const expectedAppBarButtonCompactHeight = 64;

        double const expectedAppBarButtonRightLabelPositionWidth = 84;
        double const expectedAppBarButtonRightLabelPositionHeight = 48;

        double const expectedAppBarButtonCollapsedLabelPositionWidth = 68;
        double const expectedAppBarButtonCollapsedLabelPositionHeight = 48;

        double const expectedAppBarButtonInOverflowWidth = 158;
        double const expectedAppBarButtonInOverflowHeight = 32;

        xaml_controls::AppBarButton^ appBarButton = nullptr;
        xaml_controls::AppBarButton^ appBarButtonWithMultiLineLabel = nullptr;
        xaml_controls::AppBarButton^ appBarButtonCompact = nullptr;
        xaml_controls::AppBarButton^ appBarButtonRightLabelPosition = nullptr;
        xaml_controls::AppBarButton^ appBarButtonCollapsedLabelPosition = nullptr;
        xaml_controls::AppBarButton^ appBarButtonInOverflow = nullptr;

        // Override the window size to make sure the CommandBar doesn't try to use full width
        // on phone.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 600));

        // Force focus to ensure a consistent sizing of the overflow menu
        // since it's based on last input mode.
        xaml_controls::Button^ focusButton = nullptr;
        RunOnUIThread([&]()
        {
            focusButton = ref new xaml_controls::Button();
            TestServices::WindowHelper->WindowContent = focusButton;
        });
        TestServices::WindowHelper->WaitForIdle();
        FocusTestHelper::EnsureFocus(focusButton, FocusState::Keyboard);
        focusButton = nullptr;
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <AppBarButton x:Name="appBarButton" Label="button" Icon="Accept"/>
                        <AppBarButton x:Name="appBarButtonWithMultiLineLabel" Label="button button line" Icon="Accept"/>
                        <AppBarButton x:Name="appBarButtonCompact" Label="button" Icon="Accept" IsCompact="True"/>
                        <AppBarButton x:Name="appBarButtonCollapsedLabelPosition" Label="button" Icon="Accept" LabelPosition="Collapsed"/>
                        <CommandBar DefaultLabelPosition="Right" IsOpen="True">
                            <AppBarButton x:Name="appBarButtonRightLabelPosition" Label="button" Icon="Accept"/>
                            <CommandBar.SecondaryCommands>
                                <AppBarButton x:Name="appBarButtonInOverflow" Label="button" Icon="Accept"/>
                            </CommandBar.SecondaryCommands>
                        </CommandBar>
                    </StackPanel>)"));

            appBarButton = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"appBarButton"));
            appBarButtonWithMultiLineLabel = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"appBarButtonWithMultiLineLabel"));
            appBarButtonCompact = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"appBarButtonCompact"));
            appBarButtonRightLabelPosition = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"appBarButtonRightLabelPosition"));
            appBarButtonCollapsedLabelPosition = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"appBarButtonCollapsedLabelPosition"));
            appBarButtonInOverflow = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"appBarButtonInOverflow"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedAppBarButtonWidth, appBarButton->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarButtonHeight, appBarButton->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarButtonWithMultiLineLabelWidth, appBarButtonWithMultiLineLabel->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarButtonWithMultiLineLabelHeight, appBarButtonWithMultiLineLabel->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarButtonCompactWidth, appBarButtonCompact->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarButtonCompactHeight, appBarButtonCompact->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarButtonRightLabelPositionWidth, appBarButtonRightLabelPosition->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarButtonRightLabelPositionHeight, appBarButtonRightLabelPosition->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarButtonCollapsedLabelPositionWidth, appBarButtonCollapsedLabelPosition->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarButtonCollapsedLabelPositionHeight, appBarButtonCollapsedLabelPosition->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarButtonInOverflowWidth, appBarButtonInOverflow->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarButtonInOverflowHeight, appBarButtonInOverflow->ActualHeight);
        });
    }

    void AppBarButtonIntegrationTests::LabelOnRightStyleIsDisabled()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
        {
            xaml_controls::Grid^ rootGrid = nullptr;

            RunOnUIThread([&]()
            {
                // If LabelOnRightStyle is disabled, BorderThickness should not be 5.
                rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                    LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                          <CommandBar DefaultLabelPosition="Right" Style="{ThemeResource CommandBarWithoutRevealStyle}">
                                <CommandBar.Resources>
                                    <Style TargetType="AppBarButton">
                                        <Setter Property="BorderThickness" Value="1" />
                                        <Setter Property="Template">
                                            <Setter.Value>
                                                <ControlTemplate TargetType="AppBarButton">
                                                    <Grid x:Name="Root" >
                                                        <Grid.Resources>
                                                            <Style x:Name="LabelOnRightStyle" TargetType="AppBarButton">
                                                                 <Setter Property="BorderThickness" Value="5" />
                                                            </Style>
                                                        </Grid.Resources>
                                                        <TextBlock Text="Hi"/>
                                                    </Grid>
                                                </ControlTemplate>
                                            </Setter.Value>
                                        </Setter>
                                    </Style>
                                </CommandBar.Resources>
                                <AppBarButton Icon="Accept" />
                            </CommandBar>
                        </Grid>)"));

                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                xaml_controls::CommandBar^ cmdBar = safe_cast<xaml_controls::CommandBar^>(rootGrid->Children->GetAt(0));
                cmdBar->IsOpen = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            return rootGrid;
        }
        );
    }

    xaml_input::KeyboardAccelerator^ AppBarButtonIntegrationTests::CreateKeyboardAccelerator(::Windows::System::VirtualKey key, ::Windows::System::VirtualKeyModifiers modifiers)
    {
        auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
        keyboardAccelerator->Key = key;
        keyboardAccelerator->Modifiers = modifiers;
        return keyboardAccelerator;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AppBarButton
