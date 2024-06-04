// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarView.g.h"
#include "CalendarViewGeneratorHost.h"
#include "CalendarViewGeneratorMonthViewHost.h"
#include "CalendarPanel.g.h"
#include "CalendarLayoutStrategy.g.h"
#include "CalendarLayoutStrategyImpl.h"

using namespace DirectUI;
using namespace DirectUI::Components::Moco;

_Check_return_ HRESULT CalendarView::InitializeIndexCorrectionTableIfNeeded()
{
    auto pMonthPanel = m_tpMonthViewItemHost->GetPanel();
    if (pMonthPanel)
    {
        ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;
        unsigned indexOfSkippedDay = 0;
        unsigned correctionForSkippedDay = 0;

        if (m_tpMonthViewItemHost.Cast<CalendarViewGeneratorMonthViewHost>()->IsUsingSamoaTimeZone(true /*forceUpdate*/))
        {
            // Fix for https://task.ms/4074103 (CalendarView: 1/1/2012 is skipped in Samoa timezone due to the TimeZone changing from UTC-11:00 to UTC+13:00 at the end of 12/31/2011)
            // The code below will find the index of the skipped day in Samoa time, and shift all the days after this index (inclusive) by one cell to make sure
            // the days after this index have correct dayofweek.
            // we use layout strategy to push the days to the right, see details from
            // \dxaml\xcp\components\moco\inc\CalendarLayoutStrategyImpl.h

            if (SUCCEEDED(m_tpCalendar->put_Year(2012)) // in some calendar systems, this date doesn't exist, we should not fail on these calendar systems.
                && SUCCEEDED(m_tpCalendar->put_Month(1))
                && SUCCEEDED(m_tpCalendar->put_Day(2)))
            {
                wf::DateTime date;
                int index = -1;

                IFC_RETURN(m_tpCalendar->GetDateTime(&date));
                IFC_RETURN(m_tpMonthViewItemHost->CalculateOffsetFromMinDate(date, &index));
                if (index > 0)
                {
                    indexOfSkippedDay = static_cast<unsigned>(index);
                    correctionForSkippedDay = 1;
                }
            }
        }

        IFC_RETURN(pMonthPanel->GetLayoutStrategy(&spCalendarLayoutStrategy));

        auto& table = spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->GetIndexCorrectionTable();

        table.SetCorrectionEntryForSkippedDay(indexOfSkippedDay, correctionForSkippedDay);
    }
    return S_OK;
}