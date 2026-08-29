// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CommandBarAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <AutomationClient\AutomationClientInitializer.h>
#include <AutomationClient\AutomationClientManager.h>
#include "TestCleanupWrapper.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <UIAutomationHelper.h>
#include <PopupHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace CommandBar {

    WCHAR CommandBarAutomationIntegrationTests::s_automationName[] = L"TestCommandBarName";
    WCHAR CommandBarAutomationIntegrationTests::s_automationId[] = L"TestCommandBarId";

    bool CommandBarAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        // Disable control state transitions to reduce test execution time.
        featureDisableTransitionsForTest.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTransitionsForTest, true);
        return true;
    }

    bool CommandBarAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool CommandBarAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    //  Loads simple markup with CommandBar and verifies AutomationProperties are correct.
    void CommandBarAutomationIntegrationTests::VerifyAutomationProperties()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        auto spHasLoadedEvent = std::make_shared<Event>();
        auto spHasUnloadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);
        auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Unloaded);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();

            auto button = ref new xaml_controls::AppBarButton();
            button->Label = "button";
            cmdBar->PrimaryCommands->Append(button);

            page = TestServices::WindowHelper->SetupSimulatedAppPage();

            loadedRegistration.Attach(cmdBar, ref new xaml::RoutedEventHandler([spHasLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                spHasLoadedEvent->Set();
            }));

            unloadedRegistration.Attach(cmdBar, ref new xaml::RoutedEventHandler([spHasUnloadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                spHasUnloadedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = false;
            page->BottomAppBar = cmdBar;
        });
        spHasLoadedEvent->WaitForDefault();

        // Find "More options" button.
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"More options";
        uiaInfo.m_AutomationID = s_automationId;
        uiaInfo.m_ItemStatus = L"TestCommandBar";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LOG_OUTPUT(L"Verifying UIA Client side node for More app bar button in CommandBar exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);
        });

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Find "Less app bar" button.
        uiaInfo.m_Name = L"Less app bar";

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LOG_OUTPUT(L"Verifying UIA Client side node for Less app bar button in CommandBar exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);
        });

        // We need to remove commandbar else there will be an assert.
        RunOnUIThread([&]()
        {
            page->BottomAppBar = nullptr;
        });
        spHasUnloadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarAutomationIntegrationTests::VerifyAutomationWindowPattern()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();

            auto button = ref new xaml_controls::AppBarButton();
            button->Label = "button";
            cmdBar->PrimaryCommands->Append(button);

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = true;
            page->BottomAppBar = cmdBar;

            xaml_automation::AutomationProperties::SetName(cmdBar, ref new Platform::String(s_automationName));
            xaml_automation::AutomationProperties::SetAutomationId(cmdBar, ref new Platform::String(s_automationId));
        });
        TestServices::WindowHelper->WaitForIdle();

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = s_automationName;
        uiaInfo.m_AutomationID = s_automationId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationCommandBar;
            wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationCommandBarWindowPattern;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationCommandBar);

            LOG_OUTPUT(L"Verifying that the CommandBar supports the Window pattern when it is opened.");
            spUIAutomationCommandBar->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationCommandBarWindowPattern);
            VERIFY_IS_NOT_NULL(spUIAutomationCommandBarWindowPattern);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto cmdBarAP = safe_cast<Microsoft::UI::Xaml::Automation::Provider::IWindowProvider^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(cmdBar));

            // Verify IWindowProdiver properties
            VERIFY_IS_TRUE(cmdBarAP->IsModal);
            VERIFY_IS_TRUE(cmdBarAP->IsTopmost);
            VERIFY_IS_FALSE(cmdBarAP->Maximizable);
            VERIFY_IS_FALSE(cmdBarAP->Minimizable);
            VERIFY_ARE_EQUAL(cmdBarAP->InteractionState, xaml_automation::WindowInteractionState::Running);
            VERIFY_ARE_EQUAL(cmdBarAP->VisualState, xaml_automation::WindowVisualState::Normal);

            // Verify IWindowProvider methods that doesn't do anything
            cmdBarAP->Close();
            cmdBarAP->SetVisualState(xaml_automation::WindowVisualState::Normal);
            cmdBarAP->WaitForInputIdle(0 /* milliseconds */);

            cmdBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationCommandBar;
            wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationCommandBarWindowPattern;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationCommandBar);

            LOG_OUTPUT(L"Verifying that the CommandBar doesn't supports the Window pattern when it is closed.");
            spUIAutomationCommandBar->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationCommandBarWindowPattern);
            VERIFY_IS_NULL(spUIAutomationCommandBarWindowPattern);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarAutomationIntegrationTests::VerifyPositionAndSize()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Page^ page = nullptr;
        xaml_controls::CommandBar^ commandBar = nullptr;

        auto hasLoadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);

        RunOnUIThread([&]()
        {
            commandBar = ref new xaml_controls::CommandBar();

            loadedRegistration.Attach(commandBar, ref new xaml::RoutedEventHandler([hasLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                hasLoadedEvent->Set();
            }));

            auto primaryAppBarButton = ref new xaml_controls::AppBarButton();
            xaml_automation::AutomationProperties::SetName(primaryAppBarButton, ref new Platform::String(L"PrimaryAppBarButton"));
            xaml_automation::AutomationProperties::SetAutomationId(primaryAppBarButton, ref new Platform::String(L"PrimaryAppBarButton"));
            commandBar->PrimaryCommands->Append(primaryAppBarButton);

            commandBar->PrimaryCommands->Append(ref new xaml_controls::AppBarSeparator());

            auto primaryAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
            xaml_automation::AutomationProperties::SetName(primaryAppBarToggleButton, ref new Platform::String(L"PrimaryAppBarToggleButton"));
            xaml_automation::AutomationProperties::SetAutomationId(primaryAppBarToggleButton, ref new Platform::String(L"PrimaryAppBarToggleButton"));
            commandBar->PrimaryCommands->Append(primaryAppBarToggleButton);

            auto primaryCollapsedAppBarButton = ref new xaml_controls::AppBarButton();
            primaryCollapsedAppBarButton->Visibility = xaml::Visibility::Collapsed;
            commandBar->PrimaryCommands->Append(primaryCollapsedAppBarButton);

            auto secondaryAppBarButton = ref new xaml_controls::AppBarButton();
            xaml_automation::AutomationProperties::SetName(secondaryAppBarButton, ref new Platform::String(L"SecondaryAppBarButton"));
            xaml_automation::AutomationProperties::SetAutomationId(secondaryAppBarButton, ref new Platform::String(L"SecondaryAppBarButton"));
            commandBar->SecondaryCommands->Append(secondaryAppBarButton);

            commandBar->SecondaryCommands->Append(ref new xaml_controls::AppBarSeparator());

            auto secondaryAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
            xaml_automation::AutomationProperties::SetName(secondaryAppBarToggleButton, ref new Platform::String(L"SecondaryAppBarToggleButton"));
            xaml_automation::AutomationProperties::SetAutomationId(secondaryAppBarToggleButton, ref new Platform::String(L"SecondaryAppBarToggleButton"));
            commandBar->SecondaryCommands->Append(secondaryAppBarToggleButton);

            auto secondaryCollapsedAppBarButton = ref new xaml_controls::AppBarButton();
            secondaryCollapsedAppBarButton->Visibility = xaml::Visibility::Collapsed;
            commandBar->PrimaryCommands->Append(secondaryCollapsedAppBarButton);

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = commandBar;
            commandBar->IsOpen = true;
        });

        hasLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(page);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"PrimaryAppBarButton";
            uiaInfo.m_AutomationID = L"PrimaryAppBarButton";

            int positionInSet = -1;
            int sizeOfSet = -1;

            wrl::ComPtr<IUIAutomationElement> uiAutomationElement;
            wrl::ComPtr<IUIAutomationElement4> uiAutomationElement4;

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
            VERIFY_IS_NOT_NULL(uiAutomationElement);
            VERIFY_SUCCEEDED(uiAutomationElement.As(&uiAutomationElement4));

            LOG_OUTPUT(L"The AppBarButton in PrimaryCommands should be the first element in a set of three.");
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentPositionInSet(&positionInSet));
            VERIFY_ARE_EQUAL(positionInSet, 1);
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentSizeOfSet(&sizeOfSet));
            VERIFY_ARE_EQUAL(sizeOfSet, 3);

            uiaInfo.m_Name = L"PrimaryAppBarToggleButton";
            uiaInfo.m_AutomationID = L"PrimaryAppBarToggleButton";

            automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
            VERIFY_IS_NOT_NULL(uiAutomationElement);
            VERIFY_SUCCEEDED(uiAutomationElement.As(&uiAutomationElement4));

            LOG_OUTPUT(L"The AppBarToggleButton in PrimaryCommands should be the second element in a set of three.");
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentPositionInSet(&positionInSet));
            VERIFY_ARE_EQUAL(positionInSet, 2);
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentSizeOfSet(&sizeOfSet));
            VERIFY_ARE_EQUAL(sizeOfSet, 3);

            uiaInfo.m_Name = L"Less app bar";
            uiaInfo.m_AutomationID = L"MoreButton";
            uiaInfo.m_cType = UIA_ButtonControlTypeId;

            automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
            VERIFY_IS_NOT_NULL(uiAutomationElement);
            VERIFY_SUCCEEDED(uiAutomationElement.As(&uiAutomationElement4));

            LOG_OUTPUT(L"The MoreButton next to PrimaryCommands should be the third element in a set of three.");
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentPositionInSet(&positionInSet));
            VERIFY_ARE_EQUAL(positionInSet, 3);
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentSizeOfSet(&sizeOfSet));
            VERIFY_ARE_EQUAL(sizeOfSet, 3);

            uiaInfo.m_Name = L"SecondaryAppBarButton";
            uiaInfo.m_AutomationID = L"SecondaryAppBarButton";

            automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, page);
            automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
            VERIFY_IS_NOT_NULL(uiAutomationElement);
            VERIFY_SUCCEEDED(uiAutomationElement.As(&uiAutomationElement4));

            LOG_OUTPUT(L"The AppBarButton in SecondaryCommands should be the first element in a set of two.");
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentPositionInSet(&positionInSet));
            VERIFY_ARE_EQUAL(positionInSet, 1);
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentSizeOfSet(&sizeOfSet));
            VERIFY_ARE_EQUAL(sizeOfSet, 2);

            uiaInfo.m_Name = L"SecondaryAppBarToggleButton";
            uiaInfo.m_AutomationID = L"SecondaryAppBarToggleButton";

            automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, page);
            automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
            VERIFY_IS_NOT_NULL(uiAutomationElement);
            VERIFY_SUCCEEDED(uiAutomationElement.As(&uiAutomationElement4));

            LOG_OUTPUT(L"The AppBarToggleButton in SecondaryCommands should be the second element in a set of two.");
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentPositionInSet(&positionInSet));
            VERIFY_ARE_EQUAL(positionInSet, 2);
            VERIFY_SUCCEEDED(uiAutomationElement4->get_CurrentSizeOfSet(&sizeOfSet));
            VERIFY_ARE_EQUAL(sizeOfSet, 2);
        });

        RunOnUIThread([&]()
        {
            page->BottomAppBar = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarAutomationIntegrationTests::VerifyNonTabStopAppBarButtonsAreStillKeyboardFocusable()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Page^ page = nullptr;
        xaml_controls::CommandBar^ commandBar = nullptr;

        RunOnUIThread([&]()
        {
            commandBar = ref new xaml_controls::CommandBar();

            auto primaryAppBarButton = ref new xaml_controls::AppBarButton();
            primaryAppBarButton->IsTabStop = false;
            xaml_automation::AutomationProperties::SetName(primaryAppBarButton, ref new Platform::String(L"PrimaryAppBarButton"));
            xaml_automation::AutomationProperties::SetAutomationId(primaryAppBarButton, ref new Platform::String(L"PrimaryAppBarButton"));
            commandBar->PrimaryCommands->Append(primaryAppBarButton);

            auto primaryAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
            primaryAppBarToggleButton->IsTabStop = false;
            xaml_automation::AutomationProperties::SetName(primaryAppBarToggleButton, ref new Platform::String(L"PrimaryAppBarToggleButton"));
            xaml_automation::AutomationProperties::SetAutomationId(primaryAppBarToggleButton, ref new Platform::String(L"PrimaryAppBarToggleButton"));
            commandBar->PrimaryCommands->Append(primaryAppBarToggleButton);

            auto secondaryAppBarButton = ref new xaml_controls::AppBarButton();
            secondaryAppBarButton->IsTabStop = false;
            xaml_automation::AutomationProperties::SetName(secondaryAppBarButton, ref new Platform::String(L"SecondaryAppBarButton"));
            xaml_automation::AutomationProperties::SetAutomationId(secondaryAppBarButton, ref new Platform::String(L"SecondaryAppBarButton"));
            commandBar->SecondaryCommands->Append(secondaryAppBarButton);

            auto secondaryAppBarToggleButton = ref new xaml_controls::AppBarToggleButton();
            secondaryAppBarToggleButton->IsTabStop = false;
            xaml_automation::AutomationProperties::SetName(secondaryAppBarToggleButton, ref new Platform::String(L"SecondaryAppBarToggleButton"));
            xaml_automation::AutomationProperties::SetAutomationId(secondaryAppBarToggleButton, ref new Platform::String(L"SecondaryAppBarToggleButton"));
            commandBar->SecondaryCommands->Append(secondaryAppBarToggleButton);

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = commandBar;
            commandBar->IsOpen = true;
        });
        
        TestServices::WindowHelper->WaitForIdle();

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(page);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"PrimaryAppBarButton";
            uiaInfo.m_AutomationID = L"PrimaryAppBarButton";
            
            BOOL isKeyboardFocusable = FALSE;
            wrl::ComPtr<IUIAutomationElement> uiAutomationElement;

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
            VERIFY_IS_NOT_NULL(uiAutomationElement);
            
            LOG_OUTPUT(L"The AppBarButton in PrimaryCommands should be keyboard focusable.");
            VERIFY_SUCCEEDED(uiAutomationElement->get_CurrentIsKeyboardFocusable(&isKeyboardFocusable));
            VERIFY_IS_TRUE(isKeyboardFocusable);

            uiaInfo.m_Name = L"PrimaryAppBarToggleButton";
            uiaInfo.m_AutomationID = L"PrimaryAppBarToggleButton";

            automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
            VERIFY_IS_NOT_NULL(uiAutomationElement);
            
            LOG_OUTPUT(L"The AppBarToggleButton in PrimaryCommands should be keyboard focusable.");
            VERIFY_SUCCEEDED(uiAutomationElement->get_CurrentIsKeyboardFocusable(&isKeyboardFocusable));
            VERIFY_IS_TRUE(isKeyboardFocusable);
        
            uiaInfo.m_Name = L"SecondaryAppBarButton";
            uiaInfo.m_AutomationID = L"SecondaryAppBarButton";

            automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, page);
            automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
            VERIFY_IS_NOT_NULL(uiAutomationElement);
            
            LOG_OUTPUT(L"The AppBarButton in SecondaryCommands should be keyboard focusable.");
            VERIFY_SUCCEEDED(uiAutomationElement->get_CurrentIsKeyboardFocusable(&isKeyboardFocusable));
            VERIFY_IS_TRUE(isKeyboardFocusable);

            uiaInfo.m_Name = L"SecondaryAppBarToggleButton";
            uiaInfo.m_AutomationID = L"SecondaryAppBarToggleButton";

            automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, page);
            automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
            VERIFY_IS_NOT_NULL(uiAutomationElement);
            
            LOG_OUTPUT(L"The AppBarToggleButton in SecondaryCommands should be keyboard focusable.");
            VERIFY_SUCCEEDED(uiAutomationElement->get_CurrentIsKeyboardFocusable(&isKeyboardFocusable));
            VERIFY_IS_TRUE(isKeyboardFocusable);
        });
        
        RunOnUIThread([&]()
        {
            page->BottomAppBar = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::CommandBar
