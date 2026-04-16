// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AppBarAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationClientInitializer.h>
#include "TestCleanupWrapper.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBar {

    WCHAR AppBarAutomationIntegrationTests::s_automationName[] = L"TestAppBarName";
    WCHAR AppBarAutomationIntegrationTests::s_automationId[] = L"TestAppBarId";

    bool AppBarAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        // Disable control state transitions to reduce test execution time.
        m_featureDisableTransitionsForTest.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTransitionsForTest, true);
        return true;
    }

    bool AppBarAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AppBarAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    //  Loads simple markup with AppBar and verifies AutomationProperties are correct.
    void AppBarAutomationIntegrationTests::VerifyAutomationProperties()
    {
        VerifyAutomationProperties(false /*useAppBarAutomationName*/);
        VerifyAutomationProperties(true /*useAppBarAutomationName*/);
    }

    void AppBarAutomationIntegrationTests::VerifyAutomationProperties(bool useAppBarAutomationName)
    {
        TestCleanupWrapper cleanup;
        xaml_controls::AppBar^ appBar = SetupAppBar(false, useAppBarAutomationName /*setAppBarAutomationName*/);

        // Find L"More options for TestAppBar" or "More app bar" button depending on whether the AppBar AutomationName is set or not.
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = useAppBarAutomationName ? L"More options for TestAppBar" : L"More options";
        uiaInfo.m_AutomationID = L"TestAppBar";
        uiaInfo.m_ItemStatus = L"TestAppBar";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LOG_OUTPUT(L"Verifying UIA Client side node for More app bar button in AppBar exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            appBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Find "Less app bar for TestAppBar" or "Less app bar" button depending on whether the AppBar automation name is set or not.
        uiaInfo.m_Name = useAppBarAutomationName ? L"Less app bar for TestAppBar" : L"Less app bar";

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LOG_OUTPUT(L"Verifying UIA Client side node for Less app bar button in AppBar exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AppBarAutomationIntegrationTests::VerifyAutomationWindowPattern()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = SetupAppBar(true);

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(appBar, ref new Platform::String(s_automationName));
            xaml_automation::AutomationProperties::SetAutomationId(appBar, ref new Platform::String(s_automationId));
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

            LOG_OUTPUT(L"Verifying that the AppBar supports the Window pattern when it is opened.");
            spUIAutomationCommandBar->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationCommandBarWindowPattern);

            VERIFY_IS_NOT_NULL(spUIAutomationCommandBarWindowPattern);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto appBarAP = safe_cast<Microsoft::UI::Xaml::Automation::Provider::IWindowProvider^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(appBar));

            // Verify IWindowProdiver properties
            VERIFY_IS_TRUE(appBarAP->IsModal);
            VERIFY_IS_TRUE(appBarAP->IsTopmost);
            VERIFY_IS_FALSE(appBarAP->Maximizable);
            VERIFY_IS_FALSE(appBarAP->Minimizable);
            VERIFY_ARE_EQUAL(appBarAP->InteractionState, xaml_automation::WindowInteractionState::Running);
            VERIFY_ARE_EQUAL(appBarAP->VisualState, xaml_automation::WindowVisualState::Normal);

            // Verify IWindowProvider methods that doesn't do anything
            appBarAP->Close();
            appBarAP->SetVisualState(xaml_automation::WindowVisualState::Normal);
            appBarAP->WaitForInputIdle(0 /* milliseconds */);

            appBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationCommandBar;
            wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationCommandBarWindowPattern;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationCommandBar);

            LOG_OUTPUT(L"Verifying that the appBar doesn't supports the Window pattern when it is closed.");
            spUIAutomationCommandBar->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationCommandBarWindowPattern);

            VERIFY_IS_NULL(spUIAutomationCommandBarWindowPattern);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AppBarAutomationIntegrationTests::VerifyNoLightDismissInTreeWhenCollapsed()
    {
        TestCleanupWrapper cleanup;

        SetupAppBar(false);

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"Close";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIACloseButton;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIACloseButton);
            VERIFY_IS_NULL(spUIACloseButton);   // WPF_HOSTING_MODE_FAILURE: This fails when wpf-hosted.
        });
    }

    void AppBarAutomationIntegrationTests::VerifyLightDismissInTreeWhenExpanded()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = SetupAppBar(true);

        auto spClosedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        RunOnUIThread([&]()
        {
            closedRegistration.Attach(appBar, ref new wf::EventHandler<Platform::Object^>([spClosedEvent](Platform::Object^ sender, Platform::Object^ e) {
                spClosedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        Automation::AutomationClient::UIAElementInfo uiaInfoCloseButton;
        uiaInfoCloseButton.m_Name = L"Close";
        Automation::AutomationClient::UIAElementInfo uiaInfoAppBar;
        uiaInfoAppBar.m_Name = L"TestAppBar";

        // Temporary fix for UiaEndpoint synchronization
        Automation::AutomationClient::AutomationClientInitializer::TEMP_WaitForDefault();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIACloseButton;
            wrl::ComPtr<IUIAutomationInvokePattern> spInvokePattern;

            auto spAutomationClientManagerClose = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoCloseButton);
            spAutomationClientManagerClose->GetCurrentUIAutomationElement(&spUIACloseButton);
            VERIFY_IS_NOT_NULL(spUIACloseButton);
            VERIFY_SUCCEEDED(spUIACloseButton->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&spInvokePattern)));
            VERIFY_IS_NOT_NULL(spInvokePattern);

            VERIFY_SUCCEEDED(spInvokePattern->Invoke());

            wrl::ComPtr<IUIAutomationElement> spUIAAppBar;
            wrl::ComPtr<IUIAutomationExpandCollapsePattern> spExpandCollapsePattern;
            ExpandCollapseState state;

            auto spAutomationClientManagerAppBar = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoAppBar);
            spAutomationClientManagerAppBar->GetCurrentUIAutomationElement(&spUIAAppBar);
            VERIFY_IS_NOT_NULL(spUIAAppBar);
            VERIFY_SUCCEEDED(spUIAAppBar->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, IID_PPV_ARGS(&spExpandCollapsePattern)));
            VERIFY_IS_NOT_NULL(spExpandCollapsePattern);
            VERIFY_SUCCEEDED(spExpandCollapsePattern->get_CurrentExpandCollapseState(&state));
            VERIFY_ARE_EQUAL(state, ExpandCollapseState_Collapsed);

        });

        spClosedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(appBar->IsOpen);
        });
    }


    xaml_controls::AppBar^ AppBarAutomationIntegrationTests::SetupAppBar(bool isOpen, bool setAppBarAutomationName)
    {
        xaml_controls::AppBar^ appBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        auto spHasLoadedEvent = std::make_shared<Event>();
        auto spHasUnloadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Loaded);
        auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Unloaded);

        // Setup our environment.
        RunOnUIThread([&]()
        {
            appBar = ref new xaml_controls::AppBar();

            if (setAppBarAutomationName)
            {
                xaml_automation::AutomationProperties::SetName(appBar, "TestAppBar");
            }

            page = TestServices::WindowHelper->SetupSimulatedAppPage();

            loadedRegistration.Attach(appBar, ref new xaml::RoutedEventHandler([spHasLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                spHasLoadedEvent->Set();
            }));

            unloadedRegistration.Attach(appBar, ref new xaml::RoutedEventHandler([spHasUnloadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                spHasUnloadedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Open AppBar and wait for Loaded
        RunOnUIThread([&]()
        {
            page->TopAppBar = appBar;
            appBar->IsOpen = isOpen;
        });
        spHasLoadedEvent->WaitForDefault();

        return appBar;
    }


} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AppBar
