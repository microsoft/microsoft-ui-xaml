// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplitMenuFlyoutItemAutomationPeer.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(SplitMenuFlyoutItemAutomationPeer)
    {
        public:
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
            IFACEMETHOD(GetAcceleratorKeyCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);
            IFACEMETHOD(HasKeyboardFocusCore)(_Out_ BOOLEAN* returnValue);
            IFACEMETHOD(IsKeyboardFocusableCore)(_Out_ BOOLEAN* returnValue);
            IFACEMETHOD(GetPeerFromPointCore)(_In_ wf::Point point, _Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            IFACEMETHOD(GetBoundingRectangleCore)(_Out_ wf::Rect* returnValue);

            _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* returnValue) final;
            _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* returnValue) final;

            // IInvokeProvider
            _Check_return_ HRESULT InvokeImpl();

            // IExpandCollapseProvider
            _Check_return_ HRESULT get_ExpandCollapseStateImpl(_Out_ xaml_automation::ExpandCollapseState* returnValue);
            _Check_return_ HRESULT ExpandImpl();
            _Check_return_ HRESULT CollapseImpl();

            _Check_return_ HRESULT RaiseExpandCollapseAutomationEvent(_In_ BOOLEAN isOpen);

        private:
            xaml_automation_peers::IAutomationPeer* GetPrimaryButtonPeer();
    };
}
