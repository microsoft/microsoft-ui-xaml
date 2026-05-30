// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <NotificationEventTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>
#include <AutomationClient\AutomationEventHandler.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    bool NotificationEventTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool NotificationEventTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool NotificationEventTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void NotificationEventTests::NotificationEventWithDisplayString()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"Button";
        uiaInfoTarget.m_AutomationID = L"Button";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();
        WEX::Logging::Log::Comment(L"Added Button to Visual Tree");

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto notifyEvent = std::make_shared<Event>();
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<AutomationClient::AutomationNotificationHandler> notificationEventHandler;
            notificationEventHandler.Attach(new AutomationClient::AutomationNotificationHandler(automationClientManager, notifyEvent, TreeScope_Subtree));
            notificationEventHandler->Init(
                NotificationKind_ItemAdded,
                NotificationProcessing_ImportantAll,
                L"Added an item",
                L"ItemAddedActivityId");
            notificationEventHandler->AttachEventHandler();

            RunOnUIThread([&]()
            {
                xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button)->RaiseNotificationEvent(
                    xaml_automation_peers::AutomationNotificationKind::ItemAdded,
                    xaml_automation_peers::AutomationNotificationProcessing::ImportantAll,
                    L"Added an item",
                    L"ItemAddedActivityId");
            });

            notificationEventHandler->ConfirmAndUnregister();
        });
    }

    void NotificationEventTests::NotificationEventWithNullDisplayString()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"Button";
        uiaInfoTarget.m_AutomationID = L"Button";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();
        WEX::Logging::Log::Comment(L"Added Button to Visual Tree");

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto notifyEvent = std::make_shared<Event>();
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<AutomationClient::AutomationNotificationHandler> notificationEventHandler;
            notificationEventHandler.Attach(new AutomationClient::AutomationNotificationHandler(automationClientManager, notifyEvent, TreeScope_Subtree));
            notificationEventHandler->Init(
                NotificationKind_ItemAdded,
                NotificationProcessing_ImportantAll,
                nullptr,
                L"ItemAddedActivityId");
            notificationEventHandler->AttachEventHandler();

            RunOnUIThread([&]()
            {
                xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button)->RaiseNotificationEvent(
                    xaml_automation_peers::AutomationNotificationKind::ItemAdded,
                    xaml_automation_peers::AutomationNotificationProcessing::ImportantAll,
                    nullptr,
                    L"ItemAddedActivityId");
            });

            notificationEventHandler->ConfirmAndUnregister();
        });
    }

    void NotificationEventTests::NotificationEventWithEmptyDisplayString()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"Button";
        uiaInfoTarget.m_AutomationID = L"Button";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();
        WEX::Logging::Log::Comment(L"Added Button to Visual Tree");

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto notifyEvent = std::make_shared<Event>();
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<AutomationClient::AutomationNotificationHandler> notificationEventHandler;
            notificationEventHandler.Attach(new AutomationClient::AutomationNotificationHandler(automationClientManager, notifyEvent, TreeScope_Subtree));
            notificationEventHandler->Init(
                NotificationKind_ItemAdded,
                NotificationProcessing_ImportantAll,
                L"",
                L"ItemAddedActivityId");
            notificationEventHandler->AttachEventHandler();

            RunOnUIThread([&]()
            {
                xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button)->RaiseNotificationEvent(
                    xaml_automation_peers::AutomationNotificationKind::ItemAdded,
                    xaml_automation_peers::AutomationNotificationProcessing::ImportantAll,
                    L"",
                    L"ItemAddedActivityId");
            });

            notificationEventHandler->ConfirmAndUnregister();
        });
    }

    void NotificationEventTests::VerifyAllEnumValues()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"Button";
        uiaInfoTarget.m_AutomationID = L"Button";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();
        WEX::Logging::Log::Comment(L"Added Button to Visual Tree");

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto notifyEvent = std::make_shared<Event>();
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<AutomationClient::AutomationNotificationHandler> notificationEventHandler;
            notificationEventHandler.Attach(new AutomationClient::AutomationNotificationHandler(automationClientManager, notifyEvent, TreeScope_Subtree));

            // NotificationKind_ItemAdded/NotificationProcessing_ImportantAll
            WEX::Logging::Log::Comment(L"NotificationKind_ItemAdded/NotificationProcessing_ImportantAll");
            notificationEventHandler->Init(
                NotificationKind_ItemAdded,
                NotificationProcessing_ImportantAll,
                nullptr,
                L"ItemAddedActivityId");
            notificationEventHandler->AttachEventHandler();

            RunOnUIThread([&]()
            {
                xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button)->RaiseNotificationEvent(
                    xaml_automation_peers::AutomationNotificationKind::ItemAdded,
                    xaml_automation_peers::AutomationNotificationProcessing::ImportantAll,
                    nullptr,
                    L"ItemAddedActivityId");
            });
            notificationEventHandler->Confirm();

            // NotificationKind_ItemRemoved/NotificationProcessing_ImportantMostRecent
            WEX::Logging::Log::Comment(L"NotificationKind_ItemRemoved/NotificationProcessing_ImportantMostRecent");
            notificationEventHandler->Init(
                NotificationKind_ItemRemoved,
                NotificationProcessing_ImportantMostRecent,
                nullptr,
                L"ItemRemovedActivityId");

            RunOnUIThread([&]()
            {
                xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button)->RaiseNotificationEvent(
                    xaml_automation_peers::AutomationNotificationKind::ItemRemoved,
                    xaml_automation_peers::AutomationNotificationProcessing::ImportantMostRecent,
                    nullptr,
                    L"ItemRemovedActivityId");
            });
            notificationEventHandler->Confirm();

            // NotificationKind_ActionCompleted/NotificationProcessing_All
            WEX::Logging::Log::Comment(L"NotificationKind_ActionCompleted/NotificationProcessing_All");
            notificationEventHandler->Init(
                NotificationKind_ActionCompleted,
                NotificationProcessing_All,
                nullptr,
                L"ActionCompletedActivityId");

            RunOnUIThread([&]()
            {
                xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button)->RaiseNotificationEvent(
                    xaml_automation_peers::AutomationNotificationKind::ActionCompleted,
                    xaml_automation_peers::AutomationNotificationProcessing::All,
                    nullptr,
                    L"ActionCompletedActivityId");
            });
            notificationEventHandler->Confirm();

            // NotificationKind_ActionAborted/NotificationProcessing_MostRecent
            WEX::Logging::Log::Comment(L"NotificationKind_ActionAborted/NotificationProcessing_MostRecent");
            notificationEventHandler->Init(
                NotificationKind_ActionAborted,
                NotificationProcessing_MostRecent,
                nullptr,
                L"ActionAbortedActivityId");

            RunOnUIThread([&]()
            {
                xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button)->RaiseNotificationEvent(
                    xaml_automation_peers::AutomationNotificationKind::ActionAborted,
                    xaml_automation_peers::AutomationNotificationProcessing::MostRecent,
                    nullptr,
                    L"ActionAbortedActivityId");
            });
            notificationEventHandler->Confirm();

            // NotificationKind_Other/NotificationProcessing_CurrentThenMostRecent
            WEX::Logging::Log::Comment(L"NotificationKind_Other/NotificationProcessing_CurrentThenMostRecent");
            notificationEventHandler->Init(
                NotificationKind_Other,
                NotificationProcessing_CurrentThenMostRecent,
                nullptr,
                L"OtherActivityId");

            RunOnUIThread([&]()
            {
                xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button)->RaiseNotificationEvent(
                    xaml_automation_peers::AutomationNotificationKind::Other,
                    xaml_automation_peers::AutomationNotificationProcessing::CurrentThenMostRecent,
                    nullptr,
                    L"OtherActivityId");
            });
            notificationEventHandler->ConfirmAndUnregister();
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer