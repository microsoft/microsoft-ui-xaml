// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ToggleSwitchAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ToggleSwitch {

    bool ToggleSwitchAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ToggleSwitchAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ToggleSwitchAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    void ToggleSwitchAutomationIntegrationTests::VerifyAutomationName()
    {
        TestCleanupWrapper cleanup;

        // The Name returned by ToggleSwitchAutomationPeer should be:
        // 1. If a UIA name has been explicitly set, use that. Otherwise:
        // 2. Use the Header text followed by the On/OffContent.

        // Notes:
        // i. We use OnContent when IsOn=true, OffContent when IsOn=false
        // ii. The default values of OnContent and OffContent are "On" and "Off" respecitvely. But we only use these properties
        //     if custom content is set. This is because ToggleState already includes this information (We don't want Narrator
        //     to read "On On ToggleSwitch").
        // iii. Any of Header, OnContent or OffContent can be set to a non-text value. In such a case, the value is ignored.

        // Test Cases:
        // 1. UIA Name set (with Header and Content)
        // 2. Header set and On/OffContent set.
        // 3. Header set.
        // 4. On/OffContent set
        // 5. No Header, No On/Offcontent
        // 6. On/OffContent set via Style
        // 7. Non-text Header, On/OffContent
        // 8. Header, non-text On/OffContent

        xaml_controls::ToggleSwitch^ toggleSwitchWithAPName;
        xaml_controls::ToggleSwitch^ toggleSwitchWithHeaderAndOnOffContent;
        xaml_controls::ToggleSwitch^ toggleSwitchWithHeader;
        xaml_controls::ToggleSwitch^ toggleSwitchWithOnOffContent;
        xaml_controls::ToggleSwitch^ toggleSwitchDefault;
        xaml_controls::ToggleSwitch^ toggleSwitchStyled;
        xaml_controls::ToggleSwitch^ toggleSwitchNonTextHeader;
        xaml_controls::ToggleSwitch^ toggleSwitchNonTextOnOffContent;

        xaml_automation_peers::AutomationPeer^ toggleSwitchWithAPNameAP;
        xaml_automation_peers::AutomationPeer^ toggleSwitchWithHeaderAndOnOffContentAP;
        xaml_automation_peers::AutomationPeer^ toggleSwitchWithHeaderAP;
        xaml_automation_peers::AutomationPeer^ toggleSwitchWithOnOffContentAP;
        xaml_automation_peers::AutomationPeer^ toggleSwitchDefaultAP;
        xaml_automation_peers::AutomationPeer^ toggleSwitchStyledAP;
        xaml_automation_peers::AutomationPeer^ toggleSwitchNonTextHeaderAP;
        xaml_automation_peers::AutomationPeer^ toggleSwitchNonTextOnOffContentAP;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <StackPanel.Resources>
                            <Style x:Key="toggleSwitchStyle" TargetType="ToggleSwitch">
                                <Setter Property="OnContent" Value="Yes" />
                                <Setter Property="OffContent" Value="No" />
                            </Style>
                        </StackPanel.Resources>
                        <ToggleSwitch x:Name="toggleSwitchWithAPName" AutomationProperties.Name="APName" Header="Header" OnContent="Yes" OffContent="No" />
                        <ToggleSwitch x:Name="toggleSwitchWithHeaderAndOnOffContent" Header="Header" OnContent="Yes" OffContent="No" />
                        <ToggleSwitch x:Name="toggleSwitchWithHeader" Header="Header" />
                        <ToggleSwitch x:Name="toggleSwitchWithOnOffContent" OnContent="Yes" OffContent="No" />
                        <ToggleSwitch x:Name="toggleSwitchDefault" />
                        <ToggleSwitch x:Name="toggleSwitchStyled" Header="Header" Style="{StaticResource toggleSwitchStyle}"  />
                        <ToggleSwitch x:Name="toggleSwitchNonTextHeader" OnContent="Yes" OffContent="No" >
                            <ToggleSwitch.Header>
                                <Rectangle Fill="Blue" Width="10" Height="10" />
                            </ToggleSwitch.Header>
                        </ToggleSwitch>
                        <ToggleSwitch x:Name="toggleSwitchNonTextOnOffContent" Header="Header">
                            <ToggleSwitch.OnContent>
                                <Rectangle Fill="Green" Width="10" Height="10" />
                            </ToggleSwitch.OnContent>
                            <ToggleSwitch.OffContent>
                                <Rectangle Fill="Red" Width="10" Height="10" />
                            </ToggleSwitch.OffContent>
                        </ToggleSwitch>
                    </StackPanel>)"));

            toggleSwitchWithAPName = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchWithAPName"));
            toggleSwitchWithHeaderAndOnOffContent = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchWithHeaderAndOnOffContent"));
            toggleSwitchWithHeader = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchWithHeader"));
            toggleSwitchWithOnOffContent = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchWithOnOffContent"));
            toggleSwitchDefault = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchDefault"));
            toggleSwitchStyled = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchStyled"));
            toggleSwitchNonTextHeader = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchNonTextHeader"));
            toggleSwitchNonTextOnOffContent = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchNonTextOnOffContent"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            toggleSwitchWithAPNameAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(toggleSwitchWithAPName);
            toggleSwitchWithHeaderAndOnOffContentAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(toggleSwitchWithHeaderAndOnOffContent);
            toggleSwitchWithHeaderAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(toggleSwitchWithHeader);
            toggleSwitchWithOnOffContentAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(toggleSwitchWithOnOffContent);
            toggleSwitchDefaultAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(toggleSwitchDefault);
            toggleSwitchStyledAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(toggleSwitchStyled);
            toggleSwitchNonTextHeaderAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(toggleSwitchNonTextHeader);
            toggleSwitchNonTextOnOffContentAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(toggleSwitchNonTextOnOffContent);

            VERIFY_ARE_EQUAL(ref new Platform::String(L"APName"), toggleSwitchWithAPNameAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Header No"), toggleSwitchWithHeaderAndOnOffContentAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Header"), toggleSwitchWithHeaderAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"No"), toggleSwitchWithOnOffContentAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L""), toggleSwitchDefaultAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Header No"), toggleSwitchStyledAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"No"), toggleSwitchNonTextHeaderAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Header"), toggleSwitchNonTextOnOffContentAP->GetName());

            toggleSwitchWithAPName->IsOn = true;
            toggleSwitchWithHeaderAndOnOffContent->IsOn = true;
            toggleSwitchWithHeader->IsOn = true;
            toggleSwitchWithOnOffContent->IsOn = true;
            toggleSwitchDefault->IsOn = true;
            toggleSwitchStyled->IsOn = true;
            toggleSwitchNonTextHeader->IsOn = true;
            toggleSwitchNonTextOnOffContent->IsOn = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(ref new Platform::String(L"APName"), toggleSwitchWithAPNameAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Header Yes"), toggleSwitchWithHeaderAndOnOffContentAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Header"), toggleSwitchWithHeaderAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Yes"), toggleSwitchWithOnOffContentAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L""), toggleSwitchDefaultAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Header Yes"), toggleSwitchStyledAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Yes"), toggleSwitchNonTextHeaderAP->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Header"), toggleSwitchNonTextOnOffContentAP->GetName());
        });
    }

    void ToggleSwitchAutomationIntegrationTests::VerifyGetClickablePoint()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ToggleSwitch^ toggleSwitch;

        RunOnUIThread([&]()
        {
            toggleSwitch = ref new xaml_controls::ToggleSwitch();
            toggleSwitch->Header = L"Multi\nline\nheader";
            TestServices::WindowHelper->WindowContent = toggleSwitch;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(toggleSwitch);
            auto clickablePoint = peer->GetClickablePoint();

            // Make sure the clickable point is on top of the Thumb area.
            // The first intersecting element is going to be the rectangle, then the thumb.
            // That's why we need to call MoveNext once below.
            auto intersectingElements = xaml_media::VisualTreeHelper::FindElementsInHostCoordinates(clickablePoint, toggleSwitch);
            auto iterator = intersectingElements->First();
            iterator->MoveNext();
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml_primitives::Thumb^>(iterator->Current));
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ToggleSwitch
