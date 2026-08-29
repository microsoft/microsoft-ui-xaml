// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarViewItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the CalendarViewItemAutomationPeer
    PARTIAL_CLASS(CalendarViewItemAutomationPeer)
    {
        public:
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);

            _Check_return_ HRESULT get_ColumnImpl(_Out_ INT* pValue) override;
            _Check_return_ HRESULT get_RowImpl(_Out_ INT* pValue) override;

            _Check_return_ HRESULT GetColumnHeaderItemsImpl(_Out_ UINT* pReturnValueCount, _Out_writes_to_ptr_(*pReturnValueCount) xaml_automation::Provider::IIRawElementProviderSimple*** ppReturnValue);
            _Check_return_ HRESULT GetRowHeaderItemsImpl(_Out_ UINT* pReturnValueCount, _Out_writes_to_ptr_(*pReturnValueCount) xaml_automation::Provider::IIRawElementProviderSimple*** ppReturnValue);

            _Check_return_ HRESULT InvokeImpl();
    };
}
