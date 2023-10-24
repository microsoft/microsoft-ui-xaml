// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DateComparer.h"
#include "calendarview_partial.h" // for CalendarConstants::s_maxTicksPerDay

using namespace DirectUI;

std::function<bool(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs)> DateComparer::GetLessThanComparer()
{
    return [this](wf::DateTime lhs, wf::DateTime rhs)
    {
        return LessThan(lhs, rhs);
    };
}

std::function<bool(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs)> DateComparer::GetAreEquivalentComparer()
{
    return [this](wf::DateTime lhs, wf::DateTime rhs)
    {
        return AreEquivalent(lhs, rhs);
    };
}


_Check_return_ HRESULT DateComparer::SetCalendarForComparison(_In_ wg::ICalendar* pCalendar)
{
    return pCalendar->Clone(&m_spCalendar);
}

int DateComparer::CompareDay(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs)
{
    HRESULT hr = S_OK;
    int result = 0;

    IFC(CompareDay(lhs, rhs, &result));

Cleanup:
    if (FAILED(hr))
    {
        THROW_HR(hr);
    }
    return result;
}

_Check_return_ HRESULT DateComparer::CompareDay(
    _In_ wf::DateTime lhs, 
    _In_ wf::DateTime rhs,
    _Out_ int* pResult)
{
    return CompareDate(lhs, rhs, CalendarConstants::s_maxTicksPerDay, &wg::ICalendar::get_Day, pResult);
}

_Check_return_ HRESULT DateComparer::CompareMonth(
    _In_ wf::DateTime lhs,
    _In_ wf::DateTime rhs,
    _Out_ int* pResult)
{
    return CompareDate(lhs, rhs, CalendarConstants::s_maxTicksPerMonth, &wg::ICalendar::get_Month, pResult);
}

_Check_return_ HRESULT DateComparer::CompareYear(
    _In_ wf::DateTime lhs,
    _In_ wf::DateTime rhs,
    _Out_ int* pResult)
{
    return CompareDate(lhs, rhs, CalendarConstants::s_maxTicksPerYear, &wg::ICalendar::get_Year, pResult);
}


// compare two datetime values, when the difference between two UTC values is greater 
// than given threshold they can be compared directly by the UTC values.
// otherwise we need to have globalization calendar to help us to determine, basically 
// we need the corresponding function from calendar (get_Day, get_Month or get_Year).

_Check_return_ HRESULT DateComparer::CompareDate(
    _In_ wf::DateTime lhs, 
    _In_ wf::DateTime rhs, 
    _In_ INT64 threshold, 
    _In_ GetUnitFunc getUnit,
    _Out_ int* pResult)
{
    int sign = 1;
    *pResult = 1;

    ASSERT(m_spCalendar);

    INT64 delta = lhs.UniversalTime - rhs.UniversalTime;
    if (delta < 0)
    {
        delta = -delta;
        sign = -1;
    }
    // comparing the date parts of two datetime is expensive. only compare them when they could in a same day/month/year.
    if (delta < threshold)
    {
        int left = 0;
        int right = 0;

        IFC_RETURN(m_spCalendar->SetDateTime(lhs));
        IFC_RETURN((m_spCalendar.Get()->*getUnit)(&left));

        IFC_RETURN(m_spCalendar->SetDateTime(rhs));
        IFC_RETURN((m_spCalendar.Get()->*getUnit)(&right));

        if (left == right)
        {
            *pResult = 0;
        }
    }

    *pResult *= sign;

    return S_OK;
}


