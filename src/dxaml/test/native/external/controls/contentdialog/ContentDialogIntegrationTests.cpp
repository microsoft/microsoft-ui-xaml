// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ContentDialogIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>

#include <TreeHelper.h>
#include <ControlHelper.h>
#include <CommandHelper.h>
#include "KeyboardInjectionOverride.h"

#include <AutomationClient\AutomationGenericTests.h>
#include <UIAutomationHelper.h>

#include <FocusTestHelper.h>
#include <WUCRenderingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace test_infra;
using namespace ::Windows::UI::ViewManagement;
using namespace MockDComp;

using namespace Concurrency;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContentDialog {

    const float g_heightComparisonErrorMargin = 0.1f;

    ref class CustomCommand : public Microsoft::UI::Xaml::Input::ICommand {
    public:
        virtual event wf::EventHandler<Object^>^ CanExecuteChanged;
        virtual event wf::EventHandler<Object^>^ Executed;

        virtual bool CanExecute(Platform::Object^ parameter)
        {
            return true;
        }

        virtual void Execute(Platform::Object^ parameter)
        {
            Executed(this, parameter);
        }
    };

    bool ContentDialogIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ContentDialogIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ContentDialogIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ContentDialogIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ContentDialog>::CanInstantiate();
    }

    void ContentDialogIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::ContentDialog>::CanEnterAndLeaveLiveTree();
    }

    void ContentDialogIntegrationTests::CanOpenAndCloseProjectedShadow()
    {
        RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
        CanOpenAndClose();
    }

    void ContentDialogIntegrationTests::CanOpenAndCloseDropShadow()
    {
        CanOpenAndClose();
    }

    void ContentDialogIntegrationTests::CanOpenAndClose()
    {
        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) ====");
        CanOpenAndCloseWorker(xaml_controls::ContentDialogPlacement::Popup, true);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.InPlace ====");
        CanOpenAndCloseWorker(xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.UnconstrainedPopup ====");
        CanOpenAndCloseWorker(xaml_controls::ContentDialogPlacement::UnconstrainedPopup);
    }

    void ContentDialogIntegrationTests::CanOpenAndCloseWorker(xaml_controls::ContentDialogPlacement placement, bool validateDCompTree)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);

        if (placement == xaml_controls::ContentDialogPlacement::InPlace)
        {
            RunOnUIThread([&]()
            {
                auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                root->Children->Append(contentDialog);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        OpenContentDialog(contentDialog, placement);

        if (validateDCompTree)
        {
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateUnconstrainedPopupPlacementBehavior()
    {
        TestCleanupWrapper cleanup;

        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);

        OpenContentDialog(contentDialog, xaml_controls::ContentDialogPlacement::UnconstrainedPopup);

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Windowed");

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateCanSwitchBetweenWindowedAndNotWindowedPopup()
    {
        TestCleanupWrapper cleanup;

        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);

        OpenContentDialog(contentDialog, xaml_controls::ContentDialogPlacement::UnconstrainedPopup);

        LOG_OUTPUT(L"Verifying initially set to windowed content dialog.");

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"InitiallyWindowed");

        CloseContentDialog(contentDialog);

        OpenContentDialog(contentDialog, xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"Verifying set to not windowed content dialog.");

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"NotWindowed");

        CloseContentDialog(contentDialog);

        OpenContentDialog(contentDialog, xaml_controls::ContentDialogPlacement::UnconstrainedPopup);

        LOG_OUTPUT(L"Verifying set to windowed content dialog after it was set to not windowed.");

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"WindowedAfterNotWindowed");

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::VisualElementsAreCorrect()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);

        OpenContentDialog(contentDialog);

        RunOnUIThread([&] ()
        {
            auto titleContentControl = safe_cast<xaml_controls::ContentControl^>(TreeHelper::GetVisualChildByName(contentDialog, L"Title"));

            VERIFY_IS_NOT_NULL(titleContentControl);

            Platform::String^ actualTitleString = safe_cast<Platform::String^>(titleContentControl->Content);
            VERIFY_IS_NOT_NULL(actualTitleString);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(contentDialog->Title)->Data(), actualTitleString->Data());

            auto contentContentPresenter = safe_cast<xaml_controls::ContentPresenter^>(TreeHelper::GetVisualChildByName(contentDialog, L"Content"));

            VERIFY_IS_NOT_NULL(contentContentPresenter);

            Platform::String^ actualContentString = safe_cast<Platform::String^>(contentContentPresenter->Content);
            VERIFY_IS_NOT_NULL(actualContentString);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(contentDialog->Content)->Data(), actualContentString->Data());

            auto primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
            VERIFY_IS_NOT_NULL(primaryButton);

            Platform::String^ actualPrimaryButtonString = safe_cast<Platform::String^>(primaryButton->Content);
            VERIFY_IS_NOT_NULL(actualPrimaryButtonString);
            VERIFY_ARE_EQUAL(contentDialog->PrimaryButtonText->Data(), actualPrimaryButtonString->Data());

            auto secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);
            VERIFY_IS_NOT_NULL(secondaryButton);

            Platform::String^ actualSecondaryButtonString = safe_cast<Platform::String^>(secondaryButton->Content);
            VERIFY_IS_NOT_NULL(actualSecondaryButtonString);
            VERIFY_ARE_EQUAL(contentDialog->SecondaryButtonText->Data(), actualSecondaryButtonString->Data());
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateSettingUICommandSetsButtonProperties()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ContentDialog^ contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

        OpenContentDialog(contentDialog);
        xaml_controls::Button^ primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
        xaml_controls::Button^ secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);
        xaml_controls::Button^ closeButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Close);

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Assigning the commands to the buttons. We expect the buttons to pick up the commands' properties.");
            CommandHelper::ValidateSettingUICommandSetsProperties_SetCommand(xaml_controls::ContentDialog::PrimaryButtonCommandProperty, contentDialog);
            CommandHelper::ValidateSettingUICommandSetsProperties_SetCommand(xaml_controls::ContentDialog::SecondaryButtonCommandProperty, contentDialog);
            CommandHelper::ValidateSettingUICommandSetsProperties_SetCommand(xaml_controls::ContentDialog::CloseButtonCommandProperty, contentDialog);

            LOG_OUTPUT(L"Validating primary button properties...");
            CommandHelper::ValidateSettingUICommandSetsProperties_VerifySetProperties(xaml_controls::Button::ContentProperty, nullptr, primaryButton);
            LOG_OUTPUT(L"Validating secondary button properties...");
            CommandHelper::ValidateSettingUICommandSetsProperties_VerifySetProperties(xaml_controls::Button::ContentProperty, nullptr, secondaryButton);
            LOG_OUTPUT(L"Validating close button properties...");
            CommandHelper::ValidateSettingUICommandSetsProperties_VerifySetProperties(xaml_controls::Button::ContentProperty, nullptr, closeButton);

            LOG_OUTPUT(L"Clearing the buttons' commands. We expect the buttons to clear the properties that were set.");
            CommandHelper::ValidateSettingUICommandSetsProperties_ClearCommand(xaml_controls::ContentDialog::PrimaryButtonCommandProperty, contentDialog);
            CommandHelper::ValidateSettingUICommandSetsProperties_ClearCommand(xaml_controls::ContentDialog::SecondaryButtonCommandProperty, contentDialog);
            CommandHelper::ValidateSettingUICommandSetsProperties_ClearCommand(xaml_controls::ContentDialog::CloseButtonCommandProperty, contentDialog);

            LOG_OUTPUT(L"Validating primary button properties...");
            CommandHelper::ValidateSettingUICommandSetsProperties_VerifyClearedProperties(xaml_controls::Button::ContentProperty, nullptr, primaryButton);
            LOG_OUTPUT(L"Validating secondary button properties...");
            CommandHelper::ValidateSettingUICommandSetsProperties_VerifyClearedProperties(xaml_controls::Button::ContentProperty, nullptr, secondaryButton);
            LOG_OUTPUT(L"Validating close button properties...");
            CommandHelper::ValidateSettingUICommandSetsProperties_VerifyClearedProperties(xaml_controls::Button::ContentProperty, nullptr, closeButton);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateSettingUICommandDoesNotOverwriteButtonText()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ContentDialog^ contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
        Platform::String^ expectedPrimaryButtonText = ref new Platform::String(L"Primary");
        Platform::String^ expectedSecondaryButtonText = ref new Platform::String(L"Secondary");
        Platform::String^ expectedCloseButtonText = ref new Platform::String(L"Close");
        Platform::String^ commandPrimaryButtonText = ref new Platform::String(L"CommandPrimary");
        Platform::String^ commandSecondaryButtonText = ref new Platform::String(L"CommandSecondary");
        Platform::String^ commandCloseButtonText = ref new Platform::String(L"CommandClose");

        RunOnUIThread([&]
        {
            contentDialog->PrimaryButtonText = expectedPrimaryButtonText;
            contentDialog->SecondaryButtonText = expectedSecondaryButtonText;
            contentDialog->CloseButtonText = expectedCloseButtonText;

            auto primaryUICommand = ref new xaml_input::XamlUICommand();
            primaryUICommand->Label = commandPrimaryButtonText;
            contentDialog->PrimaryButtonCommand = primaryUICommand;

            auto secondaryUICommand = ref new xaml_input::XamlUICommand();
            secondaryUICommand->Label = commandSecondaryButtonText;
            contentDialog->SecondaryButtonCommand = secondaryUICommand;

            auto closeUICommand = ref new xaml_input::XamlUICommand();
            closeUICommand->Label = commandCloseButtonText;
            contentDialog->CloseButtonCommand = closeUICommand;
        });

        OpenContentDialog(contentDialog);

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Expected primary button text: \"%s\"", expectedPrimaryButtonText->Data());
            LOG_OUTPUT(L"Actual primary button text: \"%s\"", contentDialog->PrimaryButtonText->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedPrimaryButtonText, contentDialog->PrimaryButtonText) == 0);

            LOG_OUTPUT(L"Expected secondary button text: \"%s\"", expectedSecondaryButtonText->Data());
            LOG_OUTPUT(L"Actual secondary button text: \"%s\"", contentDialog->SecondaryButtonText->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedSecondaryButtonText, contentDialog->SecondaryButtonText) == 0);

            LOG_OUTPUT(L"Expected close button text: \"%s\"", expectedCloseButtonText->Data());
            LOG_OUTPUT(L"Actual close button text: \"%s\"", contentDialog->CloseButtonText->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedCloseButtonText, contentDialog->CloseButtonText) == 0);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateFocusShift()
    {
        auto runScenario = [](xaml_controls::ContentDialogPlacement placement, bool forceInVisualTree = false)
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Button^ pageButton1 = nullptr;
            xaml_controls::Button^ pageButton2 = nullptr;
            xaml_controls::Button^ pageButton3 = nullptr;

            auto pageButton1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto pageButton2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto pageButton3GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            // [PB2] occurs twice at the end because we are using two different ContentDialogs in this test.
            Platform::String^ expectedFocusSequence = L"[PB2][TB1][PB2][H1][PB2]";
            Platform::String^ focusSequence = "";

            RunOnUIThread([&]()
            {
                auto panel = ref new xaml_controls::StackPanel();

                pageButton1 = ref new xaml_controls::Button();
                pageButton2 = ref new xaml_controls::Button();
                pageButton3 = ref new xaml_controls::Button();

                pageButton1->Content = "PB1";
                pageButton2->Content = "PB2";
                pageButton3->Content = "PB3";

                panel->Children->Append(pageButton1);
                panel->Children->Append(pageButton2);
                panel->Children->Append(pageButton3);

                auto root = ref new xaml_controls::Grid();
                root->Children->Append(panel);

                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(pageButton1, FocusState::Pointer);

            RunOnUIThread([&]()
            {
                pageButton1GotFocusRegistration.Attach(pageButton1, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml_controls::Button^>(args->OriginalSource)->Content + "]";
                }));
                pageButton2GotFocusRegistration.Attach(pageButton2, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml_controls::Button^>(args->OriginalSource)->Content + "]";
                }));
                pageButton3GotFocusRegistration.Attach(pageButton3, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml_controls::Button^>(args->OriginalSource)->Content + "]";
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Start the focus on the middle button button on the page.
            RunOnUIThread([&]()
            {
                // Make sure our focus sequence is clear before we start the actual test.
                focusSequence = "";

                pageButton2->Focus(xaml::FocusState::Programmatic);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Test with ContentDialog having a Control for Content.
            xaml_controls::ContentDialog^ contentDialog = CreateContentDialog(ContentDialogContent::TextBoxContent);

            auto dialogGotFocusRegistration = CreateSafeEventRegistration(xaml::UIElement, GotFocus);

            if (placement == xaml_controls::ContentDialogPlacement::InPlace || forceInVisualTree)
            {
                RunOnUIThread([&]()
                {
                    auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                    root->Children->Append(contentDialog);
                });
            }
            else
            {
                RunOnUIThread([&]()
                {
                    contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
                });
            }
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto gotFocusElement = forceInVisualTree ? safe_cast<xaml::UIElement^>(TreeHelper::GetVisualChildByName(contentDialog, L"LayoutRoot")) : safe_cast<xaml::UIElement^>(contentDialog);
                dialogGotFocusRegistration.Attach(gotFocusElement, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    Platform::String^ name;

                    if (dynamic_cast<xaml::FrameworkElement^>(args->OriginalSource))
                    {
                        name = dynamic_cast<xaml::FrameworkElement^>(args->OriginalSource)->Name;
                    }
                    else if (dynamic_cast<xaml_docs::Hyperlink^>(args->OriginalSource))
                    {
                        name = dynamic_cast<xaml_docs::Hyperlink^>(args->OriginalSource)->Name;
                    }

                    focusSequence += "[" + name + "]";
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            OpenContentDialog(contentDialog, placement);
            CloseContentDialog(contentDialog);

            // Test with ContentDialog having a focusable, non-Control DependencyObject for Content.
            xaml_docs::Hyperlink^ hyperlink = nullptr;
            RunOnUIThread([&]()
            {
                hyperlink = safe_cast<xaml_docs::Hyperlink^>(xaml_markup::XamlReader::Load(
                    L"<Hyperlink xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"           x:Name='H1' NavigateUri='http://www.microsoft.com'>Microsoft link</Hyperlink>"));

                xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock();
                textBlock->Inlines->Append(hyperlink);
                contentDialog->Content = textBlock;
            });
            TestServices::WindowHelper->WaitForIdle();

            OpenContentDialog(contentDialog, placement);
            CloseContentDialog(contentDialog);

            LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
            LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
            VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);
        };

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) ====");
        runScenario(xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) & dialog in the visual tree ====");
        runScenario(xaml_controls::ContentDialogPlacement::Popup, true /*forceInVisualTree*/);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.InPlace ====");
        runScenario(xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.UnconstrainedPopup ====");
        runScenario(xaml_controls::ContentDialogPlacement::UnconstrainedPopup);
    }

    void ContentDialogIntegrationTests::ValidateFocusTrapping()
    {
        auto runScenario = [](xaml_controls::ContentDialogPlacement placement)
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::WaitForIdleBeforeAndAfter);

            RunOnUIThread([&]()
            {
                // We create a root panel with some buttons. When the ContentDialog is open, none of these buttons should ever get focus.

                auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                    LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                            xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <Button x:Name="TopButton" Content="Top" VerticalAlignment="Top" HorizontalAlignment="Center" />
                        <Button x:Name="BottomButton" Content="Bottom" VerticalAlignment="Bottom" HorizontalAlignment="Center" />
                        <Button x:Name="LeftButton" Content="Left" VerticalAlignment="Center" HorizontalAlignment="Left" />
                        <Button x:Name="RightButton" Content="Right" VerticalAlignment="Top" HorizontalAlignment="Right" />
                    </Grid>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::ContentDialog^ contentDialog = CreateContentDialog(ContentDialogContent::TextBoxContent);

            if (placement == xaml_controls::ContentDialogPlacement::InPlace)
            {
                RunOnUIThread([&]()
                {
                    auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                    root->Children->Append(contentDialog);
                });
            }
            else
            {
                RunOnUIThread([&]()
                {
                    contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
                });
            }
            TestServices::WindowHelper->WaitForIdle();

            OpenContentDialog(contentDialog, placement);

            xaml_controls::Button^ primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
            xaml_controls::Button^ secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);
            xaml_controls::TextBox^ textBoxInContentDialog = GetTextBoxFromContentDialogContent(contentDialog);

            LOG_OUTPUT(L"Initial focus should be on ContentDialog content");
            VerifyFocusedElement(textBoxInContentDialog);

            LOG_OUTPUT(L"Pressing Gamepad Up should not move focus");
            CommonInputHelper::Up(InputDevice::Gamepad);
            VerifyFocusedElement(textBoxInContentDialog);

            LOG_OUTPUT(L"Pressing Gamepad Down and Left should move focus to primary button");
            CommonInputHelper::Down(InputDevice::Gamepad);
            CommonInputHelper::Left(InputDevice::Gamepad);
            VerifyFocusedElement(primaryButton);

            LOG_OUTPUT(L"Pressing Gamepad Down should not move focus");
            CommonInputHelper::Down(InputDevice::Gamepad);
            VerifyFocusedElement(primaryButton);

            LOG_OUTPUT(L"Pressing Gamepad Left should not move focus");
            CommonInputHelper::Left(InputDevice::Gamepad);
            VerifyFocusedElement(primaryButton);

            LOG_OUTPUT(L"Pressing Gamepad Right should move focus to secondary button");
            CommonInputHelper::Right(InputDevice::Gamepad);
            VerifyFocusedElement(secondaryButton);

            LOG_OUTPUT(L"Pressing Gamepad Right should not move focus");
            CommonInputHelper::Right(InputDevice::Gamepad);
            VerifyFocusedElement(secondaryButton);

            LOG_OUTPUT(L"Pressing Gamepad Up should move focus to ContentDialog content");
            CommonInputHelper::Up(InputDevice::Gamepad);
            VerifyFocusedElement(textBoxInContentDialog);

            LOG_OUTPUT(L"Pressing Tab should move focus to primary button");
            TestServices::KeyboardHelper->Tab();
            VerifyFocusedElement(primaryButton);

            LOG_OUTPUT(L"Pressing Tab should move focus to secondary button");
            TestServices::KeyboardHelper->Tab();
            VerifyFocusedElement(secondaryButton);

            LOG_OUTPUT(L"Pressing Tab should move focus to ContentDialog content");
            TestServices::KeyboardHelper->Tab();
            VerifyFocusedElement(textBoxInContentDialog);

            LOG_OUTPUT(L"Pressing Shift+Tab should move focus to secondary button");
            TestServices::KeyboardHelper->ShiftTab();
            VerifyFocusedElement(secondaryButton);

            LOG_OUTPUT(L"Pressing Shift+Tab should move focus to primary button");
            TestServices::KeyboardHelper->ShiftTab();
            VerifyFocusedElement(primaryButton);

            LOG_OUTPUT(L"Pressing Shift+Tab should move focus to ContentDialog content");
            TestServices::KeyboardHelper->ShiftTab();
            VerifyFocusedElement(textBoxInContentDialog);

            CloseContentDialog(contentDialog);
        };

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) ====");
        runScenario(xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.InPlace ====");
        runScenario(xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.UnconstrainedPopup ====");
        runScenario(xaml_controls::ContentDialogPlacement::UnconstrainedPopup);
    }

    void ContentDialogIntegrationTests::ValidateFocusShiftWhenPreviouslyFocusedElementIsRemoved()
    {
        auto runScenario = [](xaml_controls::ContentDialogPlacement placement)
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ panel = nullptr;
            xaml_controls::Button^ pageButton1 = nullptr;
            xaml_controls::Button^ pageButton2 = nullptr;
            Platform::String^ pageButton1ContentString = L"PB1";

            auto pageButton1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto pageButton2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            Platform::String^ expectedFocusSequence = L"[PB2][TB1][PB1]";
            Platform::String^ focusSequence = "";

            RunOnUIThread([&]()
            {
                pageButton1 = ref new xaml_controls::Button();
                pageButton1->Content = pageButton1ContentString;
                pageButton2 = ref new xaml_controls::Button();
                pageButton2->Content = "PB2";

                pageButton1GotFocusRegistration.Attach(pageButton1, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml_controls::Button^>(args->OriginalSource)->Content + "]";
                }));
                pageButton2GotFocusRegistration.Attach(pageButton2, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml_controls::Button^>(args->OriginalSource)->Content + "]";
                }));

                panel = ref new xaml_controls::StackPanel();
                panel->Children->Append(pageButton1);
                panel->Children->Append(pageButton2);

                auto root = ref new xaml_controls::Grid();
                root->Children->Append(panel);

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Start the focus on the middle button button on the page.
            RunOnUIThread([&]()
            {
                // Make sure our focus sequence is clear before we start the actual test.
                focusSequence = "";

                pageButton2->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Add ContentDialog. Open it so the focus moves inside this ContentDialog, and then close it.
            xaml_controls::ContentDialog^ contentDialog = CreateContentDialog(ContentDialogContent::TextBoxContent);
            auto contentDialogGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, GotFocus);
            contentDialogGotFocusRegistration.Attach(contentDialog, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Name + "]";
            }));

            if (placement == xaml_controls::ContentDialogPlacement::InPlace)
            {
                RunOnUIThread([&]()
                {
                    auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                    root->Children->Append(contentDialog);
                });
            }
            else
            {
                RunOnUIThread([&]()
                {
                    contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
                });
            }
            TestServices::WindowHelper->WaitForIdle();

            OpenContentDialog(contentDialog, placement);

            // Remove the LastFocusedElement before ContentDialog was opened.
            RunOnUIThread([&]()
            {
                panel->Children->RemoveAtEnd();
            });
            TestServices::WindowHelper->WaitForIdle();

            CloseContentDialog(contentDialog);

            // Verify there is a Button (pageButton1) in the VisualTree; an indication that
            // the  app has not crashed and the app page is still up.
            RunOnUIThread([&]()
            {
                auto button = TreeHelper::GetVisualChildByType<xaml_controls::Button>(panel);
                VERIFY_IS_NOT_NULL(button);
                VERIFY_ARE_EQUAL(button->Content->ToString(), pageButton1ContentString);
            });

            LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
            LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
            VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);
        };

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) ====");
        runScenario(xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.InPlace ====");
        runScenario(xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.UnconstrainedPopup ====");
        runScenario(xaml_controls::ContentDialogPlacement::UnconstrainedPopup);
    }

    void ContentDialogIntegrationTests::DoesFullSizeWorkCorrectlyInV2Template()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);

        float windowHeight = 0.f;
        double dialogHeight = 0.0;
        float tolerance = 1.0f; //Floating point rounding that occurs in the layout system can cause some of the below checks to fail. So we include some tolerance in our checks.
        xaml_controls::Border^ backgroundElement = nullptr;

        RunOnUIThread([&] ()
        {
            windowHeight = xaml::Window::Current->Bounds.Height;

            // Test the MaxHeight < WindowHeight behavior first.
            contentDialog->MaxHeight = floor(windowHeight * 0.75);
        });

        OpenContentDialog(contentDialog);

        RunOnUIThread([&] ()
        {
            backgroundElement = safe_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByName(contentDialog, L"BackgroundElement"));
            VERIFY_IS_NOT_NULL(backgroundElement);

            VERIFY_IS_TRUE(backgroundElement->ActualHeight <= contentDialog->MaxHeight + tolerance);
            dialogHeight = backgroundElement->ActualHeight;

            contentDialog->FullSizeDesired = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            VERIFY_IS_TRUE(backgroundElement->ActualHeight <= contentDialog->MaxHeight + tolerance);
            VERIFY_IS_TRUE(backgroundElement->ActualHeight > dialogHeight);

            contentDialog->FullSizeDesired = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog);

        // Now test the MaxHeight > WindowHeight behavior.
        RunOnUIThread([&] ()
        {
            contentDialog->MaxHeight = windowHeight * 2;
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenContentDialog(contentDialog);

        RunOnUIThread([&] ()
        {
            backgroundElement = safe_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByName(contentDialog, L"BackgroundElement"));
            VERIFY_IS_NOT_NULL(backgroundElement);

            VERIFY_IS_TRUE(backgroundElement->ActualHeight <= contentDialog->MaxHeight + tolerance);
            VERIFY_IS_TRUE(backgroundElement->ActualHeight <= windowHeight + tolerance);

            contentDialog->FullSizeDesired = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            VERIFY_IS_TRUE(backgroundElement->ActualHeight <= contentDialog->MaxHeight + tolerance);
            VERIFY_IS_TRUE(backgroundElement->ActualHeight <= windowHeight + tolerance);
            VERIFY_IS_TRUE(backgroundElement->ActualHeight > dialogHeight);

            contentDialog->FullSizeDesired = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::CanClickButtons()
    {
        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) ====");
        CanClickButtonsWorker(xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.InPlace ====");
        CanClickButtonsWorker(xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.UnconstrainedPopup ====");
        CanClickButtonsWorker(xaml_controls::ContentDialogPlacement::UnconstrainedPopup);
    }

    void ContentDialogIntegrationTests::CanClickButtonsWorker(xaml_controls::ContentDialogPlacement placement)
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent, placement);

        auto contentDialogResult = xaml_controls::ContentDialogResult::None;
        xaml_controls::Button^ primaryButton = nullptr;
        xaml_controls::Button^ secondaryButton = nullptr;

        if (placement == xaml_controls::ContentDialogPlacement::InPlace)
        {
            RunOnUIThread([&]()
            {
                auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                root->Children->Append(contentDialog);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        auto showAsyncResult = OpenContentDialog(contentDialog, placement);

        primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
        ControlHelper::DoClickUsingTap<xaml_controls::Button>(primaryButton);

        // Wait for the content dialog to close.
        contentDialogResult = create_task(showAsyncResult).get();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::Primary, contentDialogResult);

        showAsyncResult = OpenContentDialog(contentDialog, placement);

        secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);
        ControlHelper::DoClickUsingTap<xaml_controls::Button>(secondaryButton);

        // Wait for the content dialog to close.
        contentDialogResult = create_task(showAsyncResult).get();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::Secondary, contentDialogResult);
    }

    void ContentDialogIntegrationTests::CanDeferButtonClick()
    {
        LOG_OUTPUT(L"Validate can defer Primary button click.");
        CanDeferButtonClickHelper(xaml_controls::ContentDialogButton::Primary);

        LOG_OUTPUT(L"Validate can defer Secondary button click.");
        CanDeferButtonClickHelper(xaml_controls::ContentDialogButton::Secondary);

        LOG_OUTPUT(L"Validate can defer Close button click.");
        CanDeferButtonClickHelper(xaml_controls::ContentDialogButton::Close);
    }

    void ContentDialogIntegrationTests::CanDeferButtonClickHelper(xaml_controls::ContentDialogButton buttonType)
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);
        wf::IAsyncOperation<xaml_controls::ContentDialogResult>^ showAsyncOperation = nullptr;
        auto contentDialogResult = xaml_controls::ContentDialogResult::None;
        auto showAsyncResult = OpenContentDialog(contentDialog);
        xaml_controls::ContentDialogButtonClickDeferral^ deferral = nullptr;

        if (buttonType == xaml_controls::ContentDialogButton::Close)
        {
            RunOnUIThread([&]()
            {
                contentDialog->CloseButtonText = L"Close";
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        xaml_controls::Button^ button = nullptr;
        auto buttonClickEvent = std::make_shared<Event>();

        // Because CreateSafeEventRegistration is actually a magical macro with an indiscernible return type, we'll create both registrations.
        auto primaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, PrimaryButtonClick);
        auto secondaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, SecondaryButtonClick);
        auto closeButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, CloseButtonClick);

        RunOnUIThread([&]()
        {
            button = GetButton(contentDialog, buttonType);

            auto handler = ref new wf::TypedEventHandler<xaml_controls::ContentDialog^, xaml_controls::ContentDialogButtonClickEventArgs^>(
                [&](xaml_controls::ContentDialog^ sender, xaml_controls::ContentDialogButtonClickEventArgs^ args)
            {
                LOG_OUTPUT(L"Firing click event.");

                // Get the deferral
                deferral = args->GetDeferral();

                // Set the event.
                buttonClickEvent->Set();
            });

            primaryButtonClickRegistration.Attach(contentDialog, handler);
            secondaryButtonClickRegistration.Attach(contentDialog, handler);
            closeButtonClickRegistration.Attach(contentDialog, handler);
        });

        ControlHelper::DoClickUsingTap<xaml_primitives::ButtonBase>(button);

        // Wait for the buttonClickEvent to fire before continuing.
        buttonClickEvent->WaitForDefault();

        // Verify that the dialog is still open
        RunOnUIThread([&] ()
        {
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, contentDialog->Visibility);
        });

        RunOnUIThread([&] ()
        {
            // Complete the deferral. We can continue now.
            deferral->Complete();
        });

        // Wait for the content dialog to close.
        contentDialogResult = create_task(showAsyncResult).get();
        TestServices::WindowHelper->WaitForIdle();

        // Verify the content dialog closed.
        // We'll split this into two separate VERIFY so that the logs are more concise.
        switch (buttonType)
        {
        case xaml_controls::ContentDialogButton::Primary:
            VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::Primary, contentDialogResult);
            break;

        case xaml_controls::ContentDialogButton::Secondary:
            VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::Secondary, contentDialogResult);
            break;

        case xaml_controls::ContentDialogButton::Close:
            VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::None, contentDialogResult);
            break;
        }
    }

    void ContentDialogIntegrationTests::DoesTabBehaviorWork()
    {
        auto runScenario = [](xaml_controls::ContentDialogPlacement placement)
        {
            TestCleanupWrapper cleanup;

            xaml_controls::ContentDialog^ contentDialog = nullptr;
            xaml_controls::Button^ contentButton = nullptr;

            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, GotFocus);
            auto contentButtonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            const size_t tabCount = 5;
            Platform::String^ expectedFocusSequence = L"[C][B1][B2][B3][P][S][B1][S][P][B3][B2][B1][C]";
            Platform::String^ focusSequence = "";

            RunOnUIThread([&]()
            {
                contentDialog = safe_cast<xaml_controls::ContentDialog^>(xaml_markup::XamlReader::Load(
                    L"<ContentDialog xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"    PrimaryButtonText='P'"
                    L"    SecondaryButtonText='S'>"
                    L"    <StackPanel>"
                    L"        <Button Content='B1'/>"
                    L"        <Button Content='B2'/>"
                    L"        <Button Content='B3'/>"
                    L"    </StackPanel>"
                    L"</ContentDialog>"
                ));

                gotFocusRegistration.Attach(contentDialog, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml_controls::Button^>(args->OriginalSource)->Content + "]";
                }));

                auto rootGrid = ref new xaml_controls::Grid();
                contentButton = ref new xaml_controls::Button();

                contentButtonGotFocusRegistration.Attach(contentButton, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml_controls::Button^>(args->OriginalSource)->Content + "]";
                }));

                contentButton->Content = "C";
                rootGrid->Children->Append(contentButton);

                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Make sure our focus sequence is clear before we start the actual test.
            focusSequence = "";

            // Start the focus on button in the content area.
            RunOnUIThread([&]()
            {
                contentButton->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            if (placement == xaml_controls::ContentDialogPlacement::InPlace)
            {
                RunOnUIThread([&]()
                {
                    auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                    root->Children->Append(contentDialog);
                });
            }
            else
            {
                RunOnUIThread([&]()
                {
                    contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
                });
            }
            TestServices::WindowHelper->WaitForIdle();

            // When we open the content dialog, the first button should grab focus.
            OpenContentDialog(contentDialog, placement);

            // Tab to move focus through the control.
            for (size_t i = 0; i < tabCount; ++i)
            {
                TestServices::KeyboardHelper->Tab();
                TestServices::WindowHelper->WaitForIdle();
            }

            // Shift-Tab to move focus through the control in reverse.
            for (size_t i = 0; i < tabCount; ++i)
            {
                TestServices::KeyboardHelper->ShiftTab();
                TestServices::WindowHelper->WaitForIdle();
            }

            CloseContentDialog(contentDialog);

            VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);
        };

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) ====");
        runScenario(xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.InPlace ====");
        runScenario(xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.UnconstrainedPopup ====");
        runScenario(xaml_controls::ContentDialogPlacement::UnconstrainedPopup);
    }

    void ContentDialogIntegrationTests::ValidateSIPInteractionWithMultipleTextBoxes()
    {
        // This test ensures there is no problem when grippers are shown or hidden by text boxes are hosted inside content dialog.
        // the issue is caused by conflict of having both grippers and content dialog are children of popup root.
        // During layout process on content dialog, it erroneously triggered gripper hiding.

        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::MultipleTextBoxContent);
        OpenContentDialog(contentDialog);

        xaml_controls::TextBox^ textBox0;
        xaml_controls::TextBox^ textBox7;
        xaml_controls::Button^ button;

        RunOnUIThread([&]()
        {
            auto content = safe_cast<xaml_controls::StackPanel^>(contentDialog->Content);
            textBox0 = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(contentDialog, L"textBox0"));
            VERIFY_IS_NOT_NULL(textBox0);
            textBox7 = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(contentDialog, L"textBox7"));
            VERIFY_IS_NOT_NULL(textBox7);
            button = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(contentDialog, L"button1"));
            VERIFY_IS_NOT_NULL(button);
        });
        TestServices::WindowHelper->WaitForIdle();

        // keep tapping through textboxes and buttons to hit crash scenario
        int maxAttempts = 5;
        for (int i = 0; i < maxAttempts; i++)
        {
            TestServices::InputHelper->Tap(textBox7);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(button);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(textBox0);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(button);
            TestServices::WindowHelper->WaitForIdle();
        }
        // We're done with the dialog - close it down now.
        CloseContentDialog(contentDialog);
        TestServices::WindowHelper->WaitForIdle();
    }

    void ContentDialogIntegrationTests::ValidateSIPInteraction()
    {
        // This test ensures that when the SIP comes up as a result of tapping a TextBox, the dialog buttons are still interactable
        // and that the content shrinks appropriately. This test verifies these behaviors by getting the bounds of the button and
        // content before the SIP comes up, and checks them against the bounds of these elements after the SIP is up.
        // If the final button.Y value < original button.Y value, we know the buttons have moved up as a result of the SIP.
        // Similarly, if the final content.Height < original content.Height, we know that the content has shrunk because of the SIP.

        wf::EventRegistrationToken inputPaneShowingToken = {};

        // Since InputPane is not agile, we can't use SafeEventRegistration. We need to manage the SIP events manually.
        TestCleanupWrapper cleanup([&inputPaneShowingToken]()
        {
            RunOnUIThread([&inputPaneShowingToken]()
            {
                InputPane::GetForCurrentView()->Showing -= inputPaneShowingToken;
                inputPaneShowingToken = {};
            });

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::ButtonAndTextBox);
        auto showAsyncResult = OpenContentDialog(contentDialog);

        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::Button^ primaryButton = nullptr;
        xaml_controls::ScrollViewer^ contentScrollViewer = nullptr;

        wf::Rect primaryButtonBoundsOriginal;
        wf::Rect primaryButtonBoundsFinal;

        wf::Rect contentBoundsOriginal;
        wf::Rect contentBoundsFinal;

        RunOnUIThread([&] ()
        {
            // Setting full size will make sure that buttons need to be relocated and the content resized when the SIP shows.
            contentDialog->FullSizeDesired = true;
        });

        InputPane^ inputPane;
        auto SIPShowingEvent = std::make_shared<Event>();

        // Register for SIP events so we know when they fire.
        RunOnUIThread([&]()
        {
            inputPane = InputPane::GetForCurrentView();
            inputPaneShowingToken = inputPane->Showing += ref new wf::TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
            {
               SIPShowingEvent->Set();
            });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);

            contentScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(contentDialog, L"ContentScrollViewer"));

            // Log the bounds of the button and the bounds of the content before the SIP shows.
            primaryButtonBoundsOriginal = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(primaryButton, false);
            contentBoundsOriginal = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(contentScrollViewer, false);
        });

        RunOnUIThread([&] ()
        {
            auto content = safe_cast<xaml_controls::StackPanel^>(contentDialog->Content);
            textBox = TreeHelper::GetVisualChildByType<xaml_controls::TextBox>(content);

            // Don't show the keyboard unless we explicitly tap on it.
            textBox->PreventKeyboardDisplayOnProgrammaticFocus = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Open the Keyboard SIP by tapping on box.
        TestServices::InputHelper->Tap(textBox);

        //The SIP should pop up. Wait for SIP event.
        SIPShowingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // The SIP event fired. Log the bounds of the button and the bounds of the content.
        RunOnUIThread([&] ()
        {
            primaryButtonBoundsFinal = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(primaryButton, false);
            contentBoundsFinal = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(contentScrollViewer, false);
        });
        TestServices::WindowHelper->WaitForIdle();

        // We're done with the dialog - close it down now.
        CloseContentDialog(contentDialog);

        // If the SIP popped up, we expect that the bounds have changed. The buttons have moved up, and the content bounds have become shorter.
        bool didButtonMoveUp = primaryButtonBoundsFinal.Y < primaryButtonBoundsOriginal.Y;
        bool didContentResize = contentBoundsFinal.Height < contentBoundsOriginal.Height;

        // Print out some information in the failure cases.
        if (!didButtonMoveUp)
        {
            LOG_OUTPUT(L"Button bounds before SIP event: [X: %f, Y: %f, W: %f, H: %f]", primaryButtonBoundsOriginal.X, primaryButtonBoundsOriginal.Y, primaryButtonBoundsOriginal.Width, primaryButtonBoundsOriginal.Height);
            LOG_OUTPUT(L"Button bounds after SIP event : [X: %f, Y: %f, W: %f, H: %f]", primaryButtonBoundsFinal.X, primaryButtonBoundsFinal.Y, primaryButtonBoundsFinal.Width, primaryButtonBoundsFinal.Height);
        }

        if (!didContentResize)
        {
            LOG_OUTPUT(L"ContentScrollViewer bounds before SIP event: [X: %f, Y: %f, W: %f, H: %f]", contentBoundsOriginal.X, contentBoundsOriginal.Y, contentBoundsOriginal.Width, contentBoundsOriginal.Height);
            LOG_OUTPUT(L"ContentScrollViewer bounds after SIP event : [X: %f, Y: %f, W: %f, H: %f]", contentBoundsFinal.X, contentBoundsFinal.Y, contentBoundsFinal.Width, contentBoundsFinal.Height);
        }

        // Finally, verify that the buttons moved and content resized.
        VERIFY_IS_TRUE(didButtonMoveUp);
        VERIFY_IS_TRUE(didContentResize);
    }

    void ContentDialogIntegrationTests::DoesRespondToEscapeKey()
    {
        TestCleanupWrapper cleanup;

        auto runScenario = [](bool inVisualTree, xaml_controls::ContentDialogPlacement placement)
        {
            xaml_controls::ContentDialog^ contentDialog;

            if (inVisualTree || placement == xaml_controls::ContentDialogPlacement::InPlace)
            {
                contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent, xaml_controls::ContentDialogPlacement::InPlace);

                RunOnUIThread([&]()
                {
                    auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                    root->Children->Append(contentDialog);
                });
            }
            else
            {
                contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);
            }
            TestServices::WindowHelper->WaitForIdle();

            auto task = create_task(OpenContentDialog(contentDialog, placement));

            Platform::String^ escapeKeySequence = L"$d$_esc#$u$_esc";
            TestServices::KeyboardHelper->PressKeySequence(escapeKeySequence);

            // Wait for the content dialog to close.
            task.wait();

            VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::None, task.get());
        };

        LOG_OUTPUT(L"==== Validate using a ContentDialog *not* in the visual tree, ContentDialogPlacement.Popup ====");
        runScenario(false /* inVisualTree */, xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate using a ContentDialog *in* the visual tree, ContentDialogPlacement.Popup ====");
        runScenario(true /* inVisualTree */, xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate using a ContentDialog *in* the visual tree, ContentDialogPlacement.InPlace ====");
        runScenario(true /* inVisualTree */, xaml_controls::ContentDialogPlacement::InPlace);
    }

    void ContentDialogIntegrationTests::ValidateUIETDefault()
    {
        // Set the window height to be greater than 640 to avoid the top-alignment
        // behavioral differences between desktop and phone.
        ValidateUIETDefaultWorker(wf::Size(400, 650), ContentDialogSize::DefaultSize);
    }

    void ContentDialogIntegrationTests::ValidateUIETFullSize()
    {
        // Set the window height to be greater than 640 to avoid the top-alignment
        // behavioral differences between desktop and phone.
        ValidateUIETDefaultWorker(wf::Size(400, 650), ContentDialogSize::FullSize);
    }

    void ContentDialogIntegrationTests::ValidateUIETDefaultWorker(wf::Size windowSize, ContentDialogSize contentDialogSize)
    {
        ValidateTreeParams params(
            contentDialogSize == ContentDialogSize::DefaultSize ? L"Default" : L"FullSize",
            windowSize,
            1.f,
            // Test setup.
            [contentDialogSize]()
            {
                return ValidateUIElementTestSetup(contentDialogSize);
            },
            // Test cleanup.
            []()
            {
                xaml_controls::ContentDialog^ contentDialog = nullptr;

                RunOnUIThread([&contentDialog]()
                {
                    auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                    contentDialog = safe_cast<xaml_controls::ContentDialog^>(root->Children->GetAt(0));
                });

                if (contentDialog != nullptr)
                {
                    CloseContentDialog(contentDialog);
                }
            });

        // The ContentDialog dimmer rect is not updating with high-contrast changes so its color
        // isn't one of the high-contrast colors so it would fail the color validation.
        // Disable it for now.  This issue is tracked.
        params.SkipHCColorValidationOnMasterMismatch = true;

        ControlHelper::ValidateUIElementTree(params);
    }

    xaml_controls::Panel^ ContentDialogIntegrationTests::ValidateUIElementTestSetup(ContentDialogSize desiredSize)
    {
        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);

        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            contentDialog->Name = L"ContentDialog";
            contentDialog->VerticalAlignment = xaml::VerticalAlignment::Top;
            contentDialog->FullSizeDesired = (desiredSize == ContentDialogSize::FullSize);

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(contentDialog);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenContentDialog(contentDialog);

        return rootPanel;
    }

    void ContentDialogIntegrationTests::ValidateTextPanelFitsWithinWindow()
    {
        TestCleanupWrapper cleanup;

        wf::Rect contentDialogBounds = {};

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextPanel);
        auto showAsyncResult = OpenContentDialog(contentDialog);

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            contentDialogBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(contentDialog));
            LOG_OUTPUT(L"ContentDialog bounds left=%f top=%f width=%f height=%f", contentDialogBounds.Left, contentDialogBounds.Top, contentDialogBounds.Width, contentDialogBounds.Height);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto backgroundElement = safe_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByName(contentDialog, L"BackgroundElement"));
            VERIFY_IS_NOT_NULL(backgroundElement);

            auto dialogWidth = backgroundElement->ActualWidth;
            auto windowWidth = xaml::Window::Current->Bounds.Width;

            VERIFY_IS_TRUE(dialogWidth <= windowWidth);
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateAddingContentDialogToWindowContentDoesNotChangePositioning()
    {
        TestCleanupWrapper cleanup;

        wf::Rect layoutRootBoundsOutOfVisualTree = {};
        wf::Rect layoutRootBoundsInVisualTree = {};

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextPanel);
        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            auto backgroundElement = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"BackgroundElement", contentDialog));
            layoutRootBoundsOutOfVisualTree = ControlHelper::GetBounds(backgroundElement);
            LOG_OUTPUT(L"Outside of visual tree, bounds = (%f, %f, %f, %f)", layoutRootBoundsOutOfVisualTree.Left, layoutRootBoundsOutOfVisualTree.Top, layoutRootBoundsOutOfVisualTree.Width, layoutRootBoundsOutOfVisualTree.Height);
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog);

        auto contentDialog2 = SetupContentDialogTest(ContentDialogContent::TextPanel);

        RunOnUIThread([&]()
        {
            auto stackPanel = ref new xaml_controls::StackPanel();

            auto rectangle = ref new xaml_shapes::Rectangle();
            rectangle->Width = 100;
            rectangle->Height = 100;

            stackPanel->Children->Append(rectangle);
            stackPanel->Children->Append(contentDialog2);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenContentDialog(contentDialog2);

        RunOnUIThread([&]()
        {
            auto backgroundElement = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"BackgroundElement", contentDialog2));
            layoutRootBoundsInVisualTree = ControlHelper::GetBounds(backgroundElement);
            LOG_OUTPUT(L"Inside visual tree, bounds = (%f, %f, %f, %f)", layoutRootBoundsInVisualTree.Left, layoutRootBoundsInVisualTree.Top, layoutRootBoundsInVisualTree.Width, layoutRootBoundsInVisualTree.Height);
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog2);

        VERIFY_ARE_EQUAL(layoutRootBoundsOutOfVisualTree.Left, layoutRootBoundsInVisualTree.Left);
        VERIFY_ARE_EQUAL(layoutRootBoundsOutOfVisualTree.Top, layoutRootBoundsInVisualTree.Top);
        VERIFY_ARE_EQUAL(layoutRootBoundsOutOfVisualTree.Right, layoutRootBoundsInVisualTree.Right);
        VERIFY_ARE_EQUAL(layoutRootBoundsOutOfVisualTree.Bottom, layoutRootBoundsInVisualTree.Bottom);
    }

    void ContentDialogIntegrationTests::ValidateRTLContentDialogPosition()
    {
        TestCleanupWrapper cleanup;

        wf::Rect ltrBounds = {};
        wf::Rect rtlBounds = {};

        // Set up the LTR ContentDialog and show it.
        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            auto backgroundElement = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"BackgroundElement", contentDialog));
            ltrBounds = ControlHelper::GetBounds(backgroundElement);
            LOG_OUTPUT(L"LTR ContentDialog:, bounds = (%f, %f, %f, %f)", ltrBounds.Left, ltrBounds.Top, ltrBounds.Width, ltrBounds.Height);
        });
        TestServices::WindowHelper->WaitForIdle();
        CloseContentDialog(contentDialog);

        // Set up the RTL ContentDialog and show it.
        auto contentDialog2 = SetupContentDialogTest(ContentDialogContent::Empty);
        RunOnUIThread([&]()
        {
            contentDialog2->FlowDirection = xaml::FlowDirection::RightToLeft;
        });
        OpenContentDialog(contentDialog2);

        RunOnUIThread([&]()
        {
            auto backgroundElement = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"BackgroundElement", contentDialog2));
            rtlBounds = ControlHelper::GetBounds(backgroundElement);
            LOG_OUTPUT(L"RTL ContentDialog, bounds = (%f, %f, %f, %f)", rtlBounds.Left, rtlBounds.Top, rtlBounds.Width, rtlBounds.Height);
        });
        TestServices::WindowHelper->WaitForIdle();
        CloseContentDialog(contentDialog2);

        // Validate the RTL ContentDialog bounds are same with LTR ContentDialog bounds.
        VERIFY_ARE_EQUAL(ltrBounds.Left, rtlBounds.Left);
        VERIFY_ARE_EQUAL(ltrBounds.Top, rtlBounds.Top);
        VERIFY_ARE_EQUAL(ltrBounds.Right, rtlBounds.Right);
        VERIFY_ARE_EQUAL(ltrBounds.Bottom, rtlBounds.Bottom);
    }

    void ContentDialogIntegrationTests::DisengagementDoesNotCloseContentDialog()
    {
        TestCleanupWrapper cleanup;

        InputDevice device = InputDevice::Gamepad;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Button);
        xaml_controls::Button^ btn1 = nullptr;

        auto btn1FocusEngaged = std::make_shared<Event>();
        auto btn1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

        auto btn1FocusDisengaged = std::make_shared<Event>();
        auto btn1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

        RunOnUIThread([&]()
        {
            btn1 = safe_cast<xaml_controls::Button^>(contentDialog->Content);
            btn1->IsFocusEngagementEnabled = true;

            btn1FocusEngagedRegistration.Attach(btn1, [&]() { btn1FocusEngaged->Set(); });
            btn1FocusDisengagedRegistration.Attach(btn1, [&]() { btn1FocusDisengaged->Set(); });
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenContentDialog(contentDialog);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            btn1->Focus(FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(btn1->IsFocusEngagementEnabled, true);
            VERIFY_ARE_EQUAL(btn1->IsFocusEngaged, false);
        });
        CommonInputHelper::Accept(device);
        btn1FocusEngaged->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(btn1->IsFocusEngagementEnabled, true);
            VERIFY_ARE_EQUAL(btn1->IsFocusEngaged, true);
        });
        CommonInputHelper::Cancel(device);
        btn1FocusDisengaged->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(contentDialog->Visibility, xaml::Visibility::Visible);
            VERIFY_ARE_EQUAL(btn1->IsFocusEngagementEnabled, true);
            VERIFY_ARE_EQUAL(btn1->IsFocusEngaged, false);
            VERIFY_ARE_EQUAL(btn1->FocusState, FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateCommandSpaceSizeWhenFullSizeDesiredIsTrue()
    {
        ValidateCommandSpaceSizeWorker(true /* isFullSizeDesired */, wf::Size(318, 80));
    }

    void ContentDialogIntegrationTests::ValidateCommandSpaceSizeWorker(bool isFullSizeDesired, wf::Size expectedCommandSpaceSize)
    {
        TestCleanupWrapper cleanup;

        // Validate the CommandSpace size to ensure the Primary/SecondaryButton is shown correctly in the ContentDialog.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 800));

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::ContentDialog^ contentDialog = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                      <StackPanel Background="Orange">
                        <ContentDialog x:Name="ContentDialogTest" Title="MyTitle" PrimaryButtonText="OK" SecondaryButtonText="Cancel" />
                      </StackPanel>
                    </Grid>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            contentDialog = safe_cast<xaml_controls::ContentDialog^>(rootPanel->FindName(L"ContentDialogTest"));
            contentDialog->FullSizeDesired = isFullSizeDesired;
        });

        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            auto commandSpace = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"CommandSpace", contentDialog));

            VERIFY_ARE_EQUAL(expectedCommandSpaceSize.Width, commandSpace->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCommandSpaceSize.Height, commandSpace->ActualHeight);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateUpdatingPropertiesChangesButtonValues()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);

        wf::IAsyncOperation<xaml_controls::ContentDialogResult>^ showAsyncOperation = nullptr;
        xaml_controls::Button^ primaryButton = nullptr;
        xaml_controls::Button^ secondaryButton = nullptr;
        xaml_controls::TextBlock^ primaryButtonTextBlock = nullptr;
        xaml_controls::TextBlock^ secondaryButtonTextBlock = nullptr;

        auto showAsyncResult = OpenContentDialog(contentDialog);

        RunOnUIThread([&] ()
        {
            primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
            secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);

            primaryButtonTextBlock = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(primaryButton);
            secondaryButtonTextBlock = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(secondaryButton);

            VERIFY_ARE_EQUAL(contentDialog->IsPrimaryButtonEnabled, primaryButton->IsEnabled);
            contentDialog->IsPrimaryButtonEnabled = false;
            VERIFY_IS_FALSE(primaryButton->IsEnabled);

            VERIFY_ARE_EQUAL(secondaryButton->IsEnabled, secondaryButton->IsEnabled);
            contentDialog->IsSecondaryButtonEnabled = false;
            VERIFY_IS_FALSE(secondaryButton->IsEnabled);

            VERIFY_ARE_EQUAL(primaryButtonTextBlock->Text, contentDialog->PrimaryButtonText);
            VERIFY_ARE_EQUAL(secondaryButtonTextBlock->Text, contentDialog->SecondaryButtonText);
            contentDialog->PrimaryButtonText = "primary";
            contentDialog->SecondaryButtonText = "secondary";
        });
        TestServices::WindowHelper->WaitForIdle();

        // NOTE: If we check if the TextBlock->Text == Dialog->ButtonText immediately after setting it, the check fails.
        // We need to wait just a bit for the change to propagate, hence a WaitForIdle and separate Run block.
        RunOnUIThread([&] ()
        {
            primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
            secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);

            primaryButtonTextBlock = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(primaryButton);
            secondaryButtonTextBlock = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(secondaryButton);

            VERIFY_ARE_EQUAL(primaryButtonTextBlock->Text, contentDialog->PrimaryButtonText);
            VERIFY_ARE_EQUAL(secondaryButtonTextBlock->Text, contentDialog->SecondaryButtonText);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::DoButtonsGrowInHeightWithContent()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

        double defaultPrimaryButtonHeight = 0;
        double defaultSecondaryButtonHeight = 0;

        RunOnUIThread([&]()
        {
            contentDialog->IsPrimaryButtonEnabled = true;
            contentDialog->PrimaryButtonText = "primary";

            contentDialog->IsSecondaryButtonEnabled = true;
            contentDialog->SecondaryButtonText = "secondary";
        });

        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            auto button1 = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
            defaultPrimaryButtonHeight = button1->ActualHeight;

            auto button2 = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);
            defaultSecondaryButtonHeight = button2->ActualHeight;

            // Add multi-line content to the buttons to grow their height.
            contentDialog->PrimaryButtonText = "primary\nprimary\nprimary";
            contentDialog->SecondaryButtonText = "secondary\nsecondary\nsecondary";
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto button1 = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
            auto button2 = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);

            VERIFY_IS_GREATER_THAN(button1->ActualHeight, defaultPrimaryButtonHeight);
            VERIFY_IS_GREATER_THAN(button2->ActualHeight, defaultSecondaryButtonHeight);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::DoesNotTopAlignInNonFullScreenWindow()
    {
        TestCleanupWrapper cleanup;

        // The worker sets a window size override so it will not be full screen.
        DoesNotTopAlignWorker();
    }

    void ContentDialogIntegrationTests::DoesNotTopAlignOnXbox()
    {
        TestCleanupWrapper cleanup([]() {
            TestServices::WindowHelper->SetForceIsFullScreen(false);
            test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        // Make sure the framework thinks it's full-screen otherwise we're just testing
        // the 'does not top align in non-fullscreen windows' scenario, which is already
        // covered by the DoesNotTopAlignInNonFullScreenWindow test.
        TestServices::WindowHelper->SetForceIsFullScreen(true);

        DoesNotTopAlignWorker();
    }

    void ContentDialogIntegrationTests::DoesNotTopAlignWorker()
    {
        // ContentDialog tries to top-align itself when the window height is less than 640.
        // This test verifies that even under those circumstances.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 630));

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            auto backgroundElement = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"BackgroundElement", contentDialog));
            auto bounds = backgroundElement->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));

            VERIFY_IS_GREATER_THAN(bounds.Y, 0);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::VerifyCommandAreaIsVisibleWithDefaultButtonsTallContent()
    {
        VerifyCommandAreaIsVisibleWorker(ContentDialogContent::TallGrid, false);
    }

    void ContentDialogIntegrationTests::VerifyCommandAreaIsVisibleWithTallButtonsTallContent()
    {
        VerifyCommandAreaIsVisibleWorker(ContentDialogContent::TallGrid, true);
    }

    void ContentDialogIntegrationTests::VerifyCommandAreaIsVisibleWorker(ContentDialogContent content, bool areButtonsTall)
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(content);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(560, 800));

        RunOnUIThread([&]()
        {
            contentDialog->PrimaryButtonText = areButtonsTall ? "primary\nprimary\nprimary" : "primary";
            contentDialog->SecondaryButtonText = areButtonsTall ? "secondary\nsecondary\nsecondary" : "secondary";
        });

        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            auto button1 = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
            auto button2 = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);

            wf::Rect contentDialogBounds = {};
            contentDialogBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(contentDialog));
            LOG_OUTPUT(L"ContentDialog bounds left=%f top=%f width=%f height=%f", contentDialogBounds.Left, contentDialogBounds.Top, contentDialogBounds.Width, contentDialogBounds.Height);

            wf::Rect buttonBounds = {};
            buttonBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(button1));
            LOG_OUTPUT(L"Button bounds left=%f top=%f width=%f height=%f", buttonBounds.Left, buttonBounds.Top, buttonBounds.Width, buttonBounds.Height);

            // Verify that the buttons are within the visible bounds of the dialog
            VERIFY_IS_TRUE(ControlHelper::IsContainedIn(buttonBounds, contentDialogBounds));
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::VerifyDialogBoundsConstrainedToWindowBoundsDefaultButtonsTallContent()
    {
        VerifyDialogBoundsConstrainedToWindowBoundsWorker(ContentDialogContent::TallGrid, false /* areButtonsTall */);
    }

    void ContentDialogIntegrationTests::VerifyDialogBoundsConstrainedToWindowBoundsTallButtonsTallContent()
    {
        VerifyDialogBoundsConstrainedToWindowBoundsWorker(ContentDialogContent::TallGrid, true /* areButtonsTall */);
    }

    void ContentDialogIntegrationTests::VerifyDialogBoundsConstrainedToWindowBoundsDefaultButtonsShortContent()
    {
        VerifyDialogBoundsConstrainedToWindowBoundsWorker(ContentDialogContent::Default, false /* areButtonsTall */);
    }

    void ContentDialogIntegrationTests::VerifyDialogBoundsConstrainedToWindowBoundsTallButtonsShortContent()
    {
        VerifyDialogBoundsConstrainedToWindowBoundsWorker(ContentDialogContent::Default, true /* areButtonsTall */);
    }

    void ContentDialogIntegrationTests::VerifyDialogBoundsConstrainedToWindowBoundsWorker(ContentDialogContent content, bool areButtonsTall)
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(content);

        if (content == ContentDialogContent::TallGrid)
        {
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(560, 400));
        }
        else
        {
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(560, 800));
        }

        RunOnUIThread([&]()
        {
            contentDialog->PrimaryButtonText = areButtonsTall ? "primary\nprimary\nprimary" : "primary";
            contentDialog->SecondaryButtonText = areButtonsTall ? "secondary\nsecondary\nsecondary" : "secondary";
        });

        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            wf::Rect windowBounds;
            windowBounds = TestServices::WindowHelper->WindowBounds;
            LOG_OUTPUT(L"Window bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);

            wf::Rect contentDialogBounds = {};
            auto backgroundElement = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"BackgroundElement", contentDialog));
            contentDialogBounds = ControlHelper::GetBounds(backgroundElement);
            LOG_OUTPUT(L"ContentDialog bounds left=%f top=%f width=%f height=%f", contentDialogBounds.Left, contentDialogBounds.Top, contentDialogBounds.Width, contentDialogBounds.Height);

            // Verify that the dialog is within the visible bounds of the window
            VERIFY_IS_TRUE(ControlHelper::IsContainedIn(contentDialogBounds, windowBounds));
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::VerifyLongTitleDoesNotVerticallyClip()
    {
        TestCleanupWrapper cleanup;

        double expectedHeightOfTitleContent = 200;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Default);

        OpenContentDialog(contentDialog);

        RunOnUIThread([&] ()
        {
            // Set the ContentDialog's title to be a tall grid.
            auto grid = ref new xaml_controls::Grid();
            grid->Width = 200;
            grid->Height = expectedHeightOfTitleContent;
            grid->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Cyan);

            contentDialog->Title = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            auto titleContentControl = safe_cast<xaml_controls::ContentControl^>(TreeHelper::GetVisualChildByName(contentDialog, L"Title"));
            wf::Rect contentDialogTitleBounds = {};
            contentDialogTitleBounds = ControlHelper::GetBounds(titleContentControl);
            LOG_OUTPUT(L"ContentDialog Title bounds left=%f top=%f width=%f height=%f", contentDialogTitleBounds.Left, contentDialogTitleBounds.Top, contentDialogTitleBounds.Width, contentDialogTitleBounds.Height);

            // Verify that the actual bounds of the contentDialog are as tall as we originally set them to be.
            VERIFY_IS_TRUE(contentDialogTitleBounds.Height >= expectedHeightOfTitleContent);
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateAutomationNameByDelayingSetTitleValue()
    {
        TestCleanupWrapper cleanup;

        // Set the Title property later which is at the Loaded event handler and
        // validate the Automation Name property after opened ContentDialog.
        static WCHAR titleAutomationName[] = L"ContentDialog Title Name";

        Automation::AutomationClient::UIAElementInfo uiaInfo;

        uiaInfo.m_Name = titleAutomationName;
        uiaInfo.m_AutomationID = L"ContentDialog Automation ID";
        uiaInfo.m_cType = UIA_TextControlTypeId;

        xaml_controls::ContentDialog^ contentDialog = CreateContentDialog(ContentDialogContent::Empty);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = ref new xaml_controls::StackPanel();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
        });

        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Opened);
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Loaded);

        auto openedEvent = std::make_shared<Event>();
        auto loadedEvent = std::make_shared<Event>();

        openedRegistration.Attach(contentDialog, [&]() { openedEvent->Set(); });
        loadedRegistration.Attach(contentDialog, [&]()
        {
            contentDialog->Title = ref new Platform::String(titleAutomationName);
            contentDialog->PrimaryButtonText = L"OK";
            contentDialog->SecondaryButtonText = L"Cancel";

            loadedEvent->Set();
        });

        RunOnUIThread([&]()
        {
            contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
            contentDialog->ShowAsync();
        });
        openedEvent->WaitForDefault();
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            AutoBSTR propertyValue;

            auto spAutomationClientManager = Automation::AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            spUIAutomationElement->get_CurrentName(propertyValue.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Current AutomationName = '%s', Expected = '%s'.", propertyValue.Get(), titleAutomationName);

            VERIFY_IS_TRUE(!wcscmp(titleAutomationName, propertyValue));
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::ValidateClosingPopupInButtonHandlerDoesNotCrash()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Default);

        auto primaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, PrimaryButtonClick);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Closed);

        auto closedEvent = std::make_shared<Event>();

        primaryButtonClickRegistration.Attach(contentDialog,
            [&]()
            {
                // Most applications would not directly close the open popup like this, but this is the
                // min-repro necessary to validate to make sure this crash no longer occurs.
                // In the real world, what would likely lead to this scenario is removing the ContentDialog's
                // parent from the visual tree - for example, by navigating to another page.
                // If we do that while the ContentDialog popup is open, then it will be closed.
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(contentDialog->XamlRoot);
                popups->GetAt(0)->IsOpen = false;
            });

        closedRegistration.Attach(contentDialog, [&](){ closedEvent->Set(); });

        auto task = create_task(OpenContentDialog(contentDialog));

        ControlHelper::DoClickUsingTap<xaml_primitives::ButtonBase>(GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary));

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        task.wait();
    }

    void ContentDialogIntegrationTests::ValidateParentedContentDialogSize()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentDialog^ contentDialog = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <ContentDialog x:Name="ContentDialogTest" Title="MyTitle" PrimaryButtonText="OK" SecondaryButtonText="Cancel" />
                    </Grid>)"));

            contentDialog = safe_cast<xaml_controls::ContentDialog^>(rootPanel->FindName(L"ContentDialogTest"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(contentDialog->ActualWidth, 0);
            VERIFY_ARE_EQUAL(contentDialog->ActualHeight, 0);
        });
    }

    void ContentDialogIntegrationTests::DoesSupportCloseButton()
    {
        auto runScenario = [](xaml_controls::ContentDialogPlacement placement)
        {
            TestCleanupWrapper cleanup;

            auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
            auto customCommand = ref new CustomCommand();
            Platform::String^ expectedCommandParameter = "param";

            Event commandExecutedEvent;
            auto commandExecutedRegistration = CreateSafeEventRegistration(CustomCommand, Executed);
            commandExecutedRegistration.Attach(customCommand, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^, Platform::Object^ param)
            {
                VERIFY_ARE_EQUAL(expectedCommandParameter, safe_cast<Platform::String^>(param));
                commandExecutedEvent.Set();
            }));

            RunOnUIThread([&]()
            {
                contentDialog->CloseButtonText = "Close";
                contentDialog->CloseButtonCommand = customCommand;
                contentDialog->CloseButtonCommandParameter = expectedCommandParameter;

                auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                root->Children->Append(contentDialog);
            });

            Event closeButtonClickEvent;
            auto closeButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, CloseButtonClick);

            auto doValidation = [&](const task<xaml_controls::ContentDialogResult>& task)
            {
                // Wait for the ContentDialog's show async operation to complete, which
                // happens when it closes as well as waiting for the click event
                // and command to execute.
                task.wait();
                closeButtonClickEvent.WaitForDefault();
                commandExecutedEvent.WaitForDefault();

                VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::None, task.get());
            };

            LOG_OUTPUT(L"Validate the Close button using a tap.");
            auto showTask = create_task(OpenContentDialog(contentDialog, placement));

            auto closeButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Close);
            VERIFY_IS_NOT_NULL(closeButton);

            closeButtonClickRegistration.Attach(contentDialog, [&]() { closeButtonClickEvent.Set(); });

            ControlHelper::DoClickUsingTap<xaml_controls::Button>(closeButton);
            doValidation(showTask);

            LOG_OUTPUT(L"Validate the Close button using the ESCAPE key.");
            showTask = create_task(OpenContentDialog(contentDialog, placement));

            CommonInputHelper::Cancel(InputDevice::Keyboard);
            doValidation(showTask);

            LOG_OUTPUT(L"Validate the Close button using the GAMEPAD_B button.");
            showTask = create_task(OpenContentDialog(contentDialog, placement));

            CommonInputHelper::Cancel(InputDevice::Gamepad);
            doValidation(showTask);

            LOG_OUTPUT(L"Validate the Close button using the BACK button.");
            showTask = create_task(OpenContentDialog(contentDialog, placement));

            bool backButtonPressHandled = false;
            TestServices::Utilities->InjectBackButtonPress(&backButtonPressHandled);
            VERIFY_IS_TRUE(backButtonPressHandled);
            doValidation(showTask);
        };

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) ====");
        runScenario(xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.InPlace ====");
        runScenario(xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.UnconstrainedPopup ====");
        runScenario(xaml_controls::ContentDialogPlacement::UnconstrainedPopup);

    }

    void ContentDialogIntegrationTests::ValidateButtonsLayout()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 800));

        auto validationRules = ref new Platform::String(DefaultUIElementTreeValidationRules);

        std::array<std::tuple<const wchar_t*, bool, bool, bool>, 8> testCases =
        {
            std::make_tuple(L"none", false, false, false),       // None enabled
            std::make_tuple(L"PSC", true, true, true),           // All enabled
            std::make_tuple(L"PS", true, true, false),           // Primary & Secondary enabled
            std::make_tuple(L"PC", true, false, true),           // Primary & Close enabled
            std::make_tuple(L"P", true, false, false),           // Primary enabled
            std::make_tuple(L"SC", false, true, true),           // Secondary & Close enabled
            std::make_tuple(L"S", false, true, false),           // Secondary enabled
            std::make_tuple(L"C", false, false, true)            // Close enabled
        };

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
        auto task = create_task(OpenContentDialog(contentDialog));

        for (auto testCase : testCases)
        {
            RunOnUIThread([&]()
            {
                contentDialog->PrimaryButtonText = (std::get<1>(testCase) ? "Primary" : "");
                contentDialog->SecondaryButtonText = (std::get<2>(testCase) ? "Secondary" : "");
                contentDialog->CloseButtonText = (std::get<3>(testCase) ? "Close" : "");
            });
            TestServices::WindowHelper->WaitForIdle();

            auto testCaseId = ref new Platform::String(std::get<0>(testCase));
            TestServices::Utilities->VerifyUIElementTreeWithRulesInline(testCaseId, validationRules);
        }

        RunOnUIThread([&]() { contentDialog->Hide(); });
        task.wait();
    }

    void ContentDialogIntegrationTests::CanStyleButtons()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

        RunOnUIThread([&]()
        {
            auto style = safe_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                LR"(<Style xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" TargetType="Button">
                        <Setter Property="Tag" Value="StyledButton"/>
                    </Style>)"));

            contentDialog->PrimaryButtonText = "Primary";
            contentDialog->PrimaryButtonStyle = style;

            contentDialog->SecondaryButtonText = "Secondary";
            contentDialog->SecondaryButtonStyle = style;

            contentDialog->CloseButtonText = "Close";
            contentDialog->CloseButtonStyle = style;
        });

        auto task = create_task(OpenContentDialog(contentDialog));

        RunOnUIThread([&]()
        {
            auto primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
            auto secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);
            auto closeButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Close);

            auto expectedTag = ref new Platform::String(L"StyledButton");

            VERIFY_ARE_EQUAL(expectedTag, safe_cast<Platform::String^>(primaryButton->Tag));
            VERIFY_ARE_EQUAL(expectedTag, safe_cast<Platform::String^>(secondaryButton->Tag));
            VERIFY_ARE_EQUAL(expectedTag, safe_cast<Platform::String^>(closeButton->Tag));
        });

        RunOnUIThread([&]() { contentDialog->Hide(); });
        task.wait();
    }

    void ContentDialogIntegrationTests::CanCloseDialogWithGamepadBAndEscape()
    {
        TestCleanupWrapper cleanup;

        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Closed);
        auto closedEvent = std::make_shared<Event>();

        LOG_OUTPUT(L"Creating ContentDialog.");
        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

        RunOnUIThread([&]()
        {
            closedRegistration.Attach(contentDialog, [&]()
            {
                LOG_OUTPUT(L"ContentDialog.Closed event raised.");
                closedEvent->Set();
            });

            LOG_OUTPUT(L"Setting up ContentDialog buttons.");
            contentDialog->PrimaryButtonText = "Primary";
            contentDialog->SecondaryButtonText = "Secondary";
            contentDialog->CloseButtonText = "Close";
        });

        LOG_OUTPUT(L"Showing ContentDialog.");
        auto task = create_task(OpenContentDialog(contentDialog));
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Sending GamepadB keydown to ContentDialog.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_GamepadB");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Expecting ContentDialog.Closed was NOT raised.");
        VERIFY_IS_FALSE(closedEvent->HasFired());

        LOG_OUTPUT(L"Sending GamepadB keyup to ContentDialog.");
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_GamepadB");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Expecting ContentDialog.Closed was raised.");
        VERIFY_IS_TRUE(closedEvent->HasFired());
        closedEvent->Reset();

        LOG_OUTPUT(L"Waiting for ContentDialog closure.");
        task.wait();

        task = create_task(OpenContentDialog(contentDialog));
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Sending Escape keydown to ContentDialog.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_esc");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Expecting ContentDialog.Closed was raised.");
        VERIFY_IS_TRUE(closedEvent->HasFired());

        LOG_OUTPUT(L"Releasing Escape key.");
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_esc");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for ContentDialog closure.");
        task.wait();
    }

    void ContentDialogIntegrationTests::DoesFireButtonCommandsAfterDeferralCompletes()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
        auto customCommand = ref new CustomCommand();
        xaml_controls::ContentDialogButtonClickDeferral^ deferral = nullptr;

        Event buttonClickEvent;
        auto buttonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, PrimaryButtonClick);
        buttonClickRegistration.Attach(contentDialog, ref new wf::TypedEventHandler<xaml_controls::ContentDialog^, xaml_controls::ContentDialogButtonClickEventArgs^>(
            [&](xaml_controls::ContentDialog^ sender, xaml_controls::ContentDialogButtonClickEventArgs^ args)
        {
            deferral = args->GetDeferral();
            buttonClickEvent.Set();
        }));

        Event commandExecutedEvent;
        auto commandExecutedRegistration = CreateSafeEventRegistration(CustomCommand, Executed);
        commandExecutedRegistration.Attach(customCommand, [&]() { commandExecutedEvent.Set(); });

        RunOnUIThread([&]()
        {
            contentDialog->PrimaryButtonText = "Primary";
            contentDialog->PrimaryButtonCommand = customCommand;
        });

        auto task = create_task(OpenContentDialog(contentDialog));

        auto primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
        ControlHelper::DoClickUsingTap<xaml_controls::Button>(primaryButton);
        buttonClickEvent.WaitForDefault();

        // The command should only fire after the deferral completes.
        VERIFY_IS_FALSE(commandExecutedEvent.HasFired());

        // Now complete the deferral, the ContentDialog's show async operation should
        // complete and the command should execute.
        RunOnUIThread([&]() { deferral->Complete(); });

        task.wait();
        commandExecutedEvent.WaitForDefault();
    }

    void ContentDialogIntegrationTests::DoesNotFireButtonCommandsOnCancel()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
        auto customCommand = ref new CustomCommand();
        xaml_controls::ContentDialogButtonClickDeferral^ deferral = nullptr;

        Event buttonClickEvent;
        auto buttonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, PrimaryButtonClick);
        buttonClickRegistration.Attach(contentDialog, ref new wf::TypedEventHandler<xaml_controls::ContentDialog^, xaml_controls::ContentDialogButtonClickEventArgs^>(
            [&](xaml_controls::ContentDialog^ sender, xaml_controls::ContentDialogButtonClickEventArgs^ args)
        {
            args->Cancel = true;
            buttonClickEvent.Set();
        }));

        Event commandExecutedEvent;
        auto commandExecutedRegistration = CreateSafeEventRegistration(CustomCommand, Executed);
        commandExecutedRegistration.Attach(customCommand, [&]() { commandExecutedEvent.Set(); });

        RunOnUIThread([&]()
        {
            contentDialog->PrimaryButtonText = "Primary";
            contentDialog->PrimaryButtonCommand = customCommand;
        });

        auto task = create_task(OpenContentDialog(contentDialog));

        auto primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
        ControlHelper::DoClickUsingTap<xaml_controls::Button>(primaryButton);
        buttonClickEvent.WaitForDefault();

        // The command should not have fired because we canceled the closing event.
        VERIFY_IS_FALSE(commandExecutedEvent.HasFired());

        RunOnUIThread([&]() { contentDialog->Hide(); });
        task.wait();
    }

    void ContentDialogIntegrationTests::DoesNotCrashWhenRestoringFocusToParentlessHyperlink()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

        xaml_controls::StackPanel^ panel = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::TextBlock^ textBlock = nullptr;
        xaml_docs::Hyperlink^ hyperlink = nullptr;

        RunOnUIThread([&]()
        {
            auto run = ref new xaml_docs::Run();
            run->Text = "I'm a hyperlink!";

            hyperlink = ref new xaml_docs::Hyperlink();
            hyperlink->Inlines->Append(run);

            textBlock = ref new xaml_controls::TextBlock();
            textBlock->Inlines->Append(hyperlink);

            button = ref new xaml_controls::Button();
            button->Content = ref new Platform::String(L"I'm a button!");

            panel = ref new xaml_controls::StackPanel();
            panel->Children->Append(button);
            panel->Children->Append(textBlock);

            auto root= safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
            root->Children->Append(panel);

            button->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Since the button should have focus initially, tab to get focus on the hyperlink since
        // we want the ContentDialog to save a reference to this element when it opens so that
        // it can try to return it when it closes.
        TestServices::KeyboardHelper->Tab();
        RunOnUIThread([&]()
        {
            auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
            WEX::Common::Throw::If(!focusedElement->Equals(hyperlink), E_FAIL, L"Hyperlink element is not focused.");
        });

        auto task = create_task(OpenContentDialog(contentDialog));

        // Remove the hyperlink from the TextBlock and remove the TextBlock from the tree to let
        // it get destroyed.
        RunOnUIThread([&]()
        {
            textBlock->Inlines->RemoveAt(0);
            panel->Children->RemoveAt(1);

            textBlock = nullptr;
        });

        // Now close the dialog, which should 'try' to restore focus to the hyperlink
        // element and shouldn't crash while doing so.
        RunOnUIThread([&]() { contentDialog->Hide(); });
        task.wait();
    }

    void ContentDialogIntegrationTests::CanMoveBetweenButtonsWithKeyboard()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Button);
        xaml_controls::Button^ primaryButton = nullptr;
        xaml_controls::Button^ secondaryButton = nullptr;

        auto primaryButtonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto secondaryButtonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        auto secondaryButtonEvent = std::make_shared<Event>();
        auto primaryButtonEvent = std::make_shared<Event>();

        auto task = create_task(OpenContentDialog(contentDialog));

        RunOnUIThread([&]()
        {
            primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
            secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);

            secondaryButtonGotFocusRegistration.Attach(secondaryButton, [secondaryButtonEvent]()
            {
                secondaryButtonEvent->Set();
            });

            primaryButtonGotFocusRegistration.Attach(primaryButton, [primaryButtonEvent]()
            {
                primaryButtonEvent->Set();
            });
        });

        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(primaryButton, FocusState::Keyboard);
        VerifyFocusedElement(primaryButton);

        LOG_OUTPUT(L"Pressing right on keyboard");
        CommonInputHelper::Right(InputDevice::Keyboard);
        secondaryButtonEvent->WaitForDefault();
        VerifyFocusedElement(secondaryButton);

        LOG_OUTPUT(L"Pressing right on keyboard should not wrap around to the primary button");
        primaryButtonEvent->Reset();
        CommonInputHelper::Right(InputDevice::Keyboard);
        primaryButtonEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
        VerifyFocusedElement(secondaryButton);

        LOG_OUTPUT(L"Pressing left on keyboard");
        CommonInputHelper::Left(InputDevice::Keyboard);
        primaryButtonEvent->WaitForDefault();
        VerifyFocusedElement(primaryButton);

        RunOnUIThread([&]() { contentDialog->Hide(); });
        task.wait();
    }

    // Validates that the dialog supports tall BackgroundElement::BorderThickness in the non-content space.
    void ContentDialogIntegrationTests::CanUseTallBackgroundBorderThicknessInNonContentSpace()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::ScrollViewer^ contentScrollViewer = nullptr;

        LOG_OUTPUT(L"Creating ContentDialog.");
        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextPanel);

        LOG_OUTPUT(L"Opening ContentDialog.");
        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            contentScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(contentDialog, L"ContentScrollViewer"));

            LOG_OUTPUT(L"Using a tall BorderThickness for the BackgroundElement part.");
            auto backgroundElement = safe_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"BackgroundElement", contentDialog));
            backgroundElement->BorderThickness = xaml::Thickness({ 1, 275, 1, 275 });
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(contentScrollViewer->ActualHeight, 0.0);
        });

        LOG_OUTPUT(L"Closing ContentDialog.");
        CloseContentDialog(contentDialog);
    }

    // Validates that the dialog supports tall ContentScrollViewer::Margin in the non-content space.
    void ContentDialogIntegrationTests::CanUseTallScrollViewerMarginInNonContentSpace()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::ScrollViewer^ contentScrollViewer = nullptr;

        LOG_OUTPUT(L"Creating ContentDialog.");
        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextPanel);

        LOG_OUTPUT(L"Opening ContentDialog.");
        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Changing ContentDialog to full size.");
            contentDialog->FullSizeDesired = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            contentScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(contentDialog, L"ContentScrollViewer"));

            LOG_OUTPUT(L"Using a tall Margin for the ContentScrollViewer part.");
            contentScrollViewer->Margin = xaml::Thickness({ 1, 275, 1, 275 });
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(contentScrollViewer->ActualHeight, 0.0);
        });

        LOG_OUTPUT(L"Closing ContentDialog.");
        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::DoesEnterInvokeDefaultButtonWithFocusOnNonEnterConsumingContent()
    {
        TestCleanupWrapper cleanup;

        auto runScenario = [](xaml_controls::ContentDialogButton defaultButton)
        {
            auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
            xaml_controls::ContentControl^ doesNotHandleEnterControl = nullptr;

            auto primaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, PrimaryButtonClick);
            auto secondaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, SecondaryButtonClick);
            auto closeButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, CloseButtonClick);

            bool wasPrimaryClicked = false;
            bool wasSecondaryClicked = false;
            bool wasCloseClicked = false;

            RunOnUIThread([&]()
            {
                doesNotHandleEnterControl = ref new xaml_controls::ContentControl();
                doesNotHandleEnterControl->Content = L"does not handle ENTER";
                doesNotHandleEnterControl->IsTabStop = true;
                doesNotHandleEnterControl->UseSystemFocusVisuals = true;
                contentDialog->Content = doesNotHandleEnterControl;

                contentDialog->PrimaryButtonText = "Primary";
                contentDialog->SecondaryButtonText = "Secondary";
                contentDialog->CloseButtonText = "Close";

                contentDialog->DefaultButton = defaultButton;

                primaryButtonClickRegistration.Attach(contentDialog, [&wasPrimaryClicked]() { wasPrimaryClicked = true; });
                secondaryButtonClickRegistration.Attach(contentDialog, [&wasSecondaryClicked]() { wasSecondaryClicked = true; });
                closeButtonClickRegistration.Attach(contentDialog, [&wasCloseClicked]() { wasCloseClicked = true; });

                TestServices::WindowHelper->WindowContent = contentDialog;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto task = create_task(OpenContentDialog(contentDialog));

            // Set focus to our control that does not handle ENTER.
            RunOnUIThread([&]()
            {
                doesNotHandleEnterControl->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            Platform::String^ enterKeySequence = L"$d$_enter#$u$_enter";
            TestServices::KeyboardHelper->PressKeySequence(enterKeySequence);
            TestServices::WindowHelper->WaitForIdle();

            // If no default button is set, then no button should get invoked, so
            // close the ContentDialog manually.
            if (defaultButton == xaml_controls::ContentDialogButton::None)
            {
                RunOnUIThread([&]() { contentDialog->Hide(); });
            }

            // Wait for the content dialog to close.
            task.wait();

            // Validate that the correct button was clicked and the correct result
            // was returned based on the configured default button.
            switch (defaultButton)
            {
            case xaml_controls::ContentDialogButton::None:
                VERIFY_IS_FALSE(wasPrimaryClicked);
                VERIFY_IS_FALSE(wasSecondaryClicked);
                VERIFY_IS_FALSE(wasCloseClicked);
                break;

            case xaml_controls::ContentDialogButton::Primary:
                VERIFY_IS_TRUE(wasPrimaryClicked);
                VERIFY_IS_FALSE(wasSecondaryClicked);
                VERIFY_IS_FALSE(wasCloseClicked);
                VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::Primary, task.get());
                break;

            case xaml_controls::ContentDialogButton::Secondary:
                VERIFY_IS_FALSE(wasPrimaryClicked);
                VERIFY_IS_TRUE(wasSecondaryClicked);
                VERIFY_IS_FALSE(wasCloseClicked);
                VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::Secondary, task.get());
                break;

            case xaml_controls::ContentDialogButton::Close:
                VERIFY_IS_FALSE(wasPrimaryClicked);
                VERIFY_IS_FALSE(wasSecondaryClicked);
                VERIFY_IS_TRUE(wasCloseClicked);
                VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::None, task.get());
                break;
            }
        };

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to None ====");
        runScenario(xaml_controls::ContentDialogButton::None);

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to Primary ====");
        runScenario(xaml_controls::ContentDialogButton::Primary);

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to Secondary ====");
        runScenario(xaml_controls::ContentDialogButton::Secondary);

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to Close ====");
        runScenario(xaml_controls::ContentDialogButton::Close);
    }

    void ContentDialogIntegrationTests::DoesEnterNotInvokeDefaultButtonWithFocusOnEnterConsumingContent()
    {
        TestCleanupWrapper cleanup;

        auto runScenario = [](xaml_controls::ContentDialogButton defaultButton)
        {
            auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
            xaml_controls::Button^ doesHandleEnterControl = nullptr;

            auto primaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, PrimaryButtonClick);
            auto secondaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, SecondaryButtonClick);
            auto closeButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, CloseButtonClick);

            bool wasPrimaryClicked = false;
            bool wasSecondaryClicked = false;
            bool wasCloseClicked = false;

            RunOnUIThread([&]()
            {
                doesHandleEnterControl = ref new xaml_controls::Button();
                doesHandleEnterControl->Content = L"does handle ENTER";
                contentDialog->Content = doesHandleEnterControl;

                contentDialog->PrimaryButtonText = "Primary";
                contentDialog->SecondaryButtonText = "Secondary";
                contentDialog->CloseButtonText = "Close";

                contentDialog->DefaultButton = defaultButton;

                primaryButtonClickRegistration.Attach(contentDialog, [&wasPrimaryClicked]() { wasPrimaryClicked = true; });
                secondaryButtonClickRegistration.Attach(contentDialog, [&wasSecondaryClicked]() { wasSecondaryClicked = true; });
                closeButtonClickRegistration.Attach(contentDialog, [&wasCloseClicked]() { wasCloseClicked = true; });

                TestServices::WindowHelper->WindowContent = contentDialog;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto task = create_task(OpenContentDialog(contentDialog));

            // Set focus to our control that does handle ENTER.
            RunOnUIThread([&]()
            {
                doesHandleEnterControl->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            Platform::String^ enterKeySequence = L"$d$_enter#$u$_enter";
            TestServices::KeyboardHelper->PressKeySequence(enterKeySequence);
            TestServices::WindowHelper->WaitForIdle();

            // The ContentDialog should still be open.
            VERIFY_IS_FALSE(task.is_done());

            // Validate that no buttons were clicked.
            VERIFY_IS_FALSE(wasPrimaryClicked);
            VERIFY_IS_FALSE(wasSecondaryClicked);
            VERIFY_IS_FALSE(wasCloseClicked);

            // Manually close the content dialog.
            RunOnUIThread([&]() { contentDialog->Hide(); });
            task.wait();
        };

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to None ====");
        runScenario(xaml_controls::ContentDialogButton::None);

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to Primary ====");
        runScenario(xaml_controls::ContentDialogButton::Primary);

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to Secondary ====");
        runScenario(xaml_controls::ContentDialogButton::Secondary);

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to Close ====");
        runScenario(xaml_controls::ContentDialogButton::Close);
    }

    void ContentDialogIntegrationTests::DoesEnterNotInvokeDefaultButtonWithFocusOnAnotherCommandButton()
    {
        TestCleanupWrapper cleanup;

        auto runScenario = [](xaml_controls::ContentDialogButton defaultButton)
        {
            auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

            auto primaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, PrimaryButtonClick);
            auto secondaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, SecondaryButtonClick);
            auto closeButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, CloseButtonClick);

            bool wasPrimaryClicked = false;
            bool wasSecondaryClicked = false;
            bool wasCloseClicked = false;

            RunOnUIThread([&]()
            {
                contentDialog->PrimaryButtonText = "Primary";
                contentDialog->SecondaryButtonText = "Secondary";
                contentDialog->CloseButtonText = "Close";

                contentDialog->DefaultButton = defaultButton;

                primaryButtonClickRegistration.Attach(contentDialog, [&wasPrimaryClicked]() { wasPrimaryClicked = true; });
                secondaryButtonClickRegistration.Attach(contentDialog, [&wasSecondaryClicked]() { wasSecondaryClicked = true; });
                closeButtonClickRegistration.Attach(contentDialog, [&wasCloseClicked]() { wasCloseClicked = true; });

                TestServices::WindowHelper->WindowContent = contentDialog;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto task = create_task(OpenContentDialog(contentDialog));

            // Choose a button in the command area to focus that is not the
            // default button.
            auto focusButton = xaml_controls::ContentDialogButton::None;
            RunOnUIThread([&]()
            {
                switch (defaultButton)
                {
                case xaml_controls::ContentDialogButton::Primary:
                    focusButton = xaml_controls::ContentDialogButton::Secondary;
                    break;

                case xaml_controls::ContentDialogButton::Secondary:
                    focusButton = xaml_controls::ContentDialogButton::Close;
                    break;

                case xaml_controls::ContentDialogButton::Close:
                    focusButton = xaml_controls::ContentDialogButton::Primary;
                    break;
                }

                GetButton(contentDialog, focusButton)->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            Platform::String^ enterKeySequence = L"$d$_enter#$u$_enter";
            TestServices::KeyboardHelper->PressKeySequence(enterKeySequence);
            TestServices::WindowHelper->WaitForIdle();

            // Wait for the content dialog to close.
            task.wait();

            // Validate that the correct button was clicked and the correct result
            // was returned based on the button we chose to focus that is not
            // the default button.
            switch (focusButton)
            {
            case xaml_controls::ContentDialogButton::Primary:
                VERIFY_IS_TRUE(wasPrimaryClicked);
                VERIFY_IS_FALSE(wasSecondaryClicked);
                VERIFY_IS_FALSE(wasCloseClicked);
                VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::Primary, task.get());
                break;

            case xaml_controls::ContentDialogButton::Secondary:
                VERIFY_IS_FALSE(wasPrimaryClicked);
                VERIFY_IS_TRUE(wasSecondaryClicked);
                VERIFY_IS_FALSE(wasCloseClicked);
                VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::Secondary, task.get());
                break;

            case xaml_controls::ContentDialogButton::Close:
                VERIFY_IS_FALSE(wasPrimaryClicked);
                VERIFY_IS_FALSE(wasSecondaryClicked);
                VERIFY_IS_TRUE(wasCloseClicked);
                VERIFY_ARE_EQUAL(xaml_controls::ContentDialogResult::None, task.get());
                break;
            }
        };

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to Primary ====");
        runScenario(xaml_controls::ContentDialogButton::Primary);

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to Secondary ====");
        runScenario(xaml_controls::ContentDialogButton::Secondary);

        LOG_OUTPUT(L"==== Run scenario with DefaultButton set to Close ====");
        runScenario(xaml_controls::ContentDialogButton::Close);
    }

    void ContentDialogIntegrationTests::ValidateDefaultButtonStyleAppliedCorrectly()
    {
        TestCleanupWrapper cleanup;

        auto runScenario = [] (xaml_controls::ContentDialogButton defaultButton)
        {
            auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
            xaml_controls::ContentControl^ focusableControl = nullptr;

            // The visual state we'll test for when we expect to be visualizing the default button.
            Platform::String^ defaultButtonIsVisualizedState = L"NoDefaultButton";
            switch (defaultButton)
            {
            case xaml_controls::ContentDialogButton::Primary:
                defaultButtonIsVisualizedState = L"PrimaryAsDefaultButton";
                break;

            case xaml_controls::ContentDialogButton::Secondary:
                defaultButtonIsVisualizedState = L"SecondaryAsDefaultButton";
                break;

            case xaml_controls::ContentDialogButton::Close:
                defaultButtonIsVisualizedState = L"CloseAsDefaultButton";
                break;
            }

            RunOnUIThread([&]()
            {
                contentDialog->PrimaryButtonText = "Primary";
                contentDialog->SecondaryButtonText = "Secondary";
                contentDialog->CloseButtonText = "Close";

                contentDialog->DefaultButton = defaultButton;

                focusableControl = ref new xaml_controls::ContentControl();
                focusableControl->Content = L"I'm some focusable content!";
                focusableControl->IsTabStop = true;
                focusableControl->UseSystemFocusVisuals = true;
                contentDialog->Content = focusableControl;

                TestServices::WindowHelper->WindowContent = contentDialog;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto task = create_task(OpenContentDialog(contentDialog));

            LOG_OUTPUT(L"Case 1: Focus is not in the command area -> default button should be visualized.");
            RunOnUIThread([&]()
            {
                focusableControl->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(contentDialog, L"DefaultButtonStates", defaultButtonIsVisualizedState));

            LOG_OUTPUT(L"Case 2: Focus is in the command area and on the default button -> default button should be visualized.");
            RunOnUIThread([&]()
            {
                GetButton(contentDialog, defaultButton)->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(contentDialog, L"DefaultButtonStates", defaultButtonIsVisualizedState));

            LOG_OUTPUT(L"Case 3: Focus is in the command area but not on the default button -> default button should not be visualized.");
            RunOnUIThread([&]()
            {
                auto focusButton = xaml_controls::ContentDialogButton::None;
                switch (defaultButton)
                {
                case xaml_controls::ContentDialogButton::Primary:
                    focusButton = xaml_controls::ContentDialogButton::Secondary;
                    break;

                case xaml_controls::ContentDialogButton::Secondary:
                    focusButton = xaml_controls::ContentDialogButton::Close;
                    break;

                case xaml_controls::ContentDialogButton::Close:
                    focusButton = xaml_controls::ContentDialogButton::Primary;
                    break;
                }

                GetButton(contentDialog, focusButton)->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(contentDialog, L"DefaultButtonStates", L"NoDefaultButton"));

            RunOnUIThread([&]() { contentDialog->Hide(); });
            task.wait();
        };

        LOG_OUTPUT(L"==== Validate Primary default button style application ====");
        runScenario(xaml_controls::ContentDialogButton::Primary);

        LOG_OUTPUT(L"==== Validate Secondary default button style application ====");
        runScenario(xaml_controls::ContentDialogButton::Secondary);

        LOG_OUTPUT(L"==== Validate Close default button style application ====");
        runScenario(xaml_controls::ContentDialogButton::Close);
    }

    void ContentDialogIntegrationTests::DoesInitiallyFocusFirstFocusableContent()
    {
        auto runScenario = [](xaml_controls::ContentDialogPlacement placement)
        {
            TestCleanupWrapper cleanup;

            auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
            xaml_controls::Button^ focusableContent = nullptr;

            RunOnUIThread([&]()
            {
                contentDialog->PrimaryButtonText = L"Primary";
                contentDialog->SecondaryButtonText = L"Secondary";
                contentDialog->CloseButtonText = L"Close";

                focusableContent = ref new xaml_controls::Button();
                focusableContent->Content = "Focusable Content!";
                contentDialog->Content = focusableContent;

                auto root = ref new xaml_controls::Grid();
                root->Children->Append(contentDialog);

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto task = create_task(OpenContentDialog(contentDialog, placement));

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(focusableContent->Equals(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            });

            RunOnUIThread([&]() { contentDialog->Hide(); });
            task.wait();
        };

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) ====");
        runScenario(xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.InPlace ====");
        runScenario(xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.UnconstrainedPopup ====");
        runScenario(xaml_controls::ContentDialogPlacement::UnconstrainedPopup);
    }

    void ContentDialogIntegrationTests::DoesInitiallyFocusDefaultButton()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

        RunOnUIThread([&]()
        {
            contentDialog->PrimaryButtonText = L"Primary";
            contentDialog->SecondaryButtonText = L"Secondary";
            contentDialog->CloseButtonText = L"Close";

            TestServices::WindowHelper->WindowContent = contentDialog;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto runScenario = [](xaml_controls::ContentDialog^ contentDialog, xaml_controls::ContentDialogButton defaultButton)
        {
            RunOnUIThread([&]() { contentDialog->DefaultButton = defaultButton; });

            auto task = create_task(OpenContentDialog(contentDialog));

            RunOnUIThread([&]()
            {
                auto expectedFocusElement = GetButton(contentDialog, defaultButton);
                VERIFY_IS_TRUE(expectedFocusElement->Equals(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            });

            RunOnUIThread([&]() { contentDialog->Hide(); });
            task.wait();
        };

        LOG_OUTPUT(L"==== Run scenario with Primary as default button ====");
        runScenario(contentDialog, xaml_controls::ContentDialogButton::Primary);

        LOG_OUTPUT(L"==== Run scenario with Secondary as default button ====");
        runScenario(contentDialog, xaml_controls::ContentDialogButton::Secondary);

        LOG_OUTPUT(L"==== Run scenario with Close as default button ====");
        runScenario(contentDialog, xaml_controls::ContentDialogButton::Close);
    }

    void ContentDialogIntegrationTests::DoesInitiallyFocusFirstFocusableCommandButton()
    {
        auto runScenario = [](xaml_controls::ContentDialogPlacement placement)
        {
            TestCleanupWrapper cleanup;

            auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

            RunOnUIThread([&]()
            {
                contentDialog->PrimaryButtonText = L"Primary";
                contentDialog->SecondaryButtonText = L"Secondary";
                contentDialog->CloseButtonText = L"Close";

                auto root = ref new xaml_controls::Grid();
                root->Children->Append(contentDialog);

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() { contentDialog->DefaultButton = xaml_controls::ContentDialogButton::None; });

            LOG_OUTPUT(L"All buttons enabled -> Primary should get focus.");
            auto task = create_task(OpenContentDialog(contentDialog, placement));

            RunOnUIThread([&]()
            {
                auto expectedFocusElement = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
                VERIFY_IS_TRUE(expectedFocusElement->Equals(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            });

            RunOnUIThread([&]() { contentDialog->Hide(); });
            task.wait();

            LOG_OUTPUT(L"Without Primary button enabled -> Secondary should get focus.");

            RunOnUIThread([&]() { contentDialog->PrimaryButtonText = L""; });

            task = create_task(OpenContentDialog(contentDialog, placement));

            RunOnUIThread([&]()
            {
                auto expectedFocusElement = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);
                VERIFY_IS_TRUE(expectedFocusElement->Equals(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            });

            RunOnUIThread([&]() { contentDialog->Hide(); });
            task.wait();

            LOG_OUTPUT(L"Without Primary & Secondary buttons enabled -> Close should get focus.");

            RunOnUIThread([&]() { contentDialog->SecondaryButtonText = L""; });

            task = create_task(OpenContentDialog(contentDialog, placement));

            RunOnUIThread([&]()
            {
                auto expectedFocusElement = GetButton(contentDialog, xaml_controls::ContentDialogButton::Close);
                VERIFY_IS_TRUE(expectedFocusElement->Equals(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            });

            RunOnUIThread([&]() { contentDialog->Hide(); });
            task.wait();
        };

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.Popup (default) ====");
        runScenario(xaml_controls::ContentDialogPlacement::Popup);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.InPlace ====");
        runScenario(xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"==== Validate with ContentDialogPlacement.UnconstrainedPopup ====");
        runScenario(xaml_controls::ContentDialogPlacement::UnconstrainedPopup);
    }

    void ContentDialogIntegrationTests::ButtonsRetainFocusWhenWindowSizeChanges()
    {
        ButtonsRetainFocusWhenWindowSizeChangesWorker();
    }

    void ContentDialogIntegrationTests::ButtonsRetainFocusWhenWindowSizeChangesWorker()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);

        auto buttonClickEvent = std::make_shared<Event>();
        auto secondaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, SecondaryButtonClick);
        RunOnUIThread([&]()
        {
            contentDialog->PrimaryButtonText = L"Primary";
            contentDialog->SecondaryButtonText = L"Secondary";
            contentDialog->CloseButtonText = L"Close";

            auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);

            auto buttonOnPage = ref new xaml_controls::Button();
            buttonOnPage->Content = L"Focusable button on page";
            buttonOnPage->Margin = Thickness({ 10.0, 100.0, 10.0, 10.0 });
            root->Children->Append(buttonOnPage);

            secondaryButtonClickRegistration.Attach(contentDialog,[buttonClickEvent]()
            {
                buttonClickEvent->Set();
            });
        });
        TestServices::WindowHelper->WaitForIdle();

        auto task = create_task(OpenContentDialog(contentDialog));

        LOG_OUTPUT(L"Setting focus to secondary button");
        RunOnUIThread([&]()
        {
            auto secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);
            secondaryButton->Focus(FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Resizing window to 300x300");
        TestServices::WindowHelper->SetDesktopWindowSize(300, 300);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto secondaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Secondary);
            VERIFY_ARE_NOT_EQUAL(secondaryButton->FocusState, FocusState::Unfocused);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]() { contentDialog->Hide(); });
        task.wait();
    }

     void ContentDialogIntegrationTests::CanClickButtonsNotCoveredByDialogWithPlacementInPlace()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentDialog^ contentDialog = nullptr;
        xaml_controls::Button^ button = nullptr;

        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        Event clickEvent;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                        Background="Orange">
                        <Grid.RowDefinitions>
                            <RowDefinition Height="100" />
                            <RowDefinition />
                        </Grid.RowDefinitions>
                        <Button x:Name="button" Content="button" Background="Orange" />
                        <ContentDialog x:Name="contentDialog" Grid.Row="1" />
                    </Grid>)"));

            contentDialog = safe_cast<xaml_controls::ContentDialog^>(root->FindName(L"contentDialog"));
            button = safe_cast<xaml_controls::Button^>(root->FindName(L"button"));

            clickRegistration.Attach(button, [&] () { clickEvent.Set(); });

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenContentDialog(contentDialog, xaml_controls::ContentDialogPlacement::InPlace);

        LOG_OUTPUT(L"Try tapping the button, which should not be occluded by the ContentDialog.");
        ControlHelper::DoClickUsingTap<xaml_controls::Button>(button);
        clickEvent.WaitForDefault();

        LOG_OUTPUT(L"Successfully tapped the button.");
        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::CanCallHideInButtonClickHandler()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);
        auto showAsyncResult = OpenContentDialog(contentDialog);

        auto primaryButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, PrimaryButtonClick);
        primaryButtonClickRegistration.Attach(contentDialog, [&]()
        {
            LOG_OUTPUT(L"Calling ContentDialog.Hide() in button click handler.");
            contentDialog->Hide();
        });

        auto primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
        ControlHelper::DoClickUsingTap<xaml_controls::Button>(primaryButton);

        LOG_OUTPUT(L"Wait for the dialog to close, which should not result in a crash.");
        auto task = create_task(showAsyncResult);
        task.wait();
    }

    void ContentDialogIntegrationTests::CanCallHideInClosedHandler()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::TextContent);
        auto showAsyncResult = OpenContentDialog(contentDialog);

        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Closed);
        closedRegistration.Attach(contentDialog, [&]()
        {
            LOG_OUTPUT(L"Calling ContentDialog.Hide() in the closed event handler.");
            contentDialog->Hide();
        });

        auto primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
        ControlHelper::DoClickUsingTap<xaml_controls::Button>(primaryButton);

        LOG_OUTPUT(L"Wait for the dialog to close, which should not result in a crash.");
        auto task = create_task(showAsyncResult);
        task.wait();
    }

     void ContentDialogIntegrationTests::DoesRestyledXboxInsiderHubDialogStretchHorizontally()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1024, 768));

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Empty);
        auto dialogStyle = safe_cast<xaml::Style^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XboxInsiderHubDialog.xaml"));

        RunOnUIThread([&]()
        {
            contentDialog->Style = dialogStyle;

            // The repro requires the dialog to be in the visual tree.
            TestServices::WindowHelper->WindowContent = contentDialog;
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            auto backgroundElement = safe_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"BackgroundElement", contentDialog));
            auto backgroundElementOffset = backgroundElement->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));

            LOG_OUTPUT(L"BackgroundElement Bounds=(%f, %f, %f, %f)", backgroundElementOffset.X, backgroundElementOffset.Y, backgroundElement->ActualWidth, backgroundElement->ActualHeight);

            // Validate that the dialog stretches to  window width.
            VERIFY_ARE_EQUAL(xaml::Window::Current->Bounds.Width, backgroundElement->ActualWidth);

            // Validate that the dialog is centered.
            VERIFY_ARE_EQUAL((xaml::Window::Current->Bounds.Height - backgroundElement->ActualHeight) * 0.5, backgroundElementOffset.Y);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::CanShowPopupAndInPlaceDialogsAtSameTime()
    {
        TestCleanupWrapper cleanup;

        auto popupDialog = SetupContentDialogTest(ContentDialogContent::Default);
        auto inPlaceDialog = SetupContentDialogTest(ContentDialogContent::Default);

        RunOnUIThread([&]()
        {
            // The InPlace dialog needs to be in the visual tree.
            TestServices::WindowHelper->WindowContent = inPlaceDialog;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open a popup & in-place dialog at the same time, which should not crash.");
        auto popupDialogTask = create_task(OpenContentDialog(popupDialog));
        auto inPlaceDialogTask = create_task(OpenContentDialog(inPlaceDialog, xaml_controls::ContentDialogPlacement::InPlace));

        LOG_OUTPUT(L"Press ESCAPE twice, to close the dialogs.");
        TestServices::KeyboardHelper->Escape();
        TestServices::KeyboardHelper->Escape();

        inPlaceDialogTask.wait();
        popupDialogTask.wait();
    }

    void ContentDialogIntegrationTests::CanShowTwoNonSiblingInPlaceDialogsAtSameTime()
    {
        TestCleanupWrapper cleanup;

        auto dialog1 = SetupContentDialogTest(ContentDialogContent::Default);
        auto dialog2 = SetupContentDialogTest(ContentDialogContent::Default);

        RunOnUIThread([&]()
        {
            auto border1 = ref new xaml_controls::Border();
            border1->Child = dialog1;

            auto border2 = ref new xaml_controls::Border();
            border2->Child = dialog2;

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(border1);
            root->Children->Append(border2);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open both in-place dialogs at the same time, which should not crash.");
        auto task1 = create_task(OpenContentDialog(dialog1, xaml_controls::ContentDialogPlacement::InPlace));
        auto task2 = create_task(OpenContentDialog(dialog2, xaml_controls::ContentDialogPlacement::InPlace));

        LOG_OUTPUT(L"Press ESCAPE twice, to close the dialogs.");
        TestServices::KeyboardHelper->Escape();
        TestServices::KeyboardHelper->Escape();

        task2.wait();
        task1.wait();
    }

    void ContentDialogIntegrationTests::CanNotShowTwoPopupDialogsAtSameTime()
    {
        TestCleanupWrapper cleanup;

        auto dialog1 = SetupContentDialogTest(ContentDialogContent::Default);
        auto dialog2 = SetupContentDialogTest(ContentDialogContent::Default);

        LOG_OUTPUT(L"Try to open both popup dialogs at the same time, which should throw an exception.");
        OpenContentDialog(dialog1);

        RunOnUIThread([&]()
        {
            dialog2->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
            VERIFY_THROWS_SPECIFIC_WINRT(dialog2->ShowAsync(), Platform::Exception^, [](Platform::Exception^ ex) { return ex->HResult == E_ASYNC_OPERATION_NOT_STARTED; });
        });

        CloseContentDialog(dialog1);
    }

    void ContentDialogIntegrationTests::CanNotShowTwoSiblingInPlaceDialogsAtSameTime()
    {
        TestCleanupWrapper cleanup;

        auto dialog1 = SetupContentDialogTest(ContentDialogContent::Default);
        auto dialog2 = SetupContentDialogTest(ContentDialogContent::Default);

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::Grid();
            root->Children->Append(dialog1);
            root->Children->Append(dialog2);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Try to open both in-place dialogs at the same time, which should throw an exception.");
        OpenContentDialog(dialog1, xaml_controls::ContentDialogPlacement::InPlace);

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_SPECIFIC_WINRT(dialog2->ShowAsync(xaml_controls::ContentDialogPlacement::InPlace), Platform::Exception^, [](Platform::Exception^ ex) { return ex->HResult == E_ASYNC_OPERATION_NOT_STARTED; });
        });

        CloseContentDialog(dialog1);
    }

    void ContentDialogIntegrationTests::CanReOpenPopupDialogAsInPlaceDialog()
    {
        TestCleanupWrapper cleanup;

        auto dialog = SetupContentDialogTest(ContentDialogContent::Default);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = dialog;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"First open the dialog as a popup.");
        auto task = create_task(OpenContentDialog(dialog));

        LOG_OUTPUT(L"Press ESCAPE to close the dialog.");
        TestServices::KeyboardHelper->Escape();

        task.wait();

        LOG_OUTPUT(L"Now open the dialog in-place.");
        task = create_task(OpenContentDialog(dialog, xaml_controls::ContentDialogPlacement::InPlace));

        LOG_OUTPUT(L"Press ESCAPE to close the dialog.");
        TestServices::KeyboardHelper->Escape();

        task.wait();
    }

    void ContentDialogIntegrationTests::CanReOpenInPlaceDialogAsPopupDialog()
    {
        TestCleanupWrapper cleanup;

        auto dialog = SetupContentDialogTest(ContentDialogContent::Default);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = dialog;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"First open the dialog in-place.");
        auto task = create_task(OpenContentDialog(dialog, xaml_controls::ContentDialogPlacement::InPlace));

        LOG_OUTPUT(L"Press ESCAPE to close the dialog.");
        TestServices::KeyboardHelper->Escape();

        task.wait();

        LOG_OUTPUT(L"Now open the dialog as a popup.");
        task = create_task(OpenContentDialog(dialog));

        LOG_OUTPUT(L"Press ESCAPE to close the dialog.");
        TestServices::KeyboardHelper->Escape();

        task.wait();
    }

    void ContentDialogIntegrationTests::CanOpenAndCloseWithInvalidTemplate()
    {
        TestCleanupWrapper cleanup;
        auto dialog = SetupContentDialogTest(ContentDialogContent::Default);

        RunOnUIThread([&]()
        {
            auto invalidTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                LR"(<ControlTemplate
                        TargetType="ContentDialog"
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <TextBlock Text="I'm an invalid template!"/>
                    </ControlTemplate>)"));

            dialog->Template = invalidTemplate;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open and then Close the dialog with an invalid template -> it should not crash.");
        OpenContentDialog(dialog);
        CloseContentDialog(dialog);
    }

    void ContentDialogIntegrationTests::CanShowAfterCancelingInSameTick()
    {
        TestCleanupWrapper cleanup;

        auto dialog1 = SetupContentDialogTest(ContentDialogContent::Default);
        auto dialog2 = SetupContentDialogTest(ContentDialogContent::Default);

        RunOnUIThread([&]()
        {
            // Tell dialogs what XamlRoot to use (for WPF-hosting mode)
            dialog1->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
            dialog2->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;

            LOG_OUTPUT(L"Show a dialog and then immediately cancel its async op.");
            auto op1 = dialog1->ShowAsync();
            op1->Cancel();

            LOG_OUTPUT(L"Try to show another dialog within the same tick -> should not crash.");
            auto op2 = dialog2->ShowAsync();
        });

        CloseContentDialog(dialog2);
    }

    void ContentDialogIntegrationTests::ValidateLayoutRootIsNotPutBackInTreeOnClose()
    {
        TestCleanupWrapper cleanup;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Default);

        RunOnUIThread([&]()
        {
            // The LayoutRoot part is only transplanted to the popup if the dialog is already in
            // the visual tree when it is open, so make sure out test dialog is in the tree.
            TestServices::WindowHelper->WindowContent = contentDialog;
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenContentDialog(contentDialog);
        CloseContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"After closing the dialog, validate that the LayoutRoot part is not in the dialog's tree");
            auto layoutRoot = TreeHelper::GetVisualChildByName(contentDialog, L"LayoutRoot");
            VERIFY_IS_NULL(layoutRoot);
        });
    }

    void ContentDialogIntegrationTests::DoesNotCrashWhenOpenWithUnsupportedTemplateAndInputPaneOpens()
    {
        TestCleanupWrapper cleanup;
        XamlRoot^ xamlRoot = nullptr;

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Default);

        RunOnUIThread([&]()
        {
            auto unsupportedTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                LR"(<ControlTemplate
                        TargetType="ContentDialog"
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <TextBlock Text="I'm an invalid template!"/>
                    </ControlTemplate>)"));

            contentDialog->Template = unsupportedTemplate;
            xamlRoot = contentDialog->XamlRoot;
        });

        OpenContentDialog(contentDialog);
        TestServices::WindowHelper->SimulateInputPaneOccludedRect(xamlRoot, wf::Rect(0, 300, 400, 300));
        CloseContentDialog(contentDialog);
    }

    void ContentDialogIntegrationTests::CanUseContentDialogWithSmokeBackgroundPart()
    {
        VerifyContentDialogSmokeBackgroundPart(xaml_controls::ContentDialogPlacement::Popup);
    }

    void ContentDialogIntegrationTests::InPlaceContentDialogWithoutSmokeBackgroundPart()
    {
        VerifyContentDialogSmokeBackgroundPart(xaml_controls::ContentDialogPlacement::InPlace);
    }

    void ContentDialogIntegrationTests::VerifyContentDialogSmokeBackgroundPart(xaml_controls::ContentDialogPlacement placement)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        auto contentDialog = SetupContentDialogTest(ContentDialogContent::Default);
        auto contentDialogStyle = safe_cast<xaml::Style^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ContentDialogWithSmokeBackgroundPart.xaml"));

        RunOnUIThread([&]()
        {
            contentDialog->Style = contentDialogStyle;

            if (placement == xaml_controls::ContentDialogPlacement::InPlace)
            {
                auto root = safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent);
                root->Children->Append(contentDialog);
            }
        });

        auto task = create_task(OpenContentDialog(contentDialog, placement));

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        auto primaryButton = GetButton(contentDialog, xaml_controls::ContentDialogButton::Primary);
        ControlHelper::DoClickUsingTap<xaml_controls::Button>(primaryButton);

        task.wait();
    }

    void ContentDialogIntegrationTests::LoadedAndUnloadedArriveAtTheRightTimes()
    {
        TestCleanupWrapper cleanup;

        auto rootGridLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto contentDialogLoadedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Loaded);
        auto contentDialogUnloadedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Unloaded);
        auto rootGridLoadedEvent = std::make_shared<Event>();
        auto contentDialogLoadedEvent = std::make_shared<Event>();
        auto contentDialogUnloadedEvent = std::make_shared<Event>();

        xaml_controls::ContentDialog^ contentDialog = nullptr;
        auto contentDialogTemplate = safe_cast<xaml_controls::ControlTemplate^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ContentDialogTemplate.xaml"));
        
        RunOnUIThread([&]()
        {
            contentDialog = ref new xaml_controls::ContentDialog();

            // At the time we call ContentDialog.ShowAsync, the default templates have not yet been loaded, and as such,
            // calling ApplyTemplate will not apply the default template and we will apply the template during measure,
            // which will cause an extraneous Unloaded event.  To avoid that and to simulate a custom content dialog,
            // we'll explicitly load and set a ContentDialog template at this time.
            contentDialog->Template = contentDialogTemplate;
        });

        int loadedCount = 0;
        int unloadedCount = 0;

        contentDialogLoadedRegistration.Attach(contentDialog, ref new xaml::RoutedEventHandler([contentDialogLoadedEvent, contentDialog, &loadedCount](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            LOG_OUTPUT(L"ContentDialog loaded.");
            loadedCount++;

            VERIFY_ARE_EQUAL(1, loadedCount);

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(contentDialog->IsLoaded);
            });

            contentDialogLoadedEvent->Set();
        }));

        contentDialogUnloadedRegistration.Attach(contentDialog, ref new xaml::RoutedEventHandler([contentDialogUnloadedEvent, contentDialog, &unloadedCount](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            LOG_OUTPUT(L"ContentDialog.Unloaded event raised.");
            unloadedCount++;

            VERIFY_ARE_EQUAL(1, unloadedCount);

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(contentDialog->IsLoaded);
            });

            contentDialogUnloadedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            auto windowContent = ref new xaml_controls::Grid();
            windowContent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::White);

            rootGridLoadedRegistration.Attach(windowContent, ref new xaml::RoutedEventHandler([rootGridLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                rootGridLoadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = windowContent;
        });

        rootGridLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
        });

        LOG_OUTPUT(L"Opening the ContentDialog.");
        auto task = create_task(OpenContentDialog(contentDialog));
        contentDialogLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Closing the ContentDialog.");
        CloseContentDialog(contentDialog);
        contentDialogUnloadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        task.wait();
    }

    xaml_controls::ContentDialog^ ContentDialogIntegrationTests::SetupContentDialogTest(ContentDialogContent content, xaml_controls::ContentDialogPlacement placement)
    {
        xaml_controls::ContentDialog^ contentDialog = CreateContentDialog(content);
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto loadedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            // We sometimes run into problems with the visual tree if we don't wait for window content to load.
            // To fix that, we'll put a placeholder stack panel as the window content and wait for it to load
            // before continuing with the test.
            auto windowContent = ref new xaml_controls::Grid();
            windowContent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::White);

            loadedRegistration.Attach(windowContent, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = windowContent;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            if (placement != xaml_controls::ContentDialogPlacement::InPlace)
            {
                contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
            }
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        return contentDialog;
    }

    xaml_controls::ContentDialog^ ContentDialogIntegrationTests::CreateContentDialog(ContentDialogContent content)
    {
        xaml_controls::ContentDialog^ contentDialog = nullptr;

        RunOnUIThread([&]()
        {
            contentDialog = ref new xaml_controls::ContentDialog();

            if (content != ContentDialogContent::Empty)
            {
                contentDialog->Title = L"ContentDialog Title";
                contentDialog->PrimaryButtonText = L"OK";
                contentDialog->SecondaryButtonText = L"Cancel";
            }

            if (content == ContentDialogContent::TextBoxContent)
            {
                auto grid = ref new xaml_controls::Grid();
                auto textBox = ref new xaml_controls::TextBox();

                // Having a TextBox wider than the ContentDialog resizes the dialog when
                // the TextBox is tapped, and makes the test click to the left of the button sometimes.
                // To prevent that setting a small MaxWidth value.
                textBox->MaxWidth = 60;
                textBox->Name = L"TB1";
                grid->Children->Append(textBox);
                contentDialog->Content = grid;
            }
            else if (content == ContentDialogContent::ButtonAndTextBox)
            {
                auto stackPanel = ref new xaml_controls::StackPanel();
                auto button1 = ref new xaml_controls::Button();
                auto textBox = ref new xaml_controls::TextBox();

                button1->Content = "Focus me";

                // Having a TextBox wider than the ContentDialog resizes the dialog when
                // the TextBox is tapped, and makes the test click to the left of the button sometimes.
                // To prevent that setting a small MaxWidth value.
                textBox->MaxWidth = 60;
                textBox->Name = L"TB1";

                // Adding the button first means it will get focus before the TextBox will.
                // This ensures that the SIP won't immediately pop up when the ContentDialog is shown.
                stackPanel->Children->Append(button1);
                stackPanel->Children->Append(textBox);
                contentDialog->Content = stackPanel;
            }
            else if (content == ContentDialogContent::MultipleTextBoxContent)
            {
                auto stackPanel = ref new xaml_controls::StackPanel();
                for (size_t i = 0; i < 10; ++i)
                {
                    auto textBox = ref new xaml_controls::TextBox();
                    textBox->Text = L"TextBox" + i;
                    textBox->Name = L"textBox" + i;

                    stackPanel->Children->Append(textBox);
                }
                auto button1 = ref new xaml_controls::Button();
                button1->Content = L"Dismiss SIP";
                button1->Name = L"button1";
                stackPanel->Children->Append(button1);

                contentDialog->Content = stackPanel;
            }
            else if (content == ContentDialogContent::TextPanel)
            {
                contentDialog->Content = L"Lorem ipsum dolor sit amet, consectetur adipisicing elit,  "
                    + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                    + "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
                    + "aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "
                    + "voluptate velit esse cillum dolore eu fugiat nulla pariatur. ";
            }
            else if (content == ContentDialogContent::Button)
            {
                auto button1 = ref new xaml_controls::Button();
                button1->Name = L"btn1";

                button1->Content = L"ContentButton";
                contentDialog->Content = button1;
            }
            else if (content == ContentDialogContent::TallGrid)
            {
                auto grid = ref new xaml_controls::Grid();
                grid->Height = 1000;
                grid->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);

                contentDialog->Content = grid;
            }
            else
            {
                contentDialog->Content = L"ContentDialog Content";
            }
        });

        return contentDialog;
    }

    wf::IAsyncOperation<xaml_controls::ContentDialogResult>^ ContentDialogIntegrationTests::OpenContentDialog(
        xaml_controls::ContentDialog^ contentDialog,
        xaml_controls::ContentDialogPlacement placement
        )
    {
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Opened);
        auto openedEvent = std::make_shared<Event>();

        wf::IAsyncOperation<xaml_controls::ContentDialogResult>^ showAsyncOperation = nullptr;

        openedRegistration.Attach(contentDialog, [&](){ openedEvent->Set(); });

        RunOnUIThread([&]()
        {
            if (placement == xaml_controls::ContentDialogPlacement::Popup)
            {
                contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
            }

            showAsyncOperation = contentDialog->ShowAsync(placement);
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        return showAsyncOperation;
    }

    void ContentDialogIntegrationTests::CloseContentDialog(xaml_controls::ContentDialog^ contentDialog)
    {
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Closed);
        auto closedEvent = std::make_shared<Event>();

        closedRegistration.Attach(contentDialog, [&](){ closedEvent->Set(); });

        RunOnUIThread([&](){ contentDialog->Hide(); });

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    xaml_controls::TextBox^ ContentDialogIntegrationTests::GetTextBoxFromContentDialogContent(xaml_controls::ContentDialog^ contentDialog)
    {
        // Retrieves the TextBox created by CreateContentDialog.

        xaml_controls::TextBox^ textBoxInContentDialog;
        RunOnUIThread([&]()
        {
            textBoxInContentDialog = safe_cast<xaml_controls::TextBox^>(safe_cast<xaml::FrameworkElement^>(contentDialog->Content)->FindName(L"TB1"));
        });
        return textBoxInContentDialog;
    }

    void ContentDialogIntegrationTests::VerifyFocusedElement(Platform::Object^ expectedFocusedElement)
    {
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(expectedFocusedElement));
        });
    }

    xaml_controls::Button^ ContentDialogIntegrationTests::GetButton(xaml_controls::ContentDialog^ contentDialog, xaml_controls::ContentDialogButton buttonType)
    {
        xaml_controls::Button^ button = nullptr;

        RunOnUIThread([&]()
        {
            Platform::String^ buttonName = nullptr;
            switch (buttonType)
            {
            case xaml_controls::ContentDialogButton::Primary:
                buttonName = L"PrimaryButton";
                break;
            case xaml_controls::ContentDialogButton::Secondary:
                buttonName = L"SecondaryButton";
                break;
            case xaml_controls::ContentDialogButton::Close:
                buttonName = L"CloseButton";
                break;
            }

            // First look under the ContentDialog itself.
            button = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(contentDialog, buttonName));

            // Next try looking under the popup root.
            if (button == nullptr)
            {
                button = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByNameFromOpenPopups(buttonName, contentDialog));
            }
        });

        return button;
    }

    Platform::String^ ContentDialogIntegrationTests::GetResourcesPath()
    {
        return GetPackageFolder() + L"resources\\native\\controls\\contentdialog\\";
    }

    bool ContentDialogIntegrationTests::IsInWPFHostingMode()
    {
        WEX::Common::String hostingMode;
        WEX::TestExecution::RuntimeParameters::TryGetValue(L"HostingMode", hostingMode);
        return hostingMode.CompareNoCase(L"WPF") == 0;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ContentDialog
