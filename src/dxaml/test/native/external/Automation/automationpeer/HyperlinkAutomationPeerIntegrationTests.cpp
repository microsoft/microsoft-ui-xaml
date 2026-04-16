// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <Patterns\InvokePatternHandler.h>
#include <HyperlinkAutomationPeerIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

#include <ChangeDPI.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    bool HyperlinkAutomationPeerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool HyperlinkAutomationPeerIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool HyperlinkAutomationPeerIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    xaml_docs::Hyperlink^ HyperlinkAutomationPeerIntegrationTests::SetupHyperlinkTest()
    {
        auto gridRoot = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
            L"<Grid \r\n"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' \r\n"
            L"  x:Name='root' Background='#000000' Width='480' Height='768' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
            L"      <TextBlock x:Name='tb' Width='100' Height='400'> \r\n"
            L"          <Hyperlink x:Name='hyperlink' AutomationProperties.Name='hyperlink'> \r\n"
            L"              Hello world \r\n"
            L"          </Hyperlink> \r\n"
            L"      </TextBlock> \r\n"
            L"      <TextBlock x:Name='tbN' Width='100' Height='400'> \r\n"
            L"          <Hyperlink x:Name='hyperlink1Name' AutomationProperties.Name='hyperlink1Name' NavigateUri=\"http://www.bing.com\"> \r\n"
            L"              Hello world \r\n"
            L"          </Hyperlink> \r\n"
            L"      </TextBlock> \r\n"
            L"      <TextBlock x:Name='tbNN' Width='100' Height='400'> \r\n"
            L"          <Hyperlink x:Name='hyperlink2Name' NavigateUri=\"http://www.bing.com\"> \r\n"
            L"              HyperlinkNoName \r\n"
            L"          </Hyperlink> \r\n"
            L"      </TextBlock> \r\n"
            L"      <TextBlock x:Name='tbNC' Width='100' Height='400'> \r\n"
            L"          <Hyperlink x:Name='hyperlink3Name' NavigateUri=\"http://www.bing.com\"> \r\n"
            L"          </Hyperlink> \r\n"
            L"      </TextBlock> \r\n"
            L"</Grid>"));
        TestServices::WindowHelper->WindowContent = gridRoot;

        xaml_docs::Hyperlink^ hyperlink = safe_cast<xaml_docs::Hyperlink^>(gridRoot->FindName(L"hyperlink"));

        return hyperlink;
    }

    //
    // Test Cases
    //
    void HyperlinkAutomationPeerIntegrationTests::DoesSupportEssentialPatterns()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"hyperlink";
        uiaInfo.m_AutomationID = L"hyperlink";
        uiaInfo.m_ItemStatus = L"hyperlink";
        uiaInfo.m_cType = UIA_HyperlinkControlTypeId;

        RunOnUIThread([&]()
        {
            SetupHyperlinkTest();
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            AutomationClient::AutomationGenericTests automationGenericTests(spAutomationClientManager);
            automationGenericTests.DoesSupportEssentialPatternsForControlType(uiaInfo.m_cType);
        });

    }

    void HyperlinkAutomationPeerIntegrationTests::CanBeInvokedByUIA()
    {
        TestCleanupWrapper cleanup;
        xaml_docs::Hyperlink^ hyperlink = nullptr;
        Automation::AutomationClient::UIAElementInfo uiaInfo;

        auto spClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_docs::Hyperlink, Click);


        RunOnUIThread([&]()
        {
            uiaInfo.m_Name = L"hyperlink";
            uiaInfo.m_AutomationID = L"hyperlink";
            uiaInfo.m_ItemStatus = L"hyperlink";
            uiaInfo.m_cType = UIA_HyperlinkControlTypeId;

            hyperlink = SetupHyperlinkTest();

            clickRegistration.Attach(hyperlink,
                ref new wf::TypedEventHandler<xaml_docs::Hyperlink^, xaml_docs::HyperlinkClickEventArgs^>([spClickEvent](xaml_docs::Hyperlink^ sender, xaml_docs::HyperlinkClickEventArgs^ e)
                {
                    spClickEvent->Set();
                }));
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<Automation::Patterns::InvokePatternHandler> spAutomationInvokePatternHandler;

            auto spAutomationClientManager = Automation::AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationInvokePatternHandler.Attach(new Automation::Patterns::InvokePatternHandler(spAutomationClientManager));
            spAutomationInvokePatternHandler->Invoke();
        });
        TestServices::WindowHelper->WaitForIdle();
        spClickEvent->WaitForDefault();
    }

    void HyperlinkAutomationPeerIntegrationTests::VerifyDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"hyperlink1Name";

        // Setup
        RunOnUIThread([&]()
        {
            SetupHyperlinkTest();
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spHyperlinkWithName;
            wrl::ComPtr<IUIAutomationElement> spHyperlinkWithContent;
            wrl::ComPtr<IUIAutomationElement> spHyperlinkWithNeither;
            wrl::ComPtr<IUIAutomationElement> spTextBlockOverHyperlinkWithName;
            wrl::ComPtr<IUIAutomationElement> spTextBlockOverHyperlinkWithContent;
            wrl::ComPtr<IUIAutomationElement> spTextBlockOverHyperlinkWithNeither;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spHyperlinkWithName);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"HyperlinkAutomationPeerIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"HyperlinkAutomationPeerIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for HyperlinkWithName exists.");
            VERIFY_IS_NOT_NULL(spHyperlinkWithName);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spHyperlinkWithName.");
            spHyperlinkWithName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            spUIAutomationTreeWalker->GetParentElement(spHyperlinkWithName.Get(), &spTextBlockOverHyperlinkWithName);
            spUIAutomationTreeWalker->GetNextSiblingElement(spTextBlockOverHyperlinkWithName.Get(), &spTextBlockOverHyperlinkWithContent);

            LOG_OUTPUT(L"Navigate to the second link.");
            spUIAutomationTreeWalker->GetFirstChildElement(spTextBlockOverHyperlinkWithContent.Get(), &spHyperlinkWithContent);
            VERIFY_IS_NOT_NULL(spHyperlinkWithContent);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spHyperlinkWithContent.");
            spHyperlinkWithContent->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"HyperlinkNoName", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the third link.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spTextBlockOverHyperlinkWithContent.Get(), &spTextBlockOverHyperlinkWithNeither);
            spUIAutomationTreeWalker->GetFirstChildElement(spTextBlockOverHyperlinkWithNeither.Get(), &spHyperlinkWithNeither);
            VERIFY_IS_NOT_NULL(spHyperlinkWithNeither);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spHyperlinkWithNeither.");
            spHyperlinkWithNeither->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"http://www.bing.com/", (autoVar.Storage())->bstrVal));
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
