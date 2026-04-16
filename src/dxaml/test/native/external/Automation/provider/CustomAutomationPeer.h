// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "pch.h"

namespace Tests { namespace Native { namespace External { namespace Automation { namespace Provider {

    typedef struct
    {
        xaml_automation_peers::AutomationControlType controlType;
        Platform::String^ localizedControlType;
    } ControlTypeData;

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class CustomAutomationPeer sealed : public Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer
    {
    public:
        CustomAutomationPeer(Microsoft::UI::Xaml::FrameworkElement^ control) : Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer(control)
        {
            m_currentControlType = xaml_automation_peers::AutomationControlType::Button;
        }

        virtual ~CustomAutomationPeer()
        {
        }

        void SetControlType(xaml_automation_peers::AutomationControlType controlType)
        {
            m_currentControlType = controlType;
        }

    protected:
        xaml_automation_peers::AutomationControlType GetAutomationControlTypeCore() override
        {
            return m_currentControlType;
        }

    private:
        xaml_automation_peers::AutomationControlType m_currentControlType;
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class CustomControl sealed : public Microsoft::UI::Xaml::FrameworkElement
    {
    protected:
        xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
        {
            return ref new CustomAutomationPeer(this);
        }
    };
} } } } }
