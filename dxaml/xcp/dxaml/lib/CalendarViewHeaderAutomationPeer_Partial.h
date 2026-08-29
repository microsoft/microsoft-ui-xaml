// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarViewHeaderAutomationPeer.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(CalendarViewHeaderAutomationPeer)
    {
    public:
        IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue) override { return m_name.CopyTo(returnValue); }
        void Initialize(
            _In_ wrl_wrappers::HString&& name,
            _In_ int month,
            _In_ int year,
            _In_ xaml_controls::CalendarViewDisplayMode mode)
        {
            m_name = std::move(name);
            m_month = month;
            m_year = year;
            m_mode = mode;
        }
        int GetMonth() const { return m_month; }
        int GetYear() const { return m_year; }
        int GetMode() const { return m_mode; }

    private:
        wrl_wrappers::HString m_name;
        int m_month = -1;
        int m_year = -1;
        xaml_controls::CalendarViewDisplayMode m_mode = xaml_controls::CalendarViewDisplayMode_Month;
    };
}