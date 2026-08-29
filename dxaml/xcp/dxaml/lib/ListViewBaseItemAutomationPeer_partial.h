// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemInvokeAdapter.g.h"
#include "ListViewBaseItem.g.h"
#include "ListViewBaseItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ListViewBaseItemAutomationPeer
    PARTIAL_CLASS(ListViewBaseItemAutomationPeer)
    {
        public:
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);

            // IDragProvider
            _Check_return_ HRESULT get_IsGrabbedImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_DropEffectImpl(_Out_ HSTRING* pValue);
            _Check_return_ HRESULT get_DropEffectsImpl(_Out_ UINT* pCount, _Outptr_ HSTRING** pValue);
            _Check_return_ HRESULT GetGrabbedItemsImpl(_Out_ UINT* returnValueCount, _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** returnValue);

        private:
            _Check_return_ HRESULT ShouldSupportInvokePattern(_In_ DirectUI::ListViewBaseItem* listViewbaseItem,_Out_ bool* supportInvokePatternOut);

        private:
            // This is IInspectable and not DO, also it will never cross the dll boundary and doesn't require to be
            // a TrackerPtr.
            ctl::ComPtr<DirectUI::ItemInvokeAdapter> m_spInvokeAdapter;
    };
}
