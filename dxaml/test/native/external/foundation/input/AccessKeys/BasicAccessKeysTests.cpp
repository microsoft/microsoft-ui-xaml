// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicAccessKeysTests.h"
#include <TestCleanupWrapper.h>
#include <TestEvent.h>
#include <XamlTailored.h>
#include <TreeHelper.h>
#include "AccessKeyTestHelper.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

        bool BasicAccessKeysTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool BasicAccessKeysTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool BasicAccessKeysTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void BasicAccessKeysTests::AccessKeyNavigation()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            RunOnUIThread([&]()
            {
                button->Content = L"Button";
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            {
                EnableAccessKeyMode akMode(false);
                TestServices::WindowHelper->WaitForIdle();

                akMode.VerifyAKModeHasNotExited();
                LOG_OUTPUT(L"Trigger state change");

                akMode.ExitAccessModeWithAKManager();

                akMode.VerifyAKModeHasExited();
            }

            // Verify we aren't locked out of entering again
            {
                EnableAccessKeyMode akMode(false);
                TestServices::WindowHelper->WaitForIdle();

                akMode.VerifyAKModeHasNotExited();
                LOG_OUTPUT(L"Trigger state change");

                akMode.ExitAccessModeWithAKManager();

                akMode.VerifyAKModeHasExited();
            }
        }

        void BasicAccessKeysTests::EnterExitAccessKeyModeUsingKeyInput()
        {
            TestCleanupWrapper cleanup;
            VerifyEnterExitAccessKeyModeUsingKeyInput();
        }

        void BasicAccessKeysTests::VerifyEnterExitAccessKeyModeUsingKeyInput()
        {
            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_controls::Button^ button = nullptr;
            RunOnUIThread([&]()
            {
                rootPanel = ref new xaml_controls::StackPanel();

                button = ref new xaml_controls::Button();
                button->Content = L"Test Button";
                rootPanel->Children->Append(button);

                button->AccessKey = L"A";
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();
            
            LOG_OUTPUT(L"Verify entering and exiting access key mode using alt key.");
            {
                EnableAccessKeyMode enterAccessKeyMode;

                AccessKeyTestHelper::InjectAccessKey(L"A", button);
            }

            LOG_OUTPUT(L"Verify exiting access key mode using escape key.");
            {
                EnableAccessKeyMode enableAccessKeyMode(false);

                AccessKeyTestHelper::InjectAccessKey(L"A", button);
                enableAccessKeyMode.ExitAccessModeUsingEscKey();
            }
        }

        void BasicAccessKeysTests::EnterAccessKeyModeUsingEnterDisplayModeForXamlRoot()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button1;
            xaml_controls::Button^ button2;
            xaml_controls::StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                rootPanel = ref new xaml_controls::StackPanel();

                button1 = ref new xaml_controls::Button();
                button1->Content = L"button 1";
                button1->AccessKey = L"A";

                rootPanel->Children->Append(button1);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            LOG_OUTPUT(L"EnterDisplayModeForXamlRoot on RootPanel");
            {
                EnableAccessKeyMode enterAccessKeyMode(false, false, false);
                auto scopeGuard = wil::scope_exit([&]
                {
                    enterAccessKeyMode.ExitAccessModeWithAKManager();
                });
                RunOnUIThread([&]()
                {
                    xaml_input::AccessKeyManager::EnterDisplayMode(rootPanel->XamlRoot);
                });
                enterAccessKeyMode.VerifyAKModeHasNotExited();
                TestServices::WindowHelper->WaitForIdle();
                AccessKeyTestHelper::InjectAccessKey(L"A", button1, true);
                TestServices::WindowHelper->WaitForIdle();
             }
        }

        void BasicAccessKeysTests::AccessKeyInvokedEvent()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->AccessKey = L"A";
                button->ExitDisplayModeOnAccessKeyInvoked = false;
                clickRegistration.Attach(button, [&]()
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            LOG_OUTPUT(L"Verify firing AccessKeyInvokedEvent.");
            {
                EnableAccessKeyMode enableAccessKeyMode;
                AccessKeyTestHelper::InjectAccessKey(L"A", button);
                buttonClickEvent->WaitForDefault();

                // now handle the AccessKeyInvoked event and make sure invoke does not happen on the control
                buttonClickEvent->Reset();
                AccessKeyTestHelper::InjectAccessKey(L"A", button, true /* expectingAccessKeyInvoked */, true /* handleAccessKeyInvokedEvent */);
                buttonClickEvent->WaitForNoThrow(std::chrono::milliseconds(100));
                VERIFY_IS_FALSE(buttonClickEvent->HasFired());
            }
        }

        void BasicAccessKeysTests::TextElementAccessKeyInvokedEvent()
        {
            TestCleanupWrapper cleanup;
            xaml_docs::Hyperlink ^te = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            // Restore AreKeyTipsEnabled for the next test
            auto testGuard = wil::scope_exit([] {
                RunOnUIThread([&]()
                {
                    xaml_input::AccessKeyManager::AreKeyTipsEnabled = true;
                });
            });

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
                    L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
                    L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
                    L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
                    L"    </TextBlock>"
                    L"  </StackPanel>";
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = rootPanel;
                xaml_input::AccessKeyManager::AreKeyTipsEnabled = false;
            });

            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            RunOnUIThread([&]()
            {
                te = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink"));
                te->AccessKey = L"A";
                te->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enableAccessKeyMode;

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Verify firing AccessKeyInvokedEvent.");
            AccessKeyTestHelper::InjectAccessKey(L"A", te, true /*expectingAccessKeyInvoked*/, true /*handleAccessKeyInvokedEvent*/);
        }

        void BasicAccessKeysTests::TextElementInRichTextBlockAccessKeyInvokedEvent()
        {
            TestCleanupWrapper cleanup;
            xaml_docs::Hyperlink ^te = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            // Restore AreKeyTipsEnabled for the next test
            auto testGuard = wil::scope_exit([] {
                RunOnUIThread([&]()
                {
                    xaml_input::AccessKeyManager::AreKeyTipsEnabled = true;
                });
            });

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
                    L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
                    L"    <RichTextBlock x:Name='rtxbl' Height='200' Width='200'>"
                    L"    <Paragraph x:Name='para1'>"
                    L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
                    L"    </Paragraph>"
                    L"    </RichTextBlock>"
                    L"  </StackPanel>";
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = rootPanel;
                xaml_input::AccessKeyManager::AreKeyTipsEnabled = false;
            });

            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();
            RunOnUIThread([&]()
            {
                te = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink"));
                te->AccessKey = L"A";
                te->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enableAccessKeyMode;

            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::InjectAccessKey(L"A", te, true /*expectingAccessKeyInvoked*/, true /*handleAccessKeyInvokedEvent*/);
        }

        void BasicAccessKeysTests::TextElementInSpanAccessKeyInvokedEvent()
        {
            TestCleanupWrapper cleanup;
            xaml_docs::Hyperlink ^te = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            // Restore AreKeyTipsEnabled for the next test
            auto testGuard = wil::scope_exit([] {
                RunOnUIThread([&]()
                {
                    xaml_input::AccessKeyManager::AreKeyTipsEnabled = true;
                });
            });

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
                    L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
                    L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
                    L"      Lorem ipsum dolor sit amet, consectetur adipiscing elit."
                    L"          <Span> This span contains a hyperlink"
                    L"              <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
                    L"          </Span>"
                    L"   </TextBlock>"
                    L"</StackPanel>";
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = rootPanel;
                xaml_input::AccessKeyManager::AreKeyTipsEnabled = false;
            });

            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            RunOnUIThread([&]()
            {
                te = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink"));
                te->AccessKey = L"A";
                te->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enableAccessKeyMode;

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Verify firing AccessKeyInvokedEvent.");
            AccessKeyTestHelper::InjectAccessKey(L"A", te, true /*expectingAccessKeyInvoked*/, true /*handleAccessKeyInvokedEvent*/);
        }


        void BasicAccessKeysTests::AccessKeyShownHiddenEvent()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            std::shared_ptr<Event> accessKeyShownEvent = std::make_shared<Event>();
            auto accessKeyShownRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayRequested);
            std::shared_ptr<Event> accessKeyHiddenEvent = std::make_shared<Event>();
            auto accessKeyHiddenRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayDismissed);

            Platform::String^ accessKey = L"A";
            Platform::String^ typedKey = L"a";
            Platform::String^ expectedPressedKey = L"";  // For initial Shown event

            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->AccessKey = accessKey;
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(button, [&]()
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                });

                accessKeyShownRegistration.Attach(
                    button,
                    ref new wf::TypedEventHandler<UIElement^, xaml_input::AccessKeyDisplayRequestedEventArgs^>(
                        [=](UIElement^, xaml_input::AccessKeyDisplayRequestedEventArgs^ args)
                {
                    LOG_OUTPUT(L"AccessKeyShown event fired, PressedKeys=%ws", args->PressedKeys->Data());
                    accessKeyShownEvent->Set();

                    VERIFY_ARE_EQUAL(args->PressedKeys, expectedPressedKey); // In this test, we only show the initial access key state, so the shown even will fire with empty character
                }));

                accessKeyHiddenRegistration.Attach(button, [&]()
                {
                    LOG_OUTPUT(L"AccessKeyHidden event fired!");
                    accessKeyHiddenEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            LOG_OUTPUT(L"Verify firing AccessKeyShown and AccessKeyHidden events.");
            {
                EnableAccessKeyMode enableAccessKeyMode;
                accessKeyShownEvent->WaitForDefault();
                AccessKeyTestHelper::InjectAccessKey(typedKey, button);
                LOG_OUTPUT(L"Verify button click event fired.");
                buttonClickEvent->WaitForDefault();
                LOG_OUTPUT(L"Verify AccessKeyShown event fired.");
                accessKeyShownEvent->WaitForDefault();
             }

            LOG_OUTPUT(L"Verify AccessKeyHidden event fired.");
            accessKeyHiddenEvent->WaitForDefault();
        }

        void BasicAccessKeysTests::TextElementAccessKeyShownHiddenEvent()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> hyperlinkClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_docs::Hyperlink, Click);
            std::shared_ptr<Event> accessKeyShownEvent = std::make_shared<Event>();
            auto accessKeyShownRegistration = CreateSafeEventRegistration(xaml_docs::TextElement, AccessKeyDisplayRequested);
            std::shared_ptr<Event> accessKeyHiddenEvent = std::make_shared<Event>();
            auto accessKeyHiddenRegistration = CreateSafeEventRegistration(xaml_docs::TextElement, AccessKeyDisplayDismissed);

            Platform::String^ accessKey = L"A";
            Platform::String^ typedKey = L"a";
            Platform::String^ expectedPressedKey = L"";  // For initial Shown event

            xaml_docs::Hyperlink ^te = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            // Restore AreKeyTipsEnabled for the next test
            auto testGuard = wil::scope_exit([] {
                RunOnUIThread([&]()
                {
                    xaml_input::AccessKeyManager::AreKeyTipsEnabled = true;
                });
            });

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
                    L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
                    L"    <TextBlock x:Name='txbl' Height='200' Width='200'>"
                    L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
                    L"    </TextBlock>"
                    L"  </StackPanel>";
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = rootPanel;
                xaml_input::AccessKeyManager::AreKeyTipsEnabled = false;
            });

            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            RunOnUIThread([&]()
            {
                te = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink"));
                te->AccessKey = accessKey;
                te->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(te, [&]()
                {
                    LOG_OUTPUT(L"Hyperlink click event fired!");
                    hyperlinkClickEvent->Set();
                });

                accessKeyShownRegistration.Attach(
                    te,
                    ref new wf::TypedEventHandler<xaml_docs::TextElement^, xaml_input::AccessKeyDisplayRequestedEventArgs^>(
                        [=](xaml_docs::TextElement^, xaml_input::AccessKeyDisplayRequestedEventArgs^ args)
                {
                    LOG_OUTPUT(L"AccessKeyShown event fired, PressedKeys=%ws", args->PressedKeys->Data());
                    accessKeyShownEvent->Set();

                    VERIFY_ARE_EQUAL(args->PressedKeys, expectedPressedKey); // In this test, we only show the initial access key state, so the shown even will fire with empty character
                }));

                accessKeyHiddenRegistration.Attach(te, [&]()
                {
                    LOG_OUTPUT(L"AccessKeyHidden event fired!");
                    accessKeyHiddenEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            LOG_OUTPUT(L"Verify firing AccessKeyShown and AccessKeyHidden events.");
            {
                EnableAccessKeyMode enableAccessKeyMode;
                accessKeyShownEvent->WaitForDefault();
                AccessKeyTestHelper::InjectAccessKey(typedKey, te);
                LOG_OUTPUT(L"Verify button click event fired.");
                hyperlinkClickEvent->WaitForDefault();
                LOG_OUTPUT(L"Verify AccessKeyShown event fired.");
                accessKeyShownEvent->WaitForDefault();
            }

            LOG_OUTPUT(L"Verify AccessKeyHidden event fired.");
            accessKeyHiddenEvent->WaitForDefault();
        }

        void BasicAccessKeysTests::RichTextBlockTextElementAccessKeyShownHiddenEvent()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            // Restore AreKeyTipsEnabled for the next test
            auto testGuard = wil::scope_exit([] {
                RunOnUIThread([&]()
                {
                    xaml_input::AccessKeyManager::AreKeyTipsEnabled = true;
                });
            });

            std::shared_ptr<Event> hyperlinkClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_docs::Hyperlink, Click);
            std::shared_ptr<Event> accessKeyShownEvent = std::make_shared<Event>();
            auto accessKeyShownRegistration = CreateSafeEventRegistration(xaml_docs::TextElement, AccessKeyDisplayRequested);
            std::shared_ptr<Event> accessKeyHiddenEvent = std::make_shared<Event>();
            auto accessKeyHiddenRegistration = CreateSafeEventRegistration(xaml_docs::TextElement, AccessKeyDisplayDismissed);

            Platform::String^ accessKey = L"A";
            Platform::String^ typedKey = L"a";
            Platform::String^ expectedPressedKey = L"";  // For initial Shown event

            xaml_docs::Hyperlink ^te = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"      VerticalAlignment='Center' HorizontalAlignment='Center'>"
                    L"    <Button x:Name='btn1' Height='200' Width='200' Content='Button 1'/>"
                    L"    <RichTextBlock x:Name='rtxbl' Height='200' Width='200'>"
                    L"    <Paragraph x:Name='para1'>"
                    L"      <Hyperlink x:Name='hyperlink' Foreground='LightBlue'>Navigate!</Hyperlink>"
                    L"    </Paragraph>"
                    L"    </RichTextBlock>"
                    L"  </StackPanel>";
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = rootPanel;
                xaml_input::AccessKeyManager::AreKeyTipsEnabled = false;
            });

            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            RunOnUIThread([&]()
            {
                te = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink"));
                te->AccessKey = accessKey;
                te->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(te, [&]()
                {
                    LOG_OUTPUT(L"Hyperlink click event fired!");
                    hyperlinkClickEvent->Set();
                });

                accessKeyShownRegistration.Attach(
                    te,
                    ref new wf::TypedEventHandler<xaml_docs::TextElement^, xaml_input::AccessKeyDisplayRequestedEventArgs^>(
                        [=](xaml_docs::TextElement^, xaml_input::AccessKeyDisplayRequestedEventArgs^ args)
                {
                    LOG_OUTPUT(L"AccessKeyShown event fired, PressedKeys=%ws", args->PressedKeys->Data());
                    accessKeyShownEvent->Set();

                    VERIFY_ARE_EQUAL(args->PressedKeys, expectedPressedKey); // In this test, we only show the initial access key state, so the shown even will fire with empty character
                }));

                accessKeyHiddenRegistration.Attach(te, [&]()
                {
                    LOG_OUTPUT(L"AccessKeyHidden event fired!");
                    accessKeyHiddenEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Verify firing AccessKeyShown and AccessKeyHidden events.");
            {
                EnableAccessKeyMode enableAccessKeyMode;
                accessKeyShownEvent->WaitForDefault();
                AccessKeyTestHelper::InjectAccessKey(typedKey, te);
                LOG_OUTPUT(L"Verify button click event fired.");
                hyperlinkClickEvent->WaitForDefault();
                LOG_OUTPUT(L"Verify AccessKeyShown event fired.");
                accessKeyShownEvent->WaitForDefault();
            }

            LOG_OUTPUT(L"Verify AccessKeyHidden event fired.");
            accessKeyHiddenEvent->WaitForDefault();
        }

        void BasicAccessKeysTests::AccessKeyPressed()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->ExitDisplayModeOnAccessKeyInvoked = false;
                clickRegistration.Attach(button,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_AccessKeyPressed, button, buttonClickEvent);
        }

        void BasicAccessKeysTests::AccessKeyPressedOnHiddenOrDeletedControl()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Button^ button;
            xaml_controls::StackPanel^ rootPanel;

            auto btnClick = std::make_shared<Event>();
            auto btnClickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

            RunOnUIThread([&]()
            {
                rootPanel = ref new xaml_controls::StackPanel();

                button = ref new xaml_controls::Button();
                button->Content = L"Test Button";
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                btnClickRegistration.Attach(button, [&]() {btnClick->Set(); });

                rootPanel->Children->Append(button);
                button->AccessKey = L"A";
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            LOG_OUTPUT(L"Press access key on collapsed control.");
            {
                EnableAccessKeyMode enterAccessKeyMode;
                LOG_OUTPUT(L"Change button visibility to collapsed.");
                RunOnUIThread([&]()
                {
                    button->Visibility = xaml::Visibility::Collapsed;
                });
                TestServices::WindowHelper->WaitForIdle();
                AccessKeyTestHelper::InjectAccessKey(L"A", button, false);

                RunOnUIThread([&]()
                {
                    button->Visibility = xaml::Visibility::Visible;
                });
                TestServices::WindowHelper->WaitForIdle();
                VERIFY_IS_FALSE(btnClick->HasFired());
             }

            LOG_OUTPUT(L"Press access key on deleted control.");
            {
                EnableAccessKeyMode enterAccessKeyMode;
                LOG_OUTPUT(L"Delete button.");
                RunOnUIThread([&]()
                {
                    rootPanel->Children->RemoveAt(0);
                    button = nullptr;
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::KeyboardHelper->PressKeySequence(L"A");
                TestServices::WindowHelper->WaitForIdle();
            }
            VERIFY_IS_FALSE(btnClick->HasFired());
        }

        void BasicAccessKeysTests::MultipleAccessKeysPressed()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(button,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_MultipleAccessKeysPressed, button, buttonClickEvent);
        }

        void BasicAccessKeysTests::LowerCaseAccessKeyPressed()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(button,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_LowerCaseAccessKeyPressed, button, buttonClickEvent);
        }

        void BasicAccessKeysTests::UpperCaseAccessKeyPressed()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(button,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_UpperCaseAccessKeyPressed, button, buttonClickEvent);
        }

        void BasicAccessKeysTests::TwoCharAccessKeysPressed()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(button,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_TwoCharactersAccessKey, button, buttonClickEvent);
        }

        void BasicAccessKeysTests::ThreeCharAccessKeysPressed()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(button,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_ThreeCharactersAccessKey, button, buttonClickEvent);
        }

        void BasicAccessKeysTests::UnicodeCharAccessKey()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(button,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_UnicodeCharactersAccessKey, button, buttonClickEvent);
        }

        void BasicAccessKeysTests::NullAccessKeyNotMatching()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->ExitDisplayModeOnAccessKeyInvoked = false;

                clickRegistration.Attach(button,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_NullAccessKey, button, buttonClickEvent);
        }

        void BasicAccessKeysTests::DuplicateKeys()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button1 = nullptr;
            xaml_controls::Button^ button2 = nullptr;

            std::shared_ptr<Event> button1ClickEvent = std::make_shared<Event>();
            auto clickRegistration1 = CreateSafeEventRegistration(xaml_controls::Button, Click);

            std::shared_ptr<Event> button2ClickEvent = std::make_shared<Event>();
            auto clickRegistration2 = CreateSafeEventRegistration(xaml_controls::Button, Click);

            RunOnUIThread([&]()
            {
                auto rootPanel = ref new xaml_controls::StackPanel();
                TestServices::WindowHelper->WindowContent = rootPanel;

                button1 = ref new xaml_controls::Button();
                button1->Content = L"Test Button 1";
                button1->ExitDisplayModeOnAccessKeyInvoked = false;
                rootPanel->Children->Append(button1);
                clickRegistration1.Attach(button1,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn1 click event fired!");
                    button1ClickEvent->Set();
                }));

                button2 = ref new xaml_controls::Button();
                button2->Content = L"Test Button 2";
                button2->ExitDisplayModeOnAccessKeyInvoked = false;
                rootPanel->Children->Append(button2);
                clickRegistration2.Attach(button2,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Btn2 click event fired!");
                    button2ClickEvent->Set();
                }));

                button1->AccessKey = L"A";
                button2->AccessKey = L"A";
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            {
                EnableAccessKeyMode enterAccessKeyMode;
                // press A and only button 1 should fire click event
                TestServices::KeyboardHelper->PressKeySequence(L"A");
                button1ClickEvent->WaitForDefault();
                VERIFY_IS_FALSE(button2ClickEvent->HasFired());
            }
        }

        void BasicAccessKeysTests::ScopeOwners()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Grid^ root;
            xaml_controls::StackPanel^ innerPanel;
            xaml_controls::StackPanel^ outerPanel;
            xaml_controls::Button^ buttonB;
            xaml_controls::Button^ button1;
            xaml_controls::Button^ button2;

            auto buttonB_clickEvent = std::make_shared<Event>();
            auto buttonB_clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

            auto button2_clickEvent = std::make_shared<Event>();
            auto button2_clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

            auto button1_clickEvent = std::make_shared<Event>();
            auto button1_clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

            RunOnUIThread([&]()
            {
                root = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"<StackPanel x:Name='outerPanel'>"
                            L"<StackPanel x:Name='innerPanel' Orientation='Horizontal' >"
                                L"<Button AccessKey='1' x:Name='button1' ExitDisplayModeOnAccessKeyInvoked='false'>Button 1</Button>"
                                L"<Button AccessKey='2' x:Name='button2' ExitDisplayModeOnAccessKeyInvoked='false'>Button 2</Button>"
                            L"</StackPanel>"
                            L"<Button AccessKey='A'>Button A</Button>"
                            L"<Button AccessKey='B' x:Name='buttonB'>Button B</Button>"
                        L"</StackPanel>"
                    L"</Grid>"
                    ));

                innerPanel = safe_cast<xaml_controls::StackPanel^>(root->FindName(L"innerPanel"));
                outerPanel = safe_cast<xaml_controls::StackPanel^>(root->FindName(L"outerPanel"));
                buttonB = safe_cast<xaml_controls::Button^>(root->FindName(L"buttonB"));
                button1 = safe_cast<xaml_controls::Button^>(root->FindName(L"button1"));
                button2 = safe_cast<xaml_controls::Button^>(root->FindName(L"button2"));

                buttonB_clickRegistration.Attach(buttonB, [&]()
                {
                    LOG_OUTPUT(L"buttonB click event fired!");
                    buttonB_clickEvent->Set();
                });

                button2_clickRegistration.Attach(button2, [&]()
                {
                    LOG_OUTPUT(L"button2 click event fired!");
                    button2_clickEvent->Set();
                });


                button1_clickRegistration.Attach(button2, [&]()
                {
                    LOG_OUTPUT(L"button1 click event fired!");
                    button1_clickEvent->Set();
                });


                buttonB->ExitDisplayModeOnAccessKeyInvoked = false;
                button2->ExitDisplayModeOnAccessKeyInvoked = false;
                button1->ExitDisplayModeOnAccessKeyInvoked = false;

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            RunOnUIThread([&]()
            {
                // Make inner panel a scope.  This way root scope will include Buttons A and B and not buttons 1,2
                innerPanel->IsAccessKeyScope = true;
                outerPanel->IsAccessKeyScope = false;

                // Set focus to button B so that when AK mode enters in root scope.
                buttonB->Focus(FocusState::Programmatic);
            });
            TestServices::WindowHelper->WaitForIdle();

            {
                EnableAccessKeyMode akMode;

                buttonB_clickEvent->Reset();
                TestServices::KeyboardHelper->PressKeySequence(L"B");
                buttonB_clickEvent->WaitForDefault();
                VERIFY_IS_TRUE(buttonB_clickEvent->HasFired());

                button2_clickEvent->Reset();
                TestServices::KeyboardHelper->PressKeySequence(L"2");
                button2_clickEvent->WaitForNoThrow(std::chrono::milliseconds(100));
                VERIFY_IS_FALSE(button2_clickEvent->HasFired());
            }

            RunOnUIThread([&]()
            {
                innerPanel->IsAccessKeyScope = false;
                outerPanel->IsAccessKeyScope = false;

                button1->IsAccessKeyScope = true;
                buttonB->AccessKeyScopeOwner = button1;
                // Set focus to button 2 so that Buttons A,1,2 are in root scope. When AK mode starts
                button2->Focus(FocusState::Programmatic);
            });
            TestServices::WindowHelper->WaitForIdle();

            {
                EnableAccessKeyMode akMode;

                buttonB_clickEvent->Reset();
                TestServices::KeyboardHelper->PressKeySequence(L"B");
                buttonB_clickEvent->WaitForNoThrow(std::chrono::milliseconds(100));
                VERIFY_IS_FALSE(buttonB_clickEvent->HasFired());

                button2_clickEvent->Reset();
                TestServices::KeyboardHelper->PressKeySequence(L"2");
                button2_clickEvent->WaitForDefault();
            }

            // Verify we do not invoke access keys outside of a scope defined by an element.  In this case button B
            RunOnUIThread([&]()
            {
                buttonB->Focus(FocusState::Programmatic);
            });
            TestServices::WindowHelper->WaitForIdle();

            {
                EnableAccessKeyMode akMode;

                buttonB_clickEvent->Reset();
                TestServices::KeyboardHelper->PressKeySequence(L"B");
                buttonB_clickEvent->WaitForDefault();

                button2_clickEvent->Reset();
                TestServices::KeyboardHelper->PressKeySequence(L"2");
                button2_clickEvent->WaitForNoThrow(std::chrono::milliseconds(100));
                VERIFY_IS_FALSE(button2_clickEvent->HasFired());

                // The scope owner should also not be reachable.
                button1_clickEvent->Reset();
                TestServices::KeyboardHelper->PressKeySequence(L"1");
                button2_clickEvent->WaitForNoThrow(std::chrono::milliseconds(100));
                VERIFY_IS_FALSE(button1_clickEvent->HasFired());
            }
        }

        void BasicAccessKeysTests::BasicHotKey()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
            auto buttonAccessKeyShownEvent = std::make_shared<Event>();
            auto buttonAccessKeyShownRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayRequested);
            auto buttonAccessKeyHiddenEvent = std::make_shared<Event>();
            auto buttonAccessKeyHiddenRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayDismissed);

            RunOnUIThread([&]()
            {
                button->Content = "Button";
                button->AccessKey = L"a";
                button->ExitDisplayModeOnAccessKeyInvoked = false;
                clickRegistration.Attach(button, [&]() { buttonClickEvent->Set();});
                buttonAccessKeyShownRegistration.Attach(button, [&]() { buttonAccessKeyShownEvent->Set();});
                buttonAccessKeyHiddenRegistration.Attach(button, [&]() { buttonAccessKeyHiddenEvent->Set();});
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            LOG_OUTPUT(L"Verify firing AccessKeyInvokedEvent by firing the hotkey.");
            AccessKeyTestHelper::InjectAccessKey(L"$d$_alt#$d$_a#$u$_a#$u$_alt", button);
            buttonClickEvent->WaitForDefault();

            buttonAccessKeyShownEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(buttonAccessKeyShownEvent->HasFired());
            buttonAccessKeyHiddenEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(buttonAccessKeyHiddenEvent->HasFired());
        }

        void BasicAccessKeysTests::SettingAccessKeyOverridesFEAPAccessKey()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel;
            xaml_controls::Button^ btn;
            xaml_docs::Hyperlink^ hyperlink;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' AccessKey='A' Content='Button' ExitDisplayModeOnAccessKeyInvoked='false'/>"
                    L"    <TextBlock><Hyperlink x:Name='hyperlink' AccessKey='Q' ExitDisplayModeOnAccessKeyInvoked='false'>Hyperlink</Hyperlink></TextBlock>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                hyperlink = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink"));

                xaml_automation_peers::AutomationPeer^ buttonAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(btn);

                Platform::String^ expectedAPAccessKeyMessage = L"Alt, " + btn->AccessKey;

                VERIFY_ARE_EQUAL(expectedAPAccessKeyMessage, buttonAP->GetAccessKey());
            });
        }

        void BasicAccessKeysTests::SettingAccessKeyDoesNotOverrideWhenSetOnFEAP()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel;
            xaml_controls::Button^ btn;
            xaml_docs::Hyperlink^ hyperlink;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' AccessKey='A' AutomationProperties.AccessKey='S' ExitDisplayModeOnAccessKeyInvoked='false' Content='Button' />"
                    L"    <TextBlock><Hyperlink x:Name='hyperlink' AutomationProperties.AccessKey='Q' ExitDisplayModeOnAccessKeyInvoked='false'>Hyperlink</Hyperlink></TextBlock>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));

                xaml_automation_peers::AutomationPeer^ buttonAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(btn);

                VERIFY_IS_TRUE(buttonAP->GetAccessKey()->Equals(L"S"));
            });
        }

        void BasicAccessKeysTests::WindowMoveEndsAKSequence()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->MaximizeDesktopWindow();
                TestServices::WindowHelper->WaitForIdle();
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            TestServices::WindowHelper->SetDesktopWindowSize(600, 500);
            TestServices::WindowHelper->MoveDesktopWindow(50, 50);
            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::StackPanel^ rootPanel;
            xaml_controls::Button^ button = nullptr;

            auto accessKeyShownEvent = std::make_shared<Event>();
            auto accessKeyShownRegistration = CreateSafeEventRegistration(xaml_controls::Button, AccessKeyDisplayRequested);
            std::shared_ptr<Event> accessKeyHiddenEvent = std::make_shared<Event>();
            auto accessKeyHiddenRegistration = CreateSafeEventRegistration(xaml_controls::Button, AccessKeyDisplayDismissed);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' AccessKey='1' Content='Button' />"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                accessKeyShownRegistration.Attach(button, [accessKeyShownEvent]()
                {
                    accessKeyShownEvent->Set();
                });
                accessKeyHiddenRegistration.Attach(button, [&]()
                {
                    LOG_OUTPUT(L"AccessKeyHidden event fired!");
                    accessKeyHiddenEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            const auto hwnd = ::GetAncestor(reinterpret_cast<HWND>(TestServices::WindowHelper->GetUIAWindowHandle()), GA_ROOT);
            RECT before = {};
            VERIFY_IS_TRUE(::GetWindowRect(hwnd, &before));

            LOG_OUTPUT(L"Entering access key mode");
            TestServices::KeyboardHelper->Alt();
            accessKeyShownEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_FALSE(accessKeyHiddenEvent->HasFired());

            TestServices::WindowHelper->MoveDesktopWindow(before.left, before.top);
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_FALSE(accessKeyHiddenEvent->HasFired());

            TestServices::WindowHelper->MoveDesktopWindow(before.left + 10, before.top);
            RECT after = {};
            VERIFY_IS_TRUE(::GetWindowRect(hwnd, &after));
            VERIFY_ARE_EQUAL(before.left + 10, after.left);
            VERIFY_ARE_EQUAL(before.top, after.top);
            VERIFY_ARE_EQUAL(before.right - before.left, after.right - after.left);
            VERIFY_ARE_EQUAL(before.bottom - before.top, after.bottom - after.top);

            accessKeyHiddenEvent->WaitForDefault();
        }

        void BasicAccessKeysTests::WindowResizeEndsAKSequence()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->MaximizeDesktopWindow();
                TestServices::WindowHelper->WaitForIdle();
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            TestServices::WindowHelper->SetDesktopWindowSize(600, 500);
            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::StackPanel^ rootPanel;
            xaml_controls::Button^ button = nullptr;

            auto accessKeyShownEvent = std::make_shared<Event>();
            auto accessKeyShownRegistration = CreateSafeEventRegistration(xaml_controls::Button, AccessKeyDisplayRequested);
            std::shared_ptr<Event> accessKeyHiddenEvent = std::make_shared<Event>();
            auto accessKeyHiddenRegistration = CreateSafeEventRegistration(xaml_controls::Button, AccessKeyDisplayDismissed);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' AccessKey='1' Content='Button' />"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                accessKeyShownRegistration.Attach(button, [accessKeyShownEvent]()
                {
                    accessKeyShownEvent->Set();
                });
                accessKeyHiddenRegistration.Attach(button, [&]()
                {
                    LOG_OUTPUT(L"AccessKeyHidden event fired!");
                    accessKeyHiddenEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            const auto hwnd = ::GetAncestor(reinterpret_cast<HWND>(TestServices::WindowHelper->GetUIAWindowHandle()), GA_ROOT);
            RECT before = {};
            VERIFY_IS_TRUE(::GetWindowRect(hwnd, &before));

            LOG_OUTPUT(L"Entering access key mode");
            TestServices::KeyboardHelper->Alt();
            accessKeyShownEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_FALSE(accessKeyHiddenEvent->HasFired());

            const auto width = (before.right - before.left) / 2;
            const auto height = (before.bottom - before.top) / 2;
            TestServices::WindowHelper->SetDesktopWindowSize(width, height);
            RECT after = {};
            VERIFY_IS_TRUE(::GetWindowRect(hwnd, &after));
            VERIFY_ARE_EQUAL(width, after.right - after.left);
            VERIFY_ARE_EQUAL(height, after.bottom - after.top);

            accessKeyHiddenEvent->WaitForDefault();
        }

        void BasicAccessKeysTests::AltKeyCodesDoNotFireAccessKeys()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel;
            xaml_controls::Button^ button;

            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' AccessKey='1' Content='Button' />"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                clickRegistration.Attach(button, [&]()
                {
                    LOG_OUTPUT(L"Btn click event fired!");
                    buttonClickEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            AccessKeyTestHelper::TryMovingFocusToXamlForInit();

            //Numpad1 should not fire AccessKeys when Numlock is off
            LOG_OUTPUT(L"Injecting Alt, then Numpad 1 with Numlock Off");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$u$_alt#$d$_n1#$u$_n1");
            buttonClickEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(buttonClickEvent->HasFired());

            LOG_OUTPUT(L"Turning on NumLock");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_numlock#$u$_numlock");

            auto scopeGuard = wil::scope_exit([&]
            {
                LOG_OUTPUT(L"Turning off NumLock");
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_numlock#$u$_numlock");
                TestServices::WindowHelper->WaitForIdle();
            });

            TestServices::WindowHelper->WaitForIdle();

            // This sequence should not fire access keys, as we are using the Numpad in hot-key mode.
            LOG_OUTPUT(L"Injecting Alt + Numpad1");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_n1#$u$_n1#$u$_alt");
            buttonClickEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(buttonClickEvent->HasFired());

            // This sequence should fire access keys, as we are using number keys in hot-key mode.
            LOG_OUTPUT(L"Injecting Alt + 1");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_1#$u$_1#$u$_alt");
            LOG_OUTPUT(L"Expecting Button click");
            buttonClickEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                buttonClickEvent->Reset();
            });
            TestServices::WindowHelper->WaitForIdle();

            // In regular access key mode, numpad should fire access keys.
            LOG_OUTPUT(L"Injecting Alt, then Numpad 1");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$u$_alt#$d$_n1#$u$_n1");
            LOG_OUTPUT(L"Expecting Button click");
            buttonClickEvent->WaitForDefault();
        }
    }
    }
} } }
