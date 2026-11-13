// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CollectionViewSource.g.h"

namespace DirectUI
{

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

    private:

        _Check_return_ HRESULT OnSourceChanged(_In_ IInspectable *pOldSource, _In_ IInspectable *pNewSource);
        _Check_return_ HRESULT OnIsSourceGroupedChanged();
        _Check_return_ HRESULT OnItemsPathChanged();
        _Check_return_ HRESULT EnsureView(_In_ IInspectable *pNewSource);

        static bool IsSourceValid(_In_ IInspectable *pSource);

        // Implemented in CollectionViewSource_Shaping.cpp.
        _Check_return_ HRESULT IsShapingRequested(_In_ IInspectable *pSource, _Out_ bool *pIsRequested);
        _Check_return_ HRESULT CreateShapedView(_In_ IInspectable *pSource, _In_opt_ xaml_data::ICollectionView *pExistingView, _Outptr_ xaml_data::ICollectionView **ppView);
        _Check_return_ HRESULT OfferShapingOnView(_In_ IInspectable *pSource, _In_ xaml_data::ICollectionView *pView);
        _Check_return_ HRESULT ComputeShapedOutput();
        _Check_return_ HRESULT RefreshShapedView();
        _Check_return_ HRESULT OnShapedSourceVectorChanged(_In_ wfc::IVectorChangedEventArgs* pArgs);
        _Check_return_ HRESULT ApplyShapedSourceChange(wfc::CollectionChange change, UINT index, _Out_ bool* pIsApplied);
        void DetachShapedSource();

    protected:

        void OnReferenceTrackerWalk(INT walkType ) final;

    private:

        CVSViewChangedEventSource *m_pCVSViewChangedEventSource;

        TrackerPtr<wfc::IVector<xaml_data::SortDescription*>> m_tpSortDescriptions;
        TrackerPtr<wfc::IVector<xaml_data::PropertyGroupDescription*>> m_tpGroupDescriptions;
        TrackerPtr<xaml_data::ICollectionViewFilter> m_tpFilter;
        TrackerPtr<xaml_data::ICollectionViewComparer> m_tpCustomSort;
        bool m_isApplyingCustomSort = false;

        // Non-null only while View is a shaped view.
        TrackerPtr<wfc::IVector<IInspectable*>> m_tpShapedSource;
        TrackerPtr<wfc::IObservableVector<IInspectable*>> m_tpShapedSourceObservable;
        TrackerPtr<wfc::IVector<IInspectable*>> m_tpShapedOutput;
        ctl::EventPtr<VectorChangedEventCallback> m_epShapedSourceVectorChangedHandler;
        bool m_isShapedOutputGrouped = false;
        bool m_isComputingShape = false;

        // Flat output only: the source index of each output item, so a source Add, Remove or
        // Replace updates the view incrementally. Invalid after a change that was not applied.
        std::vector<UINT> m_shapedSourceIndices;
        bool m_areShapedSourceIndicesValid = false;
        UINT m_shapedSourceSize = 0;
        UINT m_shapedSourceChangeCount = 0;

        // Open DeferRefresh deferrals; a change made while any is open recomputes once, when the last closes.
        UINT m_deferRefreshLevel = 0;
        bool m_isRefreshPending = false;
    };
}
