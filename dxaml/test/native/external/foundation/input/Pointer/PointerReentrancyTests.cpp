// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PointerReentrancyTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Pointer {

        bool PointerReentrancyTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool PointerReentrancyTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool PointerReentrancyTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void PointerReentrancyTests::NestedMessagePumpInButtonClick()
        {
            TestCleanupWrapper cleanup;
            Button^ button;
            auto buttonClickRegistration = CreateSafeEventRegistration(Button, Click);

            Button^ button2;
            auto button2ClickRegistration = CreateSafeEventRegistration(Button, Click);

            TextBox^ textBox;
            ComboBox^ comboBox;

            auto comboBoxOpenedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);

            auto testDoneEvent = std::make_shared<Event>();

            std::atomic<int> clickEventsReceived {0};

            RunOnUIThread([&]()
            {
                auto panel = ref new StackPanel();

                button = ref new Button();
                button->Width = 200.0;
                button->Height = 50.0;
                button->Content = "Xaml Button";
                panel->Children->Append(button);

                buttonClickRegistration.Attach(button,
                    ref new RoutedEventHandler([button,testDoneEvent,&clickEventsReceived](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button Click event");
                    button->Content = "Running nested message pump...";
                    LOG_OUTPUT(L">>> Start nested pump.\n");

                    MSG msg{};
                    while (GetMessage(&msg, nullptr, 0, 0))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);

                        if (clickEventsReceived >= 2)
                        {
                            break;
                        }
                    }
                    LOG_OUTPUT(L">>> End nested pump.\n"); 
                    testDoneEvent->Set();             
                }));

                button2 = ref new Button();
                button2->Width = 200.0;
                button2->Height = 50.0;
                button2->Content = "button2";
                panel->Children->Append(button2);

                textBox = ref new TextBox();
                textBox->Width = 200.0;
                textBox->Height = 50.0;
                textBox->Text = "I am a textbox";
                panel->Children->Append(textBox);

                comboBox = ref new ComboBox();
                comboBox->Items->Append(L"Item 1");
                comboBox->Items->Append(L"Item 2");
                comboBox->Items->Append(L"Item 3");
                openedRegistration.Attach(comboBox, ref new wf::EventHandler<Platform::Object^>([comboBoxOpenedEvent](Platform::Object^, Platform::Object^) {
                    comboBoxOpenedEvent->Set();
                }));
                panel->Children->Append(comboBox);

                button2ClickRegistration.Attach(button2,
                    ref new RoutedEventHandler([button2,&clickEventsReceived](Platform::Object^, RoutedEventArgs^)
                {
                    clickEventsReceived++;
                    wchar_t str[100];
                    ::StringCchPrintf(str, ARRAYSIZE(str), L"button2: %d click(s)", static_cast<int>(clickEventsReceived));
                    button2->Content = Platform::StringReference(str);
                }));

                TestServices::WindowHelper->WindowContent = panel;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Click to button to trigger nested message pump.\n"); 
            TestServices::InputHelper->ClickMouseButton(MouseButton::Left, button);

            constexpr int mouseDelay {100};

            const DWORD startTime = ::GetTickCount();
            const DWORD timeoutTime = startTime + 60000;
            int round = 0;

            while (!testDoneEvent->HasFired() && ::GetTickCount() < timeoutTime)
            {
                ++round;
                LOG_OUTPUT(L"Round %d: clickEventsReceived = %d", round, static_cast<int>(clickEventsReceived));

                // Wiggle mouse and click elements a few times to get a bunch of pointer input events flowing through the system.
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(button2, 50, 50);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(button2, 0, 0);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(button2, 0, 50);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(button2, 0, 0);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->ClickMouseButton(MouseButton::Left, button2);

                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(textBox, 50, 50);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(textBox, 0, 0);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(textBox, 0, 50);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(textBox, 0, 0);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->ClickMouseButton(MouseButton::Left, textBox);

                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(comboBox, 50, 50);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(comboBox, 0, 0);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(comboBox, 0, 50);
                ::Sleep(mouseDelay);
                TestServices::InputHelper->MoveMouse(comboBox, 0, 0);
                ::Sleep(mouseDelay);
                comboBoxOpenedEvent->Reset();
                TestServices::InputHelper->ClickMouseButton(MouseButton::Left, comboBox);

                // Click again to close it. 
                comboBoxOpenedEvent->WaitForDefault();
                ::Sleep(mouseDelay);
                TestServices::InputHelper->ClickMouseButton(MouseButton::Left, comboBox);
            }

            testDoneEvent->WaitForDefault();

            // Validation is that we don't crash.
            // When I added this test, sadly I could only trigger the crash with pageheap enabled.  If we ever do
            // work to allow tests to enable pageheap, this one's a good candidate.
        }

    } } }
} } } }
