// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AutoSuggestIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <AutomationClient\AutomationClientManager.h>
#include <TestCleanupWrapper.h>
#include <Collection.h>

#include <ItemsControlHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutoSuggest {

    bool AutoSuggestIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool AutoSuggestIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AutoSuggestIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    void AutoSuggestIntegrationTests::VerifyListItemsChangedEventForAutoSuggestBox()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Platform::String^>^ itemsList;
        xaml_controls::AutoSuggestBox^ autoSuggestBox;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        wrl::ComPtr<AutomationClient::AutomationEventHandler> spAutomationEventHandler;
        uiaInfo.m_Name = L"TestAutoSuggestBox";
        uiaInfo.m_AutomationID = L"TestAutoSuggestBox";
        uiaInfo.m_cType = UIA_ListControlTypeId;

        RunOnUIThread([&]()
        {
            itemsList = ref new Platform::Collections::Vector<Platform::String^>();
            itemsList->Append("Item 1");
            itemsList->Append("Item 2");
            itemsList->Append("Item 3");
            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();
            xaml_automation::AutomationProperties::SetName(autoSuggestBox, ref new Platform::String(uiaInfo.m_Name));
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            autoSuggestBox->Text = "Hello";
            TestServices::WindowHelper->WindowContent = autoSuggestBox;
            autoSuggestBox->Focus(xaml::FocusState::Programmatic);
            autoSuggestBox->ItemsSource = itemsList;

        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            auto layoutInvalidatedEvent = std::make_shared<Event>();
            spAutomationEventHandler.Attach(new AutomationClient::AutomationEventHandler(spAutomationClientManager, layoutInvalidatedEvent, TreeScope_Subtree, UIA_LayoutInvalidatedEventId));
            spAutomationEventHandler->AttachEventHandler();
        });

        RunOnUIThread([&]()
        {
            autoSuggestBox->Focus(xaml::FocusState::Programmatic);
            itemsList->RemoveAt(2);
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            spAutomationEventHandler->Confirm();
            WEX::Logging::Log::Comment(L"UIA_LayoutInvalidatedEventId Recieved after item removal");
        });

        RunOnUIThread([&]()
        {
            autoSuggestBox->Focus(xaml::FocusState::Programmatic);
            itemsList->Append("Item 3");
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            spAutomationEventHandler->Confirm();
            WEX::Logging::Log::Comment(L"UIA_LayoutInvalidatedEventId Recieved after item addition");
        });

        RunOnUIThread([&]()
        {
            Platform::Collections::Vector<Platform::String^>^ itemsList2;
            itemsList2 = ref new Platform::Collections::Vector<Platform::String^>();
            itemsList2->Append("Item 10");
            itemsList2->Append("Item 20");
            itemsList2->Append("Item 30");
            autoSuggestBox->Focus(xaml::FocusState::Programmatic);
            autoSuggestBox->ItemsSource = itemsList2;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            spAutomationEventHandler->ConfirmAndUnregister();
            WEX::Logging::Log::Comment(L"UIA_LayoutInvalidatedEventId Recieved after resetting list");
        });

    }


} } } } }}

