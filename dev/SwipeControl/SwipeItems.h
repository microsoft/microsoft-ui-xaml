// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SwipeItems.g.h"
#include "SwipeItems.properties.h"

class SwipeItems :
    public ReferenceTracker<SwipeItems, winrt::implementation::SwipeItemsT, winrt::IObservableVector<winrt::SwipeItem>>,
    public SwipeItemsProperties
{
public:
    SwipeItems();
    virtual ~SwipeItems();

#pragma region IVector
    winrt::SwipeItem GetAt(uint32_t index);
    uint32_t Size();
    winrt::IVectorView<winrt::SwipeItem> GetView();
    bool IndexOf(winrt::SwipeItem const& value, uint32_t& index);
    void SetAt(uint32_t index, winrt::SwipeItem const& value);
    void InsertAt(uint32_t index, winrt::SwipeItem const& value);
    void RemoveAt(uint32_t index);
    void Append(winrt::SwipeItem const& value);
    void RemoveAtEnd();
    void Clear();

    // TODO:
    winrt::IIterator<winrt::SwipeItem> First() { throw winrt::hresult_not_implemented(); }
    uint32_t GetMany(uint32_t startIndex, winrt::array_view<winrt::SwipeItem> items) { throw winrt::hresult_not_implemented(); }
    void ReplaceAll(winrt::array_view<winrt::SwipeItem const> items) { throw winrt::hresult_not_implemented(); }
#pragma endregion

#pragma region IObservableVector
    winrt::event_token VectorChanged(winrt::VectorChangedEventHandler<winrt::SwipeItem> const& handler);
    void VectorChanged(winrt::event_token const token);
#pragma endregion

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void put_Items(const winrt::Collections::IVector<winrt::SwipeItem>& value);
    tracker_ref<winrt::Collections::IVector<winrt::SwipeItem>> m_items{ this };

    event_source<winrt::VectorChangedEventHandler<winrt::SwipeItem>> m_vectorChangedEventSource{ this };
};
