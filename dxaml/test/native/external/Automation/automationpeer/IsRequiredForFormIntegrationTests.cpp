// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <IsRequiredForFormIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    struct IsRequiredForFormTestData
    {
        bool supplied;
        bool isRequiredForForm;
    };

    IsRequiredForFormTestData g_TestData[] =
    {
        // TestCase                                     Supplied    IsRequiredForForm
        /* IsRequiredForFormSupplied_True */            { true,     true,       },
        /* IsRequiredForFormSupplied_False */           { true,     false,      },
        /* IsRequiredForFormNotSupplied */              { false,    false,       },
    };

    bool IsRequiredForFormIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool IsRequiredForFormIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool IsRequiredForFormIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void IsRequiredForFormIntegrationTests::VerifyIsRequiredForFormSupplied_True()
    {
        _VerifyAutomationPropertiesMarkup(IsRequiredForFormSupplied_True);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsRequiredForFormSupplied_True);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsRequiredForFormSupplied_True);
    }

    void IsRequiredForFormIntegrationTests::VerifyIsRequiredForFormSupplied_False()
    {
        _VerifyAutomationPropertiesMarkup(IsRequiredForFormSupplied_False);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsRequiredForFormSupplied_False);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsRequiredForFormSupplied_False);
    }

    void IsRequiredForFormIntegrationTests::VerifyIsRequiredForFormNotSupplied()
    {
        _VerifyAutomationPropertiesMarkup(IsRequiredForFormNotSupplied);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsRequiredForFormNotSupplied);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsRequiredForFormNotSupplied);
    }

    void IsRequiredForFormIntegrationTests::_VerifyAutomationPropertiesMarkup(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsRequiredForForm added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsRequiredForFormButton";
        uiaInfoTarget.m_AutomationID = L"IsRequiredForFormButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml=
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='IsRequiredForFormButton' x:Name='IsRequiredForFormButton'";

            if (g_TestData[test].supplied)
            {
                xaml += " AutomationProperties.IsRequiredForForm='" + g_TestData[test].isRequiredForForm + "'";
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

    void IsRequiredForFormIntegrationTests::_VerifyAutomationPropertiesCodebehind(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsRequiredForForm added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsRequiredForFormButton";
        uiaInfoTarget.m_AutomationID = L"IsRequiredForFormButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            if (g_TestData[test].supplied)
            {
                xaml_automation::AutomationProperties::SetIsRequiredForForm(button, g_TestData[test].isRequiredForForm);
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

    void IsRequiredForFormIntegrationTests::_VerifyAutomationPeer(TestCase test)
    {

        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsRequiredForForm added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsRequiredForFormButton";
        uiaInfoTarget.m_AutomationID = L"IsRequiredForFormButton";
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

    void IsRequiredForFormIntegrationTests::_VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget)
    {
        auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
        spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
        WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

        BOOL IsRequiredForForm;
        VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentIsRequiredForForm(&IsRequiredForForm));

        if (g_TestData[test].supplied)
        {
            VERIFY_ARE_EQUAL(!!IsRequiredForForm, g_TestData[test].isRequiredForForm);
        }
        else
        {
            VERIFY_ARE_EQUAL(!!IsRequiredForForm, false); // default is false
        }
    }

    /*static*/ bool IsRequiredForFormIntegrationTests::_IsRequiredForFormCore(TestCase test)
    {
        if (g_TestData[test].supplied)
        {
            return g_TestData[test].isRequiredForForm;
        }
        return false; // default is false
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
