// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuFlyoutSubItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the MenuFlyoutSubItemAutomationPeer
    PARTIAL_CLASS(MenuFlyoutSubItemAutomationPeer)
    {
        public:

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);

            // IExpandCollapseProvider
            _Check_return_ HRESULT get_ExpandCollapseStateImpl(_Out_ xaml_automation::ExpandCollapseState* returnValue);

            _Check_return_ HRESULT ExpandImpl();
            _Check_return_ HRESULT CollapseImpl();

            _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* returnValue) final;
            _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* returnValue) final;

            _Check_return_ HRESULT RaiseExpandCollapseAutomationEvent(_In_ BOOLEAN isOpen);
    };
}
