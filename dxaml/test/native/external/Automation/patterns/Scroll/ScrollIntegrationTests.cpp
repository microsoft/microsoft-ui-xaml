// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>

#include <XamlTailored.h>

#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Scroll {

    bool ScrollIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool ScrollIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ScrollIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ScrollIntegrationTests::VerifyScrollProperties()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ScrollViewer^ scrollViewer;
        xaml_controls::Button^ button;
        wrl::ComPtr<IUIAutomationScrollPattern> spScrollPattern;
        double actualValue = 0.0;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestScrollPatternProperties";
        uiaInfo.m_AutomationID = L"TestScrollPatternProperties";
        uiaInfo.m_cType = UIA_PaneControlTypeId;
        RunOnUIThread([&]()
        {
            scrollViewer = ref new xaml_controls::ScrollViewer();
            button = ref new xaml_controls::Button();
            scrollViewer->Height = 150;
            scrollViewer->Width = 150;
            button->Height = 300;
            button->Width = 300;
            scrollViewer->Content = button;
            scrollViewer->VerticalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Visible;
            scrollViewer->HorizontalScrollBarVisibility = xaml_controls::ScrollBarVisibility::Visible;
            xaml_automation::AutomationProperties::SetName(scrollViewer, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = scrollViewer;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            xaml_automation_peers::AutomationPeer^ scrollViewerAP;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            RunOnUIThread([&]()
            {
                scrollViewerAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(scrollViewer);
            });
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_ScrollPatternId, __uuidof(IUIAutomationScrollPattern), &spScrollPattern), L"ScrollIntegrationTests::VerifyScrollProperties: Failed in retreiving Scroll Pattern.");
            WEX::Common::Throw::IfNull(spScrollPattern.Get(), L"ScrollIntegrationTests::VerifyScrollProperties: This ScrollViewer doesn't support Scroll Pattern which is required.");
            LogThrow_IfFailed(spScrollPattern->SetScrollPercent(50.0, 50.0));

        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VERIFY_SUCCEEDED(spScrollPattern->get_CurrentHorizontalScrollPercent(&actualValue));
            VERIFY_ARE_EQUAL(actualValue, 50.0);

            VERIFY_SUCCEEDED(spScrollPattern->get_CurrentVerticalScrollPercent(&actualValue));
            VERIFY_ARE_EQUAL(actualValue, 50.0);

            VERIFY_SUCCEEDED(spScrollPattern->get_CurrentHorizontalViewSize(&actualValue));
            VERIFY_ARE_EQUAL(actualValue, 50.0);

            VERIFY_SUCCEEDED(spScrollPattern->get_CurrentVerticalViewSize(&actualValue));
            VERIFY_ARE_EQUAL(actualValue, 50.0);
        });
    }

} } } } } }
