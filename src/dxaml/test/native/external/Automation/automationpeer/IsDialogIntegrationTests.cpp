// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <IsDialogIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    struct IsDialogTestData
    {
        bool isDialogSupplied;
        bool isDialog;
    };

    IsDialogTestData g_TestData[] =
    {
        // TestCase                                 SupplyIP  IsDialog
        /* IsDialogSupplied_True */                 { true,   true,   },
        /* IsDialogSupplied_False */                { true,   false,  },
        /* IsDialogNotSupplied */                   { false,  false,  },
    };

    bool IsDialogIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool IsDialogIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool IsDialogIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void IsDialogIntegrationTests::VerifyIsDialogSupplied_True()
    {
        _VerifyAutomationPropertiesMarkup(IsDialogSupplied_True);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsDialogSupplied_True);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsDialogSupplied_True);
    }

    void IsDialogIntegrationTests::VerifyIsDialogSupplied_False()
    {
        _VerifyAutomationPropertiesMarkup(IsDialogSupplied_False);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsDialogSupplied_False);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsDialogSupplied_False);
    }

    void IsDialogIntegrationTests::VerifyIsDialogNotSupplied()
    {
        _VerifyAutomationPropertiesMarkup(IsDialogNotSupplied);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(IsDialogNotSupplied);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(IsDialogNotSupplied);
    }

    void IsDialogIntegrationTests::_VerifyAutomationPropertiesMarkup(TestCase testCase)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsDialog added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsDialogButton";
        uiaInfoTarget.m_AutomationID = L"IsDialogButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='IsDialogButton' x:Name='IsDialogButton'";

            if (g_TestData[testCase].isDialogSupplied)
            {
                 xaml += " AutomationProperties.IsDialog='" + g_TestData[testCase].isDialog + "'";
            }

            xaml += " />"
                "</StackPanel>";

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            _VerifyClient(testCase, uiaInfoTarget);
        });
    }

    void IsDialogIntegrationTests::_VerifyAutomationPropertiesCodebehind(TestCase testCase)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsDialog added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsDialogButton";
        uiaInfoTarget.m_AutomationID = L"IsDialogButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            if (g_TestData[testCase].isDialogSupplied)
            {
                xaml_automation::AutomationProperties::SetIsDialog(button, g_TestData[testCase].isDialog);
            }

            stackPanel->Children->Append(button);
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            _VerifyClient(testCase, uiaInfoTarget);
        });
    }

    void IsDialogIntegrationTests::_VerifyAutomationPeer(TestCase testCase)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying IsDialog added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"IsDialogButton";
        uiaInfoTarget.m_AutomationID = L"IsDialoglButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton(testCase);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            stackPanel->Children->Append(button);
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            _VerifyClient(testCase, uiaInfoTarget);
        });
    }

    void IsDialogIntegrationTests::_VerifyClient(TestCase testCase, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget)
    {
        auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
        wrl::ComPtr<IUIAutomationElement> element;
        spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&element);
        WEX::Common::Throw::IfNull(element.Get());

        wrl::ComPtr<IUIAutomationElement9> element9;
        VERIFY_SUCCEEDED(element.Get()->QueryInterface(__uuidof(IUIAutomationElement9), &element9));

        BOOL isDialog = FALSE;
        VERIFY_SUCCEEDED(element9->get_CurrentIsDialog(&isDialog));

        if (g_TestData[testCase].isDialogSupplied)
        {
            VERIFY_ARE_EQUAL(!!isDialog, g_TestData[testCase].isDialog);
        }
        else
        {
            VERIFY_ARE_EQUAL(!!isDialog, false);
        }
    }

    /*static*/ bool IsDialogIntegrationTests::_IsDialogCore(TestCase testCase)
    {
        if (g_TestData[testCase].isDialogSupplied)
        {
            return g_TestData[testCase].isDialog;
        }
        return false;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
