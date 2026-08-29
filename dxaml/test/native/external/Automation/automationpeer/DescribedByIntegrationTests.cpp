// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <DescribedByIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    bool DescribedByIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool DescribedByIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool DescribedByIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Case Wrappers
    //
    void DescribedByIntegrationTests::VerifyDescribedByNotSet()
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

            wrl::ComPtr<IUIAutomationElementArray> spDescribedByObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement->get_CurrentDescribedBy(&spDescribedByObjects));
            if (spDescribedByObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spDescribedByObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spDescribedByObjects.Get() == nullptr || length == 0);
        });
    }

    void DescribedByIntegrationTests::VerifyDescribedByEmptyList()
    {
        _VerifyDescribedByEmptyList_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyDescribedByEmptyList_AutomationPeer();
    }

    void DescribedByIntegrationTests::VerifyDescribedByOneElement()
    {
        _VerifyDescribedByOneElement_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyDescribedByOneElement_AutomationPeer();
    }

    void DescribedByIntegrationTests::VerifyDescribedByMultipleElements()
    {
        _VerifyDescribedByMultipleElements_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyDescribedByMultipleElements_AutomationPeer();
    }

    //
    // Test Cases
    //
    void DescribedByIntegrationTests::_VerifyDescribedByEmptyList_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying DescribedBy added from AutomationProperties in codebehind");

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"DescribedByButton";
        uiaInfo.m_AutomationID = L"DescribedByButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            auto describedByVector = xaml_automation::AutomationProperties::GetDescribedBy(button);
            VERIFY_IS_NOT_NULL(describedByVector);

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

            wrl::ComPtr<IUIAutomationElementArray> spDescribedByObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement->get_CurrentDescribedBy(&spDescribedByObjects));
            if (spDescribedByObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spDescribedByObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spDescribedByObjects.Get() == nullptr || length == 0);
        });
    }

    void DescribedByIntegrationTests::_VerifyDescribedByEmptyList_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying DescribedBy added from AutomationPeer");

        CustomButton^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"DescribedByButton";
        uiaInfo.m_AutomationID = L"DescribedByButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new CustomButton;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
            button->SetTestCase(DescribedByEmptyList);

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(button);
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElementArray> spDescribedByObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement->get_CurrentDescribedBy(&spDescribedByObjects));
            if (spDescribedByObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spDescribedByObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spDescribedByObjects.Get() == nullptr || length == 0);
        });
    }


    void DescribedByIntegrationTests::_VerifyDescribedByOneElement_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying DescribedBy added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"DescribedByButton";
        uiaInfoTarget.m_AutomationID = L"DescribedByButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoDescribedBy;
        uiaInfoDescribedBy.m_Name = L"DescribedByTextBlock";
        uiaInfoDescribedBy.m_AutomationID = L"DescribedByTextBlock";
        uiaInfoDescribedBy.m_cType = UIA_TextControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;
            textBlock = ref new xaml_controls::TextBlock;

            auto describedByVector = xaml_automation::AutomationProperties::GetDescribedBy(button);
            VERIFY_IS_NOT_NULL(describedByVector);
            describedByVector->Append(textBlock);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfoDescribedBy.m_Name));
            textBlock->Name = ref new Platform::String(uiaInfoDescribedBy.m_Name);

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

            auto spAutomationClientManagerDescribedBy = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoDescribedBy);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementDescribedBy;
            spAutomationClientManagerDescribedBy->GetCurrentUIAutomationElement(&spUIAutomationElementDescribedBy);
            WEX::Common::Throw::IfNull(spUIAutomationElementDescribedBy.Get());

            wrl::ComPtr<IUIAutomationElementArray> spDescribedByObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentDescribedBy(&spDescribedByObjects));
            if (spDescribedByObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spDescribedByObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, 1);

                wrl::ComPtr<IUIAutomationElement> spElement;
                VERIFY_SUCCEEDED(spDescribedByObjects->GetElement(0, &spElement));
                WEX::Common::Throw::IfNull(spElement.Get());
                VERIFY_IS_TRUE(spAutomationClientManagerDescribedBy->IsElementSame(spElement.Get()));
            }
            else
            {
                VERIFY_IS_NOT_NULL(spDescribedByObjects.Get());
            }
        });
    }

    void DescribedByIntegrationTests::_VerifyDescribedByOneElement_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying DescribedBy added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        CustomButton^ button;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"DescribedByButton";
        uiaInfoTarget.m_AutomationID = L"DescribedByButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoDescribedBy;
        uiaInfoDescribedBy.m_Name = L"DescribedByTextBlock";
        uiaInfoDescribedBy.m_AutomationID = L"DescribedByTextBlock";
        uiaInfoDescribedBy.m_cType = UIA_TextControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton;
            textBlock = ref new xaml_controls::TextBlock;

            wfc::IVector<xaml::DependencyObject^>^ describedBy = ref new Platform::Collections::Vector<xaml::DependencyObject^>;
            describedBy->Append(textBlock);
            button->SetAPDescribedByObjects(describedBy);
            button->SetTestCase(DescribedByOneElement);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfoDescribedBy.m_Name));
            textBlock->Name = ref new Platform::String(uiaInfoDescribedBy.m_Name);

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

            auto spAutomationClientManagerDescribedBy = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoDescribedBy);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementDescribedBy;
            spAutomationClientManagerDescribedBy->GetCurrentUIAutomationElement(&spUIAutomationElementDescribedBy);
            WEX::Common::Throw::IfNull(spUIAutomationElementDescribedBy.Get());

            wrl::ComPtr<IUIAutomationElementArray> spDescribedByObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentDescribedBy(&spDescribedByObjects));
            if (spDescribedByObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spDescribedByObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, 1);

                wrl::ComPtr<IUIAutomationElement> spElement;
                VERIFY_SUCCEEDED(spDescribedByObjects->GetElement(0, &spElement));
                WEX::Common::Throw::IfNull(spElement.Get());
                VERIFY_IS_TRUE(spAutomationClientManagerDescribedBy->IsElementSame(spElement.Get()));
            }
            else
            {
                VERIFY_IS_NOT_NULL(spDescribedByObjects.Get());
            }
        });
    }


    void DescribedByIntegrationTests::_VerifyDescribedByMultipleElements_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying DescribedBy added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"DescribedByButton";
        uiaInfoTarget.m_AutomationID = L"DescribedByButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[4];
        std::vector<Automation::AutomationClient::UIAElementInfo> describedByElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoDescribedBy;
            pszNames[i] = new WCHAR[wcslen(L"DescribedByTextBlockX") + 1];
            StringCchPrintf(pszNames[i], wcslen(L"DescribedByTextBlockX") + 1, L"DescribedByTextBlock%d", i + 1);
            uiaInfoDescribedBy.m_Name = pszNames[i];
            uiaInfoDescribedBy.m_AutomationID = pszNames[i];
            uiaInfoDescribedBy.m_cType = UIA_TextControlTypeId;
            describedByElementInfos.push_back(uiaInfoDescribedBy);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            auto describedByVector = xaml_automation::AutomationProperties::GetDescribedBy(button);

            VERIFY_IS_NOT_NULL(describedByVector);

            for (size_t i = 0; i < describedByElementInfos.size(); i++)
            {
                xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(describedByElementInfos[i].m_Name));
                textBlock->Name = ref new Platform::String(describedByElementInfos[i].m_Name);
                stackPanel->Children->Append(textBlock);
                describedByVector->Append(textBlock);
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

            wrl::ComPtr<IUIAutomationElementArray> spDescribedByObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentDescribedBy(&spDescribedByObjects));
            if (spDescribedByObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spDescribedByObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)describedByElementInfos.size());

                for (LONG i = 0; i < (LONG)describedByElementInfos.size(); i++)
                {
                    auto spAutomationClientManagerDescribedBy = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(describedByElementInfos[i]);
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElementDescribedBy;
                    spAutomationClientManagerDescribedBy->GetCurrentUIAutomationElement(&spUIAutomationElementDescribedBy);
                    WEX::Common::Throw::IfNull(spUIAutomationElementDescribedBy.Get());

                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spDescribedByObjects->GetElement(i, &spElement));
                    WEX::Common::Throw::IfNull(spElement.Get());
                    VERIFY_IS_TRUE(spAutomationClientManagerDescribedBy->IsElementSame(spElement.Get()));
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spDescribedByObjects.Get());
            }
        });

        for (int i = 0; i < 4; i++)
        {
            delete[] pszNames[i];
        }
    }

    void DescribedByIntegrationTests::_VerifyDescribedByMultipleElements_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying DescribedBy added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        CustomButton^ button;
        wfc::IVector<xaml::DependencyObject^>^ textBlocks;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"DescribedByButton";
        uiaInfoTarget.m_AutomationID = L"DescribedByButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[4];
        std::vector<Automation::AutomationClient::UIAElementInfo> describedByElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoDescribedBy;
            pszNames[i] = new WCHAR[wcslen(L"DescribedByTextBlockX") + 1];
            StringCchPrintf(pszNames[i], wcslen(L"DescribedByTextBlockX") + 1, L"DescribedByTextBlock%d", i + 1);
            uiaInfoDescribedBy.m_Name = pszNames[i];
            uiaInfoDescribedBy.m_AutomationID = pszNames[i];
            uiaInfoDescribedBy.m_cType = UIA_TextControlTypeId;
            describedByElementInfos.push_back(uiaInfoDescribedBy);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton;
            textBlocks = ref new Platform::Collections::Vector<xaml::DependencyObject^>;

            for (size_t i = 0; i < describedByElementInfos.size(); i++)
            {
                xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(describedByElementInfos[i].m_Name));
                textBlock->Name = ref new Platform::String(describedByElementInfos[i].m_Name);
                textBlocks->Append(textBlock);
                stackPanel->Children->Append(textBlock);
            }

            button->SetAPDescribedByObjects(textBlocks);
            button->SetTestCase(DescribedByMultipleElements);
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

            wrl::ComPtr<IUIAutomationElementArray> spDescribedByObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget->get_CurrentDescribedBy(&spDescribedByObjects));
            if (spDescribedByObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spDescribedByObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)describedByElementInfos.size());

                for (LONG i = 0; i < (LONG)describedByElementInfos.size(); i++)
                {
                    auto spAutomationClientManagerDescribedBy = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(describedByElementInfos[i]);
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElementDescribedBy;
                    spAutomationClientManagerDescribedBy->GetCurrentUIAutomationElement(&spUIAutomationElementDescribedBy);
                    WEX::Common::Throw::IfNull(spUIAutomationElementDescribedBy.Get());

                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spDescribedByObjects->GetElement(i, &spElement));
                    WEX::Common::Throw::IfNull(spElement.Get());
                    VERIFY_IS_TRUE(spAutomationClientManagerDescribedBy->IsElementSame(spElement.Get()));
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spDescribedByObjects.Get());
            }
        });

        for (int i = 0; i < 4; i++)
        {
            delete[] pszNames[i];
        }
    }

    wfc::IIterable<xaml_automation_peers::AutomationPeer^>^ DescribedByIntegrationTests::CustomButtonAP::GetDescribedByCore()
    {
        wfc::IVector<xaml_automation_peers::AutomationPeer^>^ describedBy =
            ref new Platform::Collections::Vector<xaml_automation_peers::AutomationPeer^>;

        switch (m_tc)
        {
            case DescribedByOneElement:
            case DescribedByMultipleElements:
                for (unsigned int i = 0; i < m_objects->Size; i++)
                {
                    describedBy->Append(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement((UIElement^)m_objects->GetAt(i)));
                }
                break;
        }

        return describedBy;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
