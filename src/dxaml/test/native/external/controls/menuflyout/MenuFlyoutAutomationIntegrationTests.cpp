// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MenuFlyoutAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <AutomationClient\AutomationClientInitializer.h>
#include <AutomationClient\AutomationClientManager.h>
#include "TestCleanupWrapper.h"
#include <FlyoutHelper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace MenuFlyout {

    bool MenuFlyoutAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool MenuFlyoutAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool MenuFlyoutAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    xaml_controls::Canvas^ SetupRootPanelForSubMenuTest()
    {
        xaml_controls::Canvas^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                L"<Canvas Background='RoyalBlue' "
                L" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <Button x:Name='button1' Content='Button' Width='100' Height='50' Canvas.Left='50' Canvas.Top='50' />"
                L"</Canvas>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return rootPanel;
    }

    xaml_controls::MenuFlyout^ CreateMenuFlyoutFromXaml()
    {
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>Menu item 1</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 2</MenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 3</ToggleMenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <MenuFlyoutSubItem x:Name='subItem1' Text='Menu sub item 4'>"
                L"        <MenuFlyoutItem>Menu item 2.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Menu item 2.2</MenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 2.3</ToggleMenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <MenuFlyoutSubItem  x:Name='subItem2' Text='Menu sub item 2.4'>"
                L"            <MenuFlyoutItem>Menu item 3.1</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.2</MenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"            <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 3.3</ToggleMenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"        </MenuFlyoutSubItem>"
                L"    </MenuFlyoutSubItem>"
                L"    <MenuFlyoutSubItem x:Name='splitItem1' Text='Split menu item 5'>"
                L"        <MenuFlyoutItem>Split item 5.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Split item 5.2</MenuFlyoutItem>"
                L"        <ToggleMenuFlyoutItem IsChecked='True'>Toggle split item 5.3</ToggleMenuFlyoutItem>"
                L"    </MenuFlyoutSubItem>"
                L"    <MenuFlyoutItem>Menu item 6</MenuFlyoutItem>"
                L"</MenuFlyout>"));
        });

        return menuFlyout;
    }

    void ShowMenuFlyout(xaml_controls::MenuFlyout^ menuFlyout, xaml::UIElement^ relativeTo, float horizontalOffset, float verticalOffset)
    {
        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        RunOnUIThread([&]()
        {
            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([openedEvent](Platform::Object^, Platform::Object^)
            {
                openedEvent->Set();
            }));

            menuFlyout->ShowAt(relativeTo, wf::Point(horizontalOffset, verticalOffset));
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutAutomationIntegrationTests::VerifyAutomationProperties()
    {
        TestCleanupWrapper cleanup;

        WCHAR automationName[] = L"TestMenuFlyoutName";
        WCHAR automationId[] = L"TestMenuFlyoutId";
        WCHAR automationItemStatus[] = L"TestMenuFlyout";
        CONTROLTYPEID automationTypeId = UIA_WindowControlTypeId;

        xaml_controls::Button^ button1 = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutFromXaml();

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(menuFlyout, ref new Platform::String(automationName));
            xaml_automation::AutomationProperties::SetAutomationId(menuFlyout, ref new Platform::String(automationId));
        });

        RunOnUIThread([&]()
        {
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });

        ShowMenuFlyout(menuFlyout, button1, DefaultHorizontalOffset, DefaultVerticalOffset);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(button1);  
        }

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = automationName;
        uiaInfo.m_AutomationID = automationId;
        uiaInfo.m_ItemStatus = automationItemStatus;
        uiaInfo.m_cType = automationTypeId;

        HWND handle = FlyoutHelper::GetUIAHandleToUseForWindowedFlyout(menuFlyout);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomation> spUIAutomation;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(handle, uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LOG_OUTPUT(L"Verifying that UIA client-side node for MenuFlyoutPresenter exists.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            AutoBSTR propertyValue;

            LOG_OUTPUT(L"Verifying the UIA name property from client-side node for MenuFlyoutPresenter.");
            spUIAutomationElement->get_CurrentName(propertyValue.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"NamePropertyId = '%s', Expected = '%s'.", propertyValue.Get(), automationName);
            VERIFY_IS_TRUE(!wcscmp(automationName, propertyValue));

            LOG_OUTPUT(L"Verifying the UIA class name property from client-side node for MenuFlyoutPresenter.");
            spUIAutomationElement->get_CurrentClassName(propertyValue.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"ClassNamePropertyId = '%s', Expected = '%s'.", propertyValue.Get(), L"MenuFlyout");
            VERIFY_IS_TRUE(!wcscmp(L"MenuFlyout", propertyValue));
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    //  Loads simple markup with MenuFlyout and verifies AutomationProperties are correct.
    void MenuFlyoutAutomationIntegrationTests::VerifyFlyoutMenuSubItemAutomation()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutFromXaml();

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"Menu sub item 4";
        uiaInfo.m_AutomationID = L"subItem1";
        uiaInfo.m_cType = UIA_MenuItemControlTypeId;

        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomation> spAutomation;
        wrl::ComPtr<IUIAutomationExpandCollapsePattern> spExpandCollapsePattern;

        RunOnUIThread([&]()
        {
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            items = menuFlyout->Items;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button1, DefaultHorizontalOffset, DefaultVerticalOffset);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(button1);  
        }

        HWND handle = FlyoutHelper::GetUIAHandleToUseForWindowedFlyout(menuFlyout);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(handle, uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            spAutomationClientManager->GetAutomation(&spAutomation);
            VERIFY_IS_NOT_NULL(spAutomation.Get());

            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, __uuidof(IUIAutomationExpandCollapsePattern), &spExpandCollapsePattern));
            VERIFY_IS_NOT_NULL(spExpandCollapsePattern);

            ExpandCollapseState retVal = ExpandCollapseState_Expanded;
            VERIFY_SUCCEEDED(spExpandCollapsePattern->get_CurrentExpandCollapseState(&retVal));
            VERIFY_ARE_EQUAL(retVal, ExpandCollapseState_Collapsed);

            Common::AutoVariant autoVar;
            spUIAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Verifying PositionInSet is 4.");
            VERIFY_ARE_EQUAL(autoVar.Storage()->lVal, 4);

            spUIAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Verifying SizeOfSet is 6.");
            VERIFY_ARE_EQUAL(autoVar.Storage()->lVal, 6);

            VERIFY_SUCCEEDED(spExpandCollapsePattern->Expand());
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            ExpandCollapseState retVal = ExpandCollapseState_Collapsed;
            VERIFY_SUCCEEDED(spExpandCollapsePattern->get_CurrentExpandCollapseState(&retVal));
            VERIFY_ARE_EQUAL(retVal, ExpandCollapseState_Expanded);

            VERIFY_SUCCEEDED(spExpandCollapsePattern->Collapse());
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            ExpandCollapseState retVal = ExpandCollapseState_Expanded;
            VERIFY_SUCCEEDED(spExpandCollapsePattern->get_CurrentExpandCollapseState(&retVal));
            VERIFY_ARE_EQUAL(retVal, ExpandCollapseState_Collapsed);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    //  Loads simple markup with MenuFlyout and verifies AutomationProperties are correct for Split Menu Items.
    void MenuFlyoutAutomationIntegrationTests::VerifyFlyoutMenuSplitItemAutomation()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutFromXaml();

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"Split menu item 5";
        uiaInfo.m_AutomationID = L"splitItem1";
        uiaInfo.m_cType = UIA_MenuItemControlTypeId;

        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomation> spAutomation;
        wrl::ComPtr<IUIAutomationExpandCollapsePattern> spExpandCollapsePattern;
        wrl::ComPtr<IUIAutomationInvokePattern> spInvokePattern;

        RunOnUIThread([&]()
        {
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            items = menuFlyout->Items;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button1, DefaultHorizontalOffset, DefaultVerticalOffset);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(button1);  
        }

        HWND handle = FlyoutHelper::GetUIAHandleToUseForWindowedFlyout(menuFlyout);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(handle, uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            spAutomationClientManager->GetAutomation(&spAutomation);
            VERIFY_IS_NOT_NULL(spAutomation.Get());

            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, __uuidof(IUIAutomationExpandCollapsePattern), &spExpandCollapsePattern));
            VERIFY_IS_NOT_NULL(spExpandCollapsePattern);

            ExpandCollapseState retVal = ExpandCollapseState_Expanded;
            VERIFY_SUCCEEDED(spExpandCollapsePattern->get_CurrentExpandCollapseState(&retVal));
            VERIFY_ARE_EQUAL(retVal, ExpandCollapseState_Collapsed);

            Common::AutoVariant autoVar;
            spUIAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Verifying PositionInSet is 5.");
            VERIFY_ARE_EQUAL(autoVar.Storage()->lVal, 5);

            spUIAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Verifying SizeOfSet is 6.");
            VERIFY_ARE_EQUAL(autoVar.Storage()->lVal, 6);

            // Verify that the split item supports Invoke pattern
            // VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), &spInvokePattern));
            // VERIFY_IS_NOT_NULL(spInvokePattern);

            VERIFY_SUCCEEDED(spExpandCollapsePattern->Expand());
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            ExpandCollapseState retVal = ExpandCollapseState_Collapsed;
            VERIFY_SUCCEEDED(spExpandCollapsePattern->get_CurrentExpandCollapseState(&retVal));
            VERIFY_ARE_EQUAL(retVal, ExpandCollapseState_Expanded);

            VERIFY_SUCCEEDED(spExpandCollapsePattern->Collapse());
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            ExpandCollapseState retVal = ExpandCollapseState_Expanded;
            VERIFY_SUCCEEDED(spExpandCollapsePattern->get_CurrentExpandCollapseState(&retVal));
            VERIFY_ARE_EQUAL(retVal, ExpandCollapseState_Collapsed);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutAutomationIntegrationTests::VerifyOverridingAutomationNameForSplitMenuItem()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>Menu item 1</MenuFlyoutItem>"
                L"    <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split Item' AutomationProperties.Name='Custom Split Item'>"
                L"        <MenuFlyoutItem>Split child 1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Split child 2</MenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"</MenuFlyout>"));

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            splitItem = safe_cast<xaml_controls::SplitMenuFlyoutItem^>(menuFlyout->Items->GetAt(1));
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button1, DefaultHorizontalOffset, DefaultVerticalOffset);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(button1);
        }

        HWND handle = FlyoutHelper::GetUIAHandleToUseForWindowedFlyout(menuFlyout);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"Custom Split Item";
            uiaInfo.m_AutomationID = L"splitItem1";
            uiaInfo.m_cType = UIA_MenuItemControlTypeId;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(handle, uiaInfo);

            wrl::ComPtr<IUIAutomation> spAutomation;
            spAutomationClientManager->GetAutomation(&spAutomation);

            wrl::ComPtr<IUIAutomationElement> spSplitItemElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spSplitItemElement);
            VERIFY_IS_NOT_NULL(spSplitItemElement.Get());

            // Verify split item itself has the custom automation name
            AutoBSTR splitItemName;
            VERIFY_SUCCEEDED(spSplitItemElement->get_CurrentName(splitItemName.ReleaseAndGetAddressOf()));
            LOG_OUTPUT(L"Actual: '%s', Expected: 'Custom Split Item'", splitItemName.Get());
            VERIFY_IS_TRUE(!wcscmp(L"Custom Split Item", splitItemName));

            // Get tree walker to navigate siblings
            wrl::ComPtr<IUIAutomationTreeWalker> spTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spTrueCondition;
            LogThrow_IfFailedWithMessage(spAutomation->CreateTrueCondition(&spTrueCondition), L"Failed creating TrueCondition");
            LogThrow_IfFailedWithMessage(spAutomation->CreateTreeWalker(spTrueCondition.Get(), &spTreeWalker), L"Failed creating TreeWalker");

            // Get secondary button (first child of split menu item)
            // Since SplitMenuFlyoutItem's EventSource is the primary button, primary button is not included in
            // the children of SplitMenuFlyoutItem. So first child is secondary button.
            wrl::ComPtr<IUIAutomationElement> spSecondaryButton;
            LogThrow_IfFailedWithMessage(spTreeWalker->GetFirstChildElement(spSplitItemElement.Get(), &spSecondaryButton), L"Failed to get primary button");
            VERIFY_IS_NOT_NULL(spSecondaryButton.Get());

            // Verify secondary button automation name is overridden to "Custom Split Name, More options"
            AutoBSTR secondaryName;
            VERIFY_SUCCEEDED(spSecondaryButton->get_CurrentName(secondaryName.ReleaseAndGetAddressOf()));
            LOG_OUTPUT(L"Secondary button name: '%s', Expected: 'More options for Custom Split Item'", secondaryName.Get());
            VERIFY_IS_TRUE(!wcscmp(L"More options for Custom Split Item", secondaryName));
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutAutomationIntegrationTests::VerifySplitMenuItemAutomationTreeStructure()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>Menu item 1</MenuFlyoutItem>"
                L"    <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split Item 2'>"
                L"        <MenuFlyoutItem>Split child 1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Split child 2</MenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 3</MenuFlyoutItem>"
                L"</MenuFlyout>"));

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button1, DefaultHorizontalOffset, DefaultVerticalOffset);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(button1);
        }

        HWND handle = FlyoutHelper::GetUIAHandleToUseForWindowedFlyout(menuFlyout);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"Split Item 2";
            uiaInfo.m_AutomationID = L"splitItem1";
            uiaInfo.m_cType = UIA_MenuItemControlTypeId;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(handle, uiaInfo);

            wrl::ComPtr<IUIAutomation> spAutomation;
            spAutomationClientManager->GetAutomation(&spAutomation);

            wrl::ComPtr<IUIAutomationElement> spSplitItemElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spSplitItemElement);
            VERIFY_IS_NOT_NULL(spSplitItemElement.Get());

            // Verify split item PositionInSet and SizeOfSet
            Common::AutoVariant splitItemAutoVar;
            spSplitItemElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, splitItemAutoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Split item PositionInSet: %d, Expected: 2", splitItemAutoVar.Storage()->lVal);
            VERIFY_ARE_EQUAL(2, splitItemAutoVar.Storage()->lVal);

            spSplitItemElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, splitItemAutoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Split item SizeOfSet: %d, Expected: 3", splitItemAutoVar.Storage()->lVal);
            VERIFY_ARE_EQUAL(3, splitItemAutoVar.Storage()->lVal);

            // Get tree walker to navigate siblings
            wrl::ComPtr<IUIAutomationTreeWalker> spTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spTrueCondition;
            LogThrow_IfFailedWithMessage(spAutomation->CreateTrueCondition(&spTrueCondition), L"Failed creating TrueCondition");
            LogThrow_IfFailedWithMessage(spAutomation->CreateTreeWalker(spTrueCondition.Get(), &spTreeWalker), L"Failed creating TreeWalker");

            // Get the first child of split item (should be secondary button, as primary button is filtered out)
            wrl::ComPtr<IUIAutomationElement> spSecondaryButton;
            LogThrow_IfFailedWithMessage(spTreeWalker->GetFirstChildElement(spSplitItemElement.Get(), &spSecondaryButton), L"Failed to get first child (secondary button)");
            VERIFY_IS_NOT_NULL(spSecondaryButton.Get());

            // Get next sibling after split item (should be Menu item 3)
            wrl::ComPtr<IUIAutomationElement> spNextMenuItem;
            LogThrow_IfFailedWithMessage(spTreeWalker->GetNextSiblingElement(spSplitItemElement.Get(), &spNextMenuItem), L"Failed to get next sibling");
            VERIFY_IS_NOT_NULL(spNextMenuItem.Get());

            // Verify next menu item name
            AutoBSTR nextItemName;
            VERIFY_SUCCEEDED(spNextMenuItem->get_CurrentName(nextItemName.ReleaseAndGetAddressOf()));
            LOG_OUTPUT(L"Next menu item name: '%s', Expected: 'Menu item 3'", nextItemName.Get());
            VERIFY_IS_TRUE(!wcscmp(L"Menu item 3", nextItemName));

            // Verify next menu item PositionInSet and SizeOfSet
            Common::AutoVariant nextItemAutoVar;
            spNextMenuItem->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, nextItemAutoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Next menu item PositionInSet: %d, Expected: 3", nextItemAutoVar.Storage()->lVal);
            VERIFY_ARE_EQUAL(3, nextItemAutoVar.Storage()->lVal);

            spNextMenuItem->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, nextItemAutoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Next menu item SizeOfSet: %d, Expected: 3", nextItemAutoVar.Storage()->lVal);
            VERIFY_ARE_EQUAL(3, nextItemAutoVar.Storage()->lVal);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutAutomationIntegrationTests::VerifyItemIsNotReadWhenCollapsed()
    {
        TestCleanupWrapper cleanup;

        WCHAR automationName[] = L"TestMenuFlyoutName";
        WCHAR automationId[] = L"TestMenuFlyoutId";

        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();

        xaml_controls::MenuFlyout^ menuFlyout;
        xaml_controls::MenuFlyoutItem^ hiddenItem;
        xaml_controls::MenuFlyoutSubItem^ hiddenSubItem;
        xaml_controls::SplitMenuFlyoutItem^ hiddenSplitItem;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>0</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>1</MenuFlyoutItem>"
                L"    <MenuFlyoutItem x:Name='hiddenItem'>2</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>3</MenuFlyoutItem>"
                L"    <MenuFlyoutSubItem x:Name='hiddenSubItem' Text='4'>"
                L"        <MenuFlyoutItem>4.1</MenuFlyoutItem>"
                L"    </MenuFlyoutSubItem>"
                L"    <MenuFlyoutSubItem Text='5'>"
                L"        <MenuFlyoutItem>5.1</MenuFlyoutItem>"
                L"    </MenuFlyoutSubItem>"
                L"    <SplitMenuFlyoutItem x:Name='hiddenSplitItem' Text='6'>"
                L"        <MenuFlyoutItem>6.1</MenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"    <SplitMenuFlyoutItem Text='7'>"
                L"        <MenuFlyoutItem>7.1</MenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"</MenuFlyout>"));
        });

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(menuFlyout, ref new Platform::String(automationName));
            xaml_automation::AutomationProperties::SetAutomationId(menuFlyout, ref new Platform::String(automationId));

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            hiddenItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyout->Items->GetAt(2));
            hiddenSubItem = safe_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyout->Items->GetAt(4));
            hiddenSplitItem = safe_cast<xaml_controls::SplitMenuFlyoutItem^>(menuFlyout->Items->GetAt(6));
        });

        ShowMenuFlyout(menuFlyout, button1, DefaultHorizontalOffset, DefaultVerticalOffset);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(button1);  
        }

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = automationName;
        uiaInfo.m_AutomationID = automationId;
        uiaInfo.m_cType = UIA_MenuItemControlTypeId;

        HWND handle = FlyoutHelper::GetUIAHandleToUseForWindowedFlyout(menuFlyout);

        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomation> spUIAutomation;

        wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
        wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;

        wrl::ComPtr<IUIAutomationElement> spNextSibling;
        wrl::ComPtr<IUIAutomationElement> spCurrentElement;
        Common::AutoVariant autoVar;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(handle, uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"Failed creating TrueCondition");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"Failed in creating TreeWalker.");

            // Verify that all items are returned
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spCurrentElement));
            for (int i = 0; i < 8; i++)
            {
                VERIFY_IS_NOT_NULL(spCurrentElement);

                VERIFY_SUCCEEDED(spCurrentElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));

                Platform::String^ expectedUIAName = i.ToString();
                Platform::String^ actualUIAName = ref new Platform::String((autoVar.Storage())->bstrVal);

                VERIFY_ARE_EQUAL(expectedUIAName, actualUIAName);

                spNextSibling = nullptr;
                VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spCurrentElement.Get(), &spNextSibling));
                spCurrentElement = spNextSibling;
            }
        });

        // Set the visibility of hitddenItem, hiddenSubItem and hiddenSplitItem to collapsed
        RunOnUIThread([&]()
        {
            hiddenItem->Visibility = xaml::Visibility::Collapsed;
            hiddenSubItem->Visibility = xaml::Visibility::Collapsed;
            hiddenSplitItem->Visibility = xaml::Visibility::Collapsed;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            // Verify that items 0, 1, 3, 5 and 7 are returned
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spCurrentElement));
            for (int i = 0; i < 8; i++)
            {
                if(i>0 && i%2 == 0)
                {
                    continue;
                }

                VERIFY_IS_NOT_NULL(spCurrentElement);

                VERIFY_SUCCEEDED(spCurrentElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));

                Platform::String^ expectedUIAName;
                Platform::String^ actualUIAName = ref new Platform::String((autoVar.Storage())->bstrVal);

                // Convert the int i to string expectedItemString if it is item 0 or 1
                expectedUIAName = i.ToString();
                VERIFY_ARE_EQUAL(expectedUIAName, actualUIAName);

                spNextSibling = nullptr;
                VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spCurrentElement.Get(), &spNextSibling));
                spCurrentElement = spNextSibling;
            }
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutAutomationIntegrationTests::DoesSizeOfSetAndPositionInSetExcludeSeparatorsAndCollapsedItems()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        wchar_t automationName[] = L"TestMenuFlyoutName";
        wchar_t automationId[] = L"TestMenuFlyoutId";

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                LR"(<MenuFlyout xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
                        <MenuFlyoutItem Text="Collapsed" Visibility="Collapsed"/>
                        <MenuFlyoutItem Text="Item 1"/>
                        <MenuFlyoutSeparator/>
                        <ToggleMenuFlyoutItem Text="Item 2"/>
                        <MenuFlyoutSubItem Text="Item 3"/>
                        <MenuFlyoutSeparator/>
                        <MenuFlyoutSubItem Text="Collapsed Sub Item" Visibility="Collapsed">
                            <MenuFlyoutItem Text="Sub Item 1"/>
                        </MenuFlyoutSubItem>
                        <MenuFlyoutSubItem Text="Item 4">
                            <MenuFlyoutItem Text="Sub Item 2"/>
                        </MenuFlyoutSubItem>
                        <SplitMenuFlyoutItem Text="Collapsed Split Item" Visibility="Collapsed">
                            <MenuFlyoutItem Text="Split Item 1"/>
                        </SplitMenuFlyoutItem>
                        <SplitMenuFlyoutItem Text="Item 5">
                            <MenuFlyoutItem Text="Split Item 2"/>
                        </SplitMenuFlyoutItem>
                    </MenuFlyout>)"));

            xaml_automation::AutomationProperties::SetName(menuFlyout, ref new Platform::String(automationName));
            xaml_automation::AutomationProperties::SetAutomationId(menuFlyout, ref new Platform::String(automationId));

            button = ref new xaml_controls::Button();
            button->Content = ref new Platform::String(L"Button!");
            button->Flyout = menuFlyout;

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(button);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button, 0, 0);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(button);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = automationName;
            uiaInfo.m_AutomationID = automationId;
            uiaInfo.m_cType = UIA_MenuItemControlTypeId;

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(FlyoutHelper::GetUIAHandleToUseForWindowedFlyout(menuFlyout), uiaInfo);

            wrl::ComPtr<IUIAutomation> automation;
            automationClientManager->GetAutomation(&automation);

            wrl::ComPtr<IUIAutomationElement> automationElement;
            automationClientManager->GetCurrentUIAutomationElement(&automationElement);

            wrl::ComPtr<IUIAutomationCondition> automationTrueCondition;
            LogThrow_IfFailedWithMessage(automation->CreateTrueCondition(&automationTrueCondition), L"Failed creating TrueCondition");

            wrl::ComPtr<IUIAutomationTreeWalker> treeWalker;
            LogThrow_IfFailedWithMessage(automation->CreateTreeWalker(automationTrueCondition.Get(), &treeWalker), L"Failed in creating TreeWalker.");

            wrl::ComPtr<IUIAutomationElement> currentElement;
            LogThrow_IfFailedWithMessage(treeWalker->GetFirstChildElement(automationElement.Get(), &currentElement), L"Failed to get first child.");

            const LONG expectedSizeOfSet = 5;
            LONG expectedPositionInSet = 1;

            while (expectedPositionInSet <= expectedSizeOfSet)
            {
                Common::AutoVariant autoVar;

                LogThrow_IfFailed(currentElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(expectedSizeOfSet, autoVar.Get().lVal);

                LogThrow_IfFailed(currentElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(expectedPositionInSet, autoVar.Get().lVal);

                wrl::ComPtr<IUIAutomationElement> nextElement;
                LogThrow_IfFailedWithMessage(treeWalker->GetNextSiblingElement(currentElement.Get(), &nextElement), L"Failed to get next sibling");
                currentElement = nextElement;

                ++expectedPositionInSet;
            }
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutAutomationIntegrationTests::VerifyScrollItemPattern()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutFromXaml();

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"Menu item 1";
        uiaInfo.m_AutomationID = L"MenuItem1";
        uiaInfo.m_cType = UIA_MenuItemControlTypeId;

        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomationScrollItemPattern> spScrollItemPattern;

        RunOnUIThread([&]()
        {
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button1, DefaultHorizontalOffset, DefaultVerticalOffset);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(button1);
        }

        HWND handle = FlyoutHelper::GetUIAHandleToUseForWindowedFlyout(menuFlyout);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(handle, uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &spScrollItemPattern));
            VERIFY_IS_NOT_NULL(spScrollItemPattern);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutAutomationIntegrationTests::VerifySizeAndPositionInSetCanBeOverridden()
    {
        TestCleanupWrapper cleanup;

        // Verify that the app author can set the PositionInSet and SizeOfSet AutomationProperties to override the
        // default values.

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        wchar_t automationName[] = L"TestMenuFlyoutName";
        wchar_t automationId[] = L"TestMenuFlyoutId";

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                LR"(<MenuFlyout xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
                        <MenuFlyoutItem Text="Item 1" AutomationProperties.PositionInSet="10" AutomationProperties.SizeOfSet="40" />
                        <ToggleMenuFlyoutItem Text="Item 2" AutomationProperties.PositionInSet="20" AutomationProperties.SizeOfSet="40" />
                        <MenuFlyoutSubItem Text="Item 3" AutomationProperties.PositionInSet="30" AutomationProperties.SizeOfSet="40" />
                        <SplitMenuFlyoutItem Text="Item 4" AutomationProperties.PositionInSet="40" AutomationProperties.SizeOfSet="40" />
                    </MenuFlyout>)"));

            xaml_automation::AutomationProperties::SetName(menuFlyout, ref new Platform::String(automationName));
            xaml_automation::AutomationProperties::SetAutomationId(menuFlyout, ref new Platform::String(automationId));

            button = ref new xaml_controls::Button();
            button->Content = ref new Platform::String(L"Button!");
            button->Flyout = menuFlyout;

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(button);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button, 0, 0);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(button);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = automationName;
            uiaInfo.m_AutomationID = automationId;
            uiaInfo.m_cType = UIA_MenuItemControlTypeId;

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(FlyoutHelper::GetUIAHandleToUseForWindowedFlyout(menuFlyout), uiaInfo);

            wrl::ComPtr<IUIAutomation> automation;
            automationClientManager->GetAutomation(&automation);

            wrl::ComPtr<IUIAutomationElement> automationElement;
            automationClientManager->GetCurrentUIAutomationElement(&automationElement);

            wrl::ComPtr<IUIAutomationCondition> automationTrueCondition;
            LogThrow_IfFailedWithMessage(automation->CreateTrueCondition(&automationTrueCondition), L"Failed creating TrueCondition");

            wrl::ComPtr<IUIAutomationTreeWalker> treeWalker;
            LogThrow_IfFailedWithMessage(automation->CreateTreeWalker(automationTrueCondition.Get(), &treeWalker), L"Failed in creating TreeWalker.");

            wrl::ComPtr<IUIAutomationElement> currentElement;
            LogThrow_IfFailedWithMessage(treeWalker->GetFirstChildElement(automationElement.Get(), &currentElement), L"Failed to get first child.");

            // The items are set as 10/40, 20/40, 30/40 and 40/40, so we jump up in tens:
            const LONG expectedSizeOfSet = 40;
            LONG expectedPositionInSet = 10;
            while (expectedPositionInSet <= expectedSizeOfSet)
            {
                Common::AutoVariant autoVar;

                LogThrow_IfFailed(currentElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(expectedSizeOfSet, autoVar.Get().lVal);

                LogThrow_IfFailed(currentElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(expectedPositionInSet, autoVar.Get().lVal);

                wrl::ComPtr<IUIAutomationElement> nextElement;
                LogThrow_IfFailedWithMessage(treeWalker->GetNextSiblingElement(currentElement.Get(), &nextElement), L"Failed to get next sibling");
                currentElement = nextElement;

                expectedPositionInSet += 10;
            }
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::MenuFlyout
