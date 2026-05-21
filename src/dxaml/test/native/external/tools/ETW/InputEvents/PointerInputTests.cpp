// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PointerInputTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include "MUX-ETWEvents.h"
#include "TraceConsumerSession.h"
#include <SafeEventRegistration.h>

using namespace Concurrency;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace Platform;

using namespace test_infra;
typedef std::pair<int, unsigned int> EVENT_ID_COUNT;

// Note: This constant is the minimum number of elements in a tree for these tests. If you are making changes to the tree structure
//  and these tests are failing, change this value here. The goal is for this to be zero!! Good luck :)
const int c_minElementsInTree = 4;
const int c_elementsAddedToTree = 3;

// This array contains the ids and expected counts for each event. This array is used to initialize a map which
// is then iterated on to enable each ID as well as verify the number fired matches.
EVENT_ID_COUNT idCounts[] = {
    EVENT_ID_COUNT(PointerPressedBegin_value, c_minElementsInTree + c_elementsAddedToTree),
    EVENT_ID_COUNT(PointerPressedEnd_value, c_minElementsInTree + c_elementsAddedToTree),
    EVENT_ID_COUNT(PointerEnteredBegin_value, c_minElementsInTree + c_elementsAddedToTree),
    EVENT_ID_COUNT(PointerEnteredEnd_value, c_minElementsInTree + c_elementsAddedToTree),
    EVENT_ID_COUNT(PointerReleasedBegin_value, c_minElementsInTree + c_elementsAddedToTree),
    EVENT_ID_COUNT(PointerReleasedEnd_value, c_minElementsInTree + c_elementsAddedToTree)
};

std::vector<EVENT_ID_COUNT> idVector(idCounts, idCounts + sizeof(idCounts) / sizeof(EVENT_ID_COUNT));
namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace ETW { namespace InputEvents {

    namespace etw = xaml::Tests::Tools::ETW::InputEvents;

    bool PointerInputTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PointerInputTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //------------------------------------------------------------------------
    // Test case: Drag a Rectangle in a Canvas with the left mouse button and
    // raw pointer events.
    //------------------------------------------------------------------------
    void PointerInputTests::PointerEvents()
    {
        TestCleanupWrapper cleanup;

        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> spPointerExitedEvent = std::make_shared<Event>();

        Canvas^ canvas = nullptr;
        Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;

        wf::EventRegistrationToken rectLoadedToken = {};
        wf::EventRegistrationToken rectPointerEnteredToken = {};
        wf::EventRegistrationToken rectPointerPressedToken = {};
        wf::EventRegistrationToken rectPointerReleasedToken = {};
        wf::EventRegistrationToken rectPointerExitedToken = {};

        // Create objects and subscribe to events on the UI thread
        RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                // Create a rectangle which will not have event handlers registered
                rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                rect->Width = 50;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                canvas->Children->Append(rect);

                rectLoadedToken = rect->Loaded +=
                    ref new xaml::RoutedEventHandler([&rectLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                        {
                            LOG_OUTPUT(L"Rectangle Loaded event");

                            rectLoadedEvent->Set();
                        });

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

        TestServices::WindowHelper->WaitForIdle();
        rectLoadedEvent->WaitForDefault();

        // This test is looking for specific pointer events in specific orders, however, if this is the first test being
        // run (or Xaml was shutdown after the last test), then InputHelper has to wait for a roundtrip from the system
        // compositor before it can send input and it does this by attempting to send dummy pointer input and seeing if it
        // gets it back.  When it does, it means that input is ready and it can carry out the input injection requested.
        // However, for this test, we don't want that dummy pointer input affect our test so we will send some dummy input
        // of our own to trigger this roundtrip check (if needed)
        {
            auto canvasTappedRegistration = CreateSafeEventRegistration(Canvas, Tapped);
            std::shared_ptr<Event> inputReadyEvent = std::make_shared<Event>();
            RunOnUIThread([&]()
                {
                    canvasTappedRegistration.Attach(canvas, ref new xaml_input::TappedEventHandler(
                        [inputReadyEvent](Platform::Object^, TappedRoutedEventArgs^ args)
                        {
                            inputReadyEvent->Set();
                        }));
                });
            TestServices::InputHelper->Tap(canvas);
            inputReadyEvent->WaitForDefault();
        }

        // Now that we know that input is ready and can proceed with setting up our pointer events and running the test
        RunOnUIThread([&]()
            {

                rectPointerEnteredToken = rect->PointerEntered +=
                    ref new PointerEventHandler([](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                        {
                            LOG_OUTPUT(L"Rectangle PointerEntered event");
                            args->Handled = true;
                        });

                rectPointerPressedToken = rect->PointerPressed +=
                    ref new PointerEventHandler([](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                        {
                            LOG_OUTPUT(L"Rectangle PointerPressed event");
                        });


                rectPointerReleasedToken = rect->PointerReleased +=
                    ref new PointerEventHandler([](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                        {
                            LOG_OUTPUT(L"Rectangle PointerReleased event");
                        });

                rectPointerExitedToken = rect->PointerExited +=
                    ref new PointerEventHandler([&spPointerExitedEvent](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                        {
                            LOG_OUTPUT(L"Rectangle PointerExited event");
                            spPointerExitedEvent->Set();
                        });
            });

        // Wrap TraceConsumer::Start in TraceConsumerSession for proper cleanup
        TraceConsumerSession traceSession(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);

        // Add events to be traced
        for (auto& id : idVector)
        {
            TraceConsumer::EnableTracingByEventId(id.first);
        }
        // We are going to test the pointer exited events separately because
        // we are failing in stress mode due to a test infrastructure issue. A pointer
        // exit events is still being processed as our binary is already unloaded.
        // These events get picked up in stress tests and cause the extra ETW event
        // to be fired. By not including in the vector, we are not specifying an expected value
        // for the event.
        TraceConsumer::EnableTracingByEventId(PointerExitedBegin_value);
        TraceConsumer::EnableTracingByEventId(PointerExitedEnd_value);


        LOG_OUTPUT(L"Testing mouse click");
        TestServices::InputHelper->LeftMouseClick(rect);
        TestServices::InputHelper->MoveMouse(wf::Point(200, 200));
        TestServices::WindowHelper->WaitForIdle();
        spPointerExitedEvent->WaitForDefault();

        // Stop the trace, this should only throw if incorrectly tracing
        traceSession.Stop();
        // Verify correct counts
        for (auto& id : idVector)
        {
            VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(id.first,id.second));
        }

        // Here we don't pass a second parameter to VerifyEventTraced (expected value). This means that
        // as long as we get at least one event, these will pass.
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(PointerExitedBegin_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(PointerExitedEnd_value));
        RunOnUIThread([&]()
        {

            rect->Loaded -= rectLoadedToken;
            rect->PointerEntered -= rectPointerEnteredToken;
            rect->PointerPressed -= rectPointerPressedToken;
            rect->PointerReleased -= rectPointerReleasedToken;
            rect->PointerExited -= rectPointerExitedToken;
        });

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();

    }

} } } } } } }