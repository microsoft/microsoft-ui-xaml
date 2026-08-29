// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicGestureTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Foundation::Input::Gestures;

using namespace test_infra;

bool BasicGestureTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool BasicGestureTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool BasicGestureTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void BasicGestureTests::SetupElements(
    _Out_ Microsoft::UI::Xaml::Shapes::Rectangle^* pRect,
    _Out_opt_ Microsoft::UI::Xaml::Controls::Canvas^* pCanvas)
{
    auto rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
    rect->Margin = xaml::ThicknessHelper::FromUniformLength(30);
    rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
    rect->Width = 50;
    rect->Height = 50;

    auto canvas = ref new Canvas();
    canvas->Width = 100;
    canvas->Height = 100;
    canvas->Children->Append(rect);

    TestServices::WindowHelper->WindowContent = canvas;

    *pRect = rect;
    if (pCanvas)
    {
        *pCanvas = canvas;
    }
}

void BasicGestureTests::TapEmptyTree()
{
    const auto& wh = TestServices::WindowHelper;

    // Recover from resetting the visual tree at the end
    auto shutdownGuard = wil::scope_exit([wh]
    {
        wh->ShutdownXaml();
        wh->InitializeXaml();
    });

    std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
    wf::Point point = {10, 10};

    // Xaml's touch hit testing handler doesn't get called if we don't put anything in the tree to begin with, so put
    // some elements in then reset the visual root.

    xaml_shapes::Rectangle^ rect = nullptr;
    RunOnUIThread([&]()
    {
        SetupElements(&rect);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Reset the visual tree. Don't crash.");
    wh->ResetVisualTree();

    LOG_OUTPUT(L"> Tapping nothing. Don't crash.");
    TestServices::InputHelper->Tap(point);

    gestureCompletedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
}

//------------------------------------------------------------------------
// Test case: Tap a Rectangle and listen to the Tapped event using Gestures.
//------------------------------------------------------------------------
void BasicGestureTests::TapARectangle()
{
    TestCleanupWrapper cleanup;
    std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
    xaml_shapes::Rectangle^ rect = nullptr;
    auto rectTappedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, Tapped);

    RunOnUIThread([&]()
    {
        SetupElements(&rect);
        VERIFY_IS_TRUE(rect->IsTapEnabled);

        rectTappedRegistration.Attach(rect, ref new xaml_input::TappedEventHandler(
            [gestureCompletedEvent](Platform::Object^, TappedRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"Tapped event raised.");
            VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Touch);
            gestureCompletedEvent->Set();
        }));
    });

    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Tapping the Rectangle.");
    TestServices::InputHelper->Tap(rect);
    gestureCompletedEvent->WaitForDefault();
}

//------------------------------------------------------------------------
// Test case: Validates the ability to destroy the element from the tap handler
//------------------------------------------------------------------------
void BasicGestureTests::DestroyOnTapARectangle()
{
    TestCleanupWrapper cleanup;
    std::shared_ptr<Event> returnedFromInputHelperEvent = std::make_shared<Event>();
    std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
    xaml_shapes::Rectangle^ rect;
    xaml_controls::Canvas^ canvas;

    // All lambdas capture by reference to not increnent rect refcount
    RunOnUIThread([&]()
    {
        SetupElements(&rect, &canvas);

        // Avoid using SafeEventRegistration here as it would hold an extra hat pointer to rect
        rect->Tapped += ref new xaml_input::TappedEventHandler(
            [&](Platform::Object^, TappedRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"Tapped event raised. Releasing the Rectangle and removing it from the tree.");

            // Make sure LeftMouseClick has finished as it would hold an extra reference to rect
            returnedFromInputHelperEvent->WaitForDefault();

            rect = nullptr;
            canvas->Children->Clear();

            gestureCompletedEvent->Set();
        });
    });

    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Tapping the Rectangle.");
    TestServices::InputHelper->Tap(rect);
    returnedFromInputHelperEvent->Set();
    gestureCompletedEvent->WaitForDefault();
}

// Validates the ability to reparent an element during a tap gesture.
void BasicGestureTests::ReparentRectangleDuringTap()
{
    TestCleanupWrapper cleanup;

    auto pointerPressedEvent = std::make_shared<Event>();
    auto pointerReleasedEvent = std::make_shared<Event>();

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    auto pointerMovedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerMoved);
    auto pointerReleasedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerReleased);
    auto layoutUpdatedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, LayoutUpdated);

    xaml_shapes::Rectangle^ rect;
    xaml_controls::Border^ border1;
    xaml_controls::Border^ border2;
    xaml_controls::Canvas^ canvas;

    bool reparentRect = false;

    RunOnUIThread([&]()
    {
        rect = ref new xaml_shapes::Rectangle();
        rect->Margin = xaml::ThicknessHelper::FromUniformLength(30);
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        rect->Width = 50;
        rect->Height = 50;

        border1 = ref new xaml_controls::Border();
        border1->Background = ref new SolidColorBrush(Microsoft::UI::Colors::SlateBlue);
        border1->Child = rect;

        border2 = ref new xaml_controls::Border();
        border2->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Cyan);

        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;

        canvas->Children->Append(border1);
        canvas->Children->Append(border2);

        TestServices::WindowHelper->WindowContent = canvas;
    });

    pointerPressedRegistration.Attach(
        rect,
        ref new xaml_input::PointerEventHandler(
        [&reparentRect, pointerPressedEvent, rect](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
    {
        LOG_OUTPUT(L"PointerPressed event raised.");
        pointerPressedEvent->Set();

        // Trigger a LayoutUpdated event.
        rect->Width++;

        // Reparent the rect in the next LayoutUpdated event.
        reparentRect = true;
    }));

    pointerMovedRegistration.Attach(
        rect,
        ref new xaml_input::PointerEventHandler(
        [canvas](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
    {
        LOG_OUTPUT(L"PointerMoved event raised. Pointer.PointerDeviceType=%d, Pointer.PointerId=%d", args->Pointer->PointerDeviceType, args->Pointer->PointerId);
        Microsoft::UI::Input::PointerPoint^ ptrPt = args->GetCurrentPoint(canvas);
        LOG_OUTPUT(L"Current PointerPoint.Position = (%.2f, %.2f).", ptrPt->Position.X, ptrPt->Position.Y);
    }));

    pointerReleasedRegistration.Attach(
        rect,
        ref new xaml_input::PointerEventHandler(
        [pointerReleasedEvent](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
    {
        LOG_OUTPUT(L"PointerReleased event raised.");
        pointerReleasedEvent->Set();
    }));

    layoutUpdatedRegistration.Attach(
        rect,
        ref new wf::EventHandler<Platform::Object^>(
        [&reparentRect, border1, border2, rect](Platform::Object^, Platform::Object^)
    {
        LOG_OUTPUT(L"LayoutUpdated event raised.");
        if (reparentRect)
        {
            LOG_OUTPUT(L"Reparenting rect.");
            reparentRect = false;

            xaml::UIElement^ uie1 = border1->Child;
            border1->Child = nullptr;
            border2->Child = uie1;
        }
    }));

    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Tapping the Rectangle.");
    TestServices::InputHelper->Tap(rect);
    pointerPressedEvent->WaitForDefault();
    pointerReleasedEvent->WaitForDefault();
}

//------------------------------------------------------------------------
// Test case: Double-tap a Rectangle and listen to the DoubleTapped event
// using Gestures.
//------------------------------------------------------------------------
void BasicGestureTests::DoubleTapARectangle()
{
    TestCleanupWrapper cleanup;
    UINT tappedEventCount = 0u;
    std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
    auto rectTappedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, Tapped);
    auto rectDoubleTappedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, DoubleTapped);

    RunOnUIThread([&]()
    {
        SetupElements(&rect);
        VERIFY_IS_TRUE(rect->IsDoubleTapEnabled);

        rectTappedRegistration.Attach(rect, ref new xaml_input::TappedEventHandler(
            [&tappedEventCount](Platform::Object^, TappedRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"Tapped event raised.");
            VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Touch);
            // The Tapped event is expected to be raised exactly once before the DoubleTapped event.
            VERIFY_ARE_EQUAL(tappedEventCount, 0u, L"Checking Tapped event count in Tapped event handler.");
            tappedEventCount++;
        }));

        rectDoubleTappedRegistration.Attach(rect, ref new xaml_input::DoubleTappedEventHandler(
            [&tappedEventCount, gestureCompletedEvent](Platform::Object^, DoubleTappedRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"DoubleTapped event raised.");
            VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Touch);
            // The Tapped event is expected to be raised exactly once before the DoubleTapped event.
            VERIFY_ARE_EQUAL(tappedEventCount, 1u, L"Checking Tapped event count in DoubleTapped handler.");
            gestureCompletedEvent->Set();
        }));
    });

    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Double-tapping the Rectangle.");
    TestServices::InputHelper->DoubleTap(rect);
    gestureCompletedEvent->WaitForDefault();
}

//------------------------------------------------------------------------
// Test case: Right-tap a Rectangle and listen to the RightTapped event
// using Gestures.
//------------------------------------------------------------------------
void BasicGestureTests::RightTapARectangle()
{
    TestCleanupWrapper cleanup;
    std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
    auto rectRightTappedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, RightTapped);

    RunOnUIThread([&]()
    {
        SetupElements(&rect);
        VERIFY_IS_TRUE(rect->IsRightTapEnabled);

        rectRightTappedRegistration.Attach(rect, ref new xaml_input::RightTappedEventHandler(
            [gestureCompletedEvent](Platform::Object^, RightTappedRoutedEventArgs^ args)
        {
            VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Touch);
            gestureCompletedEvent->Set();
        }));
    });

    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Right-tapping the Rectangle.");
    // RightTapped is raised when the pointer is released at the end of a hold gesture.
    TestServices::InputHelper->Hold(rect);
    gestureCompletedEvent->WaitForDefault();
}

//------------------------------------------------------------------------
// Test case: Right-tap a Rectangle using pen + barrel button
// and listen to the RightTapped event using Gestures
//------------------------------------------------------------------------
void BasicGestureTests::RightTapARectangleUsingPen()
{
    TestCleanupWrapper cleanup;
    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
    RunOnUIThread([&]()
    {
        SetupElements(&rect);
    });
    TestServices::WindowHelper->WaitForIdle();

    std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
    auto rectRightTappedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, RightTapped);
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(rect->IsRightTapEnabled);

        rectRightTappedRegistration.Attach(rect, ref new xaml_input::RightTappedEventHandler(
            [gestureCompletedEvent](Platform::Object^, RightTappedRoutedEventArgs^ args)
        {
            VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Pen);
            gestureCompletedEvent->Set();
        }));
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Right-tapping the Rectangle using pen + barrel button.");
    // RightTapped is raised when a pen is tapped with the barrel button down.
    TestServices::InputHelper->PenBarrelTap(rect);
    TestServices::WindowHelper->WaitForIdle();
    gestureCompletedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case: Hold a Rectangle and listen to the Holding event using Gestures.
//------------------------------------------------------------------------
void BasicGestureTests::HoldARectangle()
{
    TestCleanupWrapper cleanup;
    UINT holdingEventCount = 0u;
    std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
    auto rectHoldingRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, Holding);

    RunOnUIThread([&]()
    {
        SetupElements(&rect);
        VERIFY_IS_TRUE(rect->IsHoldingEnabled);

        rectHoldingRegistration.Attach(rect, ref new xaml_input::HoldingEventHandler(
            [&holdingEventCount, gestureCompletedEvent](Platform::Object^, HoldingRoutedEventArgs^ args)
        {
            VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Touch);
            switch (args->HoldingState)
            {
            case Microsoft::UI::Input::HoldingState::Started:
                VERIFY_ARE_EQUAL(holdingEventCount, 0u, L"Checking Holding event count in Holding event handler when HolderState is Started.");
                holdingEventCount++;
                break;
            case Microsoft::UI::Input::HoldingState::Completed:
                VERIFY_ARE_EQUAL(holdingEventCount, 1u, L"Checking Holding event count in Holding event handler when HolderState is Completed.");
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
    });

    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Holding the Rectangle.");
    TestServices::InputHelper->Hold(rect);
    gestureCompletedEvent->WaitForDefault();
}
