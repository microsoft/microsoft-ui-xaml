// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>

#include "pch.h"
#include "common.h"

#include <winrt/Microsoft.UI.Xaml.Interop.h>

class ShapedItemsSource6;

// The presented rows of a ShapedItemsSource6, read through to the engine's tree: nothing is
// copied. GetAt(i) walks the tree by visible-subtree counts.
//
// Deliberately an IVectorView plus INotifyCollectionChanged, not an IBindableVector: ItemsSourceView
// would take the bindable path first and expect the vector to raise its own changes.
class VisibleRowList6 : public winrt::implements<
    VisibleRowList6,
    winrt::Windows::Foundation::Collections::IVectorView<winrt::IInspectable>,
    winrt::Windows::Foundation::Collections::IIterable<winrt::IInspectable>,
    winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>
{
public:
    explicit VisibleRowList6(std::weak_ptr<ShapedItemsSource6> engine);

    // IVectorView
    winrt::IInspectable GetAt(uint32_t index) const;
    uint32_t Size() const;
    bool IndexOf(winrt::IInspectable const& value, uint32_t& index) const;
    uint32_t GetMany(uint32_t startIndex, winrt::array_view<winrt::IInspectable> items) const;

    // IIterable
    winrt::Windows::Foundation::Collections::IIterator<winrt::IInspectable> First();

    // INotifyCollectionChanged
    winrt::event_token CollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventHandler const& handler);
    void CollectionChanged(winrt::event_token const& token) noexcept;

    // Changes carry only counts: the items are placeholders, which is all the repeater reads.
    void RaiseAdded(int32_t index, int32_t count);
    void RaiseRemoved(int32_t index, int32_t count);
    void RaiseReplaced(int32_t index);
    void RaiseReset();

private:
    void Raise(
        winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction action,
        int32_t newCount,
        int32_t oldCount,
        int32_t newIndex,
        int32_t oldIndex);

    std::weak_ptr<ShapedItemsSource6> m_engine;
    winrt::event<winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventHandler> m_collectionChanged;
};
