// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A collection view implementation over an IVector<IInspectable *>

#pragma once

#include "VectorCollectionView.g.h"

namespace DirectUI
{
    class CollectionViewSource;

    // Plays WPF's ListCollectionView: a view a CollectionViewSource creates over a vector owns the shaping
    // engine (VectorCollectionView_Shaping.cpp) and answers ICollectionViewShaping. A dedicated class would be a
    // new DependencyObject type (KnownTypeIndex, metadata tables), so the existing view class carries it.
    PARTIAL_CLASS(VectorCollectionView),
        public xaml_data::ICollectionViewShaping
    {
    protected:

        VectorCollectionView();
        ~VectorCollectionView() override;

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    public:

        // ICollectionViewShaping
        IFACEMETHOD(get_SortDescriptions)(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue) override;
        IFACEMETHOD(get_GroupDescriptions)(_Outptr_ wfc::IObservableVector<xaml_data::PropertyGroupDescription*>** ppValue) override;
        IFACEMETHOD(get_Filter)(_Outptr_result_maybenull_ xaml_data::ICollectionViewFilter** ppValue) override;
        IFACEMETHOD(put_Filter)(_In_opt_ xaml_data::ICollectionViewFilter* pValue) override;
        IFACEMETHOD(Refresh)() override;
        IFACEMETHOD(DeferRefresh)(_Outptr_ wf::IClosable** ppReturnValue) override;
        IFACEMETHOD(get_CustomSort)(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue) override;
        IFACEMETHOD(put_CustomSort)(_In_opt_ xaml_data::ICollectionViewComparer* pValue) override;

        _Check_return_ HRESULT get_CollectionGroupsImpl(_Out_ wfc::IObservableVector<IInspectable *> **value) override;

        // Shaping engine (VectorCollectionView_Shaping.cpp). EnableShaping makes the view answer
        // ICollectionViewShaping; pHost is the CollectionViewSource that publishes it as View.
        _Check_return_ HRESULT EnableShaping(_In_ CollectionViewSource* pHost);

        // WPF: CollectionViewSource.ApplyPropertiesToView. The collections are shared with the host.
        _Check_return_ HRESULT ApplyShapingRules(
            _In_ wfc::IVector<xaml_data::SortDescription*>* pSortDescriptions,
            _In_ wfc::IVector<xaml_data::PropertyGroupDescription*>* pGroupDescriptions,
            _In_opt_ xaml_data::ICollectionViewFilter* pFilter,
            _In_opt_ xaml_data::ICollectionViewComparer* pCustomSort);

        // The host replaced this view: stop following the source; ICollectionViewShaping goes to the host.
        void DetachShaping();

        _Check_return_ HRESULT OnSortDescriptionsChanged();
        _Check_return_ HRESULT OnShapingRulesChanged();
        _Check_return_ HRESULT EndDeferRefresh();

        // IVector<IInspectable *>
        IFACEMETHOD(GetAt)(unsigned index, _Out_  IInspectable **item) override;

        IFACEMETHOD(get_Size)(_Out_ unsigned *size) override;

        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable *>** view) override;

        IFACEMETHOD(IndexOf)(_In_opt_ IInspectable * value, _Out_ unsigned *index, _Out_ BOOLEAN *found) override;

        IFACEMETHOD(SetAt)(unsigned index, _In_opt_ IInspectable *item) override;

        IFACEMETHOD(InsertAt)(unsigned index, _In_ IInspectable *item) override;

        IFACEMETHOD(RemoveAt)(unsigned index) override;

        IFACEMETHOD(Append)(_In_opt_ IInspectable * item) override;

        IFACEMETHOD(RemoveAtEnd)() override;

        IFACEMETHOD(Clear)() override;

        // IIterable<IInspectable *>
        IFACEMETHOD(First)(_Outptr_ wfc::IIterator<IInspectable *> **value) override;

        // ISupportIncrementalLoading portion of ICollectionView
        _Check_return_ HRESULT get_HasMoreItemsImpl(_Out_ BOOLEAN *value) override;
        _Check_return_ HRESULT LoadMoreItemsAsyncImpl(
            _In_ UINT32 count,
            _Outptr_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult> **operation) override;

    public:

        // Internal only factory method to create a list collection view
        static _Check_return_ HRESULT CreateInstance(
            _In_ wfc::IVector<IInspectable *> *pSource,
            _Outptr_ xaml_data::ICollectionView  **instance);

        // Points the view at another vector and announces the swap as a Reset.
        _Check_return_ HRESULT RetargetSource(_In_ wfc::IVector<IInspectable *> *pSource);

    protected:

        _Check_return_ HRESULT SetSource(_In_ wfc::IVector<IInspectable *> *pSource);

    private:

        _Check_return_ HRESULT OnSourceVectorChanged(
            _In_ wfc::IObservableVector<IInspectable *> *pSender,
            _In_ IInspectable *pArgs);

        _Check_return_ HRESULT GetDetachedHost(_Outptr_result_maybenull_ CollectionViewSource** ppHost);
        _Check_return_ HRESULT HasShapingRules(_Out_ bool* pHasRules);
        _Check_return_ HRESULT StartShaping(bool isGrouping);
        _Check_return_ HRESULT RefreshOverride();
        _Check_return_ HRESULT RefreshOrDefer();
        _Check_return_ HRESULT ProcessCollectionChanged(_In_ wfc::IVectorChangedEventArgs* pArgs);
        _Check_return_ HRESULT ProcessCollectionChangedWithAdjustedIndex(wfc::CollectionChange change, UINT index, _Out_ bool* pIsApplied);
        void DetachSourceList();

    private:

        // What the view presents: the source itself, or once shaping starts the InternalList (flat) or
        // a GroupedDataCollectionView over it (grouped).
        TrackerPtr<wfc::IVector<IInspectable *>> m_tpSource;
        TrackerPtr<xaml_data::ISupportIncrementalLoading> m_tpSupportIncrementalLoading;
        ctl::EventPtr<VectorChangedEventCallback> m_epVectorChangedHandler;

        bool m_isShapingEnabled = false;
        bool m_isShapingDetached = false;
        ctl::WeakRefPtr m_hostRef;

        TrackerPtr<wfc::IVector<xaml_data::SortDescription*>> m_tpSortDescriptions;
        TrackerPtr<wfc::IVector<xaml_data::PropertyGroupDescription*>> m_tpGroupDescriptions;
        TrackerPtr<xaml_data::ICollectionViewFilter> m_tpFilter;
        TrackerPtr<xaml_data::ICollectionViewComparer> m_tpCustomSort;
        bool m_isApplyingCustomSort = false;

        // The collection the view shapes; followed only while shaping (non-null m_tpInternalList).
        TrackerPtr<wfc::IVector<IInspectable *>> m_tpSourceList;
        TrackerPtr<wfc::IObservableVector<IInspectable *>> m_tpSourceListObservable;
        ctl::EventPtr<VectorChangedEventCallback> m_epSourceListChangedHandler;
        TrackerPtr<wfc::IVector<IInspectable *>> m_tpInternalList;
        TrackerPtr<xaml_data::ICollectionView> m_tpGroupedPresentation;
        bool m_isGrouping = false;
        bool m_isComputingShape = false;

        // Flat output only: the source index of each output item, so a source Add, Remove or
        // Replace updates the view incrementally. Invalid after a change that was not applied.
        std::vector<UINT> m_sourceIndices;
        bool m_areSourceIndicesValid = false;
        UINT m_sourceListSize = 0;
        UINT m_sourceListChangeCount = 0;

        // Open DeferRefresh deferrals; a change made while any is open recomputes once, when the last closes.
        UINT m_deferRefreshLevel = 0;
        bool m_isRefreshPending = false;
    };
}
