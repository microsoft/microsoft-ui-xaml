// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsSourceView.h"

class InspectingDataSource : 
	public winrt::implements<InspectingDataSource, ItemsSourceView>
{
public:
    ForwardRefToBaseReferenceTracker(ItemsSourceView)

    InspectingDataSource(const winrt::IInspectable& source);
    ~InspectingDataSource();

#pragma region ItemsSourceViewOverrides

    int32_t GetSizeCore() override;
    winrt::IInspectable GetAtCore(int index) override;
    bool HasKeyIndexMappingCore() override;
    winrt::hstring KeyFromIndexCore(int index) override;
    int IndexFromKeyCore(winrt::hstring const& id) override;
    int IndexOfCore(winrt::IInspectable const& value);
#pragma endregion

private:
    winrt::Collections::IVector<winrt::IInspectable>
    WrapIterable(const winrt::Collections::IIterable<winrt::IInspectable>& iterable);

    void UnListenToCollectionChanges();
    void ListenToCollectionChanges();

    void OnCollectionChanged(
        const winrt::IInspectable& sender,
        const winrt::NotifyCollectionChangedEventArgs& e);

    void OnBindableVectorChanged(
        const winrt::IBindableObservableVector& sender,
        const winrt::IInspectable& e);

    void OnVectorChanged(
        const winrt::Collections::IObservableVector<winrt::IInspectable>& sender,
        const winrt::Collections::IVectorChangedEventArgs& e);

    tracker_ref<winrt::Collections::IVector<winrt::IInspectable>> m_vector{ this };
    tracker_ref<winrt::Collections::IVectorView<winrt::IInspectable>> m_vectorView{ this };

    // To unhook event from data source
    tracker_ref<winrt::INotifyCollectionChanged> m_notifyCollectionChanged{ this };
    tracker_ref<winrt::IObservableVector<winrt::IInspectable>> m_observableVector{ this };
    tracker_ref<winrt::IBindableObservableVector> m_bindableObservableVector{ this };
    winrt::event_token m_eventToken{ };
    winrt::IKeyIndexMapping m_uniqueIdMaping{ nullptr };
};
