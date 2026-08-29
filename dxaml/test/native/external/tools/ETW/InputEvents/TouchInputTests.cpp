// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TouchInputTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include "MUX-ETWEvents.h"
#include "TraceConsumerSession.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace Platform;
using namespace test_infra;

// Note: This constant is the minimum number of elements in a tree for these tests. If you are making changes to the tree structure
//  and these tests are failing, change this value here.
const int c_minElementsInTree = 4;
const int c_elementsAddedToTree = 3;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace ETW { namespace InputEvents {

    namespace etw = xaml::Tests::Tools::ETW::InputEvents;

    bool TouchInputTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool TouchInputTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //------------------------------------------------------------------------
    // Test case: Drag a Rectangle in a Canvas with the left mouse button and
    // raw pointer events.
    //------------------------------------------------------------------------
    void TouchInputTests::TappedEvent()
    {
        TestCleanupWrapper cleanup;
        std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        xaml_shapes::Rectangle^ rect = nullptr;
        xaml_controls::Grid^ mainGrid = nullptr;
        auto rectTappedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, Tapped);
        wf::EventRegistrationToken rectLoadedToken = {};
        RunOnUIThread([&]()
        {
            SetupElements(&rect, &mainGrid);
            VERIFY_IS_TRUE(rect->IsTapEnabled);
            rectLoadedToken = rect->Loaded +=
                ref new xaml::RoutedEventHandler([&rectLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Rectangle Loaded event");
                rectLoadedEvent->Set();
            });

            rectTappedRegistration.Attach(rect, ref new xaml_input::TappedEventHandler(
                [&gestureCompletedEvent](Platform::Object^, TappedRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"Tapped event raised.");
                gestureCompletedEvent->Set();
            }));
            TestServices::WindowHelper->WindowContent = mainGrid;
        });
        TestServices::WindowHelper->WaitForIdle();
        rectLoadedEvent->WaitForDefault();

        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        // Add events to be traced
        TraceConsumer::EnableTracingByEventId(TappedBegin_value);
        TraceConsumer::EnableTracingByEventId(TappedEnd_value);


        LOG_OUTPUT(L"Tapping the Rectangle.");
        TestServices::InputHelper->Tap(rect);

        gestureCompletedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        session.Stop();

        // Verify correct counts
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(TappedBegin_value, c_elementsAddedToTree+c_minElementsInTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(TappedEnd_value, c_elementsAddedToTree + c_minElementsInTree));

        RunOnUIThread([&]()
        {
            rect->Loaded -= rectLoadedToken;
        });
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }
    void TouchInputTests::DoubleTappedEvent()
    {
        TestCleanupWrapper cleanup;
        std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
        xaml_controls::Grid^ mainGrid = nullptr;
        auto rectTappedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, Tapped);
        auto rectDoubleTappedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, DoubleTapped);
        wf::EventRegistrationToken rectLoadedToken = {};
        RunOnUIThread([&]()
        {
            SetupElements(&rect,&mainGrid);
            VERIFY_IS_TRUE(rect->IsDoubleTapEnabled);

            rectLoadedToken = rect->Loaded +=
                ref new xaml::RoutedEventHandler([&rectLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Rectangle Loaded event");
                rectLoadedEvent->Set();
            });

            rectDoubleTappedRegistration.Attach(rect, ref new xaml_input::DoubleTappedEventHandler(
                [&gestureCompletedEvent](Platform::Object^, DoubleTappedRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"DoubleTapped event raised.");
                gestureCompletedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = mainGrid;
        });
        TestServices::WindowHelper->WaitForIdle();
        rectLoadedEvent->WaitForDefault();
        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);

        // Add events to be traced
        TraceConsumer::EnableTracingByEventId(TappedBegin_value);
        TraceConsumer::EnableTracingByEventId(TappedEnd_value);
        TraceConsumer::EnableTracingByEventId(DoubleTappedBegin_value);
        TraceConsumer::EnableTracingByEventId(DoubleTappedEnd_value);


        LOG_OUTPUT(L"Double-tapping the Rectangle.");
        TestServices::InputHelper->DoubleTap(rect);
        gestureCompletedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        session.Stop();

        // Verify correct counts
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DoubleTappedBegin_value, c_elementsAddedToTree + c_minElementsInTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DoubleTappedEnd_value, c_elementsAddedToTree + c_minElementsInTree));

        RunOnUIThread([&]()
        {
            rect->Loaded -= rectLoadedToken;
        });
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }

    void TouchInputTests::RightTappedEvent()
    {
        TestCleanupWrapper cleanup;
        std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        xaml_shapes::Rectangle^ rect = nullptr;
        xaml_controls::Grid^ mainGrid = nullptr;

        auto rectRightTappedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, RightTapped);
        wf::EventRegistrationToken rectLoadedToken = {};

        RunOnUIThread([&]()
        {
            SetupElements(&rect, &mainGrid);
            VERIFY_IS_TRUE(rect->IsRightTapEnabled);

            rectLoadedToken = rect->Loaded +=
                ref new xaml::RoutedEventHandler([&rectLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Rectangle Loaded event");
                rectLoadedEvent->Set();
            });

            rectRightTappedRegistration.Attach(rect, ref new xaml_input::RightTappedEventHandler(
                [&gestureCompletedEvent](Platform::Object^, RightTappedRoutedEventArgs^ args)
            {
                gestureCompletedEvent->Set();
            }));
            TestServices::WindowHelper->WindowContent = mainGrid;
        });


        rectLoadedEvent->WaitForDefault();

        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        TraceConsumer::EnableTracingByEventId(RightTappedBegin_value);
        TraceConsumer::EnableTracingByEventId(RightTappedEnd_value);

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Right-tapping the Rectangle.");
        // RightTapped is raised when the pointer is released at the end of a hold gesture.
        TestServices::InputHelper->Hold(rect);
        gestureCompletedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        session.Stop();
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(RightTappedBegin_value, c_elementsAddedToTree + c_minElementsInTree));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(RightTappedEnd_value, c_elementsAddedToTree + c_minElementsInTree));

        RunOnUIThread([&]()
        {
            rect->Loaded -= rectLoadedToken;
        });
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }

    void TouchInputTests::HoldingEvent()
    {
        TestCleanupWrapper cleanup;
        std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
        xaml_controls::Grid^ mainGrid = nullptr;
        auto rectHoldingRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, Holding);

        wf::EventRegistrationToken rectLoadedToken = {};

        RunOnUIThread([&]()
        {
            SetupElements(&rect, &mainGrid);
            VERIFY_IS_TRUE(rect->IsRightTapEnabled);

            rectLoadedToken = rect->Loaded +=
                ref new xaml::RoutedEventHandler([&rectLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Rectangle Loaded event");
                rectLoadedEvent->Set();
            });

            rectHoldingRegistration.Attach(rect, ref new xaml_input::HoldingEventHandler(
                [&gestureCompletedEvent](Platform::Object^, HoldingRoutedEventArgs^ args)
            {
                switch (args->HoldingState)
                {
                case Microsoft::UI::Input::HoldingState::Started:
                    break;
                case Microsoft::UI::Input::HoldingState::Completed:
                    gestureCompletedEvent->Set();
                    break;
                case Microsoft::UI::Input::HoldingState::Canceled:
                    VERIFY_FAIL(L"Unexpected Holding event raised with HoldingState=Canceled");
                    break;
                default:
                    VERIFY_FAIL(L"Unexpected Holding event raised with unexpected HoldingState");
                    break;
                }
            }));
            TestServices::WindowHelper->WindowContent = mainGrid;
        });

        TestServices::WindowHelper->WaitForIdle();
        rectLoadedEvent->WaitForDefault();

        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        TraceConsumer::EnableTracingByEventId(HoldingBegin_value);
        TraceConsumer::EnableTracingByEventId(HoldingEnd_value);


        LOG_OUTPUT(L"Holding the Rectangle.");
        TestServices::InputHelper->Hold(rect);
        gestureCompletedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        session.Stop();
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(HoldingBegin_value, 2 * (c_elementsAddedToTree + c_minElementsInTree))); //2x for start/completed
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(HoldingEnd_value, 2 * (c_elementsAddedToTree + c_minElementsInTree)));

        RunOnUIThread([&]()
        {
            rect->Loaded -= rectLoadedToken;
        });
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }

    void TouchInputTests::SetupElements(
        _Out_ Microsoft::UI::Xaml::Shapes::Rectangle^* pRect,
        _Out_ Microsoft::UI::Xaml::Controls::Grid^* pGrid
        )
    {

        auto grid = ref new Microsoft::UI::Xaml::Controls::Grid();

        auto canvas = ref new Canvas();
        canvas->Width = 300;
        canvas->Height = 400;
        canvas->HorizontalAlignment = HorizontalAlignment::Center;
        canvas->VerticalAlignment = VerticalAlignment::Center;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        grid->Children->Append(canvas);

        auto rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();

        rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        rect->Width = 50;
        rect->Height = 50;
        canvas->Children->Append(rect);

        TestServices::WindowHelper->WindowContent = canvas;

        *pRect = rect;
        *pGrid = grid;
    }
} } } } } } }