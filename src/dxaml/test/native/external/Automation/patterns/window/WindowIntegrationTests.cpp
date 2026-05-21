// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "WindowIntegrationTests.h"
#include <Patterns\WindowPatternHandler.h>
#include <Patterns\MockWindowPatternObject.h>

#include <XamlTailored.h>
#include <TestEvent.h>

#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>
#include <array>

#include <ChangeDPI.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace Tests::Native::External::Automation::Patterns;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Window {

    bool WindowIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool WindowIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool WindowIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void WindowIntegrationTests::CanRaiseWindowEvents()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ button = nullptr;
        xaml_automation_peers::AutomationPeer^ buttonAP = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestButton";
        uiaInfo.m_AutomationID = L"TestButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button();
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
        });
        TreeHelper::AddElementIntoLivetree<xaml_controls::Button>(button);

        std::array<EVENTID, 2> ids = {
            UIA_Window_WindowClosedEventId,
            UIA_Window_WindowOpenedEventId
        };

        std::array<xaml_automation_peers::AutomationEvents, 2> events = {
            xaml_automation_peers::AutomationEvents::WindowClosed,
            xaml_automation_peers::AutomationEvents::WindowOpened,
        };

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<Patterns::WindowPatternHandler> spAutomationPatternHandler;
            RunOnUIThread([&]()
            {
                buttonAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button);
            });
            std::shared_ptr<AutomationClient::AutomationClientManager> spAutomationClientManager
                = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            int index = 0;
            for (auto id : ids)
            {
                auto spEvent = std::make_shared<Event>();
                spAutomationPatternHandler.Attach(new Patterns::WindowPatternHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, id));
                spAutomationPatternHandler->AttachEventHandler();
                RunOnUIThread([&]()
                {
                    buttonAP->RaiseAutomationEvent(events.at(index++));
                });
                spAutomationPatternHandler->ConfirmAndUnregister();
            }
        });
    }

    void WindowIntegrationTests::VerifyPropertyGetters()
    {
        TestCleanupWrapper cleanup;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"VerifyPropertyGetters";

        MockWindowPatternControl^ control;
        MockWindowPatternAutomationPeer^ mockAP;

        RunOnUIThread([&]()
        {
            control = ref new MockWindowPatternControl();
            mockAP = static_cast<MockWindowPatternAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(control));
            xaml_automation::AutomationProperties::SetName(control, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = control;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationWindowPattern> spWindowPattern;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spWindowPattern),
                     L"Failed in retreiving Window Pattern.");
            WEX::Common::Throw::IfNull(spWindowPattern.Get());

            // IWindowProvider::IsModal
            BOOL isModal = false;
            mockAP->IsModalMockValue = true;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentIsModal(&isModal));
            VERIFY_ARE_EQUAL(!!isModal, mockAP->IsModalMockValue);

            mockAP->IsModalMockValue = false;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentIsModal(&isModal));
            VERIFY_ARE_EQUAL(!!isModal, mockAP->IsModalMockValue);

            // IWindowProvider::IsTopMost
            BOOL isTopmost = false;
            mockAP->IsTopmostMockValue = true;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentIsTopmost(&isTopmost));
            VERIFY_ARE_EQUAL(!!isTopmost, mockAP->IsTopmostMockValue);

            mockAP->IsTopmostMockValue = false;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentIsModal(&isTopmost));
            VERIFY_ARE_EQUAL(!!isTopmost, mockAP->IsTopmostMockValue);

            // IWindowProvider::Maximizable
            BOOL maximizable = false;
            mockAP->MaximizableMockValue = true;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentCanMaximize(&maximizable));
            VERIFY_ARE_EQUAL(!!maximizable, mockAP->MaximizableMockValue);

            mockAP->MaximizableMockValue = false;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentCanMaximize(&maximizable));
            VERIFY_ARE_EQUAL(!!maximizable, mockAP->MaximizableMockValue);

            // IWindowProvider::Minimizable
            BOOL minimizable = false;
            mockAP->MinimizableMockValue = true;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentCanMinimize(&minimizable));
            VERIFY_ARE_EQUAL(!!minimizable, mockAP->MinimizableMockValue);

            mockAP->MinimizableMockValue = false;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentCanMinimize(&minimizable));
            VERIFY_ARE_EQUAL(!!minimizable, mockAP->MinimizableMockValue);

            // IWindowProvider::WindowInteractionState
            WindowInteractionState windowInteractionState;
            mockAP->InteractionStateMockValue = xaml_automation::WindowInteractionState::Running;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentWindowInteractionState(&windowInteractionState));
            VERIFY_ARE_EQUAL(windowInteractionState, WindowInteractionState_Running);

            mockAP->InteractionStateMockValue = xaml_automation::WindowInteractionState::Closing;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentWindowInteractionState(&windowInteractionState));
            VERIFY_ARE_EQUAL(windowInteractionState, WindowInteractionState_Closing);

            // IWindowProvider::WindowVisualState
            WindowVisualState windowVisualState;
            mockAP->VisualStateMockValue = xaml_automation::WindowVisualState::Normal;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentWindowVisualState(&windowVisualState));
            VERIFY_ARE_EQUAL(windowVisualState, WindowVisualState_Normal);

            mockAP->VisualStateMockValue = xaml_automation::WindowVisualState::Minimized;
            VERIFY_SUCCEEDED(spWindowPattern->get_CurrentWindowVisualState(&windowVisualState));
            VERIFY_ARE_EQUAL(windowVisualState, WindowVisualState_Minimized);
        });
    }

} } } } } }

