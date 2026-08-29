// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "InteractionTests.h"
#include "Layering.CustomTypes.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FeatureFlags.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layering {

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

// TODO:  Dynamically enable velocity instead of skipping test
#define VELOCITY_TESTGUARD_XAML2018 if (!Feature_Xaml2018::IsEnabled()) { LOG_OUTPUT(L"XAML2018 velocity feature disabled, skipping test"); return; }

    bool InteractionTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool InteractionTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool InteractionTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void InteractionTests::CanAddInteractionsToElements()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            LOG_OUTPUT(L"Validate attaching interactions on a FrameworkElement based type.");
            auto element = ref new xaml_shapes::Rectangle();

            VERIFY_IS_NOT_NULL(element->Interactions);
            VERIFY_ARE_EQUAL(0u, element->Interactions->Size);

            element->Interactions->Append(ref new PointerInteractions());
            VERIFY_ARE_EQUAL(1u, element->Interactions->Size);

            LOG_OUTPUT(L"Validate attaching interactions on a FrameworkElementEx based type.");
            auto element2 = ref new CustomFrameworkElementEx();

            VERIFY_IS_NOT_NULL(element2->Interactions);
            VERIFY_ARE_EQUAL(0u, element2->Interactions->Size);

            element2->Interactions->Append(ref new PointerInteractions());
            VERIFY_ARE_EQUAL(1u, element2->Interactions->Size);
        });
    }

    void InteractionTests::DoesReceiveKeyEvents()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        xaml_controls::ContentControl^ element = nullptr;
        KeyInteractions^ interaction = nullptr;

        RunOnUIThread([&]()
        {
            element = ref new xaml_controls::ContentControl();
            element->Width = 100;
            element->Height = 100;
            element->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            element->IsTabStop = true;

            interaction = ref new KeyInteractions();

            element->Interactions->Append(interaction);

            TestServices::WindowHelper->WindowContent = element;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Focus the target element.");
        RunOnUIThread([&]()
        {
            element->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        auto keyDownRegistration = CreateSafeEventRegistration(KeyInteractions, KeyDown);
        auto keyUpRegistration = CreateSafeEventRegistration(KeyInteractions, KeyUp);
        auto keyDownUIERegistration = CreateSafeEventRegistration(xaml::UIElement, KeyDown);

        Event keyDownEvent;
        Event keyUpEvent;
        Event keyDownUIEEvent;

        keyDownRegistration.Attach(interaction, [&]() { keyDownEvent.Set(); });
        keyUpRegistration.Attach(interaction, [&]() { keyUpEvent.Set(); });
        keyDownUIERegistration.Attach(element, [&]() { keyDownUIEEvent.Set(); });

        LOG_OUTPUT(L"Press a key with focus on the target element.");
        TestServices::KeyboardHelper->PressKeySequence("a");
        keyDownEvent.WaitForDefault();
        keyUpEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate that corresponding UIElement event did not get raised.");
        VERIFY_ARE_EQUAL(0, keyDownUIEEvent.TimesFired());

        LOG_OUTPUT(L"Remove the interaction from the target element.");
        RunOnUIThread([&]()
        {
            element->Interactions->RemoveAt(0);
        });

        keyDownEvent.Reset();
        keyUpEvent.Reset();

        LOG_OUTPUT(L"Press a key with focus on the target element, but with no interaction object attached.");
        TestServices::KeyboardHelper->PressKeySequence("a");
        keyDownUIEEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(0, keyDownEvent.TimesFired());
        VERIFY_ARE_EQUAL(0, keyUpEvent.TimesFired());
    }

    void InteractionTests::DoesReceivePointerEvents()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup([]()
        {
            TestServices::InputHelper->MoveMouse(wf::Point(0,0));
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_shapes::Rectangle^ element = nullptr;
        xaml_controls::Button^ dummyButton = nullptr;
        PointerInteractions^ interaction = nullptr;

        RunOnUIThread([&]()
        {
            element = ref new xaml_shapes::Rectangle();
            element->Width = 100;
            element->Height = 100;
            element->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);

            interaction = ref new PointerInteractions();

            element->Interactions->Append(interaction);

            dummyButton = ref new xaml_controls::Button();

            auto root = ref new xaml_controls::StackPanel();
            root->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            root->VerticalAlignment = xaml::VerticalAlignment::Center;

            root->Children->Append(dummyButton);
            root->Children->Append(element);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto pointerEnteredRegistration = CreateSafeEventRegistration(PointerInteractions, PointerEntered);
        auto pointerExitedRegistration = CreateSafeEventRegistration(PointerInteractions, PointerExited);
        auto pointerMovedRegistration = CreateSafeEventRegistration(PointerInteractions, PointerMoved);
        auto pointerPressedRegistration = CreateSafeEventRegistration(PointerInteractions, PointerPressed);
        auto pointerReleasedRegistration = CreateSafeEventRegistration(PointerInteractions, PointerReleased);
        auto pointerCaptureLostRegistration = CreateSafeEventRegistration(PointerInteractions, PointerCaptureLost);
        auto pointerWheelChangedRegistration = CreateSafeEventRegistration(PointerInteractions, PointerWheelChanged);

        Event pointerEntered;
        Event pointerExited;
        Event pointerMoved;
        Event pointerPressed;
        Event pointerReleased;
        Event pointerCaptureLost;
        Event pointerWheelChanged;

        pointerEnteredRegistration.Attach(interaction, [&]() { pointerEntered.Set(); });
        pointerExitedRegistration.Attach(interaction, [&]() { pointerExited.Set(); });
        pointerMovedRegistration.Attach(interaction, [&]() { pointerMoved.Set(); });
        pointerPressedRegistration.Attach(interaction, [&]() { pointerPressed.Set(); });
        pointerReleasedRegistration.Attach(interaction, [&]() { pointerReleased.Set(); });
        pointerCaptureLostRegistration.Attach(interaction, [&]() { pointerCaptureLost.Set(); });
        pointerWheelChangedRegistration.Attach(interaction, [&]() { pointerWheelChanged.Set(); });

        // Move the mouse to a dummy control to be certain that the mouse is not over the test button.
        TestServices::InputHelper->MoveMouse(dummyButton);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Move the mouse over the target element.");
        TestServices::InputHelper->MoveMouse(element);
        pointerEntered.WaitForDefault();
        pointerMoved.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Left click the mouse over the target element.");
        TestServices::InputHelper->LeftMouseClick(element);
        pointerPressed.WaitForDefault();
        pointerReleased.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Scroll the mouse wheel over the target element.");
        TestServices::InputHelper->ScrollMouseWheel(element, 10);
        pointerWheelChanged.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Capture the mouse when pointer pressed on the target element.");
        interaction->CaptureOnPointerPressed(true);
        TestServices::InputHelper->LeftMouseClick(element);
        pointerPressed.WaitForDefault();
        pointerCaptureLost.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Move the mouse off the target element.");
        TestServices::InputHelper->MoveMouse(wf::Point(0, 0));
        pointerExited.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Remove the interaction from the target element.");
        RunOnUIThread([&]()
        {
            element->Interactions->RemoveAtEnd();
        });

        LOG_OUTPUT(L"Move the mouse over the target element, but with no interaction object attached.");
        pointerEntered.Reset();
        pointerMoved.Reset();

        TestServices::InputHelper->MoveMouse(element);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(0, pointerEntered.TimesFired());
        VERIFY_ARE_EQUAL(0, pointerMoved.TimesFired());
    }

    void InteractionTests::DoesReceiveTouchTapEvents()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        xaml_shapes::Rectangle^ element = nullptr;
        TouchInteractions^ interaction = nullptr;

        RunOnUIThread([&]()
        {
            element = ref new xaml_shapes::Rectangle();
            element->Width = 100;
            element->Height = 100;
            element->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            element->IsHoldingEnabled = true;

            interaction = ref new TouchInteractions();

            element->Interactions->Append(interaction);

            TestServices::WindowHelper->WindowContent = element;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto tappedRegistration = CreateSafeEventRegistration(TouchInteractions, Tapped);
        auto doubleTappedRegistration = CreateSafeEventRegistration(TouchInteractions, DoubleTapped);
        auto holdingRegistration = CreateSafeEventRegistration(TouchInteractions, Holding);
        auto rightTappedRegistration = CreateSafeEventRegistration(TouchInteractions, RightTapped);

        Event tappedEvent;
        Event doubleTappedEvent;
        Event holdingEvent;
        Event rightTappedEvent;

        tappedRegistration.Attach(interaction, [&]() { tappedEvent.Set(); });
        doubleTappedRegistration.Attach(interaction, [&]() { doubleTappedEvent.Set(); });
        holdingRegistration.Attach(interaction, [&]() { holdingEvent.Set(); });
        rightTappedRegistration.Attach(interaction, [&]() { rightTappedEvent.Set(); });

        LOG_OUTPUT(L"Tap on the target element.");
        TestServices::InputHelper->Tap(element);
        TestServices::WindowHelper->WaitForIdle();
        tappedEvent.WaitForDefault();

        LOG_OUTPUT(L"Double-Tap on the target element.");
        TestServices::InputHelper->DoubleTap(element);
        TestServices::WindowHelper->WaitForIdle();
        doubleTappedEvent.WaitForDefault();

        LOG_OUTPUT(L"Remove the interaction from the target element.");
        RunOnUIThread([&]()
        {
            element->Interactions->Clear();
        });

        LOG_OUTPUT(L"Tap on the target element, but with no interaction object attached.");
        tappedEvent.Reset();
        TestServices::InputHelper->Tap(element);
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_ARE_EQUAL(0, tappedEvent.TimesFired());
    }

    void InteractionTests::DoesReceiveTouchHoldingEvents()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        xaml_shapes::Rectangle^ element = nullptr;
        TouchInteractions^ interaction = nullptr;

        RunOnUIThread([&]()
        {
            element = ref new xaml_shapes::Rectangle();
            element->Width = 100;
            element->Height = 100;
            element->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);

            interaction = ref new TouchInteractions();

            element->Interactions->Append(interaction);

            TestServices::WindowHelper->WindowContent = element;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto holdingRegistration = CreateSafeEventRegistration(TouchInteractions, Holding);
        auto rightTappedRegistration = CreateSafeEventRegistration(TouchInteractions, RightTapped);

        Event holdingEvent;
        Event rightTappedEvent;

        holdingRegistration.Attach(interaction, [&]() { holdingEvent.Set(); });
        rightTappedRegistration.Attach(interaction, [&]() { rightTappedEvent.Set(); });

        LOG_OUTPUT(L"Hold on the target element.");
        TestServices::InputHelper->Hold(element);
        TestServices::WindowHelper->WaitForIdle();

        holdingEvent.WaitForDefault();
        rightTappedEvent.WaitForDefault();
    }

    void InteractionTests::DoesReceiveDragEvents()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        xaml_shapes::Rectangle^ dragSource = nullptr;
        xaml_shapes::Rectangle^ dropTarget = nullptr;
        DragDropInteractions^ interaction = nullptr;

        RunOnUIThread([&]()
        {
            dragSource = ref new xaml_shapes::Rectangle();
            dragSource->Width = 100;
            dragSource->Height = 100;
            dragSource->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            dragSource->CanDrag = true;

            dropTarget = ref new xaml_shapes::Rectangle();
            dropTarget->Width = 250;
            dropTarget->Height = 250;
            dropTarget->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
            dropTarget->AllowDrop = true;

            auto root = ref new xaml_controls::StackPanel();
            root->Children->Append(dragSource);
            root->Children->Append(dropTarget);

            interaction = ref new DragDropInteractions();

            dropTarget->Interactions->Append(interaction);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        wf::Size dragSourceSize{};
        wf::Size dropTargetSize{};

        RunOnUIThread([&]()
        {
            dragSourceSize.Width = static_cast<float>(dragSource->ActualWidth);
            dragSourceSize.Height = static_cast<float>(dragSource->ActualHeight);

            dropTargetSize.Width = static_cast<float>(dropTarget->ActualWidth);
            dropTargetSize.Height = static_cast<float>(dropTarget->ActualHeight);
        });

        auto dragEnterRegistration = CreateSafeEventRegistration(DragDropInteractions, DragEnter);
        auto dragLeaveRegistration = CreateSafeEventRegistration(DragDropInteractions, DragLeave);
        auto dragOverRegistration = CreateSafeEventRegistration(DragDropInteractions, DragOver);

        Event dragEnterEvent;
        Event dragLeaveEvent;
        Event dragOverEvent;

        dragEnterRegistration.Attach(interaction, [&]() { dragEnterEvent.Set(); });
        dragLeaveRegistration.Attach(interaction, [&]() { dragLeaveEvent.Set(); });
        dragOverRegistration.Attach(interaction, [&]() { dragOverEvent.Set(); });

        LOG_OUTPUT(L"Drag our drag source over our drop target element.");
        TestServices::InputHelper->PressHoldAndPanFromCenter(dragSource, 0, static_cast<int>(dragSourceSize.Height + dropTargetSize.Height), 0.5 /*velocityFactor*/, 1000 /*holdTime*/ );
        TestServices::WindowHelper->WaitForIdle();
        dragEnterEvent.WaitForDefault();
        dragOverEvent.WaitForDefault();
        dragLeaveEvent.WaitForDefault();
    }

    void InteractionTests::DoesReceiveDropEvent()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        xaml_shapes::Rectangle^ dragSource = nullptr;
        xaml_shapes::Rectangle^ dropTarget = nullptr;
        DragDropInteractions^ interaction = nullptr;

        RunOnUIThread([&]()
        {
            dragSource = ref new xaml_shapes::Rectangle();
            dragSource->Width = 100;
            dragSource->Height = 100;
            dragSource->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            dragSource->CanDrag = true;

            dropTarget = ref new xaml_shapes::Rectangle();
            dropTarget->Width = 250;
            dropTarget->Height = 250;
            dropTarget->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
            dropTarget->AllowDrop = true;

            auto root = ref new xaml_controls::StackPanel();
            root->Children->Append(dragSource);
            root->Children->Append(dropTarget);

            interaction = ref new DragDropInteractions();

            dropTarget->Interactions->Append(interaction);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        wf::Size dragSourceSize{};
        wf::Size dropTargetSize{};

        RunOnUIThread([&]()
        {
            dragSourceSize.Width = static_cast<float>(dragSource->ActualWidth);
            dragSourceSize.Height = static_cast<float>(dragSource->ActualHeight);

            dropTargetSize.Width = static_cast<float>(dropTarget->ActualWidth);
            dropTargetSize.Height = static_cast<float>(dropTarget->ActualHeight);
        });

        auto dragEnterRegistration = CreateSafeEventRegistration(DragDropInteractions, DragEnter);
        auto dropRegistration = CreateSafeEventRegistration(DragDropInteractions, Drop);

        Event dropEvent;

        dragEnterRegistration.Attach(interaction, ref new wf::TypedEventHandler<xaml::UIElement^, xaml::DragEventArgs^>([&](xaml::UIElement^ /*sender*/, xaml::DragEventArgs^ args)
        {
            args->AcceptedOperation = ::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Copy;
        }));

        dropRegistration.Attach(interaction, [&]() { dropEvent.Set(); });

        LOG_OUTPUT(L"Drag our drag source over our drop target element.");
        TestServices::InputHelper->PressHoldAndPanFromCenter(dragSource, 0, static_cast<int>(dragSourceSize.Height * 0.5 + dropTargetSize.Height * 0.5), 0.5 /*velocityFactor*/, 1000 /*holdTime*/ );
        TestServices::WindowHelper->WaitForIdle();
        dropEvent.WaitForDefault();
    }
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layering
