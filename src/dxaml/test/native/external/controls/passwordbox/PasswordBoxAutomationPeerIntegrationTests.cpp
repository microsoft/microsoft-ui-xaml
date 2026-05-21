// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PasswordBoxAutomationPeerIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>
#include <AutomationClient\AutomationGenericTests.h>

#include <XamlTailored.h>
#include <ControlHelper.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace PasswordBox {

    bool PasswordBoxAutomationPeerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PasswordBoxAutomationPeerIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PasswordBoxAutomationPeerIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    void PasswordBoxAutomationPeerIntegrationTests::ValidateDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"PasswordBox with Name";

        xaml_controls::PasswordBox^ PasswordBoxWithName = nullptr;
        xaml_controls::PasswordBox^ PasswordBoxWithHeader = nullptr;
        xaml_controls::PasswordBox^ PasswordBoxWithPlaceholderText = nullptr;
        xaml_controls::PasswordBox^ PasswordBoxWithNoAccName = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        // Setup
        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            PasswordBoxWithName = ref new xaml_controls::PasswordBox();
            PasswordBoxWithName->Header = "PasswordBox with Name - Header";
            PasswordBoxWithName->PlaceholderText = "PasswordBox with Name - PlaceholderText";
            xaml_automation::AutomationProperties::SetName(PasswordBoxWithName, ref new Platform::String(L"PasswordBox with Name"));
            rootPanel->Children->Append(PasswordBoxWithName);

            PasswordBoxWithHeader = ref new xaml_controls::PasswordBox();
            PasswordBoxWithHeader->Header = "PasswordBox with Header";
            PasswordBoxWithHeader->PlaceholderText = "PasswordBox with Header - PlaceholderText";
            rootPanel->Children->Append(PasswordBoxWithHeader);

            PasswordBoxWithPlaceholderText = ref new xaml_controls::PasswordBox();
            PasswordBoxWithPlaceholderText->PlaceholderText = "PasswordBox with PlaceholderText";
            rootPanel->Children->Append(PasswordBoxWithPlaceholderText);

            PasswordBoxWithNoAccName = ref new xaml_controls::PasswordBox();
            rootPanel->Children->Append(PasswordBoxWithNoAccName);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spPasswordBoxWithName;
            wrl::ComPtr<IUIAutomationElement> spPasswordBoxWithHeader;
            wrl::ComPtr<IUIAutomationElement> spPasswordBoxWithPlaceholderText;
            wrl::ComPtr<IUIAutomationElement> spPasswordBoxWithNoAccName;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spPasswordBoxWithName);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for PasswordBoxWithName exists.");
            VERIFY_IS_NOT_NULL(spPasswordBoxWithName);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spPasswordBoxWithName.");
            spPasswordBoxWithName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the second PasswordBox.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spPasswordBoxWithName.Get(), &spPasswordBoxWithHeader);
            VERIFY_IS_NOT_NULL(spPasswordBoxWithHeader);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spPasswordBoxWithHeader.");
            spPasswordBoxWithHeader->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"PasswordBox with Header", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the third PasswordBox.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spPasswordBoxWithHeader.Get(), &spPasswordBoxWithPlaceholderText);
            VERIFY_IS_NOT_NULL(spPasswordBoxWithPlaceholderText);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spPasswordBoxWithPlaceholderText.");
            spPasswordBoxWithPlaceholderText->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"PasswordBox with PlaceholderText", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the fourth PasswordBox.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spPasswordBoxWithPlaceholderText.Get(), &spPasswordBoxWithNoAccName);
            VERIFY_IS_NOT_NULL(spPasswordBoxWithNoAccName);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spPasswordBoxWithNoName.");
            spPasswordBoxWithNoAccName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"", (autoVar.Storage())->bstrVal));
        });
    }

    void PasswordBoxAutomationPeerIntegrationTests::VerifyPasswordBoxPlaceholderTextIsMovedToDescribedBy()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::PasswordBox^ PasswordBoxWithPlaceholderText = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_automation_peers::AutomationPeer^ PasswordBoxWithPlaceholderTextAP = nullptr;
        Platform::String^ const placeHolderText = "Password with PlaceholderText";

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            PasswordBoxWithPlaceholderText = ref new xaml_controls::PasswordBox();
            PasswordBoxWithPlaceholderText->Name = "txb1";
            PasswordBoxWithPlaceholderTextAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(PasswordBoxWithPlaceholderText);
            VERIFY_IS_NOT_NULL(PasswordBoxWithPlaceholderTextAP);

            rootPanel->Children->Append(PasswordBoxWithPlaceholderText);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            PasswordBoxWithPlaceholderText->PlaceholderText = placeHolderText;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto describedByVector = PasswordBoxWithPlaceholderTextAP->GetDescribedByCore()->First();
            auto textBlockAP = static_cast<xaml_automation_peers::FrameworkElementAutomationPeer^>(describedByVector->Current);
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml_automation_peers::TextBlockAutomationPeer^>(textBlockAP));
            auto placeHolderTextBlock = static_cast<xaml_controls::TextBlock^>(textBlockAP->Owner);
            VERIFY_ARE_EQUAL(placeHolderText, placeHolderTextBlock->Text);

            LOG_OUTPUT(L"Verify that only one DescribedBy property was set.");
            VERIFY_IS_FALSE(describedByVector->MoveNext());
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::PasswordBox
