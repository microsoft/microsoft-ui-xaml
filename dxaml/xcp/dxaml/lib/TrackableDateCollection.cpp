// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TrackableDateCollection.h"
#include "CalendarView_Partial.h"
#include "DateComparer.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

TrackableDateCollection::TrackableDateCollection()
    : m_dateComparer(new DateComparer())
    , m_lessThanComparer(m_dateComparer->GetLessThanComparer())
    , m_areEquivalentComparer(m_dateComparer->GetAreEquivalentComparer())
    , m_addedDates(m_lessThanComparer)
    , m_removedDates(m_lessThanComparer)
{
}

_Check_return_ HRESULT TrackableDateCollection::SetCalendarForComparison(_In_ wg::ICalendar* pCalendar)
{
    // addedDates and removedDates must be empty while we could change the comparison function.
    ASSERT(m_addedDates.empty() && m_removedDates.empty());

    IFC_RETURN(m_dateComparer->SetCalendarForComparison(pCalendar));

    return S_OK;
}


void TrackableDateCollection::FetchAndResetChange(
    _In_ DateSetType& addedDates,
    _In_ DateSetType& removedDates)
{

#ifdef DBG
    {
        // currently there is no case could cause addedDates and removedDates overlap.
        // just for sure to double check

        for (auto it = m_addedDates.begin(); it != m_addedDates.end(); ++it)
        {
            ASSERT(!std::binary_search(m_removedDates.begin(), m_removedDates.end(), *it, m_lessThanComparer));
        }
    }
#endif

    addedDates.swap(m_addedDates);
    m_addedDates.clear();

    removedDates.swap(m_removedDates);
    m_removedDates.clear();
}

IFACEMETHODIMP TrackableDateCollection::RemoveAt(_In_ UINT index)
{
    IFC_RETURN(RaiseCollectionChanging(TrackableDateCollection_CollectionChanging::ItemRemoving, wf::DateTime()));

    wf::DateTime date;

    IFC_RETURN(GetAt(index, &date));
    m_removedDates.emplace(date);

    IFC_RETURN(ValueTypeObservableCollection<wf::DateTime>::RemoveAt(index));

    return S_OK;
}

IFACEMETHODIMP TrackableDateCollection::Clear()
{
    IFC_RETURN(RaiseCollectionChanging(TrackableDateCollection_CollectionChanging::Resetting, wf::DateTime()));

    unsigned size = 0;

    IFC_RETURN(get_Size(&size));

    for (unsigned i = 0; i < size; ++i)
    {
        wf::DateTime date;

        IFC_RETURN(GetAt(i, &date));
        m_removedDates.emplace(date);
    }

    IFC_RETURN(ValueTypeObservableCollection<wf::DateTime>::Clear());

    return S_OK;

}

IFACEMETHODIMP TrackableDateCollection::Append(_In_opt_  wf::DateTime item)
{
    IFC_RETURN(RaiseCollectionChanging(TrackableDateCollection_CollectionChanging::ItemInserting, item));

    m_addedDates.emplace(item);

    IFC_RETURN(ValueTypeObservableCollection<wf::DateTime>::Append(item));
    return S_OK;
}

IFACEMETHODIMP TrackableDateCollection::SetAt(_In_ UINT index, _In_opt_ wf::DateTime item)
{
    IFC_RETURN(RaiseCollectionChanging(TrackableDateCollection_CollectionChanging::ItemChanging, item));

    wf::DateTime date;

    IFC_RETURN(GetAt(index, &date));

    // here is the only possible case could cause addedDates and removedDates overlap
    // e.g. replace a date by the same date.
    // in this case we just don't record the changes hence we don't raise SelectedDatesChangedEvent

    if (!m_areEquivalentComparer(date, item))
    {
        m_addedDates.emplace(item);
        m_removedDates.emplace(date);
    }
    
    IFC_RETURN(ValueTypeObservableCollection<wf::DateTime>::SetAt(index, item));

    return S_OK;
}

IFACEMETHODIMP TrackableDateCollection::InsertAt(_In_ UINT index, _In_opt_ wf::DateTime item)
{
    IFC_RETURN(RaiseCollectionChanging(TrackableDateCollection_CollectionChanging::ItemInserting, item));

    m_addedDates.emplace(item);

    IFC_RETURN(ValueTypeObservableCollection<wf::DateTime>::InsertAt(index, item));

    return S_OK;
}

IFACEMETHODIMP TrackableDateCollection::IndexOf(_In_opt_ wf::DateTime value, _Out_ UINT *index, _Out_ BOOLEAN *found)
{
    *index = 0;
    *found = FALSE;

    IFC_RETURN(CheckThread());

    for (unsigned i = 0; i < m_vector.size(); ++i)
    {
        if (m_areEquivalentComparer(m_vector[i], value))
        {
            *index = i;
            *found = TRUE;
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT TrackableDateCollection::CountOf(_In_ wf::DateTime value, _Out_ unsigned* pCount)
{
    unsigned count = 0;
    *pCount = 0;

    for (unsigned i = 0; i < m_vector.size(); ++i)
    {
        if (m_areEquivalentComparer(m_vector[i], value))
        {
            count++;
        }
    }

    *pCount = count;
    return S_OK;
}

_Check_return_ HRESULT TrackableDateCollection::RemoveAll(_In_ wf::DateTime value, _In_opt_ unsigned* pFromHint/* = nullptr*/)
{
    int from = static_cast<int>(pFromHint ? *pFromHint : 0);
    int i = static_cast<int>(m_vector.size()) - 1;

    for (; i >= from; --i)
    {
        if (m_areEquivalentComparer(m_vector[i], value))
        {
            IFC_RETURN(RemoveAt(i));
        }
    }

    return S_OK;    
}

_Check_return_ HRESULT TrackableDateCollection::RaiseCollectionChanging(_In_ TrackableDateCollection_CollectionChanging action, _In_ wf::DateTime addingDate)
{
    if (m_collectionChanging)
    {
        IFC_RETURN(m_collectionChanging(action, addingDate));
    }
    return S_OK;
}

