// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextBoxAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the TextBoxAutomationPeer
    PARTIAL_CLASS(TextBoxAutomationPeer)
    {
        public:
            // Initializes a new instance of the TextBoxAutomationPeer class.
            TextBoxAutomationPeer();
            ~TextBoxAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
            _Check_return_ HRESULT GetDescribedByCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue) final;
    };
}
