// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarDatePickerAutomationPeer.g.h"

namespace DirectUI
{
    // Exposes a CalendarDatePicker to the UI Automation framework, used by accessibility aids.
    PARTIAL_CLASS(CalendarDatePickerAutomationPeer)
    {
    public:
        // Initializes a new instance of the CalendarDatePickerAutomationPeer class.
        CalendarDatePickerAutomationPeer();
        ~CalendarDatePickerAutomationPeer() override;

        IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
        IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
        IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
        IFACEMETHOD(GetLocalizedControlTypeCore)(_Out_ HSTRING* returnValue) override;

        _Check_return_ HRESULT InvokeImpl();
        
        //IValueProvider
        // Properties.
        _Check_return_ HRESULT get_IsReadOnlyImpl(_Out_ BOOLEAN* value);
        _Check_return_ HRESULT get_ValueImpl(_Out_ HSTRING* value);

        // Methods.
        _Check_return_ HRESULT SetValueImpl(HSTRING value);        
    };
}
