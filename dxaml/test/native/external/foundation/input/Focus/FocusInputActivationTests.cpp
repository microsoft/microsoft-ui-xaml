// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "FocusInputActivationTests.h"
#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <TestCleanupWrapper.h>
#include <TestEvent.h>
#include <FocusTestHelper.h>
#include <WUCRenderingScopeGuard.h>
#include <WindowAutoCloser.h>

using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::UI::ViewManagement;
using namespace ::Windows::Foundation;

using namespace test_infra;

namespace Microsoft::UI::Xaml::Tests {
    namespace Foundation::Input::Focus {

        bool FocusInputActivationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool FocusInputActivationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool FocusInputActivationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void FocusInputActivationTests::VerifyNoInputActivateRequestedFocusMoves()
        {
            TestCleanupWrapper cleanup;

            // window1
            WindowAutoCloser window1;

            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            ListView^ listview1 = nullptr;

            // window2
            WindowAutoCloser window2;
            Button^ win2btn1 = nullptr;

            RunOnUIThread([&]()
            {
                // window1
                auto rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button x:Name='btn1' Width='150' Height='50' Content='Button1'/>"
                    L"  <Button x:Name='btn2' Width='150' Height='50' Content='Button2'/>"
                    L"  <ListView x:Name='listview1'>"
                    L"      <ListViewItem Content='List Item 1' />"
                    L"      <ListViewItem Content='List Item 2' />"
                    L"      <ListViewItem Content='List Item 3' />"
                    L"  </ListView>"
                    L"</StackPanel>"));
                btn1 = safe_cast<Button^>(rootPanel->FindName(L"btn1"));
                btn2 = safe_cast<Button^>(rootPanel->FindName(L"btn2"));
                listview1 = safe_cast<ListView^>(rootPanel->FindName(L"listview1"));
                
                window1.Attach(ref new Window());
                window1->Title = "Window1";
                window1->Content = rootPanel;
                window1->Activate();

                // window2
                rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button x:Name='win2btn1' Width='150' Height='50' Content='Win2 Button1'/>"
                    L"</StackPanel>"));
                win2btn1 = safe_cast<Button^>(rootPanel->FindName(L"win2btn1"));

                window2.Attach(ref new Window());
                window2->Title = "Window2";
                window2->Content = rootPanel;
                window2->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();

            // Ensure default focus in each window.
            LOG_OUTPUT(L"---------- Setup ----------");
            LOG_OUTPUT(L"Setting default focus to btn1 in window1");
            FocusTestHelper::EnsureFocusInIsland(btn1, FocusState::Pointer);
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(btn1->XamlRoot)->Equals(btn1));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting default focus to win2btn1 in window2");
            FocusTestHelper::EnsureFocusInIsland(win2btn1, FocusState::Pointer);
            TestServices::WindowHelper->WaitForIdle();

            auto window1ActivatedRegistration = CreateSafeEventRegistration(Window, Activated);
            auto window2ActivatedRegistration = CreateSafeEventRegistration(Window, Activated);

            auto window1ActivatedEvent = std::make_shared<Event>();
            auto window2ActivatedEvent = std::make_shared<Event>();

            auto window1LastActivation = WindowActivationState::Deactivated;
            auto window2LastActivation = WindowActivationState::Deactivated;

            int window1ActivatedCount = 0;
            int window2ActivatedCount = 0;
            
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(btn1->XamlRoot)->Equals(btn1));
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(win2btn1->XamlRoot)->Equals(win2btn1));
                
                // Ensure window2 is activated
                LOG_OUTPUT(L"Activating window2");
                window2->Activate();
                window2LastActivation = WindowActivationState::CodeActivated;

                // Register for Activated events
                window1ActivatedRegistration.Attach(window1.get(),
                    ref new wf::TypedEventHandler<Platform::Object^, WindowActivatedEventArgs^>([&](Platform::Object^ sender, WindowActivatedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Window1 activated");
                    window1ActivatedCount++;
                    window1LastActivation = e->WindowActivationState;
                    window1ActivatedEvent->Set();
                }));
                window2ActivatedRegistration.Attach(window2.get(),
                    ref new wf::TypedEventHandler<Platform::Object^, WindowActivatedEventArgs^>([&](Platform::Object^ sender, WindowActivatedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Window2 activated");
                    window2ActivatedCount++;
                    window2LastActivation = e->WindowActivationState;
                    window2ActivatedEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            // --------------------------------------------------------------------------
            LOG_OUTPUT(L"-------- Collapse ---------");
            LOG_OUTPUT(L"Collapsing btn1, which should not activate window1");
            RunOnUIThread([&]()
            {
                window1ActivatedCount = 0;
                window2ActivatedCount = 0;
                btn1->Visibility = Visibility::Collapsed;
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(window1ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window1LastActivation, WindowActivationState::Deactivated);
            VERIFY_ARE_EQUAL(window2LastActivation, WindowActivationState::CodeActivated);

            // --------------------------------------------------------------------------
            LOG_OUTPUT(L"-------- Uncollapse -------");
            LOG_OUTPUT(L"Uncollapse btn1, which should not affect focus/activation");

            RunOnUIThread([&]()
            {
                window1ActivatedCount = 0;
                window2ActivatedCount = 0;
                btn1->Visibility = Visibility::Visible;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(FocusManager::GetFocusedElement(btn1->XamlRoot)->Equals(btn1));
            });

            VERIFY_ARE_EQUAL(window1ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window1LastActivation, WindowActivationState::Deactivated);
            VERIFY_ARE_EQUAL(window2LastActivation, WindowActivationState::CodeActivated);

            // --------------------------------------------------------------------------
            LOG_OUTPUT(L"------ Explicit Focus -----");
            LOG_OUTPUT(L"Explicitly focusing btn1 which SHOULD activate window1");
            RunOnUIThread([&]()
            {
                window1ActivatedCount = 0;
                window2ActivatedCount = 0;
                btn1->Focus(FocusState::Pointer);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(btn1->XamlRoot)->Equals(btn1));
            });

            VERIFY_ARE_EQUAL(window1ActivatedCount, 1);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 1);
            VERIFY_ARE_EQUAL(window1LastActivation, WindowActivationState::CodeActivated);
            VERIFY_ARE_EQUAL(window2LastActivation, WindowActivationState::Deactivated);

            // Move activation back to Window2
            LOG_OUTPUT(L"------ Activate Win2 ------");
            LOG_OUTPUT(L"Explicitly activate window2");
            RunOnUIThread([&]()
            {
                window1ActivatedCount = 0;
                window2ActivatedCount = 0;
                window2->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(window1ActivatedCount, 1);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 1);
            VERIFY_ARE_EQUAL(window1LastActivation, WindowActivationState::Deactivated);
            VERIFY_ARE_EQUAL(window2LastActivation, WindowActivationState::CodeActivated);
 
            // ------------------------ ListView tests ------------------------------------
            // Change selected index in listview1, which doesn't have focus and shouldn't change focus.
            LOG_OUTPUT(L"-------------------- ListView tests ---------------------");
            LOG_OUTPUT(L"--- Change ListView.SelectedIndex ---");
            RunOnUIThread([&]()
            {
                window1ActivatedCount = 0;
                window2ActivatedCount = 0;

                LOG_OUTPUT(L"Focus in window1 should still be on btn1");
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(listview1->XamlRoot)->Equals(btn1));

                LOG_OUTPUT(L"Setting SelectedIndex");
                listview1->SelectedIndex = 1;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"After SelectedIndex change, btn1 should still have focus");
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(listview1->XamlRoot)->Equals(btn1));
            });
            VERIFY_ARE_EQUAL(window1ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 0);

            LOG_OUTPUT(L"--- Set focus to listview1, which will activate ---");
            RunOnUIThread([&]()
            {
                listview1->Focus(FocusState::Pointer);
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(window1ActivatedCount, 1);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 1);
            VERIFY_ARE_EQUAL(window1LastActivation, WindowActivationState::CodeActivated);
            VERIFY_ARE_EQUAL(window2LastActivation, WindowActivationState::Deactivated);

            Platform::Object^ lastFocus = nullptr;
            RunOnUIThread([&]()
            {
                window1ActivatedCount = 0;
                window2ActivatedCount = 0;

                LOG_OUTPUT(L"Focus should no longer be on btn1");
                VERIFY_IS_FALSE(FocusManager::GetFocusedElement(btn1->XamlRoot)->Equals(btn1));
                lastFocus = FocusManager::GetFocusedElement(btn1->XamlRoot);
                VERIFY_IS_NOT_NULL(lastFocus);

                LOG_OUTPUT(L"Reactivate window2");
                window2->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(window1ActivatedCount, 1);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 1);
            VERIFY_ARE_EQUAL(window1LastActivation, WindowActivationState::Deactivated);
            VERIFY_ARE_EQUAL(window2LastActivation, WindowActivationState::CodeActivated);

            LOG_OUTPUT(L"--- Change ListView.SelectedIndex ---");
            LOG_OUTPUT(L"Selecting a diferent item, which should move focus but not activate window1");
            RunOnUIThread([&]()
            {
                window1ActivatedCount = 0;
                window2ActivatedCount = 0;

                VERIFY_ARE_EQUAL(listview1->SelectedIndex, 1); // should still be on 1

                LOG_OUTPUT(L"Setting SelectedIndex");
                listview1->SelectedIndex = 2;
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(window1ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 0);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Ensuring focus moved");
                VERIFY_IS_FALSE(FocusManager::GetFocusedElement(listview1->XamlRoot)->Equals(lastFocus));
            });

            LOG_OUTPUT(L"--- Enable a control in an island without a focused element ---");
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Activating window1");
                window1->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()            
            {
                window1ActivatedCount = 0;
                window2ActivatedCount = 0;

                LOG_OUTPUT(L"Disabling win2btn1.");
                win2btn1->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(window1ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window1LastActivation, WindowActivationState::CodeActivated);
            VERIFY_ARE_EQUAL(window2LastActivation, WindowActivationState::Deactivated);

            RunOnUIThread([&]()            
            {
                window1ActivatedCount = 0;
                window2ActivatedCount = 0;

                VERIFY_ARE_EQUAL(FocusManager::GetFocusedElement(win2btn1->XamlRoot), nullptr);

                LOG_OUTPUT(L"Enabling win2btn1, it should automatically get focus -- but the HWND shouldn't get focus/activation.");
                win2btn1->IsEnabled = true;

                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(win2btn1->XamlRoot)->Equals(win2btn1));
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(window1ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window2ActivatedCount, 0);
            VERIFY_ARE_EQUAL(window1LastActivation, WindowActivationState::CodeActivated);
            VERIFY_ARE_EQUAL(window2LastActivation, WindowActivationState::Deactivated);

        }

    }
}
