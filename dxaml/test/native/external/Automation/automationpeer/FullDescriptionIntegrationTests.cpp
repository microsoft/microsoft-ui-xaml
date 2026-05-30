// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <FullDescriptionIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    struct FullDescriptionTestData
    {
        bool fSupplyFullDescription;
        Platform::String^ fullDescription;
    };

    FullDescriptionTestData g_TestData[] =
    {
        // TestCase                             SupplyFD    FullDescription
        /* FullDescription */                   { true,     "This is a string!",    },
        /* EmptyString */                       { true,     "",                     },
        /* FullDescriptionNotSet */             { false,    nullptr,                },
    };

    bool FullDescriptionIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool FullDescriptionIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool FullDescriptionIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void FullDescriptionIntegrationTests::VerifyFullDescriptionWithString()
    {
        _VerifyAutomationPropertiesMarkup(FullDescription);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(FullDescription);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(FullDescription);
    }

    void FullDescriptionIntegrationTests::VerifyFullDescriptionEmptyString()
    {
        _VerifyAutomationPropertiesMarkup(EmptyString);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(EmptyString);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(EmptyString);
    }

    void FullDescriptionIntegrationTests::VerifyFullDescriptionNotSet()
    {
        _VerifyAutomationPropertiesMarkup(FullDescriptionNotSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(FullDescriptionNotSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(FullDescriptionNotSet);
    }

    void FullDescriptionIntegrationTests::_VerifyAutomationPropertiesMarkup(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FullDescription added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FullDescriptionButton";
        uiaInfoTarget.m_AutomationID = L"FullDescriptionButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='FullDescriptionButton' x:Name='FullDescriptionButton'";

            if (g_TestData[test].fSupplyFullDescription)
            {
                xaml += " AutomationProperties.FullDescription='" + g_TestData[test].fullDescription + "'";
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

    void FullDescriptionIntegrationTests::_VerifyAutomationPropertiesCodebehind(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FullDescription added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FullDescriptionButton";
        uiaInfoTarget.m_AutomationID = L"FullDescriptionButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            if (g_TestData[test].fSupplyFullDescription)
            {
                xaml_automation::AutomationProperties::SetFullDescription(button, g_TestData[test].fullDescription);
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

    void FullDescriptionIntegrationTests::_VerifyAutomationPeer(TestCase test)
    {

        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FullDescription added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FullDescriptionButton";
        uiaInfoTarget.m_AutomationID = L"FullDescriptionButton";
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

    void FullDescriptionIntegrationTests::_VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget)
    {
        auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
        spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
        WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

        wrl::ComPtr<IUIAutomationElement6> spUIAutomationElement6Target;
        VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement6), &spUIAutomationElement6Target));

        BSTR fullDescription = nullptr;
        VERIFY_SUCCEEDED(spUIAutomationElement6Target->get_CurrentFullDescription(&fullDescription));

        if (g_TestData[test].fSupplyFullDescription)
        {
            LOG_OUTPUT(L"fullDescription: %ws", fullDescription);
            VERIFY_ARE_EQUAL(0, wcscmp(fullDescription, g_TestData[test].fullDescription->Data()));
        }
        else
        {
            LOG_OUTPUT(L"fullDescription: %ws", fullDescription);
            VERIFY_ARE_EQUAL(0, wcscmp(fullDescription, L""));
        }

        if (fullDescription != nullptr)
        {
            SysFreeString(fullDescription);
        }
    }

    /*static*/ Platform::String^ FullDescriptionIntegrationTests::_GetFullDescriptionCore(TestCase test)
    {
        if (g_TestData[test].fSupplyFullDescription)
        {
            return g_TestData[test].fullDescription;
        }
        return nullptr;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
