// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <MUX-ETWEvents.h>
#include "PasswordBoxTests.h"
#include <InputScope.h>
#include <TextInput.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "TreeHelper.h"
#include "ETWWaiterProxy.h"
#include "TextBoxGenericTests.h"
#include <AutomationClient\AutomationGenericTests.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <AutomationClient\AutomationClientManager.h>
#include <WUCRenderingScopeGuard.h>
#include <FocusTestHelper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "KeyboardInjectionOverride.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Text;
using namespace test_infra;
using namespace MockDComp;
using namespace std;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace ::Windows::Storage;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::ApplicationModel;
using namespace ::Windows::ApplicationModel::DataTransfer;
using namespace concurrency;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

    bool PasswordBoxTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PasswordBoxTests::ClassCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    bool PasswordBoxTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PasswordBoxTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        return true;
    }

    Platform::String^ PasswordBoxTests::GetPathToFiles() const
    {
        // Get the deployment directory, and then append our test's directory to the end
        auto deploymentDir = GetTestDeploymentDir();
        return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\text\\");
    }

    //-----------------------------------------------------------------------------
    // Test case: passwordbox behavior when focus is changed  from password box
    //-----------------------------------------------------------------------------
    void PasswordBoxTests::PasswordBoxTestsFocusChange()
    {
        PasswordBoxTestsFocusChangeHelper();
    }

    void PasswordBoxTests::PasswordBoxTestsFocusChangeHelper()
    {
        TestCleanupWrapper cleanup;
        LOG_OUTPUT(L"Test passwordbox behavior when focus is changed.");

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        // focus events
        auto buttonGotFocusEvent = std::make_shared<Event>();
        auto passwordBoxGotFocusEvent = std::make_shared<Event>();

        // password changed events
        auto passwordBoxPasswordChangedEvent = std::make_shared<Event>();

        // event CB registration
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto passwordBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, GotFocus);
        auto passwordBoxPasswordChangedRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanged);

        xaml_controls::PasswordBox^ passwordBox = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_primitives::ButtonBase^ revealButton;

        xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"PasswordBoxTests.xaml"));
        VERIFY_IS_NOT_NULL(rootStackPanel);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootStackPanel;

            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
            VERIFY_IS_NOT_NULL(button);

            buttonGotFocusRegistration.Attach(
                button,
                ref new xaml::RoutedEventHandler(
                [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Button control GotFocus handler.");
                buttonGotFocusEvent->Set();
            }));

            passwordBoxGotFocusRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                [passwordBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"passwordBox control GotFocus handler.");
                passwordBoxGotFocusEvent->Set();
            }));

            passwordBoxPasswordChangedRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                [passwordBoxPasswordChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PasswordChanged event fired on passwordBox!");
                passwordBoxPasswordChangedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Setting focus to the Password control by tapping.");
        TestServices::InputHelper->Tap(passwordBox);
        passwordBoxGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Injecting abc keystrokes as password input.");
        TestServices::KeyboardHelper->PressKeySequence("abc");
        passwordBoxPasswordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // check password value
            VERIFY_IS_TRUE(passwordBox->Password == "abc");
            LOG_OUTPUT(L"Focus the button to defocus from password box.");
            button->Focus(FocusState::Pointer);
        });
        buttonGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Setting focus to the Password control again by tapping.");
        TestServices::InputHelper->Tap(passwordBox);

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Check reveal button visibility.");
        RunOnUIThread([&]()
        {
            revealButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(passwordBox, L"RevealButton"));
            VERIFY_IS_NOT_NULL(revealButton);
            auto visibility = revealButton->Visibility;
            VERIFY_IS_FALSE((visibility == xaml::Visibility::Visible), L"Reveal Button should not be visible after moving focus back to passwordbox!");
        });

        RunOnUIThread([&]()
        {
            passwordBox->Password = ""; // clear password
        });

        passwordBoxPasswordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Injecting abc keystrokes as password input.");
        TestServices::KeyboardHelper->PressKeySequence("abc");

        passwordBoxPasswordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            revealButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(passwordBox, L"RevealButton"));
            VERIFY_IS_NOT_NULL(revealButton);
            auto visibility = revealButton->Visibility;
            VERIFY_IS_TRUE((visibility == xaml::Visibility::Visible), L"Reveal Button should be visible again after starting type from blank!");
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Focus the Button to discard the input pane.");
            button->Focus(FocusState::Pointer);
        });
        buttonGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Prevent those text controls from getting focus again so input pane remains hidden.
            passwordBox->IsEnabled = false;
            LOG_OUTPUT(L"PasswordBox disabled.");
        });

        TestServices::WindowHelper->WaitForIdle();
    }



    //---------------------------------------------------------------------------------------------
    // Test case: passwordbox behavior when PasswordRevelMode is dynamically changed during the test
    //---------------------------------------------------------------------------------------------
    void PasswordBoxTests::PasswordBoxTestsModeChange()
    {
        PasswordBoxTestsModeChangeHelper();
    }

    void PasswordBoxTests::PasswordBoxTestsModeChangeHelper()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        auto rootLoadedEvent = std::make_shared<Event>();
        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

        xaml_controls::PasswordBox^ passwordBox = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"PasswordBoxTests.xaml"));
        VERIFY_IS_NOT_NULL(rootStackPanel);

        RunOnUIThread([&]()
        {
            rootLoadedRegistration.Attach(
                rootStackPanel,
                ref new xaml::RoutedEventHandler(
                [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Root loaded handler.");
                rootLoadedEvent->Set();
            }));

            button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
            VERIFY_IS_NOT_NULL(button);
            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Hidden;
            passwordBox->Password = L"abc";

            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });

        LOG_OUTPUT(L"Waiting for root loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Dcomp dump comparison to make sure password is not visible.");
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Switch PasswordRevealMode to Visible.");
            passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Visible;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Dcomp dump comparison to make sure password is now visible.");
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Switch PasswordRevealMode to Hidden and change PasswordChar to %%.");
            passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Hidden;
            passwordBox->Password = "";
            passwordBox->MaxLength = 2;
            passwordBox->PasswordChar = "%";
        });

        TestServices::WindowHelper->WaitForIdle();
        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Pointer);
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->PressKeySequence("abc");
        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(button, FocusState::Pointer);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Resetting surface id.");
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
        TestServices::Utilities->ResetMockDCompSurfaceId();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Dcomp dump comparison to make sure password char is displayed.");
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"3");

        RunOnUIThread([&]()
        {
            passwordBox->IsEnabled = false;
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    //--------------------------------------------------------------------------
    // Test case: passwordbox behavior when PasswordRevealMode is set to Peek
    //--------------------------------------------------------------------------
    void PasswordBoxTests::PasswordBoxTestsPeek()
    {

        TestCleanupWrapper cleanup;
        PasswordBoxTestHelper(RevealModePeek, TRUE);
    }

    //--------------------------------------------------------------------------
    // Test case: passwordbox behavior when PasswordRevealMode is set to Hidden
    //--------------------------------------------------------------------------
    void PasswordBoxTests::PasswordBoxTestsHidden()
    {

        TestCleanupWrapper cleanup;
        PasswordBoxTestHelper(RevealModeHidden, FALSE);
    }

    //--------------------------------------------------------------------------
    // Test case: passwordbox behavior when PasswordRevealMode is set to Visible
    //--------------------------------------------------------------------------
    void PasswordBoxTests::PasswordBoxTestsVisible()
    {
        TestCleanupWrapper cleanup;
        PasswordBoxTestHelper(RevealModeVisible, FALSE);
    }

    //----------------------------------------------------------------------------------
    // Test case: passwordbox behavior when IsPasswordRevealButtonEnabled is set to true
    //----------------------------------------------------------------------------------
    void PasswordBoxTests::PasswordBoxTestsButtonEnabled()
    {
        TestCleanupWrapper cleanup;
        PasswordBoxTestHelper(ButtonEnabled, TRUE);
    }

    //----------------------------------------------------------------------------------
    // Test case: passwordbox behavior when IsPasswordRevealButtonEnabled is set to false
    //----------------------------------------------------------------------------------
    void PasswordBoxTests::PasswordBoxTestsButtonDisabled()
    {
        TestCleanupWrapper cleanup;
        PasswordBoxTestHelper(ButtonDisabled, FALSE);
    }

    //----------------------------------------------------------------------------------
    // Test case: passwordbox behavior when on XBOX
    //----------------------------------------------------------------------------------
    void PasswordBoxTests::EnsureNoRevealButtonOnXBox()
    {
        TestCleanupWrapper cleanup;
        PasswordBoxTestHelper(ButtonEnabled, FALSE);
    }


    //------------------------------------------
    // Helper function for testing password box behavior
    //------------------------------------------
    void PasswordBoxTests::PasswordBoxTestHelper(PasswordBoxTestSetting testSetting, bool peekButtonVisible)
    {

        LPCWSTR testSettingStr;
        switch(testSetting)
        {
            case RevealModePeek:
                testSettingStr = L"PasswordRevealMode is set to Peek";
                break;
            case RevealModeHidden:
                testSettingStr = L"PasswordRevealMode is set to Hidden";
                break;
            case RevealModeVisible:
                testSettingStr = L"PasswordRevealMode is set to Visible";
                break;
            case ButtonEnabled:
                testSettingStr = L"IsPasswordRevealButtonEnabled is set to True";
                break;
            case ButtonDisabled:
                testSettingStr = L"IsPasswordRevealButtonEnabled is set to False";
                break;
            default:
                testSettingStr = L"Unknown test setting";
                break;
        }

        LOG_OUTPUT(L"Test passwordbox behavior: %ws.", testSettingStr);

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        // focus events
        auto buttonGotFocusEvent = std::make_shared<Event>();
        auto passwordBoxGotFocusEvent = std::make_shared<Event>();

        // text changed events
        auto passwordBoxPasswordChangedEvent = std::make_shared<Event>();

        // event CB registration
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto passwordBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, GotFocus);
        auto passwordBoxPasswordChangedRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanged);

        xaml_controls::PasswordBox^ passwordBox;
        xaml_controls::Button^ button = nullptr;
        xaml_primitives::ButtonBase^ revealButton;

        xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"PasswordBoxTests.xaml"));
        VERIFY_IS_NOT_NULL(rootStackPanel);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootStackPanel;
            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            switch(testSetting)
            {
                case RevealModePeek:
                    passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Peek;
                    break;
                case RevealModeHidden:
                    passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Hidden;
                    break;
                case RevealModeVisible:
                    passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Visible;
                    break;
                case ButtonEnabled:
                    passwordBox->IsPasswordRevealButtonEnabled = true;
                    break;
                case ButtonDisabled:
                    passwordBox->IsPasswordRevealButtonEnabled = false;
                    break;
            }

            button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
            VERIFY_IS_NOT_NULL(button);

            buttonGotFocusRegistration.Attach(
                button,
                ref new xaml::RoutedEventHandler(
                [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Button control GotFocus handler.");
                buttonGotFocusEvent->Set();
            }));

            passwordBoxGotFocusRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                [passwordBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"passwordBox control GotFocus handler.");
                passwordBoxGotFocusEvent->Set();
            }));

            passwordBoxPasswordChangedRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                [passwordBoxPasswordChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PasswordChanged event fired on passwordBox!");
                passwordBoxPasswordChangedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Setting focus to the Password control by tapping.");
        TestServices::InputHelper->Tap(passwordBox);

        passwordBoxGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Injecting abc keystrokes as password input.");
        TestServices::KeyboardHelper->PressKeySequence("abc");

        passwordBoxPasswordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Check reveal button visibility.");
        RunOnUIThread([&]()
        {
            revealButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(passwordBox, L"RevealButton"));
            VERIFY_IS_NOT_NULL(revealButton);
            auto visibility = revealButton->Visibility;
            if (visibility == xaml::Visibility::Visible)
            {
                VERIFY_IS_TRUE(peekButtonVisible, L"Reveal Button should be visible!");
            }
            else
            {
                VERIFY_IS_TRUE(!peekButtonVisible, L"Reveal Button should not be visible!");
            }
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Focus the button to defocus from password box.");
            button->Focus(FocusState::Pointer);
        });

        buttonGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL((Platform::String^)"abc", passwordBox->Password);
            passwordBox->IsEnabled = false;
            LOG_OUTPUT(L"PasswordBox disabled.");
        });

        TestServices::WindowHelper->WaitForIdle();

    }

    void PasswordBoxTests::PasswordBoxTestsInputScope()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::PasswordBox^ passwordBox;
        xaml_controls::Button^ button = nullptr;

        xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"PasswordBoxTests.xaml"));
        VERIFY_IS_NOT_NULL(rootStackPanel);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootStackPanel;
            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);
            button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
            VERIFY_IS_NOT_NULL(button);
         });

        TestServices::WindowHelper->WaitForIdle();

        ETWWaiterProxy etwWaiter;

        // Set input scope to NumericPin and check the ETW event to make sure correct IKS mode is set
        {
            Platform::String^ etwValidationString = GetEtwFilterString(L"IKSkinValue", static_cast<int>(::TextInput_KeyboardSkins::Bitlocker));
            etwWaiter.Start(
                WINDOWS_UI_XAML_ETW_PROVIDER,
                SetIhmKeyboardSkinInfo_value,
                etwValidationString);
        }

        RunOnUIThread([&]()
        {
            Xaml::Input::InputScope ^scope = ref new Xaml::Input::InputScope();
            Xaml::Input::InputScopeName ^scopeName = ref new Xaml::Input::InputScopeName(Xaml::Input::InputScopeNameValue::NumericPin);
            scope->Names->Append(scopeName);
            passwordBox->InputScope = scope;
        });

        TestServices::WindowHelper->WaitForIdle();
        VERIFY_NO_THROW(etwWaiter.WaitForDefault());
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(passwordBox->InputScope->Names->GetAt(0)->NameValue, Xaml::Input::InputScopeNameValue::NumericPin);
        });

        // Set input scope to Password and check the ETW event to make sure correct IKS mode is set
        {
            Platform::String^ etwValidationString = GetEtwFilterString(L"IKSkinValue", static_cast<int>(::TextInput_KeyboardSkins::Default));
            etwWaiter.Start(
                WINDOWS_UI_XAML_ETW_PROVIDER,
                SetIhmKeyboardSkinInfo_value,
                etwValidationString);
        }

        RunOnUIThread([&]()
        {
            Xaml::Input::InputScope ^scope = ref new Xaml::Input::InputScope();
            Xaml::Input::InputScopeName ^scopeName = ref new Xaml::Input::InputScopeName(Xaml::Input::InputScopeNameValue::Password);
            scope->Names->Append(scopeName);
            passwordBox->InputScope = scope;
        });

        TestServices::WindowHelper->WaitForIdle();
        VERIFY_NO_THROW(etwWaiter.WaitForDefault());
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(passwordBox->InputScope->Names->GetAt(0)->NameValue, Xaml::Input::InputScopeNameValue::Password);
        });

        // Setting inputscope to anything else other than password and numericpin should throw InvalidArg exception
        RunOnUIThread([&]()
        {
            Xaml::Input::InputScope ^scope = ref new Xaml::Input::InputScope();
            Xaml::Input::InputScopeName ^scopeName = ref new Xaml::Input::InputScopeName(Xaml::Input::InputScopeNameValue::Number);
            scope->Names->Append(scopeName);
            VERIFY_THROWS_WINRT(
                (passwordBox->InputScope = scope),
                Platform::InvalidArgumentException^,
                L"Exception thrown by setting invalid inputscope on passwordbox.");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            passwordBox->IsEnabled = false; // disable to make sure it won't accidentally getting focus and cause SIP to open
            LOG_OUTPUT(L"PasswordBox disabled.");
        });

        TestServices::WindowHelper->WaitForIdle();

    }

    //-----------------------------------------------------------------------------
    // Test case: passwordbox behavior when reveal button is right clicked
    //-----------------------------------------------------------------------------
    void PasswordBoxTests::PasswordBoxTestsNoContextMenuOnButtonClick()
    {
        TestCleanupWrapper cleanup;
        LOG_OUTPUT(L"Test passwordbox behavior reveal button is right clicked, text context menu should not show.");

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        // focus events
        auto buttonGotFocusEvent = std::make_shared<Event>();
        auto passwordBoxGotFocusEvent = std::make_shared<Event>();

        // password changed events
        auto passwordBoxPasswordChangedEvent = std::make_shared<Event>();

        auto contextMenuOpeningEvent = std::make_shared<Event>();

        // event CB registration
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto passwordBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, GotFocus);
        auto passwordBoxPasswordChangedRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanged);
        auto contextMenuOpeningRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, ContextMenuOpening);

        xaml_controls::PasswordBox^ passwordBox = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_primitives::ToggleButton^ revealButton;

        xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"PasswordBoxTests.xaml"));
        VERIFY_IS_NOT_NULL(rootStackPanel);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootStackPanel;

            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
            VERIFY_IS_NOT_NULL(button);

            buttonGotFocusRegistration.Attach(
                button,
                ref new xaml::RoutedEventHandler(
                [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Button control GotFocus handler.");
                buttonGotFocusEvent->Set();
            }));

            passwordBoxGotFocusRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                [passwordBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"passwordBox control GotFocus handler.");
                passwordBoxGotFocusEvent->Set();
            }));

            passwordBoxPasswordChangedRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                [passwordBoxPasswordChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PasswordChanged event fired on passwordBox!");
                passwordBoxPasswordChangedEvent->Set();
            }));

            contextMenuOpeningRegistration.Attach(
                passwordBox,
                ref new xaml_controls::ContextMenuOpeningEventHandler(
                [contextMenuOpeningEvent](Platform::Object^, xaml_controls::ContextMenuEventArgs^)
            {
                LOG_OUTPUT(L"ContextMenuOpening event fired on passwordBox!");
                contextMenuOpeningEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Setting focus to the Password control by tapping.");
        TestServices::InputHelper->Tap(passwordBox);
        passwordBoxGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Injecting abc keystrokes as password input.");
        TestServices::KeyboardHelper->PressKeySequence("abc");
        passwordBoxPasswordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            revealButton = safe_cast<xaml_primitives::ToggleButton^>(TreeHelper::GetVisualChildByName(passwordBox, L"RevealButton"));
            VERIFY_IS_NOT_NULL(revealButton);
            auto visibility = revealButton->Visibility;
            VERIFY_IS_TRUE((visibility == xaml::Visibility::Visible), L"Reveal Button should be visible after starting typing from blank!");
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Injecting right click on password reveal button.");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, revealButton);
        TestServices::WindowHelper->WaitForIdle();

        // context menu should not fire on passwordbox reveal button right click
        contextMenuOpeningEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
        VERIFY_IS_FALSE(contextMenuOpeningEvent->HasFired());

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Focus the Button to discard the input pane.");
            button->Focus(FocusState::Pointer);
        });
        buttonGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Prevent those text controls from getting focus again so input pane remains hidden.
            passwordBox->IsEnabled = false;
            LOG_OUTPUT(L"PasswordBox disabled.");
        });

        TestServices::WindowHelper->WaitForIdle();
    }


    Platform::String^ PasswordBoxTests::GetEtwFilterString( Platform::String^ field, unsigned int inputScope)
    {
        Platform::String^ etwFilterString = L"@" + field + L"=" + inputScope;
        return etwFilterString;
    }

    void PasswordBoxTests::CheckFiresHoldingEvent()
    {
        TestCleanupWrapper cleanup;

        auto holdEvent = std::make_shared<Event>();
        auto holdRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, Holding);

        auto contextMenuOpeningEvent = std::make_shared<Event>();
        auto contextMenuOpeningRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, ContextMenuOpening);

        xaml_controls::PasswordBox^ passwordBox = nullptr;
        RunOnUIThread([&]()
        {
            wf::Rect visibleBounds = TestServices::WindowHelper->VisibleBounds;

            passwordBox = ref new xaml_controls::PasswordBox();
            passwordBox->Password = L"Something here"; // at least context menu will fire for "Select All" option
            passwordBox->Margin = Thickness({visibleBounds.X, visibleBounds.Y, 0, 0});

            holdRegistration.Attach(passwordBox, [&]()
            {
                LOG_OUTPUT(L"PasswordBox holding event fired.");
                holdEvent->Set();
            });
            contextMenuOpeningRegistration.Attach(
                passwordBox,
                ref new xaml_controls::ContextMenuOpeningEventHandler (
                    [contextMenuOpeningEvent](Platform::Object^ sender, xaml_controls::ContextMenuEventArgs^ args)
            {
                LOG_OUTPUT(L"PasswordBox ContextMenuOpeing event fired.");
                // Surpress the context menu from drawing to prevent visual artifacts for next tests.  Test cleanup wrapped does not clean this up
                contextMenuOpeningEvent->Set();
                args->Handled = true;
            }));

            TestServices::WindowHelper->WindowContent = passwordBox;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Touch hold on the password box
        TestServices::InputHelper->Hold(passwordBox);
        holdEvent->WaitForDefault();
        contextMenuOpeningEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

    }

    void PasswordBoxTests::CheckFiresManipulationEvents()
    {
        TestCleanupWrapper cleanup;
        TextBoxGenericTests<xaml_controls::PasswordBox>::CanFireManipulationEvents();
    }

    void PasswordBoxTests::ValidatePasswordCharValidation()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::PasswordBox ^ passwordBox = nullptr;

        RunOnUIThread([&]()
        {
            passwordBox = ref new xaml_controls::PasswordBox();
            TestServices::WindowHelper->WindowContent = passwordBox;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(passwordBox->PasswordChar == L"\u25CF");  // Test the default value of a black circle is the unicode character 0x25cf
            VERIFY_THROWS_WINRT(passwordBox->PasswordChar = "\0", Platform::Exception^, L"An exception should be thrown if Password Char is the null terminator");
            VERIFY_THROWS_WINRT(passwordBox->PasswordChar = "", Platform::Exception^, L"An exception should be thrown if Password Char is an empty string");
            VERIFY_THROWS_WINRT(passwordBox->PasswordChar = "HE", Platform::Exception^, L"An exception should be thrown if Password Char is a string");
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidateRevealButtonToggle()
    {
        RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        // focus events
        auto buttonGotFocusEvent = std::make_shared<Event>();
        auto passwordBoxGotFocusEvent = std::make_shared<Event>();

        // password changed events
        auto passwordBoxPasswordChangedEvent = std::make_shared<Event>();
        auto revealButtonChcked = std::make_shared<Event>();

        // event CB registration
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto passwordBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, GotFocus);
        auto passwordBoxPasswordChangedRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanged);
        auto revealButtonChckedRegistration = CreateSafeEventRegistration(xaml_primitives::ToggleButton, Checked);

        xaml_controls::PasswordBox^ passwordBox = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_primitives::ToggleButton^ revealButton;

        xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"PasswordBoxTests.xaml"));
        VERIFY_IS_NOT_NULL(rootStackPanel);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootStackPanel;

            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
            VERIFY_IS_NOT_NULL(button);

            buttonGotFocusRegistration.Attach(
                button,
                ref new xaml::RoutedEventHandler(
                    [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Button control GotFocus handler.");
                buttonGotFocusEvent->Set();
            }));

            passwordBoxGotFocusRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                    [passwordBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"passwordBox control GotFocus handler.");
                passwordBoxGotFocusEvent->Set();
            }));

            passwordBoxPasswordChangedRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                    [passwordBoxPasswordChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PasswordChanged event fired on passwordBox!");
                passwordBoxPasswordChangedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Setting focus to the Password control by tapping.");
        TestServices::InputHelper->Tap(passwordBox);
        passwordBoxGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Injecting abc keystrokes as password input.");
        TestServices::KeyboardHelper->PressKeySequence("abc");
        passwordBoxPasswordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Check reveal button visibility.");
        RunOnUIThread([&]()
        {
            revealButton = safe_cast<xaml_primitives::ToggleButton^>(TreeHelper::GetVisualChildByName(passwordBox, L"RevealButton"));
            VERIFY_IS_NOT_NULL(revealButton);
            auto visibility = revealButton->Visibility;
            VERIFY_IS_TRUE((visibility == xaml::Visibility::Visible), L"Reveal Button should be visible after typing from blank!");
            revealButtonChckedRegistration.Attach(
                revealButton,
                ref new xaml::RoutedEventHandler(
                    [revealButtonChcked](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Reveal button checked handler.");
                revealButtonChcked->Set();
            }));
        });

        xaml_automation_peers::ToggleButtonAutomationPeer^ revealButtonAP;

        RunOnUIThread([&]()
        {
            revealButtonAP = safe_cast<xaml_automation_peers::ToggleButtonAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(revealButton));
            VERIFY_IS_NOT_NULL(revealButtonAP);
            revealButtonAP->Toggle();
        });
        revealButtonChcked->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Dcomp dump comparison to make sure password is visible.");
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
        TestServices::Utilities->ResetMockDCompSurfaceId();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Injecting additiona key should hide password.");
        passwordBoxPasswordChangedEvent->Reset();
        TestServices::KeyboardHelper->PressKeySequence("d");
        passwordBoxPasswordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::WindowHelper->SynchronouslyTickUIThread(4);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");

        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidateKeyboardShortCutAndValuePattern()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        TestCleanupWrapper cleanup;
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        // focus events
        auto buttonGotFocusEvent = std::make_shared<Event>();
        auto passwordBoxGotFocusEvent = std::make_shared<Event>();

        // password changed events
        auto passwordBoxPasswordChangedEvent = std::make_shared<Event>();
        auto revealButtonChcked = std::make_shared<Event>();

        // event CB registration
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto passwordBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, GotFocus);
        auto passwordBoxPasswordChangedRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanged);

        xaml_controls::PasswordBox^ passwordBox = nullptr;
        xaml_controls::Button^ button = nullptr;

        int passwordBoxExpectedLength = 0;

        xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"PasswordBoxTests.xaml"));
        VERIFY_IS_NOT_NULL(rootStackPanel);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootStackPanel;

            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
            VERIFY_IS_NOT_NULL(button);

            buttonGotFocusRegistration.Attach(
                button,
                ref new xaml::RoutedEventHandler(
                    [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Button control GotFocus handler.");
                buttonGotFocusEvent->Set();
            }));

            passwordBoxGotFocusRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                    [passwordBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"passwordBox control GotFocus handler.");
                passwordBoxGotFocusEvent->Set();
            }));
            passwordBoxPasswordChangedRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                    [passwordBoxPasswordChangedEvent, passwordBox, &passwordBoxExpectedLength](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                int currentLength = passwordBox->Password->Length();
                LOG_OUTPUT(L"PasswordChanged event fired on passwordBox with %d of %d expected characters.", currentLength, passwordBoxExpectedLength);

                // Because of the inherent asynchronousness of input simply waiting for idle may not be sufficient to wait for all
                // characters to be input.  It is possible that between injected keystrokes, Xaml may recognize itself as idle.
                // To get around this, we won't set the Changed event until we reach the desired length.
                if (currentLength >= passwordBoxExpectedLength)
                {
                    passwordBoxPasswordChangedEvent->Set();
                }
            }));

            passwordBox->PasswordChar = "%";
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Setting focus to the Password control by tapping.");
        TestServices::InputHelper->Tap(passwordBox);
        passwordBoxGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Injecting abc keystrokes as password input.");
        passwordBoxExpectedLength = 3;
        TestServices::KeyboardHelper->PressKeySequence("abc");
        passwordBoxPasswordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(passwordBox, ref new Platform::String(L"PasswordBox"));
        });
        TestServices::WindowHelper->WaitForIdle();
        VerifyPasswordUsingUIAValuePattern(passwordBox, L"%%%");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing ALT+F8 to reveal password");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_f8");
        TestServices::WindowHelper->WaitForIdle();
        VerifyPasswordUsingUIAValuePattern(passwordBox, L"abc");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Releasing ALT+F8 to hide password");
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_f8#$u$_alt");
        TestServices::WindowHelper->WaitForIdle();
        VerifyPasswordUsingUIAValuePattern(passwordBox, L"%%%");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"PasswordBox reveal mode changed to hidden");
            passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Hidden;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Pressing ALT+F8 to reveal password");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_f8");
        TestServices::WindowHelper->WaitForIdle();
        VerifyPasswordUsingUIAValuePattern(passwordBox, L"%%%");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Releasing ALT+F8 to hide password");
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_f8#$u$_alt");
        TestServices::WindowHelper->WaitForIdle();
        VerifyPasswordUsingUIAValuePattern(passwordBox, L"%%%");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"PasswordBox reveal mode changed to visible");
            passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Visible;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Pressing ALT+F8 to reveal password");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_f8");
        TestServices::WindowHelper->WaitForIdle();
        VerifyPasswordUsingUIAValuePattern(passwordBox, L"abc");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Releasing ALT+F8 to hide password");
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_f8#$u$_alt");
        TestServices::WindowHelper->WaitForIdle();
        VerifyPasswordUsingUIAValuePattern(passwordBox, L"abc");

        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidatePasswordTextPatternHidden()
    {
        ValidatePasswordTextPatternTestHelper(false);
    }

    void PasswordBoxTests::ValidatePasswordTextPatternRevealed()
    {
        ValidatePasswordTextPatternTestHelper(true);
    }

    void PasswordBoxTests::ValidatePasswordTextPatternTestHelper(bool passwordRevealed)
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::PasswordBox^ passwordBox = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"PasswordBox";
        uiaInfo.m_AutomationID = L"PasswordBox";
        uiaInfo.m_cType = UIA_TextControlTypeId;

        xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"PasswordBoxTests.xaml"));
        VERIFY_IS_NOT_NULL(rootStackPanel);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootStackPanel;
            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);
            passwordBox->PasswordChar = "%";
            if (passwordRevealed)
            {
                passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Visible;
            }
            LOG_OUTPUT(L"Setting Password to \"Hello World\".");
            passwordBox->Password = L"Hello World";
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(passwordBox, ref new Platform::String(uiaInfo.m_Name));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Setting focus to the Password control.");
        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            xaml_automation_peers::AutomationPeer^ passwordBoxAP;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRangeFound;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            wrl::ComPtr<IUIAutomationTextPattern2> spUITextPattern2;
            wrl::ComPtr<IUIAutomationTextRangeArray> spUITextArrangeArray;
            AutoBSTR textFromTextRange;

            RunOnUIThread([&]()
            {
                passwordBoxAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(passwordBox);
            });
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());
            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern));
            VERIFY_IS_NOT_NULL(spTextPattern.Get());

            LOG_OUTPUT(L"Test DocumentRange");
            VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            if (passwordRevealed)
            {
                VERIFY_IS_TRUE(!wcscmp(L"Hello World", textFromTextRange));
            }
            else
            {
                VERIFY_IS_TRUE(!wcscmp(L"%%%%%%%%%%%", textFromTextRange));
            }

            LOG_OUTPUT(L"Test FindText");
            AutoBSTR findBstr(L"world");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->FindText(findBstr, FALSE /*backward */, TRUE /*ignoreCase*/, spUIAutomationTextRangeFound.ReleaseAndGetAddressOf()));
            if (passwordRevealed)
            {
                VERIFY_IS_NOT_NULL(spUIAutomationTextRangeFound.Get());
            }
            else
            {
                LOG_OUTPUT(L"Password hidden should not support FindText");
                VERIFY_IS_NULL(spUIAutomationTextRangeFound.Get());
            }

            VERIFY_SUCCEEDED(spUIAutomationTextRange->FindText(findBstr, FALSE /*backward */, FALSE /*ignoreCase*/, spUIAutomationTextRangeFound.ReleaseAndGetAddressOf()));
            VERIFY_IS_NULL(spUIAutomationTextRangeFound.Get());

            LOG_OUTPUT(L"Test GetSelection");
            VERIFY_SUCCEEDED(spTextPattern->GetSelection(spUITextArrangeArray.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUITextArrangeArray.Get());
            VERIFY_SUCCEEDED(spUITextArrangeArray->GetElement(0, spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            if (textFromTextRange) // can return either blank string or NULL string
            {
                VERIFY_IS_TRUE(!wcscmp(L"", textFromTextRange));
            }
            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Line));
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            if (passwordRevealed)
            {
                VERIFY_IS_TRUE(!wcscmp(L"Hello World", textFromTextRange));
            }
            else
            {
                VERIFY_IS_TRUE(!wcscmp(L"%%%%%%%%%%%", textFromTextRange));
            }

            LOG_OUTPUT(L"Test RangeFromPoint");
            POINT point = { 50, 50 };
            VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Line));
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            if (passwordRevealed)
            {
                VERIFY_IS_TRUE(!wcscmp(L"Hello World", textFromTextRange));
            }
            else
            {
                VERIFY_IS_TRUE(!wcscmp(L"%%%%%%%%%%%", textFromTextRange));
            }

            LOG_OUTPUT(L"Test TextPattern2");
            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPattern2Id, __uuidof(IUIAutomationTextPattern2), &spUITextPattern2));
            VERIFY_IS_NOT_NULL(spUITextPattern2.Get());
            BOOL isActive = FALSE;
            VERIFY_SUCCEEDED(spUITextPattern2->GetCaretRange(&isActive, spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(!!isActive);
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            BOOL moved = FALSE;
            LOG_OUTPUT(L"Test MoveEndpointByUnit, move the start CP to end of line");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Line, 1, &moved));
            VERIFY_IS_TRUE(!!moved);

            LOG_OUTPUT(L"Test MoveEndpointByUnit, move the start CP one char before end of line");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, -1, &moved));
            VERIFY_IS_TRUE(!!moved);

            // Expansion test a character.
            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Character));
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            if (passwordRevealed)
            {
                VERIFY_IS_TRUE(!wcscmp(L"d", textFromTextRange));
            }
            else
            {
                VERIFY_IS_TRUE(!wcscmp(L"%", textFromTextRange));
            }

            LOG_OUTPUT(L"Test Move(), it should not move with positive count since it is at the end of Text");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Character, 1, &moved));
            VERIFY_IS_FALSE(!!moved);

            LOG_OUTPUT(L"Test Move(), it should move with Negative count");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Character, -11, &moved));
            VERIFY_IS_TRUE(!!moved);
            LOG_OUTPUT(L"Expanding the range, it should include the first char of \"Hello World\"");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Character));
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            if (passwordRevealed)
            {
                VERIFY_IS_TRUE(!wcscmp(L"H", textFromTextRange));
            }
            else
            {
                VERIFY_IS_TRUE(!wcscmp(L"%", textFromTextRange));
            }

            LOG_OUTPUT(L"Test Move(), it should not move with negative count since it is at the begining of Text");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Character, -1, &moved));
            VERIFY_IS_FALSE(!!moved);

            LOG_OUTPUT(L"Test Move(), it should move with positive count since it is at the begining of Text");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Character, 1, &moved));
            VERIFY_IS_TRUE(!!moved);
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            if (passwordRevealed)
            {
                VERIFY_IS_TRUE(!wcscmp(L"e", textFromTextRange));
            }
            else
            {
                VERIFY_IS_TRUE(!wcscmp(L"%", textFromTextRange));
            }

            LOG_OUTPUT(L"Test Select()");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Select());

        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidateNoRevealButton()
    {
        RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        // password changed events
        auto passwordBoxPasswordChangedEvent = std::make_shared<Event>();
        // event CB registration
        auto passwordBoxPasswordChangedRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanged);

        xaml_controls::PasswordBox^ passwordBox = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_primitives::ToggleButton^ revealButton;

        xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"NoRevealButton.xaml"));
        VERIFY_IS_NOT_NULL(rootGrid);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootGrid;

            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootGrid->FindName("passwordBox"));
            passwordBox->PreventKeyboardDisplayOnProgrammaticFocus = true;
            VERIFY_IS_NOT_NULL(passwordBox);

            button = safe_cast<xaml_controls::Button^>(rootGrid->FindName("button"));
            VERIFY_IS_NOT_NULL(button);

            passwordBoxPasswordChangedRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                    [passwordBoxPasswordChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PasswordChanged event fired on passwordBox!");
                passwordBoxPasswordChangedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Programmatic);

        LOG_OUTPUT(L"Injecting abc keystrokes as password input.");
        TestServices::KeyboardHelper->PressKeySequence("abc");
        passwordBoxPasswordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Check reveal button existence.");
        RunOnUIThread([&]()
        {
            revealButton = safe_cast<xaml_primitives::ToggleButton^>(TreeHelper::GetVisualChildByName(passwordBox, L"RevealButton"));
            VERIFY_IS_NULL(revealButton);
        });

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Dcomp dump comparison to make sure password is visible.");
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
        TestServices::Utilities->ResetMockDCompSurfaceId();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);

        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::VerifyPasswordUsingUIAValuePattern(xaml_controls::PasswordBox^ passwordBox, LPCWSTR expectedPasswordValue)
    {
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"PasswordBox";
        uiaInfo.m_AutomationID = L"PasswordBox";
        uiaInfo.m_cType = UIA_EditControlTypeId;

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(passwordBox, ref new Platform::String(L"PasswordBox"));
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationValuePattern> spValuePattern;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());
            LogThrow_IfFailed(spUIAutomationElement->GetCurrentPatternAs(UIA_ValuePatternId, __uuidof(IUIAutomationValuePattern), &spValuePattern));
            WEX::Common::Throw::IfNull(spValuePattern.Get());
            Common::AutoBSTR autoBstr;
            VERIFY_SUCCEEDED(spValuePattern->get_CurrentValue(autoBstr.ReleaseAndGetAddressOf()));
            LOG_OUTPUT(L"PasswordBox value pattern: %ws", autoBstr.Get());
            AutoBSTR::VerifyAreEqual(expectedPasswordValue, autoBstr);
        });
    }

    void PasswordBoxTests::ValidateCorrectAcceleratorKeyMessageForRevealButtonValues()
    {
        TestCleanupWrapper cleanup;
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);
        Platform::String^ revealEnabledMessage = "ALT F8 to show password";
        Platform::String^ revealDisabledMessage = "";
        xaml_controls::PasswordBox^ passwordBox;
        xaml_controls::Button^ button = nullptr;
        xaml_automation_peers::AutomationPeer^ passwordBoxAP = nullptr;
        xaml_controls::StackPanel^ rootStackPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootStackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <PasswordBox x:Name="passwordBox" PasswordRevealMode='Hidden' FontSize="20" Width="200" Margin="20,5,20,0"/>
                </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootStackPanel;
            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            passwordBoxAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(passwordBox);
        });
        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Checking AcceleratorKeymessage when PasswordRevealMode is set to Hidden in markup.");
            VERIFY_ARE_EQUAL(revealDisabledMessage, passwordBoxAP->GetAcceleratorKey());
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting PasswordRevealMode to Peek.");
            passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Peek;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(revealEnabledMessage, passwordBoxAP->GetAcceleratorKey());
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting PasswordRevealMode to Hidden.");
            passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Hidden;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(revealDisabledMessage, passwordBoxAP->GetAcceleratorKey());
        });

        TestServices::WindowHelper->WaitForIdle();

    }

    void PasswordBoxTests::ValidateRevealButtonAKMessageDoesNotOverwritePreviousMessage()
    {
        TestCleanupWrapper cleanup;
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        Platform::String^ previousKey = "PreviousKey";
        xaml_controls::PasswordBox^ passwordBox;
        xaml_controls::Button^ button = nullptr;
        xaml_automation_peers::AutomationPeer^ passwordBoxAP = nullptr;
        xaml_controls::StackPanel^ rootStackPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootStackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <PasswordBox x:Name="passwordBox" PasswordRevealMode='Hidden' AutomationProperties.AcceleratorKey='PreviousKey' FontSize="20" Width="200" Margin="20,5,20,0"/>
                </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootStackPanel;
            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            passwordBoxAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(passwordBox);
        });
        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(previousKey, passwordBoxAP->GetAcceleratorKey());
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting PasswordRevealMode to Peek.");
            passwordBox->PasswordRevealMode = xaml_controls::PasswordRevealMode::Peek;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(previousKey, passwordBoxAP->GetAcceleratorKey());
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidateCtrlDeleteBehavior()
    {
        TestCleanupWrapper cleanup;
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        Platform::String^ previousKey = "PreviousKey";
        xaml_controls::PasswordBox^ passwordBox;
        xaml_controls::StackPanel^ rootStackPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootStackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <PasswordBox x:Name="passwordBox" Password='test password' PasswordRevealMode='Hidden' FontSize="20" Width="200" Margin="20,5,20,0"/>
                </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootStackPanel;
            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

        });
        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);

        TestServices::KeyboardHelper->PressKeySequence("$d$_right#$u$_right");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence("$d$_ctrl#$d$_delete#$u$_delete#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            Platform::String^ expectedPassword = L"t";
            VERIFY_ARE_EQUAL(expectedPassword, passwordBox->Password);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidatePasswordSetAndGet(const WCHAR* setString, xaml_controls::PasswordBox^ passwordBox, xaml_controls::TextBlock^ bindingTextBlock)
    {
        Platform::String^ testString = ref new Platform::String(setString);
        RunOnUIThread([&]()
        {
            passwordBox->Password = testString;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(testString, bindingTextBlock->Text);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidatePasswordBinding()
    {
        TestCleanupWrapper cleanup;
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::PasswordBox^ passwordBox;
        xaml_controls::TextBlock^ textBlock;
        xaml_controls::StackPanel^ rootStackPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootStackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <PasswordBox x:Name="passwordBox" PasswordRevealMode='Hidden' FontSize="20" Width="200" Margin="20,5,20,0"/>
                        <TextBlock x:Name="textBlock" Text="{Binding Password, ElementName = passwordBox}" FontSize="20" Width="200" Margin="20,5,20,0"/>
                </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootStackPanel;
            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);
            textBlock = safe_cast<xaml_controls::TextBlock^>(rootStackPanel->FindName("textBlock"));
            VERIFY_IS_NOT_NULL(textBlock);

        });
        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);

        LOG_OUTPUT(L"Verifying password binding for typed in text.");
        Platform::String^ longInputText = L"0123456789ABCDEFG";
        TestServices::KeyboardHelper->PressKeySequence(longInputText);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(longInputText, textBlock->Text);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verifying setting password with binding and various length.");
        ValidatePasswordSetAndGet(L"", passwordBox, textBlock);
        ValidatePasswordSetAndGet(L"1234", passwordBox, textBlock);
        ValidatePasswordSetAndGet(L"0123456789ABCDE", passwordBox, textBlock); // 15 chars
        ValidatePasswordSetAndGet(L"0123456789ABCDEF", passwordBox, textBlock); // 16 chars
        ValidatePasswordSetAndGet(L"0123456789ABCDEFG", passwordBox, textBlock); // 17 chars
        ValidatePasswordSetAndGet(L"0123456789ABCDEF0123456789ABCDE", passwordBox, textBlock); //31 chars
        ValidatePasswordSetAndGet(L"0123456789ABCDEF0123456789ABCDEF", passwordBox, textBlock); //32 chars
        ValidatePasswordSetAndGet(L"0123456789ABCDEF0123456789ABCDEFG", passwordBox, textBlock); //33 chars
    }

    void PasswordBoxTests::ValidatePasswordBoxPlaceholderVisibility()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);
        xaml_controls::PasswordBox^ passwordBox = nullptr;
        xaml_controls::Button^ button = nullptr;

        xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"PasswordBoxTests.xaml"));
        VERIFY_IS_NOT_NULL(rootStackPanel);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootStackPanel;

            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);
            passwordBox->Password = "password"; // [SuppressMessage("Microsoft.Security", "CS001:SecretInline", Justification="This is not a credential. We're testing PasswordBox's functionality")]

            button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
            VERIFY_IS_NOT_NULL(button);
        });
        TestServices::WindowHelper->WaitForIdle();
        FocusTestHelper::EnsureFocus(button, FocusState::Pointer);

        TestServices::Utilities->VerifyUIElementTree("1");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Set Placeholder text.");
        RunOnUIThread([&]()
        {
            passwordBox->PlaceholderText = "Placeholder";
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyUIElementTree("2");
        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidatePasswordBoxPlaceholderVisibility_Backspace()
    {
        ValidatePasswordBoxPlaceholderVisibilityHelper(PasswordClearMethod::Backspace);
    }

    void PasswordBoxTests::ValidatePasswordBoxPlaceholderVisibility_ClearValue()
    {
        ValidatePasswordBoxPlaceholderVisibilityHelper(PasswordClearMethod::ClearValue);
    }

    void PasswordBoxTests::ValidatePasswordBoxPlaceholderVisibility_EmptyString()
    {
        ValidatePasswordBoxPlaceholderVisibilityHelper(PasswordClearMethod::EmptyString);
    }

    void PasswordBoxTests::ValidatePasswordBoxPlaceholderVisibilityHelper(PasswordClearMethod clearMethod)
    {
        TestCleanupWrapper cleanup;
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::PasswordBox^ passwordBox;
        xaml_controls::StackPanel^ rootStackPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootStackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <PasswordBox x:Name="passwordBox" PlaceholderText="placeholder" FontSize="20" Width="200" Margin="20,5,20,0"/>
                </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootStackPanel;
            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto placeholderTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(passwordBox, L"PlaceholderTextContentPresenter"));
            VERIFY_IS_NOT_NULL(placeholderTextBlock);

            auto visibility = placeholderTextBlock->Visibility;
            VERIFY_IS_TRUE(visibility == xaml::Visibility::Visible);
        });

        TestServices::WindowHelper->WaitForIdle();
        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);

        TestServices::KeyboardHelper->PressKeySequence(L"a");
        TestServices::WindowHelper->WaitForIdle();


        RunOnUIThread([&]()
        {
            auto placeholderTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(passwordBox, L"PlaceholderTextContentPresenter"));
            VERIFY_IS_NOT_NULL(placeholderTextBlock);

            auto visibility = placeholderTextBlock->Visibility;
            VERIFY_IS_TRUE(visibility == xaml::Visibility::Collapsed);
        });

        TestServices::WindowHelper->WaitForIdle();

        switch (clearMethod)
        {
            case PasswordClearMethod::Backspace:
                TestServices::KeyboardHelper->Backspace();
                break;
            case PasswordClearMethod::ClearValue:
                RunOnUIThread([&]()
                {
                    passwordBox->ClearValue(xaml_controls::PasswordBox::PasswordProperty);
                });
                break;
            case PasswordClearMethod::EmptyString:
                RunOnUIThread([&]()
                {
                    passwordBox->Password = L"";
                });
                break;
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto placeholderTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(passwordBox, L"PlaceholderTextContentPresenter"));
            VERIFY_IS_NOT_NULL(placeholderTextBlock);

            auto visibility = placeholderTextBlock->Visibility;
            VERIFY_IS_TRUE(visibility == xaml::Visibility::Visible);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidatePasteFromClipboard()
    {
        TestCleanupWrapper cleanup;
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::PasswordBox^ passwordBox;
        xaml_controls::TextBox^ textBox;
        xaml_controls::StackPanel^ rootStackPanel = nullptr;

        auto passwordChangedEvent = std::make_shared<Event>();
        auto passwordChangedRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanged);

        auto textBoxFocusedEvent = std::make_shared<Event>();
        auto textBoxFocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

        auto textSelectedChangedEvent = std::make_shared<Event>();
        auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

        RunOnUIThread([&]()
        {
            rootStackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <PasswordBox x:Name="passwordBox" PasswordRevealMode='Hidden' FontSize="20" Width="200" Margin="20,5,20,0"/>
                        <TextBox x:Name="textBox" Text="0123456789ABCDEFG" FontSize="20" Width="200" Margin="20,5,20,0"/>
                </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootStackPanel;

            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
            VERIFY_IS_NOT_NULL(textBox);

            textSelectionChangedRegistration.Attach(
                textBox,
                ref new xaml::RoutedEventHandler(
                    [textSelectedChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"TextBox selection changed.");
                textSelectedChangedEvent->Set();
            }));

            textBoxFocusedRegistration.Attach(
                textBox,
                ref new xaml::RoutedEventHandler(
                    [&](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"TextBox got focus.");
                textSelectedChangedEvent->Reset();
                textBox->SelectAll();
                textBoxFocusedEvent->Set();
            }));

            passwordChangedRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                    [passwordChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PasswordChanged event fired on passwordBox!");
                passwordChangedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();
        FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);

        textBoxFocusedEvent->WaitForDefault();
        textSelectedChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"textBox->CopySelectionToClipboard()");
            textBox->CopySelectionToClipboard();
        });

        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"passwordBox->PasteFromClipboard()");
            passwordBox->PasteFromClipboard();
        });

        passwordChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(passwordBox->Password, textBox->Text);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PasswordBoxTests::ValidateCanPasteClipboardContent()
    {
        TestCleanupWrapper cleanup;
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::PasswordBox^ passwordBox;
        xaml_controls::TextBox^ textBox;
        xaml_controls::StackPanel^ rootStackPanel = nullptr;

        auto passwordChangedEvent = std::make_shared<Event>();
        auto passwordChangedRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanged);

        auto textBoxFocusedEvent = std::make_shared<Event>();
        auto textBoxFocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

        auto textSelectedChangedEvent = std::make_shared<Event>();
        auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

        RunOnUIThread([&]()
        {
            rootStackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <PasswordBox x:Name="passwordBox" PasswordRevealMode='Hidden' FontSize="20" Width="200" Margin="20,5,20,0"/>
                        <TextBox x:Name="textBox" Text="0123456789ABCDEFG" FontSize="20" Width="200" Margin="20,5,20,0"/>
                </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootStackPanel;

            passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootStackPanel->FindName("passwordBox"));
            VERIFY_IS_NOT_NULL(passwordBox);

            textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
            VERIFY_IS_NOT_NULL(textBox);

            textSelectionChangedRegistration.Attach(
                textBox,
                ref new xaml::RoutedEventHandler(
                    [textSelectedChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"TextBox selection changed.");
                textSelectedChangedEvent->Set();
            }));

            textBoxFocusedRegistration.Attach(
                textBox,
                ref new xaml::RoutedEventHandler(
                    [&](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"TextBox got focus.");
                textSelectedChangedEvent->Reset();
                textBox->SelectAll();
                textBoxFocusedEvent->Set();
            }));

            passwordChangedRegistration.Attach(
                passwordBox,
                ref new xaml::RoutedEventHandler(
                    [passwordChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PasswordChanged event fired on passwordBox!");
                passwordChangedEvent->Set();
            }));
        });

        auto clipboardBitmapSet = std::make_shared<Event>();
        auto resourcesPath = GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
        create_task(StorageFile::GetFileFromPathAsync(resourcesPath + L"Smiley.bmp"))
            .then([=](StorageFile^ pFile)
        {
            VERIFY_IS_NOT_NULL(pFile);
            auto imgStreamRef = RandomAccessStreamReference::CreateFromFile(pFile);
            auto dataPackage = ref new DataPackage();
            dataPackage->SetBitmap(imgStreamRef);

            RunOnUIThread([=]()
            {
                DataTransfer::Clipboard::SetContent(dataPackage);
                clipboardBitmapSet->Set();
            });
        });

        clipboardBitmapSet->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"passwordBox->CanPasteClipboardContent");
            VERIFY_IS_FALSE(passwordBox->CanPasteClipboardContent);
        });

        TestServices::WindowHelper->WaitForIdle();
        FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);

        textBoxFocusedEvent->WaitForDefault();
        textSelectedChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"textBox->CopySelectionToClipboard()");
            textBox->CopySelectionToClipboard();
        });

        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"passwordBox->CanPasteClipboardContent");
            VERIFY_IS_TRUE(passwordBox->CanPasteClipboardContent);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    } }
} } } }
