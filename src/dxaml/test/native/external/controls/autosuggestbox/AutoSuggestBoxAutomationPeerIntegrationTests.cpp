// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AutoSuggestBoxAutomationPeerIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

#include <AutomationClient\AutomationGenericTests.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <AutomationClient\AutomationClientManager.h>
#include <Patterns\InvokePatternHandler.h>

#include <Collection.h>
#include <TreeHelper.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AutoSuggestBox {

    bool AutoSuggestBoxAutomationPeerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool AutoSuggestBoxAutomationPeerIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AutoSuggestBoxAutomationPeerIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void AutoSuggestBoxAutomationPeerIntegrationTests::VerifyControlledPeersPropertyChangedEvent()
    {
        TestCleanupWrapper cleanup;

        wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<1>> spAutomationPropertyChangedEventHandler;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestAutoSuggestBox";
        uiaInfo.m_AutomationID = L"TestAutoSuggestBox";
        uiaInfo.m_ItemStatus = L"TestAutoSuggestBox";
        uiaInfo.m_cType = UIA_GroupControlTypeId;

        auto autoSuggestBox = SetupTest(ref new Platform::String(uiaInfo.m_Name), nullptr /* AutomationName */, nullptr /* AutomationId */);

        RunOnUIThread([&]()
        {
            auto automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(autoSuggestBox);
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto controllerForUIAPropertyChangedEvent = std::make_shared<Event>();
            const std::array<PROPERTYID, 1> saPropertyIds = { UIA_ControllerForPropertyId };
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationPropertyChangedEventHandler.Attach(new AutomationClient::AutomationPropertyChangedHandler<1>(spAutomationClientManager, controllerForUIAPropertyChangedEvent, TreeScope_Subtree, saPropertyIds));
            spAutomationPropertyChangedEventHandler->AttachEventHandler();
        });

        RunOnUIThread([&]()
        {
            // As soon as handler gets called for any of the above three properties, success and bail.
            autoSuggestBox->IsSuggestionListOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            // As soon as handler gets called for any of the above three properties, success and bail.
            spAutomationPropertyChangedEventHandler->ConfirmAndUnregister();
        });
    }

    void AutoSuggestBoxAutomationPeerIntegrationTests::VerifyAutomationProperties()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBox^ textBox = nullptr;
        wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<1>> spAutomationPropertyChangedEventHandler;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestAutoSuggestBox";
        uiaInfo.m_AutomationID = L"TestAutoSuggestBox";
        uiaInfo.m_ItemStatus = L"TestAutoSuggestBox";
        uiaInfo.m_cType = UIA_GroupControlTypeId;

        auto autoSuggestBox = SetupTest(ref new Platform::String(uiaInfo.m_Name), ref new Platform::String(uiaInfo.m_Name), ref new Platform::String(uiaInfo.m_AutomationID));

        // Verify the automation properties are set for the auto suggest box
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spUITextBoxAutomationElement;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LOG_OUTPUT(L"Verifying UIA Client side node for AutoSuggestBox exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            Common::AutoVariant autoVar;

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for AutoSuggestBox.");
            spUIAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Verifying UIA AutomationId property from Client side node for AutoSuggestBox.");
            spUIAutomationElement->GetCurrentPropertyValue(UIA_AutomationIdPropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_AutomationID, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Verifying UIA ControlType property from Client side node for AutoSuggestBox.");
            spUIAutomationElement->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(uiaInfo.m_cType == ((autoVar.Storage())->lVal));

            // The text box should also have the same UIA Name as the AutoSuggestBox. The text box is the first child of the group.
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Navigate to the Text Box in AutoSuggestBox.");
            spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUITextBoxAutomationElement);
            VERIFY_IS_NOT_NULL(spUITextBoxAutomationElement);

            LOG_OUTPUT(L"Verifying UIA Name property from text box for AutoSuggestBox.");
            spUITextBoxAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));
        });

        TestServices::WindowHelper->WaitForIdle();
    }


    void AutoSuggestBoxAutomationPeerIntegrationTests::VerifyDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestAutoSuggestBox";
        uiaInfo.m_cType = UIA_GroupControlTypeId;

        auto autoSuggestBox = SetupTest(ref new Platform::String(uiaInfo.m_Name), nullptr /* AutomationName */, nullptr /* AutomationId */);

        // Verify the automation properties are set for the auto suggest box
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spUITextBoxAutomationElement;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomation> spUIAutomation;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            // The text box should also have the same UIA Name as the AutoSuggestBox. The text box is the first child of the group.
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for AutoSuggestBox exist.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            Common::AutoVariant autoVar;

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for TextBox above AutoSuggestBox.");
            spUIAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Verifying UIA ControlType property from Client side node for AutoSuggestBox.");
            spUIAutomationElement->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(uiaInfo.m_cType == ((autoVar.Storage())->lVal));

            LOG_OUTPUT(L"Navigate to the Text Box in AutoSuggestBox.");
            spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUITextBoxAutomationElement);
            VERIFY_IS_NOT_NULL(spUITextBoxAutomationElement);

            LOG_OUTPUT(L"Verifying UIA Name property from text box for AutoSuggestBox.");
            spUITextBoxAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));
        });

        TestServices::WindowHelper->WaitForIdle();
    }


    void AutoSuggestBoxAutomationPeerIntegrationTests::CanUseInvokePatternToSubmitQuery()
    {
        TestCleanupWrapper cleanup;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestAutoSuggestBox";
        uiaInfo.m_cType = UIA_GroupControlTypeId;

        auto autoSuggestBox = SetupTest(ref new Platform::String(uiaInfo.m_Name), nullptr /* AutomationName */, nullptr /* AutomationId */);

        Event querySubmittedEvent;
        auto querySubmittedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, QuerySubmitted);
        querySubmittedRegistration.Attach(autoSuggestBox, [&]() { querySubmittedEvent.Set(); });

        // Verify the automation properties are set for the auto suggest box
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> automationElement;
            automationClientManager->GetCurrentUIAutomationElement(&automationElement);

            wrl::ComPtr<IUIAutomationInvokePattern> invokePattern;
            LogThrow_IfFailed(automationElement->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), &invokePattern));
            LogThrow_IfFailed(invokePattern->Invoke());
        });

        LOG_OUTPUT(L"Wait for the QuerySubmitted event to fire after invoking it using the UIA's Invoke pattern.");
        querySubmittedEvent.WaitForDefault();
    }

    xaml_controls::AutoSuggestBox^ AutoSuggestBoxAutomationPeerIntegrationTests::SetupTest(Platform::String^ headerText, Platform::String^ automationName, Platform::String^ automationId)
    {
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;

        RunOnUIThread([&]()
        {
            auto itemList = ref new Platform::Collections::Vector<Platform::String^>();

            itemList->Append("Red");
            itemList->Append("Blue");
            itemList->Append("Yellow");

            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();
            autoSuggestBox->ItemsSource = itemList;
            autoSuggestBox->Header = headerText;

            if (automationName != nullptr)
            {
                xaml_automation::AutomationProperties::SetName(autoSuggestBox, automationName);
            }

            if (automationId)
            {
                xaml_automation::AutomationProperties::SetAutomationId(autoSuggestBox, automationId);
            }

            TestServices::WindowHelper->WindowContent = autoSuggestBox;
        });
        TestServices::WindowHelper->WaitForIdle();

        return autoSuggestBox;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AutoSuggestBox
