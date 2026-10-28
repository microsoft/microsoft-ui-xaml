// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "VisibleRowList6.h"
#include "ShapedItemsSource6.h"

namespace
{
    using winrt::Microsoft::UI::Xaml::Interop::IBindableIterable;
    using winrt::Microsoft::UI::Xaml::Interop::IBindableIterator;
    using winrt::Microsoft::UI::Xaml::Interop::IBindableVector;
    using winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView;

    // The NewItems/OldItems of a change: a size and nothing else.
    class CountOnlyItems : public winrt::implements<CountOnlyItems, IBindableVector, IBindableIterable>
    {
    public:
        explicit CountOnlyItems(uint32_t size) : m_size(size) {}

        winrt::IInspectable GetAt(uint32_t index) const
        {
            if (index >= m_size)
            {
                throw winrt::hresult_out_of_bounds();
            }
            return nullptr;
        }
        uint32_t Size() const noexcept { return m_size; }
        IBindableVectorView GetView() const { throw winrt::hresult_not_implemented(); }
        bool IndexOf(winrt::IInspectable const&, uint32_t& index) const noexcept
        {
            index = 0;
            return false;
        }
        void SetAt(uint32_t, winrt::IInspectable const&) { throw winrt::hresult_illegal_method_call(); }
        void InsertAt(uint32_t, winrt::IInspectable const&) { throw winrt::hresult_illegal_method_call(); }
        void RemoveAt(uint32_t) { throw winrt::hresult_illegal_method_call(); }
        void Append(winrt::IInspectable const&) { throw winrt::hresult_illegal_method_call(); }
        void RemoveAtEnd() { throw winrt::hresult_illegal_method_call(); }
        void Clear() { throw winrt::hresult_illegal_method_call(); }
        IBindableIterator First() const { throw winrt::hresult_not_implemented(); }

    private:
        uint32_t m_size;
    };

    class RowIterator : public winrt::implements<RowIterator, winrt::Windows::Foundation::Collections::IIterator<winrt::IInspectable>>
    {
    public:
        explicit RowIterator(winrt::Windows::Foundation::Collections::IVectorView<winrt::IInspectable> const& rows) :
            m_rows(rows),
            m_size(rows.Size())
        {
        }

        winrt::IInspectable Current() const
        {
            if (m_index >= m_size)
            {
                throw winrt::hresult_out_of_bounds();
            }
            return m_rows.GetAt(m_index);
        }
        bool HasCurrent() const noexcept { return m_index < m_size; }
        bool MoveNext()
        {
            if (m_index < m_size)
            {
                ++m_index;
            }
            return m_index < m_size;
        }
        uint32_t GetMany(winrt::array_view<winrt::IInspectable> items)
        {
            uint32_t copied = 0;
            while (copied < items.size() && m_index < m_size)
            {
                items[copied++] = m_rows.GetAt(m_index++);
            }
            return copied;
        }

    private:
        winrt::Windows::Foundation::Collections::IVectorView<winrt::IInspectable> m_rows;
        uint32_t m_size;
        uint32_t m_index{ 0 };
    };
}

VisibleRowList6::VisibleRowList6(std::weak_ptr<ShapedItemsSource6> engine) :
    m_engine(std::move(engine))
{
}

winrt::IInspectable VisibleRowList6::GetAt(uint32_t index) const
{
    auto const engine = m_engine.lock();
    if (!engine || index >= static_cast<uint32_t>(engine->Size()))
    {
        throw winrt::hresult_out_of_bounds();
    }
    return engine->NodeAt(static_cast<int32_t>(index))->Item;
}

uint32_t VisibleRowList6::Size() const
{
    auto const engine = m_engine.lock();
    return engine ? static_cast<uint32_t>(engine->Size()) : 0;
}

bool VisibleRowList6::IndexOf(winrt::IInspectable const& value, uint32_t& index) const
{
    index = 0;
    auto const engine = m_engine.lock();
    if (!engine)
    {
        return false;
    }
    auto const position = engine->IndexOf(engine->NodeOfPresented(value));
    if (position < 0)
    {
        return false;
    }
    index = static_cast<uint32_t>(position);
    return true;
}

uint32_t VisibleRowList6::GetMany(uint32_t startIndex, winrt::array_view<winrt::IInspectable> items) const
{
    auto const engine = m_engine.lock();
    if (!engine)
    {
        return 0;
    }
    auto const size = static_cast<uint32_t>(engine->Size());
    uint32_t copied = 0;
    for (auto i = startIndex; i < size && copied < items.size(); ++i)
    {
        items[copied++] = engine->NodeAt(static_cast<int32_t>(i))->Item;
    }
    return copied;
}

winrt::Windows::Foundation::Collections::IIterator<winrt::IInspectable> VisibleRowList6::First()
{
    return winrt::make<RowIterator>(*this);
}

winrt::event_token VisibleRowList6::CollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventHandler const& handler)
{
    return m_collectionChanged.add(handler);
}

void VisibleRowList6::CollectionChanged(winrt::event_token const& token) noexcept
{
    m_collectionChanged.remove(token);
}

void VisibleRowList6::RaiseAdded(int32_t index, int32_t count)
{
    Raise(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction::Add, count, 0, index, -1);
}

void VisibleRowList6::RaiseRemoved(int32_t index, int32_t count)
{
    Raise(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction::Remove, 0, count, -1, index);
}

void VisibleRowList6::RaiseReplaced(int32_t index)
{
    Raise(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction::Replace, 1, 1, index, index);
}

void VisibleRowList6::RaiseReset()
{
    Raise(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction::Reset, 0, 0, -1, -1);
}

void VisibleRowList6::Raise(
    winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction action,
    int32_t newCount,
    int32_t oldCount,
    int32_t newIndex,
    int32_t oldIndex)
{
    if (!m_collectionChanged)
    {
        return;
    }
    IBindableVector newItems{ nullptr };
    IBindableVector oldItems{ nullptr };
    if (action != winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction::Reset)
    {
        newItems = winrt::make<CountOnlyItems>(static_cast<uint32_t>(newCount));
        oldItems = winrt::make<CountOnlyItems>(static_cast<uint32_t>(oldCount));
    }
    m_collectionChanged(*this, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs(action, newItems, oldItems, newIndex, oldIndex));
}
