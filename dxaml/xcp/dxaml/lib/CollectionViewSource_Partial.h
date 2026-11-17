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
        _Check_return_ HRESULT GetCVSViewChangedSource(_Outptr_ ICVSViewChangedEventSource **ppSource);

        _Check_return_ HRESULT get_SortDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue);
        _Check_return_ HRESULT get_GroupDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::PropertyGroupDescription*>** ppValue);
        _Check_return_ HRESULT get_FilterImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewFilter** ppValue);
        _Check_return_ HRESULT put_FilterImpl(_In_opt_ xaml_data::ICollectionViewFilter* pValue);
        _Check_return_ HRESULT RefreshImpl();
        _Check_return_ HRESULT DeferRefreshImpl(_Outptr_ wf::IClosable** ppReturnValue);
        _Check_return_ HRESULT get_CustomSortImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue);
        _Check_return_ HRESULT put_CustomSortImpl(_In_opt_ xaml_data::ICollectionViewComparer* pValue);

        _Check_return_ HRESULT OnShapingDescriptionsChanged();
        _Check_return_ HRESULT OnSortDescriptionsChanged();
        _Check_return_ HRESULT EndDeferRefresh();

        // The shaping view's output switches between flat and grouped. Consumers read CollectionGroups once
        // per ItemsSource, so View is replaced. *pIsHandled is false if pView is not this source's View.
        _Check_return_ HRESULT OnShapingViewGroupingChanged(_In_ VectorCollectionView* pView, _Out_ bool* pIsHandled);

    private:

        _Check_return_ HRESULT OnSourceChanged(_In_ IInspectable *pOldSource, _In_ IInspectable *pNewSource);
        _Check_return_ HRESULT OnIsSourceGroupedChanged();
        _Check_return_ HRESULT OnItemsPathChanged();
        _Check_return_ HRESULT EnsureView(_In_ IInspectable *pNewSource);

        static bool IsSourceValid(_In_ IInspectable *pSource);

        // Implemented in CollectionViewSource_Shaping.cpp.
        _Check_return_ HRESULT IsShapingRequested(_In_ IInspectable *pSource, _Out_ bool *pIsRequested);
        _Check_return_ HRESULT EnsureShapingDescriptions();
        _Check_return_ HRESULT ApplyPropertiesToView(_In_ VectorCollectionView* pView);
        _Check_return_ HRESULT DetachShapingView();

    protected:

        void OnReferenceTrackerWalk(INT walkType ) final;

    private:

        CVSViewChangedEventSource *m_pCVSViewChangedEventSource;

        // The rules live here only while there is no shaping view; the view owns them while it is View.
        // The descriptor collections are shared with the view.
        TrackerPtr<wfc::IVector<xaml_data::SortDescription*>> m_tpSortDescriptions;
        TrackerPtr<wfc::IVector<xaml_data::PropertyGroupDescription*>> m_tpGroupDescriptions;
        TrackerPtr<xaml_data::ICollectionViewFilter> m_tpFilter;
        TrackerPtr<xaml_data::ICollectionViewComparer> m_tpCustomSort;
        bool m_isApplyingCustomSort = false;

        // View, when it is a VectorCollectionView that shapes (WPF: the view of the ViewRecord).
        TrackerPtr<xaml_data::ICollectionView> m_tpShapingView;

        // Open DeferRefresh deferrals taken through this source. While any is open, the shaping view holds
        // one deferral of its own (m_spViewDeferral).
        UINT m_deferRefreshLevel = 0;
        bool m_isRefreshPending = false;
        Microsoft::WRL::ComPtr<wf::IClosable> m_spViewDeferral;
    };
}
