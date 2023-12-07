// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PopupAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the PopupAutomationPeer
    PARTIAL_CLASS(PopupAutomationPeer)
    {
        public:
            // Initializes a new instance of the PopupAutomationPeer class.
            PopupAutomationPeer();
            ~PopupAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue);

            _Check_return_ HRESULT get_IsModalImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_IsTopmostImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_MaximizableImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_MinimizableImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_InteractionStateImpl(_Out_ xaml_automation::WindowInteractionState* pValue);
            _Check_return_ HRESULT get_VisualStateImpl(_Out_ xaml_automation::WindowVisualState* pValue);

            _Check_return_ HRESULT CloseImpl();
            _Check_return_ HRESULT SetVisualStateImpl(_In_ xaml_automation::WindowVisualState state);
            _Check_return_ HRESULT WaitForInputIdleImpl(_In_ INT milliseconds, _Out_ BOOLEAN* returnValue);
    };
}
