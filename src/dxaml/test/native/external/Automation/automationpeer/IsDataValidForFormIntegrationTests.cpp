// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <IsDataValidForFormIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    struct IsDataValidTestData
    {
        bool fSupplyIsDataValid;
        bool fIsDataValid;
    };

    IsDataValidTestData g_TestData[] =
    {
        // TestCase                                     SupplyIDV   IsDataValid
        /* IsDataValidSupplied_True */                  { true,     true,       },
        /* IsDataValidSupplied_False */                 { true,     false,      },
        /* IsDataValidNotSupplied */                    { false,    true,       },
    };

    bool IsDataValidForFormIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool IsDataValidForFormIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool IsDataValidForFormIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void IsDataValidForFormIntegrationTests::VerifyIsDataValidSupplied_True()
    {
        _VerifyAutomationPropertiesMarkup(IsDataValidSupplied_True);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsDataValidSupplied_True);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsDataValidSupplied_True);
    }

    void IsDataValidForFormIntegrationTests::VerifyIsDataValidSupplied_False()
    {
        _VerifyAutomationPropertiesMarkup(IsDataValidSupplied_False);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsDataValidSupplied_False);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsDataValidSupplied_False);
    }

    void IsDataValidForFormIntegrationTests::VerifyIsDataValidNotSupplied()
    {
        _VerifyAutomationPropertiesMarkup(IsDataValidNotSupplied);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsDataValidNotSupplied);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsDataValidNotSupplied);
    }

    void IsDataValidForFormIntegrationTests::_VerifyAutomationPropertiesMarkup(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsDataValidForForm added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsDataValidButton";
        uiaInfoTarget.m_AutomationID = L"IsDataValidButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml=
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='IsDataValidButton' x:Name='IsDataValidButton'";

            if (g_TestData[test].fSupplyIsDataValid)
            {
                xaml += " AutomationProperties.IsDataValidForForm='" + g_TestData[test].fIsDataValid + "'";
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

    void IsDataValidForFormIntegrationTests::_VerifyAutomationPropertiesCodebehind(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsDataValidForForm added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsDataValidButton";
        uiaInfoTarget.m_AutomationID = L"IsDataValidButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            if (g_TestData[test].fSupplyIsDataValid)
            {
                xaml_automation::AutomationProperties::SetIsDataValidForForm(button, g_TestData[test].fIsDataValid);
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

    void IsDataValidForFormIntegrationTests::_VerifyAutomationPeer(TestCase test)
    {

        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsDataValidForForm added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsDataValidButton";
        uiaInfoTarget.m_AutomationID = L"IsDataValidButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
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

    void IsDataValidForFormIntegrationTests::_VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget)
    {
        auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
        spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
        WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

        BOOL isDataValidForForm;
        VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentIsDataValidForForm(&isDataValidForForm));

        if (g_TestData[test].fSupplyIsDataValid)
        {
            VERIFY_ARE_EQUAL(!!isDataValidForForm, g_TestData[test].fIsDataValid);
        }
        else
        {
            VERIFY_ARE_EQUAL(!!isDataValidForForm, true);
        }
    }

    /*static*/ bool IsDataValidForFormIntegrationTests::_IsDataValidForFormCore(TestCase test)
    {
        if (g_TestData[test].fSupplyIsDataValid)
        {
            return g_TestData[test].fIsDataValid;
        }
        return true;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
