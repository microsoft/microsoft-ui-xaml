// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ListBoxAutomationIntegrationTests.h"

#include <ItemsControlHelper.h>
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <AutomationClient\AutomationClientManager.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListBox {

    bool ListBoxAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ListBoxAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ListBoxAutomationIntegrationTests::TestCleanup()
    {
           test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListBox^ list;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestListBoxName";
        uiaInfo.m_AutomationID = L"TestListBoxId";
        uiaInfo.m_ItemStatus = L"TestListBox";
        uiaInfo.m_cType = UIA_PaneControlTypeId;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 666.67f));

        RunOnUIThread([&]()
        {
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListBox x:Name='list' Height='500' Width='500'>" \
                L"      <ListBoxItem AutomationProperties.PositionInSet='3' AutomationProperties.SizeOfSet='8' AutomationProperties.Level='3'>Item 1</ListBoxItem>" \
                L"      <ListBoxItem AutomationProperties.PositionInSet='4' AutomationProperties.SizeOfSet='5' AutomationProperties.Level='1'>Item 2</ListBoxItem>" \
                L"      <ListBoxItem AutomationProperties.PositionInSet='1' AutomationProperties.SizeOfSet='10' AutomationProperties.Level='1'>Item 3</ListBoxItem>" \
                L"   </ListBox>" \
                L"</Grid>";

            xaml_controls::Grid^ rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListBox^>(rootPanel->FindName(L"list"));

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

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for ListBox exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            Common::AutoVariant autoVar;

            LOG_OUTPUT(L"Navigate to First Item in ListBox and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 3);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 8);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 3);

            LOG_OUTPUT(L"Navigate to Second Item in ListBox and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempAAutomationElement.Get(), &spUIListItemTempBAutomationElement), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get second ListBox.");
            VERIFY_IS_NOT_NULL(spUIListItemTempBAutomationElement);

            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 4);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 5);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);

            LOG_OUTPUT(L"Navigate to third Item in ListBox and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempBAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get third ListItem.");
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

    void ListBoxAutomationIntegrationTests::VerifySizePosLevelFromGenerated()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestListBoxName";
        uiaInfo.m_AutomationID = L"TestListBoxId";
        uiaInfo.m_ItemStatus = L"TestListBox";
        uiaInfo.m_cType = UIA_PaneControlTypeId;
        auto list = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, 1000 /*EnoughItemsForVirtualization*/);
        std::shared_ptr<AutomationClient::AutomationClientManager> spAutomationClientManager;
        int firstRealizedIndex = 0;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(list, ref new Platform::String(uiaInfo.m_Name));
            xaml_automation::AutomationProperties::SetAutomationId(list, ref new Platform::String(uiaInfo.m_AutomationID));

            // Prevent the list from receiving focus at the start of the test, which would cause the first item to remain
            // realized after the ScrollIntoView call.
            list->IsEnabled = false;

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

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for ListBox exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            Common::AutoVariant autoVar;

            LOG_OUTPUT(L"Navigate to First Item in ListBox and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);

            LOG_OUTPUT(L"Navigate to Second Item in ListBox and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempAAutomationElement.Get(), &spUIListItemTempBAutomationElement), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get second ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempBAutomationElement);

            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);

            LOG_OUTPUT(L"Navigate to third Item in ListBox and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempBAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get third ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 3);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);
        });

        RunOnUIThread([&]()
        {
            unsigned int index = list->Items->Size - 1;
            list->ScrollIntoView(list->Items->GetAt(index));
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            //Initialize Test variables
            xaml_controls::VirtualizingStackPanel^ vsp = dynamic_cast<xaml_controls::VirtualizingStackPanel^>(list->ItemsPanelRoot);
            xaml_controls::UIElementCollection^ uieCollection = vsp->Children;
            firstRealizedIndex = list->IndexFromContainer(uieCollection->GetAt(0));
            LOG_OUTPUT(L"first Realized Index: %d", firstRealizedIndex);
            VERIFY_IS_GREATER_THAN(static_cast<unsigned int>(firstRealizedIndex), list->Items->Size - 20);
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

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for ListBox exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            Common::AutoVariant autoVar;

            LOG_OUTPUT(L"Navigate to First Item in scrolled ListBox and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == firstRealizedIndex + 1);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);

            LOG_OUTPUT(L"Navigate to Second Item in scrolled ListBox and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempAAutomationElement.Get(), &spUIListItemTempBAutomationElement), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get second ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempBAutomationElement);

            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == firstRealizedIndex + 2);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempBAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);

            LOG_OUTPUT(L"Navigate to third Item in scrolled ListBox and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListItemTempBAutomationElement.Get(), &spUIListItemTempAAutomationElement), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get third ListItem.");
            VERIFY_IS_NOT_NULL(spUIListItemTempAAutomationElement);

            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == firstRealizedIndex + 3);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1000);
            spUIListItemTempAAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);
        });
    }



} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ListBox
