// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GroupDescription.g.h"

namespace DirectUI
{
    class VectorCollectionView;

    // WPF's GroupDescription. The view calls GroupNameFromItemCore and NamesMatchCore through the overrides interface, so an
    // app's derived class decides the groups; PropertyGroupDescription answers them natively.
    PARTIAL_CLASS(GroupDescription)
    {
    public:
        _Check_return_ HRESULT GetDefaultValue2(_In_ const CDependencyProperty* pDP, _Out_ CValue* pValue) override;

        _Check_return_ HRESULT get_GroupNamesImpl(_Outptr_ wfc::IObservableVector<IInspectable*>** ppValue);
        _Check_return_ HRESULT get_SortDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue);
        _Check_return_ HRESULT get_CustomSortImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue);
        _Check_return_ HRESULT put_CustomSortImpl(_In_opt_ xaml_data::ICollectionViewComparer* pValue);

        // Public, as in WPF: they call the overridables, so any view can group by a description.
        _Check_return_ HRESULT GroupNameFromItemImpl(_In_opt_ IInspectable* pItem, INT level, _In_opt_ HSTRING culture, _Outptr_result_maybenull_ IInspectable** ppResult);
        _Check_return_ HRESULT NamesMatchImpl(_In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ BOOLEAN* pResult);

        // WPF: GroupNameFromItem is abstract; a GroupDescription that does not override it puts every item in one
        // group named null.
        virtual _Check_return_ HRESULT GroupNameFromItemCoreImpl(_In_opt_ IInspectable* pItem, INT level, _In_opt_ HSTRING culture, _Outptr_result_maybenull_ IInspectable** ppResult);

        // WPF: Object.Equals(groupName, itemName).
        virtual _Check_return_ HRESULT NamesMatchCoreImpl(_In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ BOOLEAN* pResult);

        wfc::IVector<xaml_data::SortDescription*>* GetSortDescriptionsNoRef() const
        {
            return m_tpSortDescriptions.Get();
        }

        wfc::IVector<IInspectable*>* GetGroupNamesNoRef() const
        {
            return m_tpGroupNames.Get();
        }

        // A view grouping by this description regroups when it changes (WPF: GroupDescription.PropertyChanged).
        _Check_return_ HRESULT AddOwner(_In_ VectorCollectionView* pView);
        _Check_return_ HRESULT OnSortDescriptionsChanged();
        _Check_return_ HRESULT NotifyOwners();

    private:
        TrackerPtr<wfc::IVector<xaml_data::SortDescription*>> m_tpSortDescriptions;
        TrackerPtr<wfc::IVector<IInspectable*>> m_tpGroupNames;
        TrackerPtr<xaml_data::ICollectionViewComparer> m_tpCustomSort;
        std::vector<ctl::WeakRefPtr> m_owners;
        bool m_isApplyingCustomSort = false;
    };
}
