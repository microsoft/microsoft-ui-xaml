// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarViewAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the CalendarViewAutomationPeer
    PARTIAL_CLASS(CalendarViewAutomationPeer)
    {
        public:
            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue);

            // Properties.
            _Check_return_ HRESULT get_CanSelectMultipleImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_IsSelectionRequiredImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_IsReadOnlyImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_ValueImpl(_Out_ HSTRING* pValue);
            _Check_return_ HRESULT get_RowOrColumnMajorImpl(_Out_ xaml_automation::RowOrColumnMajor* pValue);
            _Check_return_ HRESULT get_ColumnCountImpl(_Out_ INT* pValue);
            _Check_return_ HRESULT get_RowCountImpl(_Out_ INT* pValue);

            // Methods.
            _Check_return_ HRESULT GetSelectionImpl(_Out_ UINT* pReturnValueCount, _Out_writes_to_ptr_(*pReturnValueCount) xaml_automation::Provider::IIRawElementProviderSimple*** ppReturnValue);
            _Check_return_ HRESULT SetValueImpl(_In_ HSTRING value);
            _Check_return_ HRESULT GetColumnHeadersImpl(_Out_ UINT* pReturnValueCount, _Out_writes_to_ptr_(*pReturnValueCount) xaml_automation::Provider::IIRawElementProviderSimple*** ppReturnValue);
            _Check_return_ HRESULT GetRowHeadersImpl(_Out_ UINT* pReturnValueCount, _Out_writes_to_ptr_(*pReturnValueCount) xaml_automation::Provider::IIRawElementProviderSimple*** ppReturnValue);
            _Check_return_ HRESULT GetItemImpl(_In_ INT row, _In_ INT column, _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue);

            _Check_return_ HRESULT RaiseSelectionEvents(_In_ xaml_controls::ICalendarViewSelectedDatesChangedEventArgs* pSelectionChangedEventArgs);

    private:
            HRESULT RemoveAPs(_Inout_ wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAPCollection);
            static const UINT BulkChildrenLimit = 20;
    };
}
