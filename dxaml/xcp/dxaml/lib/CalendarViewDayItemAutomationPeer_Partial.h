// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarViewDayItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the CalendarViewDayItemAutomationPeer
    PARTIAL_CLASS(CalendarViewDayItemAutomationPeer)
    {
        public:
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            IFACEMETHOD(IsEnabledCore)(_Out_ BOOLEAN* pReturnValue);

            _Check_return_ HRESULT get_ColumnImpl(_Out_ INT* pValue) override;
            _Check_return_ HRESULT get_RowImpl(_Out_ INT* pValue) override;

            _Check_return_ HRESULT GetColumnHeaderItemsImpl(_Out_ UINT* pReturnValueCount, _Out_writes_to_ptr_(*pReturnValueCount) xaml_automation::Provider::IIRawElementProviderSimple*** ppReturnValue);
            _Check_return_ HRESULT GetRowHeaderItemsImpl(_Out_ UINT* pReturnValueCount, _Out_writes_to_ptr_(*pReturnValueCount) xaml_automation::Provider::IIRawElementProviderSimple*** ppReturnValue);

            _Check_return_ HRESULT get_SelectionContainerImpl(_Outptr_result_maybenull_ xaml_automation::Provider::IIRawElementProviderSimple** ppValue);
            _Check_return_ HRESULT get_IsSelectedImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT AddToSelectionImpl();
            _Check_return_ HRESULT RemoveFromSelectionImpl();
            _Check_return_ HRESULT SelectImpl();

    };
}
