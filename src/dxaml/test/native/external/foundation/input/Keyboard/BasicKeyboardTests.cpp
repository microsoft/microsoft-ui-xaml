// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicKeyboardTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <FocusTestHelper.h>
#include <KeyboardInjectionOverride.h>

using namespace ::Windows::System; // for VirtualKey

using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Keyboard {

        enum EventType
        {
            KeyDown,
            KeyUp,
        };
        struct EventData
        {
            VirtualKey vk;
            bool isKeyReleased;
            bool wasKeyDown;
            EventType type;
        };

        bool BasicKeyboardTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool BasicKeyboardTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool BasicKeyboardTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Send a simple key press to a Button and confirm it gets the
        //            KeyDown and KeyUp events with the correct VirtualKey code.
        //            Then send a space key to the button to confirm it correctly
        //            results in a Click event, and that KeyDown and KeyUp get
        //            handled by the button (and therefore aren't visible to a
        //            regular "unhandled events only" event listener).
        //            Finally send an arrow key to the button to confirm it results
        //            in single KeyDown and KeyUp events.
        //------------------------------------------------------------------------
        void BasicKeyboardTests::BasicKeyDownKeyUp()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> buttonKeyDownEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonKeyUpEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();

            Button^ btn = nullptr;
            Button^ btnIslandFocusDummy = nullptr;
            auto keyDownRegistration = CreateSafeEventRegistration(UIElement, KeyDown);
            auto keyUpRegistration = CreateSafeEventRegistration(UIElement, KeyUp);
            auto clickRegistration = CreateSafeEventRegistration(Button, Click);
            auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);

            int buttonKeyDownCount = 0;
            int buttonKeyUpCount = 0;
            int buttonClickCount = 0;
            int buttonGotFocusCount = 0;

            StackPanel^ mainStackPanel = nullptr;
            Platform::String^ strToType = "h";
            VirtualKey virtualKeys[] = { VirtualKey::H };

            RunOnUIThread([&]()
            {
                mainStackPanel = ref new StackPanel();

                // due to the way focus works in Xaml Island, we need to move focus to island first before continuing with the test
                // this dummy button serves the purpose of moving focus to the island
                // in case of UAP mode, it is just another button which gets focus before the test scenario begins
                btnIslandFocusDummy = ref new Button();
                btnIslandFocusDummy->Width = 150;
                btnIslandFocusDummy->Height = 50;
                btnIslandFocusDummy->Content = "Button Island Focus";
                btnIslandFocusDummy->HorizontalAlignment = HorizontalAlignment::Center;

                btn = ref new Button();
                btn->Width = 150;
                btn->Height = 50;
                btn->Content = "Button";
                btn->HorizontalAlignment = HorizontalAlignment::Center;

                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn->HorizontalAlignment = HorizontalAlignment::Center;
                keyDownRegistration.Attach(btn,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyDown event");
                    VERIFY_IS_TRUE(buttonKeyDownCount < ARRAYSIZE(virtualKeys));
                    VERIFY_ARE_EQUAL(args->Key, virtualKeys[buttonKeyDownCount]);
                    buttonKeyDownCount++;
                    buttonKeyDownEvent->Set();
                }));
                keyUpRegistration.Attach(btn,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyUp event");
                    VERIFY_IS_TRUE(buttonKeyUpCount < ARRAYSIZE(virtualKeys));
                    VERIFY_ARE_EQUAL(args->Key, virtualKeys[buttonKeyUpCount]);
                    buttonKeyUpCount++;
                    buttonKeyUpEvent->Set();
                }));
                clickRegistration.Attach(btn,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button Click event");
                    buttonClickCount++;
                    buttonClickEvent->Set();
                }));
                gotFocusRegistration.Attach(btn,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus event");
                    buttonGotFocusCount++;
                }));

                // add buttons to the main stack panel
                mainStackPanel->Children->Append(btnIslandFocusDummy);
                mainStackPanel->Children->Append(btn);
            });
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus( btnIslandFocusDummy, FocusState::Keyboard );

            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(buttonGotFocusCount, 1); // should have focus now

            LOG_OUTPUT(L"Sending h key");
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            buttonKeyDownEvent->WaitForDefault();
            LOG_OUTPUT(L"Got KeyDown.");
            buttonKeyUpEvent->WaitForDefault();
            LOG_OUTPUT(L"Got KeyUp.");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(buttonKeyDownCount, 1);
                VERIFY_ARE_EQUAL(buttonKeyUpCount, 1);
                VERIFY_ARE_EQUAL(buttonClickCount, 0);
                VERIFY_ARE_EQUAL(buttonGotFocusCount, 1);
            });

            // Reset state and send a space key to see if it correctly triggers Click event.
            // Button will consume the key events.
            strToType = " ";
            virtualKeys[0] = VirtualKey::Space;
            buttonKeyDownCount = 0;
            buttonKeyUpCount = 0;
            buttonClickCount = 0;
            buttonGotFocusCount = 0;

            LOG_OUTPUT(L"Sending space key");
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            buttonClickEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(buttonKeyDownCount, 0);
                VERIFY_ARE_EQUAL(buttonKeyUpCount, 0);
                VERIFY_ARE_EQUAL(buttonClickCount, 1);
                VERIFY_ARE_EQUAL(buttonGotFocusCount, 0);
            });

            // Reset state and send left arrow key to verify exactly one KeyDown + KeyUp combination is triggered.
            virtualKeys[0] = VirtualKey::Left;
            buttonKeyDownCount = 0;
            buttonKeyUpCount = 0;
            buttonClickCount = 0;
            buttonGotFocusCount = 0;

            LOG_OUTPUT(L"Sending left key");
            TestServices::KeyboardHelper->Left();
            buttonKeyDownEvent->WaitForDefault();
            LOG_OUTPUT(L"Got KeyDown.");
            buttonKeyUpEvent->WaitForDefault();
            LOG_OUTPUT(L"Got KeyUp.");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(buttonKeyDownCount, 1);
                VERIFY_ARE_EQUAL(buttonKeyUpCount, 1);
                VERIFY_ARE_EQUAL(buttonClickCount, 0);
                VERIFY_ARE_EQUAL(buttonGotFocusCount, 0);
            });
        }

        //------------------------------------------------------------------------
        // Test case: Send multiple key presses to a Button, including a capital
        //            letter, confirming the multiple KeyDown and KeyUp events
        //            get delivered in the right order and with the right key
        //            event arg data.
        //------------------------------------------------------------------------
        void BasicKeyboardTests::KeyEventArgs()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> buttonGotFocusEvent = std::make_shared<Event>();
            std::shared_ptr<Event> lastKeyReceivedEvent = std::make_shared<Event>();

            Button^ btn = nullptr;
            auto keyDownRegistration = CreateSafeEventRegistration(UIElement, KeyDown);
            auto keyUpRegistration = CreateSafeEventRegistration(UIElement, KeyUp);
            auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);

            // Send key sequence for typing "Hiii", holding the 'i' down a bit longer
            Platform::String^ strToType = "$d$_shift#$d$_h#$u$_h#$u$_shift#$d$_i#$d$_i#$d$_i#$u$_i";

            EventData keys[] =
            {
                { VirtualKey::Shift, false, false, EventType::KeyDown },
                { VirtualKey::H,     false, false, EventType::KeyDown },
                { VirtualKey::H,     true,  true,  EventType::KeyUp },
                { VirtualKey::Shift, true,  true,  EventType::KeyUp },
                { VirtualKey::I,     false, false, EventType::KeyDown },
                { VirtualKey::I,     false, true,  EventType::KeyDown },
                { VirtualKey::I,     false, true,  EventType::KeyDown },
                { VirtualKey::I,     true,  true,  EventType::KeyUp },
            };
            int keyEvent = 0;

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();

                btn = ref new Button();
                btn->Width = 150;
                btn->Height = 50;
                btn->Content = "Button";
                btn->HorizontalAlignment = HorizontalAlignment::Center;
                keyDownRegistration.Attach(btn,
                    ref new KeyEventHandler([&keyEvent, &keys](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyDown event");
                    VERIFY_IS_TRUE(keyEvent < ARRAYSIZE(keys));
                    VERIFY_ARE_EQUAL(EventType::KeyDown, keys[keyEvent].type);
                    VERIFY_ARE_EQUAL(args->Key, keys[keyEvent].vk);
                    VERIFY_ARE_EQUAL(args->KeyStatus.IsKeyReleased, keys[keyEvent].isKeyReleased);
                    VERIFY_ARE_EQUAL(args->KeyStatus.WasKeyDown, keys[keyEvent].wasKeyDown);
                    keyEvent++;
                }));
                keyUpRegistration.Attach(btn,
                    ref new KeyEventHandler([lastKeyReceivedEvent, &keyEvent, &keys](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyUp event");
                    VERIFY_IS_TRUE(keyEvent < ARRAYSIZE(keys));
                    VERIFY_ARE_EQUAL(EventType::KeyUp, keys[keyEvent].type);
                    VERIFY_ARE_EQUAL(args->Key, keys[keyEvent].vk);
                    VERIFY_ARE_EQUAL(args->KeyStatus.IsKeyReleased, keys[keyEvent].isKeyReleased);
                    VERIFY_ARE_EQUAL(args->KeyStatus.WasKeyDown, keys[keyEvent].wasKeyDown);
                    keyEvent++;

                    if (keyEvent == ARRAYSIZE(keys))
                    {
                        lastKeyReceivedEvent->Set();
                    }
                }));
                gotFocusRegistration.Attach(btn,
                    ref new RoutedEventHandler([buttonGotFocusEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus event");
                    buttonGotFocusEvent->Set();
                }));

                mainStackPanel->Children->Append(btn);

                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Keyboard);
            });

            buttonGotFocusEvent->WaitForDefault();

            KeyboardInjectionIgnoreEventWaitOverride keyboardWaitOverride;

            LOG_OUTPUT(L"Sending key");
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            lastKeyReceivedEvent->WaitForDefault();
            VERIFY_ARE_EQUAL(keyEvent, (int)ARRAYSIZE(keys));
        }

        void BasicKeyboardTests::VerifyDeviceIdEqualsEmptyString()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> buttonKeyDownEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonGotFocusEvent = std::make_shared<Event>();

            Button^ btn = nullptr;
            auto keyDownRegistration = CreateSafeEventRegistration(UIElement, KeyDown);
            auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();

                btn = ref new Button();
                btn->Width = 150;
                btn->Height = 50;
                btn->Content = "Button";
                btn->HorizontalAlignment = HorizontalAlignment::Center;

                keyDownRegistration.Attach(btn,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyDown event");
                    VERIFY_IS_TRUE(args->DeviceId == L"");
                    buttonKeyDownEvent->Set();
                }));

                gotFocusRegistration.Attach(btn,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus event");
                    buttonGotFocusEvent->Set();
                }));

                mainStackPanel->Children->Append(btn);
                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Keyboard);
            });

            buttonGotFocusEvent->WaitForDefault();
            TestServices::KeyboardHelper->PressKeySequence(L"a");

            buttonKeyDownEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonKeyDownEvent->HasFired());
        }

        void BasicKeyboardTests::VerifyKeyStatus()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> buttonKeyDownEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonKeyUpEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonGotFocusEvent = std::make_shared<Event>();

            Button^ btn = nullptr;
            auto keyDownRegistration = CreateSafeEventRegistration(UIElement, KeyDown);
            auto keyUpRegistration = CreateSafeEventRegistration(UIElement, KeyUp);
            auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();

                btn = ref new Button();
                btn->Width = 150;
                btn->Height = 50;
                btn->Content = "Button";
                btn->HorizontalAlignment = HorizontalAlignment::Center;

                keyDownRegistration.Attach(btn,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyDown event");

                    VERIFY_IS_TRUE(btn->Equals(args->OriginalSource));
                    VERIFY_IS_FALSE(args->Handled);
                    VERIFY_ARE_EQUAL(args->OriginalKey, ::Windows::System::VirtualKey::A);
                    VERIFY_ARE_EQUAL(args->Key, ::Windows::System::VirtualKey::A);

                    VERIFY_IS_FALSE(args->KeyStatus.IsExtendedKey);
                    VERIFY_IS_FALSE(args->KeyStatus.IsKeyReleased);
                    VERIFY_IS_FALSE(args->KeyStatus.IsMenuKeyDown);
                    VERIFY_IS_FALSE(args->KeyStatus.WasKeyDown);
                    VERIFY_IS_TRUE(args->KeyStatus.RepeatCount == 1);
                    VERIFY_IS_TRUE(args->KeyStatus.ScanCode == 30);

                    buttonKeyDownEvent->Set();
                }));

                keyUpRegistration.Attach(btn,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyUp event");

                    VERIFY_IS_TRUE(btn->Equals(args->OriginalSource));
                    VERIFY_IS_FALSE(args->Handled);
                    VERIFY_ARE_EQUAL(args->OriginalKey, ::Windows::System::VirtualKey::A);
                    VERIFY_ARE_EQUAL(args->Key, ::Windows::System::VirtualKey::A);

                    VERIFY_IS_FALSE(args->KeyStatus.IsExtendedKey);
                    VERIFY_IS_FALSE(args->KeyStatus.IsMenuKeyDown);
                    VERIFY_IS_TRUE(args->KeyStatus.IsKeyReleased);
                    VERIFY_IS_TRUE(args->KeyStatus.WasKeyDown);
                    VERIFY_IS_TRUE(args->KeyStatus.RepeatCount == 1);
                    VERIFY_IS_TRUE(args->KeyStatus.ScanCode == 30);

                    buttonKeyUpEvent->Set();
                }));

                gotFocusRegistration.Attach(btn,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus event");
                    buttonGotFocusEvent->Set();
                }));

                mainStackPanel->Children->Append(btn);
                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Keyboard);
            });

            buttonGotFocusEvent->WaitForDefault();
            TestServices::KeyboardHelper->PressKeySequence(L"a");

            buttonKeyDownEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonKeyDownEvent->HasFired());

            buttonKeyUpEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonKeyUpEvent->HasFired());
        }

        void BasicKeyboardTests::VerifyIsMenuKeyDown()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> buttonKeyDownEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonKeyUpEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonGotFocusEvent = std::make_shared<Event>();

            Button^ btn = nullptr;
            auto keyDownRegistration = CreateSafeEventRegistration(UIElement, KeyDown);
            auto keyUpRegistration = CreateSafeEventRegistration(UIElement, KeyUp);
            auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);

            int count = 0;

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();

                btn = ref new Button();
                btn->Width = 150;
                btn->Height = 50;
                btn->Content = "Button";
                btn->HorizontalAlignment = HorizontalAlignment::Center;

                keyDownRegistration.Attach(btn,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyDown event");

                    if (count == 0)
                    {
                        VERIFY_IS_TRUE(args->KeyStatus.IsMenuKeyDown);
                        VERIFY_IS_TRUE(args->KeyStatus.IsExtendedKey);
                    }
                    else if (count == 1)
                    {
                        VERIFY_IS_TRUE(args->KeyStatus.IsMenuKeyDown);
                        VERIFY_IS_FALSE(args->KeyStatus.IsExtendedKey);
                    }

                    buttonKeyDownEvent->Set();
                }));

                keyUpRegistration.Attach(btn,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyUp event");

                    if (count == 0)
                    {
                        VERIFY_IS_TRUE(args->KeyStatus.IsMenuKeyDown);
                        VERIFY_IS_TRUE(args->KeyStatus.IsExtendedKey);
                    }
                    else if (count == 1)
                    {
                        VERIFY_IS_TRUE(args->KeyStatus.IsMenuKeyDown);
                        VERIFY_IS_FALSE(args->KeyStatus.IsExtendedKey);
                    }

                    count++;

                    buttonKeyUpEvent->Set();
                }));

                gotFocusRegistration.Attach(btn,
                    ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus event");
                    buttonGotFocusEvent->Set();
                }));

                mainStackPanel->Children->Append(btn);
                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Keyboard);
            });

            buttonGotFocusEvent->WaitForDefault();
            TestServices::KeyboardHelper->PressKeySequence("$d$_ralt#$u$_ralt");

            buttonKeyDownEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonKeyDownEvent->HasFired());

            buttonKeyUpEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonKeyUpEvent->HasFired());

            buttonKeyDownEvent->Reset();
            buttonKeyUpEvent->Reset();

            TestServices::KeyboardHelper->Alt();

            buttonKeyDownEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonKeyDownEvent->HasFired());

            buttonKeyUpEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonKeyUpEvent->HasFired());
        }

        void BasicKeyboardTests::VerifyCtrlAltKeys()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> buttonGotFocusEvent = std::make_shared<Event>();
            std::shared_ptr<Event> lastKeyReceivedEvent = std::make_shared<Event>();

            Button^ btn = nullptr;
            auto keyDownRegistration = CreateSafeEventRegistration(UIElement, KeyDown);
            auto keyUpRegistration = CreateSafeEventRegistration(UIElement, KeyUp);

            Platform::String^ strToType = "$d$_ctrl#$d$_alt#$d$_e#$u$_e#$u$_ctrl#$u$_alt";

            EventData keys[] =
            {
                { VirtualKey::Control, false, false, EventType::KeyDown },
                { VirtualKey::Menu, false, false, EventType::KeyDown },
                { VirtualKey::E, false, false, EventType::KeyDown },
                { VirtualKey::E, true, true,  EventType::KeyUp },
                { VirtualKey::Control, true, true, EventType::KeyUp },
                { VirtualKey::Menu, true, true, EventType::KeyUp },
            };
            int keyEvent = 0;

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();

                btn = ref new Button();
                btn->Width = 150;
                btn->Height = 50;
                btn->Content = "Button";
                btn->HorizontalAlignment = HorizontalAlignment::Center;
                keyDownRegistration.Attach(btn,
                    ref new KeyEventHandler([&keyEvent, &keys](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyDown event");
                    VERIFY_IS_TRUE(keyEvent < ARRAYSIZE(keys));
                    VERIFY_ARE_EQUAL(EventType::KeyDown, keys[keyEvent].type);
                    VERIFY_ARE_EQUAL(args->Key, keys[keyEvent].vk);
                    VERIFY_ARE_EQUAL(args->KeyStatus.IsKeyReleased, keys[keyEvent].isKeyReleased);
                    VERIFY_ARE_EQUAL(args->KeyStatus.WasKeyDown, keys[keyEvent].wasKeyDown);
                    keyEvent++;
                }));
                keyUpRegistration.Attach(btn,
                    ref new KeyEventHandler([lastKeyReceivedEvent, &keyEvent, &keys](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button KeyUp event");
                    VERIFY_IS_TRUE(keyEvent < ARRAYSIZE(keys));
                    VERIFY_ARE_EQUAL(EventType::KeyUp, keys[keyEvent].type);
                    VERIFY_ARE_EQUAL(args->Key, keys[keyEvent].vk);
                    VERIFY_ARE_EQUAL(args->KeyStatus.IsKeyReleased, keys[keyEvent].isKeyReleased);
                    VERIFY_ARE_EQUAL(args->KeyStatus.WasKeyDown, keys[keyEvent].wasKeyDown);
                    keyEvent++;

                    if (keyEvent == ARRAYSIZE(keys))
                    {
                        lastKeyReceivedEvent->Set();
                    }
                }));

                mainStackPanel->Children->Append(btn);

                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(btn, FocusState::Keyboard);

            LOG_OUTPUT(L"Sending key");
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            lastKeyReceivedEvent->WaitForDefault();
            VERIFY_ARE_EQUAL(keyEvent, (int)ARRAYSIZE(keys));
       }

        void BasicKeyboardTests::EnterKeyDownKeyUpOnButton()
        {
            TestCleanupWrapper cleanup;
            Button^ btn = nullptr;

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();
                btn = ref new Button();
                btn->Content = ref new Platform::String(L"Click");
                mainStackPanel->Children->Append(btn);
                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnterKeyDownKeyUpHelper(btn);
        }

        void BasicKeyboardTests::EnterKeyDownKeyUpOnTextBox()
        {
            TestCleanupWrapper cleanup;
            TextBox^ textbox = nullptr;

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();
                textbox = ref new TextBox();
                textbox->Text = ref new Platform::String(L"TextBox");
                mainStackPanel->Children->Append(textbox);
                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnterKeyDownKeyUpHelper(textbox);
        }

        void BasicKeyboardTests::EnterKeyDownKeyUpHelper(xaml_controls::Control^ control)
        {
            std::shared_ptr<Event> controlKeyDownEvent = std::make_shared<Event>();
            std::shared_ptr<Event> controlKeyUpEvent = std::make_shared<Event>();

            auto keyDownRegistration = CreateSafeEventRegistrationForHandledEvents(UIElement, KeyDownEvent);
            auto keyUpRegistration = CreateSafeEventRegistrationForHandledEvents(UIElement, KeyUpEvent);

            int controlKeyDownCount = 0;
            int controlKeyUpCount = 0;

            RunOnUIThread([&]()
            {
                keyDownRegistration.Attach(control,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Control KeyDown event");
                    controlKeyDownCount++;
                    controlKeyDownEvent->Set();
                }));
                keyUpRegistration.Attach(control,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Control KeyUp event");
                    controlKeyUpCount++;
                    controlKeyUpEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(control, FocusState::Keyboard);

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Sending Enter Key");
            TestServices::KeyboardHelper->Enter();
            controlKeyDownEvent->WaitForDefault();
            LOG_OUTPUT(L"Got KeyDown.");
            controlKeyUpEvent->WaitForDefault();
            LOG_OUTPUT(L"Got KeyUp.");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(controlKeyDownCount, 1);
                VERIFY_ARE_EQUAL(controlKeyUpCount, 1);
            });
        }

        void BasicKeyboardTests::F10KeyDown()
        {
            TestCleanupWrapper cleanup;
            Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();
                button = ref new Button();
                button->Content = ref new Platform::String(L"Click");
                mainStackPanel->Children->Append(button);
                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto keyDownEvent = std::make_shared<Event>();
            auto keyDownRegistration = CreateSafeEventRegistrationForHandledEvents(UIElement, KeyDownEvent);

            RunOnUIThread([&]()
            {
                keyDownRegistration.Attach(button,
                    ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Got KeyDown");
                    keyDownEvent->Set();
                }));
            });

            FocusTestHelper::EnsureFocus(button, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->PressKeySequence("f10");
            TestServices::WindowHelper->WaitForIdle();

            keyDownEvent->WaitForDefault();
        }
    } } }
} } } }
