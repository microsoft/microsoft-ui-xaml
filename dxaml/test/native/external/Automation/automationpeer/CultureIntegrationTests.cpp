// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <CultureIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    struct CultureTestData
    {
        bool fSupplyCulture;
        int culture;
    };

    CultureTestData g_TestData[] =
    {
        // TestCase                     SupplyCulture  Culture
        /* CultureSet */                { true,         123 },
        /* CultureNotSet */             { false,        0x409 /* Default culture */ },
    };

    bool CultureIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool CultureIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool CultureIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void CultureIntegrationTests::VerifyCultureSet()
    {
        _VerifyAutomationPropertiesMarkup(CultureSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(CultureSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(CultureSet);
    }

    void CultureIntegrationTests::VerifyCultureNotSet()
    {
        _VerifyAutomationPropertiesMarkup(CultureNotSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(CultureNotSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(CultureNotSet);
    }


    void CultureIntegrationTests::_VerifyAutomationPropertiesMarkup(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying Culture added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"CultureButton";
        uiaInfoTarget.m_AutomationID = L"CultureButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='CultureButton' x:Name='CultureButton'";

            if (g_TestData[test].fSupplyCulture)
            {
                xaml += " AutomationProperties.Culture='" + g_TestData[test].culture.ToString() + "'";
            }

            xaml += " />"
                "</StackPanel>";

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            _VerifyClient(test, uiaInfoTarget);
        });
    }

    void CultureIntegrationTests::_VerifyAutomationPropertiesCodebehind(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying Culture added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"CultureButton";
        uiaInfoTarget.m_AutomationID = L"CultureButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            if (g_TestData[test].fSupplyCulture)
            {
                xaml_automation::AutomationProperties::SetCulture(button, g_TestData[test].culture);
            }

            stackPanel->Children->Append(button);
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            _VerifyClient(test, uiaInfoTarget);
        });
    }

    void CultureIntegrationTests::_VerifyAutomationPeer(TestCase test)
    {

        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying Culture added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"CultureButton";
        uiaInfoTarget.m_AutomationID = L"CultureButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel =  ref new xaml_controls::StackPanel;
            button = ref new CustomButton(test);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            stackPanel->Children->Append(button);
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            _VerifyClient(test, uiaInfoTarget);
        });
    }

    void CultureIntegrationTests::_VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget)
    {
        auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
        spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
        WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

        int culture = 0;
        VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentCulture(&culture));

        LOG_OUTPUT(L"Actual Culture: %d, Expected Culture: %d", culture, g_TestData[test].culture);
        VERIFY_ARE_EQUAL(culture, g_TestData[test].culture);
    }

    /*static*/ int CultureIntegrationTests::_GetCultureCore(TestCase test)
    {
        return g_TestData[test].culture;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
