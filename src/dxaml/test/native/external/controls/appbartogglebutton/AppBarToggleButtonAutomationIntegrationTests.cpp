// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AppBarToggleButtonAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <generic\ButtonBaseTests.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarToggleButton {

    ref class CommandTestHelper : public Microsoft::UI::Xaml::Input::ICommand {
    public:
        virtual event wf::EventHandler<Object^>^ CanExecuteChanged;
        virtual event wf::EventHandler<Object^>^ Executed;

        virtual bool CanExecute(Platform::Object^ parameter)
        {
            return true;
        }

        virtual void Execute(Platform::Object^ parameter)
        {
            Executed(this, nullptr);
        }
    };

    bool AppBarToggleButtonAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool AppBarToggleButtonAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AppBarToggleButtonAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void AppBarToggleButtonAutomationIntegrationTests::VerifyAutomationProperties()
    {
        TestCleanupWrapper cleanup;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestToggleButton";
        uiaInfo.m_AutomationID = L"TestToggleButton";
        uiaInfo.m_ItemStatus = L"TestToggleButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            auto cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->VerticalAlignment = xaml::VerticalAlignment::Top;
            cmdBar->IsOpen = true;

            auto appBarToggleButtonWithName = ref new xaml_controls::AppBarToggleButton();
            xaml_automation::AutomationProperties::SetName(appBarToggleButtonWithName, ref new Platform::String(uiaInfo.m_Name));
            cmdBar->PrimaryCommands->Append(appBarToggleButtonWithName);

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(cmdBar);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spAppBarToggleButtonWithName;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            LOG_OUTPUT(L"Verifying UIA Client side node for AppBarToggleButton exists.");
            spAutomationClientManager->GetCurrentUIAutomationElement(&spAppBarToggleButtonWithName);
            VERIFY_IS_NOT_NULL(spAppBarToggleButtonWithName);

            LOG_OUTPUT(L"Verifying UIA Name property for AppBarToggleButtonWithName.");
            Common::AutoVariant autoVar;
            spAppBarToggleButtonWithName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Verifying UIA LocalizedControlType property for AppBarToggleButton.");
            Common::AutoBSTR autoBstr;
            spAppBarToggleButtonWithName->get_CurrentLocalizedControlType(autoBstr.ReleaseAndGetAddressOf());
            AutoBSTR::VerifyAreEqual(L"app bar toggle button", autoBstr);
        });
    }

    void AppBarToggleButtonAutomationIntegrationTests::VerifyHasNoUIAControlChildren()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestToggleButton";
        uiaInfo.m_AutomationID = L"TestToggleButton";
        uiaInfo.m_ItemStatus = L"TestToggleButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->VerticalAlignment = xaml::VerticalAlignment::Top;
            cmdBar->IsOpen = true;

            auto appBarToggleButton = ref new xaml_controls::AppBarToggleButton();
            xaml_automation::AutomationProperties::SetName(appBarToggleButton, ref new Platform::String(uiaInfo.m_Name));
            appBarToggleButton->Label = "TestButton";
            cmdBar->PrimaryCommands->Append(appBarToggleButton);

            rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(cmdBar);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spAppBarToggleButton;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            wrl::ComPtr<IUIAutomationElementArray> spItems;
            int itemCount;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spAppBarToggleButton);

            spItems = UIAutomationHelper::GetChildren(spUIAutomation.Get(), spAppBarToggleButton.Get());
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

    void AppBarToggleButtonAutomationIntegrationTests::VerifyToggleExecutesCommand()
    {
        TestCleanupWrapper cleanup;

        CommandTestHelper^ command = ref new CommandTestHelper();

        auto commandExecutedEvent = std::make_shared<Event>();
        auto commandExecutedRegistration = CreateSafeEventRegistration(CommandTestHelper, Executed);

        auto toggledEvent = std::make_shared<Event>();
        auto toggledRegistration = CreateSafeEventRegistration(xaml_controls::AppBarToggleButton, Checked);

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestToggleButton";
        uiaInfo.m_AutomationID = L"TestToggleButton";
        uiaInfo.m_ItemStatus = L"TestToggleButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        xaml_controls::AppBarToggleButton^ appBarToggleButtonWithName;

        RunOnUIThread([&]()
        {
            auto cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->VerticalAlignment = xaml::VerticalAlignment::Top;
            cmdBar->IsOpen = true;

            appBarToggleButtonWithName = ref new xaml_controls::AppBarToggleButton();
            xaml_automation::AutomationProperties::SetName(appBarToggleButtonWithName, ref new Platform::String(uiaInfo.m_Name));
            appBarToggleButtonWithName->Command = command;
            cmdBar->PrimaryCommands->Append(appBarToggleButtonWithName);

            commandExecutedRegistration.Attach(command, [commandExecutedEvent]() {commandExecutedEvent->Set(); });
            toggledRegistration.Attach(appBarToggleButtonWithName, [toggledEvent]() {toggledEvent->Set(); });

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(cmdBar);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        xaml_automation_peers::ToggleButtonAutomationPeer^ toggleButtonPeer;

        RunOnUIThread([&]()
        {
            toggleButtonPeer = safe_cast<xaml_automation_peers::ToggleButtonAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(appBarToggleButtonWithName));
            VERIFY_IS_NOT_NULL(toggleButtonPeer);
            VERIFY_ARE_EQUAL(Microsoft::UI::Xaml::Automation::ToggleState::Off, toggleButtonPeer->ToggleState);
            toggleButtonPeer->Toggle();

            commandExecutedEvent->WaitForDefault();
            VERIFY_IS_TRUE(commandExecutedEvent->HasFired());
            VERIFY_IS_TRUE(toggledEvent->HasFired());
            VERIFY_ARE_EQUAL(Microsoft::UI::Xaml::Automation::ToggleState::On, toggleButtonPeer->ToggleState);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AppBarToggleButton
