// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicGestureManipulationTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <DisableErrorReportingScopeGuard.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Gestures {

        bool BasicGestureManipulationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool BasicGestureManipulationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool BasicGestureManipulationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        // Verifies valid and invalid Gestures and System ManipulationModes combinations.
        void BasicGestureManipulationTests::DManipAndGestureModeCombinations()
        {
            DisableErrorReportingScopeGuard disableErrors;

            RunOnUIThread([&]()
            {
                Microsoft::UI::Xaml::Shapes::Rectangle^ rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();

                // Check valid combinations
                rect->ManipulationMode = ManipulationModes::System;
                rect->ManipulationMode = ManipulationModes::All;
                rect->ManipulationMode = ManipulationModes::None;
                rect->ManipulationMode = ManipulationModes::TranslateX | ManipulationModes::TranslateInertia;
                rect->ManipulationMode = ManipulationModes::TranslateX | ManipulationModes::System;
                rect->ManipulationMode = ManipulationModes::TranslateX | ManipulationModes::TranslateY | ManipulationModes::System;
                rect->ManipulationMode = ManipulationModes::TranslateY | ManipulationModes::TranslateInertia | ManipulationModes::System;
                rect->ManipulationMode = ManipulationModes::TranslateX | ManipulationModes::TranslateY | ManipulationModes::TranslateInertia | ManipulationModes::System;
                rect->ManipulationMode = ManipulationModes::TranslateRailsY | ManipulationModes::System;
                rect->ManipulationMode = ManipulationModes::TranslateY | ManipulationModes::TranslateRailsY | ManipulationModes::TranslateInertia | ManipulationModes::System;
                rect->ManipulationMode = ManipulationModes::Scale | ManipulationModes::System;
                rect->ManipulationMode = ManipulationModes::Scale | ManipulationModes::ScaleInertia | ManipulationModes::System;

                // Check invalid combinations
                VERIFY_THROWS_WINRT(
                    rect->ManipulationMode = ManipulationModes::System | ManipulationModes::TranslateInertia,
                    Platform::Exception^);
                VERIFY_THROWS_WINRT(
                    rect->ManipulationMode = ManipulationModes::System | ManipulationModes::All,
                    Platform::Exception^);
                VERIFY_THROWS_WINRT(
                    rect->ManipulationMode = ManipulationModes::System | ManipulationModes::Rotate,
                    Platform::Exception^);
                VERIFY_THROWS_WINRT(
                    rect->ManipulationMode = ManipulationModes::System | ManipulationModes::ScaleInertia,
                    Platform::Exception^);
                VERIFY_THROWS_WINRT(
                    rect->ManipulationMode = ManipulationModes::System | ManipulationModes::TranslateX | ManipulationModes::Rotate,
                    Platform::Exception^);
            });
        }
        // Verify that the gesture maipulation events occur and in the proper sequence
        void BasicGestureManipulationTests::ValidateEventSequence()
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(300, 300));

            auto sequenceCompleteEvent = std::make_shared<Event>();

            auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
            auto pointerReleasedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerReleased);
            auto manipulationStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationStarting);
            auto manipulationInertiaStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationInertiaStarting);
            auto manipulationStartedRegistration = CreateSafeEventRegistration(UIElement, ManipulationStarted);
            auto manipulationDeltaRegistration = CreateSafeEventRegistration(UIElement, ManipulationDelta);
            auto manipulationCompletedRegistration = CreateSafeEventRegistration(UIElement, ManipulationCompleted);

            // Event bit flags are specified in the order in which we expect to see them so that if the flag we are
            // attempting to set is less than or equal to the existing flags value, we know we are out of sequence.
            const uint8_t PointerPressedFlag = 0x1;
            const uint8_t ManipulationStartingFlag = 0x2;
            const uint8_t PointerReleasedFlag = 0x4;
            const uint8_t ManipulationStartedFlag = 0x8;
            const uint8_t ManipulationDeltaFlag = 0x10;
            const uint8_t ManipulationInertiaStartingFlag = 0x20;
            const uint8_t ManipulationCompletedFlag = 0x40;
            const uint8_t AllFlags = 0x7F;

            // Depending on our action this is the set of flags that we expect to be set.
            const uint8_t TapEventFlags = PointerPressedFlag | ManipulationStartingFlag | PointerReleasedFlag;
            const uint8_t ManipulationWithInertiaFlags = AllFlags & ~PointerReleasedFlag;
            const uint8_t ManipulationWithoutInertiaFlags = ManipulationWithInertiaFlags & ~ManipulationInertiaStartingFlag;

            // Define these early on so we can pass them into the lambdas
            uint8_t eventFlags = 0;

            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
            RunOnUIThread([&]()
                {
                    ScrollViewer^ mainScrollViewer = ref new ScrollViewer();
                    mainScrollViewer->HorizontalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                    mainScrollViewer->VerticalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                    mainScrollViewer->HorizontalScrollMode = xaml_controls::ScrollMode::Enabled;
                    mainScrollViewer->VerticalScrollMode = xaml_controls::ScrollMode::Enabled;
                    mainScrollViewer->ZoomMode = xaml_controls::ZoomMode::Enabled;

                    canvas = ref new Canvas();
                    canvas->Width = 300;
                    canvas->Height = 400;
                    canvas->HorizontalAlignment = HorizontalAlignment::Center;
                    canvas->VerticalAlignment = VerticalAlignment::Center;
                    canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

                    mainScrollViewer->Content = canvas;

                    rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                    rect->Width = 100;
                    rect->Height = 200;
                    rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                    rect->ManipulationMode = ManipulationModes::All;
                    canvas->Children->Append(rect);
                    Canvas::SetLeft(rect, (canvas->Width - rect->Width) / 2.0);
                    Canvas::SetTop(rect, (canvas->Height - rect->Height) / 2.0);

                    TestServices::WindowHelper->WindowContent = mainScrollViewer;

                    pointerPressedRegistration.Attach(rect,
                        ref new xaml_input::PointerEventHandler(
                            [=, &eventFlags](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
                            {
                                LOG_OUTPUT(L"PointerPressed Event");
                                VERIFY_IS_TRUE(eventFlags < PointerPressedFlag);
                                eventFlags |= PointerPressedFlag;
                            }));
                    pointerReleasedRegistration.Attach(rect,
                        ref new xaml_input::PointerEventHandler(
                            [=, eventFlags](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
                            {
                                LOG_OUTPUT(L"PointerReleased Event");
                                VERIFY_IS_TRUE(eventFlags < PointerReleasedFlag);
                                eventFlags |= PointerReleasedFlag;
                                // This should be the final event for non manipulation sequence
                                sequenceCompleteEvent->Set();
                            }));

                    manipulationStartingRegistration.Attach(rect,
                        ref new xaml_input::ManipulationStartingEventHandler(
                            [=, eventFlags](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
                            {
                                LOG_OUTPUT(L"ManipulationStarting Event");
                                VERIFY_IS_TRUE(eventFlags < ManipulationStartingFlag);
                                eventFlags |= ManipulationStartingFlag;
                            }));
                    manipulationStartedRegistration.Attach(rect,
                        ref new xaml_input::ManipulationStartedEventHandler(
                            [=, eventFlags](Platform::Object^, ManipulationStartedRoutedEventArgs^ args)
                            {
                                LOG_OUTPUT(L"ManipulationStarted Event");
                                VERIFY_IS_TRUE(eventFlags < ManipulationStartedFlag);
                                eventFlags |= ManipulationStartedFlag;
                            }));

                    manipulationDeltaRegistration.Attach(rect,
                        ref new xaml_input::ManipulationDeltaEventHandler(
                            [=, eventFlags](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                            {
                                // Manipulation Delta can occur multiple times and following a inertia starting, so 
                                // all we care about is that we haven't gotten an event that should come after inertia
                                // starting.
                                LOG_OUTPUT(L"ManipulationDelta Event");
                                VERIFY_IS_TRUE(eventFlags < (ManipulationInertiaStartingFlag << 1));
                                eventFlags |= ManipulationDeltaFlag;
                             }));

                    manipulationCompletedRegistration.Attach(rect,
                        ref new xaml_input::ManipulationCompletedEventHandler(
                            [=, eventFlags](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                            {
                                LOG_OUTPUT(L"ManipulationCompleted Event");
                                VERIFY_IS_TRUE(eventFlags < ManipulationCompletedFlag);
                                eventFlags |= ManipulationCompletedFlag;
                                // This should be the last event for a manipulation sequence
                                sequenceCompleteEvent->Set();
                            }));

                    manipulationInertiaStartingRegistration.Attach(rect,
                        ref new xaml_input::ManipulationInertiaStartingEventHandler(
                            [=, eventFlags](Platform::Object^, ManipulationInertiaStartingRoutedEventArgs^ args)
                            {
                                LOG_OUTPUT(L"ManipulationInertiaStarting Event");
                                VERIFY_IS_TRUE(eventFlags < ManipulationInertiaStartingFlag);
                                eventFlags |= ManipulationInertiaStartingFlag;
                            }));
                });
            TestServices::WindowHelper->WaitForIdle();

            // The following sequence of actions, in addition to testing just tap and pan scenarios also allows
            // use to test consecutive taps, taps followed by pan, pan follwed by tap and consecutive pans.

            // Tap the rectangle;
            eventFlags = 0;
            sequenceCompleteEvent->Reset();
            LOG_OUTPUT(L"Tapping the Rectangle.");
            TestServices::InputHelper->Tap(rect);
            sequenceCompleteEvent->WaitForDefault();
            VERIFY_ARE_EQUAL(eventFlags, TapEventFlags);
 
            // Tap the rectangle;
            eventFlags = 0;
            sequenceCompleteEvent->Reset();
            LOG_OUTPUT(L"Tapping the Rectangle.");
            TestServices::InputHelper->Tap(rect);
            sequenceCompleteEvent->WaitForDefault();
            VERIFY_ARE_EQUAL(eventFlags, TapEventFlags);

            // Drag the rectangle to the right with the mouse with interia.
            eventFlags = 0;
            sequenceCompleteEvent->Reset();
            LOG_OUTPUT(L"Launching horizontal drag operation (no inertia).");
            TestServices::InputHelper->DragFromCenter(rect, 100 /*relX*/, 0 /*relY*/, 1 /*velocityFactor*/);
            sequenceCompleteEvent->WaitForDefault();
            VERIFY_ARE_EQUAL(eventFlags, ManipulationWithInertiaFlags);

            // Turn off interia
            RunOnUIThread([&]()
                {
                    rect->ManipulationMode = ManipulationModes::All & ~ManipulationModes::TranslateInertia;
                });

            // Pan the rectangle to the left with out inertia.
            eventFlags = 0;
            sequenceCompleteEvent->Reset();
            LOG_OUTPUT(L"Launching horizontal drag operation (with inertia).");
            TestServices::InputHelper->PanFromCenter(rect, 100 /*relX*/, 0 /*relY*/, 1 /*velocityFactor*/);
            sequenceCompleteEvent->WaitForDefault();
            VERIFY_ARE_EQUAL(eventFlags, ManipulationWithoutInertiaFlags);

            // Tap the rectangle;
            eventFlags = 0;
            sequenceCompleteEvent->Reset();
            LOG_OUTPUT(L"Tapping the Rectangle.");
            TestServices::InputHelper->Tap(rect);
            sequenceCompleteEvent->WaitForDefault();
            VERIFY_ARE_EQUAL(eventFlags, TapEventFlags);

            // Turn off Translate X (and inertia back on)
            RunOnUIThread([&]()
                {
                    rect->ManipulationMode = ManipulationModes::All & ~(ManipulationModes::TranslateX | ManipulationModes::TranslateRailsX);
                });

            // Pan the rectangle to the left.  Note that we since we are panning we do get a starting, but because it is in the
            // wrong direction we don't actually get any deltas or interia starting events.
            eventFlags = 0;
            sequenceCompleteEvent->Reset();
            LOG_OUTPUT(L"Launching horizontal drag operation with translate X turned off.");
            TestServices::InputHelper->PanFromCenter(rect, 100 /*relX*/, 0 /*relY*/, 1 /*velocityFactor*/);
            sequenceCompleteEvent->WaitForDefault();
            VERIFY_ARE_EQUAL(eventFlags, ManipulationWithoutInertiaFlags & ~ManipulationDeltaFlag);
        }

        //------------------------------------------------------------------------
        // Test case: Drag a Rectangle horizontally with mouse using Gesture.
        //------------------------------------------------------------------------
        void BasicGestureManipulationTests::DragARectangle()
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>();
            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
            wf::EventRegistrationToken rectManipulationStartingToken = {};
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
                rect->ManipulationMode = ManipulationModes::TranslateX;
                canvas->Children->Append(rect);

                rectManipulationStartingToken = rect->ManipulationStarting +=
                    ref new xaml_input::ManipulationStartingEventHandler(
                    [](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarting, Mode: %d", args->Mode);
                    VERIFY_ARE_EQUAL(args->Mode, ManipulationModes::TranslateX);
                });

                rectManipulationStartedToken = rect->ManipulationStarted +=
                    ref new xaml_input::ManipulationStartedEventHandler(
                    [](Platform::Object^ sender, ManipulationStartedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarted, Cumulative.Translation.X: %f, Position: (%f, %f)",
                        args->Cumulative.Translation.X, args->Position.X, args->Position.Y);
                    VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X > 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Expansion, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Rotation, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Scale, 1);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, args->Cumulative.Translation.X);
                });

                rectManipulationDeltaToken = rect->ManipulationDelta +=
                    ref new xaml_input::ManipulationDeltaEventHandler(
                    [](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                {
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X > 0);
                    VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, args->Cumulative.Translation.X);
                });

                rectManipulationCompletedToken = rect->ManipulationCompleted +=
                    ref new xaml_input::ManipulationCompletedEventHandler(
                    [manipulationCompletedEvent](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted, Cumulative.Translation.X: %f, Position: (%f, %f), IsInertial: %d",
                        args->Cumulative.Translation.X, args->Position.X, args->Position.Y, args->IsInertial);
                    VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X >= 90);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X <= 100);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Expansion, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Rotation, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Scale, 1);
                    manipulationCompletedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            // Drag the rectangle to the right with the mouse and without inertia.
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching horizontal drag operation.");
            TestServices::InputHelper->DragFromCenter(rect, 100 /*relX*/, 0 /*relY*/, 1 /*velocityFactor*/);
            manipulationCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                rect->ManipulationStarting -= rectManipulationStartingToken;
                rect->ManipulationStarted -= rectManipulationStartedToken;
                rect->ManipulationDelta -= rectManipulationDeltaToken;
                rect->ManipulationCompleted -= rectManipulationCompletedToken;
            });
        }

        //------------------------------------------------------------------------
        // Test case: Drag a Rectangle horizontally with mouse using a Gesture in a
        // vertically scrollable ScrollViewer.
        //------------------------------------------------------------------------
        void BasicGestureManipulationTests::DragARectangleInScrollViewer()
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>();
            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
            wf::EventRegistrationToken rectManipulationStartingToken = {};
            wf::EventRegistrationToken rectManipulationStartedToken = {};
            wf::EventRegistrationToken rectManipulationDeltaToken = {};
            wf::EventRegistrationToken rectManipulationCompletedToken = {};

            ::Windows::Foundation::Size size(300, 300);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            RunOnUIThread([&]()
            {
                ScrollViewer^ mainScrollViewer = ref new ScrollViewer();
                mainScrollViewer->HorizontalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                mainScrollViewer->VerticalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                mainScrollViewer->HorizontalScrollMode = xaml_controls::ScrollMode::Disabled;
                mainScrollViewer->VerticalScrollMode = xaml_controls::ScrollMode::Enabled;

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

                mainScrollViewer->Content = canvas;

                rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                rect->Width = 50;
                rect->Height = 60;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->ManipulationMode = ManipulationModes::System | ManipulationModes::TranslateX;
                rect->IsTapEnabled = false;
                rect->IsDoubleTapEnabled = false;
                rect->IsRightTapEnabled = false;
                rect->IsHoldingEnabled = false;

                canvas->Children->Append(rect);

                rectManipulationStartingToken = rect->ManipulationStarting +=
                    ref new xaml_input::ManipulationStartingEventHandler(
                    [](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarting, Mode: %d", args->Mode);
                    VERIFY_ARE_EQUAL(args->Mode, ManipulationModes::System | ManipulationModes::TranslateX);
                });

                rectManipulationStartedToken = rect->ManipulationStarted +=
                    ref new xaml_input::ManipulationStartedEventHandler(
                    [](Platform::Object^ sender, ManipulationStartedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarted, Cumulative.Translation.X: %f, Position: (%f, %f)",
                        args->Cumulative.Translation.X, args->Position.X, args->Position.Y);
                    VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X > 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Expansion, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Rotation, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Scale, 1);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, args->Cumulative.Translation.X);
                });

                rectManipulationDeltaToken = rect->ManipulationDelta +=
                    ref new xaml_input::ManipulationDeltaEventHandler(
                    [](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                {
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X > 0);
                    VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, args->Cumulative.Translation.X);
                });

                rectManipulationCompletedToken = rect->ManipulationCompleted +=
                    ref new xaml_input::ManipulationCompletedEventHandler(
                    [manipulationCompletedEvent](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted, Cumulative.Translation.X: %f, Position: (%f, %f), IsInertial: %d",
                        args->Cumulative.Translation.X, args->Position.X, args->Position.Y, args->IsInertial);
                    VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X >= 90);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X <= 100);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Expansion, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Rotation, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Scale, 1);
                    manipulationCompletedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = mainScrollViewer;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // Scale a Rectangle with touch using a Gesture in a scrollable and zoomable ScrollViewer.
        void BasicGestureManipulationTests::ScaleARectangleInScrollViewer()
        {
            ScaleARectangleInScrollViewer(false /*useInertia*/, false /*setManipulationModeInPointerPressed*/);
        }

        // Scale a Rectangle with touch using a Gesture in a scrollable and zoomable ScrollViewer with inertia.
        void BasicGestureManipulationTests::ScaleARectangleInScrollViewerWithInertia()
        {
            ScaleARectangleInScrollViewer(true /*useInertia*/, false /*setManipulationModeInPointerPressed*/);
        }

        // Scale a Rectangle with touch using a Gesture in a scrollable and zoomable ScrollViewer after setting ManipulationMode
        // in Rectangle's PointerPressed handler.
        void BasicGestureManipulationTests::SetSystemAndScaleInPointerPressed()
        {
            ScaleARectangleInScrollViewer(false /*useInertia*/, true /*setManipulationModeInPointerPressed*/);
        }

        void BasicGestureManipulationTests::ScaleARectangleInScrollViewer(bool useInertia, bool setManipulationModeInPointerPressed)
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));

            auto manipulationCompletedEvent = std::make_shared<Event>();
            auto viewChangedEvent = std::make_shared<Event>();

            auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
            auto manipulationStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationStarting);
            auto manipulationDeltaRegistration = CreateSafeEventRegistration(UIElement, ManipulationDelta);
            auto manipulationCompletedRegistration = CreateSafeEventRegistration(UIElement, ManipulationCompleted);
            auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;

            RunOnUIThread([&]()
            {
                ScrollViewer^ mainScrollViewer = ref new ScrollViewer();
                mainScrollViewer->HorizontalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                mainScrollViewer->VerticalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                mainScrollViewer->HorizontalScrollMode = xaml_controls::ScrollMode::Enabled;
                mainScrollViewer->VerticalScrollMode = xaml_controls::ScrollMode::Enabled;
                mainScrollViewer->ZoomMode = xaml_controls::ZoomMode::Enabled;

                canvas = ref new Canvas();
                canvas->Width = 600;
                canvas->Height = 800;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

                mainScrollViewer->Content = canvas;

                rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                rect->Width = 200;
                rect->Height = 400;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                if (setManipulationModeInPointerPressed)
                {
                    rect->ManipulationMode = ManipulationModes::None;
                }
                else
                {
                    rect->ManipulationMode = ManipulationModes::System | ManipulationModes::Scale;
                    if (useInertia)
                    {
                        rect->ManipulationMode = rect->ManipulationMode | ManipulationModes::ScaleInertia;
                    }
                }
                rect->RenderTransform = ref new Microsoft::UI::Xaml::Media::ScaleTransform();
                canvas->Children->Append(rect);
                Canvas::SetLeft(rect, (canvas->Width - rect->Width) / 2.0);
                Canvas::SetTop(rect, (canvas->Height - rect->Height) / 2.0);

                pointerPressedRegistration.Attach(rect,
                    ref new xaml_input::PointerEventHandler(
                    [rect, setManipulationModeInPointerPressed, useInertia](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
                {
                    if (setManipulationModeInPointerPressed)
                    {
                        LOG_OUTPUT(L"Setting Rectangle ManipulationMode in PointerPressed");
                        ManipulationModes manipulationMode = ManipulationModes::System | ManipulationModes::Scale;
                        if (useInertia)
                        {
                            manipulationMode = manipulationMode | ManipulationModes::ScaleInertia;
                        }
                        rect->ManipulationMode = manipulationMode;
                    }

                }));

                manipulationStartingRegistration.Attach(rect,
                    ref new xaml_input::ManipulationStartingEventHandler(
                    [useInertia](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarting, Mode: %d", args->Mode);
                    ManipulationModes manipulationMode = ManipulationModes::System | ManipulationModes::Scale;
                    if (useInertia)
                    {
                        manipulationMode = manipulationMode | ManipulationModes::ScaleInertia;
                    }
                    VERIFY_ARE_EQUAL(args->Mode, manipulationMode);
                }));

                manipulationDeltaRegistration.Attach(rect,
                    ref new xaml_input::ManipulationDeltaEventHandler(
                    [canvas, useInertia](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationDelta, Cumulative.Translation.X: %f, Cumulative.Translation.Y: %f, Cumulative.Scale: %f",
                        args->Cumulative.Translation.X, args->Cumulative.Translation.Y, args->Cumulative.Scale);

                    LOG_OUTPUT(L"ManipulationDelta, Delta.Expansion: %f, Delta.Rotation: %f, Delta.Scale: %f",
                        args->Delta.Expansion, args->Delta.Rotation, args->Delta.Scale);

                    LOG_OUTPUT(L"ManipulationDelta, Velocities.Angular: %f, Velocities.Expansion: %f, Velocities.Linear.X: %f, Velocities.Linear.Y: %f",
                        args->Velocities.Angular, args->Velocities.Expansion, args->Velocities.Linear.X, args->Velocities.Linear.Y);

                    VERIFY_IS_FALSE(!useInertia && args->IsInertial);
                    VERIFY_IS_TRUE(args->Cumulative.Scale > 1);
                    VERIFY_IS_TRUE(args->Cumulative.Expansion > 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.X, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Rotation, 0);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, (canvas->Width - args->Cumulative.Scale * rect->Width) / 2.0);
                    Canvas::SetTop(rect, (canvas->Height - args->Cumulative.Scale * rect->Height) / 2.0);
                    Microsoft::UI::Xaml::Media::ScaleTransform^ st = dynamic_cast<Microsoft::UI::Xaml::Media::ScaleTransform^>(rect->RenderTransform);
                    st->ScaleX = args->Cumulative.Scale;
                    st->ScaleY = args->Cumulative.Scale;
                }));

                manipulationCompletedRegistration.Attach(rect,
                    ref new xaml_input::ManipulationCompletedEventHandler(
                    [manipulationCompletedEvent, mainScrollViewer, useInertia](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted, Cumulative.Scale: %f, args->Cumulative.Expansion: %f, Position: (%f, %f)",
                        args->Cumulative.Scale, args->Cumulative.Expansion, args->Position.X, args->Position.Y);
                    VERIFY_IS_FALSE(!useInertia && args->IsInertial);
                    VERIFY_IS_TRUE(args->Cumulative.Scale > 1.00);
                    VERIFY_IS_TRUE(args->Cumulative.Expansion > 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.X, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Rotation, 0);
                    manipulationCompletedEvent->Set();

                    LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", mainScrollViewer->HorizontalOffset, mainScrollViewer->VerticalOffset, mainScrollViewer->ZoomFactor);
                    VERIFY_ARE_EQUAL(mainScrollViewer->HorizontalOffset, 0.0);
                    VERIFY_ARE_EQUAL(mainScrollViewer->VerticalOffset, 0.0);
                    VERIFY_ARE_EQUAL(mainScrollViewer->ZoomFactor, 1.0f);
                }));

                viewChangingRegistration.Attach(mainScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                    [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                        args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                        args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                        args->IsInertial);
                }));

                viewChangedRegistration.Attach(mainScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [mainScrollViewer, viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                        mainScrollViewer->HorizontalOffset, mainScrollViewer->VerticalOffset, mainScrollViewer->ZoomFactor, args->IsIntermediate);
                    if (args->IsIntermediate == false)
                    {
                        viewChangedEvent->Set();
                    }
                    VERIFY_IS_TRUE(mainScrollViewer->VerticalOffset > 0);
                    VERIFY_ARE_EQUAL(mainScrollViewer->HorizontalOffset, 0.0);
                    VERIFY_ARE_EQUAL(mainScrollViewer->ZoomFactor, 1.0f);
                }));

                TestServices::WindowHelper->WindowContent = mainScrollViewer;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching zoom-in operation.");
            TestServices::InputHelper->ZoomInToEdges(rect, 80 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Vertical, 1.0 /*velocityFactor*/);
            manipulationCompletedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching vertical pan operation.");
            TestServices::InputHelper->PanFromCenter(rect, 0 /*relX*/, -50 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();
        }

        // Reset Gesture scaling in PointerPressed handler and zoom in ScrollViewer with DManip.
        void BasicGestureManipulationTests::ResetScaleInPointerPressed()
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(300, 300));

            auto viewChangedEvent = std::make_shared<Event>();

            auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
            auto manipulationStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationStarting);
            auto manipulationDeltaRegistration = CreateSafeEventRegistration(UIElement, ManipulationDelta);
            auto manipulationCompletedRegistration = CreateSafeEventRegistration(UIElement, ManipulationCompleted);
            auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

            int pointerPressedEvents = 0;
            int manipulationEvents = 0;
            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;

            RunOnUIThread([&]()
            {
                ScrollViewer^ mainScrollViewer = ref new ScrollViewer();
                mainScrollViewer->HorizontalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                mainScrollViewer->VerticalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                mainScrollViewer->HorizontalScrollMode = xaml_controls::ScrollMode::Enabled;
                mainScrollViewer->VerticalScrollMode = xaml_controls::ScrollMode::Enabled;
                mainScrollViewer->ZoomMode = xaml_controls::ZoomMode::Enabled;

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

                mainScrollViewer->Content = canvas;

                rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                rect->Width = 100;
                rect->Height = 200;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->ManipulationMode = ManipulationModes::System | ManipulationModes::Scale;
                canvas->Children->Append(rect);
                Canvas::SetLeft(rect, (canvas->Width - rect->Width) / 2.0);
                Canvas::SetTop(rect, (canvas->Height - rect->Height) / 2.0);

                pointerPressedRegistration.Attach(rect,
                    ref new xaml_input::PointerEventHandler(
                    [rect, &pointerPressedEvents](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"PointerPressed, Rectangle ManipulationMode: %d", rect->ManipulationMode);
                    pointerPressedEvents++;
                    VERIFY_IS_TRUE(pointerPressedEvents == 1 || pointerPressedEvents == 2);
                    if (pointerPressedEvents == 1)
                    {
                        VERIFY_ARE_EQUAL(rect->ManipulationMode, ManipulationModes::System | ManipulationModes::Scale);
                        LOG_OUTPUT(L"Resetting Rectangle ManipulationMode in PointerPressed");
                        rect->ManipulationMode = ManipulationModes::System;
                    }
                    else if (pointerPressedEvents == 2)
                    {
                        VERIFY_ARE_EQUAL(rect->ManipulationMode, ManipulationModes::System);
                    }
                }));

                manipulationStartingRegistration.Attach(rect,
                    ref new xaml_input::ManipulationStartingEventHandler(
                    [&manipulationEvents](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarting, Mode: %d", args->Mode);
                    manipulationEvents++;
                }));

                manipulationDeltaRegistration.Attach(rect,
                    ref new xaml_input::ManipulationDeltaEventHandler(
                    [&manipulationEvents](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationDelta, Cumulative.Translation.X: %f, Cumulative.Translation.Y: %f, Cumulative.Scale: %f",
                        args->Cumulative.Translation.X, args->Cumulative.Translation.Y, args->Cumulative.Scale);
                    manipulationEvents++;
                }));

                manipulationCompletedRegistration.Attach(rect,
                    ref new xaml_input::ManipulationCompletedEventHandler(
                    [&manipulationEvents](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted, Cumulative.Scale: %f, args->Cumulative.Expansion: %f, Position: (%f, %f)",
                        args->Cumulative.Scale, args->Cumulative.Expansion, args->Position.X, args->Position.Y);
                    manipulationEvents++;
                }));

                viewChangingRegistration.Attach(mainScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                    [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                        args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                        args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                        args->IsInertial);
                }));

                viewChangedRegistration.Attach(mainScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [mainScrollViewer, viewChangedEvent, manipulationEvents](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                        mainScrollViewer->HorizontalOffset, mainScrollViewer->VerticalOffset, mainScrollViewer->ZoomFactor, args->IsIntermediate);
                    if (args->IsIntermediate == false)
                    {
                        viewChangedEvent->Set();
                        LOG_OUTPUT(L"Final ScrollViewer view is (x, y, z) = (%f, %f, %f).", mainScrollViewer->HorizontalOffset, mainScrollViewer->VerticalOffset, mainScrollViewer->ZoomFactor);
                        VERIFY_ARE_EQUAL(manipulationEvents, 0);
                        VERIFY_IS_TRUE(mainScrollViewer->HorizontalOffset > 0.0);
                        VERIFY_IS_TRUE(mainScrollViewer->VerticalOffset > 0.0);
                        VERIFY_IS_TRUE(mainScrollViewer->ZoomFactor > 1.0f);
                    }
                }));

                TestServices::WindowHelper->WindowContent = mainScrollViewer;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching zoom-in operation.");
            TestServices::InputHelper->ZoomInToEdges(rect, 40 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Vertical, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();
        }

        // Pan a ScrollViewer with DManip by touching a Gesture-zoomable Rectangle.
        void BasicGestureManipulationTests::PanAZoomableRectangle()
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(300, 300));

            auto viewChangedEvent = std::make_shared<Event>();

            auto manipulationDeltaRegistration = CreateSafeEventRegistration(UIElement, ManipulationDelta);
            auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

            int manipulationDeltaEvents = 0;
            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;

            RunOnUIThread([&]()
            {
                ScrollViewer^ mainScrollViewer = ref new ScrollViewer();
                mainScrollViewer->HorizontalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                mainScrollViewer->VerticalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Hidden;
                mainScrollViewer->HorizontalScrollMode = xaml_controls::ScrollMode::Enabled;
                mainScrollViewer->VerticalScrollMode = xaml_controls::ScrollMode::Enabled;
                mainScrollViewer->ZoomMode = xaml_controls::ZoomMode::Enabled;

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

                mainScrollViewer->Content = canvas;

                rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                rect->Width = 100;
                rect->Height = 200;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->ManipulationMode = ManipulationModes::System | ManipulationModes::Scale;
                canvas->Children->Append(rect);
                Canvas::SetLeft(rect, (canvas->Width - rect->Width) / 2.0);
                Canvas::SetTop(rect, (canvas->Height - rect->Height) / 2.0);

                manipulationDeltaRegistration.Attach(rect,
                    ref new xaml_input::ManipulationDeltaEventHandler(
                    [&manipulationDeltaEvents](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationDelta, Cumulative.Translation.X: %f, Cumulative.Translation.Y: %f, Cumulative.Scale: %f",
                        args->Cumulative.Translation.X, args->Cumulative.Translation.Y, args->Cumulative.Scale);
                    manipulationDeltaEvents++;
                }));

                viewChangingRegistration.Attach(mainScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                    [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                        args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                        args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                        args->IsInertial);
                }));

                viewChangedRegistration.Attach(mainScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [mainScrollViewer, viewChangedEvent, manipulationDeltaEvents](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                        mainScrollViewer->HorizontalOffset, mainScrollViewer->VerticalOffset, mainScrollViewer->ZoomFactor, args->IsIntermediate);
                    if (args->IsIntermediate == false)
                    {
                        viewChangedEvent->Set();
                        LOG_OUTPUT(L"Final ScrollViewer view is (x, y, z) = (%f, %f, %f).", mainScrollViewer->HorizontalOffset, mainScrollViewer->VerticalOffset, mainScrollViewer->ZoomFactor);
                        VERIFY_ARE_EQUAL(manipulationDeltaEvents, 0);
                        VERIFY_ARE_EQUAL(mainScrollViewer->HorizontalOffset, 0.0);
                        VERIFY_IS_TRUE(mainScrollViewer->VerticalOffset > 0.0);
                        VERIFY_ARE_EQUAL(mainScrollViewer->ZoomFactor, 1.0f);
                    }
                }));

                TestServices::WindowHelper->WindowContent = mainScrollViewer;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching vertical pan operation.");
            TestServices::InputHelper->PanFromCenter(rect, 0 /*relX*/, -50 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();
        }


        void BasicGestureManipulationTests::PanARectangleWithManualCompletion()
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>(EventOptions::ManualReset);
            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;

            auto manipulationDeltaRegistration = CreateSafeEventRegistration(UIElement, ManipulationDelta);
            auto manipulationCompletedRegistration = CreateSafeEventRegistration(UIElement, ManipulationCompleted);

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

                manipulationDeltaRegistration.Attach(rect, ref new xaml_input::ManipulationDeltaEventHandler(
                    [manipulationCompletedEvent](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationDelta, Cumulative.Translation.X: %f, IsInertial: %d",
                        args->Cumulative.Translation.X, args->IsInertial);

                    // We should not continue to get ManipulationDelta events after ManipulationCompleted has fired.
                    VERIFY_IS_FALSE(manipulationCompletedEvent->HasFired());

                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    Canvas::SetLeft(rect, args->Cumulative.Translation.X);

                    if (args->IsInertial)
                    {
                        args->Complete();
                    }
                }));

                manipulationCompletedRegistration.Attach(rect, ref new xaml_input::ManipulationCompletedEventHandler(
                    [manipulationCompletedEvent](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted, Cumulative.Translation.X: %f, Position: (%f, %f), IsInertial: %d",
                        args->Cumulative.Translation.X, args->Position.X, args->Position.Y, args->IsInertial);

                    // We should not continue to get ManipulationDelta events after ManipulationCompleted has fired.
                    VERIFY_IS_FALSE(manipulationCompletedEvent->HasFired());

                    manipulationCompletedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            // Pan the rectangle to the right with inertia.
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching horizontal pan operation.");

            TestServices::InputHelper->DragFromCenter(rect, 100 /*relX*/, 0 /*relY*/, 1.0 /*velocityFactor*/);
            manipulationCompletedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Wait 100ms to make sure we don't get superfluous manipulation events");
            ::Sleep(100);
        }


        //------------------------------------------------------------------------
        // Test case: Zoom a Rectangle without inertia using Gestures.
        //------------------------------------------------------------------------
        void BasicGestureManipulationTests::ZoomARectangle()
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>();
            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
            wf::EventRegistrationToken rectManipulationStartingToken = {};
            wf::EventRegistrationToken rectManipulationStartedToken = {};
            wf::EventRegistrationToken rectManipulationDeltaToken = {};
            wf::EventRegistrationToken rectManipulationCompletedToken = {};

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 600;
                canvas->Height = 800;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

                mainGrid->Children->Append(canvas);

                rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                rect->Width = 200;
                rect->Height = 400;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->ManipulationMode = ManipulationModes::Scale;
                rect->RenderTransform = ref new Microsoft::UI::Xaml::Media::ScaleTransform();
                canvas->Children->Append(rect);
                Canvas::SetLeft(rect, (canvas->Width - rect->Width) / 2.0);
                Canvas::SetTop(rect, (canvas->Height - rect->Height) / 2.0);

                rectManipulationStartingToken = rect->ManipulationStarting +=
                    ref new xaml_input::ManipulationStartingEventHandler(
                    [](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarting, Mode: %d", args->Mode);
                    VERIFY_ARE_EQUAL(args->Mode, ManipulationModes::Scale);
                });

                rectManipulationStartedToken = rect->ManipulationStarted +=
                    ref new xaml_input::ManipulationStartedEventHandler(
                    [canvas](Platform::Object^ sender, ManipulationStartedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarted, Cumulative.Scale: %f, args->Cumulative.Expansion: %f, Position: (%f, %f)",
                        args->Cumulative.Scale, args->Cumulative.Expansion, args->Position.X, args->Position.Y);
                    VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Touch);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.X, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Rotation, 0);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, (canvas->Width - args->Cumulative.Scale * rect->Width) / 2.0);
                    Canvas::SetTop(rect, (canvas->Height - args->Cumulative.Scale * rect->Height) / 2.0);
                    Microsoft::UI::Xaml::Media::ScaleTransform^ st = dynamic_cast<Microsoft::UI::Xaml::Media::ScaleTransform^>(rect->RenderTransform);
                    st->ScaleX = args->Cumulative.Scale;
                    st->ScaleY = args->Cumulative.Scale;
                });

                rectManipulationDeltaToken = rect->ManipulationDelta +=
                    ref new xaml_input::ManipulationDeltaEventHandler(
                    [canvas](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationDelta, Cumulative.Scale: %f, args->Cumulative.Expansion: %f",
                        args->Cumulative.Scale, args->Cumulative.Expansion);
                    VERIFY_IS_FALSE(args->IsInertial);
                    VERIFY_IS_TRUE(args->Cumulative.Scale > 1);
                    VERIFY_IS_TRUE(args->Cumulative.Expansion > 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.X, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Rotation, 0);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, (canvas->Width - args->Cumulative.Scale * rect->Width) / 2.0);
                    Canvas::SetTop(rect, (canvas->Height - args->Cumulative.Scale * rect->Height) / 2.0);
                    Microsoft::UI::Xaml::Media::ScaleTransform^ st = dynamic_cast<Microsoft::UI::Xaml::Media::ScaleTransform^>(rect->RenderTransform);
                    st->ScaleX = args->Cumulative.Scale;
                    st->ScaleY = args->Cumulative.Scale;
                });

                rectManipulationCompletedToken = rect->ManipulationCompleted +=
                    ref new xaml_input::ManipulationCompletedEventHandler(
                    [manipulationCompletedEvent](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted, Cumulative.Scale: %f, args->Cumulative.Expansion: %f, Position: (%f, %f)",
                        args->Cumulative.Scale, args->Cumulative.Expansion, args->Position.X, args->Position.Y);
                    VERIFY_IS_FALSE(args->IsInertial);
                    // Allowing a range of zoom factors to account for random timing fluctuations.
                    VERIFY_IS_TRUE(args->Cumulative.Scale > 1.25);
                    VERIFY_IS_TRUE(args->Cumulative.Scale < 1.65);
                    VERIFY_IS_TRUE(args->Cumulative.Expansion > 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.X, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Rotation, 0);
                    manipulationCompletedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(1);

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching zoom-in operation.");
            TestServices::InputHelper->ZoomInToEdges(rect, 66 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Vertical, 1.0 /*velocityFactor*/);
            manipulationCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final Canvas.Left: %f, Canvas.Top: %f, after zoom in.",
                    Canvas::GetLeft(rect), Canvas::GetTop(rect));

                rect->ManipulationStarting -= rectManipulationStartingToken;
                rect->ManipulationStarted -= rectManipulationStartedToken;
                rect->ManipulationDelta -= rectManipulationDeltaToken;
                rect->ManipulationCompleted -= rectManipulationCompletedToken;
            });
        }

        //------------------------------------------------------------------------
        // Test case: Pan a Rectangle with inertia to test the properties of
        //            ManipulationInertiaStartingEventArgs.
        //------------------------------------------------------------------------
        void BasicGestureManipulationTests::InertiaStartingArgs()
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>();
            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
            auto manipulationStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationStarting);
            auto manipulationInertiaStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationInertiaStarting);
            auto manipulationCompletedRegistration = CreateSafeEventRegistration(UIElement, ManipulationCompleted);

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

                manipulationStartingRegistration.Attach(rect,
                    ref new xaml_input::ManipulationStartingEventHandler(
                    [](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarting, Mode: %d", args->Mode);
                    VERIFY_ARE_EQUAL(args->Mode, ManipulationModes::TranslateX | ManipulationModes::TranslateInertia);
                }));

                manipulationInertiaStartingRegistration.Attach(rect,
                    ref new xaml_input::ManipulationInertiaStartingEventHandler(
                    [canvas](Platform::Object^ sender, ManipulationInertiaStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationInertiaStarting, Cumulative.Translation.X: %f",
                        args->Cumulative.Translation.X);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X > 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, args->Cumulative.Translation.X);

                    // Test basic properties.
                    VERIFY_ARE_EQUAL(args->Container, rect);
                    VERIFY_ARE_EQUAL(args->Delta.Expansion, 0);
                    VERIFY_ARE_EQUAL(args->Delta.Rotation, 0);
                    VERIFY_ARE_EQUAL(args->Delta.Scale, 1.0f);
                    VERIFY_ARE_EQUAL(args->Delta.Translation.X, 0);
                    VERIFY_ARE_EQUAL(args->Delta.Translation.Y, 0);
                    VERIFY_ARE_EQUAL(args->Velocities.Angular, 0);
                    VERIFY_ARE_EQUAL(args->Velocities.Expansion, 0);
                    VERIFY_IS_TRUE(args->Velocities.Linear.X < 1);
                    VERIFY_ARE_EQUAL(args->Velocities.Linear.Y, 0);

                    // Test getting, updating, and setting back (even though this is effectively no-op) the ExpansionBehavior
                    LOG_OUTPUT(L"Updating ExpansionBehavior");
                    InertiaExpansionBehavior^ expansion = args->ExpansionBehavior;
                    expansion->DesiredDeceleration = 123;
                    expansion->DesiredExpansion = 345;
                    args->ExpansionBehavior = expansion;

                    // Test getting, updating, and setting back (even though this is effectively no-op) the RotationBehavior
                    LOG_OUTPUT(L"Updating RotationBehavior");
                    InertiaRotationBehavior^ rotation = args->RotationBehavior;
                    rotation->DesiredDeceleration = 234;
                    rotation->DesiredRotation = 456;
                    args->RotationBehavior = rotation;

                    // Test getting, updating, and setting back (even though this is effectively no-op) the TranslationBehavior
                    LOG_OUTPUT(L"Updating TranslationBehavior");
                    InertiaTranslationBehavior^ translation = args->TranslationBehavior;
                    translation->DesiredDeceleration = 345;
                    translation->DesiredDisplacement = 567;
                    args->TranslationBehavior = translation;

                    // Verify the updated values are all there.
                    VERIFY_ARE_EQUAL(args->ExpansionBehavior->DesiredDeceleration, 123);
                    VERIFY_ARE_EQUAL(args->ExpansionBehavior->DesiredExpansion, 345);
                    VERIFY_ARE_EQUAL(args->RotationBehavior->DesiredDeceleration, 234);
                    VERIFY_ARE_EQUAL(args->RotationBehavior->DesiredRotation, 456);
                    VERIFY_ARE_EQUAL(args->TranslationBehavior->DesiredDeceleration, 345);
                    VERIFY_ARE_EQUAL(args->TranslationBehavior->DesiredDisplacement, 567);
                }));

                manipulationCompletedRegistration.Attach(rect,
                    ref new xaml_input::ManipulationCompletedEventHandler(
                    [manipulationCompletedEvent](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted");
                    manipulationCompletedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            // Pan the rectangle to the right with inertia.
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching horizontal pan operation.");
            TestServices::InputHelper->PanFromCenter(rect, 100 /*relX*/, 0 /*relY*/, 0.65 /*velocityFactor*/);
            manipulationCompletedEvent->WaitForDefault();
        }

        //------------------------------------------------------------------------
        // Test case: Validates the ability to successfully leave the tree in ManipulationDelta during inertia.
        //------------------------------------------------------------------------
        void BasicGestureManipulationTests::LeaveTreeInInertia()
        {
            // Leak: TouchInteractionContext leaked when element leaves tree during touch interaction
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>();
            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
            auto manipulationStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationStarting);
            auto manipulationDeltaRegistration = CreateSafeEventRegistration(UIElement, ManipulationDelta);
            auto manipulationInertiaStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationInertiaStarting);

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

                manipulationStartingRegistration.Attach(rect,
                    ref new xaml_input::ManipulationStartingEventHandler(
                    [](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarting, Mode: %d.", args->Mode);
                    VERIFY_ARE_EQUAL(args->Mode, ManipulationModes::TranslateX | ManipulationModes::TranslateInertia);
                }));

                manipulationDeltaRegistration.Attach(rect,
                    ref new xaml_input::ManipulationDeltaEventHandler(
                    [canvas, manipulationCompletedEvent](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationDeltaRoutedEventArgs, Cumulative.Translation.X: %f, IsInertial=%d.",
                        args->Cumulative.Translation.X, args->IsInertial);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X > 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, args->Cumulative.Translation.X);

                    if (args->IsInertial && args->Cumulative.Translation.X > 120)
                    {
                        LOG_OUTPUT(L"Panned Rectangle leaving the tree during inertia phase.");
                        canvas->Children->Clear();
                        manipulationCompletedEvent->Set();
                    }
                }));

                manipulationInertiaStartingRegistration.Attach(rect,
                    ref new xaml_input::ManipulationInertiaStartingEventHandler(
                    [canvas](Platform::Object^ sender, ManipulationInertiaStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationInertiaStarting, Cumulative.Translation.X: %f.",
                        args->Cumulative.Translation.X);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X > 0);
                    VERIFY_ARE_EQUAL(args->Cumulative.Translation.Y, 0);
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);
                    Canvas::SetLeft(rect, args->Cumulative.Translation.X);
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            // Pan the rectangle to the right with inertia.
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching horizontal pan operation.");
            TestServices::InputHelper->PanFromCenter(rect, 100 /*relX*/, 0 /*relY*/, 0.85 /*velocityFactor*/);
            manipulationCompletedEvent->WaitForDefault();
        }


        void BasicGestureManipulationTests::RotateARectangle()
        {   
            RectRotateHelperData data;
            data.setDesiredDecelerationTo = 1000.0f; // decelerate quickly so it stays close to the target
            data.expectedEndAngle = 90.0f;
            RotateARectangleHelper(data);
        }

        void BasicGestureManipulationTests::RotateARectangleWithDesiredRotation()
        {   
            RectRotateHelperData data;
            data.setDesiredRotationTo = 45.0f;
            data.expectedEndAngle = 135.0f;
            RotateARectangleHelper(data);
        }

        void BasicGestureManipulationTests::RotateARectangleWithDesiredDeceleration()
        {   
            RectRotateHelperData data;
            data.setDesiredDecelerationTo = 0.002f;
            // I did a 50x run here and found the values fell between 139.9 and 146.8, so I took the
            // average of these and gave it an error margin double the variance that we saw in the sample
            // to try and keep the test stable.
            data.expectedEndAngle = 143.4f; 
            data.errorMargin = 6.8f;
            RotateARectangleHelper(data);
        }

        void BasicGestureManipulationTests::RotateARectangleHelper(const RectRotateHelperData& data)
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>();
            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
            wf::EventRegistrationToken rectManipulationStartingToken = {};
            wf::EventRegistrationToken rectManipulationStartedToken = {};
            wf::EventRegistrationToken rectManipulationDeltaToken = {};
            wf::EventRegistrationToken rectManipulationCompletedToken = {};

            auto manipulationStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationStarting);
            auto manipulationStartedRegistration = CreateSafeEventRegistration(UIElement, ManipulationStarted);
            auto manipulationDeltaRegistration = CreateSafeEventRegistration(UIElement, ManipulationDelta);
            auto manipulationCompletedRegistration = CreateSafeEventRegistration(UIElement, ManipulationCompleted);
            auto manipulationInertiaStartingRegistration = CreateSafeEventRegistration(UIElement, ManipulationInertiaStarting);

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
                rect->Width = 250;
                rect->Height = 250;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                rect->ManipulationMode = ManipulationModes::All;
                canvas->Children->Append(rect);

                manipulationStartingRegistration.Attach(rect, 
                    ref new xaml_input::ManipulationStartingEventHandler(
                    [](Platform::Object^, ManipulationStartingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarting, Mode: %d", args->Mode);
                    VERIFY_ARE_EQUAL(args->Mode, ManipulationModes::All);
                }));

                manipulationStartedRegistration.Attach(rect, 
                    ref new xaml_input::ManipulationStartedEventHandler(
                    [](Platform::Object^ sender, ManipulationStartedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationStarted, Cumulative.Translation.X: %f, Position: (%f, %f)",
                        args->Cumulative.Translation.X, args->Position.X, args->Position.Y);
                }));

                manipulationDeltaRegistration.Attach(rect, 
                    ref new xaml_input::ManipulationDeltaEventHandler(
                    [data](Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationDelta, Cumulative.Rotation: %f, Position: (%f, %f), IsInertial: %d",
                        args->Cumulative.Rotation, args->Position.X, args->Position.Y, args->IsInertial);

                    VERIFY_IS_LESS_THAN(args->Cumulative.Rotation, data.expectedEndAngle + data.errorMargin);
                    VERIFY_IS_GREATER_THAN(args->Cumulative.Rotation, data.expectedStartAngle - data.errorMargin);
                }));

                manipulationCompletedRegistration.Attach(rect, 
                    ref new xaml_input::ManipulationCompletedEventHandler(
                    [manipulationCompletedEvent,data](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted, Cumulative.Rotation: %f, Position: (%f, %f), IsInertial: %d",
                        args->Cumulative.Rotation, args->Position.X, args->Position.Y, args->IsInertial);

                    LOG_OUTPUT(L"Expect cumulative rotation to be around %d degrees", static_cast<int>(data.expectedEndAngle));
                    VERIFY_IS_LESS_THAN(args->Cumulative.Rotation, data.expectedEndAngle + data.errorMargin);
                    VERIFY_IS_GREATER_THAN(args->Cumulative.Rotation, data.expectedEndAngle - data.errorMargin);
                    manipulationCompletedEvent->Set();
                }));

                manipulationInertiaStartingRegistration.Attach(rect, 
                    ref new xaml_input::ManipulationInertiaStartingEventHandler(
                    [data](Platform::Object^, ManipulationInertiaStartingRoutedEventArgs^ args)
                {
                    if (data.setDesiredRotationTo != 0.0f)
                    {
                        args->RotationBehavior->DesiredRotation = data.setDesiredRotationTo;
                    }
                    if (data.setDesiredDecelerationTo != 0.0f)
                    {
                        args->RotationBehavior->DesiredDeceleration = data.setDesiredDecelerationTo;
                    }
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Perform a rotation manipulation on the rectangle");

            // We use a relatively slow duration here to avoid inertia kicking in
            const float rotationAngleDegCounterClockwise = -90.0f;
            const int durationMs = 500;
            const bool pivotRotate = true; // indicates the first finger will act as a pivot for the rotation
            TestServices::InputHelper->InjectRotate(rect, {125.0f, 125.0f}, {175.0f, 125.0f}, rotationAngleDegCounterClockwise, durationMs, pivotRotate);
            manipulationCompletedEvent->WaitForDefault();
        }
    } } }
} } } }
