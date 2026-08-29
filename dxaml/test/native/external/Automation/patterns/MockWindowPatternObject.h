// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "pch.h"

#pragma warning(disable: 4100) // Unreferenced parameters are perfectly legal C++ and common for formal interfaces, not indicative a real error.

namespace Tests { namespace Native { namespace External { namespace Automation { namespace Patterns {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class MockWindowPatternAutomationPeer sealed : public xaml_automation::Peers::FrameworkElementAutomationPeer, xaml_automation::Provider::IWindowProvider
    {
    public:
        MockWindowPatternAutomationPeer(Microsoft::UI::Xaml::FrameworkElement^ control) : Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer(control){}

        virtual ~MockWindowPatternAutomationPeer(){}

        // These properties are defined on IWindowProvider as read only.
        // For each property we define a corresponding "Mock" property that is publicly settable. 
        virtual property xaml_automation::WindowInteractionState InteractionState { xaml_automation::WindowInteractionState get() { return InteractionStateMockValue; }}
        property xaml_automation::WindowInteractionState InteractionStateMockValue;

        virtual property bool IsModal { bool get() { return IsModalMockValue; } }
        property bool IsModalMockValue;

        virtual property bool IsTopmost { bool get() { return IsTopmostMockValue; }}
        property bool IsTopmostMockValue;

        virtual property bool Maximizable { bool get() { return MaximizableMockValue; }}
        property bool MaximizableMockValue;

        virtual property bool Minimizable { bool get() { return MinimizableMockValue; }}
        property bool MinimizableMockValue;

        virtual property xaml_automation::WindowVisualState VisualState { xaml_automation::WindowVisualState get() { return VisualStateMockValue; }}
        property xaml_automation::WindowVisualState VisualStateMockValue;

        virtual void Close()
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual void SetVisualState(Microsoft::UI::Xaml::Automation::WindowVisualState state)
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual bool WaitForInputIdle(int milliseconds)
        {
            return false;
        }

    protected:
        Platform::Object^ GetPatternCore(Microsoft::UI::Xaml::Automation::Peers::PatternInterface patternInterface) override
        {
            if (patternInterface == xaml_automation_peers::PatternInterface::Window)
            {
                return this;
            }

            return __super::GetPatternCore(patternInterface);
        }
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class MockWindowPatternControl sealed : public Microsoft::UI::Xaml::FrameworkElement
    {
    protected:
        xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
        {
            return ref new MockWindowPatternAutomationPeer(this);
        }
    };
} } } } }
