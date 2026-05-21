// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "FlipViewIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <Collection.h>

#include <ControlHelper.h>
#include <TreeHelper.h>
#include <SelectorHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace FlipView {

    bool FlipViewIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool FlipViewIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool FlipViewIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void FlipViewIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::FlipView>::CanInstantiate();
    }

    void FlipViewIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::FlipView>::CanEnterAndLeaveLiveTree();
    }

    xaml_controls::FlipView^ FlipViewIntegrationTests::CreateFlipView(xaml_controls::Orientation orientation, unsigned int itemsCount, bool useNonVirtualizingStackPanel)
    {
        xaml_controls::FlipView^ flipView = nullptr;

        Platform::String^ strOrientation;
        if (orientation == xaml_controls::Orientation::Horizontal)
        {
            strOrientation = L"Horizontal";
        }
        else
        {
            strOrientation = L"Vertical";
        }

        Platform::String^ itemsPanelType = useNonVirtualizingStackPanel ? L"StackPanel" : L"VirtualizingStackPanel";

        RunOnUIThread([&]()
        {
            flipView = dynamic_cast<xaml_controls::FlipView^>(xaml_markup::XamlReader::Load(
                L"<FlipView x:Name='flipView' Height='100' Width='150' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <FlipView.ItemsPanel>"
                L"      <ItemsPanelTemplate>"
                L"          <"+ itemsPanelType + " AreScrollSnapPointsRegular='True' Orientation='" + strOrientation + L"' />"
                L"      </ItemsPanelTemplate>"
                L"  </FlipView.ItemsPanel>"
                L"</FlipView>"));

            auto items = ref new Platform::Collections::Vector<Platform::String^>();
            for (unsigned int i = 0; i < itemsCount; i++)
            {
                items->Append(L"Item " + i);
            }

            flipView->ItemsSource = items;
        });

        return flipView;
    }

    xaml_controls::FlipView^ FlipViewIntegrationTests::SetupBasicFlipView(xaml_controls::Orientation orientation, unsigned int itemsCount)
    {
        xaml_controls::FlipView^ flipView = nullptr;

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, Loaded);
        auto spHasLoadedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            flipView = CreateFlipView(orientation, itemsCount);

            loadedRegistration.Attach(flipView, ref new xaml::RoutedEventHandler([spHasLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                spHasLoadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = flipView;
        });
        TestServices::WindowHelper->WaitForIdle();
        spHasLoadedEvent->WaitForDefault();

        return flipView;
    }

    void FlipViewIntegrationTests::CanFlipWithMouse()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 5);
        xaml::FrameworkElement^ rightButton = nullptr;
        xaml::FrameworkElement^ leftButton = nullptr;

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        selectionChangedRegistration.Attach(flipView,
            ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
            selectionChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            auto selectedIndex = static_cast<int>(flipView->SelectedIndex);
            VERIFY_ARE_EQUAL(0, selectedIndex);

            //Click on right arrow to change selection
            rightButton = TreeHelper::GetVisualChildByName(flipView, L"NextButtonHorizontal");
            VERIFY_IS_NOT_NULL(rightButton);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Dragging the mouse over FlipView to show the Previous / Next buttons");
        TestServices::InputHelper->DragFromCenter(flipView, 0 /*relX*/, 10 /*relY*/, 0.1 /*velocityFactor*/);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Clicking on right arrow button to change selection");
        TestServices::InputHelper->LeftMouseClick(rightButton);

        LOG_OUTPUT(L"Waiting for selection changed event from mouse click");
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));

            //Click on left arrow to change selection
            leftButton = TreeHelper::GetVisualChildByName(flipView, L"PreviousButtonHorizontal");
            VERIFY_IS_NOT_NULL(leftButton);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Dragging the mouse over FlipView to show the Previous / Next buttons");
        TestServices::InputHelper->DragFromCenter(flipView, 0 /*relX*/, 10 /*relY*/, 0.1 /*velocityFactor*/);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Clicking on left arrow button to change selection");
        TestServices::InputHelper->LeftMouseClick(leftButton);

        LOG_OUTPUT(L"Waiting for selection changed event from mouse click");
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, static_cast<int>(flipView->SelectedIndex));
        });
    }

    void FlipViewIntegrationTests::CanFlipWithTouch()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 5);
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        RunOnUIThread([&]()
        {
            // Changing the FlipView width so that a single Flick(),
            // delta of 150 pixels, moves to next item.
            flipView->Width = 200;
        });

        selectionChangedRegistration.Attach(flipView, ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
            selectionChangedEvent->Set();
        }));

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Flick(flipView, FlickDirection::West);

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));
        });

        TestServices::InputHelper->Flick(flipView, FlickDirection::East);

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, static_cast<int>(flipView->SelectedIndex));
        });
    }

    void FlipViewIntegrationTests::CanAddManiplateAndRemoveItemsInSuccession()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::FlipView^ flipView = nullptr;

        auto buttonClickEvent = std::make_shared<Event>();
        auto buttonClickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Button x:Name='button' Content='Press Me'/>"
                L"    <FlipView x:Name='flipView'/>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            flipView = safe_cast<xaml_controls::FlipView^>(rootPanel->FindName(L"flipView"));
            VERIFY_IS_NOT_NULL(flipView);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            buttonClickRegistration.Attach(button, ref new xaml::RoutedEventHandler(
                [flipView]
            (Platform::Object^ sender, xaml::IRoutedEventArgs^ args)
            {
                static bool click = false;

                if (click)
                {
                    while (flipView->Items->Size)
                    {
                        flipView->Items->RemoveAt(flipView->Items->Size - 1);
                        while (static_cast<unsigned int>(flipView->SelectedIndex) < flipView->Items->Size - 1)
                            flipView->SelectedIndex++;
                    }
                }
                else
                {
                    flipView->Items->Append(ref new Platform::String(L"this"));
                    flipView->Items->Append(ref new Platform::String(L"is"));
                    flipView->Items->Append(ref new Platform::String(L"awesome"));
                }

                click = !click;
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);

        // Crash after the second tap since scrolling to the next item is not done yet
        // and we delete the item
        TestServices::InputHelper->Tap(button);

        // No crash
    }

    void FlipViewIntegrationTests::CanFlipWithGamepad()
    {
        TestCleanupWrapper cleanup;

        InputDevice device = InputDevice::Gamepad;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Vertical, 5);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        selectionChangedRegistration.Attach(flipView,
            ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
            selectionChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            // Focus the FlipView to ensure it gets the key press.
            flipView->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, static_cast<int>(flipView->SelectedIndex));
        });

        LOG_OUTPUT(L"Pressing down to change selection on vertical FlipView");
        CommonInputHelper::Down(device);

        LOG_OUTPUT(L"Waiting for selection changed event");
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));
        });

        selectionChangedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing right to test if we consume the key");
        CommonInputHelper::Right(device);

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));
        });
    }

    void FlipViewIntegrationTests::CanFlipWithKeyboard()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Vertical, 5);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        selectionChangedRegistration.Attach(flipView,
            ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
            selectionChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            // Focus the FlipView to ensure it gets the key press.
            flipView->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing 'Down' key to change selection on vertical FlipView");
        TestServices::KeyboardHelper->Down();

        LOG_OUTPUT(L"Waiting for selection changed event from key press");
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));
        });

        LOG_OUTPUT(L"Pressing 'Up' key to change selection on vertical FlipView");
        TestServices::KeyboardHelper->Up();

        LOG_OUTPUT(L"Waiting for selection changed event from key press");
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, static_cast<int>(flipView->SelectedIndex));
        });

        LOG_OUTPUT(L"Pressing 'PageDown' key to change selection on vertical FlipView");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");

        LOG_OUTPUT(L"Waiting for selection changed event from key press");
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));
        });

        LOG_OUTPUT(L"Pressing 'PageUp' key to change selection on vertical FlipView");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");

        LOG_OUTPUT(L"Waiting for selection changed event from key press");
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, static_cast<int>(flipView->SelectedIndex));
        });
    }

    void FlipViewIntegrationTests::CanVirtualizeAndRealize()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::FlipView^ flipView = SetupBasicFlipView(xaml_controls::Orientation::Vertical, 50);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        selectionChangedRegistration.Attach(flipView,
            ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
            selectionChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            auto unrealizedContainer = flipView->ItemContainerGenerator->ContainerFromIndex(30);
            VERIFY_IS_NULL(unrealizedContainer);

            flipView->SelectedIndex = 30;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for selection changed event from programmatic selection");
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            auto realizedContainer = flipView->ItemContainerGenerator->ContainerFromIndex(30);
            VERIFY_IS_NOT_NULL(realizedContainer);

            auto virtualizedContainer = flipView->ItemContainerGenerator->ContainerFromIndex(0);
            VERIFY_IS_NULL(virtualizedContainer);
        });
    }

    void FlipViewIntegrationTests::ValidateScrollableWithMouseWheel()
    {
        ValidateScrollableWithMouseWheelWithOptionalAnimation(true /*animate*/);
    }

    void FlipViewIntegrationTests::ValidateScrollableWithMouseWheelWithoutAnimation()
    {
        ValidateScrollableWithMouseWheelWithOptionalAnimation(false /*animate*/);
    }

    void FlipViewIntegrationTests::ValidateScrollableWithMouseWheelWithOptionalAnimation(bool animate)
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureDisableGlobalAnimations(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations, !animate);

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::FlipView^ flipView = nullptr;
        xaml_controls::ScrollViewer^ scrollingHost = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, GotFocus);
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);
        auto scrollViewerViewChangedEvent = std::make_shared<Event>();
        auto scrollViewerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ScrollViewer x:Name='scrollViewer' HorizontalScrollBarVisibility='Visible'>"
                L"    <StackPanel HorizontalAlignment='Left' VerticalAlignment='Top' Orientation='Horizontal'>"
                L"      <Button x:Name='button1' Content='Button1' Width='200' Height='300' />"
                L"      <FlipView x:Name='flipView' Width='800' Height='500' Background='Aqua'>"
                L"          <FlipViewItem>Item 0</FlipViewItem>"
                L"          <FlipViewItem>Item 1</FlipViewItem>"
                L"      </FlipView>"
                L"      <Button x:Name='button2' Content='Button2' Width='200' Height='300' />"
                L"    </StackPanel>"
                L"  </ScrollViewer>"
                L"</Grid>"));

            loadedRegistration.Attach(rootPanel, [loadedEvent](){ loadedEvent->Set();});
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flipView = safe_cast<xaml_controls::FlipView^>(rootPanel->FindName(L"flipView"));
            scrollingHost = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(flipView, L"ScrollingHost"));

            gotFocusRegistration.Attach(
                flipView,
                ref new xaml::RoutedEventHandler(
                [gotFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"ValidateScrollEmptyFlipViewWithMouse: GotFocus event fired on FlipViewer!");
                gotFocusEvent->Set();
            }));

            selectionChangedRegistration.Attach(
                flipView,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e)
            {
                LOG_OUTPUT(L"ValidateScrollEmptyFlipViewWithMouse: SelectionChanged!");
                selectionChangedEvent->Set();
            }));

            scrollViewerViewChangedRegistration.Attach(scrollingHost,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [scrollViewerViewChangedEvent, scrollingHost](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollingHost->HorizontalOffset, scrollingHost->VerticalOffset, scrollingHost->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    scrollViewerViewChangedEvent->Set();
                }
            }));
        });

        // Click the FlipView and ensure the GotFocus.
        test_infra::TestServices::InputHelper->LeftMouseClick(flipView);
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Scroll the mouse wheel to change the selection item of FlipView.
        TestServices::InputHelper->ScrollMouseWheel(flipView, -1 /* numberOfWheelClicks */);
        LOG_OUTPUT(L"Waiting for SelectionChanged event");
        selectionChangedEvent->WaitForDefault();
        LOG_OUTPUT(L"Waiting for final ViewChanged event");
        scrollViewerViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));
            selectionChangedEvent->Reset();
            scrollViewerViewChangedEvent->Reset();
        });

        TestServices::InputHelper->ScrollMouseWheel(flipView, 1 /* numberOfWheelClicks */);
        LOG_OUTPUT(L"Waiting for SelectionChanged event");
        selectionChangedEvent->WaitForDefault();
        LOG_OUTPUT(L"Waiting for final ViewChanged event");
        scrollViewerViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, static_cast<int>(flipView->SelectedIndex));
        });
    }

    void FlipViewIntegrationTests::ValidateScrollingWithMouseWheelQuicklyIsIgnored()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 5);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        selectionChangedRegistration.Attach(flipView,
            ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
            selectionChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            // Focus the FlipView to ensure it gets the mouse wheel input.
            flipView->Focus(xaml::FocusState::Pointer);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Scroll the mouse wheel to change the selection item of FlipView.
        TestServices::InputHelper->ScrollMouseWheel(flipView, -1 /* numberOfWheelClicks */);
        TestServices::InputHelper->ScrollMouseWheel(flipView, -1 /* numberOfWheelClicks */);
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));
        });
    }

    void FlipViewIntegrationTests::ValidateEmptyFlipViewDoesNotCrashWithMouseWheel()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::FlipView^ flipView = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, GotFocus);
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ScrollViewer x:Name='scrollViewer' HorizontalScrollBarVisibility='Visible'>"
                L"    <StackPanel HorizontalAlignment='Left' VerticalAlignment='Top' Orientation='Horizontal'>"
                L"      <Button x:Name='button1' Content='Button1' Width='200' Height='300' />"
                L"      <FlipView x:Name='flipView' Width='800' Height='500' Background='Aqua' />"
                L"      <Button x:Name='button2' Content='Button2' Width='200' Height='300' />"
                L"    </StackPanel>"
                L"  </ScrollViewer>"
                L"</Grid>"));

            loadedRegistration.Attach(rootPanel, [loadedEvent](){ loadedEvent->Set();});
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flipView = safe_cast<xaml_controls::FlipView^>(rootPanel->FindName(L"flipView"));
        });

        test_infra::TestServices::InputHelper->LeftMouseClick(flipView);
        TestServices::InputHelper->ScrollMouseWheel(flipView, 1 /* numberOfWheelClicks */);
        TestServices::WindowHelper->WaitForIdle();
    }

    // Validates that the FlipView SelectedIndex property is restored when the FlipView is put back into the tree and DManip is aware of the offset.
    void FlipViewIntegrationTests::CanRestoreSelection()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 5);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        RunOnUIThread([&]()
        {
            // Changing the FlipView width so that a single Flick(),
            // delta of 150 pixels, moves to next item.
            flipView->Width = 200;
        });

        selectionChangedRegistration.Attach(flipView,
            ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
            selectionChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting selection index to 2.");
            flipView->SelectedIndex = 2;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for selection changed event.");
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Taking FlipView out of the tree.");
            TestServices::WindowHelper->WindowContent = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Putting back FlipView into the tree.");
            TestServices::WindowHelper->WindowContent = flipView;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, static_cast<int>(flipView->SelectedIndex));
            selectionChangedEvent->Reset();
        });

        LOG_OUTPUT(L"Flicking to move to selection index 3.");
        TestServices::InputHelper->Flick(flipView, FlickDirection::West);

        LOG_OUTPUT(L"Waiting for selection changed event.");
        selectionChangedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(3, static_cast<int>(flipView->SelectedIndex));
        });
    }

    void FlipViewIntegrationTests::ValidateCollapsedButtonState()
    {
        TestCleanupWrapper cleanup;

        xaml::FrameworkElement^ leftButton = nullptr;
        xaml::FrameworkElement^ rightButton = nullptr;
        xaml_controls::FlipView^ flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 1);

        // Click the mouse to the FlipView and then move the mouse to show the Previous/Next Buttons
        test_infra::TestServices::InputHelper->LeftMouseClick(flipView);
        test_infra::TestServices::InputHelper->MoveMouse(flipView);

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            leftButton = TreeHelper::GetVisualChildByName(flipView, L"PreviousButtonHorizontal");
            VERIFY_IS_NOT_NULL(leftButton);
            VERIFY_IS_TRUE(leftButton->Visibility == xaml::Visibility::Collapsed);

            rightButton = TreeHelper::GetVisualChildByName(flipView, L"NextButtonHorizontal");
            VERIFY_IS_NOT_NULL(rightButton);
            VERIFY_IS_TRUE(rightButton->Visibility == xaml::Visibility::Collapsed);
        });
    }

    void FlipViewIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(900, 400),
            1.f,
            []()
            {
                xaml_controls::StackPanel^ rootPanel = nullptr;

                xaml_controls::FlipView^ restHorizontalFlipView = CreateFlipView(xaml_controls::Orientation::Horizontal, 3);
                xaml_controls::FlipView^ hoverHorizontalFlipView = CreateFlipView(xaml_controls::Orientation::Horizontal, 3);
                xaml_controls::FlipView^ pressHorizontalFlipView = CreateFlipView(xaml_controls::Orientation::Horizontal, 3);

                xaml_controls::Button^ restHorizontalFlipViewPreviousButton = nullptr;
                xaml_controls::Button^ restHorizontalFlipViewNextButton = nullptr;
                xaml_controls::Button^ hoverHorizontalFlipViewPreviousButton = nullptr;
                xaml_controls::Button^ hoverHorizontalFlipViewNextButton = nullptr;
                xaml_controls::Button^ pressHorizontalFlipViewPreviousButton = nullptr;
                xaml_controls::Button^ pressHorizontalFlipViewNextButton = nullptr;

                xaml_controls::FlipView^ restVerticalFlipView = CreateFlipView(xaml_controls::Orientation::Vertical, 3);
                xaml_controls::FlipView^ hoverVerticalFlipView = CreateFlipView(xaml_controls::Orientation::Vertical, 3);
                xaml_controls::FlipView^ pressVerticalFlipView = CreateFlipView(xaml_controls::Orientation::Vertical, 3);

                xaml_controls::Button^ restVerticalFlipViewPreviousButton = nullptr;
                xaml_controls::Button^ restVerticalFlipViewNextButton = nullptr;
                xaml_controls::Button^ hoverVerticalFlipViewPreviousButton = nullptr;
                xaml_controls::Button^ hoverVerticalFlipViewNextButton = nullptr;
                xaml_controls::Button^ pressVerticalFlipViewPreviousButton = nullptr;
                xaml_controls::Button^ pressVerticalFlipViewNextButton = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();
                    rootPanel->Orientation = xaml_controls::Orientation::Horizontal;

                    rootPanel->Children->Append(restHorizontalFlipView);
                    rootPanel->Children->Append(hoverHorizontalFlipView);
                    rootPanel->Children->Append(pressHorizontalFlipView);
                    rootPanel->Children->Append(restVerticalFlipView);
                    rootPanel->Children->Append(hoverVerticalFlipView);
                    rootPanel->Children->Append(pressVerticalFlipView);

                    restHorizontalFlipView->SelectedIndex = 1;
                    hoverHorizontalFlipView->SelectedIndex = 1;
                    pressHorizontalFlipView->SelectedIndex = 1;

                    restVerticalFlipView->SelectedIndex = 1;
                    hoverVerticalFlipView->SelectedIndex = 1;
                    pressVerticalFlipView->SelectedIndex = 1;

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    restHorizontalFlipViewPreviousButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(restHorizontalFlipView, "PreviousButtonHorizontal"));
                    VERIFY_IS_NOT_NULL(restHorizontalFlipViewPreviousButton);
                    restHorizontalFlipViewPreviousButton->Visibility = xaml::Visibility::Visible;

                    restHorizontalFlipViewNextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(restHorizontalFlipView, "NextButtonHorizontal"));
                    VERIFY_IS_NOT_NULL(restHorizontalFlipViewNextButton);
                    restHorizontalFlipViewNextButton->Visibility = xaml::Visibility::Visible;

                    hoverHorizontalFlipViewPreviousButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hoverHorizontalFlipView, "PreviousButtonHorizontal"));
                    VERIFY_IS_NOT_NULL(hoverHorizontalFlipViewPreviousButton);
                    hoverHorizontalFlipViewPreviousButton->Visibility = xaml::Visibility::Visible;

                    hoverHorizontalFlipViewNextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hoverHorizontalFlipView, "NextButtonHorizontal"));
                    VERIFY_IS_NOT_NULL(hoverHorizontalFlipViewNextButton);
                    hoverHorizontalFlipViewNextButton->Visibility = xaml::Visibility::Visible;

                    pressHorizontalFlipViewPreviousButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(pressHorizontalFlipView, "PreviousButtonHorizontal"));
                    VERIFY_IS_NOT_NULL(pressHorizontalFlipViewPreviousButton);
                    pressHorizontalFlipViewPreviousButton->Visibility = xaml::Visibility::Visible;

                    pressHorizontalFlipViewNextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(pressHorizontalFlipView, "NextButtonHorizontal"));
                    VERIFY_IS_NOT_NULL(pressHorizontalFlipViewNextButton);
                    pressHorizontalFlipViewNextButton->Visibility = xaml::Visibility::Visible;

                    restVerticalFlipViewPreviousButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(restVerticalFlipView, "PreviousButtonVertical"));
                    VERIFY_IS_NOT_NULL(restVerticalFlipViewPreviousButton);
                    restVerticalFlipViewPreviousButton->Visibility = xaml::Visibility::Visible;

                    restVerticalFlipViewNextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(restVerticalFlipView, "NextButtonVertical"));
                    VERIFY_IS_NOT_NULL(restVerticalFlipViewNextButton);
                    restVerticalFlipViewNextButton->Visibility = xaml::Visibility::Visible;

                    hoverVerticalFlipViewPreviousButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hoverVerticalFlipView, "PreviousButtonVertical"));
                    VERIFY_IS_NOT_NULL(hoverVerticalFlipViewPreviousButton);
                    hoverVerticalFlipViewPreviousButton->Visibility = xaml::Visibility::Visible;

                    hoverVerticalFlipViewNextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hoverVerticalFlipView, "NextButtonVertical"));
                    VERIFY_IS_NOT_NULL(hoverVerticalFlipViewNextButton);
                    hoverVerticalFlipViewNextButton->Visibility = xaml::Visibility::Visible;

                    pressVerticalFlipViewPreviousButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(pressVerticalFlipView, "PreviousButtonVertical"));
                    VERIFY_IS_NOT_NULL(pressVerticalFlipViewPreviousButton);
                    pressVerticalFlipViewPreviousButton->Visibility = xaml::Visibility::Visible;

                    pressVerticalFlipViewNextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(pressVerticalFlipView, "NextButtonVertical"));
                    VERIFY_IS_NOT_NULL(pressVerticalFlipViewNextButton);
                    pressVerticalFlipViewNextButton->Visibility = xaml::Visibility::Visible;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(restHorizontalFlipViewPreviousButton, "Normal", false);
                    VisualStateManager::GoToState(restHorizontalFlipViewNextButton, "Normal", false);
                    VisualStateManager::GoToState(hoverHorizontalFlipViewPreviousButton, "PointerOver", false);
                    VisualStateManager::GoToState(hoverHorizontalFlipViewNextButton, "PointerOver", false);
                    VisualStateManager::GoToState(pressHorizontalFlipViewPreviousButton, "Pressed", false);
                    VisualStateManager::GoToState(pressHorizontalFlipViewNextButton, "Pressed", false);

                    VisualStateManager::GoToState(restVerticalFlipViewPreviousButton, "Normal", false);
                    VisualStateManager::GoToState(restVerticalFlipViewNextButton, "Normal", false);
                    VisualStateManager::GoToState(hoverVerticalFlipViewPreviousButton, "PointerOver", false);
                    VisualStateManager::GoToState(hoverVerticalFlipViewNextButton, "PointerOver", false);
                    VisualStateManager::GoToState(pressVerticalFlipViewPreviousButton, "Pressed", false);
                    VisualStateManager::GoToState(pressVerticalFlipViewNextButton, "Pressed", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            }
        );
    }

    void FlipViewIntegrationTests::ValidateOneItem()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 1);

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto items = ref new Platform::Collections::Vector<Platform::String^>();
            items->Append(L"New Item!");

            flipView->ItemsSource = items;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FlipViewIntegrationTests::ValidateHidingFlipViewHidesNavigationButtons()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 5);
        xaml::FrameworkElement^ rightButton = nullptr;
        xaml::FrameworkElement^ leftButton = nullptr;

        RunOnUIThread([&]()
        {
            leftButton = TreeHelper::GetVisualChildByName(flipView, L"PreviousButtonHorizontal");
            rightButton = TreeHelper::GetVisualChildByName(flipView, L"NextButtonHorizontal");

            leftButton->Visibility = xaml::Visibility::Visible;
            rightButton->Visibility = xaml::Visibility::Visible;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flipView->Visibility = xaml::Visibility::Collapsed;

            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, leftButton->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, rightButton->Visibility);
        });
    }

    void FlipViewIntegrationTests::ValidateDisablingFlipViewHidesNavigationButtons()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 5);
        xaml::FrameworkElement^ rightButton = nullptr;
        xaml::FrameworkElement^ leftButton = nullptr;

        RunOnUIThread([&]()
        {
            leftButton = TreeHelper::GetVisualChildByName(flipView, L"PreviousButtonHorizontal");
            rightButton = TreeHelper::GetVisualChildByName(flipView, L"NextButtonHorizontal");

            leftButton->Visibility = xaml::Visibility::Visible;
            rightButton->Visibility = xaml::Visibility::Visible;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flipView->IsEnabled = false;
        });

        // The event raised to indicate that IsEnabled changed is an asynchronous event,
        // so we need to wait for idle before checking for its effects.
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, leftButton->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, rightButton->Visibility);
        });
    }

    void FlipViewIntegrationTests::CannotScrollPastEdges()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 2);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(
                flipView,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e)
            {
                selectionChangedEvent->Set();
            }));
        });

        // Scroll up and make sure that we don't change the selection.
        TestServices::InputHelper->ScrollMouseWheel(flipView, 1 /* numberOfWheelClicks */);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, static_cast<int>(flipView->SelectedIndex));
        });

        // Now scroll down to go to the second item.
        TestServices::InputHelper->ScrollMouseWheel(flipView, -1 /* numberOfWheelClicks */);
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));
        });

        // Scroll down and make sure that we don't change the selection.
        TestServices::InputHelper->ScrollMouseWheel(flipView, -1 /* numberOfWheelClicks */);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, static_cast<int>(flipView->SelectedIndex));
        });
    }

    void FlipViewIntegrationTests::ValidateResizeFlipView()
    {
        TestCleanupWrapper cleanup;
        DoValidateResizeFlipView(false /*useNonVirtualizingStackPanel*/);
    }

    void FlipViewIntegrationTests::ValidateResizeFlipViewWithStackPanelItemsPanel()
    {
        TestCleanupWrapper cleanup;
        DoValidateResizeFlipView(true /*useNonVirtualizingStackPanel*/);
    }

    void FlipViewIntegrationTests::DoValidateResizeFlipView(bool useNonVirtualizingStackPanel)
    {
        xaml_controls::FlipView^ flipView;
        int indexToSelect = 2;

        RunOnUIThread([&]()
        {
            flipView = CreateFlipView(xaml_controls::Orientation::Horizontal, 5, useNonVirtualizingStackPanel);
            flipView->Width = 100;
            flipView->Height = 100;
            flipView->HorizontalAlignment = xaml::HorizontalAlignment::Left;

            flipView->SelectedIndex = indexToSelect;

            auto rootpanel = ref new xaml_controls::Grid();
            rootpanel->Children->Append(flipView);
            TestServices::WindowHelper->WindowContent = rootpanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flipView->Width = 400;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(indexToSelect, flipView->SelectedIndex);
        });
    }

    void FlipViewIntegrationTests::ChangeSelectionWhileSuspended()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->SetIsSuspended(false);
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        int viewChangedCount = 0;
        xaml_controls::FlipView^ flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 5);
        xaml_controls::ScrollViewer^ scrollingHost;

        RunOnUIThread([&]()
        {
            scrollingHost = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(flipView, L"ScrollingHost"));
        });

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        selectionChangedRegistration.Attach(flipView,
            ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^, xaml_controls::SelectionChangedEventArgs^)
        {
            LOG_OUTPUT(L"SelectionChanged raised.");
            selectionChangedEvent->Set();
        }));

        auto scrollViewerViewChangedEvent = std::make_shared<Event>();
        auto scrollViewerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        scrollViewerViewChangedRegistration.Attach(scrollingHost,
            ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [&viewChangedCount, scrollViewerViewChangedEvent, scrollingHost](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
        {
            LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                scrollingHost->HorizontalOffset, scrollingHost->VerticalOffset, scrollingHost->ZoomFactor, args->IsIntermediate);
            viewChangedCount++;
            if (args->IsIntermediate == false)
            {
                scrollViewerViewChangedEvent->Set();
            }
        }));

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Changing FlipView.SelectedIndex to 1.");
            flipView->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        scrollViewerViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        selectionChangedEvent->Reset();
        scrollViewerViewChangedEvent->Reset();

        RunOnUIThread([&]()
        {
            double expectedHorizontalOffset = 3.0;
            LOG_OUTPUT(L"scrollingHost->HorizontalOffset, expected: %f, actual: %f", expectedHorizontalOffset, scrollingHost->HorizontalOffset);
            VERIFY_IS_TRUE(AreClose(expectedHorizontalOffset, scrollingHost->HorizontalOffset));
            LOG_OUTPUT(L"Expecting more than 2 ViewChanged occurrences for animated item changes.");
            VERIFY_IS_TRUE(viewChangedCount > 2);
            viewChangedCount = 0;

            LOG_OUTPUT(L"Changing FlipView.SelectedIndex to 2.");
            flipView->SelectedIndex = 2;
        });

        selectionChangedEvent->WaitForDefault();
        scrollViewerViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        selectionChangedEvent->Reset();
        scrollViewerViewChangedEvent->Reset();

        RunOnUIThread([&]()
        {
            double expectedHorizontalOffset = 4.0;
            LOG_OUTPUT(L"scrollingHost->HorizontalOffset, expected: %f, actual: %f", expectedHorizontalOffset, scrollingHost->HorizontalOffset);
            VERIFY_IS_TRUE(AreClose(expectedHorizontalOffset, scrollingHost->HorizontalOffset));
            LOG_OUTPUT(L"Expecting more than 2 ViewChanged occurrences for animated item changes.");
            VERIFY_IS_TRUE(viewChangedCount > 2);
            viewChangedCount = 0;

            TestServices::WindowHelper->SetIsSuspended(true);

            LOG_OUTPUT(L"Changing FlipView.SelectedIndex to 1.");
            flipView->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        scrollViewerViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        selectionChangedEvent->Reset();
        scrollViewerViewChangedEvent->Reset();

        RunOnUIThread([&]()
        {
            double expectedHorizontalOffset = 3.0;
            LOG_OUTPUT(L"scrollingHost->HorizontalOffset, expected: %f, actual: %f", expectedHorizontalOffset, scrollingHost->HorizontalOffset);
            VERIFY_IS_TRUE(AreClose(expectedHorizontalOffset, scrollingHost->HorizontalOffset));
            LOG_OUTPUT(L"Expecting 2 ViewChanged occurrences for non-animated item changes.");
            VERIFY_ARE_EQUAL(viewChangedCount, 2);
            viewChangedCount = 0;

            LOG_OUTPUT(L"Changing FlipView.SelectedIndex to 2.");
            flipView->SelectedIndex = 0;
        });

        selectionChangedEvent->WaitForDefault();
        scrollViewerViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        selectionChangedEvent->Reset();
        scrollViewerViewChangedEvent->Reset();

        RunOnUIThread([&]()
        {
            double expectedHorizontalOffset = 2.0;
            LOG_OUTPUT(L"scrollingHost->HorizontalOffset, expected: %f, actual: %f", expectedHorizontalOffset, scrollingHost->HorizontalOffset);
            VERIFY_IS_TRUE(AreClose(expectedHorizontalOffset, scrollingHost->HorizontalOffset));
            LOG_OUTPUT(L"Expecting 2 ViewChanged occurrences for non-animated item changes.");
            VERIFY_ARE_EQUAL(viewChangedCount, 2);
        });
    }

    void FlipViewIntegrationTests::ValidateResizeFlipViewWhileChangingItem()
    {
        TestCleanupWrapper cleanup;

        // Regression coverage: "Facebook for Win10 home page tabs getting out of alignment"
        // The problem in this bug is due to:
        //   1. When changing the selected item, the item gets animated into position by the ScrollViewer
        //   2. When the FlipView is resized, it adjusts its ScrollViewer's offset to keep the selected item in view.
        // The bug occurs when both of these happen at the same time: the FlipView gets resized while it is changing item.
        //
        // We test this fix by resizing the FlipView in its SelectionChanged event handler.
        // We verify that the ScrollViewer ends up at the correct offset.

        double flipViewInitialHeight = 400;
        double flipViewNewHeight = 300;
        xaml_controls::FlipView^ flipView;
        xaml_controls::ScrollViewer^ scrollingHost;

        RunOnUIThread([&]()
        {
            flipView = CreateFlipView(xaml_controls::Orientation::Horizontal, 3);
            flipView->VerticalAlignment = xaml::VerticalAlignment::Top;
            flipView->Height = flipViewInitialHeight;
            flipView->Width = 400;

            xaml_controls::Grid^ rootpanel = ref new xaml_controls::Grid();
            rootpanel->Children->Append(flipView);

            TestServices::WindowHelper->WindowContent = rootpanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollingHost = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(flipView, L"ScrollingHost"));
        });

        auto scrollViewerViewChangedEvent = std::make_shared<Event>();
        auto scrollViewerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        scrollViewerViewChangedRegistration.Attach(scrollingHost,
            ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
            [&](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
        {
            if (args->IsIntermediate == false)
            {
                scrollViewerViewChangedEvent->Set();
            }
        }));

        // We resize the FlipView in the SelectionChanged event handler:
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);
        selectionChangedRegistration.Attach(flipView, [&]()
        {
            flipView->Height = flipViewNewHeight;
            selectionChangedEvent->Set();
        });

        RunOnUIThread([&]()
        {
            flipView->SelectedIndex = 1;
        });
        selectionChangedEvent->WaitForDefault();
        scrollViewerViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // The FlipView's ScrollViewer uses item-based scrolling. So the offset is not measured in pixels. Instead it is measured in proportion
            // to the item size (1.0 is the "width" of a single item). We expect the offset for item 1 to be 3.0 (see ItemsPresenter::IndexToOffset).
            double expectedHorizontalOffset = 3;
            LOG_OUTPUT(L"scrollingHost->HorizontalOffset, expected: %f, actual: %f", expectedHorizontalOffset, scrollingHost->HorizontalOffset);
            VERIFY_IS_TRUE(AreClose(expectedHorizontalOffset, scrollingHost->HorizontalOffset));
        });
    }

    void LogFocusedElement()
    {
        RunOnUIThread([&]()
        {
            auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);

            if (auto fe = dynamic_cast<xaml::FrameworkElement^>(focusedElement))
            {
                LOG_OUTPUT(L"Focused element: %s %s", fe->ToString()->Data(), fe->Name->Data());
            }
            else
            {
                LOG_OUTPUT(L"Focused element is null or not a FrameworkElement");
            }
        });
    }

    void FlipViewIntegrationTests::VerifyInitialSelectedIndex()
    {
        TestCleanupWrapper cleanup;

        // Verify that the SelectedIndex that is set at creation time is maintained when
        // FlipView finishes loading.

        int initialSelectedIndex = 2;
        xaml_controls::FlipView^ flipView;
        xaml_controls::FlipViewItem^ flipViewItemToSelect;

        Event flipViewItemGotFocus;
        auto flipViewItemGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::FlipViewItem, GotFocus);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Page^>(xaml_markup::XamlReader::Load(
                LR"(<Page xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <FlipView x:Name="flipView">
                            <FlipViewItem x:Name="flipViewItem0" >Item 0</FlipViewItem>
                            <FlipViewItem x:Name="flipViewItem1" >Item 1</FlipViewItem>
                            <FlipViewItem x:Name="flipViewItem2" >Item 2</FlipViewItem>
                            <FlipViewItem x:Name="flipViewItem3" >Item 3</FlipViewItem>
                            <FlipViewItem x:Name="flipViewItem4" >Item 4</FlipViewItem>
                        </FlipView>
                </Page>)"));

            flipView = safe_cast<xaml_controls::FlipView^>(rootPanel->FindName(L"flipView"));
            flipViewItemToSelect = safe_cast<xaml_controls::FlipViewItem^>(rootPanel->FindName(L"flipViewItem2"));
            flipView->SelectedIndex = initialSelectedIndex;

            flipViewItemGotFocusRegistration.Attach(flipViewItemToSelect, [&flipViewItemGotFocus]() {flipViewItemGotFocus.Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        {
            // Log the focused element, even if the wait fails.
            auto scopeExit = wil::scope_exit([&]()
            {
                LogFocusedElement();
            });
            LOG_OUTPUT(L"Wait for flipViewItem2 to get focus.");
            flipViewItemGotFocus.WaitForDefault();
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(initialSelectedIndex, flipView->SelectedIndex);
            VERIFY_ARE_EQUAL(flipView->Items->GetAt(initialSelectedIndex), flipView->SelectedItem);

            LogFocusedElement();
            auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_ARE_EQUAL(flipView->SelectedItem, focusedElement);
        });
    }

    void FlipViewIntegrationTests::VerifySelectionChangeViaAutomationPeer()
    {
        TestCleanupWrapper cleanup;

        // Verify that the SelectedIndex property gets updated correctly if a FlipViewItem is
        // scrolled into view via its AutomationPeer.

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Vertical, 5);
        TestServices::WindowHelper->WaitForIdle();

        int itemToSelectIndex = 2;

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);
        selectionChangedRegistration.Attach(flipView, [&](){ selectionChangedEvent->Set(); });

        RunOnUIThread([&]()
        {
            auto flipViewPeer = ref new xaml_automation_peers::FlipViewAutomationPeer(flipView);
            auto itemPeer = ref new xaml_automation_peers::FlipViewItemDataAutomationPeer(flipView->Items->GetAt(itemToSelectIndex), flipViewPeer);
            itemPeer->ScrollIntoView();
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(itemToSelectIndex, flipView->SelectedIndex);

            auto expectedSelectedItem = flipView->Items->GetAt(itemToSelectIndex);
            VERIFY_IS_TRUE(expectedSelectedItem->Equals(flipView->SelectedItem));
        });
    }

    void FlipViewIntegrationTests::ValidatePagingKeyInteractionVertical()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Vertical, 5);

        RunOnUIThread([&]()
        {
            // Focus the FlipView to ensure it gets the key press.
            flipView->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 0);

        // These should navigate the FlipView:
        LOG_OUTPUT(L"Changing selection with PageDown");
        TestServices::KeyboardHelper->PageDown();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 1);

        LOG_OUTPUT(L"Changing selection with PageDown");
        TestServices::KeyboardHelper->PageDown();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 2);

        LOG_OUTPUT(L"Changing selection with PageUp");
        TestServices::KeyboardHelper->PageUp();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 1);

        LOG_OUTPUT(L"Changing selection with PageUp");
        TestServices::KeyboardHelper->PageUp();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 0);

        // Gamepad:
        // These should behave the same as the page keys.
        LOG_OUTPUT(L"Changing selection with GamepadRightTrigger");
        TestServices::KeyboardHelper->GamepadRightTrigger();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 1);

        LOG_OUTPUT(L"Changing selection with GamepadRightTrigger");
        TestServices::KeyboardHelper->GamepadRightTrigger();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 2);

        LOG_OUTPUT(L"Changing selection with GamepadLeftTrigger");
        TestServices::KeyboardHelper->GamepadLeftTrigger();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 1);

        LOG_OUTPUT(L"Changing selection with GamepadLeftTrigger");
        TestServices::KeyboardHelper->GamepadLeftTrigger();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 0);
    }

    void FlipViewIntegrationTests::ValidatePagingKeyInteractionHorizontal()
    {
        TestCleanupWrapper cleanup;

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 5);

        RunOnUIThread([&]()
        {
            // Focus the FlipView to ensure it gets the key press.
            flipView->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 0);

        // These should navigate the FlipView (even though it's Horizontal):
        LOG_OUTPUT(L"Changing selection with PageDown");
        TestServices::KeyboardHelper->PageDown();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 1);

        LOG_OUTPUT(L"Changing selection with PageDown");
        TestServices::KeyboardHelper->PageDown();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 2);

        LOG_OUTPUT(L"Changing selection with PageUp");
        TestServices::KeyboardHelper->PageUp();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 1);

        LOG_OUTPUT(L"Changing selection with PageUp");
        TestServices::KeyboardHelper->PageUp();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 0);

        // Gamepad:
        // These should behave the same as the page keys.
        LOG_OUTPUT(L"Changing selection with GamepadRightShoulder");
        TestServices::KeyboardHelper->GamepadRightShoulder();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 1);

        LOG_OUTPUT(L"Changing selection with GamepadRightShoulder");
        TestServices::KeyboardHelper->GamepadRightShoulder();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 2);

        LOG_OUTPUT(L"Changing selection with GamepadLeftShoulder");
        TestServices::KeyboardHelper->GamepadLeftShoulder();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 1);

        LOG_OUTPUT(L"Changing selection with GamepadLeftShoulder");
        TestServices::KeyboardHelper->GamepadLeftShoulder();
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 0);
    }

    void FlipViewIntegrationTests::VerifyGamepadKeyDownRoutedOrHandledVertical()
    {
        VerifyGamepadKeyDownRoutedOrHandledRunner(xaml_controls::Orientation::Vertical);
    }

    void FlipViewIntegrationTests::VerifyGamepadKeyDownRoutedOrHandledHorizontal()
    {
        VerifyGamepadKeyDownRoutedOrHandledRunner(xaml_controls::Orientation::Horizontal);
    }

    void FlipViewIntegrationTests::VerifyGamepadKeyDownRoutedOrHandledRunner(xaml_controls::Orientation orientation)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootGrid = nullptr;
        xaml_controls::FlipView^ flipView = nullptr;

        RunOnUIThread([&]()
        {
            rootGrid = ref new xaml_controls::Grid();
            flipView = CreateFlipView(orientation, 5);

            rootGrid->Children->Append(flipView);

            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto rootKeyDownEvent = std::make_shared<Event>();
        auto rootKeyDownRegistration = CreateSafeEventRegistration(xaml_controls::Grid, KeyDown);
        rootKeyDownRegistration.Attach(rootGrid, [&](){ rootKeyDownEvent->Set(); });

        RunOnUIThread([&]()
        {
            // Focus the FlipView to ensure it gets the key press.
            flipView->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();
        SelectorHelper::VerifySelectedIndex(flipView, 0);

        bool isVertical = orientation == xaml_controls::Orientation::Vertical;

        // If vertical, then Right/Left/Shoulders should not be handled by the FlipView, and should bubble up to the root.
        // We can test if they bubbled up by seeing if the root received a KeyDown event.
        // If horizontal, then Right/Left/Shoulders should be handled by the FlipView so the root KeyDown event should not fire.
        {
            LOG_OUTPUT(L"Pressing Gamepad Right");
            rootKeyDownEvent->Reset();
            CommonInputHelper::Right(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), isVertical);
        }

        {
            LOG_OUTPUT(L"Pressing Gamepad Left");
            rootKeyDownEvent->Reset();
            CommonInputHelper::Left(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), isVertical);
        }

        {
            LOG_OUTPUT(L"Pressing Gamepad Right Shoulder");
            rootKeyDownEvent->Reset();
            TestServices::KeyboardHelper->GamepadRightShoulder();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), isVertical);
        }

        {
            LOG_OUTPUT(L"Pressing Gamepad Left Shoulder");
            rootKeyDownEvent->Reset();
            TestServices::KeyboardHelper->GamepadLeftShoulder();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), isVertical);
        }

        // If vertical, then Up/Down/Triggers should be handled by the FlipView, so the root KeyDown event should not fire.
        // If horizontal, then these should not be handled by the FlipView, so we expect the root to receive a KeyDown event.
        {
            LOG_OUTPUT(L"Pressing Gamepad Down");
            rootKeyDownEvent->Reset();
            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), !isVertical);
        }

        {
            LOG_OUTPUT(L"Pressing Gamepad Up");
            rootKeyDownEvent->Reset();
            CommonInputHelper::Up(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), !isVertical);
        }

        {
            LOG_OUTPUT(L"Pressing Gamepad Right Trigger");
            rootKeyDownEvent->Reset();
            TestServices::KeyboardHelper->GamepadRightTrigger();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), !isVertical);
        }

        {
            LOG_OUTPUT(L"Pressing Gamepad Left Trigger");
            rootKeyDownEvent->Reset();
            TestServices::KeyboardHelper->GamepadLeftTrigger();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(rootKeyDownEvent->HasFired(), !isVertical);
        }
    }

    void FlipViewIntegrationTests::CanFillAutoSuggestBox()
    {
        auto inputPaneShowingEvent = std::make_shared<Event>();
        wf::EventRegistrationToken inputPaneShowingToken;

        auto inputPaneHidingEvent = std::make_shared<Event>();
        wf::EventRegistrationToken inputPaneHidingToken;

        wuv::InputPane^ inputPane = nullptr;

        TestCleanupWrapper cleanup([&]()
        {
            RunOnUIThread([&]()
            {
                inputPane->Showing -= inputPaneShowingToken;
                inputPane->Hiding -= inputPaneHidingToken;

                inputPaneShowingToken = {};
                inputPaneHidingToken = {};

                inputPane = nullptr;
            });

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::FlipView^ flipView = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        xaml_controls::ScrollViewer^ scrollViewerInFlipView = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        bool inputPaneShown = false;

        RunOnUIThread([&]()
        {
            inputPane = test_infra::TestServices::WindowHelper->GetInputPaneForMainView();

            inputPaneShowingToken = inputPane->Showing += ref new wf::TypedEventHandler<wuv::InputPane^, wuv::InputPaneVisibilityEventArgs^>([&](wuv::InputPane^ pane, wuv::InputPaneVisibilityEventArgs^ e)
            {
                LOG_OUTPUT(L"InputPane is showing");
                inputPaneShowingEvent->Set();
            });

            inputPaneHidingToken = inputPane->Hiding += ref new wf::TypedEventHandler<wuv::InputPane^, wuv::InputPaneVisibilityEventArgs^>([&](wuv::InputPane^ pane, wuv::InputPaneVisibilityEventArgs^ e)
            {
                LOG_OUTPUT(L"InputPane is hiding");
                inputPaneHidingEvent->Set();
            });

            rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' Background='DarkRed' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L" <FlipView x:Name='flipView'>"
                L"  <FlipView.ItemTemplate>"
                L"   <DataTemplate>"
                L"    <Grid Width='200' Height='600' Background='DarkBlue' VerticalAlignment='Center'>"
                L"     <Grid.RowDefinitions>"
                L"      <RowDefinition Height='Auto'/>"
                L"      <RowDefinition Height='*'/>"
                L"     </Grid.RowDefinitions>"
                L"     <Button x:Name='button' Content='{Binding}'/>"
                L"     <AutoSuggestBox x:Name='autoSuggestBox' Grid.Row='1' VerticalAlignment='Bottom'/>"
                L"    </Grid>"
                L"   </DataTemplate>"
                L"  </FlipView.ItemTemplate>"
                L" </FlipView>"
                L"</Grid>"));

            loadedRegistration.Attach(
                rootPanel,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        LOG_OUTPUT(L"Waiting for root loaded.");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flipView = safe_cast<xaml_controls::FlipView^>(rootPanel->FindName(L"flipView"));
            VERIFY_IS_NOT_NULL(flipView);

            scrollViewerInFlipView = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(flipView, L"ScrollingHost"));
            VERIFY_IS_NOT_NULL(scrollViewerInFlipView);

            viewChangedRegistration.Attach(
                scrollViewerInFlipView,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [scrollViewerInFlipView, viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewerInFlipView->HorizontalOffset, scrollViewerInFlipView->VerticalOffset, scrollViewerInFlipView->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            auto items = ref new Platform::Collections::Vector<Platform::String^>();
            for (unsigned int i = 0; i < 3; i++)
            {
                items->Append(L"Item - Index " + i);
            }
            flipView->ItemsSource = items;

            selectionChangedRegistration.Attach(
                flipView,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^, xaml_controls::SelectionChangedEventArgs^)
            {
                selectionChangedEvent->Set();
            }));

            flipView->SelectedIndex = 1;
        });

        LOG_OUTPUT(L"Waiting for selection change.");
        selectionChangedEvent->WaitForDefault();
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flipView->SelectedIndex, 1);

            xaml_controls::ContentControl^ cc = safe_cast<xaml_controls::ContentControl^>(flipView->ContainerFromItem(flipView->SelectedItem));
            xaml::FrameworkElement^ feContentTemplateRoot = safe_cast<xaml::FrameworkElement^>(cc->ContentTemplateRoot);

            button = dynamic_cast<xaml_controls::Button^>(feContentTemplateRoot->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            autoSuggestBox = dynamic_cast<xaml_controls::AutoSuggestBox^>(feContentTemplateRoot->FindName(L"autoSuggestBox"));
            VERIFY_IS_NOT_NULL(autoSuggestBox);
        });

        LOG_OUTPUT(L"Tapping the AutoSuggestBox to show the SIP.");
        TestServices::InputHelper->Tap(autoSuggestBox);

        LOG_OUTPUT(L"Waiting for the SIP to show.");
        inputPaneShowingEvent->WaitForNoThrow(std::chrono::milliseconds(2000), true /*enforceUnderDebugger*/);
        inputPaneShown = inputPaneShowingEvent->HasFired();
        if (inputPaneShown)
        {
            TestServices::WindowHelper->SynchronouslyTickUIThread(15);
        }
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flipView->SelectedIndex, 1);

            LOG_OUTPUT(L"Focusing the Button to discard the SIP if present.");
            button->Focus(FocusState::Pointer);
        });

        if (inputPaneShown)
        {
            LOG_OUTPUT(L"Waiting for the SIP to hide.");
            inputPaneHidingEvent->WaitForDefault();
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flipView->SelectedIndex, 1);
        });
    }

    void FlipViewIntegrationTests::IncreaseSelectedIndexDuringAnimation()
    {
        ChangeSelectedIndexDuringAnimation(1 /*selectedIndexDelta*/, true  /*whileIncrementingSelectedIndex*/);
        ChangeSelectedIndexDuringAnimation(2 /*selectedIndexDelta*/, true  /*whileIncrementingSelectedIndex*/);
        ChangeSelectedIndexDuringAnimation(1 /*selectedIndexDelta*/, false /*whileIncrementingSelectedIndex*/);
        ChangeSelectedIndexDuringAnimation(2 /*selectedIndexDelta*/, false /*whileIncrementingSelectedIndex*/);
    }

    void FlipViewIntegrationTests::DecreaseSelectedIndexDuringAnimation()
    {
        ChangeSelectedIndexDuringAnimation(-1 /*selectedIndexDelta*/, true  /*whileIncrementingSelectedIndex*/);
        ChangeSelectedIndexDuringAnimation(-2 /*selectedIndexDelta*/, true  /*whileIncrementingSelectedIndex*/);
        ChangeSelectedIndexDuringAnimation(-1 /*selectedIndexDelta*/, false /*whileIncrementingSelectedIndex*/);
        ChangeSelectedIndexDuringAnimation(-2 /*selectedIndexDelta*/, false /*whileIncrementingSelectedIndex*/);
    }

    // Changes the FlipView's SelectedIndex in the middle of an animation resulting from another SelectedIndex change.
    // 'selectedIndexDelta' indicates the delta between the SelectedIndex being animated into, and the SelectedIndex to jump to. It can be positive or negative.
    // 'whileIncrementingSelectedIndex' is True when the animation is caused by a SelectedIndex increment, and is False for a decrement.
    void FlipViewIntegrationTests::ChangeSelectedIndexDuringAnimation(int selectedIndexDelta, bool whileIncrementingSelectedIndex)
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(500, 200);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        const int initialSelectedIndex = 3;
        const int intermediarySelectedIndex = initialSelectedIndex + (whileIncrementingSelectedIndex ? -1 : 1);
        const int newSelectedIndex = initialSelectedIndex + selectedIndexDelta;
        bool selectedIndexChanged = false;

        xaml_controls::FlipView^ flipView = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::ScrollViewer^ scrollViewerInFlipView = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, Loaded);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ChangeSelectedIndexDuringAnimation with selectedIndexDelta=%d and whileIncrementingSelectedIndex=%d.", selectedIndexDelta, whileIncrementingSelectedIndex);

            flipView = safe_cast<xaml_controls::FlipView^>(xaml_markup::XamlReader::Load(
                L"<FlipView x:Name='flipView' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <FlipView.ItemTemplate>"
                L"    <DataTemplate>"
                L"      <Grid Width='500' Height='200'>"
                L"        <Grid.Background>"
                L"          <LinearGradientBrush StartPoint='0,0' EndPoint='1,1'>"
                L"            <GradientStop Color='DarkRed' Offset='0.0'/>"
                L"            <GradientStop Color='White' Offset='0.5'/>"
                L"            <GradientStop Color='DarkBlue' Offset='1.0'/>"
                L"          </LinearGradientBrush>"
                L"        </Grid.Background>"
                L"        <TextBlock Text='{Binding}' FontSize='20'/>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </FlipView.ItemTemplate>"
                L"</FlipView>"));
            VERIFY_IS_NOT_NULL(flipView);

            loadedRegistration.Attach(
                flipView,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = flipView;
        });

        LOG_OUTPUT(L"Waiting for FlipView's Loaded event.");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollViewerInFlipView = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(flipView, L"ScrollingHost"));
            VERIFY_IS_NOT_NULL(scrollViewerInFlipView);

            viewChangedRegistration.Attach(
                scrollViewerInFlipView,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [&selectedIndexChanged, initialSelectedIndex, newSelectedIndex, whileIncrementingSelectedIndex, flipView, scrollViewerInFlipView, viewChangedEvent]
                    (Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewerInFlipView->HorizontalOffset, scrollViewerInFlipView->VerticalOffset, scrollViewerInFlipView->ZoomFactor, args->IsIntermediate);
                if (!selectedIndexChanged &&
                    flipView->SelectedIndex == initialSelectedIndex &&
                    ((whileIncrementingSelectedIndex && scrollViewerInFlipView->HorizontalOffset >= initialSelectedIndex + 1.1) ||
                     (!whileIncrementingSelectedIndex && scrollViewerInFlipView->HorizontalOffset <= initialSelectedIndex + 2.9)))
                {
                    LOG_OUTPUT(L"Setting selection during animation to newSelectedIndex=%d.", newSelectedIndex);
                    selectedIndexChanged = true;
                    flipView->SelectedIndex = newSelectedIndex;
                }
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            auto items = ref new Platform::Collections::Vector<Platform::String^>();
            for (int i = 0; i < 2*initialSelectedIndex+1; i++)
            {
                items->Append(L"Item - Index " + i);
            }
            flipView->ItemsSource = items;

            selectionChangedRegistration.Attach(
                flipView,
                ref new xaml_controls::SelectionChangedEventHandler([newSelectedIndex, flipView, selectionChangedEvent](Platform::Object^, xaml_controls::SelectionChangedEventArgs^)
            {
                LOG_OUTPUT(L"FlipView SelectionChanged handler with SelectionIndex=%d.", flipView->SelectedIndex);
                selectionChangedEvent->Set();
            }));

            LOG_OUTPUT(L"Setting selection to intermediarySelectedIndex=%d.", intermediarySelectedIndex);
            flipView->SelectedIndex = intermediarySelectedIndex;
        });

        LOG_OUTPUT(L"Waiting for view and selection change.");
        selectionChangedEvent->WaitForDefault();
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flipView->SelectedIndex, intermediarySelectedIndex);
            selectionChangedEvent->Reset();
            viewChangedEvent->Reset();

            LOG_OUTPUT(L"Setting selection to initialSelectedIndex=%d.", initialSelectedIndex);
            flipView->SelectedIndex = initialSelectedIndex;
            flipView->UseTouchAnimationsForAllNavigation = false;
        });

        LOG_OUTPUT(L"Waiting for view and selection change.");
        selectionChangedEvent->WaitForDefault();
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flipView->SelectedIndex, newSelectedIndex);
        });
    }

    void FlipViewIntegrationTests::CanBringFlipViewIntoView()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::FlipView^ flipView;
        xaml_controls::FlipViewItem^ flipViewItem;
        xaml_controls::ScrollViewer^ scrollViewer;

        Event flipViewItemgotFocusEvent;
        auto flipViewItemGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::FlipViewItem, GotFocus);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <ScrollViewer Width="200" Height="400" x:Name="scrollViewer">
                            <StackPanel Background="Yellow">
                                <Button Content="Button A" Margin="0,0,0,400" />
                                <FlipView x:Name="flipView" Height="300">
                                    <FlipViewItem x:Name="flipViewItem" Background="Red">
                                        <TextBlock Text="Item 1" />
                                    </FlipViewItem>
                                </FlipView>
                            </StackPanel>
                        </ScrollViewer>
                    </Grid>)"));

            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"scrollViewer"));
            flipView = safe_cast<xaml_controls::FlipView^>(rootPanel->FindName(L"flipView"));
            flipViewItem = safe_cast<xaml_controls::FlipViewItem^>(rootPanel->FindName(L"flipViewItem"));

            flipViewItemGotFocusRegistration.Attach(flipViewItem,
                [&flipViewItemgotFocusEvent]()
            {
                flipViewItemgotFocusEvent.Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        auto scrollViewerBounds = ControlHelper::GetBounds(scrollViewer);
        auto flipViewBounds = ControlHelper::GetBounds(flipView);

        // FlipView should start not in the scroll port.
        VERIFY_IS_FALSE(ControlHelper::IsContainedIn(flipViewBounds, scrollViewerBounds));

        RunOnUIThread([&]()
        {
            flipViewItem->Focus(xaml::FocusState::Keyboard);
        });
        flipViewItemgotFocusEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // After focusing, FlipView should be brought into view.
        flipViewBounds = ControlHelper::GetBounds(flipView);
        VERIFY_IS_TRUE(ControlHelper::IsContainedIn(flipViewBounds, scrollViewerBounds));
    }

    // Verify that FlipView only flips a single item given a series of mouse wheel deltas,
    // then flips back after an immediate mouse wheel delta for the other direction.
    void FlipViewIntegrationTests::MouseWheelInputsFlipOnce()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([&]()
            {
                TestServices::Utilities->RestoreDefaultFlipViewScrollWheelDelay();
            });

            // Restore the default 200ms required delay between wheel deltas to trigger successive flips.
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        auto flipView = SetupBasicFlipView(xaml_controls::Orientation::Horizontal, 10);
        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, GotFocus);
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, SelectionChanged);

        gotFocusRegistration.Attach(
            flipView,
            ref new xaml::RoutedEventHandler(
                [gotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
        {
            LOG_OUTPUT(L"GotFocus event raised.");
            gotFocusEvent->Set();
        }));

        selectionChangedRegistration.Attach(
            flipView,
            ref new xaml_controls::SelectionChangedEventHandler(
                [selectionChangedEvent](Platform::Object^, xaml_controls::SelectionChangedEventArgs^)
        {
            LOG_OUTPUT(L"SelectionChanged event raised.");
            selectionChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            // Temporarily increasing the idle time required to trigger a new flip from 200ms to 750ms
            // to guarantee stability of the test. It not possible to prevent gaps between wheel messages larger than 200ms.
            TestServices::Utilities->SetFlipViewScrollWheelDelay(750);

            flipView->SelectedIndex = 5;
        });

        LOG_OUTPUT(L"Waiting for SelectionChanged event from programmatic selection of 6th item.");
        selectionChangedEvent->WaitForDefault();
        selectionChangedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Giving focus to FlipView.");
        TestServices::InputHelper->LeftMouseClick(flipView);
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Even though messages are sent over a period longer than 750ms, only a single flip is expected
        // because there is no idling period longer than 750ms.
        LOG_OUTPUT(L"Sending mouse wheel messages over a period longer than 750ms.");
        for (int mouseWheelDeltaCount = 0; mouseWheelDeltaCount < 12; mouseWheelDeltaCount++)
        {
            TestServices::InputHelper->ScrollMouseWheel(flipView, -1 /*numberOfWheelClicks*/);
            Sleep(75);
        }

        LOG_OUTPUT(L"Waiting for SelectionChanged event.");
        selectionChangedEvent->WaitForDefault();
        selectionChangedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(6, static_cast<int>(flipView->SelectedIndex));
        });

        Sleep(700);
        LOG_OUTPUT(L"Sending a mouse wheel message after an idling period longer than 750ms.");
        TestServices::InputHelper->ScrollMouseWheel(flipView, -1 /*numberOfWheelClicks*/);

        LOG_OUTPUT(L"Waiting for SelectionChanged event.");
        selectionChangedEvent->WaitForDefault();
        selectionChangedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(7, static_cast<int>(flipView->SelectedIndex));
        });

        LOG_OUTPUT(L"Sending an immediate mouse wheel message for the opposite direction which is expected to trigger a flip.");
        TestServices::InputHelper->ScrollMouseWheel(flipView, 1 /*numberOfWheelClicks*/);

        LOG_OUTPUT(L"Waiting for SelectionChanged event.");
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(6, static_cast<int>(flipView->SelectedIndex));
        });
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::FlipView
