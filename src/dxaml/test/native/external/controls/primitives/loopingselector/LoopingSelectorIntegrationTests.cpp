// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "LoopingSelectorIntegrationTests.h"

#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <Collection.h>

#include <TreeHelper.h>
#include <ControlHelper.h>
#include <LoopingSelectorHelper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include "FocusTestHelper.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace LoopingSelector {

    // TODO 7132718: Change this value to 0.5 once the bug is fixed where
    // calling PanFromCenter() flicks if velocityFactor isn't low enough.
    double LoopingSelectorIntegrationTests::s_PanVelocityFactor = 0.1;

    bool LoopingSelectorIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool LoopingSelectorIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool LoopingSelectorIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    void LoopingSelectorIntegrationTests::TestUpDownButtons()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        auto loopingSelector = CreateTestLoopingSelector();

        xaml_controls::Grid^ rootPanel;
        xaml_primitives::ButtonBase^ upButton;
        xaml_primitives::ButtonBase^ downButton;

        std::shared_ptr<Event> selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, SelectionChanged);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(loopingSelector, [&](){ selectionChangedEvent->Set(); });

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            upButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(loopingSelector, L"UpButton"));
            downButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(loopingSelector, L"DownButton"));
            WEX::Common::Throw::If(upButton == nullptr, E_FAIL, L"upButton should not be null.");
            WEX::Common::Throw::If(downButton == nullptr, E_FAIL, L"downButton should not be null.");

            LOG_OUTPUT(L"LoopingSelector's up/down buttons should start collapsed");
            VERIFY_IS_TRUE(upButton->Visibility == xaml::Visibility::Collapsed);
            VERIFY_IS_TRUE(downButton->Visibility == xaml::Visibility::Collapsed);
            VERIFY_IS_TRUE(loopingSelector->SelectedIndex == 0);
        });

        LOG_OUTPUT(L"Moving the mouse over the LoopingSelector. This should cause the up/down buttons to appear");
        TestServices::InputHelper->MoveMouse(loopingSelector);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(upButton->Visibility == xaml::Visibility::Visible);
            VERIFY_IS_TRUE(downButton->Visibility == xaml::Visibility::Visible);
        });

        LOG_OUTPUT(L"Clicking the Down button");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->LeftMouseClick(downButton);
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(loopingSelector->SelectedIndex == 1);
        });

        LOG_OUTPUT(L"Clicking the Up button");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->LeftMouseClick(upButton);
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(loopingSelector->SelectedIndex == 0);
        });

        LOG_OUTPUT(L"Move mouse away from LoopingSelector");
        TestServices::InputHelper->MoveMouse(wf::Point(0, 0));
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(upButton->Visibility == xaml::Visibility::Collapsed);
            VERIFY_IS_TRUE(downButton->Visibility == xaml::Visibility::Collapsed);
        });

        LOG_OUTPUT(L"Tapping the Down button with Pen");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->PenTap(downButton);
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(loopingSelector->SelectedIndex == 1);
        });

        LOG_OUTPUT(L"Tapping the Up button with Pen");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->PenTap(upButton);
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(loopingSelector->SelectedIndex == 0);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void LoopingSelectorIntegrationTests::UIETree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                auto loopingSelectorRest = CreateTestLoopingSelector();
                auto loopingSelectorHover = CreateTestLoopingSelector();
                auto loopingSelectorUpButtonHover = CreateTestLoopingSelector();
                auto loopingSelectorUpButtonPressed = CreateTestLoopingSelector();
                xaml_controls::StackPanel^ rootPanel;
                xaml_controls::TextBlock^ pointerOverItem;
                xaml_controls::TextBlock^ pressedItem;

                RunOnUIThread([&]()
                {
                    // This avoids problems with the first control in the window sometimes receiving focus.
                    loopingSelectorRest->IsTabStop = false;
                    loopingSelectorHover->IsTabStop = false;
                    loopingSelectorUpButtonHover->IsTabStop = false;
                    loopingSelectorUpButtonPressed->IsTabStop = false;

                    pointerOverItem = ref new xaml_controls::TextBlock();
                    pointerOverItem->Text = "PointerOver";
                    loopingSelectorHover->Items->InsertAt(1, pointerOverItem);

                    pressedItem = ref new xaml_controls::TextBlock();
                    pressedItem->Text = "Pressed";
                    loopingSelectorHover->Items->InsertAt(2, pressedItem);

                    rootPanel = ref new xaml_controls::StackPanel();
                    rootPanel->Orientation = xaml::Controls::Orientation::Horizontal;
                    rootPanel->Children->Append(loopingSelectorRest);
                    rootPanel->Children->Append(loopingSelectorHover);
                    rootPanel->Children->Append(loopingSelectorUpButtonHover);
                    rootPanel->Children->Append(loopingSelectorUpButtonPressed);
                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(loopingSelectorHover, "PointerOver", true);
                    VisualStateManager::GoToState(loopingSelectorUpButtonHover, "PointerOver", true);
                    VisualStateManager::GoToState(loopingSelectorUpButtonPressed, "PointerOver", true);

                    auto pointerOverLoopingSelectorItem = GetContainingLoopingSelectorItem(pointerOverItem);
                    VisualStateManager::GoToState(pointerOverLoopingSelectorItem, "PointerOver", true);

                    auto pressedLoopingSelectorItem = GetContainingLoopingSelectorItem(pressedItem);
                    VisualStateManager::GoToState(pressedLoopingSelectorItem, "Pressed", true);
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    auto upButtonHover = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(loopingSelectorUpButtonHover, L"UpButton"));
                    auto upButtonPressed = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(loopingSelectorUpButtonPressed, L"UpButton"));
                    VisualStateManager::GoToState(upButtonHover, "PointerOver", true);
                    VisualStateManager::GoToState(upButtonPressed, "Pressed", true);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void LoopingSelectorIntegrationTests::TapOnItem()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        auto loopingSelector = CreateTestLoopingSelector();
        xaml_controls::Border^ itemForSelection;
        xaml_controls::Grid^ rootPanel;

        std::shared_ptr<Event> selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, SelectionChanged);

        std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, Loaded);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(loopingSelector, [selectionChangedEvent](){ selectionChangedEvent->Set(); });
            loadedEventRegistration.Attach(loopingSelector, [loadedEvent](){ loadedEvent->Set(); });

            wfc::IVector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>();

            itemForSelection = dynamic_cast<xaml_controls::Border^> (xaml_markup::XamlReader::Load(
                L"<Border xmlns = 'http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  Background='Green' Width='50' Height='40'>"
                L"  <TextBlock Text='YES' />"
                L"</Border>"));

            for (int i = 0; i < 100; i++)
            {
                Platform::Object^ o = ::Windows::Foundation::PropertyValue::CreateInt32(i);
                items->Append(o);
            }

            items->InsertAt(2, itemForSelection);

            loopingSelector->Items = items;

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_NOT_NULL(itemForSelection);

        // Tap on the item to select it:
        TestServices::InputHelper->Tap(itemForSelection);
        TestServices::WindowHelper->WaitForIdle();
        selectionChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(loopingSelector->SelectedItem == itemForSelection);
        });
    }

    void LoopingSelectorIntegrationTests::LoopingSelectorItemPointerOverPressed()
    {
        // Verify:
        // 1. Mouse over a LoopingSelectorItem causes it to go to PointerOver VisualState
        // 2. Mouse press on a LoopingSelectorItem causes it to go to Pressed VisualState
        // 3. Release mouse causes it to get selected and go to Selected VisualState
        // We do this by registering for the VisualStateGroup.CurrentStateChanged event
        // and checking VisualStateGroup.CurrentState matches the expected state as we interact with
        // the LoopingSelector using the mouse.
        // We also include regression coverage for:
        //     Date/TimePicker - Tapping and dragging an item permanently highlights the item

        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        auto loopingSelector = CreateTestLoopingSelector();
        xaml_controls::Grid^ rootPanel;
        xaml_controls::TextBlock^ itemForSelection;
        xaml_primitives::LoopingSelectorItem^ loopingSelectorItem;

        std::shared_ptr<Event> selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, SelectionChanged);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(loopingSelector, [&](){ selectionChangedEvent->Set(); });

            itemForSelection = ref new xaml_controls::TextBlock();
            itemForSelection->Text = "Select Me";
            loopingSelector->Items->InsertAt(2, itemForSelection);

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Move this mouse to a suitable initial position:
        TestServices::InputHelper->MoveMouse(wf::Point(0, 0));
        TestServices::WindowHelper->WaitForIdle();

        xaml::VisualStateGroup^ loopingSelectorItemVisualStateGroup;
        auto stateChangedEvent = std::make_shared<Event>();
        auto stateChangedRegistration = CreateSafeEventRegistration(Xaml::VisualStateGroup, CurrentStateChanged);

        RunOnUIThread([&]()
        {
            loopingSelectorItem = GetContainingLoopingSelectorItem(itemForSelection);

            // Register for VisualStateGroup.VisualStateChanging on the LoopingSelectorItem:
            auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(loopingSelectorItem, 0));
            auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
            VERIFY_IS_TRUE(groups->Size == 1);
            loopingSelectorItemVisualStateGroup = groups->GetAt(0);

            stateChangedRegistration.Attach(loopingSelectorItemVisualStateGroup, [&]() { stateChangedEvent->Set(); });
        });
        TestServices::WindowHelper->WaitForIdle();

        // Test PointerOver state:
        TestServices::InputHelper->MoveMouse(itemForSelection);
        stateChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto currentState = loopingSelectorItemVisualStateGroup->CurrentState;
            VERIFY_ARE_EQUAL(ref new Platform::String(L"PointerOver"), currentState->Name);
        });

        // Test Pressed state:
        stateChangedEvent->Reset();
        TestServices::InputHelper->MouseButtonDown(itemForSelection, 5, 5, MouseButton::Left);
        stateChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto currentState = loopingSelectorItemVisualStateGroup->CurrentState;
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Pressed"), currentState->Name);
        });

        // Test Selected state:
        selectionChangedEvent->Reset();
        stateChangedEvent->Reset();
        TestServices::InputHelper->MouseButtonUp(itemForSelection, 5, 5, MouseButton::Left);
        stateChangedEvent->WaitForDefault();
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto currentState = loopingSelectorItemVisualStateGroup->CurrentState;
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Selected"), currentState->Name);
            VERIFY_IS_TRUE(loopingSelector->SelectedItem == itemForSelection);
        });

        RunOnUIThread([&]()
        {
            loopingSelector->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Regression coverage for:
        //     Date/TimePicker - Tapping and dragging an item permanently highlights the item
        // We press on an item and drag horizontally out of the LoopingSelector. We verify that the item
        // does not get stuck in the Pressed state.

        // These values were changed to work around InjectPressAndDrag not working correctly on 64 bit OS
        TestServices::InputHelper->PressHoldAndPanFromCenter(itemForSelection, 100 /*relX*/, 0 /*relY*/, 10 /*velocityFactor*/, 600 /*holdTime*/);
        stateChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto currentState = loopingSelectorItemVisualStateGroup->CurrentState;
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Normal"), currentState->Name);
        });
    }

    void LoopingSelectorIntegrationTests::TestPanning()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        std::shared_ptr<Event> gotFocusEvent = std::make_shared<Event>();
        auto gotFocusEventRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, GotFocus);

        std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, Loaded);

        auto loopingSelector = CreateTestLoopingSelector();
        xaml_controls::Grid^ rootPanel;

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);

            gotFocusEventRegistration.Attach(loopingSelector,[&]() { gotFocusEvent->Set(); });
            loadedEventRegistration.Attach(loopingSelector,[&]() { loadedEvent->Set(); });
            
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        // Focus the LoopingSelector
        RunOnUIThread([&]()
        {
            loopingSelector->Focus(xaml::FocusState::Programmatic);
        });
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"DoLoopingSelectorSelectionChange");
        LoopingSelectorHelper::DoLoopingSelectorSelectionChange(loopingSelector);

        TestServices::WindowHelper->WaitForIdle();
    }

    void LoopingSelectorIntegrationTests::KeyboardNavigation()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        auto loopingSelector = CreateTestLoopingSelector();
        xaml_controls::Grid^ rootPanel;
        int indexOfLastItem = -1;
        int expectedNumberOfItemsToScollWithPageUpDown = -1;

        std::shared_ptr<Event> selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, SelectionChanged);

        std::shared_ptr<Event> gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, GotFocus);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(loopingSelector,[&](){ selectionChangedEvent->Set(); });
            gotFocusRegistration.Attach(loopingSelector, [&](){ gotFocusEvent->Set(); });

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);
            TestServices::WindowHelper->WindowContent = rootPanel;

            indexOfLastItem = (int)loopingSelector->Items->Size - 1;
            expectedNumberOfItemsToScollWithPageUpDown = static_cast<int>(floor((loopingSelector->Height / loopingSelector->ItemHeight) / 2));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(loopingSelector->SelectedIndex == 0);
            loopingSelector->Focus(xaml::FocusState::Programmatic);
        });
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Changing selection with the Down button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_down#$u$_down", 1);

        LOG_OUTPUT(L"Changing selection with the Up button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_up#$u$_up", 0);

        LOG_OUTPUT(L"Changing selection with the Up button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_up#$u$_up", indexOfLastItem);

        LOG_OUTPUT(L"Changing selection with the Down button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_down#$u$_down", 0);

        LOG_OUTPUT(L"Changing selection with the PageDown button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_pagedown#$u$_pagedown", expectedNumberOfItemsToScollWithPageUpDown);

        LOG_OUTPUT(L"Changing selection with the PageUp button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_pageup#$u$_pageup", 0);

        LOG_OUTPUT(L"Changing selection with the PageUp button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_pageup#$u$_pageup", indexOfLastItem - expectedNumberOfItemsToScollWithPageUpDown + 1);

        LOG_OUTPUT(L"Changing selection with the Home button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_home#$u$_home", 0);

        LOG_OUTPUT(L"Changing selection with the End button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_end#$u$_end", indexOfLastItem);

        LOG_OUTPUT(L"Changing selection with the Down button, reset back to index 0.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_down#$u$_down", 0);

        // Gamepad: RightTrigger == PgDn, LeftTrigger == PgUp.
        LOG_OUTPUT(L"Changing selection with the RightTrigger button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_GamepadRightTrigger#$u$_GamepadRightTrigger", expectedNumberOfItemsToScollWithPageUpDown);

        LOG_OUTPUT(L"Changing selection with the LeftTrigger button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_GamepadLeftTrigger#$u$_GamepadLeftTrigger", 0);

        LOG_OUTPUT(L"Changing selection with the LeftTrigger button.");
        VerifySelectionChangeWithKeyboard(loopingSelector, selectionChangedEvent, L"$d$_GamepadLeftTrigger#$u$_GamepadLeftTrigger", indexOfLastItem - expectedNumberOfItemsToScollWithPageUpDown + 1);
    }

    void LoopingSelectorIntegrationTests::VerifySelectionChangeWithKeyboard(
        xaml_primitives::LoopingSelector^ loopingSelector,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& selectionChangedEvent,
        Platform::String^ keySeq,
        int expectedSelectedIndex)
    {
        selectionChangedEvent->Reset();
        TestServices::KeyboardHelper->PressKeySequence(keySeq);
        selectionChangedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(loopingSelector->SelectedIndex == expectedSelectedIndex);
        });
    }

    void LoopingSelectorIntegrationTests::VerifyHeightChangeDoesNotResultInMissingItems()
    {
        TestCleanupWrapper cleanup;

        // Regression coverage for:
        // [Reminder/Calendar] In the AM/PM column only PM option is available.
        // In this bug, a TimePickerFlyout with Placement=Full was showing with items missing from the AM/PM LoopingSelector
        // The root cause of the bug was due to the LoopingSelector changing height as it opened.
        // When the LoopingSelector was going from a small height to a large height it was missing a call to LoopingSelector::Balance
        // resulting in not enough LoopingSelectorItems being realized.
        // This is only hit in the scenario where ShouldLoop is false and SelectedIndex is not 0.
        //
        // We test this by resizing a LoopingSelector from tall to short to tall.
        //
        // We validate:
        //   1. The appropriate number of LoopingSelectorItems have been created.
        //      i.e. one for each item, since the LoopingSelector is tall enough to fit them all.
        //   2. Each LoopingSelectorItem is visible on the screen.
        //      LoopingSelector will sometimes maintain realized LoopingSelectorItems in the visual tree but will hide them by positioning
        //      them outside of its bounds, so for this reason it is not enough to just validate that the items are in the visual tree.

        auto loopingSelector = CreateTestLoopingSelector();
        unsigned int numberOfItems = 5;

        RunOnUIThread([&]()
        {
            loopingSelector->Height = 300;
            loopingSelector->ItemHeight = 50;
            loopingSelector->ShouldLoop = false;

            auto items = ref new Platform::Collections::Vector<Platform::Object^>();
            for (unsigned int i = 0; i < numberOfItems; i++)
            {
                Platform::Object^ o = ::Windows::Foundation::PropertyValue::CreateInt32(i);
                items->Append(o);
            }
            loopingSelector->Items = items;
            loopingSelector->SelectedIndex = 2;

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            loopingSelector->Height = 10;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            loopingSelector->Height = 300;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Get the realized LoopingSelectorItems:
            auto loopingSelectorItems = ref new Platform::Collections::Vector<xaml_primitives::LoopingSelectorItem^>();
            TreeHelper::GetVisualChildrenByType(loopingSelector, loopingSelectorItems);

            // We expect all of the items to be visible on the screen, hence we expect a realized LoopingSelectorItem for
            // each of the Items in the LoopingSelector:
            VERIFY_ARE_EQUAL(numberOfItems, loopingSelectorItems->Size);

            // All of the LoopingSelectorItems should be visible in the LoopingSelector:
            auto loopingSelectorBounds = ControlHelper::GetBounds(loopingSelector);
            for (unsigned int i = 0; i < numberOfItems; i++)
            {
                auto item = loopingSelectorItems->GetAt(i);
                auto itemBounds = ControlHelper::GetBounds(item);
                VERIFY_IS_TRUE(ControlHelper::IsContainedIn(itemBounds, loopingSelectorBounds));
            }
        });
    }

    void LoopingSelectorIntegrationTests::LoopingSelectorPanLoop()
    {
        LoopingSelectorPanHelper(true /*shouldLoop*/);
    }

    void LoopingSelectorIntegrationTests::LoopingSelectorPanNoLoop()
    {
        LoopingSelectorPanHelper(false /*shouldLoop*/);
    }

    void LoopingSelectorIntegrationTests::LoopingSelectorPanHelper(bool shouldLoop)
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        auto loopingSelector = CreateTestLoopingSelector();
        xaml_controls::Grid^ rootPanel;
        xaml_controls::ScrollViewer^ scrollViewer;

        xaml_controls::TextBlock^ firstItem;
        xaml_controls::TextBlock^ lastItem;

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            firstItem = ref new xaml_controls::TextBlock();
            firstItem->Text = "First Item";
            loopingSelector->Items->InsertAt(0, firstItem);

            lastItem = ref new xaml_controls::TextBlock();
            lastItem->Text = "Last Item";
            loopingSelector->Items->InsertAt((int)loopingSelector->Items->Size, lastItem);

            // Start a few elements down. This ensures that lastItem is not rendered yet.
            loopingSelector->SelectedIndex = 5;
            loopingSelector->ShouldLoop = shouldLoop;

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Get the template ScrollViewer so we can listen to its ViewChanged event.
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(loopingSelector, L"ScrollViewer"));
            VERIFY_IS_NOT_NULL(scrollViewer);

            viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanging, NextView: (%.0f, %.0f), FinalView: (%.0f, %.0f), IsInertial: %d",
                    args->NextView->HorizontalOffset,
                    args->NextView->VerticalOffset,
                    args->FinalView->HorizontalOffset,
                    args->FinalView->VerticalOffset,
                    args->IsInertial);
            }));

            // IsIntermediate is true while a pan hasn't completed its movement and false otherwise.
            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));
        });

        // Do three small pans instead of one large one to increase stability.
        int numberOfPans = 3;
        int panAmount = 100;
        for (int i = 0; i < numberOfPans; i++ )
        {
            test_infra::TestServices::InputHelper->PanFromCenter(loopingSelector, 0 /*relX*/, panAmount /*relY*/, s_PanVelocityFactor /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();

            // Before we begin the next pan, wait for the viewChangedEvent to fire so we know the current pan has finished.
            viewChangedEvent->WaitForDefault();
            viewChangedEvent->Reset();
            TestServices::WindowHelper->WaitForIdle();
        }

        // Get the bounds of the last item after we pan.
        ::Windows::Foundation::Rect postPanBounds = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(lastItem, false);
        LOG_OUTPUT(L"postPanBounds: [X: %f, Y: %f, W: %f, H: %f]", postPanBounds.X, postPanBounds.Y, postPanBounds.Width, postPanBounds.Height);

        if (shouldLoop)
        {
            // Get the bounds of the first item. Since we should have looped, the first item should be below the last item.
            ::Windows::Foundation::Rect firstItemBounds = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(firstItem, false);
            LOG_OUTPUT(L"firstItemBounds: [X: %f, Y: %f, W: %f, H: %f]", firstItemBounds.X, firstItemBounds.Y, firstItemBounds.Width, firstItemBounds.Height);

            VERIFY_IS_LESS_THAN(postPanBounds.Y, firstItemBounds.Y);
        }
        else
        {
            ::Windows::Foundation::Rect zeroBounds = {0,0,0,0};

            // If we didn't loop, then the last item shouldn't have been realized and thus the bounds should be zero.
            VERIFY_ARE_EQUAL(zeroBounds, postPanBounds);

            // Note that we don't attempt to tap here, since InputHelper would throw an exception (zero height exception).
            // Verify that we're at the top of the list instead.
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(loopingSelector->SelectedItem, firstItem);
            });
        }
    }

    void LoopingSelectorIntegrationTests::TestBalanceAndNormalize()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::Grid^ rootPanel;
        auto loopingSelector = CreateTestLoopingSelector();

        // Note that these values are arbitrary; the test should function fine if these values are changed.
        const unsigned int numberOfItemsShown = 5;
        const int panAmount = 250;

        int initialSelectedIndex;
        int itemHeight;

        // Explicitly set the height here so we can control how many items we expect (as virtualization depends on the height of the viewport)
        RunOnUIThread([&]()
        {
            initialSelectedIndex = loopingSelector->Items->Size / 2;
            itemHeight = loopingSelector->ItemHeight;

            loopingSelector->Height = itemHeight * numberOfItemsShown;
            loopingSelector->SelectedIndex = initialSelectedIndex;

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Setup the listener for the viewChangedEvent.
        xaml_controls::ScrollViewer^ scrollViewer;
        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(loopingSelector, L"ScrollViewer"));
            VERIFY_IS_NOT_NULL(scrollViewer);

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));
        });

        // Do an initial verification before panning.
        ValidatePositionsOfRealizedItems(loopingSelector, numberOfItemsShown, initialSelectedIndex);

        // Pan vertically by the Pan amount
        test_infra::TestServices::InputHelper->PanFromCenter(loopingSelector, 0 /*relX*/, panAmount /*relY*/, s_PanVelocityFactor /*velocityFactor*/);
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Calculate the number of items we expect that we panned for a given pan amount.
        int numberOfItemsPanned = panAmount / itemHeight;

        // Verify after we pan.
        ValidatePositionsOfRealizedItems(loopingSelector, numberOfItemsShown, initialSelectedIndex - numberOfItemsPanned);
    }

    void LoopingSelectorIntegrationTests::ValidatePositionsOfRealizedItems(xaml_primitives::LoopingSelector^ loopingSelector, unsigned int numberOfItemsShown, int expectedSelectedIndex)
    {
        RunOnUIThread([&]()
        {
            const double itemHeight = loopingSelector->ItemHeight;

            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(loopingSelector, L"ScrollViewer"));
            VERIFY_IS_NOT_NULL(scrollViewer);

            auto selectorCanvas = safe_cast<xaml_controls::Canvas^>(scrollViewer->Content);
            VERIFY_IS_NOT_NULL(selectorCanvas);

            // This tests the Normalize part of LoopingSelector, which ensures that even after we scroll the ScrollViewer, the vertical offset remains consistent
            // relative to the actual height of the backing canvas. The SV's VerticalOffset is roughly twice [the actual height of the canvas plus the looping selector height].
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, (selectorCanvas->ActualHeight - loopingSelector->Height + itemHeight) / 2);

            // Verify that the number of children shown in the canvas is [(number of items shown in viewport * 2) + 1 for the middle item];
            // This tests the Balance part of LoopingSelector, which ensures that the number of items in the canvas is not more than is needed.
            xaml_controls::UIElementCollection^ childCollection = selectorCanvas->Children;
            VERIFY_ARE_EQUAL(childCollection->Size, (numberOfItemsShown * 2) + 1);

            // Validation check to ensure that our pan successfully went to the value we expected
            VERIFY_ARE_EQUAL(loopingSelector->SelectedIndex, expectedSelectedIndex);

            // Get the selected item's bounds.
            auto selectedItem = LoopingSelectorHelper::FindLoopingSelectorItemByInt(childCollection, expectedSelectedIndex);
            VERIFY_IS_NOT_NULL(selectedItem);

            ::Windows::Foundation::Rect selectedBounds = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(selectedItem, true);
            LOG_OUTPUT(L"SelectedItem Val: %d, bounds: [X: %f, Y: %f, W: %f, H: %f]", expectedSelectedIndex, selectedBounds.X, selectedBounds.Y, selectedBounds.Width, selectedBounds.Height);

            // Get the range of items in the selector based on the expected selected index.
            const int minIndex = expectedSelectedIndex - numberOfItemsShown;
            const int maxIndex = expectedSelectedIndex + numberOfItemsShown;

            // The expected viewport for items is the number of items shown times the height of those items.
            const double expectedBoundingViewport = itemHeight * numberOfItemsShown;
            const int viewportBuffer = 5; // Small buffer to account for possible rendering inconsistencies.

            ::Windows::Foundation::Rect oldBounds = {0,0,0,0};
            for (int i = minIndex; i <= maxIndex; i++)
            {
                // Verify that we've found the item that we expect.
                auto item = LoopingSelectorHelper::FindLoopingSelectorItemByInt(childCollection, i);
                VERIFY_IS_NOT_NULL(item);

                // Get the bounds of that item
                ::Windows::Foundation::Rect itemBounds = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(item, true);
                LOG_OUTPUT(L"ItemBounds Val: %d, bounds: [X: %f, Y: %f, W: %f, H: %f]", i, itemBounds.X, itemBounds.Y, itemBounds.Width, itemBounds.Height);

                // Don't compare against old bounds if there aren't any old bounds yet.
                if (oldBounds.Y != 0)
                {
                    // Since we're traversing the list in order, we expect new items to appear below old items.
                    VERIFY_IS_TRUE(itemBounds.Y > oldBounds.Y);
                }

                // Further, we expect all items less than the selectedIndex to appear above the selected item (within a viewport of ItemHeight * numItemsShown), and the rest to appear below.
                if (i < expectedSelectedIndex)
                {
                    VERIFY_IS_TRUE(itemBounds.Y >= (selectedBounds.Y - expectedBoundingViewport - viewportBuffer) && itemBounds.Y <= selectedBounds.Y);
                }
                else if (i > expectedSelectedIndex)
                {
                    VERIFY_IS_TRUE(itemBounds.Y >= selectedBounds.Y && itemBounds.Y <= (selectedBounds.Y + expectedBoundingViewport + viewportBuffer));
                }

                // Cache this item's bounds for the next time aroud.
                oldBounds = itemBounds;
            }
        });
    }

    void LoopingSelectorIntegrationTests::ValidateTappingItemLoopsCorrectly()
    {
        // Regression coverage for:
        //   TimePicker loops all the way through every option in a column before landing on item when moving from "12" to any number above it.
        // Because LoopingSelector loops its items, multiple LoopingSelectorItems can represent the same value. We want to make sure that when tapping
        // an item, that very item is what gets scrolled into position and not a different item representing the same value.

        // We create a LoopingSelector and set its initial SelectedIndex to the last item. We then tap on the the first item, which is visible
        // due to the fact that the items loop around.
        // The buggy behavior was that instead of just moving the tapped item into the selection, the LoopingSelector looped all the way around in the opposite
        // direction to select a different LoopingSelectorItem which represented the same value.
        // So, in this test we verify that after tapping on a LoopingSelectorItem, that exact item is still visible after the Selection changes.
        TestCleanupWrapper cleanup;

        int itemCount = 50;
        int lastIndex = itemCount - 1;
        int firstIndex = 0;
        xaml_primitives::LoopingSelectorItem^ lastItem;
        xaml_primitives::LoopingSelectorItem^ firstItem;

        auto loopingSelector = CreateTestLoopingSelector(itemCount);

        RunOnUIThread([&]()
        {
            loopingSelector->SelectedIndex = itemCount - 1;

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto items = ref new Platform::Collections::Vector<xaml_primitives::LoopingSelectorItem^>();
            TreeHelper::GetVisualChildrenByType<xaml_controls::Primitives::LoopingSelectorItem>(loopingSelector, items);

            lastItem = LoopingSelectorHelper::FindLoopingSelectorItemByInt(items, lastIndex);
            firstItem = LoopingSelectorHelper::FindLoopingSelectorItemByInt(items, firstIndex);

            auto loopingSelectorBounds = ControlHelper::GetBounds(loopingSelector);
            auto lastItemBounds = ControlHelper::GetBounds(lastItem);
            auto firstItemBounds = ControlHelper::GetBounds(firstItem);

            // because we loop, both the last item and the first item should be visible:
            VERIFY_IS_TRUE(ControlHelper::IsContainedIn(lastItemBounds, loopingSelectorBounds));
            VERIFY_IS_TRUE(ControlHelper::IsContainedIn(firstItemBounds, loopingSelectorBounds));

            VERIFY_IS_GREATER_THAN_OR_EQUAL(firstItemBounds.Top, lastItemBounds.Bottom, L"first item should be below last item (i.e. it should loop around)");
        });
        TestServices::WindowHelper->WaitForIdle();

        std::shared_ptr<Event> selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, SelectionChanged);
        selectionChangedRegistration.Attach(loopingSelector, [selectionChangedEvent](){ selectionChangedEvent->Set(); });

        FocusTestHelper::EnsureFocus(loopingSelector, FocusState::Keyboard);

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(firstItem);
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, loopingSelector->SelectedIndex);

            // After tapping, we make sure that the same LoopingSelectorItem that we tapped is visible
            // and that it is still being used to represent the same content (i.e. has not been recycled by
            // virtualization).

            VERIFY_ARE_EQUAL(firstIndex, safe_cast<int>(firstItem->Content));
            VERIFY_ARE_EQUAL(lastIndex, safe_cast<int>(lastItem->Content));

            auto loopingSelectorBounds = ControlHelper::GetBounds(loopingSelector);
            auto lastItemBounds = ControlHelper::GetBounds(lastItem);
            auto firstItemBounds = ControlHelper::GetBounds(firstItem);

            VERIFY_IS_TRUE(ControlHelper::IsContainedIn(lastItemBounds, loopingSelectorBounds));
            VERIFY_IS_TRUE(ControlHelper::IsContainedIn(firstItemBounds, loopingSelectorBounds));

            VERIFY_IS_GREATER_THAN_OR_EQUAL(firstItemBounds.Top, lastItemBounds.Bottom, L"first item should be below last item (i.e. it should loop around)");
        });
    }

    // LoopingSelector does not export a constructor via WinRT. This is because it is not designed to be used by 3rd party apps.
    // Rather it is for use exclusively by DatePickerFlyout and TimePickerFlyout.
    // In order to test this control in isolation we rely on a test hook to create the control.
    xaml_primitives::LoopingSelector^ LoopingSelectorIntegrationTests::CreateTestLoopingSelector(int itemCount)
    {
        xaml_primitives::LoopingSelector^ loopingSelector;

        RunOnUIThread([&]()
        {
            Platform::Object^ loopingSelectorAsObj;
            TestServices::Utilities->CreateLoopingSelector(&loopingSelectorAsObj);
            loopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(loopingSelectorAsObj);
            VERIFY_IS_NOT_NULL(loopingSelector);

            loopingSelector->Width = 100;
            loopingSelector->Height = 300;
            loopingSelector->ItemHeight = 50;
            loopingSelector->ItemWidth = 100;
            loopingSelector->ItemTemplate = nullptr;

            wfc::IVector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>();
            for (int i = 0; i < itemCount; i++)
            {
                Platform::Object^ o = ::Windows::Foundation::PropertyValue::CreateInt32(i);
                items->Append(o);
            }

            loopingSelector->Items = items;
        });
        return loopingSelector;
    }

    xaml_primitives::LoopingSelectorItem^ LoopingSelectorIntegrationTests::GetContainingLoopingSelectorItem(xaml::FrameworkElement^ element)
    {
        auto parent = safe_cast<xaml::FrameworkElement^>(element->Parent);
        if (parent == nullptr)
        {
            return nullptr;
        }

        auto lsi = safe_cast<xaml_primitives::LoopingSelectorItem^>(parent);
        if (lsi)
        {
            return lsi;
        }
        else
        {
            return GetContainingLoopingSelectorItem(parent);
        }
    }

} } } } } } } // Microsoft::UI::Xaml::Tests::Controls::Primitives::LoopingSelector
