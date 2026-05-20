// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <AutomationClient\AutomationClientManager.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Generic {

    template <typename TClassUnderTest>
    class ButtonBaseTests
    {
    public:
        static void CanClickUsingTap()
        {
            TestCleanupWrapper cleanup;
            TClassUnderTest^ button = nullptr;
            xaml_controls::Grid^ rootGrid = nullptr;

            std::shared_ptr<Event> spClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(TClassUnderTest, Click);
            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

            RunOnUIThread([&]()
            {
                rootGrid = ref new xaml_controls::Grid();
                button = ref new TClassUnderTest();
                button->HorizontalAlignment = xaml::HorizontalAlignment::Center;
                button->VerticalAlignment = xaml::VerticalAlignment::Center;

                clickRegistration.Attach(button, [&]() { spClickEvent->Set(); });

                rootGrid->Children->Append(button);

                loadedRegistration.Attach(rootGrid, [loadedEvent]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootGrid;
            });

            LOG_OUTPUT(L"Waiting for rootGrid to be loaded...");
            loadedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(button);
            spClickEvent->WaitForDefault();
        }

        static void CanClickUsingUIA()
        {
            TestCleanupWrapper cleanup;
            TClassUnderTest^ button = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;

            auto spClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(TClassUnderTest, Click);
            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

            RunOnUIThread([&]()
            {
                auto rootPanel = ref new xaml_controls::Grid();

                button = ref new TClassUnderTest();
                uiaInfo.m_Name = L"TestButton";
                uiaInfo.m_AutomationID = L"TestButton";
                uiaInfo.m_ItemStatus = L"TestButton";
                uiaInfo.m_cType = UIA_ButtonControlTypeId;
                xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));

                clickRegistration.Attach(button, [&]() { spClickEvent->Set(); });

                button->Content = "Test Button";

                rootPanel->Children->Append(button);

                loadedRegistration.Attach(rootPanel, [loadedEvent]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            LOG_OUTPUT(L"Waiting for rootGrid to be loaded...");
            loadedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            wrl::ComPtr<Automation::Patterns::InvokePatternHandler> spAutomationInvokePatternHandler;
            auto spAutomationClientManager = Automation::AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationInvokePatternHandler.Attach(new Automation::Patterns::InvokePatternHandler(spAutomationClientManager));
            spAutomationInvokePatternHandler->Invoke();

            TestServices::WindowHelper->WaitForIdle();
            spClickEvent->WaitForDefault();
        }

        static void CanFireRightTappedEventUsingHold()
        {
            TestCleanupWrapper cleanup;
            TClassUnderTest^ button = nullptr;
            xaml_controls::Grid^ rootGrid = nullptr;

            std::shared_ptr<Event> spRightTappedEvent = std::make_shared<Event>();
            auto rightTappedEventRegistration = CreateSafeEventRegistration(TClassUnderTest, RightTapped);
            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

            RunOnUIThread([&]()
            {
                rootGrid = ref new xaml_controls::Grid();
                button = ref new TClassUnderTest();
                button->HorizontalAlignment = xaml::HorizontalAlignment::Center;
                button->VerticalAlignment = xaml::VerticalAlignment::Center;

                rightTappedEventRegistration.Attach(button, [&]() { spRightTappedEvent->Set(); });

                rootGrid->Children->Append(button);

                loadedRegistration.Attach(rootGrid, [loadedEvent]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootGrid;
            });

            LOG_OUTPUT(L"Waiting for rootGrid to be loaded...");
            loadedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Hold(button);
            spRightTappedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

    }; // class ButtonBaseTests

} } } } } // namespace Microsoft::UI::Xaml::Tests::Generic
