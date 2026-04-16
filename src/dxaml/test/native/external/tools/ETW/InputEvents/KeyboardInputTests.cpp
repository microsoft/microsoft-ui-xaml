// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "KeyboardInputTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include "TraceConsumerSession.h"
#include "MUX-ETWEvents.h"

using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace test_infra;

// Note: This constant is the minimum number of elements in a tree for these tests. If you are making changes to the tree structure
//  and these tests are failing, change this value here.
const int MIN_ELEMENTS_IN_TREE = 4;
const int ELEMENTS_ADDED_TO_TREE = 3;
const unsigned int NUM_EVENTS = 8;
namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace ETW { namespace InputEvents {

    bool KeyboardInputTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool KeyboardInputTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //------------------------------------------------------------------------
    // Test case: Send a simple key press to a Button and confirm it gets the
    //            KeyDown, KeyUp events
    //------------------------------------------------------------------------
    void KeyboardInputTests::KeyTests()
    {
        TestCleanupWrapper cleanup;

        std::shared_ptr<Event> buttonKeyDownEvent = std::make_shared<Event>();
        std::shared_ptr<Event> buttonKeyUpEvent = std::make_shared<Event>();

        Button^ btn = nullptr;
        auto keyDownRegistration = CreateSafeEventRegistration(UIElement, KeyDown);
        auto keyUpRegistration = CreateSafeEventRegistration(UIElement, KeyUp);
        auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);

        int buttonGotFocusCount = 0;

        Platform::String ^strToType = "h";
        RunOnUIThread([&]()
        {
            StackPanel^ mainStackPanel = ref new StackPanel();

            btn = ref new Button();
            btn->Width = 150;
            btn->Height = 50;
            btn->Content = "Button";
            btn->HorizontalAlignment = HorizontalAlignment::Center;

            mainStackPanel->Children->Append(btn);
            TestServices::WindowHelper->WindowContent = mainStackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            keyDownRegistration.Attach(btn,
                ref new KeyEventHandler([buttonKeyDownEvent](Platform::Object^, KeyRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"Button KeyDown event");
                buttonKeyDownEvent->Set();
            }));
            keyUpRegistration.Attach(btn,
                ref new KeyEventHandler([buttonKeyUpEvent](Platform::Object^, KeyRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"Button KeyUp event");
                buttonKeyUpEvent->Set();
            }));

            gotFocusRegistration.Attach(btn,
                ref new RoutedEventHandler([&buttonGotFocusCount](Platform::Object^, RoutedEventArgs^)
            {
                ++buttonGotFocusCount;
                LOG_OUTPUT(L"Button GotFocus event");
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            btn->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();
        VERIFY_ARE_EQUAL(buttonGotFocusCount, 1); // should have focus now

        TraceConsumerSession traceSession(WINDOWS_UI_XAML_ETW_PROVIDER);

        TraceConsumer::EnableTracingByEventId(KeyDownBegin_value);
        TraceConsumer::EnableTracingByEventId(KeyDownEnd_value);
        TraceConsumer::EnableTracingByEventId(KeyUpBegin_value);
        TraceConsumer::EnableTracingByEventId(KeyUpEnd_value);
        TraceConsumer::EnableTracingByEventId(KeyUpHandlerBegin_value);
        TraceConsumer::EnableTracingByEventId(KeyUpHandlerEnd_value);
        TraceConsumer::EnableTracingByEventId(KeyDownHandlerBegin_value);
        TraceConsumer::EnableTracingByEventId(KeyDownHandlerEnd_value);

        LOG_OUTPUT(L"Sending key");
        TestServices::KeyboardHelper->PressKeySequence(strToType);
        buttonKeyDownEvent->WaitForDefault();
        LOG_OUTPUT(L"Got KeyDown.");
        buttonKeyUpEvent->WaitForDefault();
        LOG_OUTPUT(L"Got KeyUp.");

        traceSession.Stop();

        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(KeyUpBegin_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(KeyUpEnd_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(KeyUpHandlerBegin_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(KeyUpHandlerEnd_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(KeyDownBegin_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(KeyDownEnd_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(KeyDownHandlerBegin_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(KeyDownHandlerEnd_value));

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }

} } } } } } }
