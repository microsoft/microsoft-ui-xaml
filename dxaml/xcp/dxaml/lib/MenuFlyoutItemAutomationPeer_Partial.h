// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuFlyoutItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the MenuFlyoutItemAutomationPeer
    PARTIAL_CLASS(MenuFlyoutItemAutomationPeer)
    {
        public:

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
            IFACEMETHOD(GetAcceleratorKeyCore)(_Out_ HSTRING* returnValue);

            _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* returnValue) final;
            _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* returnValue) final;

            _Check_return_ HRESULT InvokeImpl();
    };
}
