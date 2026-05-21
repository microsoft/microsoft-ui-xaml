// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ListViewBaseItemAutomationIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>
#include <AutomationClient\AutomationGenericTests.h>

#include <XamlTailored.h>
#include <ControlHelper.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include "dataclasses.h"
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace Tests::Native::External::Controls::Helpers;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListViewBaseItem {

    bool ListViewBaseItemAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ListViewBaseItemAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ListViewBaseItemAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ListViewBaseItemAutomationIntegrationTests::VerifyDefaultAutomationNameContent()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            // Set up data items
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listViewDMP' Width='300' AutomationProperties.Name='TestListViewName'>"
                L"        <ListViewItem Content = 'One' AutomationProperties.AutomationId = 'ListViewItem1' />"
                L"        <ListViewItem Content = '2' AutomationProperties.Name = 'TWO!' />"
                L"        <ListViewItem AutomationProperties.AutomationId = 'ListViewItem3' />"
                L"    </ListView>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spListViewItem1;
            wrl::ComPtr<IUIAutomationElement> spListViewItem2;
            wrl::ComPtr<IUIAutomationElement> spListViewItem3;
            wrl::ComPtr<IUIAutomationElement> spListView;

            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            Common::AutoVariant autoVar;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestListViewName";

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spListView);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListViewBaseItemAutomationIntegrationTests::VerifyDefaultAutomationName: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListViewBaseItemAutomationIntegrationTests::VerifyDefaultAutomationName: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for spListViewBaseWithName exists.");
            VERIFY_IS_NOT_NULL(spListView);

            LOG_OUTPUT(L"Navigate to the ListViewBaseItem.");
            spUIAutomationTreeWalker->GetFirstChildElement(spListView.Get(), &spListViewItem1);
            VERIFY_IS_NOT_NULL(spListViewItem1);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for first list view item.");
            spListViewItem1->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"One", (autoVar.Storage())->bstrVal));
            spListViewItem1->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == UIA_ListItemControlTypeId);

            LOG_OUTPUT(L"Navigate to the next item.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spListViewItem1.Get(), &spListViewItem2);
            VERIFY_IS_NOT_NULL(spListViewItem2);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for first list view item.");
            spListViewItem2->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"TWO!", (autoVar.Storage())->bstrVal));
            spListViewItem2->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == UIA_ListItemControlTypeId);

            LOG_OUTPUT(L"Navigate to the next item.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spListViewItem2.Get(), &spListViewItem3);
            VERIFY_IS_NOT_NULL(spListViewItem3);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for first list view item.");
            spListViewItem3->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"", (autoVar.Storage())->bstrVal));
            spListViewItem3->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == UIA_ListItemControlTypeId);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ListViewBaseItem
