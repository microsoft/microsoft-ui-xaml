// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ButtonAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>

#include <generic\ButtonBaseTests.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <Patterns\InvokePatternHandler.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Button {

    bool ButtonAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ButtonAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ButtonAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ButtonAutomationIntegrationTests::SetupButton(Automation::AutomationClient::UIAElementInfo uiaInfo)
    {
        xaml_controls::Button^ button;

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button();
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
        });
        TreeHelper::AddElementIntoLivetree<xaml_controls::Button>(button);
    }

    //
    // Test Cases
    //
    void ButtonAutomationIntegrationTests::DoesSupportEssentialPatterns()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestButton";
        uiaInfo.m_AutomationID = L"TestButton";
        uiaInfo.m_ItemStatus = L"TestButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        SetupButton(uiaInfo);

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
        AutomationClient::AutomationGenericTests automationGenericTests(spAutomationClientManager);
        automationGenericTests.DoesSupportEssentialPatternsForControlType(uiaInfo.m_cType);
    }

    void ButtonAutomationIntegrationTests::CanBeInvokedByUIA()
    {
        Generic::ButtonBaseTests<xaml_controls::Button>::CanClickUsingUIA();
    }

    void ButtonAutomationIntegrationTests::CanRaiseUIAInvokeEventE2E()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestButton";
        uiaInfo.m_AutomationID = L"TestButton";
        uiaInfo.m_ItemStatus = L"TestButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        SetupButton(uiaInfo);

        wrl::ComPtr<Patterns::InvokePatternHandler> spAutomationInvokePatternHandler;
        auto invokeEvent = std::make_shared<Event>();
        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
        spAutomationInvokePatternHandler.Attach(new Patterns::InvokePatternHandler(spAutomationClientManager, invokeEvent, TreeScope_Subtree));
        spAutomationInvokePatternHandler->AttachEventHandler();
        spAutomationInvokePatternHandler->Invoke();
        spAutomationInvokePatternHandler->ConfirmAndUnregister();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Button
