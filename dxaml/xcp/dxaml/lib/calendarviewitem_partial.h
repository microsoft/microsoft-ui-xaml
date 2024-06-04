// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "calendarviewitem.g.h"

namespace DirectUI
{

    PARTIAL_CLASS(CalendarViewItem)
    {

    public:

        // Called when a pointer makes a tap gesture on a ListViewBaseItem.
        IFACEMETHOD(OnTapped)(
            _In_ xaml_input::ITappedRoutedEventArgs* pArgs)
            override;

        // Handles when a key is pressed down on the CalendarView.
        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;
    
    protected:
        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_result_maybenull_ xaml_automation_peers::IAutomationPeer** returnValue);

    public:
        _Check_return_ HRESULT GetDate(_Out_ wf::DateTime* pDate) override { return get_Date(pDate); }

#if DBG
        _Check_return_ HRESULT put_Date(_In_ wf::DateTime value) override;
#endif
    };
}
