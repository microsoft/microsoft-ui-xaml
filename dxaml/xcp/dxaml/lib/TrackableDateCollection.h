// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "JoltCollections.h"

namespace DirectUI
{

    class DateComparer;

    // keep this outside of class TrackableDateCollection so other class can easily
    // forward declare this enum class. (so far we can't forward declar a nested type)

    enum class TrackableDateCollection_CollectionChanging
    {
        Resetting = 0,
        ItemInserting = 1,
        ItemRemoving = 2,
        ItemChanging = 3
    };

    class TrackableDateCollection : public ValueTypeObservableCollection<wf::DateTime>
    {
    public:

        typedef std::set<wf::DateTime, std::function<bool(wf::DateTime, wf::DateTime)>> DateSetType;

        TrackableDateCollection();

        // grab the changes since last time we call this, and reset the changes.
        void FetchAndResetChange(
            _In_ DateSetType& addedDates,
            _In_ DateSetType& removedDates);

        _Check_return_ HRESULT SetCalendarForComparison(_In_ wg::ICalendar* pCalendar);

        _Check_return_ HRESULT CountOf(_In_ wf::DateTime value, _Out_ unsigned* pCount);

        _Check_return_ HRESULT RemoveAll(_In_ wf::DateTime value, _In_opt_ unsigned* pFromHint = nullptr);

        typedef std::function<HRESULT(_In_ TrackableDateCollection_CollectionChanging action, _In_ wf::DateTime addingDate)> CollectionChangingCallback;

        void SetCollectionChangingCallback(_In_ CollectionChangingCallback callback)
        {
            m_collectionChanging = callback;
        }

    public:
        IFACEMETHOD(RemoveAt)(_In_ UINT index) override;

        IFACEMETHOD(Clear)() override;


        IFACEMETHOD(Append)(_In_opt_  wf::DateTime item) override;

        IFACEMETHOD(InsertAt)(_In_ UINT index, _In_opt_ wf::DateTime item) override;

        IFACEMETHOD(SetAt)(_In_ UINT index, _In_opt_ wf::DateTime item) override;

        IFACEMETHOD(IndexOf)(_In_opt_ wf::DateTime value, _Out_ UINT *index, _Out_ BOOLEAN *found) override;

    private:
        _Check_return_ HRESULT RaiseCollectionChanging(_In_ TrackableDateCollection_CollectionChanging action, _In_ wf::DateTime addingDate);

    private:
        std::unique_ptr<DateComparer> m_dateComparer;
        std::function<bool(wf::DateTime, wf::DateTime)> m_lessThanComparer;
        std::function<bool(wf::DateTime, wf::DateTime)> m_areEquivalentComparer;
        DateSetType m_addedDates;
        DateSetType m_removedDates;
        CollectionChangingCallback m_collectionChanging;
    };
}


