// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MoCoAutomationIntegrationTests.h"
#include <ItemsControlHelper.h>
#include <MocoHelper.h>
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <AutomationClient\AutomationClientManager.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace MoCo {

    bool MoCoAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool MoCoAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool MoCoAutomationIntegrationTests::TestCleanup()
    {
           test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListViewBase^ list;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestListViewName";
        uiaInfo.m_AutomationID = L"TestListViewId";
        uiaInfo.m_ItemStatus = L"TestListView";
        uiaInfo.m_cType = UIA_PaneControlTypeId;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 666.67f));

        RunOnUIThread([&]()
        {
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView x:Name='list' Height='500' Width='500'>" \
                L"      <ListViewItem AutomationProperties.PositionInSet='3' AutomationProperties.SizeOfSet='8' AutomationProperties.Level='3'>Item 1</ListViewItem>" \
                L"      <ListViewItem AutomationProperties.PositionInSet='4' AutomationProperties.SizeOfSet='5' AutomationProperties.Level='1'>Item 2</ListViewItem>" \
                L"      <ListViewItem AutomationProperties.PositionInSet='1' AutomationProperties.SizeOfSet='10' AutomationProperties.Level='1'>Item 3</ListViewItem>" \
                L"   </ListView>" \
                L"</Grid>";

            xaml_controls::Grid^ rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"list"));

            xaml_automation::AutomationProperties::SetName(list, ref new Platform::String(uiaInfo.m_Name));
            xaml_automation::AutomationProperties::SetAutomationId(list, ref new Platform::String(uiaInfo.m_AutomationID));

            TestServices::WindowHelper->WindowContent = rootPanel;

        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spUIListItemTempAAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spUIListItemTempBAutomationElement;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomation> spUIAutomation;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for ListView exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            Common::AutoVariant autoVar;

            LOG_OUTPUT(L"Navigate to First Item in ListView and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 3);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 8);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 3);

            LOG_OUTPUT(L"Navigate to Second Item in ListView and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempAAutomationElement.Get(), &spUIListItemTempBAutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get second ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempBAutomationElement);

            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 4);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 5);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);

            LOG_OUTPUT(L"Navigate to third Item in ListView and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempBAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get third ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 10);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoAutomationIntegrationTests::VerifySizePosLevelFromGenerated()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestListViewName";
        uiaInfo.m_AutomationID = L"TestListViewId";
        uiaInfo.m_ItemStatus = L"TestListView";
        uiaInfo.m_cType = UIA_PaneControlTypeId;
        xaml_controls::ListViewBase^ list = MocoHelper::SetUpBasicEnvironment(MocoHelper::ListControlType::ListView, 1000, 400, 200);
        std::shared_ptr<AutomationClient::AutomationClientManager> spAutomationClientManager;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(list, ref new Platform::String(uiaInfo.m_Name));
            xaml_automation::AutomationProperties::SetAutomationId(list, ref new Platform::String(uiaInfo.m_AutomationID));

            TestServices::WindowHelper->WindowContent = list;
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spUIListItemTempAAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spUIListItemTempBAutomationElement;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomation> spUIAutomation;

            spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for ListView exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            Common::AutoVariant autoVar;

            LOG_OUTPUT(L"Navigate to First Item in ListView and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);

            LOG_OUTPUT(L"Navigate to Second Item in ListView and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempAAutomationElement.Get(), &spUIListItemTempBAutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get second ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempBAutomationElement);

            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);

            LOG_OUTPUT(L"Navigate to third Item in ListView and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempBAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get third ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 3);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);
        });

        // Scroll to last
        int scrollIndex = 0;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Check if the last item is virtualized");
            auto lastItem = list->ContainerFromIndex(list->Items->Size - 1);
            VERIFY_IS_NULL(lastItem);

            LOG_OUTPUT(L"Check if first item is realized");
            auto firstItem = list->ContainerFromIndex(0);
            VERIFY_IS_NOT_NULL(firstItem);

            scrollIndex = list->Items->Size - 1;
        });

        ItemsControlHelper::ScrollToIndex<xaml_controls::ListViewBase, Object>(list, scrollIndex);

        RunOnUIThread([&]()
        {
            // Calling update layout so the virtualization path can kick in.
            list->UpdateLayout();

            LOG_OUTPUT(L"Check if the last item is realized");
            auto lastItem = list->ContainerFromIndex(list->Items->Size - 1);
            VERIFY_IS_NOT_NULL(lastItem);

            LOG_OUTPUT(L"Check if first item is virtualized.");
            auto firstItem = list->ContainerFromIndex(1);
            VERIFY_IS_NULL(firstItem);
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        int firstCacheIndex;

        RunOnUIThread([&]()
        {
            //Initialize Test variables
            auto isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            LOG_OUTPUT(L"first cache: %d, first visible: %d, lastvisible: %d, lastcache: %d", isp->FirstCacheIndex, isp->FirstVisibleIndex, isp->LastVisibleIndex, isp->LastCacheIndex);

            firstCacheIndex = isp->FirstCacheIndex;
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spUIListItemTempAAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spUIListItemTempBAutomationElement;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomation> spUIAutomation;

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for ListView exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            Common::AutoVariant autoVar;

            LOG_OUTPUT(L"Navigate to First Item in scrolled ListView and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == firstCacheIndex + 1);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);

            LOG_OUTPUT(L"Navigate to Second Item in scrolled ListView and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempAAutomationElement.Get(), &spUIListItemTempBAutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get second ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempBAutomationElement);

            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == firstCacheIndex + 2);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);

            LOG_OUTPUT(L"Navigate to third Item in scrolled ListView and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempBAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get third ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == firstCacheIndex + 3);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);
        });
    }

    void MoCoAutomationIntegrationTests::VerifyBringIntoView()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ScrollViewer^ scrollViewer;
        xaml_controls::ListViewBase^ list;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestListViewName";
        uiaInfo.m_AutomationID = L"TestListViewId";
        uiaInfo.m_ItemStatus = L"TestListView";
        uiaInfo.m_cType = UIA_PaneControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlText =
                L"<ScrollViewer Height='50' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>" \
                L"    <ListView x:Name='listView'>" \
                L"        <ListViewItem>1</ListViewItem>" \
                L"        <ListViewItem>2</ListViewItem>" \
                L"        <ListViewItem>3</ListViewItem>" \
                L"        <ListViewItem>4</ListViewItem>" \
                L"        <ListViewItem>5</ListViewItem>" \
                L"    </ListView>" \
                L"</ScrollViewer>";

            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListView^>(scrollViewer->FindName(L"listView"));

            xaml_automation::AutomationProperties::SetName(list, ref new Platform::String(uiaInfo.m_Name));
            xaml_automation::AutomationProperties::SetAutomationId(list, ref new Platform::String(uiaInfo.m_AutomationID));

            TestServices::WindowHelper->WindowContent = scrollViewer;
        });

        TestServices::WindowHelper->WaitForIdle();

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            list->Focus(FocusState::Keyboard);

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. IsIntermediate=%d.", args->IsIntermediate);
                if (args->IsIntermediate == false)
                {
                    viewChangedEvent->Set();
                }
            }));
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> automationElement;
            wrl::ComPtr<IUIAutomationElement> listItemTempAAutomationElement;
            wrl::ComPtr<IUIAutomationElement> listItemTempBAutomationElement;
            wrl::ComPtr<IUIAutomationCondition> automationTrueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> automationTreeWalker;
            wrl::ComPtr<IUIAutomation> spUIAutomation;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&automationElement);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&automationTrueCondition), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(automationTrueCondition.Get(), &automationTreeWalker), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for ListView exist.");
            VERIFY_IS_NOT_NULL(automationElement);

            Common::AutoVariant autoVar;

            LogThrow_IfFailedWithMessage(automationTreeWalker->GetFirstChildElement(automationElement.Get(), &listItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed to Get first ListItem.");
            VERIFY_IS_NOT_NULL(listItemTempAAutomationElement);
            LogThrow_IfFailedWithMessage(automationTreeWalker->GetNextSiblingElement(listItemTempAAutomationElement.Get(), &listItemTempBAutomationElement), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed to Get second ListItem.");
            VERIFY_IS_NOT_NULL(listItemTempBAutomationElement);
            LogThrow_IfFailedWithMessage(automationTreeWalker->GetNextSiblingElement(listItemTempBAutomationElement.Get(), &listItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed to Get third ListItem.");
            VERIFY_IS_NOT_NULL(listItemTempAAutomationElement);

            LOG_OUTPUT(L"Verify that the item's peer supports the ScrollItem pattern.");
            wrl::ComPtr<IUIAutomationScrollItemPattern> scrollPattern;
            LogThrow_IfFailedWithMessage(listItemTempAAutomationElement->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &scrollPattern), L"Failed in fetching ScrollItem pattern.");
            VERIFY_IS_NOT_NULL(scrollPattern);

            LOG_OUTPUT(L"Scrolling into view");
            scrollPattern->ScrollIntoView();
        });

        LOG_OUTPUT(L"Waiting for the scrollViewer to finish scrolling.");
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(0.0f, scrollViewer->VerticalOffset, L"outer scrollviewer has not scrolled to bring the item into view");
        });
    }

    void MoCoAutomationIntegrationTests::VerifyBringIntoViewInGridViewInsideSV()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ScrollViewer^ scrollViewer;
        xaml_controls::ListViewBase^ list;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestGridViewName";
        uiaInfo.m_AutomationID = L"TestGridViewId";
        uiaInfo.m_ItemStatus = L"TestGridView";
        uiaInfo.m_cType = UIA_PaneControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlText =
                L"<ScrollViewer Height='50' Width='50' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>" \
                L"    <GridView x:Name='gridView'>" \
                L"        <GridViewItem>1</GridViewItem>" \
                L"        <GridViewItem>2</GridViewItem>" \
                L"        <GridViewItem>3</GridViewItem>" \
                L"        <GridViewItem>4</GridViewItem>" \
                L"        <GridViewItem>5</GridViewItem>" \
                L"    </GridView>" \
                L"</ScrollViewer>";

            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::GridView^>(scrollViewer->FindName(L"gridView"));

            xaml_automation::AutomationProperties::SetName(list, ref new Platform::String(uiaInfo.m_Name));
            xaml_automation::AutomationProperties::SetAutomationId(list, ref new Platform::String(uiaInfo.m_AutomationID));

            TestServices::WindowHelper->WindowContent = scrollViewer;
        });

        TestServices::WindowHelper->WaitForIdle();

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            list->Focus(FocusState::Keyboard);

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. IsIntermediate=%d.", args->IsIntermediate);
                if (args->IsIntermediate == false)
                {
                    viewChangedEvent->Set();
                }
            }));
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> automationElement;
            wrl::ComPtr<IUIAutomationElement> listItemTempAAutomationElement;
            wrl::ComPtr<IUIAutomationElement> listItemTempBAutomationElement;
            wrl::ComPtr<IUIAutomationCondition> automationTrueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> automationTreeWalker;
            wrl::ComPtr<IUIAutomation> spUIAutomation;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&automationElement);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&automationTrueCondition), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(automationTrueCondition.Get(), &automationTreeWalker), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for GridView exist.");
            VERIFY_IS_NOT_NULL(automationElement);

            Common::AutoVariant autoVar;

            LogThrow_IfFailedWithMessage(automationTreeWalker->GetFirstChildElement(automationElement.Get(), &listItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed to Get first GridItem.");
            VERIFY_IS_NOT_NULL(listItemTempAAutomationElement);
            LogThrow_IfFailedWithMessage(automationTreeWalker->GetNextSiblingElement(listItemTempAAutomationElement.Get(), &listItemTempBAutomationElement), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed to Get second GridItem.");
            VERIFY_IS_NOT_NULL(listItemTempBAutomationElement);
            LogThrow_IfFailedWithMessage(automationTreeWalker->GetNextSiblingElement(listItemTempBAutomationElement.Get(), &listItemTempAAutomationElement), L"MoCoAutomationIntegrationTests::VerifyBringIntoView: Failed to Get third GridItem.");
            VERIFY_IS_NOT_NULL(listItemTempAAutomationElement);

            LOG_OUTPUT(L"Verify that the item's peer supports the ScrollItem pattern.");
            wrl::ComPtr<IUIAutomationScrollItemPattern> scrollPattern;
            LogThrow_IfFailedWithMessage(listItemTempAAutomationElement->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &scrollPattern), L"Failed in fetching ScrollItem pattern.");
            VERIFY_IS_NOT_NULL(scrollPattern);

            LOG_OUTPUT(L"Scrolling into view");
            scrollPattern->ScrollIntoView();
        });

        LOG_OUTPUT(L"Waiting for the scrollViewer to finish scrolling.");
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(0.0f, scrollViewer->VerticalOffset, L"outer scrollviewer has not scrolled to bring the item into view");
        });
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Moco
