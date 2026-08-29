// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <FlowsToIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    bool FlowsToIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool FlowsToIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool FlowsToIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Case Wrappers
    //
    void FlowsToIntegrationTests::VerifyFlowsToNotSet()
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

            wrl::ComPtr<IUIAutomationElementArray> spFlowsToObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement->get_CurrentFlowsTo(&spFlowsToObjects));
            if (spFlowsToObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsToObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spFlowsToObjects.Get() == nullptr || length == 0);
        });
    }

    void FlowsToIntegrationTests::VerifyFlowsToEmptyList()
    {
        _VerifyFlowsToEmptyList_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyFlowsToEmptyList_AutomationPeer();
    }

    void FlowsToIntegrationTests::VerifyFlowsToOneElement()
    {
        _VerifyFlowsToOneElement_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyFlowsToOneElement_AutomationPeer();
    }

    void FlowsToIntegrationTests::VerifyFlowsToMultipleElements()
    {
        _VerifyFlowsToMultipleElements_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyFlowsToMultipleElements_AutomationPeer();
    }

    //
    // Test Cases
    //
    void FlowsToIntegrationTests::_VerifyFlowsToEmptyList_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsTo added from AutomationProperties in codebehind");

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"FlowsToButton";
        uiaInfo.m_AutomationID = L"FlowsToButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            auto flowsToVector = xaml_automation::AutomationProperties::GetFlowsTo(button);
            VERIFY_IS_NOT_NULL(flowsToVector);

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

            wrl::ComPtr<IUIAutomationElementArray> spFlowsToObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement->get_CurrentFlowsTo(&spFlowsToObjects));
            if (spFlowsToObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsToObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spFlowsToObjects.Get() == nullptr || length == 0);
        });
    }

    void FlowsToIntegrationTests::_VerifyFlowsToEmptyList_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsTo added from AutomationPeer");

        CustomButton^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"FlowsToButton";
        uiaInfo.m_AutomationID = L"FlowsToButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new CustomButton;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
            button->SetTestCase(FlowsToEmptyList);

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(button);
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElementArray> spFlowsToObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement->get_CurrentFlowsTo(&spFlowsToObjects));
            if (spFlowsToObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsToObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spFlowsToObjects.Get() == nullptr || length == 0);
        });
    }

    void FlowsToIntegrationTests::_VerifyFlowsToOneElement_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsTo added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FlowsToButton";
        uiaInfoTarget.m_AutomationID = L"FlowsToButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoFlowsTo;
        uiaInfoFlowsTo.m_Name = L"FlowsToTextBlock";
        uiaInfoFlowsTo.m_AutomationID = L"FlowsToTextBlock";
        uiaInfoFlowsTo.m_cType = UIA_TextControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;
            textBlock = ref new xaml_controls::TextBlock;

            auto flowsToVector = xaml_automation::AutomationProperties::GetFlowsTo(button);
            VERIFY_IS_NOT_NULL(flowsToVector);
            flowsToVector->Append(textBlock);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfoFlowsTo.m_Name));
            textBlock->Name = ref new Platform::String(uiaInfoFlowsTo.m_Name);

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

            auto spAutomationClientManagerFlowsTo = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoFlowsTo);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementFlowsTo;
            spAutomationClientManagerFlowsTo->GetCurrentUIAutomationElement(&spUIAutomationElementFlowsTo);
            WEX::Common::Throw::IfNull(spUIAutomationElementFlowsTo.Get());

            wrl::ComPtr<IUIAutomationElementArray> spFlowsToObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentFlowsTo(&spFlowsToObjects));
            if (spFlowsToObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsToObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, 1);

                wrl::ComPtr<IUIAutomationElement> spElement;
                VERIFY_SUCCEEDED(spFlowsToObjects->GetElement(0, &spElement));
                WEX::Common::Throw::IfNull(spElement.Get());
                VERIFY_IS_TRUE(spAutomationClientManagerFlowsTo->IsElementSame(spElement.Get()));
            }
            else
            {
                VERIFY_IS_NOT_NULL(spFlowsToObjects.Get());
            }
        });
    }

    void FlowsToIntegrationTests::_VerifyFlowsToOneElement_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsTo added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        CustomButton^ button;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FlowsToButton";
        uiaInfoTarget.m_AutomationID = L"FlowsToButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoFlowsTo;
        uiaInfoFlowsTo.m_Name = L"FlowsToTextBlock";
        uiaInfoFlowsTo.m_AutomationID = L"FlowsToTextBlock";
        uiaInfoFlowsTo.m_cType = UIA_TextControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton;
            textBlock = ref new xaml_controls::TextBlock;

            wfc::IVector<xaml::DependencyObject^>^ flowsTo = ref new Platform::Collections::Vector<xaml::DependencyObject^>;
            flowsTo->Append(textBlock);
            button->SetAPFlowsToObjects(flowsTo);
            button->SetTestCase(FlowsToOneElement);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfoFlowsTo.m_Name));
            textBlock->Name = ref new Platform::String(uiaInfoFlowsTo.m_Name);

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

            auto spAutomationClientManagerFlowsTo = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoFlowsTo);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementFlowsTo;
            spAutomationClientManagerFlowsTo->GetCurrentUIAutomationElement(&spUIAutomationElementFlowsTo);
            WEX::Common::Throw::IfNull(spUIAutomationElementFlowsTo.Get());

            wrl::ComPtr<IUIAutomationElementArray> spFlowsToObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentFlowsTo(&spFlowsToObjects));
            if (spFlowsToObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsToObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, 1);

                wrl::ComPtr<IUIAutomationElement> spElement;
                VERIFY_SUCCEEDED(spFlowsToObjects->GetElement(0, &spElement));
                WEX::Common::Throw::IfNull(spElement.Get());
                VERIFY_IS_TRUE(spAutomationClientManagerFlowsTo->IsElementSame(spElement.Get()));
            }
            else
            {
                VERIFY_IS_NOT_NULL(spFlowsToObjects.Get());
            }
        });
    }

    void FlowsToIntegrationTests::_VerifyFlowsToMultipleElements_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsTo added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FlowsToButton";
        uiaInfoTarget.m_AutomationID = L"FlowsToButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[4];
        std::vector<Automation::AutomationClient::UIAElementInfo> flowsToElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoFlowsTo;
            pszNames[i] = new WCHAR[wcslen(L"FlowsToTextBlockX") + 1];
            StringCchPrintf(pszNames[i], wcslen(L"FlowsToTextBlockX") + 1, L"FlowsToTextBlock%d", i + 1);
            uiaInfoFlowsTo.m_Name = pszNames[i];
            uiaInfoFlowsTo.m_AutomationID = pszNames[i];
            uiaInfoFlowsTo.m_cType = UIA_TextControlTypeId;
            flowsToElementInfos.push_back(uiaInfoFlowsTo);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            auto flowsToVector = xaml_automation::AutomationProperties::GetFlowsTo(button);

            VERIFY_IS_NOT_NULL(flowsToVector);

            for (size_t i = 0; i < flowsToElementInfos.size(); i++)
            {
                xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(flowsToElementInfos[i].m_Name));
                textBlock->Name = ref new Platform::String(flowsToElementInfos[i].m_Name);
                stackPanel->Children->Append(textBlock);
                flowsToVector->Append(textBlock);
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

            wrl::ComPtr<IUIAutomationElementArray> spFlowsToObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentFlowsTo(&spFlowsToObjects));
            if (spFlowsToObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsToObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)flowsToElementInfos.size());

                for (LONG i = 0; i < (LONG)flowsToElementInfos.size(); i++)
                {
                    auto spAutomationClientManagerFlowsTo = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(flowsToElementInfos[i]);
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElementFlowsTo;
                    spAutomationClientManagerFlowsTo->GetCurrentUIAutomationElement(&spUIAutomationElementFlowsTo);
                    WEX::Common::Throw::IfNull(spUIAutomationElementFlowsTo.Get());

                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spFlowsToObjects->GetElement(i, &spElement));
                    WEX::Common::Throw::IfNull(spElement.Get());
                    VERIFY_IS_TRUE(spAutomationClientManagerFlowsTo->IsElementSame(spElement.Get()));
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spFlowsToObjects.Get());
            }
        });

        for (int i = 0; i < 4; i++)
        {
            delete[] pszNames[i];
        }
    }

    void FlowsToIntegrationTests::_VerifyFlowsToMultipleElements_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying FlowsTo added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        CustomButton^ button;
        wfc::IVector<xaml::DependencyObject^>^ textBlocks;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"FlowsToButton";
        uiaInfoTarget.m_AutomationID = L"FlowsToButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[4];
        std::vector<Automation::AutomationClient::UIAElementInfo> flowsToElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoFlowsTo;
            pszNames[i] = new WCHAR[wcslen(L"FlowsToTextBlockX") + 1];
            StringCchPrintf(pszNames[i], wcslen(L"FlowsToTextBlockX") + 1, L"FlowsToTextBlock%d", i + 1);
            uiaInfoFlowsTo.m_Name = pszNames[i];
            uiaInfoFlowsTo.m_AutomationID = pszNames[i];
            uiaInfoFlowsTo.m_cType = UIA_TextControlTypeId;
            flowsToElementInfos.push_back(uiaInfoFlowsTo);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton;
            textBlocks = ref new Platform::Collections::Vector<xaml::DependencyObject^>;

            for (size_t i = 0; i < flowsToElementInfos.size(); i++)
            {
                xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(flowsToElementInfos[i].m_Name));
                textBlock->Name = ref new Platform::String(flowsToElementInfos[i].m_Name);
                textBlocks->Append(textBlock);
                stackPanel->Children->Append(textBlock);
            }

            button->SetAPFlowsToObjects(textBlocks);
            button->SetTestCase(FlowsToMultipleElements);
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

            wrl::ComPtr<IUIAutomationElementArray> spFlowsToObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentFlowsTo(&spFlowsToObjects));
            if (spFlowsToObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spFlowsToObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)flowsToElementInfos.size());

                for (LONG i = 0; i < (LONG)flowsToElementInfos.size(); i++)
                {
                    auto spAutomationClientManagerFlowsTo = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(flowsToElementInfos[i]);
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElementFlowsTo;
                    spAutomationClientManagerFlowsTo->GetCurrentUIAutomationElement(&spUIAutomationElementFlowsTo);
                    WEX::Common::Throw::IfNull(spUIAutomationElementFlowsTo.Get());

                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spFlowsToObjects->GetElement(i, &spElement));
                    WEX::Common::Throw::IfNull(spElement.Get());
                    VERIFY_IS_TRUE(spAutomationClientManagerFlowsTo->IsElementSame(spElement.Get()));
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spFlowsToObjects.Get());
            }
        });

        for (int i = 0; i < 4; i++)
        {
            delete[] pszNames[i];
        }
    }

    wfc::IIterable<xaml_automation_peers::AutomationPeer^>^ FlowsToIntegrationTests::CustomButtonAP::GetFlowsToCore()
    {
        wfc::IVector<xaml_automation_peers::AutomationPeer^>^ flowsTo =
            ref new Platform::Collections::Vector<xaml_automation_peers::AutomationPeer^>;

        switch (m_tc)
        {
            case FlowsToOneElement:
            case FlowsToMultipleElements:
                for (unsigned int i = 0; i < m_objects->Size; i++)
                {
                    flowsTo->Append(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement((UIElement^)m_objects->GetAt(i)));
                }
                break;
        }

        return flowsTo;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
