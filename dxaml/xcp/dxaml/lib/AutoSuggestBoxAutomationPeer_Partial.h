// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "AutoSuggestBoxAutomationPeer.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(AutoSuggestBoxAutomationPeer)
    {
    public:
        // Initializes a new instance of the AutoSuggestBoxAutomationPeer class.
        AutoSuggestBoxAutomationPeer();
        ~AutoSuggestBoxAutomationPeer() override;

        IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
        IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
        IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);

        // Support the IInvokeProvider interface.
        _Check_return_ HRESULT InvokeImpl();
    };
}
