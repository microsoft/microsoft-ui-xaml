// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ManipulationInputTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include "MUX-ETWEvents.h"
#include "TraceConsumerSession.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

// Note: This constant is the minimum number of elements in a tree for these tests. If you are making changes to the tree structure
//  and these tests are failing, change this value here. Good luck :)
const int c_minElementsInTree = 4;
const int c_elementsAddedToTree = 3;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace ETW { namespace InputEvents {
    bool ManipulationInputTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ManipulationInputTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //------------------------------------------------------------------------
    // Test case: Pan a Rectangle horizontally with inertia using Gestures.
    //------------------------------------------------------------------------
    void ManipulationInputTests::ManipulationEvents()
    {
        TestCleanupWrapper cleanup;
        std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>();
        Canvas^ canvas = nullptr;
        Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
        wf::EventRegistrationToken rectManipulationStartingToken = {};
        wf::EventRegistrationToken rectManipulationInertiaStartingToken = {};
        wf::EventRegistrationToken rectManipulationStartedToken = {};
        wf::EventRegistrationToken rectManipulationDeltaToken = {};
        wf::EventRegistrationToken rectManipulationCompletedToken = {};

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

            rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
            rect->Width = 50;
            rect->Height = 60;
            rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
            rect->ManipulationMode = ManipulationModes::TranslateX | ManipulationModes::TranslateInertia;
            canvas->Children->Append(rect);

            rectManipulationStartingToken = rect->ManipulationStarting +=
                ref new xaml_input::ManipulationStartingEventHandler(
                [](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"ManipulationStarting, Mode: %d", args->Mode);
            });

            rectManipulationInertiaStartingToken = rect->ManipulationInertiaStarting +=
                ref new xaml_input::ManipulationInertiaStartingEventHandler(
                [canvas](Platform::Object^ sender, ManipulationInertiaStartingRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"ManipulationInertiaStarting");
            });

            rectManipulationStartedToken = rect->ManipulationStarted +=
                ref new xaml_input::ManipulationStartedEventHandler(
                [](Platform::Object^ sender, ManipulationStartedRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"ManipulationStarted");
            });

            rectManipulationDeltaToken = rect->ManipulationDelta +=
                ref new xaml_input::ManipulationDeltaEventHandler(
                [](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"Manipulation Delta");
            });

            rectManipulationCompletedToken = rect->ManipulationCompleted +=
                ref new xaml_input::ManipulationCompletedEventHandler(
                [&manipulationCompletedEvent](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"ManipulationCompleted");
                manipulationCompletedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = mainGrid;
        });

        // Pan the rectangle to the right with inertia.
        TestServices::WindowHelper->WaitForIdle();

        TraceConsumerSession traceSession(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        // Add events to be traced
        TraceConsumer::EnableTracingByEventId(ManipulationStartedBegin_value);
        TraceConsumer::EnableTracingByEventId(ManipulationStartedEnd_value);
        TraceConsumer::EnableTracingByEventId(ManipulationStartingBegin_value);
        TraceConsumer::EnableTracingByEventId(ManipulationStartingEnd_value);
        TraceConsumer::EnableTracingByEventId(ManipulationInertiaStartingBegin_value);
        TraceConsumer::EnableTracingByEventId(ManipulationInertiaStartingEnd_value);
        TraceConsumer::EnableTracingByEventId(ManipulationCompletedBegin_value);
        TraceConsumer::EnableTracingByEventId(ManipulationCompletedEnd_value);
        TraceConsumer::EnableTracingByEventId(ManipulationDeltaBegin_value);
        TraceConsumer::EnableTracingByEventId(ManipulationDeltaEnd_value);

        LOG_OUTPUT(L"Starting manipulation");
        TestServices::InputHelper->PanFromCenter(rect, 100 /*relX*/, 0 /*relY*/, 0.65 /*velocityFactor*/);
        LOG_OUTPUT(L"Waiting for event");
        manipulationCompletedEvent->WaitForDefault();
        LOG_OUTPUT(L"Got event");
        traceSession.Stop();

        // Verify correct counts
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationStartedBegin_value, c_minElementsInTree + c_elementsAddedToTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationStartedEnd_value, c_minElementsInTree + c_elementsAddedToTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationStartingBegin_value, c_minElementsInTree + c_elementsAddedToTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationStartingEnd_value, c_minElementsInTree + c_elementsAddedToTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationInertiaStartingBegin_value, c_minElementsInTree + c_elementsAddedToTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationInertiaStartingEnd_value, c_minElementsInTree + c_elementsAddedToTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationCompletedBegin_value, c_minElementsInTree + c_elementsAddedToTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationCompletedEnd_value, c_minElementsInTree + c_elementsAddedToTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationDeltaBegin_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ManipulationDeltaEnd_value));

        RunOnUIThread([&]()
        {
            rect->ManipulationStarting -= rectManipulationStartingToken;
            rect->ManipulationInertiaStarting -= rectManipulationInertiaStartingToken;
            rect->ManipulationStarted -= rectManipulationStartedToken;
            rect->ManipulationDelta -= rectManipulationDeltaToken;
            rect->ManipulationCompleted -= rectManipulationCompletedToken;
        });

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }
}}}}}}}
