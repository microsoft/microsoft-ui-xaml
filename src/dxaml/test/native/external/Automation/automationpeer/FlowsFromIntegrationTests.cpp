// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <FlowsFromIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    bool FlowsFromIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool FlowsFromIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool FlowsFromIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Case Wrappers
    //
    void FlowsFromIntegrationTests::VerifyFlowsFromNotSet()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"Button";
        uiaInfo.m_AutomationID = L"Button";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement2> spUIAutomationElement2Target;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement2), &spUIAutomationElement2Target));

            wrl::ComPtr<IUIAutomationElementArray> spFlowsFromObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement2Target->get_CurrentFlowsFrom(&spFlowsFromObjects));
            if (spFlowsFromObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsFromObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spFlowsFromObjects.Get() == nullptr || length == 0);
        });
    }

    void FlowsFromIntegrationTests::VerifyFlowsFromEmptyList()
    {
        _VerifyFlowsFromEmptyList_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyFlowsFromEmptyList_AutomationPeer();
    }

    void FlowsFromIntegrationTests::VerifyFlowsFromOneElement()
    {
        _VerifyFlowsFromOneElement_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyFlowsFromOneElement_AutomationPeer();
    }

    void FlowsFromIntegrationTests::VerifyFlowsFromMultipleElements()
    {
        _VerifyFlowsFromMultipleElements_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyFlowsFromMultipleElements_AutomationPeer();
    }

    //
    // Test Cases
    //
    void FlowsFromIntegrationTests::_VerifyFlowsFromEmptyList_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsFrom added from AutomationProperties in codebehind");

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"FlowsFromButton";
        uiaInfo.m_AutomationID = L"FlowsFromButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            auto flowsFromVector = xaml_automation::AutomationProperties::GetFlowsFrom(button);
            VERIFY_IS_NOT_NULL(flowsFromVector);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement2> spUIAutomationElement2Target;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement2), &spUIAutomationElement2Target));

            wrl::ComPtr<IUIAutomationElementArray> spFlowsFromObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement2Target->get_CurrentFlowsFrom(&spFlowsFromObjects));
            if (spFlowsFromObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsFromObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spFlowsFromObjects.Get() == nullptr || length == 0);
        });
    }

    void FlowsFromIntegrationTests::_VerifyFlowsFromEmptyList_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsFrom added from AutomationPeer");

        CustomButton^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"FlowsFromButton";
        uiaInfo.m_AutomationID = L"FlowsFromButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new CustomButton;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
            button->SetTestCase(FlowsFromEmptyList);

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(button);
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement2> spUIAutomationElement2Target;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement2), &spUIAutomationElement2Target));

            wrl::ComPtr<IUIAutomationElementArray> spFlowsFromObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement2Target->get_CurrentFlowsFrom(&spFlowsFromObjects));
            if (spFlowsFromObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsFromObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spFlowsFromObjects.Get() == nullptr || length == 0);
        });
    }

    void FlowsFromIntegrationTests::_VerifyFlowsFromOneElement_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsFrom added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FlowsFromButton";
        uiaInfoTarget.m_AutomationID = L"FlowsFromButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoFlowsFrom;
        uiaInfoFlowsFrom.m_Name = L"FlowsFromTextBlock";
        uiaInfoFlowsFrom.m_AutomationID = L"FlowsFromTextBlock";
        uiaInfoFlowsFrom.m_cType = UIA_TextControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;
            textBlock = ref new xaml_controls::TextBlock;

            auto flowsFromVector = xaml_automation::AutomationProperties::GetFlowsFrom(button);
            VERIFY_IS_NOT_NULL(flowsFromVector);
            flowsFromVector->Append(textBlock);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfoFlowsFrom.m_Name));
            textBlock->Name = ref new Platform::String(uiaInfoFlowsFrom.m_Name);

            stackPanel->Children->Append(button);
            stackPanel->Children->Append(textBlock);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            auto spAutomationClientManagerFlowsFrom = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoFlowsFrom);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementFlowsFrom;
            spAutomationClientManagerFlowsFrom->GetCurrentUIAutomationElement(&spUIAutomationElementFlowsFrom);
            WEX::Common::Throw::IfNull(spUIAutomationElementFlowsFrom.Get());

            wrl::ComPtr<IUIAutomationElement2> spUIAutomationElement2Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement2), &spUIAutomationElement2Target));

            wrl::ComPtr<IUIAutomationElementArray> spFlowsFromObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement2Target->get_CurrentFlowsFrom(&spFlowsFromObjects));
            if (spFlowsFromObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsFromObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, 1);

                wrl::ComPtr<IUIAutomationElement> spElement;
                VERIFY_SUCCEEDED(spFlowsFromObjects->GetElement(0, &spElement));
                WEX::Common::Throw::IfNull(spElement.Get());
                VERIFY_IS_TRUE(spAutomationClientManagerFlowsFrom->IsElementSame(spElement.Get()));
            }
            else
            {
                VERIFY_IS_NOT_NULL(spFlowsFromObjects.Get());
            }
        });
    }

    void FlowsFromIntegrationTests::_VerifyFlowsFromOneElement_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsFrom added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        CustomButton^ button;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FlowsFromButton";
        uiaInfoTarget.m_AutomationID = L"FlowsFromButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoFlowsFrom;
        uiaInfoFlowsFrom.m_Name = L"FlowsFromTextBlock";
        uiaInfoFlowsFrom.m_AutomationID = L"FlowsFromTextBlock";
        uiaInfoFlowsFrom.m_cType = UIA_TextControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton;
            textBlock = ref new xaml_controls::TextBlock;

            wfc::IVector<xaml::DependencyObject^>^ flowsFrom = ref new Platform::Collections::Vector<xaml::DependencyObject^>;
            flowsFrom->Append(textBlock);
            button->SetAPFlowsFromObjects(flowsFrom);
            button->SetTestCase(FlowsFromOneElement);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfoFlowsFrom.m_Name));
            textBlock->Name = ref new Platform::String(uiaInfoFlowsFrom.m_Name);

            stackPanel->Children->Append(button);
            stackPanel->Children->Append(textBlock);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            auto spAutomationClientManagerFlowsFrom = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoFlowsFrom);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementFlowsFrom;
            spAutomationClientManagerFlowsFrom->GetCurrentUIAutomationElement(&spUIAutomationElementFlowsFrom);
            WEX::Common::Throw::IfNull(spUIAutomationElementFlowsFrom.Get());

            wrl::ComPtr<IUIAutomationElement2> spUIAutomationElement2Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement2), &spUIAutomationElement2Target));

            wrl::ComPtr<IUIAutomationElementArray> spFlowsFromObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement2Target->get_CurrentFlowsFrom(&spFlowsFromObjects));
            if (spFlowsFromObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsFromObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, 1);

                wrl::ComPtr<IUIAutomationElement> spElement;
                VERIFY_SUCCEEDED(spFlowsFromObjects->GetElement(0, &spElement));
                WEX::Common::Throw::IfNull(spElement.Get());
                VERIFY_IS_TRUE(spAutomationClientManagerFlowsFrom->IsElementSame(spElement.Get()));
            }
            else
            {
                VERIFY_IS_NOT_NULL(spFlowsFromObjects.Get());
            }
        });
    }

    void FlowsFromIntegrationTests::_VerifyFlowsFromMultipleElements_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsFrom added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FlowsFromButton";
        uiaInfoTarget.m_AutomationID = L"FlowsFromButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[4];
        std::vector<Automation::AutomationClient::UIAElementInfo> flowsFromElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoFlowsFrom;
            pszNames[i] = new WCHAR[wcslen(L"FlowsFromTextBlockX") + 1];
            StringCchPrintf(pszNames[i], wcslen(L"FlowsFromTextBlockX") + 1, L"FlowsFromTextBlock%d", i + 1);
            uiaInfoFlowsFrom.m_Name = pszNames[i];
            uiaInfoFlowsFrom.m_AutomationID = pszNames[i];
            uiaInfoFlowsFrom.m_cType = UIA_TextControlTypeId;
            flowsFromElementInfos.push_back(uiaInfoFlowsFrom);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            auto flowsFromVector = xaml_automation::AutomationProperties::GetFlowsFrom(button);

            VERIFY_IS_NOT_NULL(flowsFromVector);

            for (size_t i = 0; i < flowsFromElementInfos.size(); i++)
            {
                xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(flowsFromElementInfos[i].m_Name));
                textBlock->Name = ref new Platform::String(flowsFromElementInfos[i].m_Name);
                stackPanel->Children->Append(textBlock);
                flowsFromVector->Append(textBlock);
            }

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            stackPanel->Children->Append(button);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            wrl::ComPtr<IUIAutomationElement2> spUIAutomationElement2Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement2), &spUIAutomationElement2Target));

            wrl::ComPtr<IUIAutomationElementArray> spFlowsFromObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement2Target->get_CurrentFlowsFrom(&spFlowsFromObjects));
            if (spFlowsFromObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsFromObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)flowsFromElementInfos.size());

                for (LONG i = 0; i < (LONG)flowsFromElementInfos.size(); i++)
                {
                    auto spAutomationClientManagerFlowsFrom = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(flowsFromElementInfos[i]);
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElementFlowsFrom;
                    spAutomationClientManagerFlowsFrom->GetCurrentUIAutomationElement(&spUIAutomationElementFlowsFrom);
                    WEX::Common::Throw::IfNull(spUIAutomationElementFlowsFrom.Get());

                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spFlowsFromObjects->GetElement(i, &spElement));
                    WEX::Common::Throw::IfNull(spElement.Get());
                    VERIFY_IS_TRUE(spAutomationClientManagerFlowsFrom->IsElementSame(spElement.Get()));
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spFlowsFromObjects.Get());
            }
        });

        for (int i = 0; i < 4; i++)
        {
            delete[] pszNames[i];
        }
    }

    void FlowsFromIntegrationTests::_VerifyFlowsFromMultipleElements_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsFrom added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        CustomButton^ button;
        wfc::IVector<xaml::DependencyObject^>^ textBlocks;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FlowsFromButton";
        uiaInfoTarget.m_AutomationID = L"FlowsFromButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[4];
        std::vector<Automation::AutomationClient::UIAElementInfo> flowsFromElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoFlowsFrom;
            pszNames[i] = new WCHAR[wcslen(L"FlowsFromTextBlockX") + 1];
            StringCchPrintf(pszNames[i], wcslen(L"FlowsFromTextBlockX") + 1, L"FlowsFromTextBlock%d", i + 1);
            uiaInfoFlowsFrom.m_Name = pszNames[i];
            uiaInfoFlowsFrom.m_AutomationID = pszNames[i];
            uiaInfoFlowsFrom.m_cType = UIA_TextControlTypeId;
            flowsFromElementInfos.push_back(uiaInfoFlowsFrom);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton;
            textBlocks = ref new Platform::Collections::Vector<xaml::DependencyObject^>;

            for (size_t i = 0; i < flowsFromElementInfos.size(); i++)
            {
                xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(flowsFromElementInfos[i].m_Name));
                textBlock->Name = ref new Platform::String(flowsFromElementInfos[i].m_Name);
                textBlocks->Append(textBlock);
                stackPanel->Children->Append(textBlock);
            }

            button->SetAPFlowsFromObjects(textBlocks);
            button->SetTestCase(FlowsFromMultipleElements);
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            stackPanel->Children->Append(button);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            wrl::ComPtr<IUIAutomationElement2> spUIAutomationElement2Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement2), &spUIAutomationElement2Target));

            wrl::ComPtr<IUIAutomationElementArray> spFlowsFromObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement2Target->get_CurrentFlowsFrom(&spFlowsFromObjects));
            if (spFlowsFromObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsFromObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)flowsFromElementInfos.size());

                for (LONG i = 0; i < (LONG)flowsFromElementInfos.size(); i++)
                {
                    auto spAutomationClientManagerFlowsFrom = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(flowsFromElementInfos[i]);
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElementFlowsFrom;
                    spAutomationClientManagerFlowsFrom->GetCurrentUIAutomationElement(&spUIAutomationElementFlowsFrom);
                    WEX::Common::Throw::IfNull(spUIAutomationElementFlowsFrom.Get());

                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spFlowsFromObjects->GetElement(i, &spElement));
                    WEX::Common::Throw::IfNull(spElement.Get());
                    VERIFY_IS_TRUE(spAutomationClientManagerFlowsFrom->IsElementSame(spElement.Get()));
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spFlowsFromObjects.Get());
            }
        });

        for (int i = 0; i < 4; i++)
        {
            delete[] pszNames[i];
        }
    }

    wfc::IIterable<xaml_automation_peers::AutomationPeer^>^ FlowsFromIntegrationTests::CustomButtonAP::GetFlowsFromCore()
    {
        wfc::IVector<xaml_automation_peers::AutomationPeer^>^ flowsFrom =
            ref new Platform::Collections::Vector<xaml_automation_peers::AutomationPeer^>;

        switch (m_tc)
        {
            case FlowsFromOneElement:
            case FlowsFromMultipleElements:
                for (unsigned int i = 0; i < m_objects->Size; i++)
                {
                    flowsFrom->Append(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement((UIElement^)m_objects->GetAt(i)));
                }
                break;
        }

        return flowsFrom;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
