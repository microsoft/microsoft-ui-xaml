// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include "CustomAutomationPeer.h"
#include <ProviderSideIntegrationTests.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Tests::Native::External::Automation::Provider;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Provider {

    bool ProviderSideIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        return true;
    }
     
    bool ProviderSideIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ProviderSideIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ProviderSideIntegrationTests::VerifyLocalizedControlType()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            CustomControl^ control = ref new CustomControl();
            CustomAutomationPeer^ customAP = static_cast<CustomAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(control));

            const ControlTypeData controlTypes[] =
            {
                // String               Expected role
                { xaml_automation_peers::AutomationControlType::Button, L"button" },
                { xaml_automation_peers::AutomationControlType::Calendar, L"calendar" },
                { xaml_automation_peers::AutomationControlType::CheckBox, L"check box" },
                { xaml_automation_peers::AutomationControlType::ComboBox, L"combo box" },
                { xaml_automation_peers::AutomationControlType::Edit, L"edit" },
                { xaml_automation_peers::AutomationControlType::Hyperlink, L"link" },
                { xaml_automation_peers::AutomationControlType::Image, L"image" },
                { xaml_automation_peers::AutomationControlType::ListItem, L"list item" },
                { xaml_automation_peers::AutomationControlType::List, L"list" },
                { xaml_automation_peers::AutomationControlType::Menu, L"menu" },
                { xaml_automation_peers::AutomationControlType::MenuBar, L"menu bar" },
                { xaml_automation_peers::AutomationControlType::MenuItem, L"menu item" },
                { xaml_automation_peers::AutomationControlType::ProgressBar, L"progress bar" },
                { xaml_automation_peers::AutomationControlType::RadioButton, L"radio button" },
                { xaml_automation_peers::AutomationControlType::ScrollBar, L"scroll bar" },
                { xaml_automation_peers::AutomationControlType::Slider, L"slider" },
                { xaml_automation_peers::AutomationControlType::Spinner, L"spinner" },
                { xaml_automation_peers::AutomationControlType::StatusBar, L"status bar" },
                { xaml_automation_peers::AutomationControlType::Tab, L"tab" },
                { xaml_automation_peers::AutomationControlType::TabItem, L"tab item" },
                { xaml_automation_peers::AutomationControlType::Text, L"text" },
                { xaml_automation_peers::AutomationControlType::ToolBar, L"tool bar" },
                { xaml_automation_peers::AutomationControlType::ToolTip, L"tool tip" },
                { xaml_automation_peers::AutomationControlType::Tree, L"tree" },
                { xaml_automation_peers::AutomationControlType::TreeItem, L"tree item" },
                { xaml_automation_peers::AutomationControlType::Custom, L"custom" },
                { xaml_automation_peers::AutomationControlType::Group, L"group" },
                { xaml_automation_peers::AutomationControlType::Thumb, L"thumb" },
                { xaml_automation_peers::AutomationControlType::DataGrid, L"data grid" },
                { xaml_automation_peers::AutomationControlType::DataItem, L"data item" },
                { xaml_automation_peers::AutomationControlType::Document, L"document" },
                { xaml_automation_peers::AutomationControlType::SplitButton, L"split button" },
                { xaml_automation_peers::AutomationControlType::Window, L"window" },
                { xaml_automation_peers::AutomationControlType::Pane, L"pane" },
                { xaml_automation_peers::AutomationControlType::Header, L"header" },
                { xaml_automation_peers::AutomationControlType::HeaderItem, L"header item" },
                { xaml_automation_peers::AutomationControlType::Table, L"table" },
                { xaml_automation_peers::AutomationControlType::TitleBar, L"title bar" },
                { xaml_automation_peers::AutomationControlType::Separator, L"separator" },
                { xaml_automation_peers::AutomationControlType::SemanticZoom, L"semantic zoom" },
                { xaml_automation_peers::AutomationControlType::AppBar, L"app bar" },
            };

            int controlTypesSize = sizeof(controlTypes) / sizeof(ControlTypeData);
            for (int i = 0; i < controlTypesSize; i++)
            {
                customAP->SetControlType(controlTypes[i].controlType);
                Platform::String^ localizedControlType = customAP->GetLocalizedControlType();
                VERIFY_IS_TRUE(!Platform::String::CompareOrdinal(localizedControlType, controlTypes[i].localizedControlType));
            }
        });
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Automation::Provider
