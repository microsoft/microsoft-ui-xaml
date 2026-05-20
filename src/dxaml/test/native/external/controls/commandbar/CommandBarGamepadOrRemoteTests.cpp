// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <TreeHelper.h>
#include "CommandBarGamepadOrRemoteTests.h"
#include "KeyboardInjectionOverride.h"

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;

using namespace test_infra;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Controls {
        namespace CommandBar {

        Platform::String^ CommandBarGamepadOrRemoteTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\controls\\commandbar\\";
        }

        bool CommandBarGamepadOrRemoteTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

    bool CommandBarGamepadOrRemoteTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

        bool CommandBarGamepadOrRemoteTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        UIElement^ CommandBarGamepadOrRemoteTests::SetupTest(Platform::String^ xamlFile)
        {
            auto rootPage = safe_cast<Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + xamlFile));
            VERIFY_IS_NOT_NULL(rootPage);

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(Page, Loaded);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CommandBarGamepadOrRemoteTests: SetupTest(%s)", xamlFile->Data());

                loadedRegistration.Attach(rootPage, [&]()
                {
                    LOG_OUTPUT(L"[RootPage]: Loaded");
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPage;
            });
            TestServices::WindowHelper->WaitForIdle();
            loadedEvent->WaitForDefault();

            return rootPage;
        }

        //-------------------------------------------------------------------------------
        // Test case: Renders CommandBarGamepadOrRemoteTestScene.xaml
        //            Verifies simple focus transition works within elements
        //            in and out of the commandbars using gamepad or remote
        //-------------------------------------------------------------------------------
        void CommandBarGamepadOrRemoteTests::SimpleNavigation()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);

            InputDevice device = InputDevice::Gamepad;

            auto page = safe_cast<Page^>(SetupTest(L"CommandBarGamepadOrRemoteTestScene.xaml"));
            xaml_controls::CommandBar^ topCommandBar = nullptr;

            auto topCommandBar_OpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            auto topCommandBar_OpenedEvent = std::make_shared<Event>();

            auto topCommandBar_ClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
            auto topCommandBar_ClosedEvent = std::make_shared<Event>();

            auto btn1_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto btn1_gotFocusEvent = std::make_shared<Event>();

            auto overflowbtn0_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowbtn0_gotFocusEvent = std::make_shared<Event>();

            auto overflowbtn1_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowbtn1_gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                topCommandBar = safe_cast<xaml_controls::CommandBar^>(TreeHelper::GetVisualChildByName(page, L"topCommandBar"));
                VERIFY_IS_NOT_NULL(topCommandBar);

                topCommandBar_OpenedRegistration.Attach(topCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[topCommandBar]: Opened Event Fired");
                    topCommandBar_OpenedEvent->Set();
                });

                topCommandBar_ClosedRegistration.Attach(topCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[topCommandBar]: Closed Event Fired");
                    topCommandBar_ClosedEvent->Set();
                });

                auto topAppBarBtn0 = safe_cast<AppBarButton^>(topCommandBar->PrimaryCommands->GetAt(0));
                VERIFY_IS_NOT_NULL(topAppBarBtn0);

                LOG_OUTPUT(L"Setting focus on topAppBarBtn0");
                topAppBarBtn0->Focus(FocusState::Keyboard);

                auto topAppBarBtn1 = safe_cast<AppBarToggleButton^>(topCommandBar->PrimaryCommands->GetAt(1));
                VERIFY_IS_NOT_NULL(topAppBarBtn1);

                btn1_gotFocusRegistration.Attach(topAppBarBtn1, [&]()
                {
                    LOG_OUTPUT(L"[topAppBarBtn1]: Got Focus Event Fired");
                    btn1_gotFocusEvent->Set();
                });

                auto topOverflowAppBarBtn0 = safe_cast<AppBarButton^>(topCommandBar->SecondaryCommands->GetAt(0));
                VERIFY_IS_NOT_NULL(topOverflowAppBarBtn0);

                overflowbtn0_gotFocusRegistration.Attach(topOverflowAppBarBtn0, [&]()
                {
                    LOG_OUTPUT(L"[topOverflowAppBarBtn0]: Got Focus Event Fired");
                    overflowbtn0_gotFocusEvent->Set();
                });

                auto topOverflowAppBarBtn1 = safe_cast<AppBarToggleButton^>(topCommandBar->SecondaryCommands->GetAt(1));
                VERIFY_IS_NOT_NULL(topOverflowAppBarBtn1);

                overflowbtn1_gotFocusRegistration.Attach(topOverflowAppBarBtn1, [&]()
                {
                    LOG_OUTPUT(L"[topOverflowAppBarBtn1]: Got Focus Event Fired");
                    overflowbtn1_gotFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Right(device); //topAppBarBtn0->topAppBarBtn1
            CommonInputHelper::Right(device); //topAppBarBtn1->ExpandButton
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Accept(device); //Open overflow
            topCommandBar_OpenedEvent->WaitForDefault();

            VERIFY_IS_TRUE(btn1_gotFocusEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();            
            
            CommonInputHelper::Down(device); //topOverflowAppBarBtn0->topOverflowAppBarBtn1
            CommonInputHelper::Accept(device);
            TestServices::WindowHelper->WaitForIdle();
            topCommandBar_ClosedEvent->WaitForDefault();

            VERIFY_IS_TRUE(overflowbtn0_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(overflowbtn1_gotFocusEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();

            auto bottomAppBarBtn0_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto bottomAppBarBtn0_gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                auto bottomAppBarBtn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(page->BottomAppBar, L"MoreButton"));
                VERIFY_IS_NOT_NULL(bottomAppBarBtn);

                bottomAppBarBtn0_gotFocusRegistration.Attach(bottomAppBarBtn, [&]()
                {
                    LOG_OUTPUT(L"[bottomAppBarBtn0]: Got Focus Event Fired");
                    bottomAppBarBtn0_gotFocusEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Down(device); //topAppBar ExpandButton->bottom AppBar ExpandButton
            TestServices::WindowHelper->WaitForIdle();

            bottomAppBarBtn0_gotFocusEvent->WaitForDefault();

            auto btn_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto btn_gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                auto btn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(page, L"btn"));
                VERIFY_IS_NOT_NULL(btn);

                btn_gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"[btn]: Got Focus Event Fired");
                    btn_gotFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            for (int i=0;i<6;i++)
            {
                //Navigate buttons in the bottom appbar
                CommonInputHelper::Left(device);
            }
            TestServices::WindowHelper->WaitForIdle();
            CommonInputHelper::Up(device); //bottomAppBarButton0->(Non-AppBar) btn
            TestServices::WindowHelper->WaitForIdle();

            btn_gotFocusEvent->WaitForDefault();
        }

        //-------------------------------------------------------------------------------
        // Test case: Renders CommandBarGamepadOrRemoteTestScene.xaml
        //            Verifies that once you enter a commandbar (in visual-tree) overflow,
        //              you can't exit the focus trap using gamepad
        //              until a selection is made or expand button is pressed
        //-------------------------------------------------------------------------------
        void CommandBarGamepadOrRemoteTests::FocusTrapInOverflow_CommandBarInTree()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);

            InputDevice device = InputDevice::Gamepad;

            auto page = safe_cast<Page^>(SetupTest(L"CommandBarGamepadOrRemoteTestScene.xaml"));
            xaml_controls::CommandBar^ topCommandBar = nullptr;
            Button^ topAppBarExpandButton = nullptr;
            auto topCommandBar_OpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            auto topCommandBar_OpenedEvent = std::make_shared<Event>();

            auto topCommandBar_ClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
            auto topCommandBar_ClosedEvent = std::make_shared<Event>();

            auto overflowbtn0_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowbtn0_gotFocusEvent = std::make_shared<Event>();

            auto overflowbtn1_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowbtn1_gotFocusEvent = std::make_shared<Event>();

            AppBarButton^ topOverflowAppBarBtn0 = nullptr;

            RunOnUIThread([&]()
            {
                topAppBarExpandButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(page, L"MoreButton"));
                VERIFY_IS_NOT_NULL(topAppBarExpandButton);

                LOG_OUTPUT(L"Setting focus on topAppBarExpandButton");
                topAppBarExpandButton->Focus(FocusState::Keyboard);

                topCommandBar = safe_cast<xaml_controls::CommandBar^>(TreeHelper::GetVisualChildByName(page, L"topCommandBar"));
                VERIFY_IS_NOT_NULL(topCommandBar);

                topCommandBar_OpenedRegistration.Attach(topCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[topCommandBar]: Opened Event Fired");
                    topCommandBar_OpenedEvent->Set();
                });

                topCommandBar_ClosedRegistration.Attach(topCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[topCommandBar]: Closed Event Fired");
                    topCommandBar_ClosedEvent->Set();
                });

                topOverflowAppBarBtn0 = safe_cast<AppBarButton^>(topCommandBar->SecondaryCommands->GetAt(0));
                VERIFY_IS_NOT_NULL(topOverflowAppBarBtn0);

                overflowbtn0_gotFocusRegistration.Attach(topOverflowAppBarBtn0, [&]()
                {
                    LOG_OUTPUT(L"[topOverflowAppBarBtn0]: Got Focus Event Fired");
                    overflowbtn0_gotFocusEvent->Set();
                });

                auto topOverflowAppBarBtn1 = safe_cast<AppBarToggleButton^>(topCommandBar->SecondaryCommands->GetAt(1));
                VERIFY_IS_NOT_NULL(topOverflowAppBarBtn1);

                overflowbtn1_gotFocusRegistration.Attach(topOverflowAppBarBtn1, [&]()
                {
                    LOG_OUTPUT(L"[topOverflowAppBarBtn1]: Got Focus Event Fired");
                    overflowbtn1_gotFocusEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            CommonInputHelper::Accept(device); //Open overflow
            topCommandBar_OpenedEvent->WaitForDefault();

            CommonInputHelper::Down(device); //expand button->topAppBarBtn0
            CommonInputHelper::Down(device); //topAppBarBtn0->topAppBarBtn1

            overflowbtn1_gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(overflowbtn0_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(overflowbtn1_gotFocusEvent->HasFired());

            //Attempts to exit the trap
            CommonInputHelper::Right(device);
            CommonInputHelper::Left(device);

            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Up(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(topOverflowAppBarBtn0->FocusState == FocusState::Unfocused);
            });

            // Arow down to get to the expand button
            CommonInputHelper::Down(device);
            CommonInputHelper::Accept(device); //Close Overflow

            topCommandBar_ClosedEvent->WaitForDefault();
        }

        //-------------------------------------------------------------------------------
        // Test case: Renders CommandBarGamepadOrRemoteTestScene.xaml
        //            Verifies that once you enter a bottom-commandbar overflow,
        //              you can't exit the focus trap using gamepad
        //              until a selection is made or expand button is pressed
        //-------------------------------------------------------------------------------
        void CommandBarGamepadOrRemoteTests::FocusTrapInOverflow_BottomCommandBar()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);

            InputDevice device = InputDevice::Gamepad;

            auto page = safe_cast<Page^>(SetupTest(L"CommandBarGamepadOrRemoteTestScene.xaml"));
            xaml_controls::CommandBar^ bottomCommandBar = nullptr;
            Button^ bottomAppBarExpandButton = nullptr;
            auto bottomCommandBar_OpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            auto bottomCommandBar_OpenedEvent = std::make_shared<Event>();

            auto bottomCommandBar_ClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
            auto bottomCommandBar_ClosedEvent = std::make_shared<Event>();

            auto overflowbtn0_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowbtn0_gotFocusEvent = std::make_shared<Event>();

            auto overflowbtn1_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowbtn1_gotFocusEvent = std::make_shared<Event>();

            AppBarButton^ bottomOverflowAppBarBtn0 = nullptr;

            RunOnUIThread([&]()
            {
                bottomCommandBar = safe_cast<xaml_controls::CommandBar^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"bottomCommandBar", page));
                VERIFY_IS_NOT_NULL(bottomCommandBar);

                bottomAppBarExpandButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(bottomCommandBar, L"MoreButton"));
                VERIFY_IS_NOT_NULL(bottomAppBarExpandButton);

                LOG_OUTPUT(L"Setting focus on bottomAppBarExpandButton");
                bottomAppBarExpandButton->Focus(FocusState::Keyboard);

                bottomCommandBar_OpenedRegistration.Attach(bottomCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Opened Event Fired");
                    bottomCommandBar_OpenedEvent->Set();
                });

                bottomCommandBar_ClosedRegistration.Attach(bottomCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Closed Event Fired");
                    bottomCommandBar_ClosedEvent->Set();
                });
            
                bottomOverflowAppBarBtn0 = safe_cast<AppBarButton^>(bottomCommandBar->SecondaryCommands->GetAt(0));
                VERIFY_IS_NOT_NULL(bottomOverflowAppBarBtn0);

                overflowbtn0_gotFocusRegistration.Attach(bottomOverflowAppBarBtn0, [&]()
                {
                    LOG_OUTPUT(L"[bottomOverflowAppBarBtn0]: Got Focus Event Fired");
                    overflowbtn0_gotFocusEvent->Set();
                });

                auto bottomOverflowAppBarBtn1 = safe_cast<AppBarButton^>(bottomCommandBar->SecondaryCommands->GetAt(1));
                VERIFY_IS_NOT_NULL(bottomOverflowAppBarBtn1);

                overflowbtn1_gotFocusRegistration.Attach(bottomOverflowAppBarBtn1, [&]()
                {
                    LOG_OUTPUT(L"[bottomOverflowAppBarBtn1]: Got Focus Event Fired");
                    overflowbtn1_gotFocusEvent->Set();
                });

            });
            TestServices::WindowHelper->WaitForIdle();
            CommonInputHelper::Accept(device); //Open overflow
            bottomCommandBar_OpenedEvent->WaitForDefault();

            CommonInputHelper::Up(device); //bottomOverflowAppBarBtn0->bottomOverflowAppBarBtn1
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(overflowbtn0_gotFocusEvent->HasFired());

            //Attempts to exit the trap
            CommonInputHelper::Right(device);
            CommonInputHelper::Left(device);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(bottomOverflowAppBarBtn0->FocusState == FocusState::Unfocused);
            });

            // Arow up to get to the expand button
            CommonInputHelper::Up(device);
            CommonInputHelper::Accept(device); //Close Overflow

            bottomCommandBar_ClosedEvent->WaitForDefault();
        }

        //-------------------------------------------------------------------------------
        // Test case: Verifies that once you enter a commandbar overflow,
        //              you can navigate to the primary items using horizontal navigation keys
        //              when the focus is on the Expand Button
        //-------------------------------------------------------------------------------
        void CommandBarGamepadOrRemoteTests::FocusNotTrappedInOverflow_ExpandButton()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);

            InputDevice device = InputDevice::Gamepad;

            auto page = safe_cast<Page^>(SetupTest(L"CommandBarGamepadOrRemoteTestScene.xaml"));
            xaml_controls::CommandBar^ bottomCommandBar = nullptr;
            Button^ bottomAppBarExpandButton = nullptr;
            auto bottomCommandBar_OpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            auto bottomCommandBar_OpenedEvent = std::make_shared<Event>();

            auto bottomCommandBar_ClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
            auto bottomCommandBar_ClosedEvent = std::make_shared<Event>();

            auto bottomAppBarBtn4_gotFocusRegistration = CreateSafeEventRegistration(AppBarButton, GotFocus);
            auto bottomAppBarBtn4_gotFocusEvent = std::make_shared<Event>();

            AppBarButton^ bottomAppBarBtn4 = nullptr;

            RunOnUIThread([&]()
            {
                bottomCommandBar = safe_cast<xaml_controls::CommandBar^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"bottomCommandBar", page));
                VERIFY_IS_NOT_NULL(bottomCommandBar);

                bottomCommandBar->IsDynamicOverflowEnabled = false;

                bottomAppBarExpandButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(bottomCommandBar, L"MoreButton"));
                VERIFY_IS_NOT_NULL(bottomAppBarExpandButton);

                LOG_OUTPUT(L"Setting focus on bottomAppBarExpandButton");
                bottomAppBarExpandButton->Focus(FocusState::Keyboard);

                bottomCommandBar_OpenedRegistration.Attach(bottomCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Opened Event Fired");
                    bottomCommandBar_OpenedEvent->Set();
                });

                bottomCommandBar_ClosedRegistration.Attach(bottomCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Closed Event Fired");
                    bottomCommandBar_ClosedEvent->Set();
                });
                                
                bottomAppBarBtn4 = safe_cast<AppBarButton^>(bottomCommandBar->PrimaryCommands->GetAt(5));
                VERIFY_IS_NOT_NULL(bottomAppBarBtn4);

                bottomAppBarBtn4_gotFocusRegistration.Attach(bottomAppBarBtn4, [&]()
                {
                    LOG_OUTPUT(L"[bottomAppBarBtn4]: Got Focus Event Fired");
                    bottomAppBarBtn4_gotFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Accept(device); //Open overflow
            bottomCommandBar_OpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // Navigate left out of the ellipses
            CommonInputHelper::Down(device);
            CommonInputHelper::Down(device);
            CommonInputHelper::Left(device);
            // Verify that the next left button got focus
            bottomAppBarBtn4_gotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Accept(device); //Close overflow
            bottomCommandBar_ClosedEvent->WaitForDefault();
        }

        void CommandBarGamepadOrRemoteTests::FocusTrapInCommandBarWhenOverflowIsOpen()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);

            InputDevice device = InputDevice::Gamepad;

            auto page = safe_cast<Page^>(SetupTest(L"CommandBarGamepadOrRemoteTestScene.xaml"));
            xaml_controls::CommandBar^ topCommandBar = nullptr;
            Button^ topAppBarExpandButton = nullptr;
            auto topCommandBar_OpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            auto topCommandBar_OpenedEvent = std::make_shared<Event>();

            auto topCommandBar_ClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
            auto topCommandBar_ClosedEvent = std::make_shared<Event>();

            auto topAppBarBtn0_lostFocusRegistration = CreateSafeEventRegistration(UIElement, LostFocus);
            auto topAppBarBtn0_lostFocusEvent = std::make_shared<Event>();

            AppBarButton^ topAppBarBtn0 = nullptr;
            RunOnUIThread([&]()
            {
                topAppBarExpandButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(page, L"MoreButton"));
                VERIFY_IS_NOT_NULL(topAppBarExpandButton);

                LOG_OUTPUT(L"Setting focus on topAppBarExpandButton");
                topAppBarExpandButton->Focus(FocusState::Keyboard);

                topCommandBar = safe_cast<xaml_controls::CommandBar^>(TreeHelper::GetVisualChildByName(page, L"topCommandBar"));
                VERIFY_IS_NOT_NULL(topCommandBar);

                topCommandBar_OpenedRegistration.Attach(topCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[topCommandBar]: Opened Event Fired");
                    topCommandBar_OpenedEvent->Set();
                });

                topCommandBar_ClosedRegistration.Attach(topCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[topCommandBar]: Closed Event Fired");
                    topCommandBar_ClosedEvent->Set();
                });

                topAppBarBtn0 = safe_cast<AppBarButton^>(topCommandBar->PrimaryCommands->GetAt(0));
                VERIFY_IS_NOT_NULL(topAppBarBtn0);

                topAppBarBtn0_lostFocusRegistration.Attach(topAppBarBtn0, [&]()
                {
                    LOG_OUTPUT(L"[topAppBarBtn0]: Lost Focus Event Fired");
                    topAppBarBtn0_lostFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            CommonInputHelper::Accept(device); //Open overflow
            topCommandBar_OpenedEvent->WaitForDefault();

            CommonInputHelper::Up(device); //topOverflowAppBarBtn0->expandButton
            CommonInputHelper::Left(device); //expand button->topAppBarBtn1
            CommonInputHelper::Left(device); //topAppBarBtn0->topAppBarBtn0

            //Attempts to exit the trap
            CommonInputHelper::Left(device);
            TestServices::WindowHelper->WaitForIdle();
            // Make sure we're still on overflow button 0
            VERIFY_IS_FALSE(topAppBarBtn0_lostFocusEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(topAppBarBtn0->FocusState == FocusState::Keyboard);
            });

            CommonInputHelper::Cancel(device); //Close overflow
            topCommandBar_ClosedEvent->WaitForDefault();
        }

        //--------------------------------------------------------------------------------------
        // Test case: Verifies that once you enter a bottom commandbar overflow,
        //            you can exit it by pressing GamepadB when the focus is on an overflow item
        //--------------------------------------------------------------------------------------
        void CommandBarGamepadOrRemoteTests::ExitOverflow_BottomCommandBar()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);

            InputDevice device = InputDevice::Gamepad;

            auto page = safe_cast<Page^>(SetupTest(L"CommandBarGamepadOrRemoteTestScene.xaml"));
            xaml_controls::CommandBar^ bottomCommandBar = nullptr;
            Button^ bottomAppBarExpandButton = nullptr;
            auto bottomCommandBar_OpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            auto bottomCommandBar_OpenedEvent = std::make_shared<Event>();

            auto bottomCommandBar_ClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
            auto bottomCommandBar_ClosedEvent = std::make_shared<Event>();

            auto overflowbtn0_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowbtn0_gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                bottomCommandBar = safe_cast<xaml_controls::CommandBar^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"bottomCommandBar", page));
                VERIFY_IS_NOT_NULL(bottomCommandBar);

                bottomAppBarExpandButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(bottomCommandBar, L"MoreButton"));
                VERIFY_IS_NOT_NULL(bottomAppBarExpandButton);

                LOG_OUTPUT(L"Setting focus on bottomAppBarExpandButton");
                bottomAppBarExpandButton->Focus(FocusState::Keyboard);

                bottomCommandBar_OpenedRegistration.Attach(bottomCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Opened Event Fired");
                    bottomCommandBar_OpenedEvent->Set();
                });

                bottomCommandBar_ClosedRegistration.Attach(bottomCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Closed Event Fired");
                    bottomCommandBar_ClosedEvent->Set();
                });

                auto bottomOverflowAppBarBtn0 = safe_cast<AppBarButton^>(bottomCommandBar->SecondaryCommands->GetAt(0));
                VERIFY_IS_NOT_NULL(bottomOverflowAppBarBtn0);

                overflowbtn0_gotFocusRegistration.Attach(bottomOverflowAppBarBtn0, [&]()
                {
                    LOG_OUTPUT(L"[bottomOverflowAppBarBtn0]: Got Focus Event Fired");
                    overflowbtn0_gotFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            CommonInputHelper::Accept(device); //Open overflow
            bottomCommandBar_OpenedEvent->WaitForDefault();

            CommonInputHelper::Up(device); //bottomOverflowAppBarBtn0->expandButton
            CommonInputHelper::Up(device); //expandButton->bottomOverflowAppBarBtn1
            CommonInputHelper::Up(device); //bottomOverflowAppBarBtn1->bottomOverflowAppBarBtn0
            TestServices::WindowHelper->WaitForIdle();
            overflowbtn0_gotFocusEvent->WaitForDefault();

            CommonInputHelper::Cancel(device); //Close overflow
            bottomCommandBar_ClosedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(bottomAppBarExpandButton->FocusState == FocusState::Keyboard);
            });
        }

        //-----------------------------------------------------------------------------------------------
        // Test case: Verifies that selecting an item in overflow sets the focus on to the Expand Button
        //-----------------------------------------------------------------------------------------------
        void CommandBarGamepadOrRemoteTests::ExitOverflow_SelectingItemInOverflow()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);

            InputDevice device = InputDevice::Gamepad;

            auto page = safe_cast<Page^>(SetupTest(L"CommandBarGamepadOrRemoteTestScene.xaml"));
            xaml_controls::CommandBar^ bottomCommandBar = nullptr;
            Button^ bottomAppBarExpandButton = nullptr;
            auto bottomCommandBar_OpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            auto bottomCommandBar_OpenedEvent = std::make_shared<Event>();

            auto bottomCommandBar_ClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
            auto bottomCommandBar_ClosedEvent = std::make_shared<Event>();

            auto overflowbtn0_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowbtn0_gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                bottomCommandBar = safe_cast<xaml_controls::CommandBar^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"bottomCommandBar", page));
                VERIFY_IS_NOT_NULL(bottomCommandBar);

                bottomAppBarExpandButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(bottomCommandBar, L"MoreButton"));
                VERIFY_IS_NOT_NULL(bottomAppBarExpandButton);

                LOG_OUTPUT(L"Setting focus on bottomAppBarExpandButton");
                bottomAppBarExpandButton->Focus(FocusState::Keyboard);

                bottomCommandBar_OpenedRegistration.Attach(bottomCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Opened Event Fired");
                    bottomCommandBar_OpenedEvent->Set();
                });

                bottomCommandBar_ClosedRegistration.Attach(bottomCommandBar, [&]()
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Closed Event Fired");
                    bottomCommandBar_ClosedEvent->Set();
                });

                auto bottomOverflowAppBarBtn0 = safe_cast<AppBarButton^>(bottomCommandBar->SecondaryCommands->GetAt(0));
                VERIFY_IS_NOT_NULL(bottomOverflowAppBarBtn0);

                overflowbtn0_gotFocusRegistration.Attach(bottomOverflowAppBarBtn0, [&]()
                {
                    LOG_OUTPUT(L"[bottomOverflowAppBarBtn0]: Got Focus Event Fired");
                    overflowbtn0_gotFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            CommonInputHelper::Accept(device); //Open overflow
            bottomCommandBar_OpenedEvent->WaitForDefault();

            CommonInputHelper::Up(device); //bottomOverflowAppBarBtn0->expandButton
            CommonInputHelper::Up(device); //expandButton->bottomOverflowAppBarBtn1
            CommonInputHelper::Up(device); //bottomOverflowAppBarBtn1->bottomOverflowAppBarBtn0
            TestServices::WindowHelper->WaitForIdle();
            overflowbtn0_gotFocusEvent->WaitForDefault();

            CommonInputHelper::Accept(device); //Close the overflow by selecting bottomOverflowappBarBtn0
            bottomCommandBar_ClosedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(bottomAppBarExpandButton->FocusState == FocusState::Keyboard);
            });
        }

        void CommandBarGamepadOrRemoteTests::DoNotAllowFocusWrapInOverflow_TopCommandBar()
        {
            DoNotAllowFocusWrapInOverflow(/* isTopCommandBar */ true);
        }

        void CommandBarGamepadOrRemoteTests::DoNotAllowFocusWrapInOverflow_BottomCommandBar()
        {
            DoNotAllowFocusWrapInOverflow(/* isTopCommandBar */ false);
        }

        void CommandBarGamepadOrRemoteTests::DoNotAllowFocusWrapInOverflow(bool isTopCommandBar)
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);

            InputDevice device = InputDevice::Gamepad;

            Platform::String^ prefix = isTopCommandBar ? "top" : "bottom";
            xaml_controls::CommandBar^ commandBar = nullptr;
            Button^ expandButton = nullptr;
            xaml_primitives::ButtonBase^ overflowButton0 = nullptr;
            xaml_primitives::ButtonBase^ overflowButton1 = nullptr;

            auto page = safe_cast<Page^>(SetupTest(L"CommandBarGamepadOrRemoteTestScene.xaml"));

            auto commandBar_OpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            auto commandBar_OpenedEvent = std::make_shared<Event>();

            auto commandBar_ClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
            auto commandBar_ClosedEvent = std::make_shared<Event>();

            auto expandButton_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto expandButton_gotFocusEvent = std::make_shared<Event>();

            auto overflowButton0_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowButton0_gotFocusEvent = std::make_shared<Event>();

            auto overflowButton1_gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto overflowButton1_gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                commandBar = safe_cast<xaml_controls::CommandBar^>(page->FindName(prefix + "CommandBar"));
                commandBar_OpenedRegistration.Attach(commandBar, [&]()
                {
                    LOG_OUTPUT(L"[commandBar]: Opened Event Fired");
                    commandBar_OpenedEvent->Set();
                });
                commandBar_ClosedRegistration.Attach(commandBar, [&]()
                {
                    LOG_OUTPUT(L"[commandBar]: Closed Event Fired");
                    commandBar_ClosedEvent->Set();
                });

                expandButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(commandBar, L"MoreButton"));
                expandButton_gotFocusRegistration.Attach(expandButton, [&]()
                {
                    LOG_OUTPUT(L"[expandButton]: Got Focus Event Fired");
                    expandButton_gotFocusEvent->Set();
                });

                overflowButton0 = safe_cast<xaml_primitives::ButtonBase^>(commandBar->SecondaryCommands->GetAt(0));
                overflowButton0_gotFocusRegistration.Attach(overflowButton0, [&]()
                {
                    LOG_OUTPUT(L"[overflowButton0]: Got Focus Event Fired");
                    overflowButton0_gotFocusEvent->Set();
                });

                overflowButton1 = safe_cast<xaml_primitives::ButtonBase^>(commandBar->SecondaryCommands->GetAt(1));
                overflowButton1_gotFocusRegistration.Attach(overflowButton1, [&]()
                {
                    LOG_OUTPUT(L"[overflowButton1]: Got Focus Event Fired");
                    overflowButton1_gotFocusEvent->Set();
                });

                LOG_OUTPUT(L"Setting focus on expandButton");
                expandButton->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            expandButton_gotFocusEvent->Reset();

            CommonInputHelper::Accept(device); //Open overflow
            commandBar_OpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            if (isTopCommandBar)
            {
                overflowButton0_gotFocusEvent->WaitForDefault();
                VERIFY_ARE_EQUAL(overflowButton0_gotFocusEvent->TimesFired(), 1);

                CommonInputHelper::Up(device); //overflowButton0->MoreButton
                expandButton_gotFocusEvent->WaitForDefault();
                VERIFY_ARE_EQUAL(expandButton_gotFocusEvent->TimesFired(), 1);

                CommonInputHelper::Up(device); //Attempts to wrap around
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(expandButton));
                });

                CommonInputHelper::Down(device); //MoreButton->overflowButton0
                overflowButton0_gotFocusEvent->WaitForDefault();
                VERIFY_ARE_EQUAL(overflowButton0_gotFocusEvent->TimesFired(), 2);

                CommonInputHelper::Down(device); //overflowButton0->overflowButton1
                overflowButton1_gotFocusEvent->WaitForDefault();
                VERIFY_ARE_EQUAL(overflowButton1_gotFocusEvent->TimesFired(), 1);

                CommonInputHelper::Down(device); //Attempts to wrap around
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(overflowButton1));
                });
            }
            else
            {
                CommonInputHelper::Down(device); //overflowButton0->overflowButton1
                overflowButton1_gotFocusEvent->WaitForDefault();
                VERIFY_ARE_EQUAL(overflowButton1_gotFocusEvent->TimesFired(), 1);

                CommonInputHelper::Down(device); //overflowButton1->MoreButton
                expandButton_gotFocusEvent->WaitForDefault();
                VERIFY_ARE_EQUAL(expandButton_gotFocusEvent->TimesFired(), 1);

                CommonInputHelper::Down(device); //Attempts to wrap around
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(expandButton));
                });

                CommonInputHelper::Up(device); //MoreButton->overflowButton1
                overflowButton1_gotFocusEvent->WaitForDefault();
                VERIFY_ARE_EQUAL(overflowButton1_gotFocusEvent->TimesFired(), 2);

                CommonInputHelper::Up(device); //overflowButton1->overflowButton0
                overflowButton0_gotFocusEvent->WaitForDefault();
                VERIFY_ARE_EQUAL(overflowButton0_gotFocusEvent->TimesFired(), 1);

                CommonInputHelper::Up(device); //Attempts to wrap around
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(overflowButton0));
                });
            }

            CommonInputHelper::Accept(device); //Close Overflow
            commandBar_ClosedEvent->WaitForDefault();
        }

} } } } } }
