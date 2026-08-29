// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AppBarLightDismissAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the AppBarLightDismissAutomationPeer
    PARTIAL_CLASS(AppBarLightDismissAutomationPeer)
    {
        public:
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationIdCore)(_Out_ HSTRING* returnValue);

            // Support the IInvokeProvider interface.
            _Check_return_ HRESULT InvokeImpl();
    };
}
