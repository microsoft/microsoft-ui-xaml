// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


namespace DirectUI
{

    class DateComparer
    {
    public:

        _Check_return_ HRESULT SetCalendarForComparison(_In_ wg::ICalendar* pCalendar);

        std::function<bool(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs)> GetLessThanComparer();
        std::function<bool(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs)> GetAreEquivalentComparer();

        // check if the whole date parts of two DateTime values are same.
        bool AreEquivalent(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs)
        {
            return CompareDay(lhs, rhs) == 0;
        }

        // check if the date part of lhs is less than the date part of rhs.
        bool LessThan(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs)
        {
            return CompareDay(lhs, rhs) == -1;
        }

        // check if two DateTimes have same dates, ignore the time part. the reasons are:
        //1. we don't care time part because this is about Calendar.
        //2. having timepart will cause problem when counting the index of a date due to the daylight saving.
        //3. developer could select a date which contains an arbitarty timepart(e.g.both 1 / 1 / 2000 8:34AM and 1 / 1 / 2000 3:12PM will select the same dayitem)

        // return
        // -1 if lhs < rhs
        // 0 if lhs == rhs
        // 1 if lhs > rhs
        _Check_return_ HRESULT CompareDay(
            _In_ wf::DateTime lhs, 
            _In_ wf::DateTime rhs, 
            _Out_ int* pResult);

        // check if two DateTime values are on the same month (including year, era, but not day)
        _Check_return_ HRESULT CompareMonth(
            _In_ wf::DateTime lhs, 
            _In_ wf::DateTime rhs, 
            _Out_ int* pResult);

        // check if two DateTime values are on the same year (including era, but not month or day)
        _Check_return_ HRESULT CompareYear(
            _In_ wf::DateTime lhs, 
            _In_ wf::DateTime rhs, 
            _Out_ int* pResult);
        
    private:

        // throw version
        int CompareDay(
            _In_ wf::DateTime lhs,
            _In_ wf::DateTime rhs);

        typedef HRESULT(__stdcall wg::ICalendar::* GetUnitFunc)(_Out_ INT32*);

        // compare two datetime values, when the difference between two UTC values is greater 
        // than given threshold they can be compared directly by the UTC values.
        // otherwise we need to have globalization calendar to help us to determine, basically 
        // we need the corresponding function from calendar (get_Day, get_Month or get_Year).
        _Check_return_ HRESULT CompareDate(
            _In_ wf::DateTime lhs, 
            _In_ wf::DateTime rhs, 
            _In_ INT64 threshold, 
            _In_ GetUnitFunc getUnit,
            _Out_ int* pResult);

    private:
        ctl::ComPtr<wg::ICalendar> m_spCalendar;

    };
}


