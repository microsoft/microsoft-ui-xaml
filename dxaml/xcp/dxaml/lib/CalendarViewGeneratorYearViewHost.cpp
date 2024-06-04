// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarViewGeneratorYearViewHost.h"
#include "CalendarView.g.h"
#include "CalendarViewItem.g.h"
#include "DateComparer.h"
#include "AutomationProperties.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Work around disruptive max/min macros
#undef max
#undef min

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::GetContainer(
    _In_ IInspectable* pItem,
    _In_opt_ xaml::IDependencyObject* pRecycledContainer,
    _Outptr_ CalendarViewBaseItem** ppContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CalendarViewItem> spContainer;

    IFC(ctl::make<CalendarViewItem>(&spContainer));

    IFC(spContainer.CopyTo(ppContainer));

Cleanup:
    return hr;
}


_Check_return_ IFACEMETHODIMP CalendarViewGeneratorYearViewHost::PrepareItemContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    wf::DateTime date;
    ctl::ComPtr<CalendarViewItem> spContainer;

    spContainer = static_cast<CalendarViewItem*>(pContainer);

    IFC(ctl::do_get_value(date, pItem));
    IFC(GetCalendar()->SetDateTime(date));
    IFC(spContainer->put_Date(date));

    // maintext
    {
        wrl_wrappers::HString mainText;
        wrl_wrappers::HString automationName;

        IFC(GetCalendar()->MonthAsFullString(
            automationName.GetAddressOf()));

        IFC(AutomationProperties::SetNameStatic(spContainer.Cast<FrameworkElement>(), automationName.Get()));

        IFC(GetCalendar()->MonthAsString(
            0, /*idealLength, set to 0 to get the abbreviated string*/
            mainText.GetAddressOf()));

        IFC(spContainer->UpdateMainText(mainText.Get()));
    }

    // label text
    {
        BOOLEAN isLabelVisible = FALSE;

        IFC(GetOwner()->get_IsGroupLabelVisible(&isLabelVisible));

        IFC(UpdateLabel(spContainer.Get(), !!isLabelVisible));
    }

    // today state will be updated in CalendarViewGeneratorHost::PrepareItemContainer

    // YearView doesn't have selection state
    
    // Make a grid effect on YearView.
    // For MonthView, we put a margin on CalendarViewDayItem in the template to achieve the grid effect.
    // For YearView and DecadeView, we can't do the same because there is no template for MonthItem and YearItem
    {
        IFC(spContainer->put_Margin(GetItemMargin()));
    }

    //This code enables the focus visuals on the CalendarViewItems in the Year Pane in the correct position.
    {
        IFC(spContainer->put_FocusVisualMargin(GetItemFocusVisualMargin()));
        IFC(spContainer->put_UseSystemFocusVisuals(TRUE));
    }

    IFC(CalendarViewGeneratorHost::PrepareItemContainer(pContainer, pItem));

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::UpdateLabel(_In_ CalendarViewBaseItem* pItem, _In_ bool isLabelVisible)
{
    bool showLabel = false;
    if (isLabelVisible)
    {
        wf::DateTime date;
        auto pCalendar = GetCalendar();
        int month = 0;
        int firstMonthOfThisYear = 0;

        // TODO: consider caching the firstday flag because we also need this information when determining snap points 
        // (however DecadeView doesn't need this for Label).
        IFC_RETURN(pItem->GetDate(&date));
        IFC_RETURN(pCalendar->SetDateTime(date));
        IFC_RETURN(pCalendar->get_FirstMonthInThisYear(&firstMonthOfThisYear));
        IFC_RETURN(pCalendar->get_Month(&month));

        showLabel = firstMonthOfThisYear == month;

        if (showLabel)
        {
            wrl_wrappers::HString labelText;
            IFC_RETURN(pCalendar->YearAsString(labelText.GetAddressOf()));
            IFC_RETURN(pItem->UpdateLabelText(labelText.Get()));
        }
    }
    IFC_RETURN(pItem->ShowLabelText(showLabel));
    return S_OK;
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::GetIsFirstItemInScope(_In_ int index, _Out_ bool* pIsFirstItemInScope)
{
    HRESULT hr = S_OK;

    *pIsFirstItemInScope = false;
    if (index == 0)
    {
        *pIsFirstItemInScope = true;
    }
    else
    {
        wf::DateTime date = {};
        int month = 0;
        int firstMonth = 0;

        IFC(GetDateAt(index, &date));
        auto pCalendar = GetCalendar();
        IFC(pCalendar->SetDateTime(date));
        IFC(pCalendar->get_Month(&month));
        IFC(pCalendar->get_FirstMonthInThisYear(&firstMonth));
        *pIsFirstItemInScope = month == firstMonth;
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::GetUnit(_Out_ int* pValue)
{
    return GetCalendar()->get_Month(pValue);
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::SetUnit(_In_ int value)
{
    return GetCalendar()->put_Month(value);
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::AddUnits(_In_ int value)
{
    return GetCalendar()->AddMonths(value);
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::AddScopes(_In_ int value)
{
    IFC_RETURN(GetCalendar()->AddYears(value));
    return S_OK;
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::GetFirstUnitInThisScope(_Out_ int* pValue)
{
    return GetCalendar()->get_FirstMonthInThisYear(pValue);
}
_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::GetLastUnitInThisScope(_Out_ int* pValue)
{
    return GetCalendar()->get_LastMonthInThisYear(pValue);
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::OnScopeChanged()
{
    return GetOwner()->FormatYearName(m_maxDateOfCurrentScope, m_pHeaderText.ReleaseAndGetAddressOf());
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::GetPossibleItemStrings(_Outptr_ const std::vector<wrl_wrappers::HString>** ppStrings)
{
    *ppStrings = &m_possibleItemStrings;

    if (m_possibleItemStrings.empty())
    {
        // for all known calendar identifiers so far (10 different calendar identifiers), we can find the longest year in no more than 3 years
        // if we start from min date of this calendar.

        // below are the longest year and the lowest index of that year we found for each calendar identifier. 
        // we hope that any new calendar in the future don't break this rule.

        // PersianCalendar, maxLength = 12 @ index 0
        // GregorianCalendar, maxLength = 12 @ index 0
        // HebrewCalendar, maxLength = 13 @ index 2
        // HijriCalendar, maxLength = 12 @ index 0
        // JapaneseCalendar, maxLength = 12 @ index 0
        // JulianCalendar, maxLength = 12 @ index 0
        // KoreanCalendar, maxLength = 12 @ index 0
        // TaiwanCalendar, maxLength = 12 @ index 0
        // ThaiCalendar, maxLength = 12 @ index 1
        // UmAlQuraCalendar, maxLength = 12 @ index 0
        {
            const int MaxNumberOfYearsToBeChecked = 3;
            wf::DateTime longestYear;
            int lengthOfLongestYear = 0;
            int numberOfMonths = 0;
            int month = 0;

            auto pCalendar = GetCalendar();

            IFC_RETURN(pCalendar->SetToMin());
            for (int i = 0; i < MaxNumberOfYearsToBeChecked; i++)
            {
                IFC_RETURN(pCalendar->get_NumberOfMonthsInThisYear(&numberOfMonths));
                if (numberOfMonths > lengthOfLongestYear)
                {
                    lengthOfLongestYear = numberOfMonths;
                    IFC_RETURN(pCalendar->GetDateTime(&longestYear));
                }
                IFC_RETURN(pCalendar->AddYears(1));
            }

            ASSERT(lengthOfLongestYear == 13 || lengthOfLongestYear == 12);
            IFC_RETURN(pCalendar->SetDateTime(longestYear));
            IFC_RETURN(pCalendar->get_FirstMonthInThisYear(&month));
            IFC_RETURN(pCalendar->put_Month(month));

            m_possibleItemStrings.reserve(lengthOfLongestYear);

            for (int i = 0; i < lengthOfLongestYear; i++)
            {
                wrl_wrappers::HString string;
                
                IFC_RETURN(pCalendar->MonthAsString(
                    0, /*idealLength, set to 0 to get the abbreviated string*/
                    string.GetAddressOf()));
                m_possibleItemStrings.emplace_back(std::move(string));
                IFC_RETURN(pCalendar->AddMonths(1));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarViewGeneratorYearViewHost::CompareDate(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs, _Out_ int* pResult)
{
    return GetOwner()->GetDateComparer()->CompareMonth(lhs, rhs, pResult);
}

INT64 CalendarViewGeneratorYearViewHost::GetAverageTicksPerUnit()
{
    // this is being used to estimate the distance between two dates,
    // it doesn't need to be (and it can't be) the exact value
    return CalendarConstants::s_ticksPerDay * 365 / 12;
}
