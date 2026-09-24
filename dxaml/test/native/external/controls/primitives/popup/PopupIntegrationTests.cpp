// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PopupIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationClientInitializer.h>
#include <UIAutomationHelper.h>
#include <Patterns\InvokePatternHandler.h>
#include <ControlHelper.h>
#include <CommonInputHelper.h>
#include <WUCRenderingScopeGuard.h>
#include <HolographicOverride.h>
#include <WindowsNumerics.h>
#include "ChangeDPI.h"
#include <PopupHelper.h>

using namespace ::Windows::Storage::Streams;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace MockDComp;
using namespace Concurrency;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace ::Windows::UI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Popup {

    bool PopupIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PopupIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PopupIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void PopupIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_primitives::Popup>::CanInstantiate();
    }

    void PopupIntegrationTests::CanPopupOpenAndClose()
    {
        TestCleanupWrapper cleanup;
        xaml_primitives::Popup^ popup1 = nullptr;

        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupClosedEvent = std::make_shared<Event>();

        auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]() {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='500' Height='700' > \r\n"
                L"  <Button x:Name='button1' Content='OpenClosePopup' /> \r\n"
                L"      <Popup x:Name='popup1' VerticalOffset='1' HorizontalOffset='1' IsOpen='false' Height='200' Width='500'> \r\n"
                L"          <StackPanel Background='SlateBlue' Height='200' Width='500'> \r\n"
                L"              <TextBlock Text='popup1(no light dismiss)'></TextBlock> \r\n"
                L"          </StackPanel> \r\n"
                L"      </Popup> \r\n"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;

            popup1 = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup1"));
            VERIFY_IS_NOT_NULL(popup1);

            openedRegistration.Attach(popup1, ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"PopupOpenClose: Popup Opened event is fired!");
                popupOpenedEvent->Set();
            }));

            closedRegistration.Attach(popup1, ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"PopupOpenClose: Popup Closed event is fired!");
                popupClosedEvent->Set();
            }));

            LOG_OUTPUT(L"PopupOpenClose: Set Popup1 IsOpen=TRUE");
            popup1->IsOpen = true;
        });

        popupOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(popup1->IsOpen);
            LOG_OUTPUT(L"PopupOpenClose: Set Popup1 IsOpen=FALSE");
            popup1->IsOpen = false;
        });

        popupClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(popup1->IsOpen);
        });
    }

    void PopupIntegrationTests::PopupCloseDoesNotClearIsOpenBinding()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        xaml_primitives::Popup^ popup1 = nullptr;

        auto popupLoadedEvent = std::make_shared<Event>();
        auto popupLoadedEventRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Loaded);
        auto popupClosedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='500' Height='700' > \r\n"
                L"  <Button x:Name='button1' Content='test button' /> \r\n"
                L"  <Popup x:Name='popup1' VerticalOffset='1' HorizontalOffset='1' DataContext='true' IsOpen='{Binding}' Height='200' Width='500' IsLightDismissEnabled='true'> \r\n"
                L"      <StackPanel Background='SlateBlue' Height='200' Width='500'> \r\n"
                L"          <TextBlock Text='popup1(no light dismiss)'></TextBlock> \r\n"
                L"      </StackPanel> \r\n"
                L"  </Popup> \r\n"
                L"</StackPanel>"));

            button1 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            popup1 = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup1"));

            popupLoadedEventRegistration.Attach(popup1, [popupLoadedEvent]()
            {
                LOG_OUTPUT(L"Popup loaded.");
                popupLoadedEvent->Set();
            });

            closedRegistration.Attach(popup1, ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"Popup closed.");
                popupClosedEvent->Set();
            }));

            // Verify initial state.
            VERIFY_IS_TRUE(popup1->IsOpen);
            VERIFY_IS_TRUE(popup1->IsLightDismissEnabled);
            VERIFY_IS_NOT_NULL(popup1->GetBindingExpression(xaml_primitives::Popup::IsOpenProperty));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        popupLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto ap = dynamic_cast<Microsoft::UI::Xaml::Automation::Provider::IWindowProvider^>(
                xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(popup1));
            ap->Close();
        });

        popupClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            // Verify the binding is still there.
            VERIFY_IS_NOT_NULL(popup1->GetBindingExpression(xaml_primitives::Popup::IsOpenProperty));
        });
    }

    void PopupIntegrationTests::DoesNotChainRightClickWithOpenLightDismissPopup()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::Popup^ popup = nullptr;
        xaml_controls::Button^ target = nullptr;

        auto targetRightTappedRegistration = CreateSafeEventRegistration(xaml_controls::Button, RightTapped);
        bool wasTargetRightTapped = false;

        RunOnUIThread([&]()
        {
            target = ref new xaml_controls::Button();
            target->Content = "Target";

            targetRightTappedRegistration.Attach(target, [&]() { wasTargetRightTapped = true; });

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(target);

            TestServices::WindowHelper->WindowContent = root;

            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;

            popup = ref new xaml_primitives::Popup();            
            popup->Child = rect;
            popup->IsLightDismissEnabled = true;
            popup->XamlRoot = root->XamlRoot;
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Inject right-click.
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, target);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            WEX::Common::Throw::If(popup->IsOpen, E_FAIL, L"The popup should have light-dismissed.");

            // The right-click should not have been chained, so the target's handler should not have
            // been invoked.
            VERIFY_IS_FALSE(wasTargetRightTapped);
        });
    }

    void PopupIntegrationTests::ReplayPointerUpdate_PopupClosedDuringReplay_DoesNotCrash()
    {
        // Original 2-popup repro (popupA + popupB).  Snapshot fits within the inline
        // stack_vector capacity (typicalOpenPopupCount = 4) so the inline arena is
        // exercised, not the heap-allocated path.
        ReplayPointerUpdate_PopupClosedDuringReplay_DoesNotCrashHelper(0 /* extraOpenPopupCount */);
    }

    void PopupIntegrationTests::ReplayPointerUpdate_PopupClosedDuringReplay_WithManyOpenPopups_DoesNotCrash()
    {
        // 5 extra dummy popups + popupA + popupB = 7 total open popups when
        // CPopupRoot::ReplayPointerUpdate iterates, exceeding typicalOpenPopupCount = 4
        // and forcing the snapshot Jupiter::stack_vector to spill from its inline
        // arena to the heap.  Exercises the same reentrant-close UAF path on the
        // heap-allocated snapshot.
        ReplayPointerUpdate_PopupClosedDuringReplay_DoesNotCrashHelper(5 /* extraOpenPopupCount */);
    }

    void PopupIntegrationTests::ReplayPointerUpdate_PopupClosedDuringReplay_DoesNotCrashHelper(size_t extraOpenPopupCount)
    {
        // The bug: CPopupRoot::ReplayPointerUpdate iterates m_pOpenPopups and calls
        // CPopup::ReplayPointerUpdate per-popup, which delivers a synthesized pointer event.
        // App-side handlers may synchronously close *other* popups during that delivery
        // (light-dismiss, cascading menus, focus-loss handlers, etc.).  Closing a popup runs
        // RemoveFromOpenPopupList -> CXcpList::Remove, which deletes the XCPListNode the outer
        // iterator is referencing -> use-after-free on the next pNode = pNode->m_pNext step.
        //
        // The repro: open popupA, hover the mouse over its content, then open popupB at the
        // same location.  Opening popupB triggers a layout-driven ReplayPreviousPointerUpdate.
        // popupB is at the head of m_pOpenPopups (CXcpList::Add head-prepends, so head = most
        // recently opened) so its replay fires first; buttonB's PointerEntered handler
        // synchronously closes popupA, which frees popupA's XCPListNode -- the same node the
        // outer iterator is about to advance to.
        //
        // When extraOpenPopupCount > 0, additional dummy popups are opened *before* popupA so
        // they sit later in the head-prepend iteration order (after popupA).  They have no
        // PointerEntered handlers and sit far outside the popupA/popupB hit-test zone, so
        // their CPopup::ReplayPointerUpdate calls are no-ops (they never received a pointer
        // message, so WindowedPopupInputSiteAdapter::ReplayPointerUpdate early-returns when
        // m_previousPointerPoint is null).  Their only role is to inflate m_pOpenPopups: when
        // (extraOpenPopupCount + 2) > typicalOpenPopupCount the snapshot Jupiter::stack_vector
        // exceeds its inline arena capacity and spills to the heap.
        TestCleanupWrapper cleanup;

        xaml_primitives::Popup^ popupA = nullptr;
        xaml_primitives::Popup^ popupB = nullptr;
        xaml_controls::Button^ buttonA = nullptr;
        xaml_controls::Button^ buttonB = nullptr;
        std::vector<xaml_primitives::Popup^> dummyPopups(extraOpenPopupCount, nullptr);

        auto popupAOpenedEvent = std::make_shared<Event>();
        auto popupBOpenedEvent = std::make_shared<Event>();
        auto popupAClosedEvent = std::make_shared<Event>();

        auto popupAOpenedReg = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupBOpenedReg = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupAClosedReg = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);
        auto buttonBPointerEnteredReg = CreateSafeEventRegistration(xaml_controls::Button, PointerEntered);

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::Grid();
            root->Width = 600;
            root->Height = 600;
            root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);

            buttonA = ref new xaml_controls::Button();
            buttonA->Content = "A";
            buttonA->Width = 200;
            buttonA->Height = 200;

            buttonB = ref new xaml_controls::Button();
            buttonB->Content = "B";
            buttonB->Width = 200;
            buttonB->Height = 200;

            popupA = ref new xaml_primitives::Popup();
            popupA->Child = buttonA;
            popupA->VerticalOffset = 10;
            popupA->HorizontalOffset = 10;
            popupA->IsLightDismissEnabled = true;

            // popupB sits over the same logical region as popupA so that the still-hot
            // pointer hits buttonB the moment popupB opens.
            popupB = ref new xaml_primitives::Popup();
            popupB->Child = buttonB;
            popupB->VerticalOffset = 10;
            popupB->HorizontalOffset = 10;
            popupB->IsLightDismissEnabled = true;

            // Dummy popups: a 1x1 Rectangle child at offset (400, 400), well outside the
            // (10, 10)..(210, 210) buttonA/buttonB hit-test zone.  Non-light-dismiss so
            // they stay open through the scenario and remain in m_pOpenPopups when
            // CPopupRoot::ReplayPointerUpdate iterates.
            for (size_t i = 0; i < dummyPopups.size(); ++i)
            {
                auto dummyChild = ref new xaml_shapes::Rectangle();
                dummyChild->Width = 1;
                dummyChild->Height = 1;

                auto dummy = ref new xaml_primitives::Popup();
                dummy->Child = dummyChild;
                dummy->VerticalOffset = 400;
                dummy->HorizontalOffset = 400;
                dummy->IsLightDismissEnabled = false;
                dummyPopups[i] = dummy;
            }

            popupAOpenedReg.Attach(popupA, ref new wf::EventHandler<Platform::Object^>(
                [popupAOpenedEvent](Platform::Object^, Platform::Object^) { popupAOpenedEvent->Set(); }));
            popupBOpenedReg.Attach(popupB, ref new wf::EventHandler<Platform::Object^>(
                [popupBOpenedEvent](Platform::Object^, Platform::Object^) { popupBOpenedEvent->Set(); }));
            popupAClosedReg.Attach(popupA, ref new wf::EventHandler<Platform::Object^>(
                [popupAClosedEvent](Platform::Object^, Platform::Object^) { popupAClosedEvent->Set(); }));

            TestServices::WindowHelper->WindowContent = root;

            for (auto& dummy : dummyPopups)
            {
                dummy->XamlRoot = root->XamlRoot;
            }
            popupA->XamlRoot = root->XamlRoot;
            popupB->XamlRoot = root->XamlRoot;

            // Open dummies first so they're prepended to m_pOpenPopups before popupA.
            // CXcpList::Add prepends at head, so post-open list order (head -> tail) becomes:
            //     popupA, dummyN, dummyN-1, ..., dummy1
            // After popupB opens later (head = most recent), it becomes:
            //     popupB, popupA, dummyN, ..., dummy1
            // i.e. popupA remains the next-iterated node after popupB.
            for (auto& dummy : dummyPopups)
            {
                dummy->IsOpen = true;
            }

            popupA->IsOpen = true;
        });

        popupAOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Move the mouse over popupA's button so the input system caches a "previous pointer"
        // state that the subsequent layout-driven replay will use.
        TestServices::InputHelper->MoveMouse(buttonA);
        TestServices::WindowHelper->WaitForIdle();

        // Arm the reentrancy bomb on buttonB.  When buttonB receives PointerEntered during
        // popupB's CPopup::ReplayPointerUpdate (head of m_pOpenPopups), close popupA
        // synchronously.  popupA is the *next* node the outer iterator will advance to;
        // closing it here frees that node's XCPListNode mid-iteration on unfixed code.
        RunOnUIThread([&]()
        {
            buttonBPointerEnteredReg.Attach(buttonB,
                ref new xaml_input::PointerEventHandler(
                    [&popupA](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"buttonB PointerEntered fired during replay; closing popupA synchronously.");
                        if (popupA != nullptr)
                        {
                            popupA->IsOpen = false;
                        }
                    }));
        });

        // Open popupB while the pointer is still hot.  Opening popupB mutates the visual tree
        // under the hot pointer; the input pipeline responds with a
        // CCoreServices::ReplayPreviousPointerUpdate that routes through
        // CXamlIslandRoot::ReplayPointerUpdate -> CPopupRoot::ReplayPointerUpdate.
        RunOnUIThread([&]()
        {
            popupB->IsOpen = true;
        });

        popupBOpenedEvent->WaitForDefault();
        popupAClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // The whole point of this test: we should reach this line at all (no AV).
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(popupA->IsOpen);
            VERIFY_IS_TRUE(popupB->IsOpen);

            // Dummies were strong-ref'd in the snapshot, then their ReplayPointerUpdate ran
            // as a no-op (no cached pointer message, so WindowedPopupInputSiteAdapter's
            // early-return path was hit).  Verify they remained open through the iteration,
            // which also confirms no cascading close happened during the replay.
            for (auto& dummy : dummyPopups)
            {
                VERIFY_IS_TRUE(dummy->IsOpen);
            }
        });
    }

    void PopupIntegrationTests::PopupTabStop()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::Button^ button2 = nullptr;
        xaml_controls::Button^ button3 = nullptr;
        xaml_primitives::Popup^ popup1 = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel HorizontalAlignment='Center' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='500' Height='700' > \r\n"
                L" <Popup x:Name='popup1' IsOpen='False' IsLightDismissEnabled='True'> \r\n"
                L"      <StackPanel> \r\n"
                L"          <Button x:Name='button1' Background='Green' Width='100' Height='100'/> \r\n"
                L"          <Button x:Name='button2' Background='Red' Width='100' Height='100'/> \r\n"
                L"          <Button x:Name='button3' Background='Blue' Width='100' Height='100'/> \r\n"
                L"      </StackPanel> \r\n"
                L"  </Popup> \r\n"
                L"</StackPanel>"));

            button1 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            button2 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
            button3 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button3"));
            popup1 = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup1"));
            VERIFY_IS_NOT_NULL(button1);
            VERIFY_IS_NOT_NULL(button2);
            VERIFY_IS_NOT_NULL(button3);
            VERIFY_IS_NOT_NULL(popup1);

            // Verify initial state.
            VERIFY_IS_TRUE(popup1->IsLightDismissEnabled);
            TestServices::WindowHelper->WindowContent = rootPanel;

            popup1->IsOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        // press tab key to move focus to the first button.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Focus should be on the first button now.
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button1));
        });

        TestServices::WindowHelper->WaitForIdle();

        // press tab key to move focus to the second button.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Focus should be on the second button now.
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button2));
        });
        TestServices::WindowHelper->WaitForIdle();

        // press tab key to move focus to the third button.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Focus should be on the third button now.
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button3));
        });

        TestServices::WindowHelper->WaitForIdle();

        // press shift tab key to move focus to the second button.
        TestServices::KeyboardHelper->ShiftTab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Focus should be on the second button now.
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button2));
        });
    }

    void PopupIntegrationTests::DoNotSetDefaultFocusOnPhoneWhenPopupIsClosed()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::Button^ button2 = nullptr;
        xaml_primitives::Popup^ popup1 = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel HorizontalAlignment='Center' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='500' Height='700' > \r\n"
                L" <Popup x:Name='popup1' IsOpen='False' IsLightDismissEnabled='True'> \r\n"
                L"      <StackPanel> \r\n"
                L"          <Button x:Name='button1' Background='Green' Width='100' Height='100'/> \r\n"
                L"      </StackPanel> \r\n"
                L"  </Popup> \r\n"
                L"  <Button x:Name='button2'/> \r\n"
                L"</StackPanel>"));

            button1 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            button2 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
            popup1 = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup1"));
            VERIFY_IS_NOT_NULL(button1);
            VERIFY_IS_NOT_NULL(button2);
            VERIFY_IS_NOT_NULL(popup1);

            // Verify initial state.
            VERIFY_IS_TRUE(popup1->IsLightDismissEnabled);
            TestServices::WindowHelper->WindowContent = rootPanel;

            popup1->IsOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        // press tab key to move focus to the first button.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Focus should be on the first button now.
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button1));
        });

        TestServices::WindowHelper->WaitForIdle();

        // close the popup.
        RunOnUIThread([&]()
        {
            popup1->IsOpen = false;
        });

        TestServices::WindowHelper->WaitForIdle();

        // On phone nothing will be focused when popup is closed.
        // On anywhere else, focus manager will focus the next focusable element (button2).
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button2));
        });
    }

    void PopupIntegrationTests::ChangeOffsetWhenShown()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        xaml_primitives::Popup^ popup1 = nullptr;
        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel HorizontalAlignment='Center' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='500' Height='700' > \r\n"
                L" <Popup x:Name='popup1' IsOpen='True' IsLightDismissEnabled='True'> \r\n"
                L"      <StackPanel> \r\n"
                L"          <Button x:Name='button1' Background='Green' Width='100' Height='100'/> \r\n"
                L"      </StackPanel> \r\n"
                L"  </Popup> \r\n"
                L"  <Button x:Name='button2'/> \r\n"
                L"</StackPanel>"));
            popup1 = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup1"));
            VERIFY_IS_NOT_NULL(popup1);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup1->HorizontalOffset = 222;
            popup1->VerticalOffset = 111;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Focus should be on the first button now.
            VERIFY_IS_TRUE(popup1->HorizontalOffset == 222);
            VERIFY_IS_TRUE(popup1->VerticalOffset == 111);
        });
    }

    void PopupIntegrationTests::SetSameChild()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        xaml_primitives::Popup^ popup1 = nullptr;
        xaml_controls::Button^ button1 = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel HorizontalAlignment='Center' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='500' Height='700' > \r\n"
                L" <Popup x:Name='popup1' IsOpen='True' IsLightDismissEnabled='True'> \r\n"
                L"      <Button x:Name='button1' Background='Green' Width='100' Height='100'/> \r\n"
                L"  </Popup> \r\n"
                L"</StackPanel>"));
            popup1 = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup1"));
            VERIFY_IS_NOT_NULL(popup1);
            button1 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            VERIFY_IS_NOT_NULL(button1);

            VERIFY_IS_TRUE(popup1->Child == button1);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup1->Child = nullptr;
            VERIFY_IS_NULL(popup1->Child);
            popup1->Child = button1;
            VERIFY_IS_TRUE(popup1->Child == button1);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    // Verify windowed popup's open and close
    void PopupIntegrationTests::WindowedPopupOpenAndClose()
    {
        WindowedPopupOpenAndCloseHelper();
    }

    void PopupIntegrationTests::PopupInHolographicModeOpenAndClose()
    {
        HolographicOverride holographicOverride;
        WindowedPopupOpenAndCloseHelper();
    }

    void PopupIntegrationTests::WindowedPopupParentCanvasCompNode()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_primitives::Popup^ popup;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='400' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                L" <Canvas CompositeMode='SourceOver' Translation='50.0,50.0,0.0'>"
                L"  <Canvas Translation='75.0,75.0,0.0'>"
                L"   <Popup x:Name='myPopup' HorizontalOffset='-10' VerticalOffset='-10'>"
                L"    <Rectangle Width='100' Height='100' Fill='Red'/>"
                L"   </Popup>"
                L"  </Canvas>"
                L" </Canvas>"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"myPopup"));
            VERIFY_IS_NOT_NULL(popup);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->Popup_SetWindowed(popup);
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void PopupIntegrationTests::PopupCycleDoesntInfiniteLoop()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel HorizontalAlignment='Center' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='500' Height='700' > \r\n"
                L"</StackPanel>"));
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::StackPanel^ sp;
        xaml_primitives::Popup^ p;
        RunOnUIThread([&]()
        {
            // Make a cycle.  This should probably error out, but it doesn't.  In late 19h1 we discovered that the
            // Skype app creates a cycle like this, and XAML gets stuck in an infinite loop.  For this test, we
            // just validate that XAML doesn't freeze up.
            sp = ref new xaml_controls::StackPanel;
            p = ref new xaml_primitives::Popup;
            p->Child = sp;
            sp->Children->Append(p);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Break the cycle to avoid a leak
            p->Child = nullptr;
            sp->Children->Clear();
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void PopupIntegrationTests::TallPopupPlacedRelativeToAnotherPopupIsBroughtInView()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        auto popupActualPlacementChangedEvent = std::make_shared<Event>();
        auto popupActualPlacementChangedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, ActualPlacementChanged);

        xaml_controls::Grid^ rootGrid;

        RunOnUIThread([&]()
        {
            rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_primitives::Popup^ absolutelyPlacedPopup;
        xaml_primitives::Popup^ relativelyPlacedPopup;

        xaml_controls::Border^ absolutelyPlacedPopupChild;

        RunOnUIThread([&]()
        {
            absolutelyPlacedPopup = ref new xaml_primitives::Popup();
            relativelyPlacedPopup = ref new xaml_primitives::Popup();

            absolutelyPlacedPopup->XamlRoot = rootGrid->XamlRoot;
            relativelyPlacedPopup->XamlRoot = rootGrid->XamlRoot;

            absolutelyPlacedPopup->ShouldConstrainToRootBounds = true;
            relativelyPlacedPopup->ShouldConstrainToRootBounds = true;

            auto textBlock1 = ref new xaml_controls::TextBlock();
            textBlock1->Text = L"Absolutely placed popup";
            absolutelyPlacedPopupChild = ref new xaml_controls::Border();
            absolutelyPlacedPopupChild->Height = 100;
            absolutelyPlacedPopupChild->Width = 100;
            absolutelyPlacedPopupChild->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
            absolutelyPlacedPopupChild->Child = textBlock1;
            absolutelyPlacedPopup->Child = absolutelyPlacedPopupChild;

            auto textBlock2 = ref new xaml_controls::TextBlock();
            textBlock2->Text = L"Relatively placed popup";
            auto relativelyPlacedPopupChild = ref new xaml_controls::Border();
            relativelyPlacedPopupChild->Height = 175;
            relativelyPlacedPopupChild->Width = 175;
            relativelyPlacedPopupChild->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);
            relativelyPlacedPopupChild->Child = textBlock2;
            relativelyPlacedPopup->Child = relativelyPlacedPopupChild;

            absolutelyPlacedPopup->HorizontalOffset = 150;
            absolutelyPlacedPopup->VerticalOffset = 150;

            relativelyPlacedPopup->PlacementTarget = absolutelyPlacedPopupChild;

            popupActualPlacementChangedRegistration.Attach(relativelyPlacedPopup, ref new wf::EventHandler<Platform::Object^>([popupActualPlacementChangedEvent](Platform::Object^, Platform::Object^)
            {
                popupActualPlacementChangedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Open the popups with the relatively-placed popup above the absolutely-placed popup.");
            relativelyPlacedPopup->DesiredPlacement = xaml_primitives::PopupPlacementMode::Top;
            absolutelyPlacedPopup->IsOpen = true;
            relativelyPlacedPopup->IsOpen = true;
        });

        popupActualPlacementChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The absolutely-placed popup should've been pushed down to accommodate the relatively-placed popup.");

            auto bounds = ControlHelper::GetBounds(absolutelyPlacedPopupChild);
            LOG_OUTPUT(L"Absolute popup bounds: (X, Y, Width, Height) = (%.0f, %.0f, %.0f, %.0f)", bounds.X, bounds.Y, bounds.Width, bounds.Height);

            VERIFY_IS_TRUE(ControlHelper::AreClose(bounds.Y, 175, 5));
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Flip the desired placement of the relatively placed popup to the bottom.");
            relativelyPlacedPopup->DesiredPlacement = xaml_primitives::PopupPlacementMode::Bottom;
        });

        popupActualPlacementChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The absolutely-placed popup should've been pushed up to accommodate the relatively-placed popup.");

            auto bounds = ControlHelper::GetBounds(absolutelyPlacedPopupChild);
            LOG_OUTPUT(L"Absolute popup bounds: (X, Y, Width, Height) = (%.0f, %.0f, %.0f, %.0f)", bounds.X, bounds.Y, bounds.Width, bounds.Height);

            VERIFY_IS_TRUE(ControlHelper::AreClose(bounds.Y, 125, 5));
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Now move the relatively placed popup to the left.");
            relativelyPlacedPopup->DesiredPlacement = xaml_primitives::PopupPlacementMode::Left;
        });

        popupActualPlacementChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The absolutely-placed popup should've been pushed to the right to accommodate the relatively-placed popup.");

            auto bounds = ControlHelper::GetBounds(absolutelyPlacedPopupChild);
            LOG_OUTPUT(L"Absolute popup bounds: (X, Y, Width, Height) = (%.0f, %.0f, %.0f, %.0f)", bounds.X, bounds.Y, bounds.Width, bounds.Height);

            VERIFY_IS_TRUE(ControlHelper::AreClose(bounds.X, 175, 5));
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Flip the desired placement of the relatively placed popup to the right.");
            relativelyPlacedPopup->DesiredPlacement = xaml_primitives::PopupPlacementMode::Right;
        });

        popupActualPlacementChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The absolutely-placed popup should've been pushed to the left to accommodate the relatively-placed popup.");

            auto bounds = ControlHelper::GetBounds(absolutelyPlacedPopupChild);
            LOG_OUTPUT(L"Absolute popup bounds: (X, Y, Width, Height) = (%.0f, %.0f, %.0f, %.0f)", bounds.X, bounds.Y, bounds.Width, bounds.Height);

            VERIFY_IS_TRUE(ControlHelper::AreClose(bounds.X, 125, 5));
        });

        RunOnUIThread([&]()
        {
            absolutelyPlacedPopup->IsOpen = false;
            relativelyPlacedPopup->IsOpen = false;
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PopupIntegrationTests::PopupSizeIncreaseDoesntChangePosition()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        auto popupActualPlacementChangedEvent = std::make_shared<Event>();
        auto popupActualPlacementChangedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, ActualPlacementChanged);

        auto popupChildLayoutUpdatedEvent = std::make_shared<Event>();
        auto popupChildLayoutUpdatedRegistration = CreateSafeEventRegistration(xaml_controls::Border, LayoutUpdated);

        xaml_controls::Grid^ rootGrid;

        RunOnUIThread([&]()
        {
            rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_primitives::Popup^ absolutelyPlacedPopup;
        xaml_primitives::Popup^ relativelyPlacedPopup;

        xaml_controls::Border^ absolutelyPlacedPopupChild;
        xaml_controls::Border^ relativelyPlacedPopupChild;

        RunOnUIThread([&]()
        {
            absolutelyPlacedPopup = ref new xaml_primitives::Popup();
            relativelyPlacedPopup = ref new xaml_primitives::Popup();

            absolutelyPlacedPopup->XamlRoot = rootGrid->XamlRoot;
            relativelyPlacedPopup->XamlRoot = rootGrid->XamlRoot;

            absolutelyPlacedPopup->ShouldConstrainToRootBounds = true;
            relativelyPlacedPopup->ShouldConstrainToRootBounds = true;

            auto textBlock1 = ref new xaml_controls::TextBlock();
            textBlock1->Text = L"Absolutely placed popup";
            absolutelyPlacedPopupChild = ref new xaml_controls::Border();
            absolutelyPlacedPopupChild->Height = 100;
            absolutelyPlacedPopupChild->Width = 100;
            absolutelyPlacedPopupChild->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
            absolutelyPlacedPopupChild->Child = textBlock1;
            absolutelyPlacedPopup->Child = absolutelyPlacedPopupChild;

            auto textBlock2 = ref new xaml_controls::TextBlock();
            textBlock2->Text = L"Relatively placed popup";
            relativelyPlacedPopupChild = ref new xaml_controls::Border();
            relativelyPlacedPopupChild->Height = 50;
            relativelyPlacedPopupChild->Width = 50;
            relativelyPlacedPopupChild->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);
            relativelyPlacedPopupChild->Child = textBlock2;
            relativelyPlacedPopup->Child = relativelyPlacedPopupChild;

            absolutelyPlacedPopup->HorizontalOffset = 100;
            absolutelyPlacedPopup->VerticalOffset = 100;

            relativelyPlacedPopup->PlacementTarget = absolutelyPlacedPopupChild;

            popupChildLayoutUpdatedRegistration.Attach(absolutelyPlacedPopupChild, ref new wf::EventHandler<Platform::Object^>([popupChildLayoutUpdatedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"Popup.Child.LayoutUpdated raised.");
                popupChildLayoutUpdatedEvent->Set();
            }));

            popupActualPlacementChangedRegistration.Attach(relativelyPlacedPopup, ref new wf::EventHandler<Platform::Object^>([popupActualPlacementChangedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"Popup.ActualPlacementChanged raised.");
                popupActualPlacementChangedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Open the popups with the relatively-placed popup above the absolutely-placed popup.");
            relativelyPlacedPopup->DesiredPlacement = xaml_primitives::PopupPlacementMode::Top;
            absolutelyPlacedPopup->IsOpen = true;
            relativelyPlacedPopup->IsOpen = true;
        });

        popupActualPlacementChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The relatively-placed popup should be above the absolutely-placed popup.");

            auto absolutelyPlacedPopupBounds = ControlHelper::GetBounds(absolutelyPlacedPopupChild);
            LOG_OUTPUT(L"Absolute popup bounds: (X, Y, Width, Height) = (%.0f, %.0f, %.0f, %.0f)", absolutelyPlacedPopupBounds.X, absolutelyPlacedPopupBounds.Y, absolutelyPlacedPopupBounds.Width, absolutelyPlacedPopupBounds.Height);

            auto relativelyPlacedPopupBounds = ControlHelper::GetBounds(relativelyPlacedPopupChild);
            LOG_OUTPUT(L"Relative popup bounds: (X, Y, Width, Height) = (%.0f, %.0f, %.0f, %.0f)", relativelyPlacedPopupBounds.X, relativelyPlacedPopupBounds.Y, relativelyPlacedPopupBounds.Width, relativelyPlacedPopupBounds.Height);

            VERIFY_IS_LESS_THAN(relativelyPlacedPopupBounds.Y, absolutelyPlacedPopupBounds.Y);
        });

        popupChildLayoutUpdatedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Make the relatively-placed popup no longer fit above the absolutely-placed popup.");
            relativelyPlacedPopupChild->Height = 150;
        });

        popupChildLayoutUpdatedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The relatively-placed popup should still be above the absolutely-placed popup even though it now fits below it. Instead, the absolutely-placed popup should be pushed down.");

            auto absolutelyPlacedPopupBounds = ControlHelper::GetBounds(absolutelyPlacedPopupChild);
            LOG_OUTPUT(L"Absolute popup bounds: (X, Y, Width, Height) = (%.0f, %.0f, %.0f, %.0f)", absolutelyPlacedPopupBounds.X, absolutelyPlacedPopupBounds.Y, absolutelyPlacedPopupBounds.Width, absolutelyPlacedPopupBounds.Height);

            auto relativelyPlacedPopupBounds = ControlHelper::GetBounds(relativelyPlacedPopupChild);
            LOG_OUTPUT(L"Relative popup bounds: (X, Y, Width, Height) = (%.0f, %.0f, %.0f, %.0f)", relativelyPlacedPopupBounds.X, relativelyPlacedPopupBounds.Y, relativelyPlacedPopupBounds.Width, relativelyPlacedPopupBounds.Height);

            VERIFY_IS_LESS_THAN(relativelyPlacedPopupBounds.Y, absolutelyPlacedPopupBounds.Y);
        });

        RunOnUIThread([&]()
        {
            absolutelyPlacedPopup->IsOpen = false;
            relativelyPlacedPopup->IsOpen = false;
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds1UR()
    {
        WindowedPopupHWNDBounds1Helper(false, false);
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds1PR()
    {
        WindowedPopupHWNDBounds1Helper(true, false);
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds1UC()
    {
        WindowedPopupHWNDBounds1Helper(false, true);
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds1PC()
    {
        WindowedPopupHWNDBounds1Helper(true, true);
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds1Helper(bool isParented, bool popupChildIsCanvas)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_primitives::Popup^ popup;
        xaml_controls::Canvas^ canvas;
        FrameworkElement^ popupChild;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='400' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                L"  <Canvas x:Name='myCanvas' Width='150' Height='150' Background='Green'/>"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            canvas = dynamic_cast<xaml_controls::Canvas^>(rootPanel->FindName(L"myCanvas"));
            VERIFY_IS_NOT_NULL(canvas);

            if (popupChildIsCanvas)
            {
                popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
                    L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
                    L"  <Canvas x:Name='popupChild'>"
                    L"    <Rectangle Width='100' Height='100' Fill='Red'/>"
                    L"  </Canvas>"
                    L"</Popup>"));
            }
            else
            {
                popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
                    L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
                    L"  <Rectangle x:Name='popupChild' Width='100' Height='100' Fill='Red'/>"
                    L"</Popup>"));
            }

            VERIFY_IS_NOT_NULL(popup);

            popupChild = dynamic_cast<FrameworkElement^>(popup->FindName(L"popupChild"));
            VERIFY_IS_NOT_NULL(popupChild);

            if (isParented)
            {
                canvas->Children->Append(popup);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->Popup_SetWindowed(popup);
            popup->XamlRoot = canvas->XamlRoot;
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            popupChild->Margin = {10, 10, 10, 10};
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

        RunOnUIThread([&]()
        {
            popupChild->Translation = {20, 20, 32};
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds2UR()
    {
        WindowedPopupHWNDBounds2Helper(false, false);
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds2PR()
    {
        WindowedPopupHWNDBounds2Helper(true, false);
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds2UC()
    {
        WindowedPopupHWNDBounds2Helper(false, true);
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds2PC()
    {
        WindowedPopupHWNDBounds2Helper(true, true);
    }

    void PopupIntegrationTests::WindowedPopupHWNDBounds2Helper(bool isParented, bool popupChildIsCanvas)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_primitives::Popup^ popup;
        xaml_controls::Canvas^ canvas;
        FrameworkElement^ popupChild;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='400' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                L"  <Canvas x:Name='myCanvas' Width='150' Height='150' Background='Green'/>"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            canvas = dynamic_cast<xaml_controls::Canvas^>(rootPanel->FindName(L"myCanvas"));
            VERIFY_IS_NOT_NULL(canvas);

            if (popupChildIsCanvas)
            {
                popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
                    L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
                    L"  <Canvas x:Name='popupChild'>"
                    L"    <Rectangle Width='100' Height='100' Fill='Red'/>"
                    L"  </Canvas>"
                    L"</Popup>"));
            }
            else
            {
                popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
                    L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
                    L"  <Rectangle x:Name='popupChild' Width='100' Height='100' Fill='Red'/>"
                    L"</Popup>"));
            }

            VERIFY_IS_NOT_NULL(popup);

            popupChild = dynamic_cast<FrameworkElement^>(popup->FindName(L"popupChild"));
            VERIFY_IS_NOT_NULL(popupChild);

            if (isParented)
            {
                canvas->Children->Append(popup);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->Popup_SetWindowed(popup);
            popup->XamlRoot = canvas->XamlRoot;
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            popup->HorizontalOffset = 10;
            popup->VerticalOffset = 10;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

        RunOnUIThread([&]()
        {
            popup->Translation = {20, 20, 32};
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

        RunOnUIThread([&]()
        {
            auto clip = ref new RectangleGeometry();
            clip->Rect = Rect(25, 25, 50, 50);
            canvas->Clip = clip;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

        RunOnUIThread([&]()
        {
            canvas->Translation = {15, 15, 0};
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"5");
    }

    void PopupIntegrationTests::WindowedPopupHWNDBoundsNested()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_primitives::Popup^ popup;
        xaml_primitives::Popup^ nestedPopup;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='400' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                L"  <Popup x:Name='myPopup'>"
                L"    <Grid Width='150' Height='150' Background='Green'>"
                L"      <Popup x:Name='myNestedPopup'>"
                L"        <Rectangle Width='100' Height='100' Fill='Red'/>"
                L"      </Popup>"
                L"    </Grid>"
                L"  </Popup>"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"myPopup"));
            VERIFY_IS_NOT_NULL(popup);

            nestedPopup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"myNestedPopup"));
            VERIFY_IS_NOT_NULL(nestedPopup);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->Popup_SetWindowed(popup);
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->Popup_SetWindowed(nestedPopup);
            nestedPopup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

        RunOnUIThread([&]()
        {
            popup->Translation = {20, 20, 0};
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

        RunOnUIThread([&]()
        {
            nestedPopup->Translation = {30, 30, 0};
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
    }

    //  Verify Input in windowed popup's content
    void PopupIntegrationTests::WindowedPopupInput()
    {
        TestCleanupWrapper cleanup;

        // Create windowed popup and open it

        xaml_primitives::Popup^ popup = nullptr;
        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        auto popupOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='400' Height='400' > \r\n"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;

        });
        // The transition from splash screen to 1st Xaml causes a PositionChaged event
        // on the main app window, which would cause any windowed popups to close.
        // Therefore, defer opening the popup until first Xaml frame is drawn.
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
                L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
                L"  <StackPanel Background='Green' Width='200' Height='200'> \r\n"
                L"    <Button x:Name='button1' Content='Button1InPopup' Width='100' Height='50'/> \r\n"
                L"  </StackPanel> \r\n"
                L"</Popup>"));
            VERIFY_IS_NOT_NULL(popup);
            popup->VerticalOffset = 10;
            popup->HorizontalOffset = -10;
            TestServices::WindowHelper->Popup_SetWindowed(popup);

            button1 = dynamic_cast<xaml_controls::Button^>(popup->FindName(L"button1"));

            openedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"WindowedPopupInput: Windowed popup: Open event fired");
                popupOpenedEvent->Set();
            }));

            closedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"WindowedPopupInput: Windowed popup: Closed event fired");
                popupClosedEvent->Set();
            }));

            popup->XamlRoot = rootPanel->XamlRoot;
            LOG_OUTPUT(L"WindowedPopupInput: Set popup->IsOpen = true");
            popup->IsOpen = true;
        });

        popupOpenedEvent->WaitForDefault();

        // Test pointer input on windowed popup's button

        auto spClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        RunOnUIThread([&]()
        {
            clickRegistration.Attach(button1, ref new xaml::RoutedEventHandler([spClickEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
               LOG_OUTPUT(L"WindowedPopupInput: Windowed popup content: Button Click event fired");
               spClickEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"WindowedPopupInput: Windowed popup content: Pointer tap on button");
        TestServices::InputHelper->Tap(button1);
        spClickEvent->WaitForDefault();
        spClickEvent->Reset();

        // Test keyboard input on windowed popup's button

        auto spGotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
        RunOnUIThread([&]()
        {
            gotFocusRegistration.Attach(button1, ref new RoutedEventHandler([spGotFocusEvent](Platform::Object^ sender, RoutedEventArgs^ e)
            {
               LOG_OUTPUT(L"WindowedPopupInput: Windowed popup content: Button GotFocus event fired");
               spGotFocusEvent->Set();
            }));

            LOG_OUTPUT(L"WindowedPopupInput: Windowed popup content: Set focus on button");
            button1->Focus(FocusState::Keyboard);
        });

        spGotFocusEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"WindowedPopupInput: Windowed popup content: Press Enter key");
        TestServices::KeyboardHelper->Enter();
        spClickEvent->WaitForDefault();
        spClickEvent->Reset();

        // Test input using UIA automation on windowed popup's button

        RunOnUIThread([&]()
        {
            auto button1AP = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(
                xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button1));
            LOG_OUTPUT(L"WindowedPopupInput: Windowed popup content: Click button using UIA");
            button1AP->Invoke();
        });
        spClickEvent->WaitForDefault();
        spClickEvent->Reset();

        // Test pointer input on windowed popup's button after popup offset has changed

        RunOnUIThread([&]()
        {
            popup->HorizontalOffset += 100;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"WindowedPopupInput: Windowed popup content: Pointer tap on button");
        TestServices::InputHelper->Tap(button1);
        spClickEvent->WaitForDefault();
        spClickEvent->Reset();

        // Close windowed popup

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(popup->IsOpen);
            LOG_OUTPUT(L"WindowedPopupInput: Set popup->IsOpen = false");
            popup->IsOpen = false;
        });

        popupClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(popup->IsOpen);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PopupIntegrationTests::WindowedPopupPointerInputCoords()
    {
        WindowedPopupPointerInputCoordsHelper(false/*isRTL*/);
    }

    void PopupIntegrationTests::WindowedPopupPointerInputCoordsRTL()
    {
        WindowedPopupPointerInputCoordsHelper(true/*isRTL*/);
    }

    void PopupIntegrationTests::WindowedPopupPointerInputCoordsHelper(bool isRTL)
    {
        TestCleanupWrapper cleanup;

        // Create windowed popup and open it

        xaml_primitives::Popup^ popup = nullptr;
        xaml::FrameworkElement^ popupChild = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        auto popupOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='400' Height='400' > \r\n"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;

            if (isRTL)
            {
                rootPanel->FlowDirection = FlowDirection::RightToLeft;
            }
        });
        // The transition from splash screen to 1st Xaml causes a PositionChaged event
        // on the main app window, which would cause any windowed popups to close.
        // Therefore, defer opening the popup until first Xaml frame is drawn.
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
                L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
                L"  <StackPanel Background='Green' Width='200' Height='200'> \r\n"
                L"    <TextBlock Text='Some text'/> \r\n"
                L"  </StackPanel> \r\n"
                L"</Popup>"));
            VERIFY_IS_NOT_NULL(popup);
            popup->VerticalOffset = 15;
            popup->HorizontalOffset = 26;
            TestServices::WindowHelper->Popup_SetWindowed(popup);
            rootPanel->Children->Append(popup);

            popupChild = (xaml::FrameworkElement^)popup->Child;

            openedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"WindowedPopupPointerInputCoords: Windowed popup: Open event fired");
                popupOpenedEvent->Set();
            }));

            closedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"WindowedPopupPointerInputCoords: Windowed popup: Closed event fired");
                popupClosedEvent->Set();
            }));

            popup->XamlRoot = rootPanel->XamlRoot;
            LOG_OUTPUT(L"WindowedPopupPointerInputCoords: Set popup->IsOpen = true");
            popup->IsOpen = true;
        });

        popupOpenedEvent->WaitForDefault();

        // Test pointer input accuracy on windowed popup

        auto spPointerPressedEvent = std::make_shared<Event>();
        auto pressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);

        int targetX = 17;
        int targetY = 23;
        int popupChildWidth = 200;
        int popupChildHeight = 200;
        int clickOffsetFromCenterX = (-(popupChildWidth  / 2) + targetX) * (isRTL ? -1 : 1);
        int clickOffsetFromCenterY = (-(popupChildHeight / 2) + targetY);

        RunOnUIThread([&]()
        {
            pressedRegistration.Attach(popupChild, ref new xaml_input::PointerEventHandler([&](Platform::Object^ sender, xaml_input::PointerRoutedEventArgs^ args)
            {
                auto pointerPoint = args->GetCurrentPoint(popupChild);
                LOG_OUTPUT(L"WindowedPopupPointerInputCoords: PointerPressed at: %f,%f (expecting ~%d,%d)", pointerPoint->Position.X, pointerPoint->Position.Y, targetX, targetY);
                VERIFY_IS_TRUE(abs(pointerPoint->Position.X - targetX) < 1.5, "Point should be within a pixel or so.");
                VERIFY_IS_TRUE(abs(pointerPoint->Position.Y - targetY) < 1.5, "Point should be within a pixel or so.");
                spPointerPressedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"WindowedPopupPointerInputCoords: Windowed popup content: Click at specific point in popup");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Left, popupChild, clickOffsetFromCenterX, clickOffsetFromCenterY);
        spPointerPressedEvent->WaitForDefault();
        spPointerPressedEvent->Reset();

        // Test pointer input accuracy on windowed popup after popup offset has changed

        RunOnUIThread([&]()
        {
            popup->HorizontalOffset += 100;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"WindowedPopupPointerInputCoords: Windowed popup content: Click at specific point in popup");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Left, popupChild, clickOffsetFromCenterX, clickOffsetFromCenterY);
        spPointerPressedEvent->WaitForDefault();
        spPointerPressedEvent->Reset();

        // Close windowed popup

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(popup->IsOpen);
            LOG_OUTPUT(L"WindowedPopupPointerInputCoords: Set popup->IsOpen = false");
            popup->IsOpen = false;
        });

        popupClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(popup->IsOpen);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    // Verify windowed popup's UIA tree
    void PopupIntegrationTests::WindowedPopupUIATree()
    {
        TestCleanupWrapper cleanup;

        // Create windowed popup and open it

        xaml_primitives::Popup^ popup = nullptr;
        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        auto popupOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='400' Height='400' > \r\n"
                L"    <Button Content='DummyButton' Width='100' Height='50' AutomationProperties.Name='DummyButton'/> \r\n"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;

        });
        // The transition from splash screen to 1st Xaml causes a PositionChaged event
        // on the main app window, which would cause any windowed popups to close.
        // Therefore, defer opening the popup until first Xaml frame is drawn.
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
                L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left' AutomationProperties.Name='PopupAutomationName'> "
                L"  <StackPanel Background='Green' Width='200' Height='200'> \r\n"
                L"    <Button x:Name='button1' Content='Button1InPopup' Width='100' Height='50' AutomationProperties.Name='ButtonAutomationName'/> \r\n"
                L"  </StackPanel> \r\n"
                L"</Popup>"));
            VERIFY_IS_NOT_NULL(popup);
            popup->VerticalOffset = 10;
            popup->HorizontalOffset = -10;
            TestServices::WindowHelper->Popup_SetWindowed(popup);
            popup->XamlRoot = rootPanel->XamlRoot;

            button1 = dynamic_cast<xaml_controls::Button^>(popup->FindName(L"button1"));

            openedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"WindowedPopupUIATree: Windowed popup: Open event fired");
                popupOpenedEvent->Set();
            }));

            closedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"WindowedPopupUIATree: Windowed popup: Closed event fired");
                popupClosedEvent->Set();
            }));

            LOG_OUTPUT(L"WindowedPopupUIATree: Set popup->IsOpen = true");
            popup->IsOpen = true;
        });

        popupOpenedEvent->WaitForDefault();

        // Verify UIA tree structure, and ensure that windowed popup is a window

        // Set focus on windowed popup's button
        auto spGotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
        RunOnUIThread([&]()
        {
            gotFocusRegistration.Attach(button1, ref new RoutedEventHandler([spGotFocusEvent](Platform::Object^ sender, RoutedEventArgs^ e)
            {
               LOG_OUTPUT(L"WindowedPopupUIATree: Windowed popup content: Button GotFocus event fired");
               spGotFocusEvent->Set();
            }));

            LOG_OUTPUT(L"WindowedPopupUIATree: Windowed popup content: Set focus on button");
            button1->Focus(FocusState::Keyboard);
        });

        spGotFocusEvent->WaitForDefault();

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(popup);
        }

        // Walk up the UIA tree from the focused button
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationElement> spAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spPopupAutomationElement;
            wrl::ComPtr<IUIAutomationElement> popupWindowAutomationElement;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = std::make_shared<AutomationClient::AutomationClientManager>();
            spAutomationClientManager->GetAutomation(&spAutomation);

            LOG_OUTPUT(L"WindowedPopupUIATree: Windowed popup content: Get focused button using UIA");
            spAutomation->GetFocusedElement(&spAutomationElement);
            WEX::Common::Throw::IfNull(spAutomationElement.Get());
            LogThrow_IfFailed(spAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.Storage()));
            VERIFY_ARE_EQUAL(autoVar.Storage()->vt, VT_BSTR);
            VERIFY_IS_TRUE(!wcscmp(L"ButtonAutomationName", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"WindowedPopupUIATree: Windowed popup content: Get button's UIA parent");
            spPopupAutomationElement.Attach(spAutomationClientManager->GetParent(spAutomationElement.Get()));
            VERIFY_IS_NOT_NULL(spPopupAutomationElement);
            LogThrow_IfFailed(spPopupAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.Storage()));
            VERIFY_ARE_EQUAL(autoVar.Storage()->vt, VT_BSTR);
            VERIFY_IS_TRUE(!wcscmp(L"PopupAutomationName", (autoVar.Storage())->bstrVal));

            wrl::ComPtr<IUIAutomationElement> spNext;
            spNext.Attach(spAutomationClientManager->GetNext(spPopupAutomationElement.Get()));
            VERIFY_IS_NULL(spNext, L"Popup node should have no next sibling");

            wrl::ComPtr<IUIAutomationElement> spPrev;
            spPrev.Attach(spAutomationClientManager->GetPrevious(spPopupAutomationElement.Get()));
            VERIFY_IS_NULL(spPrev, L"Popup node should have no previous sibling");

            // Get the uia parent. This will be the UIAWindow for the windowed popup. Note that we are not checking its name
            // because it will have a different name when running in uap versus wpfmode.
            LOG_OUTPUT(L"WindowedPopupUIATree: Windowed popup content: Get popup's UIA parent.");
            popupWindowAutomationElement.Attach(spAutomationClientManager->GetParent(spPopupAutomationElement.Get()));
            VERIFY_IS_NOT_NULL(popupWindowAutomationElement);
        });

        // Close windowed popup

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(popup->IsOpen);
            LOG_OUTPUT(L"WindowedPopupUIATree: Set popup->IsOpen = false");
            popup->IsOpen = false;
        });

        popupClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(popup->IsOpen);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PopupIntegrationTests::WindowedPopupRTL()
    {
        PopupRTLHelper(true);
    }

    void PopupIntegrationTests::PopupRTL()
    {
        PopupRTLHelper(false);
    }

    void PopupIntegrationTests::PopupRTLHelper(bool isWindowed)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_controls::Canvas^ canvas;
        xaml_primitives::Popup^ popup;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='400' HorizontalAlignment='Left' VerticalAlignment='Top' FlowDirection='RightToLeft'> \r\n"
                L" <Canvas x:Name='myCanvas' Translation='50.0,50.0,0.0'>"
                L"  <Popup x:Name='myPopup' HorizontalOffset='100' VerticalOffset='100'>"
                L"    <Rectangle Width='100' Height='100' Fill='Red'/>"
                L"  </Popup>"
                L" </Canvas>"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            canvas = dynamic_cast<xaml_controls::Canvas^>(rootPanel->FindName(L"myCanvas"));
            VERIFY_IS_NOT_NULL(canvas);

            popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"myPopup"));
            VERIFY_IS_NOT_NULL(popup);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            if (isWindowed)
            {
                TestServices::WindowHelper->Popup_SetWindowed(popup);
            }
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            canvas->CompositeMode = ElementCompositeMode::SourceOver;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
    }

    void PopupIntegrationTests::WindowedPopupMoveSize()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_primitives::Popup^ popup;
        xaml_shapes::Rectangle^ rectangle;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='400' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                L"  <Popup x:Name='myPopup' HorizontalOffset='10' VerticalOffset='10'>"
                L"    <Rectangle x:Name='myRectangle' Width='100' Height='100' Fill='Red'/>"
                L"  </Popup>"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"myPopup"));
            VERIFY_IS_NOT_NULL(popup);

            rectangle = dynamic_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"myRectangle"));
            VERIFY_IS_NOT_NULL(rectangle);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->Popup_SetWindowed(popup);
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            popup->HorizontalOffset = 20;
            popup->VerticalOffset = 20;

            rectangle->Width = 200;
            rectangle->Height = 200;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
    }

    void PopupIntegrationTests::WindowedPopup3DRequirements()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_controls::StackPanel^ rootPanel;
        xaml_primitives::Popup^ popup;
        xaml_shapes::Rectangle^ rectangle;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='400' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                L"  <Popup x:Name='myPopup' HorizontalOffset='10' VerticalOffset='10'>"
                L"    <Rectangle x:Name='myRectangle' Width='100' Height='100' Fill='Red'/>"
                L"  </Popup>"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"myPopup"));
            VERIFY_IS_NOT_NULL(popup);

            rectangle = dynamic_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"myRectangle"));
            VERIFY_IS_NOT_NULL(rectangle);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup->ShouldConstrainToRootBounds = false;
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            popup->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup->Translation = {10, 10, 32};
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

        RunOnUIThread([&]()
        {
            popup->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup->Translation = {0, 0, 0};
            auto projection = ref new PlaneProjection();
            projection->RotationY = 5;
            rectangle->Projection = projection;
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

        RunOnUIThread([&]()
        {
            popup->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            rectangle->Projection = nullptr;
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1B");

        RunOnUIThread([&]()
        {
            popup->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto compositeTransform3D = ref new CompositeTransform3D();
            compositeTransform3D->RotationY = 5;
            popup->Transform3D = compositeTransform3D;
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

        RunOnUIThread([&]()
        {
            popup->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup->Transform3D = nullptr;
            rootPanel->Rotation = 5;
            rootPanel->RotationAxis = {0, 1, 0};
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"5");
    }

    void PopupIntegrationTests::WindowedPopupHighDPI()
    {
        // We're setting the DPI awareness context in the TestData via a TEST_METHOD_PROPERTY. We pick it up when
        // creating the WPFHost and call SetThreadDpiAwarenessContext with the appropriate enum value. The TestData
        // isn't set until the test is initialized, though, so the current WPFHost won't have the correct awareness
        // context. Force initialization again (now that the TEST_METHOD_PROPERTY has been applied) to create another
        // WPFHost that can set the correct DPI awareness context.
        TestServices::InitializeHost(true /* initializeDpiAwarenessContext */);

        ChangeDPI changeDPI;
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_primitives::Popup^ popup;
        xaml_shapes::Rectangle^ rectangle;
        xaml_controls::Canvas^ popupParent;

        auto popupTapped = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
        auto popupTappedRegistration = CreateSafeEventRegistration(UIElement, Tapped);
        auto islandTapped = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
        auto islandTappedRegistration = CreateSafeEventRegistration(UIElement, Tapped);

        RunOnUIThread([&]()
        {
            rectangle = ref new xaml_shapes::Rectangle();
            rectangle->Width = 50;
            rectangle->Height = 50;
            // The tap indicator shows underneath the windowed popup, so make the popup content mostly transparent to make debugging easier.
            rectangle->Fill = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(0x10, 0xff, 0, 0));
            popupTappedRegistration.Attach(rectangle, [&]() { popupTapped->Set(); });

            popup = ref new xaml_primitives::Popup();
            popup->ShouldConstrainToRootBounds = false;
            popup->Child = rectangle;

            popupParent = ref new xaml_controls::Canvas();
            popupParent->Width = 150;
            popupParent->Height = 150;
            // Dark background to make the tap indicator more obvious
            popupParent->Background = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(0xff, 0x10, 0x10, 0x10));
            popupParent->Children->Append(popup);
            xaml_controls::Canvas::SetLeft(popupParent, 100);
            xaml_controls::Canvas::SetTop(popupParent, 100);
            islandTappedRegistration.Attach(popupParent, [&]() { islandTapped->Set(); });

            xaml_controls::Canvas^ root = ref new xaml_controls::Canvas();
            root->Children->Append(popupParent);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Opening popup.");
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        WEX::Common::String value;
        if (SUCCEEDED(WEX::TestExecution::TestData::TryGetValue(L"DpiAwarenessContext", value)))
        {
            LOG_OUTPUT(L"> Tapping popup.");
            TestServices::InputHelper->Tap(rectangle);

            LOG_OUTPUT(L"> Waiting for tap handler.");
            popupTapped->WaitForDefault();
            VERIFY_IS_TRUE(popupTapped->HasFired());
            popupTapped->Reset();
            islandTapped->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(islandTapped->HasFired());
            islandTapped->Reset();

            LOG_OUTPUT(L"> Tapping island.");
            TestServices::InputHelper->Tap(popupParent);

            LOG_OUTPUT(L"> Waiting for tap handler.");
            popupTapped->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(popupTapped->HasFired());
            popupTapped->Reset();
            // UnawareGdiScaled has island rendering problems that cause the island's content to be scaled incorrectly.
            // <VisualTreeIsland renders incorrectly under UnawareGdiScaled DPI awareness context>
            if (value.CompareNoCase(L"UnawareGdiScaled") != 0)
            {
                islandTapped->WaitForDefault();
                VERIFY_IS_TRUE(islandTapped->HasFired());
            }
            else
            {
                LOG_OUTPUT(L">>> Skipping waiting for the tap handler.");
                islandTapped->WaitForNoThrow(std::chrono::milliseconds(1000));
            }
            islandTapped->Reset();
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Closing popup.");
            popup->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void PopupIntegrationTests::ValidateLightDismissOverlayMode()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validate that the default is Off and light-dismissible there is no overlay element.");
        RunOnUIThread([&]()
        {
            auto popup = ref new xaml_primitives::Popup();
            popup->Child = ref new xaml_controls::Grid();
            popup->IsLightDismissEnabled = true;

            VERIFY_ARE_EQUAL(popup->LightDismissOverlayMode, xaml_controls::LightDismissOverlayMode::Off);
            VERIFY_IS_NULL(TestServices::Utilities->GetPopupOverlayElement(popup));
        });

        LOG_OUTPUT(L"Validate that when set to On and light-dismissible but no child there is no overlay element.");
        RunOnUIThread([&]()
        {
            auto popup = ref new xaml_primitives::Popup();
            popup->IsLightDismissEnabled = true;
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            VERIFY_IS_NULL(TestServices::Utilities->GetPopupOverlayElement(popup));
        });

        LOG_OUTPUT(L"Validate that when set to On but not light-dismissible, there is no overlay element.");
        RunOnUIThread([&]()
        {
            auto popup = ref new xaml_primitives::Popup();
            popup->Child = ref new xaml_controls::Grid();
            popup->IsLightDismissEnabled = false;
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            VERIFY_IS_NULL(TestServices::Utilities->GetPopupOverlayElement(popup));
        });

        LOG_OUTPUT(L"Validate that when set to On and light-dismissible, there is an overlay element.");
        RunOnUIThread([&]()
        {
            auto popup = ref new xaml_primitives::Popup();
            popup->Child = ref new xaml_controls::Grid();
            popup->IsLightDismissEnabled = true;
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            VERIFY_IS_NOT_NULL(TestServices::Utilities->GetPopupOverlayElement(popup));
        });

        if (!TestServices::Utilities->IsXBox)
        {
            LOG_OUTPUT(L"Validate that when set to Auto and not on Xbox, there is no overlay element.");
            RunOnUIThread([&]()
            {
                auto popup = ref new xaml_primitives::Popup();
                popup->Child = ref new xaml_controls::Grid();
                popup->IsLightDismissEnabled = true;
                popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Auto;

                VERIFY_IS_NULL(TestServices::Utilities->GetPopupOverlayElement(popup));
            });
        }
    }

    void PopupIntegrationTests::DoesAutoLightDismissOverlayModeCreateOverlayOnXbox()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto popup = ref new xaml_primitives::Popup();
            popup->Child = ref new xaml_controls::Grid();
            popup->IsLightDismissEnabled = true;
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Auto;

            VERIFY_IS_NOT_NULL(TestServices::Utilities->GetPopupOverlayElement(popup));
        });
    }

    void PopupIntegrationTests::ValidateDefaultOverlayBrush()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto expectedBrush = safe_cast<xaml_media::SolidColorBrush^>(xaml::Application::Current->Resources->Lookup(L"PopupLightDismissOverlayBackground"));

            auto popup = ref new xaml_primitives::Popup();
            popup->Child = ref new xaml_controls::Grid();
            popup->IsLightDismissEnabled = true;
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(popup);
            THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist for the popup.");

            auto overlayRect = dynamic_cast<xaml_shapes::Rectangle^>(overlayElement);
            THROW_IF_NULL_WITH_MSG(overlayRect, L"The overlay element should be a rectangle.");

            auto overlayBrush = safe_cast<xaml_media::SolidColorBrush^>(overlayRect->Fill);
            VERIFY_IS_NOT_NULL(overlayBrush);
            VERIFY_IS_TRUE(overlayBrush->Equals(expectedBrush));
        });
    }

    void PopupIntegrationTests::ValidateOverlayTreePlacement()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::Popup^ testPopup = nullptr;
        xaml_primitives::Popup^ otherPopup = nullptr;

        RunOnUIThread([&]()
        {
            // Set some content to let the test activate successfully.
            xaml_controls::Grid^ rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;

            // Open a popup first to make sure the overlay gets inserted behind its
            // popup's child, but in front of other popup's child.
            otherPopup = ref new xaml_primitives::Popup();
            otherPopup->Child = ref new xaml_controls::Grid();
            otherPopup->XamlRoot = rootGrid->XamlRoot;
            otherPopup->IsOpen = true;

            testPopup = ref new xaml_primitives::Popup();
            testPopup->Child = ref new xaml_shapes::Rectangle();
            testPopup->IsLightDismissEnabled = true;
            testPopup->XamlRoot = rootGrid->XamlRoot;
            testPopup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            testPopup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        xaml::DependencyObject^ popupRoot = nullptr;
        RunOnUIThread([&]()
        {
            auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(testPopup);
            VERIFY_IS_NOT_NULL(overlayElement);

            // If it has a visual parent, it's in the tree.
            popupRoot = xaml_media::VisualTreeHelper::GetParent(overlayElement);
            VERIFY_IS_NOT_NULL(popupRoot);

            // There should be both the popups' child elements as well as the test popup's
            // overlay element.
            auto childCount = xaml_media::VisualTreeHelper::GetChildrenCount(popupRoot);
            VERIFY_ARE_EQUAL(childCount, 3);

            // The overlay element should be behind the test popup's child but in front of the other popup's
            // child so it should be the second child.
            auto secondChild = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(popupRoot, 1));
            VERIFY_IS_TRUE(overlayElement->Equals(secondChild));

            testPopup->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // There should just be the child element from the other popup.
            auto childCount = xaml_media::VisualTreeHelper::GetChildrenCount(popupRoot);
            VERIFY_ARE_EQUAL(childCount, 1);

            auto child = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(popupRoot, 0));
            VERIFY_IS_TRUE(otherPopup->Child->Equals(child));
        });
    }

    void PopupIntegrationTests::CanChangeLightDismissOverlayModeWhileOpen()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::Popup^ popup = nullptr;

        RunOnUIThread([&]()
        {
            popup = ref new xaml_primitives::Popup();
            popup->Child = ref new xaml_controls::Grid();
            popup->IsLightDismissEnabled = true;
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;

            popup->IsOpen = true;

            // Set some content to let the test activate successfully.
            TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(popup);
            VERIFY_IS_NOT_NULL(overlayElement);

            // There should be 2 elements under the popup root.
            auto popupRoot = xaml_media::VisualTreeHelper::GetParent(popup->Child);
            auto childCount = xaml_media::VisualTreeHelper::GetChildrenCount(popupRoot);
            VERIFY_ARE_EQUAL(childCount, 2);

            // The overlay element should be behind the child, so it should be the first child.
            auto firstChild = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(popupRoot, 0));
            VERIFY_IS_TRUE(overlayElement->Equals(firstChild));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;

            auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(popup);
            VERIFY_IS_NULL(overlayElement);

            // There should be only 1 element under the popup root.
            auto popupRoot = xaml_media::VisualTreeHelper::GetParent(popup->Child);
            auto childCount = xaml_media::VisualTreeHelper::GetChildrenCount(popupRoot);
            VERIFY_ARE_EQUAL(childCount, 1);

            // The remaining child should not be the overlay element.
            auto child = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(popupRoot, 0));
            VERIFY_IS_TRUE(popup->Child->Equals(child));
        });
    }

    void PopupIntegrationTests::DoesOverlaySizeWithWindow()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::Popup^ popup = nullptr;

        RunOnUIThread([&]()
        {
            popup = ref new xaml_primitives::Popup();
            popup->Child = ref new xaml_controls::Grid();
            popup->IsLightDismissEnabled = true;
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            popup->IsOpen = true;

            // Set some content to let the test activate successfully.
            xaml_controls::Grid^ rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect windowBounds = {};
        RunOnUIThread([&]()
        {
            auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(popup);
            THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist for the popup.");

            // The overlay element should be sized to the window.
            windowBounds = xaml::Window::Current->Bounds;
            VERIFY_ARE_EQUAL(overlayElement->ActualWidth, windowBounds.Width);
            VERIFY_ARE_EQUAL(overlayElement->ActualHeight, windowBounds.Height);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Re-size the window so that we can re-check the overlay element's size.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(windowBounds.Width * 0.5f, windowBounds.Height * 0.5f));
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(popup);
            THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist for the popup.");

            // The overlay element should be sized to the window.
            windowBounds = xaml::Window::Current->Bounds;
            VERIFY_ARE_EQUAL(overlayElement->ActualWidth, windowBounds.Width);
            VERIFY_ARE_EQUAL(overlayElement->ActualHeight, windowBounds.Height);
        });
    }

    void PopupIntegrationTests::CanSetChildWhileOpenWithOverlayEnabled()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::Popup^ popup = nullptr;

        RunOnUIThread([&]()
        {
            // Set some content to let the test activate successfully.
            xaml_controls::Grid^ rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;

            popup = ref new xaml_primitives::Popup();
            popup->IsLightDismissEnabled = true;
            popup->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            popup->XamlRoot = rootGrid->XamlRoot;
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup->Child = ref new xaml_controls::Grid();

            auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(popup);
            VERIFY_IS_NOT_NULL(overlayElement);

            // There should be 2 elements under the popup root.
            auto popupRoot = xaml_media::VisualTreeHelper::GetParent(popup->Child);
            auto childCount = xaml_media::VisualTreeHelper::GetChildrenCount(popupRoot);
            VERIFY_ARE_EQUAL(childCount, 2);

            // The overlay element should be behind the child, so it should be the first child.
            auto firstChild = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(popupRoot, 0));
            VERIFY_IS_TRUE(overlayElement->Equals(firstChild));
        });
    }

    void PopupIntegrationTests::ValidateOverlayDCompTree()
    {
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::Grid^ root = nullptr;
        xaml_primitives::Popup^ popup = nullptr;

        RunOnUIThread([&]()
        {
            // Use a parented popup so that the requested theme from the root element can
            // propogate to it.
            root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                        Background="{ThemeResource SystemControlBackgroundAltHighBrush}" >
                       <Popup x:Name="popup" IsLightDismissEnabled="True" LightDismissOverlayMode="On">
                            <Grid/>
                        </Popup>
                    </Grid>)"));

            popup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"popup"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate the dark theme of the overlay.");
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Dark");

        LOG_OUTPUT(L"Validate the light theme of the overlay.");
        RunOnUIThread([&]()
        {
            root->RequestedTheme = xaml::ElementTheme::Light;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Light");

        LOG_OUTPUT(L"Validate the high-contrast theme of the overlay.");
        RunOnUIThread([&]()
        {
            TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Test;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "HC");
    }

    void PopupIntegrationTests::ValidateOverlayUIETree()
    {
        TestCleanupWrapper cleanup;

        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 400),
            1.f,
            []()
            {
                xaml_controls::Grid^ root = nullptr;
                RunOnUIThread([&]()
                {
                    // Use a parented popup so that the requested theme from the root element can
                    // propogate to it.
                    root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                        LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                            Background="{ThemeResource SystemControlBackgroundAltHighBrush}" >
                           <Popup IsOpen="True" IsLightDismissEnabled="True" LightDismissOverlayMode="On">
                                <Grid/>
                            </Popup>
                        </Grid>)"));

                    TestServices::WindowHelper->WindowContent = root;
                });
                TestServices::WindowHelper->WaitForIdle();

                return root;
            },
            []()
            {
            }
        );
    }

    void PopupIntegrationTests::WindowedPopupOpenAndCloseHelper()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_primitives::Popup^ popup = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupClosedEvent = std::make_shared<Event>();

        auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='400' Height='400' > \r\n"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;

        });
        // The transition from splash screen to 1st Xaml causes a PositionChaged event
        // on the main app window, which would cause any windowed popups to close.
        // Therefore, defer opening the popup until first Xaml frame is drawn.
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
                L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
                L"  <StackPanel Background='Green' Width='200' Height='200'> \r\n"
                L"    <Button Content='ButtonInPopup' /> \r\n"
                L"  </StackPanel> \r\n"
                L"</Popup>"));
            VERIFY_IS_NOT_NULL(popup);
            popup->VerticalOffset = 10;
            popup->HorizontalOffset = -50;
            TestServices::WindowHelper->Popup_SetWindowed(popup);

            openedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"WindowedPopupOpenAndClose: Windowed popup: Open event fired");
                popupOpenedEvent->Set();
            }));

            closedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"WindowedPopupOpenAndClose: Windowed popup: Closed event fired");
                popupClosedEvent->Set();
            }));

            popup->XamlRoot = rootPanel->XamlRoot;
            LOG_OUTPUT(L"WindowedPopupOpenAndClose: Set popup->IsOpen = true");
            popup->IsOpen = true;
        });

        popupOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(popup->IsOpen);
            LOG_OUTPUT(L"WindowedPopupOpenAndClose: Set popup->IsOpen = false");
            popup->IsOpen = false;
        });

        popupClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(popup->IsOpen);
        });
    }

    void PopupIntegrationTests::OpenPopupUnderCollapsedParent()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_controls::Canvas^ rootPanel;
        xaml_controls::Grid^ grid;
        xaml_primitives::Popup^ popup;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Loading XAML");
            rootPanel = dynamic_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
                L"  <Grid x:Name='myGrid' Width='200' Height='200'> \r\n"
                L"    <Popup x:Name='myPopup' IsOpen='False'> \r\n"
                L"      <Rectangle Width='200' Height='200' Fill='Red'/> \r\n"
                L"    </Popup> \r\n"
                L"  </Grid> \r\n"
                L"</Canvas>"));
            VERIFY_IS_NOT_NULL(rootPanel);
            grid = safe_cast<xaml_controls::Grid^>(rootPanel->FindName(L"myGrid"));
            popup = safe_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"myPopup"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Uncollapsed");

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Collapsing parent");
            grid->Visibility = xaml::Visibility::Collapsed;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Collapsed");

        RunOnUIThread([&]()
        {
            // This is the test case that verifies the fix - when un-collapsing the parent, the Popup should now render.
            LOG_OUTPUT(L"Un-collapsing parent.  Verify the Popup renders");
            grid->Visibility = xaml::Visibility::Visible;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Uncollapsed");
    }

    ref class XcbHostBackdropBrush sealed : public Microsoft::UI::Xaml::Media::XamlCompositionBrushBase
    {
    public:
        XcbHostBackdropBrush()
        {
            m_compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
        }


    protected:
        void OnConnected() override
        {
            if (m_brush == nullptr)
            {
                // Sorry Charlie, no hostbackdrop.  This will be transparent.
                m_brush = m_compositor->CreateBackdropBrush();
            }
            this->CompositionBrush = m_brush;
        }

        void OnDisconnected() override
        {
            this->CompositionBrush = nullptr;
            delete m_brush;
            m_brush = nullptr;
        }

    private:
        Microsoft::UI::Composition::Compositor^ m_compositor = nullptr;
        Microsoft::UI::Composition::CompositionBrush^ m_brush = nullptr;
    };

    void PopupIntegrationTests::HostBackdropBrush()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
        HostBackdropBrushHelper(false, false);
    }

    void PopupIntegrationTests::HostBackdropBrushWindowed()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
        HostBackdropBrushHelper(true, false);
    }

    void PopupIntegrationTests::HostBackdropBrushLTE()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
        HostBackdropBrushHelper(false, true);
    }

    void PopupIntegrationTests::HostBackdropBrushHelper(bool isWindowed, bool useLTE)
    {
        xaml_controls::Canvas^ rootPanel;
        xaml_controls::Grid^ grid;
        xaml_primitives::Popup^ popup;
        xaml_shapes::Rectangle^ rectangle;
        xaml_shapes::Rectangle^ snapshot;

        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Loading XAML");
            rootPanel = dynamic_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left' Width='200' Height='200' Background='Red'> "
                L"  <Rectangle x:Name='snapshot' Width='50' Height='50' Canvas.Top='200'/> \r\n"
                L"</Canvas>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
                L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
                L"    <Grid x:Name='myGrid' Width='100' Height='100' Background='Green'> \r\n"
               L"      <Rectangle x:Name='myRectangle' Width='50' Height='50'/> \r\n"
                L"    </Grid> \r\n"
                L"</Popup>"));
            VERIFY_IS_NOT_NULL(popup);

            if (isWindowed)
            {
                TestServices::WindowHelper->Popup_SetWindowed(popup);
            }
            popup->IsOpen = true;

            rectangle = safe_cast<xaml_shapes::Rectangle^>(popup->FindName(L"myRectangle"));
            rectangle->Fill = ref new XcbHostBackdropBrush();

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        if (useLTE)
        {
            RunOnUIThread([&]()
            {
                grid = safe_cast<xaml_controls::Grid^>(popup->FindName(L"myGrid"));
                auto lte = TestServices::WindowHelper->AddTestLTE(grid, nullptr, LTEParentMode::PopupRoot, false);
            });
        }

        xaml_imaging::RenderTargetBitmap^ rtb;
        auto renderedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            rtb = ref new xaml_imaging::RenderTargetBitmap();

            create_task(rtb->RenderAsync(rectangle)).then([&renderedEvent] ()
            {
                LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
                renderedEvent->Set();
            });
        });
        renderedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            snapshot = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"snapshot"));
            auto imageBrush = ref new xaml_media::ImageBrush();
            imageBrush->ImageSource = rtb;
            snapshot->Fill = imageBrush;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
        TestServices::Utilities->ResetMockDCompSurfaceId();

        // For some reason Windowed popups sometimes render in DWM with a very slight difference in color
        // which is causing test instability for the surface comparison - it looks like an animation is playing.
        // Disable surface comparison for Windowed popups.
        TestServices::Utilities->VerifyMockDCompOutput(isWindowed ? MockDComp::SurfaceComparison::NoComparison : MockDComp::SurfaceComparison::AllSurfaces);
    }

    void PopupIntegrationTests::ElevationTranslate()
    {
        ElevationHelper(true);
    }

    void PopupIntegrationTests::ElevationTranslateNoAnim()
    {
        ElevationHelper(false);
    }

    void PopupIntegrationTests::ElevationHelper(bool animate)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        RuntimeEnabledFeatureOverride featureDisableGlobalAnimations(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations, !animate);

        xaml_controls::Canvas^ rootPanel;
        xaml_primitives::Popup^ popup;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Loading XAML");
            rootPanel = dynamic_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left' Width='400' Height='300' Background='White' CompositeMode='SourceOver'> "
                L"    <Popup x:Name='myPopup'> \r\n"
                L"      <Rectangle Width='200' Height='200' Fill='Red'/> \r\n"
                L"    </Popup> \r\n"
                L"</Canvas>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            popup = safe_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"myPopup"));
            VERIFY_IS_NOT_NULL(popup);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::WindowHelper->SetPostTickCallback(ref new PostTickCallback([&]()
        {
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }));

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Opening Popup and apply elevation");
            popup->IsOpen = true;

            TestServices::WindowHelper->ApplyElevationEffect(popup, 0);
        });
        TestServices::WindowHelper->SynchronouslyTickUIThread(1);

        TestServices::WindowHelper->SetPostTickCallback(nullptr);

        TestServices::WindowHelper->WaitForIdle();

    }

    // Regression coverage for GitHub #3879: a Popup initialized with IsOpen='True' during
    // XAML parsing has no popup root available yet, so the open must be deferred until the
    // element enters the live tree (EnterImpl) instead of calling Open() during parsing.
    //
    // Scenario 1: IsOpen is set before the child (attribute ordering). The deferral happens
    // in CPopup::SetChild.
    void PopupIntegrationTests::InitializeWithIsOpenTrueDoesNotCrash()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_primitives::Popup^ popup = nullptr;
        xaml_controls::Border^ child = nullptr;

        RunOnUIThread([&]()
        {
            // IsOpen='True' is applied before the child during parsing. Historically this threw
            // because no popup root was reachable yet. It must instead defer and not crash.
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Width='400' Height='400'> \r\n"
                L"  <Popup x:Name='popup1' IsOpen='True'> \r\n"
                L"    <Border x:Name='child1' Width='100' Height='100' Background='Red'/> \r\n"
                L"  </Popup> \r\n"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup1"));
            VERIFY_IS_NOT_NULL(popup);
            child = dynamic_cast<xaml_controls::Border^>(rootPanel->FindName(L"child1"));
            VERIFY_IS_NOT_NULL(child);

            // The IsOpen value is honored even though the actual open was deferred.
            VERIFY_IS_TRUE(popup->IsOpen);

            // Connecting to the live tree triggers EnterImpl, which performs the deferred open.
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Popup is still open and its child laid out, proving the deferred open completed.
            VERIFY_IS_TRUE(popup->IsOpen);
            VERIFY_ARE_EQUAL(100.0, child->ActualHeight);
            VERIFY_ARE_EQUAL(100.0, child->ActualWidth);
        });
    }

    // Scenario 2: The child is parsed before IsOpen (property-element
    // ordering), so CPopup::SetChild runs while IsOpen is still false and does not defer. The
    // subsequent IsOpen='True' is applied via CPopup::SetValue while the child already exists,
    // which must also defer the open when parsing with no popup root available.
    void PopupIntegrationTests::InitializeWithChildBeforeIsOpenDoesNotCrash()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_primitives::Popup^ popup = nullptr;
        xaml_controls::Border^ child = nullptr;

        RunOnUIThread([&]()
        {
            // Popup.Child is set first, then Popup.IsOpen. IsOpen goes through SetValue with the
            // child already present, reaching the Open() path during parsing. It must defer and
            // not crash.
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Width='400' Height='400'> \r\n"
                L"  <Popup x:Name='popup1'> \r\n"
                L"    <Popup.Child> \r\n"
                L"      <Border x:Name='child1' Width='100' Height='100' Background='Red'/> \r\n"
                L"    </Popup.Child> \r\n"
                L"    <Popup.IsOpen>True</Popup.IsOpen> \r\n"
                L"  </Popup> \r\n"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup1"));
            VERIFY_IS_NOT_NULL(popup);
            child = dynamic_cast<xaml_controls::Border^>(rootPanel->FindName(L"child1"));
            VERIFY_IS_NOT_NULL(child);

            VERIFY_IS_TRUE(popup->IsOpen);

            // Connecting to the live tree triggers EnterImpl, which performs the deferred open.
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(popup->IsOpen);
            VERIFY_ARE_EQUAL(100.0, child->ActualHeight);
            VERIFY_ARE_EQUAL(100.0, child->ActualWidth);
        });
    }


} } } } } } } // Microsoft::UI::Xaml::Tests::Controls::Primitives::Popup
