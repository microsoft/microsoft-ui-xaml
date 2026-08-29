// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AppBarButtonAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <generic\ButtonBaseTests.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <AutomationClient\AutomationClientInitializer.h>
#include <Patterns\InvokePatternHandler.h>
#include <TreeHelper.h>
#include <UIAutomationHelper.h>
#include <PopupHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Button {

    bool AppBarButtonAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool AppBarButtonAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AppBarButtonAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void AppBarButtonAutomationIntegrationTests::VerifyDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestButton";
        uiaInfo.m_AutomationID = L"TestButton";
        uiaInfo.m_ItemStatus = L"TestButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->VerticalAlignment = xaml::VerticalAlignment::Top;
            cmdBar->IsOpen = true;

            auto appBarButtonWithName = ref new xaml_controls::AppBarButton();
            xaml_automation::AutomationProperties::SetName(appBarButtonWithName, ref new Platform::String(uiaInfo.m_Name));
            appBarButtonWithName->Label = "";
            cmdBar->PrimaryCommands->Append(appBarButtonWithName);

            auto appBarButtonWithLabel = ref new xaml_controls::AppBarButton();
            appBarButtonWithLabel->Label = "TestButton";
            cmdBar->PrimaryCommands->Append(appBarButtonWithLabel);

            auto appBarButtonWithNeither = ref new xaml_controls::AppBarButton();
            appBarButtonWithNeither->Label = "";
            cmdBar->PrimaryCommands->Append(appBarButtonWithNeither);

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(cmdBar);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spAppBarButtonWithName;
            wrl::ComPtr<IUIAutomationElement> spAppBarButtonWithLabel;
            wrl::ComPtr<IUIAutomationElement> spAppBarButtonWithNeither;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spAppBarButtonWithName);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for AppBar exists.");
            VERIFY_IS_NOT_NULL(spAppBarButtonWithName);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spAppBarButtonWithName.");
            spAppBarButtonWithName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the second button in app bar.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spAppBarButtonWithName.Get(), &spAppBarButtonWithLabel);
            VERIFY_IS_NOT_NULL(spAppBarButtonWithLabel);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spAppBarButtonWithLabel.");
            spAppBarButtonWithLabel->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the third button in app bar.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spAppBarButtonWithLabel.Get(), &spAppBarButtonWithNeither);
            VERIFY_IS_NOT_NULL(spAppBarButtonWithNeither);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spAppBarButtonWithNeither.");
            spAppBarButtonWithNeither->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Verifying UIA LocalizedControlType property from Client side node for AppBarButton.");
            Common::AutoBSTR autoBstr;
            spAppBarButtonWithName->get_CurrentLocalizedControlType(autoBstr.ReleaseAndGetAddressOf());
            AutoBSTR::VerifyAreEqual(L"app bar button", autoBstr);
        });
    }

    void AppBarButtonAutomationIntegrationTests::VerifyHasNoUIAControlChildren()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestButton";
        uiaInfo.m_AutomationID = L"TestButton";
        uiaInfo.m_ItemStatus = L"TestButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->VerticalAlignment = xaml::VerticalAlignment::Top;
            cmdBar->IsOpen = true;

            auto appBarButton = ref new xaml_controls::AppBarButton();
            xaml_automation::AutomationProperties::SetName(appBarButton, ref new Platform::String(uiaInfo.m_Name));
            appBarButton->Label = "TestButton";
            cmdBar->PrimaryCommands->Append(appBarButton);

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(cmdBar);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spAppBarButton;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            wrl::ComPtr<IUIAutomationElementArray> spItems;
            int itemCount;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spAppBarButton);

            spItems = UIAutomationHelper::GetChildren(spUIAutomation.Get(), spAppBarButton.Get());
            VERIFY_IS_NOT_NULL(spItems);

            LogThrow_IfFailedWithMessage(spItems->get_Length(&itemCount), L"Failed to retrieve item count.");

            for (int i = 0; i < itemCount; i++)
            {
                wrl::ComPtr<IUIAutomationElement> spChildItem;
                BOOL isControlElement;

                LogThrow_IfFailedWithMessage(spItems->GetElement(i, &spChildItem), L"Failed to retrieve child item.");
                VERIFY_IS_NOT_NULL(spChildItem);
                spChildItem->get_CurrentIsControlElement(&isControlElement);
                VERIFY_IS_FALSE(!!isControlElement);
            }
        });
    }

    void AppBarButtonAutomationIntegrationTests::VerifySupportsExpandCollapsePatternWithFlyout()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestButton";
        uiaInfo.m_AutomationID = L"TestButton";
        uiaInfo.m_ItemStatus = L"TestButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfo2;
        uiaInfo2.m_Name = L"TestButton2";
        uiaInfo2.m_AutomationID = L"TestButton2";
        uiaInfo2.m_ItemStatus = L"TestButton2";
        uiaInfo2.m_cType = UIA_ButtonControlTypeId;

        auto commandBarLoadedEvent = std::make_shared<Event>();
        auto commandBarLoadedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);
        auto commandBarOpenedEvent = std::make_shared<Event>();
        auto commandBarOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto commandBarClosedEvent = std::make_shared<Event>();
        auto commandBarClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        xaml_controls::CommandBar^ commandBar = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;

        RunOnUIThread([&]()
        {
            commandBar = ref new xaml_controls::CommandBar();
            commandBar->VerticalAlignment = xaml::VerticalAlignment::Top;

            commandBarLoadedEventRegistration.Attach(commandBar, [&]() { commandBarLoadedEvent->Set(); });
            commandBarOpenedEventRegistration.Attach(commandBar, [&]() { commandBarOpenedEvent->Set(); });
            commandBarClosedEventRegistration.Attach(commandBar, [&]() { commandBarClosedEvent->Set(); });

            auto appBarButton = ref new xaml_controls::AppBarButton();
            xaml_automation::AutomationProperties::SetName(appBarButton, ref new Platform::String(uiaInfo.m_Name));
            appBarButton->Label = "TestButton";
            commandBar->SecondaryCommands->Append(appBarButton);

            auto appBarButton2 = ref new xaml_controls::AppBarButton();
            xaml_automation::AutomationProperties::SetName(appBarButton2, ref new Platform::String(uiaInfo2.m_Name));
            appBarButton2->Label = "TestButton2";
            flyout = ref new xaml_controls::Flyout();
            flyout->Content = ref new xaml_controls::StackPanel();
            appBarButton2->Flyout = flyout;
            commandBar->SecondaryCommands->Append(appBarButton2);

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(commandBar);

            flyoutOpenedEventRegistration.Attach(flyout, [&]()
            {
                flyoutOpenedEvent->Set();
            });

            flyoutClosedEventRegistration.Attach(flyout, [&]()
            {
                flyoutClosedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        commandBarLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar->IsOpen = true;
        });

        commandBarOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        wrl::ComPtr<IUIAutomationExpandCollapsePattern> appBarButtonAsExpandCollapse;

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(commandBar);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> appBarButton;

            LOG_OUTPUT(L"Ensuring that the AppBarButton without a flyout does not report the ExpandCollapse pattern.");
            auto automationClientManager =
                PopupHelper::AreWindowedPopupsEnabled() ?
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, commandBar) :
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            automationClientManager->GetCurrentUIAutomationElement(&appBarButton);
            appBarButton->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, __uuidof(IUIAutomationExpandCollapsePattern), &appBarButtonAsExpandCollapse);
            VERIFY_IS_NULL(appBarButtonAsExpandCollapse.Get());

            LOG_OUTPUT(L"Ensuring that the AppBarButton with a flyout does report the ExpandCollapse pattern.");
            automationClientManager =
                PopupHelper::AreWindowedPopupsEnabled() ?
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo2, commandBar) :
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo2);

            automationClientManager->GetCurrentUIAutomationElement(&appBarButton);
            appBarButton->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, __uuidof(IUIAutomationExpandCollapsePattern), &appBarButtonAsExpandCollapse);
            VERIFY_IS_NOT_NULL(appBarButtonAsExpandCollapse.Get());

            LOG_OUTPUT(L"Expanding the AppBarButton with a flyout.");
            appBarButtonAsExpandCollapse->Expand();
        });

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            LOG_OUTPUT(L"Collapsing the AppBarButton with a flyout.");
            appBarButtonAsExpandCollapse->Collapse();
        });

        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar->IsOpen = false;
        });

        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Button
