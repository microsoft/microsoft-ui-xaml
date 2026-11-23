// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CollectionViewSource.g.h"

namespace DirectUI
{
    class VectorCollectionView;

    typedef CFrameworkEventSource<
        ICVSViewChangedEventSource,
        ICVSViewChangedHandler,
        IInspectable,
        IInspectable> CVSViewChangedEventSource;

    MIDL_INTERFACE("6f1a3e52-9c0b-4d7e-8a41-2b5f0c9d7e13")
    IFilterEventSource : public IUnknown
    {
    };

    typedef CFrameworkEventSource<
        IFilterEventSource,
        xaml_data::IFilterEventHandler,
        IInspectable,
        xaml_data::IFilterEventArgs> FilterEventSource;

    PARTIAL_CLASS(CollectionViewSource)
    {

    protected:

        CollectionViewSource();
        ~CollectionViewSource() override;

        // Handle the custom property changed event and call the
        // OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

    public:

        _Check_return_ HRESULT GetValue(_In_ const CDependencyProperty* pProperty, _Out_ IInspectable** ppValue) override;
        _Check_return_ HRESULT GetDefaultValue2(_In_ const CDependencyProperty* pDP, _Out_ CValue* pValue) override;
        _Check_return_ HRESULT GetCVSViewChangedSource(_Outptr_ ICVSViewChangedEventSource **ppSource);

        _Check_return_ HRESULT get_SortDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue);
        _Check_return_ HRESULT get_GroupDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::GroupDescription*>** ppValue);
        _Check_return_ HRESULT RefreshImpl();
        _Check_return_ HRESULT DeferRefreshImpl(_Outptr_ wf::IClosable** ppReturnValue);
        _Check_return_ HRESULT get_CustomSortImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue);
        _Check_return_ HRESULT put_CustomSortImpl(_In_opt_ xaml_data::ICollectionViewComparer* pValue);
        _Check_return_ HRESULT get_CultureImpl(_Out_ HSTRING* pValue);
        _Check_return_ HRESULT put_CultureImpl(_In_opt_ HSTRING value);

        // WPF: CollectionViewSource.Live*Properties, applied to View's ICollectionViewLiveShaping with IsLive*Requested.
        _Check_return_ HRESULT get_LiveSortingPropertiesImpl(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue);
        _Check_return_ HRESULT get_LiveFilteringPropertiesImpl(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue);
        _Check_return_ HRESULT get_LiveGroupingPropertiesImpl(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue);

        // The Filter event (WPF: CollectionViewSource.Filter). While it has handlers, the view filters through them
        // instead of its Filter delegate.
        _Check_return_ HRESULT AddFilterEventHandler(_In_ xaml_data::IFilterEventHandler* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT RemoveFilterEventHandler(EventRegistrationToken token);
        bool HasFilterEventHandlers() const;
        _Check_return_ HRESULT RaiseFilterEvent(_In_opt_ IInspectable* pItem, _Out_ bool* pIsAccepted);

        _Check_return_ HRESULT OnShapingDescriptionsChanged();
        _Check_return_ HRESULT OnSortDescriptionsChanged();
        _Check_return_ HRESULT EndDeferRefresh();

    private:

        _Check_return_ HRESULT OnSourceChanged(_In_ IInspectable *pOldSource, _In_ IInspectable *pNewSource);
        _Check_return_ HRESULT OnIsSourceGroupedChanged();
        _Check_return_ HRESULT OnItemsPathChanged();
        _Check_return_ HRESULT EnsureView(_In_ IInspectable *pNewSource);

        static bool IsSourceValid(_In_ IInspectable *pSource);

        // Implemented in CollectionViewSource_Shaping.cpp.
        _Check_return_ HRESULT HasShapingRules(_Out_ bool* pHasRules);
        _Check_return_ HRESULT IsShapeableSource(_In_ IInspectable *pSource, _Out_ bool *pIsShapeable);
        _Check_return_ HRESULT IsShapingRequested(_In_ IInspectable *pSource, _Out_ bool *pIsRequested);
        _Check_return_ HRESULT CheckShapingSupported(_In_ IInspectable *pSource);
        _Check_return_ HRESULT OnFilterEventHandlersChanged();
        _Check_return_ HRESULT EnsureShapingDescriptions();
        _Check_return_ HRESULT ApplyPropertiesToView(_In_ VectorCollectionView* pView);
        _Check_return_ HRESULT DetachShapingView();
        _Check_return_ HRESULT EnsureLiveShapingProperties();
        _Check_return_ HRESULT ApplyLiveShapingToView();
        _Check_return_ HRESULT GetLiveShapingRequests(_Out_ bool* pIsLiveSorting, _Out_ bool* pIsLiveFiltering, _Out_ bool* pIsLiveGrouping);

    protected:

        void OnReferenceTrackerWalk(INT walkType ) final;

    private:

        CVSViewChangedEventSource *m_pCVSViewChangedEventSource;

        // The rules live here only while there is no shaping view; the view owns them while it is View.
        // The descriptor collections are shared with the view. A view's Filter delegate stays with the view, as in
        // WPF (the CollectionViewSource filters through its Filter event).
        TrackerPtr<wfc::IVector<xaml_data::SortDescription*>> m_tpSortDescriptions;
        TrackerPtr<wfc::IVector<xaml_data::GroupDescription*>> m_tpGroupDescriptions;
        TrackerPtr<xaml_data::ICollectionViewComparer> m_tpCustomSort;
        wrl_wrappers::HString m_culture;
        bool m_isApplyingCustomSort = false;

        ctl::ComPtr<FilterEventSource> m_spFilterEventSource;

        // View, when it is a VectorCollectionView that shapes (WPF: the view of the ViewRecord).
        TrackerPtr<xaml_data::ICollectionView> m_tpShapingView;

        // Open DeferRefresh deferrals taken through this source. While any is open, the shaping view holds
        // one deferral of its own (m_spViewDeferral).
        UINT m_deferRefreshLevel = 0;
        bool m_isRefreshPending = false;
        Microsoft::WRL::ComPtr<wf::IClosable> m_spViewDeferral;

        // Unlike the other rules these stay here and are copied to each view, as WPF's are.
        ctl::ComPtr<wfc::IObservableVector<HSTRING>> m_spLiveSortingProperties;
        ctl::ComPtr<wfc::IObservableVector<HSTRING>> m_spLiveFilteringProperties;
        ctl::ComPtr<wfc::IObservableVector<HSTRING>> m_spLiveGroupingProperties;
    };
}
