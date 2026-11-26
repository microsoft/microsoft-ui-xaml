// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ThumbControlIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <ControlHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Thumb {

    namespace
    {
        // Records the drag events raised by a Thumb. Event handlers hold this through a shared_ptr
        // so a late event can never touch the test method's stack.
        struct DragLog
        {
            int startedCount = 0;
            int deltaCount = 0;
            int completedCount = 0;
            bool lastCompletedCanceled = false;
            double lastCompletedHorizontalChange = -1.0;
            double lastCompletedVerticalChange = -1.0;
        };

        // Records pointer presses that reach a Thumb, including presses already handled below it.
        struct PressLog
        {
            int pressedCount = 0;
            bool lastPressedWasHandled = false;
        };

        // Must be called on the UI thread.
        void AttachDragLog(
            xaml_primitives::Thumb^ thumb,
            std::shared_ptr<DragLog> log,
            std::shared_ptr<Event> dragStartedEvent,
            std::shared_ptr<Event> dragCompletedEvent)
        {
            thumb->DragStarted += ref new xaml_primitives::DragStartedEventHandler(
                [log, dragStartedEvent](Platform::Object^, xaml_primitives::DragStartedEventArgs^)
                {
                    ++log->startedCount;
                    dragStartedEvent->Set();
                });

            thumb->DragDelta += ref new xaml_primitives::DragDeltaEventHandler(
                [log](Platform::Object^, xaml_primitives::DragDeltaEventArgs^)
                {
                    ++log->deltaCount;
                });

            thumb->DragCompleted += ref new xaml_primitives::DragCompletedEventHandler(
                [log, dragCompletedEvent](Platform::Object^, xaml_primitives::DragCompletedEventArgs^ args)
                {
                    ++log->completedCount;
                    log->lastCompletedCanceled = args->Canceled;
                    log->lastCompletedHorizontalChange = args->HorizontalChange;
                    log->lastCompletedVerticalChange = args->VerticalChange;
                    dragCompletedEvent->Set();
                });
        }

        // Must be called on the UI thread.
        void AttachPressLog(xaml_primitives::Thumb^ thumb, std::shared_ptr<PressLog> log)
        {
            thumb->AddHandler(
                xaml::UIElement::PointerPressedEvent,
                ref new xaml_input::PointerEventHandler(
                    [log](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
                    {
                        ++log->pressedCount;
                        log->lastPressedWasHandled = args->Handled;
                    }),
                true /*handledEventsToo*/);
        }

        // Must be called on the UI thread. The Thumb is placed in a Grid because Thumb only starts a drag
        // when it has a logical parent.
        xaml_primitives::Thumb^ CreateThumbInRootGrid()
        {
            auto root = ref new xaml_controls::Grid();
            auto thumb = ref new xaml_primitives::Thumb();
            thumb->Width = 100;
            thumb->Height = 100;
            thumb->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            thumb->VerticalAlignment = xaml::VerticalAlignment::Center;
            root->Children->Append(thumb);
            TestServices::WindowHelper->WindowContent = root;
            return thumb;
        }

        xaml_primitives::Thumb^ GetTemplateThumb(xaml::FrameworkElement^ owner, const wchar_t* thumbName)
        {
            xaml_primitives::Thumb^ thumb = nullptr;
            RunOnUIThread([&]()
            {
                thumb = dynamic_cast<xaml_primitives::Thumb^>(TreeHelper::GetVisualChildByName(owner, ref new Platform::String(thumbName)));
            });
            VERIFY_IS_NOT_NULL(thumb);
            return thumb;
        }

        // Releases the left mouse button if a test exits early, so a failed assertion cannot leave it pressed
        // for the tests that run after it.
        auto ReleaseMouseOnExit(xaml::FrameworkElement^ element, int dx, int dy)
        {
            return wil::scope_exit([element, dx, dy]()
            {
                try { TestServices::InputHelper->MouseButtonUp(element, dx, dy, MouseButton::Left); } catch (...) {}
            });
        }

        // Drags with the left mouse button from the element's center to the given offset, then releases the
        // button. InputHelper::MouseDrag leaves the button pressed.
        void MouseDragAndRelease(xaml::FrameworkElement^ element, int dx, int dy)
        {
            auto releaseMouse = ReleaseMouseOnExit(element, dx, dy);
            TestServices::InputHelper->MouseDrag(element, 0, 0, element, dx, dy, MouseButton::Left);
            TestServices::InputHelper->MouseButtonUp(element, dx, dy, MouseButton::Left);
            releaseMouse.release();
        }
    }

    bool ThumbControlIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ThumbControlIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ThumbControlIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ThumbControlIntegrationTests::CancelDragWhenNotDraggingIsNoOp()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::Thumb^ thumb = nullptr;
        auto log = std::make_shared<DragLog>();
        auto dragStartedEvent = std::make_shared<Event>();
        auto dragCompletedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            thumb = CreateThumbInRootGrid();
            AttachDragLog(thumb, log, dragStartedEvent, dragCompletedEvent);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(thumb->IsDragging);

            thumb->CancelDrag();

            // Without an active drag there is nothing to cancel: no state change and no DragCompleted.
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_IS_FALSE(safe_cast<bool>(thumb->GetValue(xaml_primitives::Thumb::IsDraggingProperty)));
            VERIFY_ARE_EQUAL(0, log->completedCount);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, log->startedCount);
            VERIFY_ARE_EQUAL(0, log->completedCount);
        });
    }

    void ThumbControlIntegrationTests::CancelDragDuringMouseDragRaisesCanceledDragCompleted()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::Thumb^ thumb = nullptr;
        auto log = std::make_shared<DragLog>();
        auto dragStartedEvent = std::make_shared<Event>();
        auto dragCompletedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            thumb = CreateThumbInRootGrid();
            AttachDragLog(thumb, log, dragStartedEvent, dragCompletedEvent);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press the left mouse button on the thumb to start a drag.");
        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        auto releaseMouse = ReleaseMouseOnExit(thumb, 0, 0);
        dragStartedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, log->startedCount);
            VERIFY_IS_TRUE(thumb->IsDragging);
            VERIFY_IS_TRUE(safe_cast<bool>(thumb->GetValue(xaml_primitives::Thumb::IsDraggingProperty)));
            VERIFY_ARE_EQUAL(0, log->completedCount);
            // The press that started the drag captured the pointer, so the drag keeps receiving input
            // even if the pointer leaves the thumb.
            VERIFY_IS_NOT_NULL(thumb->PointerCaptures);
            VERIFY_ARE_EQUAL(1u, thumb->PointerCaptures->Size);

            thumb->CancelDrag();

            // CancelDrag ends the drag synchronously and reports it as canceled. The pointer never moved,
            // so the reported change is zero.
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_IS_FALSE(safe_cast<bool>(thumb->GetValue(xaml_primitives::Thumb::IsDraggingProperty)));
            VERIFY_ARE_EQUAL(1, log->completedCount);
            VERIFY_IS_TRUE(log->lastCompletedCanceled);
            VERIFY_ARE_EQUAL(0.0, log->lastCompletedHorizontalChange);
            VERIFY_ARE_EQUAL(0.0, log->lastCompletedVerticalChange);
        });

        LOG_OUTPUT(L"Release the mouse button; the canceled drag must not complete a second time.");
        TestServices::InputHelper->MouseButtonUp(thumb, 0, 0, MouseButton::Left);
        releaseMouse.release();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(1, log->startedCount);
            VERIFY_ARE_EQUAL(1, log->completedCount);
        });
    }

    void ThumbControlIntegrationTests::RemovedDragHandlersAreNotInvoked()
    {
        TestCleanupWrapper cleanup;

        struct HandlerCounts
        {
            int startedKept = 0;
            int startedRemoved = 0;
            int deltaRemovedFirst = 0;
            int deltaRemovedSecond = 0;
            int deltaAddedAgain = 0;
            int deltaRemovedThird = 0;
            int completedKept = 0;
            int completedRemoved = 0;
            double lastHorizontalChange = 0.0;
        };

        xaml_primitives::Thumb^ thumb = nullptr;
        auto counts = std::make_shared<HandlerCounts>();
        auto completedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            thumb = CreateThumbInRootGrid();

            thumb->DragStarted += ref new xaml_primitives::DragStartedEventHandler(
                [counts](Platform::Object^, xaml_primitives::DragStartedEventArgs^) { ++counts->startedKept; });
            auto startedRemovedToken = thumb->DragStarted += ref new xaml_primitives::DragStartedEventHandler(
                [counts](Platform::Object^, xaml_primitives::DragStartedEventArgs^) { ++counts->startedRemoved; });

            auto deltaFirstToken = thumb->DragDelta += ref new xaml_primitives::DragDeltaEventHandler(
                [counts](Platform::Object^, xaml_primitives::DragDeltaEventArgs^) { ++counts->deltaRemovedFirst; });
            auto deltaSecondToken = thumb->DragDelta += ref new xaml_primitives::DragDeltaEventHandler(
                [counts](Platform::Object^, xaml_primitives::DragDeltaEventArgs^) { ++counts->deltaRemovedSecond; });

            thumb->DragCompleted += ref new xaml_primitives::DragCompletedEventHandler(
                [counts, completedEvent](Platform::Object^, xaml_primitives::DragCompletedEventArgs^ args)
                {
                    ++counts->completedKept;
                    counts->lastHorizontalChange = args->HorizontalChange;
                    completedEvent->Set();
                });
            auto completedRemovedToken = thumb->DragCompleted += ref new xaml_primitives::DragCompletedEventHandler(
                [counts](Platform::Object^, xaml_primitives::DragCompletedEventArgs^) { ++counts->completedRemoved; });

            thumb->DragStarted -= startedRemovedToken;
            thumb->DragDelta -= deltaFirstToken;
            // This removes the last DragDelta handler.
            thumb->DragDelta -= deltaSecondToken;
            thumb->DragCompleted -= completedRemovedToken;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Drag the thumb 30 pixels to the right with the mouse.");
        MouseDragAndRelease(thumb, 30, 0);
        completedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, counts->startedKept);
            VERIFY_ARE_EQUAL(1, counts->completedKept);
            // The pointer really moved during the drag, so DragDelta was raised; only removed handlers listened.
            VERIFY_IS_GREATER_THAN(counts->lastHorizontalChange, 0.0);
            VERIFY_ARE_EQUAL(0, counts->startedRemoved);
            VERIFY_ARE_EQUAL(0, counts->deltaRemovedFirst);
            VERIFY_ARE_EQUAL(0, counts->deltaRemovedSecond);
            VERIFY_ARE_EQUAL(0, counts->completedRemoved);

            thumb->DragDelta += ref new xaml_primitives::DragDeltaEventHandler(
                [counts](Platform::Object^, xaml_primitives::DragDeltaEventArgs^) { ++counts->deltaAddedAgain; });
            auto deltaThirdToken = thumb->DragDelta += ref new xaml_primitives::DragDeltaEventHandler(
                [counts](Platform::Object^, xaml_primitives::DragDeltaEventArgs^) { ++counts->deltaRemovedThird; });

            // Removing one of two DragDelta handlers must leave the other one registered.
            thumb->DragDelta -= deltaThirdToken;
        });

        LOG_OUTPUT(L"Drag again after adding two DragDelta handlers and removing one of them.");
        MouseDragAndRelease(thumb, 30, 0);
        completedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, counts->startedKept);
            VERIFY_ARE_EQUAL(2, counts->completedKept);
            VERIFY_IS_GREATER_THAN(counts->deltaAddedAgain, 0);
            VERIFY_ARE_EQUAL(0, counts->deltaRemovedThird);
            VERIFY_ARE_EQUAL(0, counts->startedRemoved);
            VERIFY_ARE_EQUAL(0, counts->deltaRemovedFirst);
            VERIFY_ARE_EQUAL(0, counts->deltaRemovedSecond);
            VERIFY_ARE_EQUAL(0, counts->completedRemoved);
        });
    }

    void ThumbControlIntegrationTests::FocusStatesFollowThumbFocus()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::Thumb^ thumb = nullptr;
        xaml_controls::Button^ otherButton = nullptr;
        auto gotFocusEvent = std::make_shared<Event>();
        auto lostFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, GotFocus);
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_primitives::Thumb, LostFocus);

        RunOnUIThread([&]()
        {
            // The default Thumb style is not a tab stop; this one is, and its template has FocusStates.
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <StackPanel HorizontalAlignment="Center" VerticalAlignment="Center">
                            <Thumb x:Name="thumb" Width="100" Height="50" IsTabStop="True">
                                <Thumb.Template>
                                    <ControlTemplate TargetType="Thumb">
                                        <Grid Background="Gray">
                                            <VisualStateManager.VisualStateGroups>
                                                <VisualStateGroup x:Name="CommonStates">
                                                    <VisualState x:Name="Normal" />
                                                    <VisualState x:Name="PointerOver" />
                                                    <VisualState x:Name="Pressed" />
                                                    <VisualState x:Name="Disabled" />
                                                </VisualStateGroup>
                                                <VisualStateGroup x:Name="FocusStates">
                                                    <VisualState x:Name="Focused" />
                                                    <VisualState x:Name="Unfocused" />
                                                </VisualStateGroup>
                                            </VisualStateManager.VisualStateGroups>
                                        </Grid>
                                    </ControlTemplate>
                                </Thumb.Template>
                            </Thumb>
                            <Button x:Name="otherButton" Content="Other" />
                        </StackPanel>
                    </Grid>)"));

            thumb = safe_cast<xaml_primitives::Thumb^>(root->FindName(L"thumb"));
            otherButton = safe_cast<xaml_controls::Button^>(root->FindName(L"otherButton"));

            gotFocusRegistration.Attach(thumb, [gotFocusEvent]() { gotFocusEvent->Set(); });
            lostFocusRegistration.Attach(thumb, [lostFocusEvent]() { lostFocusEvent->Set(); });

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto focusStates = ControlHelper::GetVisualStateGroup(thumb, L"FocusStates");
        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(focusStates);
            VERIFY_IS_NOT_NULL(focusStates->CurrentState);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"Unfocused"), focusStates->CurrentState->Name);
        });

        LOG_OUTPUT(L"Give the thumb keyboard focus.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(thumb->Focus(xaml::FocusState::Keyboard));
        });
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(static_cast<int>(xaml::FocusState::Keyboard), static_cast<int>(thumb->FocusState));
            VERIFY_ARE_EQUAL(Platform::StringReference(L"Focused"), focusStates->CurrentState->Name);
        });

        LOG_OUTPUT(L"Move focus to another element.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(otherButton->Focus(xaml::FocusState::Keyboard));
        });
        lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(static_cast<int>(xaml::FocusState::Unfocused), static_cast<int>(thumb->FocusState));
            VERIFY_ARE_EQUAL(Platform::StringReference(L"Unfocused"), focusStates->CurrentState->Name);
        });
    }

    void ThumbControlIntegrationTests::ScrollBarThumbDoesNotDragWhenScrollModeDisabled()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        auto dragLog = std::make_shared<DragLog>();
        auto pressLog = std::make_shared<PressLog>();
        auto dragStartedEvent = std::make_shared<Event>();
        auto dragCompletedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::Grid();
            auto content = ref new xaml_controls::Grid();
            content->Width = 150;
            content->Height = 1000;

            scrollViewer = ref new xaml_controls::ScrollViewer();
            scrollViewer->Width = 200;
            scrollViewer->Height = 200;
            scrollViewer->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            scrollViewer->VerticalAlignment = xaml::VerticalAlignment::Center;
            scrollViewer->VerticalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Visible;
            scrollViewer->VerticalScrollMode = xaml_controls::ScrollMode::Disabled;
            scrollViewer->Content = content;

            root->Children->Append(scrollViewer);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto thumb = GetTemplateThumb(scrollViewer, L"VerticalThumb");
        RunOnUIThread([&]()
        {
            AttachDragLog(thumb, dragLog, dragStartedEvent, dragCompletedEvent);
            AttachPressLog(thumb, pressLog);
            VERIFY_ARE_EQUAL(0.0, scrollViewer->VerticalOffset);
        });

        LOG_OUTPUT(L"Press the vertical thumb while VerticalScrollMode is Disabled.");
        TestServices::InputHelper->MoveMouse(scrollViewer);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        auto releaseFirstPress = ReleaseMouseOnExit(thumb, 0, 0);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // The press reached the thumb, but the ScrollBar is ignoring user input.
            VERIFY_ARE_EQUAL(1, pressLog->pressedCount);
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(0, dragLog->startedCount);
        });

        TestServices::InputHelper->MouseButtonUp(thumb, 0, 0, MouseButton::Left);
        releaseFirstPress.release();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, dragLog->completedCount);
            VERIFY_ARE_EQUAL(0.0, scrollViewer->VerticalOffset);
            scrollViewer->VerticalScrollMode = xaml_controls::ScrollMode::Enabled;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press the same thumb after enabling vertical scrolling.");
        TestServices::InputHelper->MoveMouse(scrollViewer);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        auto releaseSecondPress = ReleaseMouseOnExit(thumb, 0, 0);
        dragStartedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pressLog->pressedCount);
            VERIFY_IS_TRUE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(1, dragLog->startedCount);
        });

        TestServices::InputHelper->MouseButtonUp(thumb, 0, 0, MouseButton::Left);
        releaseSecondPress.release();
        dragCompletedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(1, dragLog->completedCount);
        });
    }

    void ThumbControlIntegrationTests::DoesNotStartDragWhenPointerPressedAlreadyHandled()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::Thumb^ thumb = nullptr;
        auto dragLog = std::make_shared<DragLog>();
        auto pressLog = std::make_shared<PressLog>();
        auto rootPressLog = std::make_shared<PressLog>();
        auto dragStartedEvent = std::make_shared<Event>();
        auto dragCompletedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            // The Button covers the left part of the Thumb and handles the presses it receives.
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Thumb x:Name="thumb" Width="200" Height="100" HorizontalAlignment="Center" VerticalAlignment="Center">
                            <Thumb.Template>
                                <ControlTemplate TargetType="Thumb">
                                    <Grid Background="Gray">
                                        <Button x:Name="innerButton" Width="80" Height="60" HorizontalAlignment="Left" VerticalAlignment="Center" Content="Inner" />
                                    </Grid>
                                </ControlTemplate>
                            </Thumb.Template>
                        </Thumb>
                    </Grid>)"));

            thumb = safe_cast<xaml_primitives::Thumb^>(root->FindName(L"thumb"));

            // The root Grid is the Thumb's parent; presses bubble to it after the Thumb has processed them.
            root->AddHandler(
                xaml::UIElement::PointerPressedEvent,
                ref new xaml_input::PointerEventHandler(
                    [rootPressLog](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
                    {
                        ++rootPressLog->pressedCount;
                        rootPressLog->lastPressedWasHandled = args->Handled;
                    }),
                true /*handledEventsToo*/);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::Button^ innerButton = nullptr;
        RunOnUIThread([&]()
        {
            innerButton = dynamic_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(thumb, L"innerButton"));
            AttachDragLog(thumb, dragLog, dragStartedEvent, dragCompletedEvent);
            AttachPressLog(thumb, pressLog);
        });
        VERIFY_IS_NOT_NULL(innerButton);

        LOG_OUTPUT(L"Press the Button inside the Thumb's template.");
        TestServices::InputHelper->MouseButtonDown(innerButton, 0, 0, MouseButton::Left);
        auto releaseButtonPress = ReleaseMouseOnExit(innerButton, 0, 0);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // The press bubbled to the Thumb already handled by the Button, so no drag starts.
            VERIFY_ARE_EQUAL(1, pressLog->pressedCount);
            VERIFY_IS_TRUE(pressLog->lastPressedWasHandled);
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(0, dragLog->startedCount);
            VERIFY_ARE_EQUAL(1, rootPressLog->pressedCount);
        });

        TestServices::InputHelper->MouseButtonUp(innerButton, 0, 0, MouseButton::Left);
        releaseButtonPress.release();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press the Thumb's own background, to the right of the Button.");
        TestServices::InputHelper->MouseButtonDown(thumb, 60, 0, MouseButton::Left);
        auto releaseThumbPress = ReleaseMouseOnExit(thumb, 60, 0);
        dragStartedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pressLog->pressedCount);
            VERIFY_IS_TRUE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(1, dragLog->startedCount);
            // The Thumb marks the press that starts a drag as handled before it bubbles to the parent.
            VERIFY_ARE_EQUAL(2, rootPressLog->pressedCount);
            VERIFY_IS_TRUE(rootPressLog->lastPressedWasHandled);
        });

        TestServices::InputHelper->MouseButtonUp(thumb, 60, 0, MouseButton::Left);
        releaseThumbPress.release();
        dragCompletedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(thumb->IsDragging);
            VERIFY_ARE_EQUAL(1, dragLog->completedCount);
            VERIFY_IS_FALSE(dragLog->lastCompletedCanceled);
        });
    }

} } } } } } } // Microsoft::UI::Xaml::Tests::Controls::Primitives::Thumb
