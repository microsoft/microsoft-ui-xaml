// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <IsPeripheralIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    struct IsPeriperalTestData
    {
        bool fSupplyIsPeripheral;
        bool fIsPeripheral;
    };

    IsPeriperalTestData g_TestData[] =
    {
        // TestCase                                     SupplyIP  IsPeripheral
        /* IsPeripheralSupplied_True */                 { true,   true,         },
        /* IsPeripheralSupplied_False */                { true,   false,        },
        /* IsPeripheralNotSupplied */                   { false,  false,        },
    };

    bool IsPeripheralIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool IsPeripheralIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool IsPeripheralIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void IsPeripheralIntegrationTests::VerifyIsPeripheralSupplied_True()
    {
        _VerifyAutomationPropertiesMarkup(IsPeripheralSupplied_True);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsPeripheralSupplied_True);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsPeripheralSupplied_True);
    }

    void IsPeripheralIntegrationTests::VerifyIsPeripheralSupplied_False()
    {
        _VerifyAutomationPropertiesMarkup(IsPeripheralSupplied_False);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsPeripheralSupplied_False);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsPeripheralSupplied_False);
    }

    void IsPeripheralIntegrationTests::VerifyIsPeripheralNotSupplied()
    {
        _VerifyAutomationPropertiesMarkup(IsPeripheralNotSupplied);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsPeripheralNotSupplied);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsPeripheralNotSupplied);
    }

    void IsPeripheralIntegrationTests::_VerifyAutomationPropertiesMarkup(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsPeripheral added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsPeripheralButton";
        uiaInfoTarget.m_AutomationID = L"IsPeripheralButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='IsPeripheralButton' x:Name='IsPeripheralButton'";

            if (g_TestData[test].fSupplyIsPeripheral)
            {
                 xaml += " AutomationProperties.IsPeripheral='" + g_TestData[test].fIsPeripheral + "'";
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

    void IsPeripheralIntegrationTests::_VerifyAutomationPropertiesCodebehind(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsPeripheral added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsPeripheralButton";
        uiaInfoTarget.m_AutomationID = L"IsPeripheralButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            if (g_TestData[test].fSupplyIsPeripheral)
            {
                xaml_automation::AutomationProperties::SetIsPeripheral(button, g_TestData[test].fIsPeripheral);
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

    void IsPeripheralIntegrationTests::_VerifyAutomationPeer(TestCase test)
    {

        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsPeripheral added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsPeripheralButton";
        uiaInfoTarget.m_AutomationID = L"IsPeripheralButton";
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

    void IsPeripheralIntegrationTests::_VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget)
    {
        auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
        spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
        WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

        wrl::ComPtr<IUIAutomationElement3> spUIAutomationElement3Target;
        VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement3), &spUIAutomationElement3Target));

        BOOL isPeripheral;
        VERIFY_SUCCEEDED(spUIAutomationElement3Target->get_CurrentIsPeripheral(&isPeripheral));

        if (g_TestData[test].fSupplyIsPeripheral)
        {
            VERIFY_ARE_EQUAL(!!isPeripheral, g_TestData[test].fIsPeripheral);
        }
        else
        {
            VERIFY_ARE_EQUAL(!!isPeripheral, false);
        }
    }

    /*static*/ bool IsPeripheralIntegrationTests::_IsPeripheralCore(TestCase test)
    {
        if (g_TestData[test].fSupplyIsPeripheral)
        {
            return g_TestData[test].fIsPeripheral;
        }
        return false;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
