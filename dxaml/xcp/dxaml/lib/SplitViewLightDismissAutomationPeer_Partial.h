// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplitViewLightDismissAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the SplitViewLightDismissAutomationPeer
    PARTIAL_CLASS(SplitViewLightDismissAutomationPeer)
    {
        public:
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* pValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pValue);
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* pValue);
            IFACEMETHOD(GetAutomationIdCore)(_Out_ HSTRING* pValue);

            // Support the IInvokeProvider interface.
            _Check_return_ HRESULT InvokeImpl();

        private:
            HRESULT IsLightDismissEnabled(bool *pIsLightDimissEnabled);
    };
}
