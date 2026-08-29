// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AutomationClientManager.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationClient {

    class AutomationGenericTests
    {
    public:
        AutomationGenericTests(std::shared_ptr<AutomationClientManager> spAClientManager)
            : m_spAClientManager(spAClientManager)
        {
        }

        // Essential Pattern tests for various control types. Please add tests in switch case for control type of your interests if it's not here.
        // List of mandatory Patterns for a specific control type can be found here: http://msdn.microsoft.com/en-us/library/windows/desktop/ee671633(v=vs.85).aspx
        void DoesSupportEssentialPatternsForControlType(CONTROLTYPEID controlType)
        {
            WEX::Common::Throw::IfNull(m_spAClientManager.get());

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationInvokePattern> spInvokePattern;
            wrl::ComPtr<IUIAutomationExpandCollapsePattern> spExpandCollapsePattern;
            wrl::ComPtr<IUIAutomationSelectionPattern> spSelectionPattern;

            m_spAClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            switch (controlType)
            {
            case UIA_ButtonControlTypeId:
                spUIAutomationElement->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), &spInvokePattern);
                VERIFY_IS_NOT_NULL(spInvokePattern.Get(), L"AutomationGenericTests::DoesSupportEssentialPatternsForControlType: This Button doesn't support Invoke Pattern which is required.");
                break;
            case UIA_ComboBoxControlTypeId:
                spUIAutomationElement->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, __uuidof(IUIAutomationExpandCollapsePattern), &spExpandCollapsePattern);
                VERIFY_IS_NOT_NULL(spExpandCollapsePattern.Get(), L"AutomationGenericTests::DoesSupportEssentialPatternsForControlType: This ComboBox doesn't support ExpandCollapse Pattern which is required.");
                spUIAutomationElement->GetCurrentPatternAs(UIA_SelectionPatternId, __uuidof(IUIAutomationSelectionPattern), &spSelectionPattern);
                VERIFY_IS_NOT_NULL(spSelectionPattern.Get(), L"AutomationGenericTests::DoesSupportEssentialPatternsForControlType: This ComboBox doesn't support Selection Pattern which is required.");
                break;
            case UIA_HyperlinkControlTypeId:
                spUIAutomationElement->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), &spInvokePattern);
                VERIFY_IS_NOT_NULL(spInvokePattern.Get(), L"AutomationGenericTests::DoesSupportEssentialPatternsForControlType: This Hyperlink doesn't support Invoke Pattern which is required.");
                break;
            default:
                WEX::Common::Throw::IfNull(NULL, L"AutomationGenericTests::DoesSupportEssentialPatternsForControlType: Not a supported Control type. If its relevant control type please add required tests for it in the switch case.");
                break;
            }
        }

    private:
        std::shared_ptr<AutomationClientManager> m_spAClientManager;
    };
} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationClient
