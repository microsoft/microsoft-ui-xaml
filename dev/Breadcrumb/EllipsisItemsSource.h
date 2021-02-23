// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include "pch.h"
#include "common.h"

#include "Vector.h"

class EllipsisItemsSource :
    public ReferenceTracker<EllipsisItemsSource,
    reference_tracker_implements_t<winrt::INotifyCollectionChanged>::type,
    winrt::IBindableVector,
    winrt::IBindableVectorView>
{

public:
    EllipsisItemsSource();
    ~EllipsisItemsSource();

    void ResetList();
    void SetNewList(const winrt::Collections::IVector<winrt::IInspectable>&);

    // INotifyCollectionChanged
    winrt::event_token CollectionChanged(winrt::NotifyCollectionChangedEventHandler const& value);
    void CollectionChanged(winrt::event_token const& token);

    // IBindableVector
    void Clear();
    winrt::IInspectable GetAt(uint32_t const index) const;
    uint32_t Size();
    bool IndexOf(winrt::IInspectable const& value, uint32_t& index) const;
    void SetAt(uint32_t const index, winrt::IInspectable  const& value);
    void InsertAt(uint32_t const index, winrt::IInspectable const& value);
    void RemoveAt(uint32_t const index);
    void Append(winrt::IInspectable  const& value);
    void RemoveAtEnd();
    uint32_t GetMany(uint32_t const startIndex, winrt::array_view<winrt::IInspectable> values) const;
    void ReplaceAll(winrt::array_view<winrt::IInspectable  const> value);
    winrt::IIterator<winrt::IInspectable > First();

    winrt::IBindableVectorView GetView();

private:
    event_source<winrt::NotifyCollectionChangedEventHandler> m_collectionChangedEventSource{ this };
    winrt::IVector<winrt::IInspectable> m_data;
};
