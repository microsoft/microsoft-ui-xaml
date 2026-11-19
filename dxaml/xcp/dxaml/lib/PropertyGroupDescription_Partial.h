// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PropertyGroupDescription.g.h"

namespace DirectUI
{
    class VectorCollectionView;

    PARTIAL_CLASS(PropertyGroupDescription)
    {
    public:
        _Check_return_ HRESULT get_SortDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue);
        _Check_return_ HRESULT get_CustomSortImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue);
        _Check_return_ HRESULT put_CustomSortImpl(_In_opt_ xaml_data::ICollectionViewComparer* pValue);

        // Null until SortDescriptions is first read.
        wfc::IVector<xaml_data::SortDescription*>* GetSortDescriptionsNoRef() const
        {
            return m_tpSortDescriptions.Get();
        }

        // A view grouping by this description regroups when the group order changes (WPF: GroupDescription.PropertyChanged).
        _Check_return_ HRESULT AddOwner(_In_ VectorCollectionView* pView);
        _Check_return_ HRESULT OnSortDescriptionsChanged();

    private:
        _Check_return_ HRESULT NotifyOwners();

        TrackerPtr<wfc::IVector<xaml_data::SortDescription*>> m_tpSortDescriptions;
        TrackerPtr<xaml_data::ICollectionViewComparer> m_tpCustomSort;
        std::vector<ctl::WeakRefPtr> m_owners;
        bool m_isApplyingCustomSort = false;
    };
}
