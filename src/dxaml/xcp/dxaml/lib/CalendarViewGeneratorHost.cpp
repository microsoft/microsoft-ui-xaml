// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarViewGeneratorHost.h"
#include "CalendarView.g.h"
#include "CalendarViewItem.g.h"
#include "CalendarViewDayItem_Partial.h"
#include "CalendarViewDayItemChangingEventArgs.g.h"
#include "DateComparer.h"
#include "CalendarPanel.g.h"
#include "ScrollViewer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Work around disruptive max/min macros
#undef max
#undef min

// IGeneratorHost

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::get_View(
    _Outptr_ wfc::IVector<IInspectable*>** ppView)
{
    ctl::ComPtr<CalendarViewGeneratorHost> spThis(this);

    return spThis.CopyTo(ppView);
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::get_CollectionView(
    _Outptr_ xaml_data::ICollectionView** ppCollectionView)
{
    // return nullptr so MCBP knows there is no group.
    *ppCollectionView = nullptr;

    return S_OK;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::IsItemItsOwnContainer(
    _In_ IInspectable* pItem,
    _Out_ BOOLEAN* pIsOwnContainer)
{
    // our item is DateTime, not the container
    *pIsOwnContainer = false;
    return S_OK;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::GetContainerForItem(
    _In_ IInspectable* pItem,
    _In_opt_ xaml::IDependencyObject* pRecycledContainer,
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CalendarViewBaseItem> spContainer;

    IFC(GetContainer(pItem, pRecycledContainer, &spContainer));
    spContainer->SetParentCalendarView(GetOwner());

    IFC(spContainer.MoveTo(ppContainer));

Cleanup:
    return hr;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::PrepareItemContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ IInspectable* pItem)
{
    // All calendar items have same scope logical, handle it here:
    ctl::ComPtr<CalendarViewItem> spContainer(static_cast<CalendarViewItem*>(pContainer));

    IFC_RETURN(spContainer->SetIsOutOfScope(false));

    // today state
    {
        wf::DateTime date;
        bool isToday = false;
        int result = 0;

        IFC_RETURN(ctl::do_get_value(date, pItem));

        IFC_RETURN(CompareDate(date, GetOwner()->GetToday(), &result));
        if (result == 0)
        {
            BOOLEAN isTodayHighlighted = FALSE;

            IFC_RETURN(GetOwner()->get_IsTodayHighlighted(&isTodayHighlighted));

            isToday = !!isTodayHighlighted;
        }

        IFC_RETURN(spContainer->SetIsToday(isToday));
    }

    return S_OK;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::ClearContainerForItem(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ IInspectable* pItem)
{
    return S_OK;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::IsHostForItemContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _Out_ BOOLEAN* pIsHost)
{
    return E_NOTIMPL;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::GetGroupStyle(
    _In_opt_ xaml_data::ICollectionViewGroup* pGroup,
    _In_ UINT level,
    _Out_ xaml_controls::IGroupStyle** ppGroupStyle)
{
    // The modern panel is always going to ask for a GroupStyle.
    // Fortunately, it's perfectly valid to return null
    *ppGroupStyle = nullptr;
    return S_OK;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::SetIsGrouping(
    _In_ BOOLEAN isGrouping)
{
    ASSERT(!isGrouping);
    return S_OK;
}

// we don't expose this publicly, there is an override for our own controls
// to mirror the public api
_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::GetHeaderForGroup(
    _In_ IInspectable* pGroup,
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    return E_NOTIMPL;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::PrepareGroupContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ xaml_data::ICollectionViewGroup* pGroup)
{
    return E_NOTIMPL;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::ClearGroupContainerForGroup(
    _In_ xaml::IDependencyObject* pContainer,
    _In_opt_ xaml_data::ICollectionViewGroup* pItem)
{
    return E_NOTIMPL;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::CanRecycleContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _Out_ BOOLEAN* pCanRecycleContainer)
{
    *pCanRecycleContainer = TRUE;
    return S_OK;
}

_Check_return_ IFACEMETHODIMP CalendarViewGeneratorHost::SuggestContainerForContainerFromItemLookup(
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    // CalendarViewGeneratorHost has no clue
    *ppContainer = nullptr;
    return S_OK;
}


CalendarViewGeneratorHost::CalendarViewGeneratorHost()
    : m_size(0)
    , m_pOwnerNoRef(nullptr)
{
    ResetScope();
}

CalendarViewGeneratorHost::~CalendarViewGeneratorHost()
{
    VERIFYHR(DetachScrollViewerFocusEngagedEvent());
    VERIFYHR(DetachVisibleIndicesUpdatedEvent());
    ctl::ComPtr<IModernCollectionBasePanel> panel;
    if (m_tpPanel.TryGetSafeReference(&panel))
    {
        VERIFYHR(panel.Cast<CalendarPanel>()->SetOwner(nullptr));
        VERIFYHR(panel.Cast<CalendarPanel>()->SetSnapPointFilterFunction(nullptr));
    }

    ctl::ComPtr<IScrollViewer> scrollviewer;
    if (m_tpScrollViewer.TryGetSafeReference(&scrollviewer))
    {
        VERIFYHR(scrollviewer.Cast<ScrollViewer>()->SetDirectManipulationStateChangeHandler(nullptr));
    }
}


wg::ICalendar* CalendarViewGeneratorHost::GetCalendar()
{
    return GetOwner()->GetCalendar();
}

void CalendarViewGeneratorHost::ResetScope()
{
    // when scope is enabled, the current scope means the current Month for monthview, current year for yearView and current decade for decadeview
    m_minDateOfCurrentScope.UniversalTime = 0;
    m_maxDateOfCurrentScope.UniversalTime = 0;
    m_pHeaderText.Release();
    m_lastVisibleIndicesPair[0] = -1;
    m_lastVisibleIndicesPair[1] = -1;
    m_lastVisitedDateAndIndex.first.UniversalTime = 0;
    m_lastVisitedDateAndIndex.second = -1;

}

xaml::Thickness CalendarViewGeneratorHost::GetItemMargin() const
{
    const xaml::Thickness squareItemMargin{ 1.0, 1.0, 1.0, 1.0 };
    const xaml::Thickness roundedItemMargin{ 9.0, 9.0, 9.0, 9.0 };

    // Starting with the 21H2 release, the items' margin is increased from 1 to 9px when rounded corners are applied.
    CCoreServices* core = static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle());
    const bool isRoundedCalendarViewBaseItemChromeEnabled = CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeEnabled(core);

    return isRoundedCalendarViewBaseItemChromeEnabled ? roundedItemMargin : squareItemMargin;
}

xaml::Thickness CalendarViewGeneratorHost::GetItemFocusVisualMargin() const
{
    const xaml::Thickness squareItemFocusVisualMargin{ -2.0, -2.0, -2.0, -2.0 };
    const xaml::Thickness roundedItemFocusVisualMargin{ -3.0, -3.0, -3.0, -3.0 };

    // Starting with the 21H2 release, the items' focus visual margin is increased from 2 to 3px when rounded corners are applied.
    CCoreServices* core = static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle());
    const bool isRoundedCalendarViewBaseItemChromeEnabled = CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeEnabled(core);

    return isRoundedCalendarViewBaseItemChromeEnabled ? roundedItemFocusVisualMargin : squareItemFocusVisualMargin;
}

// compute how many items we have in this view, basically the number of items equals to the index of max date + 1
_Check_return_ HRESULT CalendarViewGeneratorHost::ComputeSize()
{
    HRESULT hr = S_OK;
    int index = 0;

    m_lastVisitedDateAndIndex.first = GetOwner()->GetMinDate();
    m_lastVisitedDateAndIndex.second = 0;

    ASSERT(!GetOwner()->GetDateComparer()->LessThan(GetOwner()->GetMaxDate(), GetOwner()->GetMinDate()));

    IFC(CalculateOffsetFromMinDate(GetOwner()->GetMaxDate(), &index));

    m_size = static_cast<UINT>(index)+1;

Cleanup:
    return hr;
}

// Add scopes to the given date.
_Check_return_ HRESULT CalendarViewGeneratorHost::AddScopes(_Inout_ wf::DateTime& date, _In_ int scopes)
{
    HRESULT hr = S_OK;
    auto pCalendar = GetCalendar();

    IFC(pCalendar->SetDateTime(date));
    IFC(AddScopes(scopes));
    IFC(pCalendar->GetDateTime(&date));

    // We coerce and check if the date is in Calendar's limit where this gets called.

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarViewGeneratorHost::AddUnits(_Inout_ wf::DateTime& date, _In_ int units)
{
    auto pCalendar = GetCalendar();

    IFC_RETURN(pCalendar->SetDateTime(date));
    IFC_RETURN(AddUnits(units));
    IFC_RETURN(pCalendar->GetDateTime(&date));

    // We coerce and check if the date is in Calendar's limit where this gets called.

    return S_OK;
}

// AddDays/AddMonths/AddYears takes O(N) time but given that at most time we
// generate the items continuously so we can cache the result from last call and
// call AddUnits from the cache - this way N is small enough
// time cost: amortized O(1)

_Check_return_ HRESULT CalendarViewGeneratorHost::GetDateAt(_In_ UINT index, _Out_ wf::DateTime* pDate)
{
    ASSERT(m_lastVisitedDateAndIndex.second != -1);

    if (static_cast<int>(index) == m_lastVisitedDateAndIndex.second)
    {
        // Avoid calling AddUnits(0) and simply return the last visited date.
        pDate->UniversalTime = m_lastVisitedDateAndIndex.first.UniversalTime;
    }
    else
    {
        wf::DateTime date = {};
        auto pCalendar = GetCalendar();

        IFC_RETURN(pCalendar->SetDateTime(m_lastVisitedDateAndIndex.first));
        IFC_RETURN(AddUnits(static_cast<int>(index) - m_lastVisitedDateAndIndex.second));
        IFC_RETURN(pCalendar->GetDateTime(&date));
        m_lastVisitedDateAndIndex.first = date;
        m_lastVisitedDateAndIndex.second = static_cast<int>(index);
        pDate->UniversalTime = date.UniversalTime;
    }

    return S_OK;
}

// to get the distance of two days, here are the amortized O(1) method
//1. Estimate the offset of Date2 from Date1 by dividing their UTC difference by 24 hours
//2. Call Globalization API AddDays(Date1, offset) to get an estimated date, let's say EstimatedDate, here offset comes from step1
//3. Compute the distance between EstimatedDate and Date2(keep adding 1 day on the smaller one, until we hit the another date),
//   if this distance is still big, we can do step 1 and 2 one more time
//4. Return the sum of results from step1 and step3.

_Check_return_ HRESULT CalendarViewGeneratorHost::CalculateOffsetFromMinDate(_In_ wf::DateTime date, _Out_ int* pIndex)
{
    *pIndex = 0;
    wf::DateTime estimatedDate = { m_lastVisitedDateAndIndex.first.UniversalTime };
    auto pCalendar = GetCalendar();
    ASSERT(m_lastVisitedDateAndIndex.second != -1);

    int estimatedOffset = 0;
    INT64 diffInUTC = 0;
    int diffInUnit = 0;

    const int maxEstimationRetryCount = 3;  // the max times that we should estimate
    const int maxReboundCount = 3;          // the max times that we should reduce the step when the estimation is over the boundary.
    const int minDistanceToEstimate = 3;    // the min estimated distance that we should do estimation.

    IFC_RETURN(pCalendar->SetDateTime(estimatedDate));

    // step 1: estimation. mostly we only need to up to 2 times, but if we are targeting the calendar's boundaries
    // we could need more times (uncommon scenario)
    auto averageTicksPerUnit = GetAverageTicksPerUnit();
#ifdef DBG
    int estimationCount = 0;
#endif
    while (true)
    {
        diffInUTC = date.UniversalTime - estimatedDate.UniversalTime;

        // round to the nearest integer
        diffInUnit = static_cast<int>(diffInUTC / averageTicksPerUnit);

        if (std::abs(diffInUnit) < minDistanceToEstimate)
        {
            // if two dates are close enough, we can start to check if a correction is needed.
            break;
        }
#ifdef DBG
        if (estimationCount++ > maxEstimationRetryCount)
        {
            IGNOREHR(gps->DebugTrace(XCP_TRACE_WARNING, L"CalendarViewGeneartorHost::CalculateOffsetFromMinDate[0x%p]:  estimationCount = %d.", this, estimationCount));
            ASSERT(FALSE);
        }
#endif

        // when we are targeting the calendar's boundaries, it is possible the estimation will
        // cross the boundary, in this case we should reduce the length of step.
#ifdef DBG
        int retryCount = 0;
#endif
        while (true)
        {
            HRESULT hr = AddUnits(diffInUnit);

            if (SUCCEEDED(hr))
                break;
#ifdef DBG
            if (retryCount++ > maxReboundCount)
            {
                IGNOREHR(gps->DebugTrace(XCP_TRACE_WARNING, L"CalendarViewGeneartorHost::CalculateOffsetFromMinDate[0x%p]: over boundary, retryCount = %d.", this, retryCount));
                ASSERT(FALSE);
            }
#endif
            // we crossed the boundary! reduce the length and restart from estimatedDate
            //
            // mostly a bad estimation could happen on two dates that have a huge difference (e.g. jump to 100 years ago),
            // to fix the estimation we only need to slightly reduce the diff.

            IFC_RETURN(pCalendar->SetDateTime(estimatedDate));
            diffInUnit = diffInUnit * 99 / 100;
            ASSERT(diffInUnit != 0);
        } //while (true)

        estimatedOffset += diffInUnit;

        IFC_RETURN(pCalendar->GetDateTime(&estimatedDate));
    } //while (true)

    // step 2: after estimation, we'll check if a correction is needed or not.
    // this will be done in O(N) time but given that we have a good enough
    // estimation, here N will be very small (most likely <= 2)
    int offsetCorrection = 0;
    while (true)
    {
        int result = 0;
        int step = 1;
        IFC_RETURN(CompareDate(estimatedDate, date, &result));
        if (result == 0)
        {
            // end the loop when meeting the target date
            break;
        }
        else if (result > 0)
        {
            step = -1;
        }
        IFC_RETURN(AddUnits(step));
        offsetCorrection += step;
        IFC_RETURN(pCalendar->GetDateTime(&estimatedDate));
    }

    // base + estimatedDiff + correction
    *pIndex = m_lastVisitedDateAndIndex.second + estimatedOffset + offsetCorrection;

    return S_OK;
}

// return the first date of next scope.
// parameter dateOfFirstVisibleItem is the first visible item, it could be in
// current scope, or in previous scope.
_Check_return_ HRESULT CalendarViewGeneratorHost::GetFirstDateOfNextScope(
    _In_ wf::DateTime dateOfFirstVisibleItem,
    _In_ bool forward,
    _Out_ wf::DateTime* pFirstDateOfNextScope)
{
    HRESULT hr = S_OK;
    int adjustScopes = 0;
    wf::DateTime firstDateOfNextScope = {};

    // set to the first date of current scope
    IFC(GetCalendar()->SetDateTime(m_minDateOfCurrentScope));

    if (!GetOwner()->GetDateComparer()->LessThan(m_minDateOfCurrentScope, dateOfFirstVisibleItem))
    {
        // current scope starts from the first visible line
        // in this case, we simply jump to previous or next scope
        adjustScopes = forward ? 1 : -1;
    }
    else
    {
        // current scope starts before the first visible line,
        // so when we go backwards, we go to the beginning of this scope.
        // when go forwards, we still go to the next scope
        adjustScopes = forward ? 1 : 0;
    }

    if (adjustScopes != 0)
    {
        IFC(AddScopes(adjustScopes));

        int firstUnit = 0;
        GetFirstUnitInThisScope(&firstUnit);
        SetUnit(firstUnit);
    }

    IFC(GetCalendar()->GetDateTime(&firstDateOfNextScope));

    // when the navigation button is enabled, we should always be able to navigate to the desired scope.
    ASSERT(!GetOwner()->GetDateComparer()->LessThan(firstDateOfNextScope, GetOwner()->GetMinDate()));
    ASSERT(!GetOwner()->GetDateComparer()->LessThan(GetOwner()->GetMaxDate(), firstDateOfNextScope));

Cleanup:
    *pFirstDateOfNextScope = firstDateOfNextScope;
    return hr;
}


// Give a date range (it may contain multiple scopes, the scope is a month for MonthView),
// find the scope that has higher item coverage percentage, and use it as current scope.
_Check_return_ HRESULT CalendarViewGeneratorHost::UpdateScope(
    _In_ wf::DateTime firstDate,
    _In_ wf::DateTime lastDate,
    _Out_ bool* isScopeChanged)
{
    HRESULT hr = S_OK;
    wf::DateTime lastDateOfFirstScope;
    wf::DateTime minDateOfCurrentScope;
    wf::DateTime maxDateOfCurrentScope;
    int firstUnit = 0;
    int firstUnitOfFirstScope = 0;
    int lastUnitOfFirstScope = 0;

    *isScopeChanged = false;
    auto pCalendar = GetCalendar();

    ASSERT(!GetOwner()->GetDateComparer()->LessThan(lastDate, firstDate));

    IFC(pCalendar->SetDateTime(firstDate));
    IFC(GetUnit(&firstUnit));
    IFC(AdjustToLastUnitInThisScope(&lastDateOfFirstScope, &lastUnitOfFirstScope));

    if (!GetOwner()->GetDateComparer()->LessThan(lastDateOfFirstScope, lastDate))
    {
        // The given range has only one scope, so this is the current scope
        maxDateOfCurrentScope.UniversalTime = lastDateOfFirstScope.UniversalTime;
        IFC(AdjustToFirstUnitInThisScope(&minDateOfCurrentScope));
    }
    else
    {
        // The given range has more than one scopes, let's check the first one and second one.
        wf::DateTime lastDateOfSecondScope;
        int itemCountOfFirstScope = lastUnitOfFirstScope - firstUnit + 1;
        int itemCountOfSecondScope = 0;

        wf::DateTime dateToDetermineCurrentScope;   // we'll pick a date from first scope or second scope to determine the current scope.
        int firstUnitOfSecondScope = 0;
        int lastUnitOfSecondScope = 0;

        IFC(GetFirstUnitInThisScope(&firstUnitOfFirstScope));

        // We are on the last unit of first scope, add 1 unit will move to the second scope
        IFC(AddUnits(1));

        IFC(GetFirstUnitInThisScope(&firstUnitOfSecondScope));

        // Read the last date of second scope, check if it is inside the given range.
        IFC(AdjustToLastUnitInThisScope(&lastDateOfSecondScope, &lastUnitOfSecondScope));

        if (!GetOwner()->GetDateComparer()->LessThan(lastDate, lastDateOfSecondScope))
        {
            // The given range has the whole 2nd scope
            itemCountOfSecondScope = lastUnitOfSecondScope - firstUnitOfSecondScope + 1;
        }
        else
        {
            // The given range has only a part of the 2nd scope
            int lastUnit = 0;
            IFC(pCalendar->SetDateTime(lastDate));
            IFC(GetUnit(&lastUnit));
            itemCountOfSecondScope = lastUnit - firstUnitOfSecondScope + 1;
        }

        double firstScopePercentage = (double)itemCountOfFirstScope / (lastUnitOfFirstScope-firstUnitOfFirstScope+1);
        double secondScopePercentage = (double)itemCountOfSecondScope / (lastUnitOfSecondScope-firstUnitOfSecondScope+1);

        if (firstScopePercentage < secondScopePercentage)
        {
            // second scope wins
            dateToDetermineCurrentScope.UniversalTime = lastDateOfSecondScope.UniversalTime;
        }
        else
        {
            // first scope wins
            dateToDetermineCurrentScope.UniversalTime = firstDate.UniversalTime;
        }

        IFC(pCalendar->SetDateTime(dateToDetermineCurrentScope));
        IFC(AdjustToFirstUnitInThisScope(&minDateOfCurrentScope));
        IFC(AdjustToLastUnitInThisScope(&maxDateOfCurrentScope));
    }

    // in case we start from a day other than first day, we need to adjust the scope.
    // in case we end at a day other than the last day of this month, we need to adjust the scope.
    GetOwner()->CoerceDate(minDateOfCurrentScope);
    GetOwner()->CoerceDate(maxDateOfCurrentScope);

    if (minDateOfCurrentScope.UniversalTime != m_minDateOfCurrentScope.UniversalTime ||
        maxDateOfCurrentScope.UniversalTime != m_maxDateOfCurrentScope.UniversalTime)
    {
        m_minDateOfCurrentScope = minDateOfCurrentScope;
        m_maxDateOfCurrentScope = maxDateOfCurrentScope;
        *isScopeChanged = true;

        IFC(OnScopeChanged());
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarViewGeneratorHost::AdjustToFirstUnitInThisScope(_Out_ wf::DateTime* pDate, _Out_opt_ int* pUnit /* = nullptr */)
{
    int firstUnit = 0;

    if (pUnit)
    {
        *pUnit = 0;
    }
    pDate->UniversalTime = 0;

    IFC_RETURN(GetFirstUnitInThisScope(&firstUnit));
    IFC_RETURN(SetUnit(firstUnit));
    IFC_RETURN(GetCalendar()->GetDateTime(pDate));

    if (pUnit)
    {
        *pUnit = firstUnit;
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarViewGeneratorHost::AdjustToLastUnitInThisScope(_Out_ wf::DateTime* pDate, _Out_opt_ int* pUnit /* = nullptr */)
{
    int lastUnit = 0;

    if (pUnit)
    {
        *pUnit = 0;
    }
    pDate->UniversalTime = 0;

    IFC_RETURN(GetLastUnitInThisScope(&lastUnit));
    IFC_RETURN(SetUnit(lastUnit));
    IFC_RETURN(GetCalendar()->GetDateTime(pDate));

    if (pUnit)
    {
        *pUnit = lastUnit;
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarViewGeneratorHost::NotifyStateChange(
    _In_ DMManipulationState state,
    _In_ FLOAT xCumulativeTranslation,
    _In_ FLOAT yCumulativeTranslation,
    _In_ FLOAT zCumulativeFactor,
    _In_ FLOAT xCenter,
    _In_ FLOAT yCenter,
    _In_ BOOLEAN isInertial,
    _In_ BOOLEAN isTouchConfigurationActivated,
    _In_ BOOLEAN isBringIntoViewportConfigurationActivated)
{
    switch (state)
    {
        // we change items' scope state to InScope when DMManipulation is in progress to achieve better visual effect.
        // note we only change when there is an actual move (e.g. Manipulation Started, not Starting), because user
        // tapping to select an item also causes Manipulation starting, in this case we should not change scope state.
    case DirectUI::DMManipulationStarted:
        IFC_RETURN(GetOwner()->UpdateItemsScopeState(this,
            true, /*ignoreWhenIsOutOfScopeDisabled*/
            false /*ignoreInDirectManipulation*/));
        break;
    case DirectUI::DMManipulationCompleted:
        IFC_RETURN(GetOwner()->UpdateItemsScopeState(this,
            false, /*ignoreWhenIsOutOfScopeDisabled*/ // in case we changed IsOutOfScopeEnabled to false during DManipulation
            false /*ignoreInDirectManipulation*/));
        break;
    default:
        break;
    }
    return S_OK;
}

_Check_return_ HRESULT CalendarViewGeneratorHost::AttachVisibleIndicesUpdatedEvent()
{
    if (m_tpPanel)
    {
        IFC_RETURN(m_epVisibleIndicesUpdatedHandler.AttachEventHandler(m_tpPanel.Cast<CalendarPanel>(),
            [this](IInspectable* pSender, IInspectable* pArgs)
        {
            return GetOwner()->OnVisibleIndicesUpdated(this);
        }));
    }
    return S_OK;
}

_Check_return_ HRESULT CalendarViewGeneratorHost::DetachVisibleIndicesUpdatedEvent()
{
    return DetachHandler(m_epVisibleIndicesUpdatedHandler, m_tpPanel);
}

_Check_return_ HRESULT CalendarViewGeneratorHost::AttachScrollViewerFocusEngagedEvent()
{
    if (m_tpPanel)
    {
        ctl::ComPtr<DirectUI::ScrollViewer> sv(m_tpScrollViewer.Cast<DirectUI::ScrollViewer>());
        IFC_RETURN(m_epScrollViewerFocusEngagedEventHandler.AttachEventHandler(sv.AsOrNull<xaml_controls::IControl>().Get(),
            [this](_In_ xaml_controls::IControl* pSender,
                _In_ xaml_controls::IFocusEngagedEventArgs* pArgs)
        {
            return GetOwner()->OnScrollViewerFocusEngaged(pArgs);
        }));
    }
    return S_OK;
}

_Check_return_ HRESULT CalendarViewGeneratorHost::DetachScrollViewerFocusEngagedEvent()
{
    return DetachHandler(m_epScrollViewerFocusEngagedEventHandler, m_tpScrollViewer);
}

_Check_return_ HRESULT CalendarViewGeneratorHost::SetPanel(_In_opt_ xaml_primitives::ICalendarPanel* pPanel)
{
    if (pPanel)
    {
        SetPtrValue(m_tpPanel, pPanel);
        IFC_RETURN(m_tpPanel.Cast<CalendarPanel>()->SetOwner(this));
    }
    else if (m_tpPanel)
    {
        IFC_RETURN(m_tpPanel.Cast<CalendarPanel>()->SetOwner(nullptr));
        m_tpPanel.Clear();
    }
    return S_OK;
}


_Check_return_ HRESULT CalendarViewGeneratorHost::SetScrollViewer(_In_opt_ xaml_controls::IScrollViewer* pScrollViewer)
{
    if (pScrollViewer)
    {
        SetPtrValue(m_tpScrollViewer, pScrollViewer);
    }
    else
    {
        m_tpScrollViewer.Clear();
    }
    return S_OK;
}


CalendarPanel* CalendarViewGeneratorHost::GetPanel()
{
    return m_tpPanel.Cast<CalendarPanel>();
}

ScrollViewer* CalendarViewGeneratorHost::GetScrollViewer()
{
    return m_tpScrollViewer.Cast<ScrollViewer>();
}

_Check_return_ HRESULT CalendarViewGeneratorHost::OnPrimaryPanelDesiredSizeChanged()
{
    return GetOwner()->OnPrimaryPanelDesiredSizeChanged(this);
}
