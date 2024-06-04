// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "HyperlinkButtonAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the HyperlinkButtonAutomationPeer
    PARTIAL_CLASS(HyperlinkButtonAutomationPeer)
    {
        public:
            // Initializes a new instance of the HyperlinkButtonAutomationPeer class.
            HyperlinkButtonAutomationPeer();
            ~HyperlinkButtonAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            
            // Support the IInvokeProvider interface.
            _Check_return_ HRESULT InvokeImpl();
    };
}
