// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomNavigationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>

#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>
#include <array>
#include <MockCustomNavigationPatternObject.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Automation::Patterns;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace CustomNavigation {



    bool CustomNavigationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool CustomNavigationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool CustomNavigationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void CustomNavigationIntegrationTests::TestCustomNavigationPatternAndElementProperties()
    {
        TestCleanupWrapper cleanup;
        xaml::Controls::Grid^ container;
        MockCustomNavigationProviderControl^ testCustomNav = nullptr;
        Platform::Object^ obj = nullptr;
        Platform::Object^ result = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestMock";
        uiaInfo.m_AutomationID = L"TestMock";
        uiaInfo.m_cType = UIA_ListItemControlTypeId;
        RunOnUIThread([&]()
        {
            container = ref new xaml::Controls::Grid();
            testCustomNav = ref new MockCustomNavigationProviderControl(-1, -1, -1);
            xaml_automation::AutomationProperties::SetName(testCustomNav, ref new Platform::String(uiaInfo.m_Name));
            testCustomNav->Name = "TestMock";
            testCustomNav->FontSize = 30.0;
            WEX::Logging::Log::Comment(L"Adding control to UI");
            container->Children->Append(testCustomNav);
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::Grid>(container);
        TestServices::WindowHelper->WaitForIdle();
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            MockCustomNavigationProviderControlAutomationPeer^ mockAP;
            xaml_automation_peers::AutomationPeer^ mockAPAsAutomationPeer;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            wrl::ComPtr<IUIAutomationCustomNavigationPattern> spUICustomNavigationPattern;


            WEX::Logging::Log::Comment(L"Executing test on UI thread");
            RunOnUIThread([&]()
            {
                mockAPAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(testCustomNav);
            });
            VERIFY_IS_NOT_NULL(mockAPAsAutomationPeer);
            mockAP = static_cast<MockCustomNavigationProviderControlAutomationPeer^>(mockAPAsAutomationPeer);
            VERIFY_IS_NOT_NULL(mockAP);

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());
            VERIFY_SUCCEEDED(spUIAutomationElement.As(&spUIAutomationElement4));
            VERIFY_IS_NOT_NULL(spUIAutomationElement4.Get());

            WEX::Logging::Log::Comment(L"Test CustomNavigation pattern method");
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_CustomNavigationPatternId, __uuidof(IUIAutomationCustomNavigationPattern), &spUICustomNavigationPattern),
                 L"CustomNavigationIntegrationTests::TestCustomNavigationPatternAndElementProperties: Failed in fetching Custom Navigation Pattern.");
            WEX::Common::Throw::IfNull(spUICustomNavigationPattern.Get(), L"CustomNavigationIntegrationTests::TestCustomNavigationPatternAndElementProperties: This element doesn't support Custom Navigation Pattern which is required.");

            int nPositionInSet = -1;
            int nSizeOfSet = -1;
            int nLevel = -1;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentPositionInSet(&nPositionInSet));
            VERIFY_ARE_EQUAL(nPositionInSet, 0);
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentSizeOfSet(&nSizeOfSet));
            VERIFY_ARE_EQUAL(nSizeOfSet, 3);
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentLevel(&nLevel));
            VERIFY_ARE_EQUAL(nLevel, 0);


            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementChild1;
            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Child1;
            wrl::ComPtr<IUIAutomationCustomNavigationPattern> spUICustomNavigationPatternChild1;
            VERIFY_SUCCEEDED(spUICustomNavigationPattern->Navigate(NavigateDirection_FirstChild, spUIAutomationElementChild1.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationElementChild1.Get());
            VERIFY_SUCCEEDED(spUIAutomationElementChild1.As(&spUIAutomationElement4Child1));
            VERIFY_IS_NOT_NULL(spUIAutomationElement4Child1.Get());

            VERIFY_SUCCEEDED(spUIAutomationElement4Child1->get_CurrentPositionInSet(&nPositionInSet));
            VERIFY_ARE_EQUAL(nPositionInSet, 0);
            VERIFY_SUCCEEDED(spUIAutomationElement4Child1->get_CurrentSizeOfSet(&nSizeOfSet));
            VERIFY_ARE_EQUAL(nSizeOfSet, 3);
            VERIFY_SUCCEEDED(spUIAutomationElement4Child1->get_CurrentLevel(&nLevel));
            VERIFY_ARE_EQUAL(nLevel, 1);

            WEX::Logging::Log::Comment(L"Test CustomNavigation method");
            LogThrow_IfFailedWithMessage(spUIAutomationElementChild1->GetCurrentPatternAs(UIA_CustomNavigationPatternId, __uuidof(IUIAutomationCustomNavigationPattern), &spUICustomNavigationPatternChild1),
                 L"CustomNavigationIntegrationTests::TestCustomNavigationPatternAndElementProperties: Failed in fetching Custom Navigation Pattern.");
            WEX::Common::Throw::IfNull(spUICustomNavigationPatternChild1.Get(), L"CustomNavigationIntegrationTests::TestCustomNavigationPatternAndElementProperties: This element doesn't support Custom Navigation Pattern which is required.");

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementSibling1;
            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Sibling1;

            VERIFY_SUCCEEDED(spUICustomNavigationPatternChild1->Navigate(NavigateDirection_NextSibling, spUIAutomationElementSibling1.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationElementSibling1.Get());
            VERIFY_SUCCEEDED(spUIAutomationElementSibling1.As(&spUIAutomationElement4Sibling1));
            VERIFY_IS_NOT_NULL(spUIAutomationElement4Sibling1.Get());

            VERIFY_SUCCEEDED(spUIAutomationElement4Sibling1->get_CurrentPositionInSet(&nPositionInSet));
            VERIFY_ARE_EQUAL(nPositionInSet, 1);
            VERIFY_SUCCEEDED(spUIAutomationElement4Sibling1->get_CurrentSizeOfSet(&nSizeOfSet));
            VERIFY_ARE_EQUAL(nSizeOfSet, 3);
            VERIFY_SUCCEEDED(spUIAutomationElement4Sibling1->get_CurrentLevel(&nLevel));
            VERIFY_ARE_EQUAL(nLevel, 1);
        });
    }

} } } } } }
