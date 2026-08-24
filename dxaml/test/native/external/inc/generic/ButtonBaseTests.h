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

        static void CanRecoverKeyboardInvocationAfterPointerCancellation()
        {
            TestCleanupWrapper cleanup;
            TClassUnderTest^ button = nullptr;
            xaml_controls::Border^ container = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            std::shared_ptr<Event> clickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(TClassUnderTest, Click);
            wf::Point pressPoint(40, 20);
            bool isTracking = false;

            RunOnUIThread([&]()
            {
                rootPanel = ref new xaml_controls::StackPanel();
                container = ref new xaml_controls::Border();
                button = ref new TClassUnderTest();
                button->Content = L"Button";
                button->Width = 200;
                button->Height = 40;
                container->Width = 260;
                container->Height = 80;

                clickRegistration.Attach(button, [&]() { clickEvent->Set(); });

                container->Child = button;
                rootPanel->Children->Append(container);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                button->AddHandler(xaml::UIElement::PointerPressedEvent,
                    ref new xaml_input::PointerEventHandler([&](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
                    {
                        isTracking = true;
                    }),
                    true);

                button->AddHandler(xaml::UIElement::PointerMovedEvent,
                    ref new xaml_input::PointerEventHandler([&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
                    {
                        if (!isTracking)
                        {
                            return;
                        }

                        auto point = args->GetCurrentPoint(rootPanel)->Position;
                        auto dx = point.X - pressPoint.X;
                        auto dy = point.Y - pressPoint.Y;
                        if ((dx * dx) + (dy * dy) > 20.0 * 20.0)
                        {
                            isTracking = false;
                            container->Visibility = xaml::Visibility::Collapsed;
                        }
                    }),
                    true);
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->MouseButtonDown(button, 0, 0, MouseButton::Left);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->MoveMouse(rootPanel);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->MouseButtonUp(rootPanel, 0, 0, MouseButton::Left);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                container->Visibility = xaml::Visibility::Visible;
                button->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->Space();
            clickEvent->WaitForDefault();
        }

    }; // class ButtonBaseTests

} } } } } // namespace Microsoft::UI::Xaml::Tests::Generic
