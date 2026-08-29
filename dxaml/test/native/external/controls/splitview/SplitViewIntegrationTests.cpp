// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SplitViewIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include <TreeHelper.h>

#include <ControlHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Windows::ApplicationModel::DataTransfer;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace SplitView {

    bool SplitViewIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        // Disable control state transitions to reduce test execution time.
        featureDisableTransitionsForTest.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTransitionsForTest, true);
        return true;
    }

    bool SplitViewIntegrationTests::ClassCleanup()
    {
        return true;
    }

    bool SplitViewIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool SplitViewIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void SplitViewIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::SplitView>::CanInstantiate();
    }

    void SplitViewIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::SplitView>::CanEnterAndLeaveLiveTree();
    }

    void SplitViewIntegrationTests::CanDragFromPane()
    {
        auto verificationFunction = [](Platform::String^ targetListViewName)
        {
            TestCleanupWrapper cleanup;

            xaml_controls::SplitView^ splitView = nullptr;
            xaml_controls::ListViewItem^ itemToDrag = nullptr;
            xaml_controls::ListView^ targetListView = nullptr;

            auto itemDroppedEvent = std::make_shared<Event>();

            auto listViewItemDragStartingRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, DragStarting);
            auto listViewDragOverRegistration = CreateSafeEventRegistration(xaml_controls::ListView, DragOver);
            auto listViewDropRegistration = CreateSafeEventRegistration(xaml_controls::ListView, Drop);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"            Width='400'>"
                    L"    <SplitView x:Name='SplitView' IsPaneOpen='True'>"
                    L"        <SplitView.Pane>"
                    L"            <ListView>"
                    L"                <ListViewItem>Item 1</ListViewItem>"
                    L"                <ListViewItem x:Name='ItemToDrag' CanDrag='True'>Item 2</ListViewItem>"
                    L"            </ListView>"
                    L"        </SplitView.Pane>"
                    L"        <ListView x:Name='ContentListView' HorizontalAlignment='Right' AllowDrop='True'>"
                    L"            <ListViewItem>Item 1</ListViewItem>"
                    L"        </ListView>"
                    L"    </SplitView>"
                    L"    <ListView x:Name='ExternalListView' AllowDrop='True'>"
                    L"        <ListViewItem>Item 1</ListViewItem>"
                    L"    </ListView>"
                    L"</StackPanel>"));

                splitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"SplitView"));
                itemToDrag = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"ItemToDrag"));
                targetListView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(targetListViewName));

                listViewItemDragStartingRegistration.Attach(itemToDrag, ref new wf::TypedEventHandler<xaml::UIElement^, xaml::DragStartingEventArgs^>(
                    [](xaml::UIElement^ sender, xaml::DragStartingEventArgs^ args)
                {
                    xaml_controls::ListViewItem^ listViewItem = safe_cast<xaml_controls::ListViewItem^>(sender);
                    args->Data->SetText(safe_cast<Platform::String^>(listViewItem->Content));
                    args->Data->RequestedOperation = DataPackageOperation::Copy;
                }));

                listViewDragOverRegistration.Attach(targetListView, ref new xaml::DragEventHandler(
                    [](Platform::Object^ sender, xaml::DragEventArgs^ args)
                {
                    args->AcceptedOperation = DataPackageOperation::Copy;
                }));

                listViewDropRegistration.Attach(targetListView, ref new xaml::DragEventHandler(
                    [itemDroppedEvent](Platform::Object^ sender, xaml::DragEventArgs^ args)
                {
                    xaml_controls::ListView^ listView = safe_cast<xaml_controls::ListView^>(sender);
                    xaml_controls::ListViewItem^ newListViewItem = ref new xaml_controls::ListViewItem();
                    Platform::String^ itemText = L"";

                    concurrency::create_task(args->DataView->GetTextAsync()).then(
                        [newListViewItem, listView, itemDroppedEvent](Platform::String^ text)
                    {
                        RunOnUIThread([newListViewItem, listView, itemDroppedEvent, text]()
                        {
                            newListViewItem->Content = text;
                            listView->Items->Append(newListViewItem);
                            itemDroppedEvent->Set();
                        });
                    });
                }));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->DragBetweenElements(itemToDrag, targetListView, 0.5 /*velocityFactor*/);

            itemDroppedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2u, targetListView->Items->Size);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"Item 2"), safe_cast<Platform::String^>(safe_cast<xaml_controls::ListViewItem^>(targetListView->Items->GetAt(1))->Content));
            });
        };

        verificationFunction(L"ContentListView");
        verificationFunction(L"ExternalListView");
    }

    void SplitViewIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                return BuildAllStatesTree();
            });
    }

    void SplitViewIntegrationTests::ValidateLightDismissBehavior()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Left, FlowDirection=LeftToRight");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::LeftToRight, true);

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Right, FlowDirection=LeftToRight");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::LeftToRight, true);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Left, FlowDirection=LeftToRight");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::LeftToRight, true);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Right, FlowDirection=LeftToRight");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::LeftToRight, true);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Left, FlowDirection=LeftToRight");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::LeftToRight, false);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Right, FlowDirection=LeftToRight");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::LeftToRight, false);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Left, FlowDirection=LeftToRight");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::LeftToRight, false);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Right, FlowDirection=LeftToRight");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::LeftToRight, false);

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Left, FlowDirection=RightToLeft");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::RightToLeft, true);

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Right, FlowDirection=RightToLeft");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::RightToLeft, true);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Left, FlowDirection=RightToLeft");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::RightToLeft, true);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Right, FlowDirection=RightToLeft");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::RightToLeft, true);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Left, FlowDirection=RightToLeft");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::RightToLeft, false);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Right, FlowDirection=RightToLeft");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::RightToLeft, false);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Left, FlowDirection=RightToLeft");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::RightToLeft, false);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Right, FlowDirection=RightToLeft");
        ValidateLightDismissBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::RightToLeft, false);
    }

    void SplitViewIntegrationTests::ValidateLightDismissBehaviorWorker(
        xaml_controls::SplitViewDisplayMode displayMode,
        xaml_controls::SplitViewPanePlacement placement,
        xaml::FlowDirection flowDirection,
        bool shouldLightDismiss
        )
    {
        xaml::FrameworkElement^ rootPanel = nullptr;
        xaml_controls::SplitView^ splitView = nullptr;
        xaml::FrameworkElement^ contentElement = nullptr;
        xaml::FrameworkElement^ outsideElement = nullptr;

        const bool expectedIsOpenValue = !shouldLightDismiss;
        bool isOpen = false;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <SplitView x:Name='splitView' Width='400' Height='400' OpenPaneLength='100'>"
                L"        <SplitView.Pane>"
                L"            <Border Background='Orange'>"
                L"                <Button/>"
                L"            </Border>"
                L"        </SplitView.Pane>"
                L"        <Rectangle x:Name='contentElement' Fill='Purple'/>"
                L"    </SplitView>"
                L"    <Border Width='400' Height='50' Background='GreenYellow'>"
                L"        <Button x:Name='outsideElement'/>"
                L"    </Border>"
                L"</StackPanel>"
                ));

            rootPanel->FlowDirection = flowDirection;

            splitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"splitView"));
            contentElement = safe_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"contentElement"));
            outsideElement = safe_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"outsideElement"));

            splitView->DisplayMode = displayMode;
            splitView->PanePlacement = placement;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // In cases where we expect the SlitView to light-dismiss, we expect SplitView to handle key presses such as Esc or Gamepad B.
        // If SplitView handles the keypress, the containing panel should NOT see the KeyDown event.
        // Conversely, in cases where SplitView should not light-dismiss, it should NOT handle the key event and so
        // the containing panel SHOULD see the KeyDown event.
        auto rootKeyDownEvent = std::make_shared<Event>();
        auto rootKeyDownRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, KeyDown);
        rootKeyDownRegistration.Attach(rootPanel, [&](){ rootKeyDownEvent->Set(); });

        // Test tapping outside of pane area, but within the splitview control.
        {
            RunOnUIThread([&]() { splitView->IsPaneOpen = true; });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(contentElement);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() { isOpen = splitView->IsPaneOpen; });
            VERIFY_ARE_EQUAL(isOpen, expectedIsOpenValue);
        }

        // Test tapping outside of splitview control.
        {
            RunOnUIThread([&]() { splitView->IsPaneOpen = true; });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(outsideElement);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() { isOpen = splitView->IsPaneOpen; });
            VERIFY_ARE_EQUAL(isOpen, expectedIsOpenValue);
        }

        // Test pressing the ESC key.
        {
            RunOnUIThread([&]() { splitView->IsPaneOpen = true; });
            TestServices::WindowHelper->WaitForIdle();

            rootKeyDownEvent->Reset();
            TestServices::KeyboardHelper->Escape();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), !shouldLightDismiss);

            RunOnUIThread([&]() { isOpen = splitView->IsPaneOpen; });
            VERIFY_ARE_EQUAL(isOpen, expectedIsOpenValue);
        }

        // Test pressing Gamepad B, which is internally mapped to ESC key.
        {
            RunOnUIThread([&]() { splitView->IsPaneOpen = true; });
            TestServices::WindowHelper->WaitForIdle();

            rootKeyDownEvent->Reset();
            TestServices::KeyboardHelper->GamepadB();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), !shouldLightDismiss);

            RunOnUIThread([&]() { isOpen = splitView->IsPaneOpen; });
            VERIFY_ARE_EQUAL(isOpen, expectedIsOpenValue);
        }

        // Change the size of the control.
        {
            RunOnUIThread([&]() { splitView->IsPaneOpen = true; });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                splitView->Width = splitView->ActualWidth * 0.80;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() { isOpen = splitView->IsPaneOpen; });
            VERIFY_ARE_EQUAL(isOpen, expectedIsOpenValue);
        }

        // Invoke back button
        {
            RunOnUIThread([&]() { splitView->IsPaneOpen = true; });
            TestServices::WindowHelper->WaitForIdle();

            bool backPressHandled = false;
            bool shouldBackPressGetHandled = shouldLightDismiss;
            TestServices::Utilities->InjectBackButtonPress(&backPressHandled);
            VERIFY_ARE_EQUAL(backPressHandled, shouldBackPressGetHandled);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() { isOpen = splitView->IsPaneOpen; });
            VERIFY_ARE_EQUAL(isOpen, expectedIsOpenValue);
        }

        // Resize application window (only for desktop)
        {
            RunOnUIThread([&]() { splitView->IsPaneOpen = true; });
            TestServices::WindowHelper->WaitForIdle();

            // TAEF window is always full-screen
            // Simulating app window re-size by setting size to 400x400 and then maximizing again
            TestServices::WindowHelper->SetDesktopWindowSize(400, 400);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->MaximizeDesktopWindow();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() { isOpen = splitView->IsPaneOpen; });
            VERIFY_ARE_EQUAL(isOpen, expectedIsOpenValue);
        }

        // TODO: Rotate device
    }

    void SplitViewIntegrationTests::CanSetCompactPaneLengthProperty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;

        const double splitViewWidth = 400.0;
        const double compactPaneLength = 85.0;
        const double expectedContentAreaWidth = splitViewWidth - compactPaneLength;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();

            splitView->DisplayMode = xaml_controls::SplitViewDisplayMode::CompactOverlay;
            splitView->Width = splitViewWidth;
            splitView->CompactPaneLength = compactPaneLength;

            splitView->Content = ref new xaml_shapes::Rectangle();

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(safe_cast<xaml::FrameworkElement^>(splitView->Content)->ActualWidth, expectedContentAreaWidth);
        });
    }

    void SplitViewIntegrationTests::VerifyKeyboardFocusBehavior()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Left");
        VerifyKeyboardFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Left);

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Right");
        VerifyKeyboardFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Right);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Left");
        VerifyKeyboardFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Left);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Right");
        VerifyKeyboardFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Right);
    }

    void SplitViewIntegrationTests::VerifyKeyboardFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode displayMode, xaml_controls::SplitViewPanePlacement placement)
    {
        xaml_controls::SplitView^ splitView = nullptr;
        xaml_controls::Button^ contentButton = nullptr;

        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, GotFocus);

        const double tabCount = 5;
        Platform::String^ expectedFocusSequence = L"[C][P1][P2][P3][P1][P2][P3][P2][P1][P3][P2][P1][C]";
        Platform::String^ focusSequence = "";

        RunOnUIThread([&]()
        {
            splitView = safe_cast<xaml_controls::SplitView^>(xaml_markup::XamlReader::Load(
                L"<SplitView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <SplitView.Pane>"
                L"        <StackPanel>"
                L"            <Button x:Name='paneButton1' Content='button' Tag='P1'/>"
                L"            <Button x:Name='paneButton2' Content='button' Tag='P2'/>"
                L"            <Button x:Name='paneButton3' Content='button' Tag='P3'/>"
                L"        </StackPanel>"
                L"    </SplitView.Pane>"
                L"    <Grid>"
                L"        <Button x:Name='contentButton' Content='button' Tag='C' HorizontalAlignment='Center' VerticalAlignment='Center'/>"
                L"    </Grid>"
                L"</SplitView>"
                ));

            splitView->DisplayMode = displayMode;
            splitView->PanePlacement = placement;

            gotFocusRegistration.Attach(splitView, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Tag + "]";
            }));

            contentButton = safe_cast<xaml_controls::Button^>(splitView->FindName(L"contentButton"));

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Make sure our focus sequence is clear before we start the actual test.
        focusSequence = "";

        // Start the focus on button in the content area.
        RunOnUIThread([&]()
        {
            contentButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Open the pane, which should grab focus.
        RunOnUIThread([&]()
        {
            splitView->IsPaneOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Tab to move focus through the control.
        for (size_t i = 0; i < tabCount; ++i)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        // Shift-Tab to move focus through the control in reverse.
        for (size_t i = 0; i < tabCount; ++i)
        {
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();
        }

        // Now close the pane.
        RunOnUIThread([&](){ splitView->IsPaneOpen = false; });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);
    }

    void SplitViewIntegrationTests::VerifyGamepadFocusBehavior()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Testing: (Gamepad) DisplayMode=Overlay, PanePlacement=Left");
        VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Left, InputDevice::Gamepad);

        LOG_OUTPUT(L"Testing: (Gamepad) DisplayMode=Overlay, PanePlacement=Right");
        VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Right, InputDevice::Gamepad);

        LOG_OUTPUT(L"Testing: (Gamepad) DisplayMode=CompactOverlay, PanePlacement=Left");
        VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Left, InputDevice::Gamepad);

        LOG_OUTPUT(L"Testing: (Gamepad) DisplayMode=CompactOverlay, PanePlacement=Right");
        VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Right, InputDevice::Gamepad);

        LOG_OUTPUT(L"Testing: (Gamepad) DisplayMode=Inline, PanePlacement=Left");
        VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Left, InputDevice::Gamepad);

        LOG_OUTPUT(L"Testing: (Gamepad) DisplayMode=Inline, PanePlacement=Right");
        VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Right, InputDevice::Gamepad);

        LOG_OUTPUT(L"Testing: (Gamepad) DisplayMode=CompactInline, PanePlacement=Left");
        VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Left, InputDevice::Gamepad);

        LOG_OUTPUT(L"Testing: (Gamepad) DisplayMode=CompactInline, PanePlacement=Right");
        VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Right, InputDevice::Gamepad);
    }

    void SplitViewIntegrationTests::VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode displayMode, xaml_controls::SplitViewPanePlacement placement, InputDevice device)
    {
        xaml_controls::SplitView^ splitView = nullptr;
        xaml_controls::Button^ contentButton = nullptr;

        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, GotFocus);

        auto paneClosedEvent = std::make_shared<Event>();
        auto paneClosedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneClosed);

        Platform::String^ expectedFocusSequence = nullptr;

        //Overlay Mode traps focus while inline mode doesn't
        switch (displayMode)
        {
            case(xaml_controls::SplitViewDisplayMode::Overlay):
            case(xaml_controls::SplitViewDisplayMode::CompactOverlay) :
            {
                expectedFocusSequence = L"[C][P1][P2][P3][P2][P1][P2][P3][C]";
                break;
            }
            case(xaml_controls::SplitViewDisplayMode::Inline) :
            {
                expectedFocusSequence = L"[C][P1][P2][P3][P2][P1][C][P1][P2][P3][C][P1][C]";
                break;
            }
            case(xaml_controls::SplitViewDisplayMode::CompactInline) :
            {
                expectedFocusSequence = L"[C][P1][P2][P3][P2][P1][C][P1][P2][P3][C][P1]";
                break;
            }
        }

        Platform::String^ focusSequence = "";

        RunOnUIThread([&]()
        {
            splitView = safe_cast<xaml_controls::SplitView^>(xaml_markup::XamlReader::Load(
                L"<SplitView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' OpenPaneLength='100'>"
                L"    <SplitView.Pane>"
                L"        <StackPanel>"
                L"            <Button x:Name='paneButton1' Content='button' Tag='P1'/>"
                L"            <Button x:Name='paneButton2' Content='button' Tag='P2'/>"
                L"            <Button x:Name='paneButton3' Content='button' Tag='P3'/>"
                L"        </StackPanel>"
                L"    </SplitView.Pane>"
                L"    <Grid>"
                L"        <Button x:Name='contentButton' Content='button' Tag='C' HorizontalAlignment='Center' VerticalAlignment='Top'/>"
                L"    </Grid>"
                L"</SplitView>"
            ));

            TestServices::WindowHelper->WindowContent = splitView;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            splitView->DisplayMode = displayMode;
            splitView->PanePlacement = placement;

            gotFocusRegistration.Attach(splitView, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                Platform::Object^ focusElementTag = safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Tag;
                LOG_OUTPUT(L"Got Focus: %s", (focusElementTag->ToString())->Data());
                focusSequence += "[" + focusElementTag + "]";
            }));

            paneClosedRegistration.Attach(splitView, [paneClosedEvent]
            {
                LOG_OUTPUT(L"SplitView Pane Closed");
                paneClosedEvent->Set();
            });

            contentButton = safe_cast<xaml_controls::Button^>(splitView->FindName(L"contentButton"));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Make sure our focus sequence is clear before we start the actual test.
        focusSequence = "";

        // Start the focus on button in the content area.
        RunOnUIThread([&]()
        {
            contentButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Open the pane, which should grab focus.
        RunOnUIThread([&]()
        {
            splitView->IsPaneOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        if (displayMode == xaml_controls::SplitViewDisplayMode::Inline ||
            displayMode == xaml_controls::SplitViewDisplayMode::CompactInline)
        {
            RunOnUIThread([&]()
            {
                xaml_controls::Button^ btn1 = safe_cast<xaml_controls::Button^>(splitView->FindName(L"paneButton1"));
                btn1->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        for (size_t i = 0; i < 2; i++)
        {
            CommonInputHelper::Down(device); //P1->P2->P3
            TestServices::WindowHelper->WaitForIdle();
        }

        for (size_t i = 0; i < 2; i++)
        {
            CommonInputHelper::Up(device); //P3->P2->P1
            TestServices::WindowHelper->WaitForIdle();
        }

        if (placement == xaml_controls::SplitViewPanePlacement::Left)
        {
            CommonInputHelper::Right(device); //P1->C
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Left(device); //C->P1
            TestServices::WindowHelper->WaitForIdle();
        }
        else
        {
            CommonInputHelper::Left(device); //P1->C
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Right(device); //C->P1
            TestServices::WindowHelper->WaitForIdle();
        }

        for (size_t i = 0; i < 2; i++)
        {
            CommonInputHelper::Down(device); //P1->P2->P3
            TestServices::WindowHelper->WaitForIdle();
        }

        if (placement == xaml_controls::SplitViewPanePlacement::Left)
        {
            CommonInputHelper::Right(device); //P3->C
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Left(device); //C->P3
            TestServices::WindowHelper->WaitForIdle();
        }
        else
        {
            CommonInputHelper::Left(device); //P1->C
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Right(device); //C->P1
            TestServices::WindowHelper->WaitForIdle();
        }
        // Now close the pane.
        if (displayMode == xaml_controls::SplitViewDisplayMode::Overlay ||
            displayMode == xaml_controls::SplitViewDisplayMode::CompactOverlay)
        {
            CommonInputHelper::Cancel(device);
            TestServices::WindowHelper->WaitForIdle();
        }
        else
        {
            RunOnUIThread([&]()
            {
                splitView->IsPaneOpen = false;
            });
            TestServices::WindowHelper->WaitForIdle();
        }
        paneClosedEvent->WaitForDefault();

        LOG_OUTPUT(L"FocusSequence %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);
    }

    void SplitViewIntegrationTests::ValidateElementResizeCountForTransitions()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Left");
        ValidateElementResizeCountForTransitionsWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Left, 0, 0, 1, 0);

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Right");
        ValidateElementResizeCountForTransitionsWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Right, 0, 0, 1, 0);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Left");
        ValidateElementResizeCountForTransitionsWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Left, 1, 1, 1, 0);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Right");
        ValidateElementResizeCountForTransitionsWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Right, 1, 1, 1, 0);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Left");
        ValidateElementResizeCountForTransitionsWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Left, 0, 0, 0, 0);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Right");
        ValidateElementResizeCountForTransitionsWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Right, 0, 0, 0, 0);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Left");
        ValidateElementResizeCountForTransitionsWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Left, 1, 1, 0, 0);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Right");
        ValidateElementResizeCountForTransitionsWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Right, 1, 1, 0, 0);
    }

    void SplitViewIntegrationTests::ValidateElementResizeCountForTransitionsWorker(
        xaml_controls::SplitViewDisplayMode displayMode,
        xaml_controls::SplitViewPanePlacement placement,
        size_t expectedContentAreaClosedToOpenCount,
        size_t expectedContentAreaOpenToClosedCount,
        size_t expectedPaneAreaClosedToOpenCount,
        size_t expectedPaneAreaOpenToClosedCount
        )
    {
        xaml_controls::SplitView^ splitView = nullptr;

        auto contentSizeChangedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, SizeChanged);
        auto paneSizeChangedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, SizeChanged);

        size_t contentAreaSizeChangedCount = 0;
        size_t paneAreaSizeChangedCount = 0;

        RunOnUIThread([&]()
        {
            splitView = safe_cast<xaml_controls::SplitView^>(xaml_markup::XamlReader::Load(
                L"<SplitView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <SplitView.Pane>"
                L"        <Rectangle x:Name='paneElement' Fill='Orange'/>"
                L"    </SplitView.Pane>"
                L"    <Rectangle x:Name='contentElement' Fill='Purple'/>"
                L"</SplitView>"
                ));

            splitView->DisplayMode = displayMode;
            splitView->PanePlacement = placement;

            contentSizeChangedRegistration.Attach(
                safe_cast<xaml::FrameworkElement^>(splitView->FindName(L"contentElement")),
                ref new xaml::SizeChangedEventHandler([&contentAreaSizeChangedCount](Platform::Object^, xaml::SizeChangedEventArgs^)
                {
                    ++contentAreaSizeChangedCount;
                }));

            paneSizeChangedRegistration.Attach(
                safe_cast<xaml::FrameworkElement^>(splitView->FindName(L"paneElement")),
                ref new xaml::SizeChangedEventHandler([&paneAreaSizeChangedCount](Platform::Object^, xaml::SizeChangedEventArgs^)
                {
                    ++paneAreaSizeChangedCount;
                }));

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Reset to account for initial layout.
        contentAreaSizeChangedCount = 0;
        paneAreaSizeChangedCount = 0;

        // Go from closed to opened.
        RunOnUIThread([&](){ splitView->IsPaneOpen = true; });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(contentAreaSizeChangedCount, expectedContentAreaClosedToOpenCount);
        VERIFY_ARE_EQUAL(paneAreaSizeChangedCount, expectedPaneAreaClosedToOpenCount);

        // Reset for next transition.
        contentAreaSizeChangedCount = 0;
        paneAreaSizeChangedCount = 0;

        // Go from opened to closed.
        RunOnUIThread([&](){ splitView->IsPaneOpen = false; });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(contentAreaSizeChangedCount, expectedContentAreaOpenToClosedCount);
        VERIFY_ARE_EQUAL(paneAreaSizeChangedCount, expectedPaneAreaOpenToClosedCount);
    }

    void SplitViewIntegrationTests::DoesFireEventsWhenOpeningOrClosingPane()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;
        Platform::String^ eventOrder = "";
        Platform::String^ expectedEventOrder = "[Opening][Opened][Closing][Closed]";

        auto openingRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneOpening);
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneOpened);

        auto closingRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneClosing);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneClosed);

        Event openingEvent;
        Event openedEvent;
        Event closingEvent;
        Event closedEvent;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();

            openingRegistration.Attach(splitView, [&]()
            {
                eventOrder += L"[Opening]";
                openingEvent.Set();
            });

            openedRegistration.Attach(splitView, [&]()
            {
                eventOrder += L"[Opened]";
                openedEvent.Set();
            });

            closingRegistration.Attach(splitView, [&]()
            {
                eventOrder += L"[Closing]";
                closingEvent.Set();
            });

            closedRegistration.Attach(splitView, [&]()
            {
                eventOrder += L"[Closed]";
                closedEvent.Set();
            });

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the pane and validate the Opening & Opened events fire.");
        RunOnUIThread([&]()
        {
            splitView->IsPaneOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        openingEvent.WaitForDefault();
        openedEvent.WaitForDefault();

        LOG_OUTPUT(L"Close the pane by using a size-change to trigger a light dismiss and validate that Closing & Closed events fire.");
        RunOnUIThread([&]()
        {
            splitView->Width = splitView->ActualWidth * 0.80;
        });
        TestServices::WindowHelper->WaitForIdle();
        closingEvent.WaitForDefault();
        closedEvent.WaitForDefault();

        VERIFY_ARE_EQUAL(expectedEventOrder, eventOrder);
    }

    void SplitViewIntegrationTests::CanCancelPaneClosing()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;

        auto closingRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneClosing);
        auto spClosingEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();
            splitView->IsPaneOpen = true;

            closingRegistration.Attach(
                splitView,
                ref new wf::TypedEventHandler<xaml_controls::SplitView^, xaml_controls::SplitViewPaneClosingEventArgs^>([spClosingEvent](xaml_controls::SplitView^, xaml_controls::SplitViewPaneClosingEventArgs^ args)
                {
                    args->Cancel = true;
                    spClosingEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Change the size of the control to trigger a light dismiss.
        RunOnUIThread([&]()
        {
            splitView->Width = splitView->ActualWidth * 0.80;
        });
        TestServices::WindowHelper->WaitForIdle();

        spClosingEvent->WaitForDefault();

        // Our closing handler should have canceled it.
        RunOnUIThread([&](){ VERIFY_IS_TRUE(splitView->IsPaneOpen); });
    }

    void SplitViewIntegrationTests::DoSplitViewThemeResourcesExist()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            auto openPaneLengthThemeResource = xaml::Application::Current->Resources->Lookup(L"SplitViewOpenPaneThemeLength");
            auto compactPaneLengthThemeResource = xaml::Application::Current->Resources->Lookup(L"SplitViewCompactPaneThemeLength");

            VERIFY_IS_NOT_NULL(openPaneLengthThemeResource);
            VERIFY_IS_NOT_NULL(compactPaneLengthThemeResource);

            VERIFY_ARE_EQUAL(safe_cast<double>(openPaneLengthThemeResource), 320.0);
            VERIFY_ARE_EQUAL(safe_cast<double>(compactPaneLengthThemeResource), 48.0);
        });
    }

    void SplitViewIntegrationTests::DoesFocusWorkWithHyperlink()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;
        xaml_docs::Hyperlink^ hyperlink = nullptr;
        xaml_controls::Button^ paneButton = nullptr;

        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneOpened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneClosed);

        Event openedEvent;
        Event closedEvent;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <SplitView x:Name='splitView' PaneBackground='Orange'>"
                L"        <SplitView.Pane>"
                L"            <Button x:Name='paneButton' Content='Some button'/>"
                L"        </SplitView.Pane>"
                L"        <Rectangle Fill='Purple'/>"
                L"    </SplitView>"
                L"    <TextBlock Width='100' Height='25' HorizontalAlignment='Center' VerticalAlignment='Center'>"
                L"        <Hyperlink x:Name='hyperlink'>Some Hyperlink!</Hyperlink>"
                L"    </TextBlock>"
                L"</Grid>"
                ));

            splitView = safe_cast<xaml_controls::SplitView^>(root->FindName(L"splitView"));
            hyperlink = safe_cast<xaml_docs::Hyperlink^>(root->FindName(L"hyperlink"));
            paneButton = safe_cast<xaml_controls::Button^>(root->FindName(L"paneButton"));

            openedRegistration.Attach(splitView, [&]() { openedEvent.Set(); });
            closedRegistration.Attach(splitView, [&]() { closedEvent.Set(); });

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            hyperlink->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the pane to steal focus from the hyperlink.");
        RunOnUIThread([&](){ splitView->IsPaneOpen = true; });
        openedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Focus should be on the hyperlink now.
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(paneButton));

            LOG_OUTPUT(L"Close the pane to restore focus back to the hyperlink.");
            splitView->IsPaneOpen = false;
        });
        closedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Focus should be back on the hyperlink now.
        RunOnUIThread([&](){ VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink)); });
    }

    void SplitViewIntegrationTests::CanFocusInlineSplitViewContentAreaAfterOpening()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;
        xaml::FrameworkElement^ contentElement = nullptr;
        xaml_controls::Button^ contentButton = nullptr;

        RunOnUIThread([&]()
        {
            splitView = safe_cast<xaml_controls::SplitView^>(xaml_markup::XamlReader::Load(
                L"<SplitView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' DisplayMode='Inline'>"
                L"    <SplitView.Pane>"
                L"        <Rectangle Fill='Orange'/>"
                L"    </SplitView.Pane>"
                L"    <Grid x:Name='ContentElement' Background='Purple'>"
                L"        <Button x:Name='ContentButton' VerticalAlignment='Top'/>"
                L"    </Grid>"
                L"</SplitView>"
                ));

            contentElement = safe_cast<xaml::FrameworkElement^>(splitView->FindName(L"ContentElement"));
            contentButton = safe_cast<xaml_controls::Button^>(splitView->FindName(L"ContentButton"));

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&](){ contentButton->Focus(xaml::FocusState::Keyboard); });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&](){ splitView->IsPaneOpen = true; });
        TestServices::WindowHelper->WaitForIdle();

        // Tap on the content element to move focus out of the pane.
        TestServices::InputHelper->Tap(contentElement);
        TestServices::WindowHelper->WaitForIdle();
    }

    void SplitViewIntegrationTests::CanSetOpenPaneLengthToAuto()
    {
        TestCleanupWrapper cleanup;

        xaml::FrameworkElement^ contentElement = nullptr;

        const double splitViewWidth = 500;
        const double paneElementWidth = 200;
        const double expectedContentElementWidth = splitViewWidth - paneElementWidth;

        RunOnUIThread([&]()
        {
            auto splitView = safe_cast<xaml_controls::SplitView^>(xaml_markup::XamlReader::Load(
                L"<SplitView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    IsPaneOpen='True' DisplayMode='Inline' OpenPaneLength='Auto'>"
                L"    <SplitView.Pane>"
                L"        <Rectangle x:Name='PaneElement' Fill='Orange'/>"
                L"    </SplitView.Pane>"
                L"    <Rectangle x:Name='ContentElement' Fill='Purple'/>"
                L"</SplitView>"
                ));

            auto paneElement = safe_cast<xaml::FrameworkElement^>(splitView->FindName(L"PaneElement"));
            contentElement = safe_cast<xaml::FrameworkElement^>(splitView->FindName(L"ContentElement"));

            splitView->Width = splitViewWidth;
            paneElement->Width = paneElementWidth;

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(contentElement->ActualWidth, expectedContentElementWidth);
        });
    }

    void SplitViewIntegrationTests::CanTouchSplitViewWithInfiniteWidth()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;

        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto spClickEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='{ThemeResource ApplicationPageBackgroundThemeBrush}'>"
                L"    <Grid.ColumnDefinitions>"
                L"        <ColumnDefinition Width='48'/>"
                L"        <ColumnDefinition Width='Auto'/>"
                L"    </Grid.ColumnDefinitions>"
                L"    <SplitView DisplayMode='CompactOverlay' Grid.ColumnSpan='2'>"
                L"        <SplitView.Pane>"
                L"            <Grid Background='Orange'>"
                L"                <Button x:Name='TapTarget' Content='button'/>"
                L"            </Grid>"
                L"        </SplitView.Pane>"
                L"    </SplitView>"
                L"</Grid>"
                ));

            button = safe_cast<xaml_controls::Button^>(root->FindName(L"TapTarget"));
            clickRegistration.Attach(button, ref new xaml::RoutedEventHandler([spClickEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    spClickEvent->Set();
                }));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);
        spClickEvent->WaitForDefault();
    }

    void SplitViewIntegrationTests::DoesFirePaneClosingEventWhenClosedProgrammatically()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;

        auto closingRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneClosing);
        auto spClosingEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();
            splitView->IsPaneOpen = true;

            closingRegistration.Attach(
                splitView,
                ref new wf::TypedEventHandler<xaml_controls::SplitView^, xaml_controls::SplitViewPaneClosingEventArgs^>([spClosingEvent](xaml_controls::SplitView^, xaml_controls::SplitViewPaneClosingEventArgs^)
            {
                spClosingEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Close the SplitView programmatically.
        RunOnUIThread([&]()
        {
            splitView->IsPaneOpen = false;
        });
        spClosingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void SplitViewIntegrationTests::CanInteractWithItemsInOpenPane()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Left, FlowDirection=LeftToRight");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::LeftToRight);

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Right, FlowDirection=LeftToRight");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::LeftToRight);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Left, FlowDirection=LeftToRight");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::LeftToRight);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Right, FlowDirection=LeftToRight");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::LeftToRight);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Left, FlowDirection=LeftToRight");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::LeftToRight);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Right, FlowDirection=LeftToRight");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::LeftToRight);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Left, FlowDirection=LeftToRight");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::LeftToRight);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Right, FlowDirection=LeftToRight");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::LeftToRight);

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Left, FlowDirection=RightToLeft");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::RightToLeft);

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay, PanePlacement=Right, FlowDirection=RightToLeft");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::Overlay, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::RightToLeft);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Left, FlowDirection=RightToLeft");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::RightToLeft);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay, PanePlacement=Right, FlowDirection=RightToLeft");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::RightToLeft);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Left, FlowDirection=RightToLeft");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::RightToLeft);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline, PanePlacement=Right, FlowDirection=RightToLeft");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::Inline, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::RightToLeft);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Left, FlowDirection=RightToLeft");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Left, xaml::FlowDirection::RightToLeft);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline, PanePlacement=Right, FlowDirection=RightToLeft");
        CanInteractWithItemsInOpenPaneWorker(xaml_controls::SplitViewDisplayMode::CompactInline, xaml_controls::SplitViewPanePlacement::Right, xaml::FlowDirection::RightToLeft);
    }

    void SplitViewIntegrationTests::CanInteractWithItemsInOpenPaneWorker(
        xaml_controls::SplitViewDisplayMode displayMode,
        xaml_controls::SplitViewPanePlacement placement,
        xaml::FlowDirection flowDirection
        )
    {
        xaml_controls::SplitView^ splitView = nullptr;
        xaml_controls::Button^ tapTarget = nullptr;

        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto clickEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <SplitView x:Name='splitView' Width='400' Height='400' OpenPaneLength='100'>"
                L"        <SplitView.Pane>"
                L"            <Border Background='Orange'>"
                L"                <Button x:Name='TapTarget'/>"
                L"            </Border>"
                L"        </SplitView.Pane>"
                L"        <Rectangle x:Name='contentElement' Fill='Purple'/>"
                L"    </SplitView>"
                L"</StackPanel>"
                ));

            root->FlowDirection = flowDirection;

            splitView = safe_cast<xaml_controls::SplitView^>(root->FindName(L"splitView"));
            tapTarget = safe_cast<xaml_controls::Button^>(root->FindName(L"TapTarget"));
            clickRegistration.Attach(tapTarget, ref new xaml::RoutedEventHandler([clickEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                clickEvent->Set();
            }));

            splitView->DisplayMode = displayMode;
            splitView->PanePlacement = placement;

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            splitView->IsPaneOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Validate that we can tap an item in the open pane.
        TestServices::InputHelper->Tap(tapTarget);
        clickEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void SplitViewIntegrationTests::CanSetOpenPaneLengthFromXamlString()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Control^ root = nullptr;
        xaml_controls::SplitView^ splitView = nullptr;

        RunOnUIThread([&]() {
            root = safe_cast<xaml_controls::Control^> (xaml_markup::XamlReader::Load(
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Border x:Name='MyBorder' Background='Pink'>"
                L"        <VisualStateManager.VisualStateGroups>"
                L"            <VisualStateGroup x:Name='Group1'>"
                L"                <VisualState x:Name='State1'>"
                L"                    <VisualState.Setters>"
                L"                        <Setter Target='splitView.OpenPaneLength' Value='200' />"
                L"                    </VisualState.Setters>"
                L"                </VisualState>"
                L"            </VisualStateGroup>"
                L"        </VisualStateManager.VisualStateGroups>"
                L"        <SplitView x:Name='splitView'"
                L"            PaneBackground='{ThemeResource SystemControlBackgroundAccentBrush}'"
                L"            DisplayMode='Inline'"
                L"            IsPaneOpen='True'"
                L"            OpenPaneLength='100' />"
                L"    </Border>"
                L"</UserControl>"
                ));
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]() {
            splitView = safe_cast<xaml_controls::SplitView^>(root->FindName(L"splitView"));
            VERIFY_ARE_EQUAL(100.0, splitView->OpenPaneLength);

            VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), true);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]() {
            VERIFY_ARE_EQUAL(200.0, splitView->OpenPaneLength);
        });
    }

    xaml_controls::Panel^ SplitViewIntegrationTests::BuildAllStatesTree()
    {
        xaml_controls::VariableSizedWrapGrid^ root = nullptr;

        RunOnUIThread([&]()
        {
            root = ref new xaml_controls::VariableSizedWrapGrid();
            root->Width = 400.0;
            root->Orientation = xaml_controls::Orientation::Horizontal;

            for (size_t useAutoOpenPaneLength = 0; useAutoOpenPaneLength < 2; ++useAutoOpenPaneLength)
            {
                for (auto placement = xaml_controls::SplitViewPanePlacement::Left; placement <= xaml_controls::SplitViewPanePlacement::Right; ++placement)
                {
                    for (size_t isPaneOpen = 0; isPaneOpen < 2; ++isPaneOpen)
                    {
                        for (auto displayMode = xaml_controls::SplitViewDisplayMode::Overlay; displayMode <= xaml_controls::SplitViewDisplayMode::CompactInline; ++displayMode)
                        {
                            auto splitView = safe_cast<xaml_controls::SplitView^>(xaml_markup::XamlReader::Load(
                                L"<SplitView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                                L"    Width='100' Height='75' OpenPaneLength='Auto' CompactPaneLength='5' LightDismissOverlayMode='Off'>"
                                L"    <SplitView.Pane>"
                                L"        <Rectangle x:Name='PaneElement'/>"
                                L"    </SplitView.Pane>"
                                L"</SplitView>"
                                ));

                            if (useAutoOpenPaneLength == 0)
                            {
                                splitView->OpenPaneLength = 20;
                            }
                            else
                            {
                                auto paneElement = safe_cast<xaml::FrameworkElement^>(splitView->FindName("PaneElement"));
                                paneElement->Width = 30;
                            }

                            splitView->PanePlacement = placement;
                            splitView->IsPaneOpen = (isPaneOpen > 0);
                            splitView->DisplayMode = displayMode;

                            root->Children->Append(splitView);
                        }
                    }
                }
            }

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        return root;
    }

    void SplitViewIntegrationTests::CanInteractWithPaneContentIfOpenedByDefault()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;
        xaml::FrameworkElement^ paneElement = nullptr;

        // The XamlRoot gets an initial size-changed notification when the app starts, which will close the SplitView
        // if the SplitView is added to the tree before that comes in.  We'll first add a grid as window content to
        // avoid that circumstance.
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            splitView = safe_cast<xaml_controls::SplitView^>(xaml_markup::XamlReader::Load(
                L"<SplitView IsPaneOpen='True' Width='300' Height='80' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <SplitView.Pane>"
                L"      <Rectangle x:Name='paneElement' Fill='Purple'/>"
                L"  </SplitView.Pane>"
                L"</SplitView>"
                ));

            paneElement = safe_cast<xaml::FrameworkElement^>(splitView->FindName(L"paneElement"));

            safe_cast<xaml_controls::Grid^>(TestServices::WindowHelper->WindowContent)->Children->Append(splitView);
        });
        TestServices::WindowHelper->WaitForIdle();

        bool isOpen = false;
        bool expectedIsOpenValue = true;

        // Test tapping an element inside the pane
        TestServices::InputHelper->Tap(paneElement);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]() { isOpen = splitView->IsPaneOpen; });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(isOpen, expectedIsOpenValue);
    }

    void SplitViewIntegrationTests::CanChangeOpenPaneLength()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;
        xaml_shapes::Rectangle^ refElement = nullptr;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();
            splitView->DisplayMode = xaml_controls::SplitViewDisplayMode::CompactInline;
            splitView->IsPaneOpen = true;

            refElement = ref new xaml_shapes::Rectangle();
            refElement->HorizontalAlignment = xaml::HorizontalAlignment::Left;

            splitView->Content = refElement;

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        double expectedXOffset = 100;
        RunOnUIThread([&]()
        {
            splitView->OpenPaneLength = expectedXOffset;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto offset = refElement->TransformToVisual(splitView)->TransformPoint(wf::Point(0, 0));
            VERIFY_ARE_EQUAL(offset.X, expectedXOffset);
        });
    }

    void SplitViewIntegrationTests::CanChangeCompactPaneLength()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;
        xaml_shapes::Rectangle^ refElement = nullptr;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();
            splitView->DisplayMode = xaml_controls::SplitViewDisplayMode::CompactInline;

            refElement = ref new xaml_shapes::Rectangle();
            refElement->HorizontalAlignment = xaml::HorizontalAlignment::Left;

            splitView->Content = refElement;

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        double expectedXOffset = 100;
        RunOnUIThread([&]()
        {
            splitView->CompactPaneLength = expectedXOffset;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto offset = refElement->TransformToVisual(splitView)->TransformPoint(wf::Point(0, 0));
            VERIFY_ARE_EQUAL(offset.X, expectedXOffset);
        });
    }

    void SplitViewIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedClosedOverlayContentWidth = 400;
        double const expectedClosedOverlayPaneWidth = 0;

        double const expectedOpenOverlayContentWidth = 400;
        double const expectedOpenOverlayPaneWidth = 319;

        double const expectedClosedCompactOverlayContentWidth = 400 - 48; // WindowWidth - CompactPaneLength
        double const expectedClosedCompactOverlayPaneWidth = 319;

        double const expectedOpenCompactOverlayContentWidth = 400 - 48; // WindowWidth - CompactPaneLength
        double const expectedOpenCompactOverlayPaneWidth = 319;

        double const expectedClosedInlineContentWidth = 400;
        double const expectedClosedInlinePaneWidth = 0;

        double const expectedOpenInlineContentWidth = 400 - 320; // WindowWidth - OpenPaneLength
        double const expectedOpenInlinePaneWidth = 319;

        double const expectedClosedCompactInlineContentWidth = 400 - 48; // WindowWidth - CompactPaneLength
        double const expectedClosedCompactInlinePaneWidth = 319;

        double const expectedOpenCompactInlineContentWidth = 400 - 320; // WindowWidth - OpenPaneLength
        double const expectedOpenCompactInlinePaneWidth = 319;

        xaml_controls::SplitView^ closedOverlaySplitView = nullptr;
        xaml_controls::SplitView^ openOverlaySplitView = nullptr;
        xaml_controls::SplitView^ closedCompactOverlaySplitView = nullptr;
        xaml_controls::SplitView^ openCompactOverlaySplitView = nullptr;

        xaml_controls::SplitView^ closedInlineSplitView = nullptr;
        xaml_controls::SplitView^ openInlineSplitView = nullptr;
        xaml_controls::SplitView^ closedCompactInlineSplitView = nullptr;
        xaml_controls::SplitView^ openCompactInlineSplitView = nullptr;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <SplitView x:Name="closedOverlaySplitView" Height="75" IsPaneOpen="False" DisplayMode="Overlay">
                            <SplitView.Pane>
                                <Rectangle/>
                            </SplitView.Pane>
                            <Rectangle/>
                        </SplitView>
                        <SplitView x:Name="openOverlaySplitView" Height="75" IsPaneOpen="True" DisplayMode="Overlay">
                            <SplitView.Pane>
                                <Rectangle/>
                            </SplitView.Pane>
                            <Rectangle/>
                        </SplitView>
                        <SplitView x:Name="closedCompactOverlaySplitView" Height="75" IsPaneOpen="False" DisplayMode="CompactOverlay">
                            <SplitView.Pane>
                                <Rectangle/>
                            </SplitView.Pane>
                            <Rectangle/>
                        </SplitView>
                        <SplitView x:Name="openCompactOverlaySplitView" Height="75" IsPaneOpen="True" DisplayMode="CompactOverlay">
                            <SplitView.Pane>
                                <Rectangle/>
                            </SplitView.Pane>
                            <Rectangle/>
                        </SplitView>
                        <SplitView x:Name="closedInlineSplitView" Height="75" IsPaneOpen="False" DisplayMode="Inline">
                            <SplitView.Pane>
                                <Rectangle/>
                            </SplitView.Pane>
                            <Rectangle/>
                        </SplitView>
                        <SplitView x:Name="openInlineSplitView" Height="75" IsPaneOpen="True" DisplayMode="Inline">
                            <SplitView.Pane>
                                <Rectangle/>
                            </SplitView.Pane>
                            <Rectangle/>
                        </SplitView>
                        <SplitView x:Name="closedCompactInlineSplitView" Height="75" IsPaneOpen="False" DisplayMode="CompactInline">
                            <SplitView.Pane>
                                <Rectangle/>
                            </SplitView.Pane>
                            <Rectangle/>
                        </SplitView>
                        <SplitView x:Name="openCompactInlineSplitView" Height="75" IsPaneOpen="True" DisplayMode="CompactInline">
                            <SplitView.Pane>
                                <Rectangle/>
                            </SplitView.Pane>
                            <Rectangle/>
                        </SplitView>
                    </StackPanel>)"));

            closedOverlaySplitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"closedOverlaySplitView"));
            openOverlaySplitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"openOverlaySplitView"));
            closedCompactOverlaySplitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"closedCompactOverlaySplitView"));
            openCompactOverlaySplitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"openCompactOverlaySplitView"));
            closedInlineSplitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"closedInlineSplitView"));
            openInlineSplitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"openInlineSplitView"));
            closedCompactInlineSplitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"closedCompactInlineSplitView"));
            openCompactInlineSplitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"openCompactInlineSplitView"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedClosedOverlayContentWidth, safe_cast<xaml::FrameworkElement^>(closedOverlaySplitView->Content)->ActualWidth);
            VERIFY_ARE_EQUAL(expectedClosedOverlayPaneWidth, safe_cast<xaml::FrameworkElement^>(closedOverlaySplitView->Pane)->ActualWidth);

            VERIFY_ARE_EQUAL(expectedOpenOverlayContentWidth, safe_cast<xaml::FrameworkElement^>(openOverlaySplitView->Content)->ActualWidth);
            VERIFY_ARE_EQUAL(expectedOpenOverlayPaneWidth, safe_cast<xaml::FrameworkElement^>(openOverlaySplitView->Pane)->ActualWidth);

            VERIFY_ARE_EQUAL(expectedClosedCompactOverlayContentWidth, safe_cast<xaml::FrameworkElement^>(closedCompactOverlaySplitView->Content)->ActualWidth);
            VERIFY_ARE_EQUAL(expectedClosedCompactOverlayPaneWidth, safe_cast<xaml::FrameworkElement^>(closedCompactOverlaySplitView->Pane)->ActualWidth);

            VERIFY_ARE_EQUAL(expectedOpenCompactOverlayContentWidth, safe_cast<xaml::FrameworkElement^>(openCompactOverlaySplitView->Content)->ActualWidth);
            VERIFY_ARE_EQUAL(expectedOpenCompactOverlayPaneWidth, safe_cast<xaml::FrameworkElement^>(openCompactOverlaySplitView->Pane)->ActualWidth);

            VERIFY_ARE_EQUAL(expectedClosedInlineContentWidth, safe_cast<xaml::FrameworkElement^>(closedInlineSplitView->Content)->ActualWidth);
            VERIFY_ARE_EQUAL(expectedClosedInlinePaneWidth, safe_cast<xaml::FrameworkElement^>(closedInlineSplitView->Pane)->ActualWidth);

            VERIFY_ARE_EQUAL(expectedOpenInlineContentWidth, safe_cast<xaml::FrameworkElement^>(openInlineSplitView->Content)->ActualWidth);
            VERIFY_ARE_EQUAL(expectedOpenInlinePaneWidth, safe_cast<xaml::FrameworkElement^>(openInlineSplitView->Pane)->ActualWidth);

            VERIFY_ARE_EQUAL(expectedClosedCompactInlineContentWidth, safe_cast<xaml::FrameworkElement^>(closedCompactInlineSplitView->Content)->ActualWidth);
            VERIFY_ARE_EQUAL(expectedClosedCompactInlinePaneWidth, safe_cast<xaml::FrameworkElement^>(closedCompactInlineSplitView->Pane)->ActualWidth);

            VERIFY_ARE_EQUAL(expectedOpenCompactInlineContentWidth, safe_cast<xaml::FrameworkElement^>(openCompactInlineSplitView->Content)->ActualWidth);
            VERIFY_ARE_EQUAL(expectedOpenCompactInlinePaneWidth, safe_cast<xaml::FrameworkElement^>(openCompactInlineSplitView->Pane)->ActualWidth);
        });
    }

    void SplitViewIntegrationTests::CanNotShiftTabOutOfPaneWhenContentsIsListView()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay");
        CanNotShiftTabOutOfPaneWhenContentsIsListViewWorker(xaml_controls::SplitViewDisplayMode::Overlay);

        LOG_OUTPUT(L"Testing: DisplayMode=ComapctOverlay");
        CanNotShiftTabOutOfPaneWhenContentsIsListViewWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay);
    }

    void SplitViewIntegrationTests::CanNotShiftTabOutOfPaneWhenContentsIsListViewWorker(xaml_controls::SplitViewDisplayMode displayMode)
    {
        xaml_controls::SplitView^ splitView = nullptr;
        xaml_controls::Button^ button = nullptr;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();
            splitView->DisplayMode = displayMode;

            auto listView = ref new xaml_controls::ListView();
            for (size_t i = 0; i < 3; ++i)
            {
                auto item = ref new xaml_controls::ListViewItem();
                item->Content = "Pane Item";
                listView->Items->Append(item);
            }

            splitView->Pane = listView;

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            splitView->IsPaneOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Focus should be on the first item in our pane's ListView so shift-tab
        // and verify that focus stays within the pane.
        TestServices::KeyboardHelper->ShiftTab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml::DependencyObject^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            bool isFocusInPane = TreeHelper::IsAncestorOf(splitView->Pane, focusedElement);
            VERIFY_IS_TRUE(isFocusInPane);
        });

        // Validate that forward tab keeps focus within the pane as well, just for
        // completeness because it's not specifically part of the repro scenario.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml::DependencyObject^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            bool isFocusInPane = TreeHelper::IsAncestorOf(splitView->Pane, focusedElement);
            VERIFY_IS_TRUE(isFocusInPane);
        });
    }

    void SplitViewIntegrationTests::ValidateLightDismissOverlayMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();
            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate that the default is Auto and the SplitView is in the 'OverlayNotVisible' state (or 'OverlayVisible' if on Xbox)");
        {
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(splitView->LightDismissOverlayMode, xaml_controls::LightDismissOverlayMode::Auto);
            });

            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(
                splitView,
                L"OverlayVisibilityStates",
                TestServices::Utilities->IsXBox ? L"OverlayVisible" : L"OverlayNotVisible")
                );
        }

        LOG_OUTPUT(L"Validate that when set to On the SplitView is in the 'OverlayVisible' state.");
        {
            RunOnUIThread([&]()
            {
                splitView->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
            });

            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(splitView, L"OverlayVisibilityStates", L"OverlayVisible"));
        }

        LOG_OUTPUT(L"Validate that when set to Off the SplitView is in the 'OverlayNotVisible' state.");
        {
            RunOnUIThread([&]()
            {
                splitView->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            });

            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(splitView, L"OverlayVisibilityStates", L"OverlayNotVisible"));
        }
    }

    void SplitViewIntegrationTests::IsAutoLightDismissOverlayModeVisibleOnXbox()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();
            splitView->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Auto;

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(splitView, L"OverlayVisibilityStates", L"OverlayVisible"));
    }

    void SplitViewIntegrationTests::ValidateOverlayBrush()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();
            splitView->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto expectedBrush = safe_cast<xaml_media::SolidColorBrush^>(xaml::Application::Current->Resources->Lookup(L"SplitViewLightDismissOverlayBackground"));

            auto overlayElement = TreeHelper::GetVisualChildByName(splitView, L"LightDismissLayer");
            THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist.");

            auto overlayRect = dynamic_cast<xaml_shapes::Rectangle^>(overlayElement);
            THROW_IF_NULL_WITH_MSG(overlayRect, L"The overlay element should be a rectangle.");

            auto overlayBrush = safe_cast<xaml_media::SolidColorBrush^>(overlayRect->Fill);
            VERIFY_IS_NOT_NULL(overlayBrush);
            VERIFY_IS_TRUE(overlayBrush->Equals(expectedBrush));
        });
    }

    void SplitViewIntegrationTests::ValidateOverlayUIETree()
    {
        TestCleanupWrapper cleanup;

        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                xaml_controls::StackPanel^ root = nullptr;
                RunOnUIThread([&]()
                {
                    root = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                        LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                                Background="{ThemeResource SystemControlBackgroundAltHighBrush}" >
                               <SplitView DisplayMode="Overlay" OpenPaneLength="50" IsPaneOpen="True" LightDismissOverlayMode="On" Height="100" Margin="0,0,0,10"/>
                               <SplitView DisplayMode="CompactOverlay" OpenPaneLength="50" IsPaneOpen="True" LightDismissOverlayMode="On" Height="100" Margin="0,0,0,10"/>
                               <SplitView DisplayMode="Inline" OpenPaneLength="50" IsPaneOpen="True" LightDismissOverlayMode="On" Height="100" Margin="0,0,0,10"/>
                               <SplitView DisplayMode="CompactInline" OpenPaneLength="50" IsPaneOpen="True" LightDismissOverlayMode="On" Height="100"/>
                            </StackPanel>)"));

                    TestServices::WindowHelper->WindowContent = root;
                });
                TestServices::WindowHelper->WaitForIdle();

                return root;
            }
        );
    }

    void SplitViewIntegrationTests::OpenSplitViewWithNoElementsFocused()
    {
        TestCleanupWrapper cleanup;

        // This is regression coverage for:
        // [Activation Reliability] Failure in STOWED_EXCEPTION_XAML_TEXT_The_parameter_is_incorrect_80070057_SearchUI.exe!Unknown.
        // The bug was hit with a SplitView was opened when nothing had focus. When it opens, SplitView tries to give its Pane the same FocusState as the
        // currently focused item. If there is no currently focused item, it was inadventently calling Focus with FocusState::Unfocused which causes problems.

        xaml_controls::SplitView^ splitView;
        xaml_controls::Button^ button;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <SplitView x:Name="splitView" IsTabStop="False" >
                            <SplitView.Pane>
                                <StackPanel>
                                    <Button x:Name="button" Content="Button" />
                                </StackPanel>
                            </SplitView.Pane>
                        </SplitView>
                    </Grid>)"));

            splitView = safe_cast<xaml_controls::SplitView^>(rootPanel->FindName(L"splitView"));
            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot) == nullptr);
            splitView->IsPaneOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button));
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, button->FocusState);
        });
    }

    void SplitViewIntegrationTests::DoesNotFireOpenedOrClosedEventOnDisplayModeChange()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();
            splitView->DisplayMode = xaml_controls::SplitViewDisplayMode::Overlay;
            splitView->IsPaneOpen = true;

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneOpened);
        Event openedEvent;

        openedRegistration.Attach(splitView, [&]() { openedEvent.Set(); });

        LOG_OUTPUT(L"Change the SplitView's display mode while open and validate the the PaneOpened event is not fired.");
        RunOnUIThread([&]()
        {
            splitView->DisplayMode = xaml_controls::SplitViewDisplayMode::CompactOverlay;
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(openedEvent.HasFired());

        LOG_OUTPUT(L"Close the pane and verify that the PaneClosed event does not re-fire with display mode changes.");
        RunOnUIThread([&]()
        {
            splitView->IsPaneOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneClosed);
        Event closedEvent;

        closedRegistration.Attach(splitView, [&]() { closedEvent.Set(); });

        LOG_OUTPUT(L"Change the SplitView's display mode while closed and validate the the PaneOpened event is not fired.");
        RunOnUIThread([&]()
        {
            splitView->DisplayMode = xaml_controls::SplitViewDisplayMode::Overlay;
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(closedEvent.HasFired());
    }

    void SplitViewIntegrationTests::DoesFireOpenedAndClosedEventsWhenRetemplated()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SplitView^ splitView = nullptr;

        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneOpened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, PaneClosed);

        Event openedEvent;
        Event closedEvent;

        RunOnUIThread([&]()
        {
            splitView = ref new xaml_controls::SplitView();

            splitView->Template = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                LR"(<ControlTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                        TargetType="SplitView">
                        <Grid Background="{TemplateBinding Background}"/>
                    </ControlTemplate>)"));

            openedRegistration.Attach(splitView, [&]() { openedEvent.Set(); });
            closedRegistration.Attach(splitView, [&]() { closedEvent.Set(); });

            TestServices::WindowHelper->WindowContent = splitView;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Set IsPaneOpen=true and validate that the PaneOpened event fires.");
        RunOnUIThread([&]()
        {
            splitView->IsPaneOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        openedEvent.WaitForDefault();

        LOG_OUTPUT(L"Set IsPaneOpen=false and validate that the PaneOpened event fires.");
        RunOnUIThread([&]()
        {
            splitView->IsPaneOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();
        closedEvent.WaitForDefault();
    }

    Platform::String^ SplitViewIntegrationTests::GetResourcesPath() const
    {
        return GetPackageFolder() + L"resources\\native\\controls\\splitview\\";
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Button
