// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectorAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the SelectorAutomationPeer
    PARTIAL_CLASS(SelectorAutomationPeer)
    {
        public:
            // Initializes a new instance of the SelectorAutomationPeer class.
            SelectorAutomationPeer();
            ~SelectorAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);

            // ISelectionProvider
            _Check_return_ HRESULT GetSelectionImpl(_Out_ UINT* returnValueCount, _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** returnValue);
            _Check_return_ HRESULT get_CanSelectMultipleImpl(_Out_ BOOLEAN* pValue);
            virtual _Check_return_ HRESULT get_IsSelectionRequiredImpl(_Out_ BOOLEAN* pValue);

            _Check_return_ HRESULT GetPeerForSelectedItem(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::ISelectorItemAutomationPeer** ppPeer);
            _Check_return_ HRESULT RaiseSelectionEvents(_In_ xaml_controls::ISelectionChangedEventArgs* pSelectionChangedEventArgs) noexcept;
            _Check_return_ HRESULT GetSelectedItemAutomationPeer(_Outptr_opt_ xaml_automation_peers::IItemAutomationPeer** pAP);

        private:
            static const UINT BulkChildrenLimit = 20;
    };
}
