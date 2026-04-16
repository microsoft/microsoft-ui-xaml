// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <LocalizedControlTypeIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    struct LocalizedControlTestData
    {
        bool fSupplyLCT;
        Platform::String^ lct;
    };

    const std::map<CONTROLTYPEID, const wchar_t*> make_controlTypeUIAStringMapping()
    {
        std::map<CONTROLTYPEID, const wchar_t*> m;
        m[UIA_ButtonControlTypeId] = L"button";
        m[UIA_CustomControlTypeId] = L"custom";

        return m;
    };
    const std::map<CONTROLTYPEID, const wchar_t*> c_controlTypeUIAStringMapping = make_controlTypeUIAStringMapping();

    LocalizedControlTestData g_TestData[] =
    {
        // TestCase           SupplyLCT  LocalizedControlType
        /* LCTNotSet    */  { false,     "PropertyNotSet",      },
        /* LCTSet       */  { true,      "CustomControlType",   },
        /* EmptyLCT     */  { true,      "",                    },
    };

    bool LocalizedControlTypeIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool LocalizedControlTypeIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool LocalizedControlTypeIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void LocalizedControlTypeIntegrationTests::VerifyLocalizedControlTypeNotSet()
    {
        _VerifyAutomationPropertiesMarkup(LCTNotSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(LCTNotSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(LCTNotSet);
    }

    void LocalizedControlTypeIntegrationTests::VerifyLocalizedControlTypeSet()
    {
        _VerifyAutomationPropertiesMarkup(LCTSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(LCTSet);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(LCTSet);
    }

    void LocalizedControlTypeIntegrationTests::VerifyEmptyLocalizedControlType()
    {
        _VerifyAutomationPropertiesMarkup(EmptyLCT);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(EmptyLCT);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(EmptyLCT);
    }

    void LocalizedControlTypeIntegrationTests::_VerifyAutomationPropertiesMarkup(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying LocalizedControlType added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"LocalizedControlTypeButton";
        uiaInfoTarget.m_AutomationID = L"LocalizedControlTypeButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='LocalizedControlTypeButton' x:Name='LocalizedControlTypeButton'";

            if (g_TestData[test].fSupplyLCT)
            {
                xaml += " AutomationProperties.LocalizedControlType='" + g_TestData[test].lct + "'";
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

    void LocalizedControlTypeIntegrationTests::_VerifyAutomationPropertiesCodebehind(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying LocalizedControlType added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"LocalizedControlTypeButton";
        uiaInfoTarget.m_AutomationID = L"LocalizedControlTypeButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            if (g_TestData[test].fSupplyLCT)
            {
                xaml_automation::AutomationProperties::SetLocalizedControlType(button, g_TestData[test].lct);
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

    void LocalizedControlTypeIntegrationTests::_VerifyAutomationPeer(TestCase test)
    {

        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying LocalizedControlType added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"LocalizedControlTypeButton";
        uiaInfoTarget.m_AutomationID = L"LocalizedControlTypeButton";
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

    void LocalizedControlTypeIntegrationTests::_VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget)
    {
        auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
        spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
        WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

        BSTR localizedControlType = nullptr;
        CONTROLTYPEID controlType;

        VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentLocalizedControlType(&localizedControlType));
        VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentControlType(&controlType));

        if (g_TestData[test].fSupplyLCT && test != EmptyLCT)
        {
            LOG_OUTPUT(L"localizedControlType: %ws", localizedControlType);
            VERIFY_ARE_EQUAL(0, wcscmp(localizedControlType, g_TestData[test].lct->Data()));
        }
        else
        {
            LOG_OUTPUT(L"localizedControlType: %ws", localizedControlType);
            VERIFY_ARE_EQUAL(0, wcscmp(localizedControlType, c_controlTypeUIAStringMapping.at(controlType)));
        }

        VERIFY_ARE_EQUAL(controlType, uiaInfoTarget.m_cType);

        if (localizedControlType != nullptr)
        {
            SysFreeString(localizedControlType);
        }
    }

    /*static*/ Platform::String^ LocalizedControlTypeIntegrationTests::_GetLocalizedControlTypeCore(TestCase test)
    {
        if (g_TestData[test].fSupplyLCT)
        {
            return g_TestData[test].lct;
        }
        return nullptr;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
