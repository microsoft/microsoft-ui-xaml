// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ComboBoxAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ComboBoxAutomationPeer
    PARTIAL_CLASS(ComboBoxAutomationPeer)
    {
        public:
            // Initializes a new instance of the ComboBoxAutomationPeer class.
            ComboBoxAutomationPeer();
            ~ComboBoxAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING *returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
            IFACEMETHOD(GetHelpTextCore)(_Out_ HSTRING * returnValue) override;

            // IValueProvider
            // Properties.
            _Check_return_ HRESULT get_IsReadOnlyImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_ValueImpl(_Out_ HSTRING* pValue);

            // Methods.
            _Check_return_ HRESULT SetValueImpl(_In_ HSTRING value);

            // IExpandCollapseProvider
            //Properties
            _Check_return_ HRESULT get_ExpandCollapseStateImpl(_Out_ xaml_automation::ExpandCollapseState* returnValue);

            // ISelectionProvider override
            _Check_return_ HRESULT get_IsSelectionRequiredImpl(_Out_ BOOLEAN* pValue) override;

            // IWindowProvider
            _Check_return_ HRESULT get_IsModalImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_IsTopmostImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_MaximizableImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_MinimizableImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_InteractionStateImpl(_Out_ xaml_automation::WindowInteractionState* pValue);
            _Check_return_ HRESULT get_VisualStateImpl(_Out_ xaml_automation::WindowVisualState* pValue);
            _Check_return_ HRESULT CloseImpl();
            _Check_return_ HRESULT SetVisualStateImpl(_In_ xaml_automation::WindowVisualState state);
            _Check_return_ HRESULT WaitForInputIdleImpl(_In_ INT milliseconds, _Out_ BOOLEAN* pValue);

            // Methods
            _Check_return_ HRESULT ExpandImpl();
            _Check_return_ HRESULT CollapseImpl();

            _Check_return_ HRESULT OnCreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue) override;
            _Check_return_ HRESULT RaiseExpandCollapseAutomationEvent(_In_ BOOLEAN isDropDownOpen);
    };
}
