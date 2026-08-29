// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SplitViewAutomationIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <AutomationClient\AutomationClientManager.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include <TraceConsumerSession.h>


using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace SplitView {

    bool SplitViewAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        // Disable control state transitions to reduce test execution time.
        featureDisableTransitionsForTest.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTransitionsForTest, true);
        return true;
    }

    bool SplitViewAutomationIntegrationTests::ClassCleanup()
    {
        return true;
    }

    bool SplitViewAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool SplitViewAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void SplitViewAutomationIntegrationTests::ValidateLightDismissWindowPattern()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay");
        ValidateLightDismissWindowPatternWorker(xaml_controls::SplitViewDisplayMode::Overlay, true);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay");
        ValidateLightDismissWindowPatternWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, true);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline");
        ValidateLightDismissWindowPatternWorker(xaml_controls::SplitViewDisplayMode::Inline, false);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline");
        ValidateLightDismissWindowPatternWorker(xaml_controls::SplitViewDisplayMode::CompactInline, false);
    }

    void SplitViewAutomationIntegrationTests::ValidateLightDismissCloseButton()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Testing: DisplayMode=Overlay");
        ValidateLightDismissCloseButtonWorker(xaml_controls::SplitViewDisplayMode::Overlay, true);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactOverlay");
        ValidateLightDismissCloseButtonWorker(xaml_controls::SplitViewDisplayMode::CompactOverlay, true);

        LOG_OUTPUT(L"Testing: DisplayMode=Inline");
        ValidateLightDismissCloseButtonWorker(xaml_controls::SplitViewDisplayMode::Inline, false);

        LOG_OUTPUT(L"Testing: DisplayMode=CompactInline");
        ValidateLightDismissCloseButtonWorker(xaml_controls::SplitViewDisplayMode::CompactInline, false);
    }

    void SplitViewAutomationIntegrationTests::ValidateLightDismissWindowPatternWorker(xaml_controls::SplitViewDisplayMode displayMode, bool shouldLightDismiss)
    {
        xaml_controls::SplitView^ splitView = nullptr;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <SplitView x:Name='splitView' Width='400' Height='400' OpenPaneLength='100'>"
                L"        <SplitView.Pane>"
                L"            <Border Background='Orange'>"
                L"                <Button/>"
                L"            </Border>"
                L"        </SplitView.Pane>"
                L"        <Rectangle x:Name='contentElement' Fill='Purple'/>"
                L"    </SplitView>"
                L"    <Border Width='400' Height='50' Background='GreenYellow'>"
                L"        <Button x:Name='outsideElement'/>"
                L"    </Border>"
                L"</StackPanel>"
                ));

            splitView = safe_cast<xaml_controls::SplitView^>(root->FindName(L"splitView"));

            splitView->DisplayMode = displayMode;
            splitView->PanePlacement = xaml_controls::SplitViewPanePlacement::Left;

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Test tapping outside of pane area, but within the splitview control.
        RunOnUIThread([&]() { splitView->IsPaneOpen = true; });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_AutomationID = L"PaneRoot";
            uiaInfo.m_cType = UIA_WindowControlTypeId;

            wrl::ComPtr<IUIAutomationWindowPattern> spWindowPattern;
            wrl::ComPtr<IUIAutomationElement> spAutomationPaneElement;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoId(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spAutomationPaneElement);
            LOG_OUTPUT(L"Verifying UIA Client side node for Pane element");
            VERIFY_IS_NOT_NULL(spAutomationPaneElement);    // WPF_HOSTING_MODE_FAILURE: fails here.

            VERIFY_SUCCEEDED(spAutomationPaneElement->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spWindowPattern));
            if (shouldLightDismiss)
            {
                VERIFY_IS_NOT_NULL(spWindowPattern);
            }
            else
            {
                VERIFY_IS_NULL(spWindowPattern);
            }
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_AutomationID = L"ContentRoot";
            uiaInfo.m_cType = UIA_WindowControlTypeId;

            wrl::ComPtr<IUIAutomationWindowPattern> spWindowPattern;
            wrl::ComPtr<IUIAutomationElement> spAutomationPaneElement;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoId(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spAutomationPaneElement);
            LOG_OUTPUT(L"Verifying No UIA node for content root");
            VERIFY_IS_NULL(spAutomationPaneElement);
        });
    }

    void SplitViewAutomationIntegrationTests::ValidateLightDismissCloseButtonWorker(xaml_controls::SplitViewDisplayMode displayMode, bool shouldLightDismiss)
    {

        xaml_controls::SplitView^ splitView = nullptr;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <SplitView x:Name='splitView' Width='400' Height='400' OpenPaneLength='100'>"
                L"        <SplitView.Pane>"
                L"            <Border Background='Orange'>"
                L"                <Button/>"
                L"            </Border>"
                L"        </SplitView.Pane>"
                L"        <Rectangle x:Name='contentElement' Fill='Purple'/>"
                L"    </SplitView>"
                L"    <Border Width='400' Height='50' Background='GreenYellow'>"
                L"        <Button x:Name='outsideElement'/>"
                L"    </Border>"
                L"</StackPanel>"
                ));

            splitView = safe_cast<xaml_controls::SplitView^>(root->FindName(L"splitView"));

            splitView->DisplayMode = displayMode;
            splitView->PanePlacement = xaml_controls::SplitViewPanePlacement::Left;

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Test tapping outside of pane area, but within the splitview control.
        RunOnUIThread([&]() { splitView->IsPaneOpen = true; });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_AutomationID = L"LightDismiss";
            uiaInfo.m_cType = UIA_ButtonControlTypeId;

            wrl::ComPtr<IUIAutomationInvokePattern> spInvokePattern;
            wrl::ComPtr<IUIAutomationElement> spAutomationPaneElement;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoId(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spAutomationPaneElement);
            if (shouldLightDismiss)
            {
                LOG_OUTPUT(L"Verifying UIA Client side node for lightdismiss element");
                VERIFY_IS_NOT_NULL(spAutomationPaneElement);    // WPF_HOSTING_MODE_FAILURE: Fails here.
                VERIFY_SUCCEEDED(spAutomationPaneElement->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), &spInvokePattern));
                VERIFY_IS_NOT_NULL(spInvokePattern);

                VERIFY_SUCCEEDED(spInvokePattern->Invoke());
            }
            else
            {
                VERIFY_IS_NULL(spAutomationPaneElement);
            }
        });


        if (shouldLightDismiss)
        {
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(splitView->IsPaneOpen);
            });
        }
    }

} } } } } }
