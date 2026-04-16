// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SynchronizedInputIntegrationTests.h"
#include <Patterns\SynchronizedInputPatternHandler.h>

#include <XamlTailored.h>
#include <TestEvent.h>

#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>
#include <array>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace SynchronizedInput {

    bool SynchronizedInputIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool SynchronizedInputIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool SynchronizedInputIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void SynchronizedInputIntegrationTests::CanRaiseSynchronizedInputEvents()
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

        std::array<EVENTID, 3> ids = {
            UIA_InputReachedTargetEventId,
            UIA_InputReachedOtherElementEventId,
            UIA_InputDiscardedEventId
        };

        std::array<xaml_automation_peers::AutomationEvents, 3> events = {
            xaml_automation_peers::AutomationEvents::InputReachedTarget,
            xaml_automation_peers::AutomationEvents::InputReachedOtherElement,
            xaml_automation_peers::AutomationEvents::InputDiscarded
        };

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<Patterns::SynchronizedInputPatternHandler> spAutomationPatternHandler;
            RunOnUIThread([&]()
            {
                buttonAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button);
            });
            std::shared_ptr<AutomationClient::AutomationClientManager> spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            int index = 0;
            for (auto id : ids){
                auto spEvent = std::make_shared<Event>();
                spAutomationPatternHandler.Attach(new Patterns::SynchronizedInputPatternHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, id));
                spAutomationPatternHandler->AttachEventHandler();
                RunOnUIThread([&]()
                {
                    buttonAP->RaiseAutomationEvent(events.at(index++));
                });
                spAutomationPatternHandler->ConfirmAndUnregister();
            }
        });
    }

} } } } } }
