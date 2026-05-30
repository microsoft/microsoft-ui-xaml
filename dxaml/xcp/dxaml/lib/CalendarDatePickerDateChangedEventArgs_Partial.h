// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarDatePickerDateChangedEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(CalendarDatePickerDateChangedEventArgs)
    {
    public:
        // Customized properties.
        _Check_return_ HRESULT get_NewDateImpl(_Out_ wf::IReference<wf::DateTime>** ppValue);
        _Check_return_ HRESULT put_NewDateImpl(_In_ wf::IReference<wf::DateTime>* pValue);
        _Check_return_ HRESULT get_OldDateImpl(_Out_ wf::IReference<wf::DateTime>** ppValue);
        _Check_return_ HRESULT put_OldDateImpl(_In_ wf::IReference<wf::DateTime>* pValue);

    private:
        TrackerPtr<wf::IReference<wf::DateTime>> m_tpNewDate;
        TrackerPtr<wf::IReference<wf::DateTime>> m_tpOldDate;
    };
}