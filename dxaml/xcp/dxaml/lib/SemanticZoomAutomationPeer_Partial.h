// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SemanticZoomAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the SemanticZoomAutomationPeer
    PARTIAL_CLASS(SemanticZoomAutomationPeer)
    {
        public:
            // Initializes a new instance of the SemanticZoomAutomationPeer class.
            SemanticZoomAutomationPeer();
            ~SemanticZoomAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);

            _Check_return_ HRESULT ToggleImpl();
            _Check_return_ HRESULT get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue);

            _Check_return_ HRESULT RaiseToggleStatePropertyChangedEvent(_In_ BOOLEAN bNewValue);
    };
}
