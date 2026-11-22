// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A collection view implementation over an IVector<IInspectable *>

#pragma once

#include "VectorCollectionView.g.h"

namespace DirectUI
{
    class CollectionViewSource;
    class PropertyGroupDescription;
    class VectorCollectionView;

    // Drops a default view from CollectionViewSource.GetDefaultView's table (CollectionViewSource_Shaping.cpp).
    void UnregisterDefaultView(_In_ VectorCollectionView* pView);

    // Plays WPF's ListCollectionView: a view a CollectionViewSource creates over a vector owns the shaping
    // engine (VectorCollectionView_Shaping.cpp) and answers ICollectionViewShaping. A dedicated class would be a
    // new DependencyObject type (KnownTypeIndex, metadata tables), so the existing view class carries it.
    PARTIAL_CLASS(VectorCollectionView),
        public xaml_data::ICollectionViewShaping,
        public xaml_data::ICollectionViewLiveShaping
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
        IFACEMETHOD(get_SourceCollection)(_Outptr_result_maybenull_ IInspectable** ppValue) override;
        IFACEMETHOD(get_Culture)(_Out_ HSTRING* pValue) override;
        IFACEMETHOD(put_Culture)(_In_opt_ HSTRING value) override;

        // ICollectionViewLiveShaping (WPF: ListCollectionView live shaping)
        IFACEMETHOD(get_CanChangeLiveSorting)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_CanChangeLiveFiltering)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_CanChangeLiveGrouping)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_IsLiveSorting)(_Outptr_result_maybenull_ wf::IReference<bool>** ppValue) override;
        IFACEMETHOD(put_IsLiveSorting)(_In_opt_ wf::IReference<bool>* pValue) override;
        IFACEMETHOD(get_IsLiveFiltering)(_Outptr_result_maybenull_ wf::IReference<bool>** ppValue) override;
        IFACEMETHOD(put_IsLiveFiltering)(_In_opt_ wf::IReference<bool>* pValue) override;
        IFACEMETHOD(get_IsLiveGrouping)(_Outptr_result_maybenull_ wf::IReference<bool>** ppValue) override;
        IFACEMETHOD(put_IsLiveGrouping)(_In_opt_ wf::IReference<bool>* pValue) override;
        IFACEMETHOD(get_LiveSortingProperties)(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue) override;
        IFACEMETHOD(get_LiveFilteringProperties)(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue) override;
        IFACEMETHOD(get_LiveGroupingProperties)(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue) override;

        // WPF: the live shaping part of CollectionViewSource.ApplyPropertiesToView. A property list is copied
        // only when its feature is requested.
        _Check_return_ HRESULT ApplyLiveShaping(
            bool isLiveSorting,
            bool isLiveFiltering,
            bool isLiveGrouping,
            _In_opt_ wfc::IObservableVector<HSTRING>* pSortingProperties,
            _In_opt_ wfc::IObservableVector<HSTRING>* pFilteringProperties,
            _In_opt_ wfc::IObservableVector<HSTRING>* pGroupingProperties);

        _Check_return_ HRESULT get_CollectionGroupsImpl(_Out_ wfc::IObservableVector<IInspectable *> **value) override;

        // Shaping engine (VectorCollectionView_Shaping.cpp). EnableShaping makes the view answer
        // ICollectionViewShaping; pHost is the CollectionViewSource that publishes it as View.
        _Check_return_ HRESULT EnableShaping(_In_ CollectionViewSource* pHost);

        // A default view (CollectionViewSource.GetDefaultView): shapes with its own rules, no host.
        _Check_return_ HRESULT EnableDefaultViewShaping();

        // WPF: ICollectionView.SourceCollection.
        void SetSourceCollection(_In_opt_ IInspectable* pSource);

        // A Culture is a BCP-47 language tag; empty means the user's locale.
        static _Check_return_ HRESULT ValidateCulture(_In_opt_ HSTRING value);

        // WPF: CollectionViewSource.ApplyPropertiesToView. The collections are shared with the host.
        _Check_return_ HRESULT ApplyShapingRules(
            _In_ wfc::IVector<xaml_data::SortDescription*>* pSortDescriptions,
            _In_ wfc::IVector<xaml_data::PropertyGroupDescription*>* pGroupDescriptions,
            _In_opt_ xaml_data::ICollectionViewFilter* pFilter,
            _In_opt_ xaml_data::ICollectionViewComparer* pCustomSort,
            _In_opt_ HSTRING culture);

        // The host replaced this view. As in WPF, where each view keeps the rules copied into it, the view
        // keeps following its source with its own copy of the rules; nothing it is told reaches the host.
        _Check_return_ HRESULT DetachFromHost();

        // While the host has FilterRequested handlers the view holds it, as WPF's view holds the CVS through
        // the wrapped filter; else only weakly.
        void SetFilterHost(_In_opt_ CollectionViewSource* pHost);

        _Check_return_ HRESULT OnSortDescriptionsChanged();
        _Check_return_ HRESULT OnShapingRulesChanged();
        _Check_return_ HRESULT EndDeferRefresh();

        // A group description's SortDescriptions or CustomSort changed.
        _Check_return_ HRESULT OnGroupDescriptionChanged(_In_ PropertyGroupDescription* pDescription);

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

        // Points the view at another vector and announces the swap as a Reset, after PropertyChanged for
        // CollectionGroups when the swap starts or stops grouping.
        _Check_return_ HRESULT RetargetSource(_In_ wfc::IVector<IInspectable *> *pSource, bool isGroupingChanged = false);

    protected:

        _Check_return_ HRESULT SetSource(_In_ wfc::IVector<IInspectable *> *pSource);

    private:

        _Check_return_ HRESULT OnSourceVectorChanged(
            _In_ wfc::IObservableVector<IInspectable *> *pSender,
            _In_ IInspectable *pArgs);

        _Check_return_ HRESULT HasShapingRules(_Out_ bool* pHasRules);
        _Check_return_ HRESULT StartShaping(bool isGrouping);
        _Check_return_ HRESULT RefreshOverride(bool isSkippedWhenUnchanged = false);
        _Check_return_ HRESULT RefreshOrDefer(bool isSkippedWhenUnchanged = false);
        // A rule change, Refresh or source change made while app code ran or while the view raised its
        // changes was recorded in m_isRecomputePending; this runs it once that work has unwound.
        _Check_return_ HRESULT RunPendingRecompute();
        _Check_return_ HRESULT ProcessCollectionChanged(_In_ wfc::IVectorChangedEventArgs* pArgs);
        _Check_return_ HRESULT ProcessCollectionChangedWithAdjustedIndex(wfc::CollectionChange change, UINT index, _Out_ bool* pIsApplied);
        _Check_return_ HRESULT ProcessGroupedCollectionChanged(wfc::CollectionChange change, UINT index, _Out_ bool* pIsApplied);
        _Check_return_ HRESULT GetFilterHost(_Outptr_result_maybenull_ CollectionViewSource** ppHost);
        _Check_return_ HRESULT PassesFilter(_In_opt_ IInspectable* pItem, _In_opt_ CollectionViewSource* pFilterHost, _Out_ bool* pIsAccepted);
        _Check_return_ HRESULT CreateShapingDescriptions(
            _Outptr_ wfc::IVector<xaml_data::SortDescription*>** ppSortDescriptions,
            _Outptr_ wfc::IVector<xaml_data::PropertyGroupDescription*>** ppGroupDescriptions);
        void DetachSourceList();

        // Live shaping (VectorCollectionView_Shaping.cpp; WPF: LiveShapingList / LiveShapingItem).
        struct LiveShapingState;
        struct LiveShapingItem;
        class LivePropertyObserver;
        _Check_return_ HRESULT SetIsLive(bool* pIsLive, _In_opt_ wf::IReference<bool>* pValue);
        _Check_return_ HRESULT EnsureLiveShapingProperties();
        _Check_return_ HRESULT OnLiveShapingChanged();
        _Check_return_ HRESULT RebuildLiveShapingItems();
        _Check_return_ HRESULT CreateLiveShapingItem(_In_ const LiveShapingState& state, UINT sourceIndex, _Out_ std::unique_ptr<LiveShapingItem>* pItem);
        _Check_return_ HRESULT UpdateLiveShapingItems(wfc::CollectionChange change, UINT index);
        static void DisconnectLiveShapingItems(_In_ LiveShapingState& state);
        void ClearLiveShapingItems();
        _Check_return_ HRESULT OnLivePropertyChanged(_In_ LivePropertyObserver* pObserver);
        _Check_return_ HRESULT OnLiveShapingRestoreCallback();
        _Check_return_ HRESULT RestoreLiveShaping();
        bool IsLiveShaping() const;
        bool HasSortDirtyLiveItems() const;
        _Check_return_ HRESULT OnIsLiveChanged(UINT turnedOn, UINT turnedOff);

    protected:
        // Walks the live shaping listeners, which hold the source items.
        void OnReferenceTrackerWalk(INT walkType) override;

    private:

        // What the view presents: the source itself, or once shaping starts the InternalList (flat) or
        // a GroupedDataCollectionView over it (grouped).
        TrackerPtr<wfc::IVector<IInspectable *>> m_tpSource;
        TrackerPtr<xaml_data::ISupportIncrementalLoading> m_tpSupportIncrementalLoading;
        ctl::EventPtr<VectorChangedEventCallback> m_epVectorChangedHandler;

        bool m_isShapingEnabled = false;
        bool m_isDefaultView = false;
        ctl::WeakRefPtr m_hostRef;
        TrackerPtr<xaml_data::ICollectionViewSource> m_tpFilterHost;

        TrackerPtr<wfc::IVector<xaml_data::SortDescription*>> m_tpSortDescriptions;
        TrackerPtr<wfc::IVector<xaml_data::PropertyGroupDescription*>> m_tpGroupDescriptions;
        TrackerPtr<xaml_data::ICollectionViewFilter> m_tpFilter;
        TrackerPtr<xaml_data::ICollectionViewComparer> m_tpCustomSort;
        wrl_wrappers::HString m_culture;
        TrackerPtr<IInspectable> m_tpSourceCollection;
        bool m_isApplyingCustomSort = false;

        // The collection the view shapes; followed only while shaping (non-null m_tpInternalList).
        TrackerPtr<wfc::IVector<IInspectable *>> m_tpSourceList;
        TrackerPtr<wfc::IObservableVector<IInspectable *>> m_tpSourceListObservable;
        ctl::EventPtr<VectorChangedEventCallback> m_epSourceListChangedHandler;
        TrackerPtr<wfc::IVector<IInspectable *>> m_tpInternalList;
        // The output lists the view presents (m_tpInternalList is one of them) and the grouped presentation over
        // the group list. Created once each, so the view's CollectionGroups is one object for its lifetime.
        TrackerPtr<wfc::IVector<IInspectable *>> m_tpFlatList;
        TrackerPtr<wfc::IVector<IInspectable *>> m_tpGroupList;
        TrackerPtr<xaml_data::ICollectionView> m_tpGroupedPresentation;
        // Grouped output: the group tree (WPF: CollectionViewGroupRoot); its top-level groups are the InternalList.
        ctl::ComPtr<xaml_data::ICollectionViewGroup> m_spGroupRoot;
        bool m_isGrouping = false;

        // Nonzero while the engine runs app code (filters, property getters, converters, comparers) or raises
        // its changes. A rule change, Refresh or source change made meanwhile sets m_isRecomputePending and
        // runs, once, after the work unwinds (WPF: a refresh the view needs is done, not dropped).
        UINT m_shapeBusyDepth = 0;
        bool m_isRecomputePending = false;
        bool m_isRunningPendingRecompute = false;

        // The source index of each output item (flat output; grouped output keeps them per group), so a
        // source Add, Remove or Replace updates the view incrementally. Invalid after a change that was not applied.
        std::vector<UINT> m_sourceIndices;
        bool m_areSourceIndicesValid = false;
        UINT m_sourceListSize = 0;
        UINT m_sourceListChangeCount = 0;

        // Open DeferRefresh deferrals; a change made while any is open recomputes once, when the last closes.
        UINT m_deferRefreshLevel = 0;
        bool m_isRefreshPending = false;

        // ICollectionViewLiveShaping. The state (observers per source item, dirty items) is created while any
        // live feature is on.
        bool m_isLiveSorting = false;
        bool m_isLiveFiltering = false;
        bool m_isLiveGrouping = false;
        ctl::ComPtr<wfc::IObservableVector<HSTRING>> m_spLiveSortingProperties;
        ctl::ComPtr<wfc::IObservableVector<HSTRING>> m_spLiveFilteringProperties;
        ctl::ComPtr<wfc::IObservableVector<HSTRING>> m_spLiveGroupingProperties;
        std::shared_ptr<LiveShapingState> m_spLiveShaping;
        bool m_isApplyingLiveShaping = false;
    };
}
