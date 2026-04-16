// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CalendarDatePickerAutomationIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>

#include <AutomationClient\AutomationGenericTests.h>

#include <XamlTailored.h>
#include <ControlHelper.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace CalendarDatePicker {

    bool CalendarDatePickerAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool CalendarDatePickerAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool CalendarDatePickerAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

     void CalendarDatePickerAutomationIntegrationTests::VerifyCalendarDatePickerUIAValuePattern()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        // Setup
        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <CalendarDatePicker x:Name='CalendarDatePicker' PlaceholderText='PlaceholdText' AutomationProperties.Name='CalendarDatePicker UIAValuePattern'/> "
                L"</StackPanel>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::CalendarDatePicker^ cdp = nullptr;

        // load into visual tree
        RunOnUIThread([&]()
        {
            cdp = safe_cast<xaml_controls::CalendarDatePicker^>(rootPanel->Children->GetAt(0));
        });

        TestServices::WindowHelper->WaitForIdle();

        // Verify the init Value from AutomationPeer interface is PlaceholderText
        ValidateUIAValueIsSameWithDateText(cdp);

        // build and change the date
        ::Windows::Globalization::Calendar^ calendar = ref new ::Windows::Globalization::Calendar();
        auto date = calendar->GetDateTime();

        RunOnUIThread([&]()
        {
            cdp->Date = date;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Verify Value is a new date
        ValidateUIAValueIsSameWithDateText(cdp);
    };

     void CalendarDatePickerAutomationIntegrationTests::VerifyCalendarDatePickerAPLocalizedControlTypeFromPublicAPI()
     {
         TestCleanupWrapper cleanup;
         xaml_controls::StackPanel^ rootPanel = nullptr;

         // Setup
         RunOnUIThread([&]()
         {
             rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                 L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                 L"  <CalendarDatePicker x:Name='CalendarDatePicker' /> "
                 L"</StackPanel>"));

             TestServices::WindowHelper->WindowContent = rootPanel;
         });

         TestServices::WindowHelper->WaitForIdle();

         xaml_automation_peers::AutomationPeer^ ap = nullptr;
         // load into visual tree
         RunOnUIThread([&]()
         {
             auto cdp = safe_cast<xaml_controls::CalendarDatePicker^>(rootPanel->Children->GetAt(0));
             VERIFY_IS_NOT_NULL(cdp);
             ap = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(cdp);
             VERIFY_IS_NOT_NULL(ap);

             auto localizedControlType = ap->GetLocalizedControlType()->Data();
             // The base implementation for AutomationPeer and FrameworkElementAutomationPeer return Custom.
             // Verify we override GetLocalizedControlTypeCore and provided a different string.
             // To avoid a localization problem in the tests, we just check that the string is not 'custom'.
             VERIFY_ARE_NOT_EQUAL(0, _wcsicmp(L"custom", localizedControlType));
         });
    }

    void CalendarDatePickerAutomationIntegrationTests::VerifyDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CalendarDatePicker with Name";

        // Setup
        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <CalendarDatePicker x:Name='CalendarDatePickerWithName' Header='CalendarDatePicker with Name Header' AutomationProperties.Name='CalendarDatePicker with Name'/> "
                L"  <CalendarDatePicker x:Name='CalendarDatePickerWithContent' Header='CalendarDatePicker without Name'/> "
                L"  <CalendarDatePicker x:Name='CalendarDatePickerWithNeither'/> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spCalendarDatePickerWithName;
            wrl::ComPtr<IUIAutomationElement> spCalendarDatePickerWithContent;
            wrl::ComPtr<IUIAutomationElement> spCalendarDatePickerWithNeither;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spCalendarDatePickerWithName);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"CalendarDatePickerIntegrationTests::VerifyDefaultAutomationName: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"CalendarDatePickerIntegrationTests::VerifyDefaultAutomationName: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for CalendarDatePickerWithName exists.");
            VERIFY_IS_NOT_NULL(spCalendarDatePickerWithName);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spCalendarDatePickerWithName.");
            spCalendarDatePickerWithName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the second CalendarDatePicker.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spCalendarDatePickerWithName.Get(), &spCalendarDatePickerWithContent);
            VERIFY_IS_NOT_NULL(spCalendarDatePickerWithContent);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spCalendarDatePickerWithContent.");
            spCalendarDatePickerWithContent->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"CalendarDatePicker without Name", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the third CalendarDatePicker.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spCalendarDatePickerWithContent.Get(), &spCalendarDatePickerWithNeither);
            VERIFY_IS_NOT_NULL(spCalendarDatePickerWithNeither);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spCalendarDatePickerWithNeither.");
            spCalendarDatePickerWithNeither->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"", (autoVar.Storage())->bstrVal));
        });
    }

    void CalendarDatePickerAutomationIntegrationTests::VerifyAutomationControlType()
    {
        TestCleanupWrapper cleanup;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"target";

        RunOnUIThread([&]()
        {
            auto calendarDatePicker = ref new xaml_controls::CalendarDatePicker();
            xaml_automation::AutomationProperties::SetName(calendarDatePicker, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = calendarDatePicker;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomation> uiAutomation;
            automationClientManager->GetAutomation(&uiAutomation);

            wrl::ComPtr<IUIAutomationElement> calendarDatePickerAE;
            automationClientManager->GetCurrentUIAutomationElement(&calendarDatePickerAE);

            wrl::ComPtr<IUIAutomationCondition> uiAutomationTrueCondition;
            LogThrow_IfFailedWithMessage(uiAutomation->CreateTrueCondition(&uiAutomationTrueCondition), L"Failed in creating True PropertyCondition.");

            wrl::ComPtr<IUIAutomationTreeWalker> uiAutomationTreeWalker;
            LogThrow_IfFailedWithMessage(uiAutomation->CreateTreeWalker(uiAutomationTrueCondition.Get(), &uiAutomationTreeWalker), L"Failed in creating TreeWalker.");

            Common::AutoVariant autoVar;
            calendarDatePickerAE->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_ARE_EQUAL(UIA_ButtonControlTypeId, (autoVar.Storage())->lVal);
        });
    };

    void CalendarDatePickerAutomationIntegrationTests::ValidateUIAValueIsSameWithDateText(xaml_controls::CalendarDatePicker^ cdp)
    {
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CalendarDatePicker UIAValuePattern";

        xaml_controls::TextBlock^ dateText = nullptr;
        auto expectedValue = L"";

        RunOnUIThread([&]()
        {
            dateText = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(cdp, L"DateText"));
            VERIFY_IS_NOT_NULL(dateText);
            expectedValue = dateText->Text->Data();
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomation> uiAutomation;
            automationClientManager->GetAutomation(&uiAutomation);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            automationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationValuePattern> spValuePattern;
            LogThrow_IfFailed(spUIAutomationElement->GetCurrentPatternAs(UIA_ValuePatternId, __uuidof(IUIAutomationValuePattern), &spValuePattern));
            WEX::Common::Throw::IfNull(spValuePattern.Get());
            Common::AutoBSTR autoBstr;
            VERIFY_SUCCEEDED(spValuePattern->get_CurrentValue(autoBstr.ReleaseAndGetAddressOf()));
            LOG_OUTPUT(L"Current Value from IUIAutomationValuePattern interface: %ws", autoBstr.Get());
            AutoBSTR::VerifyAreEqual(expectedValue, autoBstr);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::CalendarDatePicker
