// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AccessKeyIntegrationTests.h"

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>

#include "AccessKeyTestHelper.h"
#include "KeyboardInjectionOverride.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

    bool AccessKeyIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        return true;
    }

    bool AccessKeyIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AccessKeyIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    void AccessKeyIntegrationTests::ComboBoxAccessKeyIntegrationTest()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' Background='RoyalBlue' SelectedIndex='1' Width='350' AccessKey='A' ExitDisplayModeOnAccessKeyInvoked='false'> "
                L"    <ComboBoxItem Content='item one' />"
                L"    <ComboBoxItem Content='item two' />"
                L"    <ComboBoxItem Content='item three' />"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            auto comboBoxOpenedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
            openedRegistration.Attach(comboBox, [&]() { comboBoxOpenedEvent->Set(); });
            auto dropDownClosedEvent = std::make_shared<Event>();
            auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
            dropDownClosedRegistration.Attach(comboBox, [&]() { dropDownClosedEvent->Set(); });

            // open ComboBox
            EnableAccessKeyMode enterAccessKeyMode;
            AccessKeyTestHelper::InjectAccessKey(L"A", comboBox);
            comboBoxOpenedEvent->WaitForDefault();

            // close ComboBox
            AccessKeyTestHelper::InjectAccessKey(L"A", comboBox);
            dropDownClosedEvent->WaitForDefault();
        }
    }

    // For this test to pass, the Euro symbol has to  correctly rendered as <80>. Make sure the file is in ANSI or UTF-BOM format before modifying it
    void AccessKeyIntegrationTests::TextBoxAccessKeyIntegrationTest()
    {
        TestCleanupWrapper cleanup;

        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::Button^ button = nullptr;

        auto textBoxTextChangedEvent = std::make_shared<Event>();
        auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
        auto textBoxGotFocusEvent = std::make_shared<Event>();
        auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
        auto buttonClickedEvent = std::make_shared<Event>();
        auto buttonClickedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'>"
                L"  </TextBox>"
                L"</StackPanel>"));

            textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
            VERIFY_IS_NOT_NULL(textBox);

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            TestServices::WindowHelper->WindowContent = rootPanel;
            button->AccessKey = L"E";
            button->ExitDisplayModeOnAccessKeyInvoked = false;
            textBoxGotFocusRegistration.Attach(
                textBox,
                ref new xaml::RoutedEventHandler(
                    [textBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"TextBox control GotFocus handler.");
                textBoxGotFocusEvent->Set();
            }));

            textBoxTextChangedRegistration.Attach(
                textBox,
                ref new xaml_controls::TextChangedEventHandler(
                    [textBoxTextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
            {
                LOG_OUTPUT(L"TextBox control TextChanged handler.");
                textBoxTextChangedEvent->Set();
            }));

            buttonClickedRegistration.Attach(button,
                ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Btn click event fired!");
                buttonClickedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Setting focus to the TextBox control by tapping.");
        TestServices::InputHelper->Tap(textBox);
        textBoxGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        Platform::String ^strEuro = L"\u20AC"; //euro symbol
        LOG_OUTPUT(L"Pressing Ctrl+ALT+E to input euro sign");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_alt#$d$_e#$u$_e#$u$_alt#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        textBoxTextChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"TextInput:%ws", textBox->Text->Data());
            VERIFY_ARE_EQUAL(textBox->Text, strEuro);
            textBox->Text = "";
        });
        TestServices::WindowHelper->WaitForIdle();

        // make sure no access key 'E' invocation on button for "ctrl_alt_e" key press.
        buttonClickedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
        VERIFY_IS_FALSE(buttonClickedEvent->HasFired());

        {
            // press 'E' after entering accesskey mode, it should invoke on button.
            textBoxTextChangedEvent->Reset();
            EnableAccessKeyMode enterAccessKeyMode;
            TestServices::KeyboardHelper->PressKeySequence(L"E");
            buttonClickedEvent->WaitForDefault();
            textBoxTextChangedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(textBoxTextChangedEvent->HasFired());

            // press 'AB', it should not enter text into textbox
            Platform::String ^strAB = L"AB";
            TestServices::KeyboardHelper->PressKeySequence(strAB);
            textBoxTextChangedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(textBoxTextChangedEvent->HasFired());

            // press 'E' again, it should invoke on button.
            textBoxTextChangedEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence(L"E");
            buttonClickedEvent->WaitForDefault();
            textBoxTextChangedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(textBoxTextChangedEvent->HasFired());
        }
    }

    void AccessKeyIntegrationTests::AccessHotKeyIntegrationTest()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::ComboBox^ comboBox = nullptr;

        auto buttonClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto comboBoxOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
        auto dropDownClosedEvent = std::make_shared<Event>();
        auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        auto accessKeyShownEvent = std::make_shared<Event>();
        auto accessKeyShownRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayRequested);
        auto accessKeyHiddenEvent = std::make_shared<Event>();
        auto accessKeyHiddenRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayDismissed);
        auto isActiveChangedFired = std::make_shared<Event>();
        auto activeChangedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics, IsDisplayModeEnabledChanged);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Width='400' Height='800' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <Button x:Name='button' Content='Focus' Margin='20,80,20,0' Background='RoyalBlue' AccessKey='A' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  <ComboBox x:Name='comboBox' Background='RoyalBlue' Margin='15' SelectedIndex='1'  Width='350' AccessKey='B' ExitDisplayModeOnAccessKeyInvoked='false'> "
                L"    <ComboBoxItem Content='item one' />"
                L"    <ComboBoxItem Content='item two' />"
                L"    <ComboBoxItem Content='item three' />"
                L"  </ComboBox>"
                L"</StackPanel>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            TestServices::WindowHelper->WindowContent = rootPanel;

            openedRegistration.Attach(comboBox, [&]() { comboBoxOpenedEvent->Set(); });
            dropDownClosedRegistration.Attach(comboBox, [&]() { dropDownClosedEvent->Set(); });
            clickRegistration.Attach(button, [&]() { buttonClickEvent->Set(); });
            accessKeyShownRegistration.Attach(button, [&]() { accessKeyShownEvent->Set(); });
            accessKeyHiddenRegistration.Attach(button, [&]() { accessKeyHiddenEvent->Set(); });
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        Platform::Object^ akNavigationObject;
        VERIFY_SUCCEEDED(wf::GetActivationFactory(wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.AccessKeyManager").Get(),
            reinterpret_cast<IInspectable**>(&akNavigationObject)));
        auto nav = safe_cast<Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics^>(akNavigationObject);

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(nav->IsDisplayModeEnabled);
            LOG_OUTPUT(L"Subscribe to IsActiveChanged");
            activeChangedRegistration.Attach(nav, [&]()
            {
                LOG_OUTPUT(L"IsActiveChanged fired");
                isActiveChangedFired->Set();
            });
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press alt-a to invoke hot key on Button.");
        AccessKeyTestHelper::InjectAccessKey(L"$d$_alt#$d$_a#$u$_a#$u$_alt", button);
        buttonClickEvent->WaitForDefault();

        LOG_OUTPUT(L"Press alt-b to invoke hot key on ComboBox.");
        AccessKeyTestHelper::InjectAccessKey(L"$d$_alt#$d$_b#$u$_b#$u$_alt", comboBox);
        comboBoxOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press alt-a to invoke hot key on Button again.");
        AccessKeyTestHelper::InjectAccessKey(L"$d$_alt#$d$_a#$u$_a#$u$_alt", button);
        buttonClickEvent->WaitForDefault();

        LOG_OUTPUT(L"Press alt-b to invoke hot key on ComboBox again.");
        AccessKeyTestHelper::InjectAccessKey(L"$d$_alt#$d$_b#$u$_b#$u$_alt", comboBox);
        dropDownClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        accessKeyShownEvent->Reset();
        accessKeyHiddenEvent->Reset();
        isActiveChangedFired->Reset();

        LOG_OUTPUT(L"Make sure hot key invocation does not trigger AccessKeyNavigation state change.");
        isActiveChangedFired->WaitForNoThrow(std::chrono::milliseconds(100));
        VERIFY_IS_FALSE(isActiveChangedFired->HasFired());
        LOG_OUTPUT(L"Make sure hot key invocation does not trigger AccessKeyShown and AccessKeyHidden events.");
        accessKeyShownEvent->WaitForNoThrow(std::chrono::milliseconds(100));
        VERIFY_IS_FALSE(accessKeyShownEvent->HasFired());
        accessKeyHiddenEvent->WaitForNoThrow(std::chrono::milliseconds(100));
        VERIFY_IS_FALSE(accessKeyHiddenEvent->HasFired());

        buttonClickEvent->Reset();
        comboBoxOpenedEvent->Reset();
        dropDownClosedEvent->Reset();

        LOG_OUTPUT(L"Press alt-c, nothing should happen.");
        AccessKeyTestHelper::InjectAccessKey(L"$d$_alt#$d$_c#$u$_c#$u$_alt", button, false);
        buttonClickEvent->WaitForNoThrow(std::chrono::milliseconds(100));
        VERIFY_IS_FALSE(buttonClickEvent->HasFired());
        VERIFY_IS_FALSE(comboBoxOpenedEvent->HasFired());
        VERIFY_IS_FALSE(dropDownClosedEvent->HasFired());

        LOG_OUTPUT(L"verify regular access navigation mode still works.");
        {
            EnableAccessKeyMode enterAccessKeyMode;
            accessKeyShownEvent->WaitForDefault();
            AccessKeyTestHelper::InjectAccessKey(L"A", button);
            buttonClickEvent->WaitForDefault();
        }

        LOG_OUTPUT(L"Change button visibility to collapsed.");
        RunOnUIThread([&]()
        {
            button->Visibility = xaml::Visibility::Collapsed;
        });
        TestServices::WindowHelper->WaitForIdle();

        buttonClickEvent->Reset();

        LOG_OUTPUT(L"Press alt-a should not invoke hot key on invisible Button.");
        AccessKeyTestHelper::InjectAccessKey(L"$d$_alt#$d$_a#$u$_a#$u$_alt", button, false);
        buttonClickEvent->WaitForNoThrow(std::chrono::milliseconds(100));
        VERIFY_IS_FALSE(buttonClickEvent->HasFired());
    }

    void AccessKeyIntegrationTests::AccessKeyFilteringIntegrationTest()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::Button^ button2 = nullptr;
        xaml_controls::ComboBox^ comboBox = nullptr;

        auto buttonClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto button2ClickEvent = std::make_shared<Event>();
        auto click2Registration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto comboBoxOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
        auto dropDownClosedEvent = std::make_shared<Event>();
        auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);

        auto buttonAccessKeyShownEvent = std::make_shared<Event>();
        auto buttonAccessKeyShownRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayRequested);
        auto buttonAccessKeyHiddenEvent = std::make_shared<Event>();
        auto buttonAccessKeyHiddenRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayDismissed);
        auto buttonAccessKeyInvokedEvent = std::make_shared<Event>();
        auto buttonAccessKeyInvokedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyInvoked);

        auto button2AccessKeyShownEvent = std::make_shared<Event>();
        auto button2AccessKeyShownRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayRequested);
        auto button2AccessKeyHiddenEvent = std::make_shared<Event>();
        auto button2AccessKeyHiddenRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayDismissed);
        auto button2AccessKeyInvokedEvent = std::make_shared<Event>();
        auto button2AccessKeyInvokedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyInvoked);

        auto comboBoxAccessKeyShownEvent = std::make_shared<Event>();
        auto comboBoxAccessKeyShownRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayRequested);
        auto comboBoxAccessKeyHiddenEvent = std::make_shared<Event>();
        auto comboBoxAccessKeyHiddenRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayDismissed);
        auto comboBoxAccessKeyInvokedEvent = std::make_shared<Event>();
        auto comboBoxAccessKeyInvokedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyInvoked);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <Button x:Name = 'button' Content = 'Button1' Margin = '20,40,20,0' AccessKey='AB' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  <StackPanel> "
                L"      <ComboBox x:Name='comboBox' Background='RoyalBlue' SelectedIndex='1'  Width='350' AccessKey='AC' ExitDisplayModeOnAccessKeyInvoked='false'> "
                L"        <ComboBoxItem Content='item one' />"
                L"        <ComboBoxItem Content='item two' />"
                L"        <ComboBoxItem Content='item three' />"
                L"      </ComboBox>"
                L"      <Button x:Name = 'button2' Content = 'Button2' Margin = '20,40,20,0' AccessKey='XY' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </StackPanel> "
                L"</StackPanel>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            button2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
            VERIFY_IS_NOT_NULL(button2);

            TestServices::WindowHelper->WindowContent = rootPanel;

            clickRegistration.Attach(button, [&]() { buttonClickEvent->Set(); });
            buttonAccessKeyShownRegistration.Attach(button, [&]() { buttonAccessKeyShownEvent->Set(); });
            buttonAccessKeyHiddenRegistration.Attach(button, [&]() { buttonAccessKeyHiddenEvent->Set();});
            buttonAccessKeyInvokedRegistration.Attach(button, [&]() { buttonAccessKeyInvokedEvent->Set();});

            click2Registration.Attach(button2, [&]() { button2ClickEvent->Set(); });
            button2AccessKeyShownRegistration.Attach(button2, [&]() { button2AccessKeyShownEvent->Set(); });
            button2AccessKeyHiddenRegistration.Attach(button2, [&]() { button2AccessKeyHiddenEvent->Set();});
            button2AccessKeyInvokedRegistration.Attach(button2, [&]() { button2AccessKeyInvokedEvent->Set();});

            openedRegistration.Attach(comboBox, [&]() { comboBoxOpenedEvent->Set(); });
            dropDownClosedRegistration.Attach(comboBox, [&]() { dropDownClosedEvent->Set(); });
            comboBoxAccessKeyShownRegistration.Attach(button, [&]() { comboBoxAccessKeyShownEvent->Set(); });
            comboBoxAccessKeyHiddenRegistration.Attach(button, [&]() { comboBoxAccessKeyHiddenEvent->Set();});
            comboBoxAccessKeyInvokedRegistration.Attach(button, [&]() { comboBoxAccessKeyInvokedEvent->Set();});
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        LOG_OUTPUT(L"\nTesting AccessKey filtering scenario #1.");
        {
            EnableAccessKeyMode enterAccessKeyMode;

            LOG_OUTPUT(L"Entering AccessKey mode should fire shown event.");
            comboBoxAccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyShownEvent->WaitForDefault();

            LOG_OUTPUT(L"Press [a] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"a");
            LOG_OUTPUT(L"Waiting for AccessKeyShown event on both Button(AB) and ComboBox(AC).");
            comboBoxAccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            LOG_OUTPUT(L"AccessKeyHidden event should fire on Button(XY).");
            button2AccessKeyHiddenEvent->WaitForDefault();

            LOG_OUTPUT(L"Press [b] key and AccessKeyHidden should fire on ComboBox(AC).");
            TestServices::KeyboardHelper->PressKeySequence(L"b");
            buttonAccessKeyInvokedEvent->WaitForDefault();
            comboBoxAccessKeyInvokedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(comboBoxAccessKeyHiddenEvent->HasFired());

            // All keys should see a show event again after invoke
            comboBoxAccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyShownEvent->WaitForDefault();
        }

        LOG_OUTPUT(L"\nTesting AccessKey filtering scenario #2.");
        {
            EnableAccessKeyMode enterAccessKeyMode(false);

            comboBoxAccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyShownEvent->WaitForDefault();

            TestServices::KeyboardHelper->PressKeySequence(L"a");
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyHiddenEvent->WaitForDefault();
            comboBoxAccessKeyShownEvent->WaitForDefault();

            LOG_OUTPUT(L"Exit AccessKey mode and AccessKeyHidden event should fire on both Buttons and ComboBox .");
            enterAccessKeyMode.ExitAccessMode();
            comboBoxAccessKeyHiddenEvent->WaitForDefault();
            buttonAccessKeyHiddenEvent->WaitForDefault();
            button2AccessKeyHiddenEvent->WaitForDefault();
        }

        LOG_OUTPUT(L"\nTesting AccessKey filtering scenario #3.");
        {
            EnableAccessKeyMode enterAccessKeyMode;

            comboBoxAccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyHiddenEvent->Reset();
            comboBoxAccessKeyHiddenEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence(L"a");
            comboBoxAccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyHiddenEvent->WaitForDefault();

            LOG_OUTPUT(L"Press [d] key and no hidden event should fire.");
            button2AccessKeyHiddenEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence(L"d");
            buttonAccessKeyHiddenEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(buttonAccessKeyHiddenEvent->HasFired());
            VERIFY_IS_FALSE(button2AccessKeyHiddenEvent->HasFired());
            VERIFY_IS_FALSE(comboBoxAccessKeyHiddenEvent->HasFired());
            // At this point the filtering should still have A in the filter, so let's try to invoke with this prefix.
            TestServices::KeyboardHelper->PressKeySequence(L"b");
            buttonAccessKeyInvokedEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyShownEvent->WaitForDefault();
            comboBoxAccessKeyShownEvent->WaitForDefault();

            TestServices::KeyboardHelper->PressKeySequence(L"X");
            buttonAccessKeyHiddenEvent->WaitForDefault();
            comboBoxAccessKeyHiddenEvent->WaitForDefault();
            button2AccessKeyShownEvent->WaitForDefault();
            TestServices::KeyboardHelper->PressKeySequence(L"y");
            button2AccessKeyInvokedEvent->WaitForDefault();
        }

        LOG_OUTPUT(L"\nPressing the escape key while filtering should back up one filtered character.");
        {
            EnableAccessKeyMode enterAccessKeyMode;

            comboBoxAccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyShownEvent->WaitForDefault();

            TestServices::KeyboardHelper->PressKeySequence(L"x");
            comboBoxAccessKeyHiddenEvent->WaitForDefault();
            buttonAccessKeyHiddenEvent->WaitForDefault();
            button2AccessKeyShownEvent->WaitForDefault();

            TestServices::KeyboardHelper->Escape();
            comboBoxAccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyShownEvent->WaitForDefault();

            TestServices::KeyboardHelper->PressKeySequence(L"a");
            comboBoxAccessKeyShownEvent->WaitForDefault();
            buttonAccessKeyShownEvent->WaitForDefault();
            button2AccessKeyHiddenEvent->WaitForDefault();
        }

        LOG_OUTPUT(L"\nPress Alt-a should not fire any AccessKeyShown event.");
        {
            comboBoxAccessKeyShownEvent->Reset();
            buttonAccessKeyShownEvent->Reset();
            button2AccessKeyShownEvent->Reset();

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_a#$u$_a#$u$_alt");
            TestServices::WindowHelper->WaitForIdle();
            comboBoxAccessKeyShownEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(comboBoxAccessKeyShownEvent->HasFired());
            VERIFY_IS_FALSE(buttonAccessKeyShownEvent->HasFired());
            VERIFY_IS_FALSE(button2AccessKeyShownEvent->HasFired());
        }

        TestServices::WindowHelper->WaitForIdle();

    }

    void AccessKeyIntegrationTests::NestedMenuFlyoutCanBeNavigatedWithAccessKeys()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester bt01(L"bt01");
        AccessKeyTester bt0103(L"bt0103");
        AccessKeyTester bt010301(L"bt010301");
        xaml_controls::Grid^ rootPanel = nullptr;
        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button AccessKey='1' x:Name='bt01' ExitDisplayModeOnAccessKeyInvoked='false'>app1"
                L"      <Button.Flyout>"
                L"        <MenuFlyout>"
                L"          <MenuFlyoutItem    AccessKey='A' ExitDisplayModeOnAccessKeyInvoked='false' x:Name='bt0101'>btA</MenuFlyoutItem>"
                L"          <MenuFlyoutItem    AccessKey='B' ExitDisplayModeOnAccessKeyInvoked='false' x:Name='bt0102'>btB</MenuFlyoutItem>"
                L"          <MenuFlyoutSubItem AccessKey='C' ExitDisplayModeOnAccessKeyInvoked='false' x:Name='bt0103' Text='btC'>"
                L"            <MenuFlyoutItem AccessKey='X' ExitDisplayModeOnAccessKeyInvoked='false' x:Name='bt010301'>bt1</MenuFlyoutItem>"
                L"            <MenuFlyoutItem AccessKey='Y' ExitDisplayModeOnAccessKeyInvoked='false' x:Name='bt010302'>bt2</MenuFlyoutItem>"
                L"          </MenuFlyoutSubItem>"
                L"        </MenuFlyout>"
                L"      </Button.Flyout>"
                L"  </Button>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            bt01.Attach(rootPanel);
            bt0103.Attach(rootPanel);
            bt010301.Attach(rootPanel);

        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        LOG_OUTPUT(L"Testing nested AccessKeys in a MenuFlyout");
        {
            EnableAccessKeyMode enterAccessKeyMode;

            bt01.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(bt01.m_isShown);
            VERIFY_IS_FALSE(bt0103.m_isShown);
            VERIFY_IS_FALSE(bt010301.m_isShown);

            VERIFY_ARE_EQUAL(0, bt01.m_executeCount);
            VERIFY_ARE_EQUAL(0, bt0103.m_executeCount);
            VERIFY_ARE_EQUAL(0, bt010301.m_executeCount);

            LOG_OUTPUT(L"Press [1] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"1");
            TestServices::WindowHelper->WaitForIdle();
            bt01.keyHiddenEvent->WaitForDefault();
            bt01.keyInvokedEvent->WaitForDefault();

            VERIFY_IS_FALSE(bt01.m_isShown);
            VERIFY_IS_TRUE(bt0103.m_isShown);
            VERIFY_IS_FALSE(bt010301.m_isShown);

            VERIFY_ARE_EQUAL(1, bt01.m_executeCount);
            VERIFY_ARE_EQUAL(0, bt0103.m_executeCount);
            VERIFY_ARE_EQUAL(0, bt010301.m_executeCount);

            LOG_OUTPUT(L"Press [C] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"C");
            TestServices::WindowHelper->WaitForIdle();
            bt0103.keyHiddenEvent->WaitForDefault();
            bt0103.keyInvokedEvent->WaitForDefault();

            VERIFY_ARE_EQUAL(1, bt01.m_executeCount);
            VERIFY_ARE_EQUAL(1, bt0103.m_executeCount);
            VERIFY_ARE_EQUAL(0, bt010301.m_executeCount);

            bt010301.keyShownEvent->WaitForDefault();

            VERIFY_IS_FALSE(bt01.m_isShown);
            VERIFY_IS_FALSE(bt0103.m_isShown);
            VERIFY_IS_TRUE(bt010301.m_isShown);

            LOG_OUTPUT(L"Press [X] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"X");
            TestServices::WindowHelper->WaitForIdle();
            bt010301.keyInvokedEvent->WaitForDefault();

            VERIFY_ARE_EQUAL(1, bt01.m_executeCount);
            VERIFY_ARE_EQUAL(1, bt0103.m_executeCount);
            VERIFY_ARE_EQUAL(1, bt010301.m_executeCount);
        }
    }

    void AccessKeyIntegrationTests::CanNavigateScopesOnInvocation()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester ph1b1(L"ph1b1");
        AccessKeyTester ph1b2(L"ph1b2");
        AccessKeyTester ph2b1(L"ph2b1");
        AccessKeyTester ph2b2(L"ph2b2");
        AccessKeyTester ph1(L"ph1");
        AccessKeyTester ph2(L"ph2");

        xaml_controls::Grid^ rootPanel = nullptr;
        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Pivot> "
                L"      <PivotItem Header='ph1' x:Name='ph1' AccessKey='a' ExitDisplayModeOnAccessKeyInvoked='false' IsAccessKeyScope='true'>"
                L"          <StackPanel>"
                L"              <Button x:Name='ph1b1' Content='ph1b1' AccessKey='a' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"              <Button x:Name='ph1b2' Content='ph1b2' AccessKey='b' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"          </StackPanel>"
                L"      </PivotItem>"
                L"      <PivotItem Header='ph2' x:Name='ph2' AccessKey='b' ExitDisplayModeOnAccessKeyInvoked='false' IsAccessKeyScope='true'>"
                L"          <StackPanel >"
                L"              <Button x:Name='ph2b1' Content='ph2b1' AccessKey='a' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"              <Button x:Name='ph2b2' Content='ph2b2' AccessKey='b' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"          </StackPanel>"
                L"      </PivotItem>"
                L"  </Pivot>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            ph1b1.Attach(rootPanel);
            ph1b2.Attach(rootPanel);
            ph2b1.Attach(rootPanel);
            ph2b2.Attach(rootPanel);
            ph1.Attach(rootPanel);
            ph2.Attach(rootPanel);

        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        LOG_OUTPUT(L"Testing nested AccessKey Scope in a Pivot");

        EnableAccessKeyMode enterAccessKeyMode;

        ph1.keyShownEvent->WaitForDefault();
        ph2.keyShownEvent->WaitForDefault();

        VERIFY_IS_TRUE(ph1.m_isShown);
        VERIFY_IS_TRUE(ph2.m_isShown);
        VERIFY_IS_FALSE(ph1b1.m_isShown);
        VERIFY_IS_FALSE(ph1b2.m_isShown);
        VERIFY_IS_FALSE(ph2b1.m_isShown);
        VERIFY_IS_FALSE(ph2b2.m_isShown);

        VERIFY_ARE_EQUAL(0, ph1.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph2.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph1b1.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph1b2.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph2b1.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph2b2.m_executeCount);

        LOG_OUTPUT(L"Press [a] key.");
        TestServices::KeyboardHelper->PressKeySequence(L"a");
        TestServices::WindowHelper->WaitForIdle();

        ph1.keyInvokedEvent->WaitForDefault();
        ph1.keyHiddenEvent->WaitForDefault();
        ph2.keyHiddenEvent->WaitForDefault();
        ph1b1.keyShownEvent->WaitForDefault();
        ph1b2.keyShownEvent->WaitForDefault();

        VERIFY_IS_FALSE(ph1.m_isShown);
        VERIFY_IS_FALSE(ph2.m_isShown);
        VERIFY_IS_TRUE(ph1b1.m_isShown);
        VERIFY_IS_TRUE(ph1b2.m_isShown);
        VERIFY_IS_FALSE(ph2b1.m_isShown);
        VERIFY_IS_FALSE(ph2b2.m_isShown);

        VERIFY_ARE_EQUAL(1, ph1.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph2.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph1b1.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph1b2.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph2b1.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph2b2.m_executeCount);

        LOG_OUTPUT(L"Press [b] key.");
        TestServices::KeyboardHelper->PressKeySequence(L"b");
        TestServices::WindowHelper->WaitForIdle();
        ph1b2.keyInvokedEvent->WaitForDefault();

        VERIFY_ARE_EQUAL(1, ph1.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph2.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph1b1.m_executeCount);
        VERIFY_ARE_EQUAL(1, ph1b2.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph2b1.m_executeCount);
        VERIFY_ARE_EQUAL(0, ph2b2.m_executeCount);

        ph1.keyShownEvent->Reset();
        ph2.keyShownEvent->Reset();

        LOG_OUTPUT(L"Press Escape");
        TestServices::KeyboardHelper->Escape();
        TestServices::WindowHelper->WaitForIdle();
        ph1.keyShownEvent->WaitForDefault();
        ph2.keyShownEvent->WaitForDefault();

        VERIFY_IS_TRUE(ph1.m_isShown);
        VERIFY_IS_TRUE(ph2.m_isShown);
        VERIFY_IS_FALSE(ph1b1.m_isShown);
        VERIFY_IS_FALSE(ph1b2.m_isShown);
        VERIFY_IS_FALSE(ph2b1.m_isShown);
        VERIFY_IS_FALSE(ph2b2.m_isShown);
    }

    void AccessKeyIntegrationTests::PointerInputExitsAccessKeyMode()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"      <Button x:Name='spb2' Content='ph1b2' AccessKey='2' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            spb1.Attach(rootPanel);
            spb2.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        LOG_OUTPUT(L"Testing Pointer exit");
        {
            EnableAccessKeyMode enterAccessKeyMode(false);

            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb1.m_isShown);

            TestServices::InputHelper->Tap(rootPanel);
            TestServices::WindowHelper->WaitForIdle();
            // Tap causes the keys to hide.
            spb1.keyHiddenEvent->WaitForDefault();
            spb2.keyHiddenEvent->WaitForDefault();

            VERIFY_IS_FALSE(spb1.m_isShown);
            VERIFY_IS_FALSE(spb2.m_isShown);
            VERIFY_ARE_EQUAL(0, spb1.m_executeCount);
            VERIFY_ARE_EQUAL(0, spb2.m_executeCount);

            LOG_OUTPUT(L"Press [1] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"1");
            TestServices::WindowHelper->WaitForIdle();

            // Pressing one should not cause the buttons to click
            VERIFY_ARE_EQUAL(0, spb1.m_executeCount);
            VERIFY_ARE_EQUAL(0, spb2.m_executeCount);
        }

        LOG_OUTPUT(L"Verify can re-enter and invoke");
        {
            EnableAccessKeyMode enterAccessKeyMode;

            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb1.m_isShown);

            LOG_OUTPUT(L"Press [1] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"1");
            TestServices::WindowHelper->WaitForIdle();

            spb1.keyInvokedEvent->WaitForDefault();

            // Pressing one should not cause the buttons to click
            VERIFY_ARE_EQUAL(1, spb1.m_executeCount);
            VERIFY_ARE_EQUAL(0, spb2.m_executeCount);
        }
    }

    void AccessKeyIntegrationTests::DoNotEnterAKModeWhenNoAKElements()
    {
        Platform::Object^ aknavigationObject;
        auto isActiveChangedFired = std::make_shared<Event>();
        wf::EventRegistrationToken isActiveChangedToken;

        VERIFY_SUCCEEDED(wf::GetActivationFactory(wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.AccessKeyManager").Get(),
            reinterpret_cast<IInspectable**>(&aknavigationObject)));
        auto nav = safe_cast<Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics^>(aknavigationObject);

        xaml_controls::Grid^ rootPanel = nullptr;

        TestCleanupWrapper cleanup([&]
        {
            RunOnUIThread([&]()
            {
                nav->IsDisplayModeEnabledChanged -= isActiveChangedToken;
            });
        });

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel>"
                L"      <Button x:Name='spb1' Content='ph1b1'/>"
                L"      <Button x:Name='spb2' Content='ph1b2'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        RunOnUIThread([&]()
        {
            isActiveChangedToken = nav->IsDisplayModeEnabledChanged += ref new wf::TypedEventHandler<Platform::Object^, Platform::Object^>(
                [&](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"IsActiveChanged fired");
                isActiveChangedFired->Set();
            });
        });

        TestServices::KeyboardHelper->Alt();
        TestServices::WindowHelper->WaitForIdle();
        isActiveChangedFired->WaitForNoThrow(std::chrono::milliseconds(100));

        VERIFY_IS_FALSE(isActiveChangedFired->HasFired());
    }

    void AccessKeyIntegrationTests::TabExitsAKMode()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Button^ btn1 = nullptr;
        xaml_controls::Button^ btn = nullptr;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        auto gotFocusButtonEvent = std::make_shared<Event>();
        auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        auto gotFocusButton2Event = std::make_shared<Event>();
        auto gotFocusButton2Registration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"  <StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"      <Button x:Name='spb2' Content='ph1b2' AccessKey='2' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        RunOnUIThread([&]()
        {
            btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"spb1"));
            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"spb2"));

            gotFocusButtonRegistration.Attach(btn1, [gotFocusButtonEvent]()
            {
                LOG_OUTPUT(L"Button got focus");
                gotFocusButtonEvent->Set();
            });

            gotFocusButton2Registration.Attach(btn, [gotFocusButton2Event]()
            {
                LOG_OUTPUT(L"Button2 got focus");
                gotFocusButton2Event->Set();
            });

            spb1.Attach(rootPanel);
            spb2.Attach(rootPanel);

            btn1->Focus(FocusState::Keyboard);
        });

        gotFocusButtonEvent->WaitForDefault();

        {
            EnableAccessKeyMode akMode(false);
            TestServices::KeyboardHelper->Tab();

            spb1.keyHiddenEvent->WaitForDefault();
            spb2.keyHiddenEvent->WaitForDefault();

            VERIFY_IS_FALSE(spb1.m_isShown);
            VERIFY_IS_FALSE(spb2.m_isShown);

            gotFocusButton2Event->WaitForDefault();

            VERIFY_IS_TRUE(gotFocusButton2Event->HasFired());
            akMode.VerifyAKModeHasExited();
        }
    }

    void AccessKeyIntegrationTests::DirectionArrowsExitsAKMode()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Button^ btn = nullptr;

        AccessKeyTester spb1(L"spb1");

        auto gotFocusButtonEvent = std::make_shared<Event>();
        auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        auto keydownEvent = std::make_shared<Event>();
        auto keydownRegistration = CreateSafeEventRegistration(xaml_controls::Button, KeyDown);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"  <StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false' />"
                L"  </StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        RunOnUIThread([&]()
        {
            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"spb1"));
            keydownRegistration.Attach(btn, [keydownEvent]()
            {
                LOG_OUTPUT(L"Button received a keydown");
                keydownEvent->Set();
            });

            gotFocusButtonRegistration.Attach(btn, [gotFocusButtonEvent]()
            {
                LOG_OUTPUT(L"Button got focus");
                gotFocusButtonEvent->Set();
            });

            spb1.Attach(rootPanel);

            btn->Focus(FocusState::Keyboard);

        });

        gotFocusButtonEvent->WaitForDefault();

        {
            EnableAccessKeyMode akMode(false);
            TestServices::KeyboardHelper->Down();

            spb1.keyHiddenEvent->WaitForDefault();
            VERIFY_IS_FALSE(spb1.m_isShown);

            keydownEvent->WaitForDefault();

            VERIFY_IS_TRUE(keydownEvent->HasFired());
            akMode.VerifyAKModeHasExited();
        }
    }

    void AccessKeyIntegrationTests::ElementEnteringTreeFiresAKEvents()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        auto keyShownRegistration = CreateSafeEventRegistration(xaml_controls::Button, AccessKeyDisplayRequested);
        std::shared_ptr<Event> keyShownEvent = std::make_shared<Event>();

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
            spb1.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode enterAccessKeyMode;
            spb1.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.keyShownEvent->HasFired());

            RunOnUIThread([&]()
            {
                btn = ref new xaml_controls::Button();
                btn->Content = L"Button";
                btn->AccessKey = L"2";

                keyShownRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"KeyShown fired");
                    keyShownEvent->Set();
                });

                sp->Children->Append(btn);
            });

            TestServices::WindowHelper->WaitForIdle();
            keyShownEvent->WaitForDefault();
            VERIFY_IS_TRUE(keyShownEvent->HasFired());
        }
    }

    void AccessKeyIntegrationTests::ElementLeavingTreeFiresAKEvents()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='1' />"
                L"      <Button x:Name='spb2' Content='ph1b2' AccessKey='2'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"spb2"));

            spb1.Attach(rootPanel);
            spb2.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode enterAccessKeyMode;
            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            RunOnUIThread([&]
            {
                unsigned int index = 0;
                sp->Children->IndexOf(btn, &index);
                sp->Children->RemoveAt(index);
            });
            TestServices::WindowHelper->WaitForIdle();

            spb2.keyHiddenEvent->WaitForDefault();
            VERIFY_IS_FALSE(spb2.m_isShown);
        }
    }

    void AccessKeyIntegrationTests::ChangingVisibilityFiresCorrectEvents()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='1'/>"
                L"      <Button x:Name='spb2' Content='ph1b2' AccessKey='2'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"spb2"));

            spb1.Attach(rootPanel);
            spb2.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode enterAccessKeyMode;

            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            spb2.keyHiddenEvent->Reset();

            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Collapsing element");
                btn->Visibility = Visibility::Collapsed;
            });

            spb2.keyHiddenEvent->WaitForDefault();
            VERIFY_IS_FALSE(spb2.m_isShown);

            spb2.keyShownEvent->Reset();

            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Un-Collapsing element");
                btn->Visibility = Visibility::Visible;
            });

            spb2.keyShownEvent->WaitForDefault();
            VERIFY_IS_TRUE(spb2.m_isShown);
        }
    }

    void AccessKeyIntegrationTests::ChangingIsEnabledFiresCorrectEvents()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='1'/>"
                L"      <Button x:Name='spb2' Content='ph1b2' AccessKey='2'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"spb2"));

            spb1.Attach(rootPanel);
            spb2.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode enterAccessKeyMode;

            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            spb2.keyHiddenEvent->Reset();

            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Disabling element");
                btn->IsEnabled = false;
            });

            spb2.keyHiddenEvent->WaitForDefault();
            VERIFY_IS_FALSE(spb2.m_isShown);

            spb2.keyShownEvent->Reset();

            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Enabling element");
                btn->IsEnabled = true;
            });

            spb2.keyShownEvent->WaitForDefault();
            VERIFY_IS_TRUE(spb2.m_isShown);
        }
    }

    void AccessKeyIntegrationTests::ScopeOwnerLeavingTreeShouldUpdateScope()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"      <Button x:Name='spb2' Content='ph1b2' AccessKey='2' ExitDisplayModeOnAccessKeyInvoked='false'>"
                L"          <Button.Flyout>"
                L"              <MenuFlyout>"
                L"                  <MenuFlyoutItem AccessKey='A' x:Name='mb' ExitDisplayModeOnAccessKeyInvoked='false'>btA</MenuFlyoutItem>"
                L"              </MenuFlyout>"
                L"          </Button.Flyout>"
                L"      </Button>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"spb2"));

            spb1.Attach(rootPanel);
            spb2.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode enterAccessKeyMode;
            spb1.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            LOG_OUTPUT(L"Press [2] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"2");
            TestServices::WindowHelper->WaitForIdle();

            spb1.keyHiddenEvent->WaitForDefault();
            VERIFY_IS_FALSE(spb1.m_isShown);
            VERIFY_IS_FALSE(spb2.m_isShown);

            spb1.keyShownEvent->Reset();

            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Removing Button");
                VERIFY_IS_TRUE(sp->Children->GetAt(1)->Equals(btn));
                sp->Children->RemoveAt(1);
            });

            TestServices::WindowHelper->WaitForIdle();
            spb1.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
        }
    }

    void AccessKeyIntegrationTests::BackOutAcrossEmptyScope()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester sp1Ak(L"sp1");
        AccessKeyTester spb1Ak(L"spb1");

        xaml_controls::StackPanel^ rootPanel;

        RunOnUIThread([&]()
        {
            // AccessKey tree:
            //  Root
            //    - "A" sp1
            //      - (null) sp2
            //        - "1" spb1
            //        - "2" spb2
            //    - "X"
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Margin='20'>"
                L"  <StackPanel x:Name='sp1' AccessKey='A' IsAccessKeyScope='true' >"
                L"      <StackPanel x:Name='sp2' IsAccessKeyScope='true'>" 
                L"          <Button x:Name='spb1' Content='button1' AccessKey='1' />"
                L"          <Button x:Name='spb2' Content='button2' AccessKey='2' />"
                L"      </StackPanel>"
                L"  </StackPanel>"
                L"  <Button Content='button x' AccessKey='X' />"
                L"</StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            sp1Ak.Attach(rootPanel);
            spb1Ak.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL_ON_UITHREAD(false, Microsoft::UI::Xaml::Input::AccessKeyManager::IsDisplayModeEnabled);

            EnableAccessKeyMode enterAccessKeyMode;

            spb1Ak.keyShownEvent->WaitForDefault();
            VERIFY_ARE_EQUAL_ON_UITHREAD(true, Microsoft::UI::Xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
            TestServices::WindowHelper->WaitForIdle();

            spb1Ak.keyHiddenEvent->Reset();
            sp1Ak.keyShownEvent->Reset();
            LOG_OUTPUT(L"Press Escape.  This should back out across sp2's scope, because it's empty, to the root scope.");
            TestServices::KeyboardHelper->Escape();
            spb1Ak.keyHiddenEvent->WaitForDefault();
            sp1Ak.keyShownEvent->WaitForDefault();
            VERIFY_ARE_EQUAL_ON_UITHREAD(true, Microsoft::UI::Xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
        }

        VERIFY_ARE_EQUAL_ON_UITHREAD(false, Microsoft::UI::Xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
    }

    void AccessKeyIntegrationTests::BackOutAcrossEmptyScopeAndExit()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1Ak(L"spb1");

        xaml_controls::StackPanel^ rootPanel = nullptr;

        auto isDisplayModeEnabledChangedRegistration =
            CreateSafeEventRegistration(xaml_input::IAccessKeyManagerStatics, IsDisplayModeEnabledChanged);
        auto isDisplayModeEnabledChangedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            // AccessKey tree:
            //  Root
            //    - (null) sp1
            //      - (null) sp2
            //        - "1" spb1
            //        - "2" spb2
            //    - "X" buttonX -- gets removed from the tree
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Margin='20'>"
                L"  <StackPanel x:Name='sp1' IsAccessKeyScope='true' >"
                L"      <StackPanel x:Name='sp2' IsAccessKeyScope='true'>" 
                L"          <Button x:Name='spb1' Content='button1' AccessKey='1' />"
                L"          <Button x:Name='spb2' Content='button2' AccessKey='2' />"
                L"      </StackPanel>"
                L"  </StackPanel>"
                L"  <Button x:Name='buttonX' Content='buttonX' AccessKey='X' />"
                L"</StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            spb1Ak.Attach(rootPanel);

            isDisplayModeEnabledChangedRegistration.Attach(
                AccessKeyTestHelper::GetAccessKeyManagerStatics(),
                [isDisplayModeEnabledChangedEvent]() {
                    LOG_OUTPUT(L"IsDisplayModeEnabledChanged fired");
                    isDisplayModeEnabledChangedEvent->Set();
            });
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            VERIFY_ARE_EQUAL_ON_UITHREAD(false, Microsoft::UI::Xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
            EnableAccessKeyMode enterAccessKeyMode;

            spb1Ak.keyShownEvent->WaitForDefault();
            VERIFY_ARE_EQUAL_ON_UITHREAD(true, Microsoft::UI::Xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Remove buttonX from scene.");
                rootPanel->Children->RemoveAtEnd();
            });
            TestServices::WindowHelper->WaitForIdle();

            isDisplayModeEnabledChangedEvent->Reset();
            spb1Ak.keyHiddenEvent->Reset();
            LOG_OUTPUT(L"Press Escape.  Since buttonX was removed, this should back out across all the empty scopes"
                       L" and exit DisplayMode.");
            TestServices::KeyboardHelper->Escape();
            spb1Ak.keyHiddenEvent->WaitForDefault();
            isDisplayModeEnabledChangedEvent->WaitForDefault();
            VERIFY_ARE_EQUAL_ON_UITHREAD(false, Microsoft::UI::Xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
        }
    }

    void AccessKeyIntegrationTests::ElementEnteringTreeDuringFilterFiresAKEvents()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        auto keyShownRegistration = CreateSafeEventRegistration(xaml_controls::Button, AccessKeyDisplayRequested);
        std::shared_ptr<Event> keyShownEvent = std::make_shared<Event>();

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='12' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
            spb1.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode enterAccessKeyMode;
            spb1.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.keyShownEvent->HasFired());

            spb1.keyShownEvent->Reset();

            LOG_OUTPUT(L"Press [1] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"1");
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(spb1.keyShownEvent->HasFired());

            RunOnUIThread([&]()
            {
                btn = ref new xaml_controls::Button();
                btn->Content = L"Button";
                btn->AccessKey = L"11";

                keyShownRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"KeyShown fired");
                    keyShownEvent->Set();
                });

                sp->Children->Append(btn);
            });

            TestServices::WindowHelper->WaitForIdle();
            keyShownEvent->WaitForDefault();
            VERIFY_IS_TRUE(keyShownEvent->HasFired());
        }
    }

    void AccessKeyIntegrationTests::ElementEnteringTreeDuringFilterDoesNotFiresAKEvents()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        auto keyShownRegistration = CreateSafeEventRegistration(xaml_controls::Button, AccessKeyDisplayRequested);
        std::shared_ptr<Event> keyShownEvent = std::make_shared<Event>();

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='12' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
            spb1.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode enterAccessKeyMode;
            spb1.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.keyShownEvent->HasFired());

            spb1.keyShownEvent->Reset();

            LOG_OUTPUT(L"Press [1] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"1");
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(spb1.keyShownEvent->HasFired());

            RunOnUIThread([&]()
            {
                btn = ref new xaml_controls::Button();
                btn->Content = L"Button";
                btn->AccessKey = L"2";

                keyShownRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"KeyShown fired");
                    keyShownEvent->Set();
                });

                sp->Children->Append(btn);
            });

            TestServices::WindowHelper->WaitForIdle();
            keyShownEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(keyShownEvent->HasFired());
        }
    }

    void AccessKeyIntegrationTests::ElementLeavingTreeDuringFilterFiresAKEvents()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='11' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"      <Button x:Name='spb2' Content='ph1b2' AccessKey='12' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"spb2"));

            spb1.Attach(rootPanel);
            spb2.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode enterAccessKeyMode;
            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            LOG_OUTPUT(L"Press [1] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"1");
            TestServices::WindowHelper->WaitForIdle();

            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            RunOnUIThread([&]
            {
                VERIFY_IS_TRUE(sp->Children->GetAt(1)->Equals(btn));
                sp->Children->RemoveAt(1);
            });

            spb2.keyHiddenEvent->WaitForDefault();
            VERIFY_IS_FALSE(spb2.m_isShown);
        }
    }

    void AccessKeyIntegrationTests::ElementLeavingTreeDuringFilterDoesNotFiresAKEvents()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='11' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"      <Button x:Name='spb2' Content='ph1b2' AccessKey='2' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"spb2"));

            spb1.Attach(rootPanel);
            spb2.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode enterAccessKeyMode;
            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            LOG_OUTPUT(L"Press [1] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"1");
            TestServices::WindowHelper->WaitForIdle();

            spb2.keyHiddenEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_FALSE(spb2.m_isShown);

            spb2.keyHiddenEvent->Reset();

            RunOnUIThread([&]
            {
                unsigned int index = 0;
                sp->Children->IndexOf(btn, &index);
                sp->Children->RemoveAt(index);
            });

            spb2.keyHiddenEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(spb2.keyHiddenEvent->HasFired());
        }
    }

    void AccessKeyIntegrationTests::AppBarExitsAKModeWhenESCPressed()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester apb1(L"appB1");

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;
        xaml_controls::AppBarButton^ btn = nullptr;

        auto gotFocusButtonEvent = std::make_shared<Event>();
        auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <AppBar IsOpen='True'>"
                L"      <AppBarButton x:Name='appB1' Label='AppBar' Icon='Accept' Content='appbarButton' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </AppBar>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            btn = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"appB1"));

            apb1.Attach(rootPanel);

            gotFocusButtonRegistration.Attach(btn, [gotFocusButtonEvent]()
            {
                LOG_OUTPUT(L"Button got focus");
                gotFocusButtonEvent->Set();
            });

            btn->Focus(xaml::FocusState::Keyboard);
        });

        gotFocusButtonEvent->WaitForDefault();
        VERIFY_IS_TRUE(gotFocusButtonEvent->HasFired());

        {
            EnableAccessKeyMode akMode;
            apb1.keyShownEvent->WaitForDefault();
            VERIFY_IS_TRUE(apb1.m_isShown);

            LOG_OUTPUT(L"Press Escape");
            TestServices::KeyboardHelper->Escape();
            TestServices::WindowHelper->WaitForIdle();
            apb1.keyHiddenEvent->WaitForDefault();
            VERIFY_IS_FALSE(apb1.m_isShown);

            akMode.VerifyAKModeHasExited();
        }
    }

    void AccessKeyIntegrationTests::CanControlWhenAKModeExits()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");
        AccessKeyTester spb2(L"spb2");

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::StackPanel^ sp = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel x:Name='sp'>"
                L"      <Button x:Name='spb1' Content='ph1b1' AccessKey='1' />"
                L"      <Button x:Name='spb2' Content='ph1b2' AccessKey='2' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));

            spb1.Attach(rootPanel);
            spb2.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Test that we exit AK mode when invoking an element with ExitDisplayModeOnAccessKeyInvoked = true");
        {
            EnableAccessKeyMode enterAccessKeyMode(false);

            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            LOG_OUTPUT(L"Press [1] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"1");
            TestServices::WindowHelper->WaitForIdle();

            spb1.keyInvokedEvent->WaitForDefault();

            VERIFY_IS_FALSE(spb1.m_isShown);
            VERIFY_IS_FALSE(spb2.m_isShown);
            enterAccessKeyMode.VerifyAKModeHasExited();
        }

        LOG_OUTPUT(L"Test that we do not exit AK mode when invoking an element with ExitDisplayModeOnAccessKeyInvoked = false");
        {
            EnableAccessKeyMode enterAccessKeyMode;

            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            LOG_OUTPUT(L"Press [2] key.");
            TestServices::KeyboardHelper->PressKeySequence(L"2");
            TestServices::WindowHelper->WaitForIdle();

            spb2.keyInvokedEvent->WaitForDefault();

            spb1.keyShownEvent->WaitForDefault();
            spb2.keyShownEvent->WaitForDefault();

            VERIFY_IS_TRUE(spb1.m_isShown);
            VERIFY_IS_TRUE(spb2.m_isShown);

            enterAccessKeyMode.VerifyAKModeHasNotExited();
        }
    }

    void AccessKeyIntegrationTests::OnlyNakedAltsEnterAKMode()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");

        xaml_controls::Grid^ rootPanel = nullptr;

        auto isActiveChangedFired = std::make_shared<Event>();
        auto activeChangedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics, IsDisplayModeEnabledChanged);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button x:Name='spb1' Content='ph1b1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            spb1.Attach(rootPanel);
        });
        TestServices::WindowHelper->WaitForIdle();

        Platform::Object^ akNavigationObject;
        VERIFY_SUCCEEDED(wf::GetActivationFactory(wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.AccessKeyManager").Get(),
            reinterpret_cast<IInspectable**>(&akNavigationObject)));
        auto nav = safe_cast<Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics^>(akNavigationObject);

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(nav->IsDisplayModeEnabled);
            LOG_OUTPUT(L"Subscribe to IsActiveChanged");
            activeChangedRegistration.Attach(nav, [&]()
            {
                LOG_OUTPUT(L"IsActiveChanged fired");
                isActiveChangedFired->Set();
            });
        });
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        KeyboardInjectionIgnoreEventWaitOverride ignoreWait;

        // Up alt should never enter AK Mode without a corresponding down alt
        PressKeyVerifyNoThrow(L"$u$_alt", isActiveChangedFired);
        PressKeyVerifyNoThrow(L"$u$_ctrl#$u$_alt#$u$_del", isActiveChangedFired);
        PressKeyVerifyNoThrow(L"$u$_ctrl#$u$_del#$u$_alt", isActiveChangedFired);
        PressKeyVerifyNoThrow(L"$u$_del#$u$_alt#$u$_ctrl", isActiveChangedFired);
        PressKeyVerifyNoThrow(L"$u$_del#$u$_ctrl#$u$_alt", isActiveChangedFired);

        // Control and alt key combitions.  These should never enter AK Mode.
        PressKeyVerifyNoThrow(L"$d$_ctrl#$d$_alt#$u$_alt#$u$_ctrl", isActiveChangedFired);
        PressKeyVerifyNoThrow(L"$d$_ctrl#$d$_alt#$u$_ctrl#$u$_alt", isActiveChangedFired);
        PressKeyVerifyNoThrow(L"$d$_alt#$d$_ctrl#$u$_alt#$u$_ctrl", isActiveChangedFired);
        PressKeyVerifyNoThrow(L"$d$_alt#$d$_ctrl#$u$_ctrl#$u$_alt", isActiveChangedFired);

        // Alt pressed and released when a non-modifier key is held down will enter AK mode.  These conditions will not however.
        PressKeyVerifyNoThrow(L"$d$_a#$d$_alt#$u$_a#$u$_alt", isActiveChangedFired);
        PressKeyVerifyNoThrow(L"$d$_alt#$d$_a#$u$_alt#$u$_a", isActiveChangedFired);
        PressKeyVerifyNoThrow(L"$d$_alt#$d$_a#$u$_a#$u$_alt", isActiveChangedFired);
    }

    void AccessKeyIntegrationTests::PressKeyVerifyNoThrow(Platform::String^ keySequence, std::shared_ptr<Event> noThrowEvent)
    {
        // Press the key sequence
        LOG_OUTPUT(L"\ntesting sequence %s", keySequence->Data());
        noThrowEvent->Reset();
        TestServices::KeyboardHelper->PressKeySequence(keySequence);
        noThrowEvent->WaitForNoThrow(std::chrono::milliseconds(100));
        VERIFY_IS_FALSE(noThrowEvent->HasFired());

        // Verify can enter AKMode after the key sequence
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$u$_alt");
        noThrowEvent->WaitForDefault();
        // And Exit
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$u$_alt");
        noThrowEvent->WaitForDefault();
    }

    void AccessKeyIntegrationTests::EnterAndSpaceExitAKMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ btn = nullptr;

        auto btnClick = std::make_shared<Event>();
        auto btnClickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        auto btnInvoked = std::make_shared<Event>();
        auto btnInvokedRegistration = CreateSafeEventRegistration(xaml_controls::Button, AccessKeyInvoked);

        RunOnUIThread([&]()
        {
            btn = ref new xaml_controls::Button();
            btn->Content = "btn1";
            btn->AccessKey = "1"; // Needed to enter AK Mode.
            btn->ExitDisplayModeOnAccessKeyInvoked = false; // Want to ensure it is the space/enter key causing the exit not an invocation

            btnClickRegistration.Attach(btn, [&]() {btnClick->Set(); });
            btnInvokedRegistration.Attach(btn, [&]() {btnInvoked->Set(); });
            btn->Focus(FocusState::Programmatic);

            TestServices::WindowHelper->WindowContent = btn;
        });
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode akMode(false);
            TestServices::KeyboardHelper->Enter();

            // We should still invoke the button, but not due to the mnemonics events
            btnClick->WaitForDefault();
            VERIFY_IS_FALSE(btnInvoked->HasFired());
        }

        {
            EnableAccessKeyMode akMode(false);
            TestServices::KeyboardHelper->PressKeySequence("$d$_ #$u$_ ");

            // We should still invoke the button, but not due to the mnemonics events
            btnClick->WaitForDefault();
            VERIFY_IS_FALSE(btnInvoked->HasFired());
        }
    }

    void AccessKeyIntegrationTests::VerifyThatErrorsThrownInInvalidScenarios()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Button^ btn = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button x:Name='button' Content='ph1b1' AccessKey='1'/>"
                L"</StackPanel>"));

            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));

            rootPanel->IsAccessKeyScope = false;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT(btn->AccessKeyScopeOwner = rootPanel, Platform::InvalidArgumentException^);

            // Verify that FlyoutBase does not
            xaml_controls::Primitives::FlyoutBase^ flyout = ref new xaml_controls::Flyout();

            // Verifies that we can still add the flyout base without an error
            btn->AccessKeyScopeOwner = flyout;
        });
    }

    void AccessKeyIntegrationTests::VerifyAltAkInvokesAccessKeysInAKMode()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester btn1(L"b1");
        AccessKeyTester sp1(L"sp1");
        AccessKeyTester sp1btn1(L"sp1b1");

        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button x:Name='b1' Content='ph1b1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"      <StackPanel x:Name='sp1' AccessKey='2' ExitDisplayModeOnAccessKeyInvoked='false' IsAccessKeyScope='true'>"
                L"          <Button x:Name='sp1b1' Content='sp1b1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"      </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            btn1.Attach(rootPanel);
            sp1btn1.Attach(rootPanel);
            sp1.Attach(rootPanel);
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(sp1.GetOwner<xaml_controls::StackPanel>()->IsAccessKeyScope);
        });
        {
            EnableAccessKeyMode akMode;

            btn1.keyShownEvent->WaitForDefault();
            sp1.keyShownEvent->WaitForDefault();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_1#$u$_1#$u$_alt");
            btn1.keyInvokedEvent->WaitForDefault();

            // Navigate to the scope
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_2#$u$_2#$u$_alt");
            sp1.keyInvokedEvent->WaitForDefault();
            btn1.keyHiddenEvent->WaitForDefault();
            sp1.keyHiddenEvent->WaitForDefault();
            sp1btn1.keyShownEvent->WaitForDefault();

            // Invoke the nested button
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_1#$u$_1#$u$_alt");
            sp1btn1.keyInvokedEvent->WaitForDefault();
        }
        TestServices::WindowHelper->WaitForIdle();

        // Check functionality when we exit AK mode on invoke...
        RunOnUIThread([&]()
        {
            btn1.GetOwner<xaml_controls::Button>()->ExitDisplayModeOnAccessKeyInvoked  = true;
            sp1btn1.GetOwner<xaml_controls::Button>()->ExitDisplayModeOnAccessKeyInvoked = true;
            sp1.GetOwner<xaml_controls::StackPanel>()->ExitDisplayModeOnAccessKeyInvoked = true;
        });

        LOG_OUTPUT(L"Retest for the case when ExitDisplayModeOnAccessKeyInvoked = true");
        {
            EnableAccessKeyMode akMode(false);

            btn1.keyShownEvent->WaitForDefault();
            sp1.keyShownEvent->WaitForDefault();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_1#$u$_1#$u$_alt");
            btn1.keyInvokedEvent->WaitForDefault();
        }

        {
            EnableAccessKeyMode akMode(false);

            // Navigate to the scope
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_2#$u$_2#$u$_alt");
            btn1.keyHiddenEvent->WaitForDefault();
            sp1.keyInvokedEvent->WaitForDefault();
            sp1.keyHiddenEvent->WaitForDefault();
            sp1btn1.keyShownEvent->WaitForDefault();

            // Invoke the nested button
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_1#$u$_1#$u$_alt");
            sp1btn1.keyInvokedEvent->WaitForDefault();
        }
    }

    void AccessKeyIntegrationTests::NakedAltExitsAKMode()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester spb1(L"spb1");

        xaml_controls::Grid^ rootPanel = nullptr;

        auto isActiveChangedFired = std::make_shared<Event>();
        auto activeChangedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics, IsDisplayModeEnabledChanged);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button x:Name='spb1' Content='ph1b1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            spb1.Attach(rootPanel);
        });
        TestServices::WindowHelper->WaitForIdle();

        Platform::Object^ akNavigationObject;
        VERIFY_SUCCEEDED(wf::GetActivationFactory(wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.AccessKeyManager").Get(),
            reinterpret_cast<IInspectable**>(&akNavigationObject)));
        auto nav = safe_cast<Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics^>(akNavigationObject);

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(nav->IsDisplayModeEnabled);
            LOG_OUTPUT(L"Subscribe to IsActiveChanged");
            activeChangedRegistration.Attach(nav, [&]()
            {
                LOG_OUTPUT(L"IsActiveChanged fired");
                isActiveChangedFired->Set();
            });
        });
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        KeyboardInjectionIgnoreEventWaitOverride ignoreWait;

        // Up alt should never exit AK Mode without a corresponding down alt.  It should also come in Naked - e.g. no other keys with it
        {
            EnableAccessKeyMode akMode;

            // This worked fine in UWP.  But in Win32, Xaml does enter AK mode when
            // it receives an alt-up without a previous alt-down.
            //PressKeyVerifyNoThrow(L"$u$_alt", isActiveChangedFired);

            PressKeyVerifyNoThrow(L"$u$_ctrl#$u$_alt#$u$_del", isActiveChangedFired);
            PressKeyVerifyNoThrow(L"$u$_ctrl#$u$_del#$u$_alt", isActiveChangedFired);
            PressKeyVerifyNoThrow(L"$u$_del#$u$_alt#$u$_ctrl", isActiveChangedFired);
            PressKeyVerifyNoThrow(L"$u$_del#$u$_ctrl#$u$_alt", isActiveChangedFired);

            // Control and alt key combitions.  These should never exit AK Mode.
            PressKeyVerifyNoThrow(L"$d$_ctrl#$d$_alt#$u$_alt#$u$_ctrl", isActiveChangedFired);
            PressKeyVerifyNoThrow(L"$d$_ctrl#$d$_alt#$u$_ctrl#$u$_alt", isActiveChangedFired);
            PressKeyVerifyNoThrow(L"$d$_alt#$d$_ctrl#$u$_alt#$u$_ctrl", isActiveChangedFired);
            PressKeyVerifyNoThrow(L"$d$_alt#$d$_ctrl#$u$_ctrl#$u$_alt", isActiveChangedFired);

            // Alt pressed and released when a non-modifier key is held down will exit AK mode.  These conditions will not however.
            PressKeyVerifyNoThrow(L"$d$_a#$d$_alt#$u$_a#$u$_alt", isActiveChangedFired);
            PressKeyVerifyNoThrow(L"$d$_alt#$d$_a#$u$_alt#$u$_a", isActiveChangedFired);
            PressKeyVerifyNoThrow(L"$d$_alt#$d$_a#$u$_a#$u$_alt", isActiveChangedFired);

            akMode.VerifyAKModeHasNotExited();
        }
    }

    void AccessKeyIntegrationTests::CanChangeScopeOwnerToRootScope()
    {
        TestCleanupWrapper cleanup;

        AccessKeyTester btn1(L"btn1");
        AccessKeyTester btn2(L"btn2");
        AccessKeyTester btn3(L"btn3");

        xaml_controls::StackPanel^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button x:Name='btn1' Content='btn1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  <Button x:Name='btn2' Content='btn2' AccessKey='2' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  <Button x:Name='btn3' Content='btn3' AccessKey='A' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"</StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            btn1.Attach(rootPanel);
            btn2.Attach(rootPanel);
            btn3.Attach(rootPanel);

            btn2.GetOwner<xaml_controls::Button>()->IsAccessKeyScope = true;
            btn3.GetOwner<xaml_controls::Button>()->AccessKeyScopeOwner = btn2.GetOwner<xaml_controls::Button>();
        });
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode akMode;

            // Verify that btn3 is not shown in root scope
            btn1.keyShownEvent->WaitForDefault();
            btn2.keyShownEvent->WaitForDefault();
            btn3.keyShownEvent->WaitForNoThrow(std::chrono::milliseconds(100));
        }

        // Set the scope owner of btn 3 to nullptr - e.g. make it a part of root scope while AK Mode is inactive.
        RunOnUIThread([&]()
        {
            btn3.GetOwner<xaml_controls::Button>()->AccessKeyScopeOwner = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify All keys now show");
        {
            EnableAccessKeyMode akMode;
            btn1.keyShownEvent->WaitForDefault();
            btn2.keyShownEvent->WaitForDefault();
            btn3.keyShownEvent->WaitForDefault();
        }

        // Going to prepare to repeat the above steps but this time change the owner while AKMode is active.  Should have same behavior
        RunOnUIThread([&]()
        {
            btn3.GetOwner<xaml_controls::Button>()->AccessKeyScopeOwner = btn2.GetOwner<xaml_controls::Button>();
        });

        {
            EnableAccessKeyMode akMode;

            // Verify that btn3 is not shown in root scope
            btn1.keyShownEvent->WaitForDefault();
            btn2.keyShownEvent->WaitForDefault();
            btn3.keyShownEvent->WaitForNoThrow(std::chrono::milliseconds(100));

            // Set the scope owner of btn 3 to nullptr - e.g. make it a part of root scope while AK Mode is active.
            RunOnUIThread([&]()
            {
                btn3.GetOwner<xaml_controls::Button>()->AccessKeyScopeOwner = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // Unfortunately, changing scope owner of an element already in the tree to an already active scope
        // will not add that element to the active scope.  It will take effect when the scope is rebuilt (so I'll do that now).
        LOG_OUTPUT(L"Verify All keys now show");
        {
            EnableAccessKeyMode akMode;
            btn1.keyShownEvent->WaitForDefault();
            btn2.keyShownEvent->WaitForDefault();
            btn3.keyShownEvent->WaitForDefault();
        }
    }

    void AccessKeyIntegrationTests::ElementWithoutAutomationPeerPatternGainsFocusWhenInvoked()
    {
        TestCleanupWrapper cleanup;

        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Slider, GotFocus);

        AccessKeyTester btn(L"btn");
        AccessKeyTester slider(L"slider");

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Slider^ sliderControl = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button x:Name='btn' Content='btn1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  <Slider x:Name='slider' AccessKey='S' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"</StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sliderControl = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));

            btn.Attach(rootPanel);
            slider.Attach(rootPanel);

            gotFocusRegistration.Attach(sliderControl, [&]()
            {
                LOG_OUTPUT(L"slider gained focus");
                gotFocusEvent->Set();
            });
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode akMode;
            TestServices::KeyboardHelper->PressKeySequence("$d$_s#$u$_s");

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());
        }
    }

    void AccessKeyIntegrationTests::ScopeOwnerWithoutAutomationPeerPatternGainsFocusWhenInvoked()
    {
        TestCleanupWrapper cleanup;

        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Slider, GotFocus);

        auto gotFocusEventB = std::make_shared<Event>();
        auto gotFocusRegistrationB = CreateSafeEventRegistration(xaml_controls::ContentControl, GotFocus);

        AccessKeyTester btn(L"btn");
        AccessKeyTester slider(L"slider");
        AccessKeyTester content(L"cc");

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Slider^ sliderControl = nullptr;
        xaml_controls::ContentControl^ contentControl = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button x:Name='btn' Content='btn1' AccessKey='1' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  <ContentControl IsAccessKeyScope='True' x:Name='cc' AccessKey='2'>"
                L"      <Slider x:Name='slider' AccessKey='S' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                L"  </ContentControl>"
                L"</StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            sliderControl = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));
            contentControl = safe_cast<xaml_controls::ContentControl^>(rootPanel->FindName(L"cc"));

            sliderControl->AccessKeyScopeOwner = contentControl;

            btn.Attach(rootPanel);
            slider.Attach(rootPanel);
            content.Attach(rootPanel);

            gotFocusRegistration.Attach(sliderControl, [&]()
            {
                LOG_OUTPUT(L"slider gained focus");
                gotFocusEvent->Set();
            });

            gotFocusRegistrationB.Attach(contentControl, [&]()
            {
                LOG_OUTPUT(L"content control gained focus");
                gotFocusEventB->Set();
            });
        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        {
            EnableAccessKeyMode akMode;
            TestServices::KeyboardHelper->PressKeySequence("$d$_2#$u$_2");

            gotFocusEventB->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEventB->HasFired());
            gotFocusRegistrationB.Detach();

            TestServices::KeyboardHelper->PressKeySequence("$d$_s#$u$_s");

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());
        }
    }

} } } } }
