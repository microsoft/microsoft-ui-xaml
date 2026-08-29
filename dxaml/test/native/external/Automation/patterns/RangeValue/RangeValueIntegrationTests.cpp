// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RangeValueIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>

#include <XamlTailored.h>

#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace RangeValue {

    bool RangeValueIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool RangeValueIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool RangeValueIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void RangeValueIntegrationTests::VerifyRangeValueProperties()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Slider^ slider;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestRangeValueProperties";
        uiaInfo.m_AutomationID = L"TestRangeValueProperties";
        uiaInfo.m_cType = UIA_SliderControlTypeId;
        RunOnUIThread([&]()
        {
            slider = ref new xaml_controls::Slider();
            slider->Height = 150;
            slider->Width = 150;
            slider->SmallChange = 10;
            slider->LargeChange = 20;
            slider->Minimum = 0;
            slider->Maximum = 100;
            slider->Value = 50;
            xaml_automation::AutomationProperties::SetName(slider, ref new Platform::String(uiaInfo.m_Name));
            
            TestServices::WindowHelper->WindowContent = slider;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            xaml_automation_peers::AutomationPeer^ sliderAP;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationRangeValuePattern> spRangeValuePattern;
            double actualValue = 0.0;
            
            RunOnUIThread([&]()
            {
                sliderAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(slider);
            });
            
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_RangeValuePatternId, __uuidof(IUIAutomationRangeValuePattern), &spRangeValuePattern),
                 L"RangeValueIntegrationTests::VerifyRangeValueProperties: Failed in retreiving RangeValue Pattern.");
            WEX::Common::Throw::IfNull(spRangeValuePattern.Get(), L"RangeValueIntegrationTests::VerifyRangeValueProperties: This Slider doesn't support RangeValue Pattern which is required.");

            VERIFY_SUCCEEDED(spRangeValuePattern->get_CurrentLargeChange(&actualValue));
            VERIFY_ARE_EQUAL(actualValue, 20.0);

            VERIFY_SUCCEEDED(spRangeValuePattern->get_CurrentSmallChange(&actualValue));
            VERIFY_ARE_EQUAL(actualValue, 10.0);

            VERIFY_SUCCEEDED(spRangeValuePattern->get_CurrentMinimum(&actualValue));
            VERIFY_ARE_EQUAL(actualValue, 0.0);

            VERIFY_SUCCEEDED(spRangeValuePattern->get_CurrentMaximum(&actualValue));
            VERIFY_ARE_EQUAL(actualValue, 100.0);

            VERIFY_SUCCEEDED(spRangeValuePattern->get_CurrentValue(&actualValue));
            VERIFY_ARE_EQUAL(actualValue, 50.0);
        });
    }

} } } } } }
