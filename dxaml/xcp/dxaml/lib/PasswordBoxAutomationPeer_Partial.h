// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PasswordBoxAutomationPeer.g.h"

namespace DirectUI
{
    // Exposes a PasswordBox to the UI Automation framework, used by accessibility aids.
    PARTIAL_CLASS(PasswordBoxAutomationPeer)
    {
        public:
            // Initializes a new instance of the PasswordBoxAutomationPeer class.
            PasswordBoxAutomationPeer();
            ~PasswordBoxAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
            IFACEMETHOD(IsPasswordCore)(_Out_ BOOLEAN* pReturnValue);
            _Check_return_ HRESULT GetDescribedByCoreImpl (_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue) final;
    };
}
