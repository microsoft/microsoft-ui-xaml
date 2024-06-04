// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarViewBaseItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the CalendarViewBaseItemAutomationPeer
    PARTIAL_CLASS(CalendarViewBaseItemAutomationPeer)
    {
    public:
        IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
        IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue);

        // Properties.
        virtual _Check_return_ HRESULT get_ColumnImpl(_Out_ INT* pValue) = 0;
        _Check_return_ HRESULT get_ColumnSpanImpl(_Out_ INT* pValue);
        _Check_return_ HRESULT get_ContainingGridImpl(_Outptr_result_maybenull_ xaml_automation::Provider::IIRawElementProviderSimple** ppValue);
        virtual _Check_return_ HRESULT get_RowImpl(_Out_ INT* pValue) = 0;
        _Check_return_ HRESULT get_RowSpanImpl(_Out_ INT* pValue);

        // Methods.
        _Check_return_ HRESULT ScrollIntoViewImpl();

    protected:
        _Check_return_ HRESULT IsItemVisible(bool& isVisible);
    };
}
