// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      The definition of GroupedDataCollectionView that allows grouped
//      data to be consumed by DXaml

#pragma once

#include "GroupedDataCollectionView.g.h"
#include "ReadOnlyCollectionView.h"

namespace DirectUI
{
    class PropertyPathListener;

    // This class will not do any kind of virtualization of the
    // collection group data
    class __declspec(uuid(__GroupedDataCollectionView_GUID)) GroupedDataCollectionView:
        public ReadOnlyCollectionView<GroupedDataCollectionViewGenerated>,
        public IPropertyPathListenerHost
    {
    protected:

        GroupedDataCollectionView();
        ~GroupedDataCollectionView() override;

    public:

        // IVector<IInspectable *>
        IFACEMETHOD(GetAt)(_In_opt_ UINT index, _Out_  IInspectable **item) override;

        IFACEMETHOD(get_Size)(_Out_ UINT *size) override;

        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable *>** view) override;

        IFACEMETHOD(IndexOf)(_In_opt_ IInspectable * value, _Out_ UINT *index, _Out_ BOOLEAN *found) override;

        // IIterable<IInspectable *>
        IFACEMETHOD(First)(_Outptr_ wfc::IIterator<IInspectable *> **value) override;

        // ICollectionView overrides
        _Check_return_ HRESULT get_CollectionGroupsImpl(_Out_ wfc::IObservableVector<IInspectable *> **value) override;

    public:

        static _Check_return_ HRESULT CreateInstance(
            _In_ wfc::IVector<IInspectable *> *pSource,
            _In_ xaml::IPropertyPath *pItemsPath,
            _Outptr_ ICollectionView **instance);

    public:

        // IPropertyPathListenerHost
        _Check_return_ HRESULT GetTraceString(_Outptr_result_z_ const WCHAR **pszTraceString) override;
        _Check_return_ HRESULT SourceChanged() override;

        _Check_return_ HRESULT OnGroupItemsChanged(
            _In_ xaml_data::ICollectionViewGroup *pGroup,
            _In_ wfc::IVectorChangedEventArgs *pArgs);

    protected:

        // Note: This implementation assumes that there's no virtualization of groups only of items
        // and thus we can always get a full vector of all of the groups
        _Check_return_ HRESULT SetSource(
            _In_ wfc::IVector<IInspectable *> *pSource,
            _In_ xaml::IPropertyPath *pItemsPath);

    private:

        _Check_return_ HRESULT CalculateCount(_Out_ UINT *count);

        _Check_return_ HRESULT CreatePropertyPathListener(_In_ xaml::IPropertyPath *pItemsPath);

        _Check_return_ HRESULT CalculateGroups();
        _Check_return_ HRESULT CalculateCollectionViewGroup(_In_ IInspectable *pGroup, _Outptr_ xaml_data::ICollectionViewGroup **ppCVG);

        _Check_return_ HRESULT OnGroupsChange(_In_ wfc::IVectorChangedEventArgs *pArgs);

        _Check_return_ HRESULT NotifyOfItemChange(
            _In_ UINT nIndex,
            _In_ wfc::CollectionChange change);

        _Check_return_ HRESULT NotifyOfGroupChange(
            _In_ UINT nIndex,
            _In_ wfc::CollectionChange change,
            _In_ xaml_data::ICollectionViewGroup *pCVG = NULL);

        _Check_return_ HRESULT AdjustItemsVector(
            _In_ UINT nIndex,
            wfc::CollectionChange change,
            _In_ IInspectable *pNewItem);

        _Check_return_ HRESULT GetBaseIndexOfGroup(_In_ UINT nGroupIndex, _Out_ UINT *pnGroupBaseIndex);

        _Check_return_ HRESULT GetGroupItems(
            _In_ xaml_data::ICollectionViewGroup *pGroup,
            _Outptr_ wfc::IVector<IInspectable *> **ppItems);

        _Check_return_ HRESULT GetGroupItem(
            _In_ xaml_data::ICollectionViewGroup *pGroup,
            _In_ UINT nIndex,
            _Outptr_ IInspectable **ppItem);

        _Check_return_ HRESULT GetGroupAt(UINT nIndex, _Outptr_ xaml_data::ICollectionViewGroup **ppGroup);

        _Check_return_ HRESULT GetGroupSize(
            _In_ xaml_data::ICollectionViewGroup *pGroup,
            _Out_ UINT *pnSize);

        void ClearGroupsOwner();

    private:

        static const UINT cNumberOfItemsNotCached = static_cast<UINT>(-1);
        UINT m_nNumberOfItemsInAllGroups;

        TrackerPtr<wfc::IVector<IInspectable *>> m_tpSource;
        TrackerPtr<wfc::IObservableVector<IInspectable *>> m_tpObservable;
        TrackerPtr<wfc::IObservableVector<IInspectable *>> m_tpCollectionGroups;

        TrackerPtr<PropertyPathListener> m_tpListener;

        ctl::EventPtr<VectorChangedEventCallback> m_epVectorChangedHandler;
    };
}
