// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemInvokeAdapter.g.h"
#include "ListViewBaseHeaderItem.g.h"
#include "ListViewBaseHeaderItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ListViewBaseHeaderItemAutomationPeer
    PARTIAL_CLASS(ListViewBaseHeaderItemAutomationPeer)
    {
        public:
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            IFACEMETHOD(GetBoundingRectangleCore)(_Out_ wf::Rect* returnValue);
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** patternOut);

            _Check_return_ HRESULT get_ParentItemsControlAutomationPeer(_Out_ xaml_automation_peers::IItemsControlAutomationPeer** ppParentItemsControl);

            _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* pReturnValue) final;
            _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* pReturnValue) final;
            _Check_return_ HRESULT GetLevelCoreImpl(_Out_ INT* pReturnValue) final;

        private:
            _Check_return_ HRESULT ShouldSupportInvokePattern(_In_ DirectUI::ListViewBaseHeaderItem* listViewbaseHeaderItem, _Out_ bool* supportInvokePatternOut);

        private:
            // Adapter that performs Invoke action
            ctl::ComPtr<DirectUI::ItemInvokeAdapter> m_itemInvokeAdapter;
    };
}
