// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PointerRegressionTests.h"
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

        bool PointerRegressionTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool PointerRegressionTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool PointerRegressionTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        // Regression test
        // Reproduced by doing a mouse over of a button that has a 3D transform.
        // The app would crash when attempting to set the transform for the current pointer point transform.
        // This test reproduces the same conditions and verifies it does not crash and the click occurs successfully.
        void PointerRegressionTests::ProjectedButtonClick()
        {
            TestCleanupWrapper cleanup;
            Button^ button = nullptr;
            auto buttonClickEvent = std::make_shared<Event>();
            auto buttonClickRegistration = CreateSafeEventRegistration(Button, Click);

            RunOnUIThread([&]()
            {
                auto mainGrid = ref new Grid();

                mainGrid->HorizontalAlignment = HorizontalAlignment::Left;
                mainGrid->VerticalAlignment = VerticalAlignment::Top;

                auto planeProjection = ref new PlaneProjection();
                planeProjection->RotationX = 30.0;
                planeProjection->RotationY = 30.0;

                auto button = ref new Button();
                button->Width = 100.0;
                button->Height = 100.0;
                button->Content = "Projected Button";
                button->Projection = planeProjection;
                mainGrid->Children->Append(button);

                buttonClickRegistration.Attach(button,
                    ref new RoutedEventHandler([buttonClickEvent](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button Click event");
                    buttonClickEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Note that Tap currently has trouble getting the size of a projected element.
            // Click on the center of the button (50,50), since the button is 100x100, located in the upper left,
            // and the projection is oriented around the center.
            TestServices::InputHelper->Tap(wf::Point(50.0, 50.0));

            buttonClickEvent->WaitForDefault();
        }

        // Note: Even before the fix, this test will frequently pass if pageheap is not enabled.
        //       Running in a loop would hit the failure around 50% of the time.
        void PointerRegressionTests::DelayedEventArgsPointerPointUse()
        {
            TestCleanupWrapper cleanup;
            xaml_shapes::Rectangle^ rectangle = nullptr;
            auto pointerPressedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, PointerPressed);

            PointerRoutedEventArgs^ savedEventArgs = nullptr;
            auto pointerPressedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                auto mainGrid = ref new Grid();

                mainGrid->HorizontalAlignment = HorizontalAlignment::Left;
                mainGrid->VerticalAlignment = VerticalAlignment::Top;

                rectangle = ref new xaml_shapes::Rectangle();
                rectangle->Width = 100.0;
                rectangle->Height = 100.0;
                rectangle->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                mainGrid->Children->Append(rectangle);

                pointerPressedRegistration.Attach(rectangle, ref new xaml_input::PointerEventHandler([&savedEventArgs, pointerPressedEvent, rectangle](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"PointerPressed raised.");
                    if (savedEventArgs != nullptr)
                    {
                        LOG_OUTPUT(L"  savedEventArgs exists: calling GetCurrentPoint");
                        auto pointerPoint = savedEventArgs->GetCurrentPoint(rectangle);
                        LOG_OUTPUT(L"  saved Rectangle PointerPressed at Position=(%.2f,%.2f).", pointerPoint->Position.X, pointerPoint->Position.Y);
                    }

                    savedEventArgs = args;
                    pointerPressedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Tap the rectangle, which will send it a PointerPressed and likely a few other Pointer* messages.
            TestServices::InputHelper->Tap(rectangle);

            LOG_OUTPUT(L"Waiting for pointerPressedEvent");
            pointerPressedEvent->WaitForDefault();
            LOG_OUTPUT(L"Waiting for idle");
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_NOT_NULL(savedEventArgs);
            auto oldSavedEventArgs = savedEventArgs;

            // Move the mouse to generate some more input
            TestServices::InputHelper->MoveMouse(rectangle, -20, -20);
            TestServices::InputHelper->MoveMouse(rectangle,  20, -20);
            TestServices::InputHelper->MoveMouse(rectangle,  20,  20);
            TestServices::InputHelper->MoveMouse(rectangle, -20,  20);
            TestServices::InputHelper->MoveMouse(rectangle, -20, -20);
            TestServices::WindowHelper->WaitForIdle();

            // Tap the rectangle again, which will cause it to use the old event args
            TestServices::InputHelper->Tap(rectangle);
            pointerPressedEvent->WaitForDefault();

            // We should get here without crashing, and the savedEventArgs should have changed.
            VERIFY_ARE_NOT_EQUAL(oldSavedEventArgs, savedEventArgs);
        }

    } } }
} } } }
