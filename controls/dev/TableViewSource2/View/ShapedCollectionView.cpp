// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "View/ShapedCollectionView.h"
#include "View/ShapedRowQueries.h"

namespace
{
    // Forward-only iterator over any read-only bindable vector, by index (Size / GetAt per step).
    // Holds the vector strongly; both users are native objects that hold no app object.
    struct BindableIndexIterator :
        winrt::implements<BindableIndexIterator, winrt::IBindableIterator>
    {
        explicit BindableIndexIterator(winrt::IBindableVector const& vector) : m_vector(vector) {}

        winrt::IInspectable Current() { return m_vector.GetAt(m_index); }
        bool HasCurrent() { return m_index < m_vector.Size(); }
        bool MoveNext()
        {
            if (HasCurrent())
            {
                ++m_index;
            }
            return HasCurrent();
        }

    private:
        winrt::IBindableVector m_vector;
        uint32_t m_index{ 0 };
    };

    // D3 item list: count nullptrs, the shape InspectingDataSource::OnVectorChanged raises. Stores
    // only the count (O(1) per event); the Collections Vector is not on the Tabular DLL's include
    // path. Read-only: mutators throw E_ILLEGAL_METHOD_CALL.
    struct PlaceholderItems :
        winrt::implements<PlaceholderItems, winrt::IBindableVector, winrt::IBindableIterable>
    {
        explicit PlaceholderItems(uint32_t count) noexcept : m_count(count) {}

        uint32_t Size() const noexcept { return m_count; }
        winrt::IInspectable GetAt(uint32_t index) const
        {
            if (index >= m_count)
            {
                throw winrt::hresult_out_of_bounds();
            }
            return nullptr;
        }
        bool IndexOf(winrt::IInspectable const& value, uint32_t& index) const noexcept
        {
            index = 0;
            return !value && m_count > 0;
        }
        winrt::IBindableVectorView GetView() { throw winrt::hresult_not_implemented(); }
        void SetAt(uint32_t, winrt::IInspectable const&) { throw winrt::hresult_illegal_method_call(); }
        void InsertAt(uint32_t, winrt::IInspectable const&) { throw winrt::hresult_illegal_method_call(); }
        void RemoveAt(uint32_t) { throw winrt::hresult_illegal_method_call(); }
        void Append(winrt::IInspectable const&) { throw winrt::hresult_illegal_method_call(); }
        void RemoveAtEnd() { throw winrt::hresult_illegal_method_call(); }
        void Clear() { throw winrt::hresult_illegal_method_call(); }
        winrt::IBindableIterator First() { return winrt::make<BindableIndexIterator>(*this); }

    private:
        uint32_t const m_count;
    };

    winrt::IBindableVector MakePlaceholderItems(uint32_t count)
    {
        return winrt::make<PlaceholderItems>(count);
    }
}

ShapedCollectionView::ShapedCollectionView(ShapedRowQueries const& rows) noexcept
    : m_rows(&rows)
{
}

uint32_t ShapedCollectionView::Size()
{
    return m_rows ? m_rows->RowCount() : 0;
}

winrt::IInspectable ShapedCollectionView::GetAt(uint32_t index)
{
    if (!m_rows)
    {
        throw winrt::hresult_out_of_bounds();
    }

    // RowAt range-checks (ShapedTree::GetAt only asserts).
    ShapedNode* const node = m_rows->RowAt(index);
    if (node->IsGroup())
    {
        // GroupedEntry header values land with GroupBy in Milestone 4.
        throw winrt::hresult_not_implemented();
    }

    return node->AsRecord()->Item.get();
}

bool ShapedCollectionView::IndexOf(winrt::IInspectable const& value, uint32_t& index)
{
    index = 0;
    if (!m_rows)
    {
        return false;
    }

    auto const found = m_rows->IndexOfItem(value);
    if (!found)
    {
        return false;
    }

    index = *found;
    return true;
}

winrt::IBindableVectorView ShapedCollectionView::GetView()
{
    return *this;
}

void ShapedCollectionView::SetAt(uint32_t, winrt::IInspectable const&)
{
    throw winrt::hresult_illegal_method_call();
}

void ShapedCollectionView::InsertAt(uint32_t, winrt::IInspectable const&)
{
    throw winrt::hresult_illegal_method_call();
}

void ShapedCollectionView::RemoveAt(uint32_t)
{
    throw winrt::hresult_illegal_method_call();
}

void ShapedCollectionView::Append(winrt::IInspectable const&)
{
    throw winrt::hresult_illegal_method_call();
}

void ShapedCollectionView::RemoveAtEnd()
{
    throw winrt::hresult_illegal_method_call();
}

void ShapedCollectionView::Clear()
{
    throw winrt::hresult_illegal_method_call();
}

winrt::IBindableIterator ShapedCollectionView::First()
{
    return winrt::make<BindableIndexIterator>(*this);
}

winrt::event_token ShapedCollectionView::CollectionChanged(winrt::NotifyCollectionChangedEventHandler const& handler)
{
    return m_collectionChanged.add(handler);
}

void ShapedCollectionView::CollectionChanged(winrt::event_token const& token)
{
    m_collectionChanged.remove(token);
}

winrt::hstring ShapedCollectionView::KeyFromIndex(int32_t index)
{
    if (!m_rows || index < 0)
    {
        throw winrt::hresult_out_of_bounds();
    }

    return m_rows->KeyAt(static_cast<uint32_t>(index));
}

int32_t ShapedCollectionView::IndexFromKey(winrt::hstring const& key)
{
    if (!m_rows)
    {
        return -1;
    }

    auto const index = m_rows->IndexOfKey(key);
    return index ? static_cast<int32_t>(*index) : -1;
}

void ShapedCollectionView::NotifyRemoved(uint32_t index, uint32_t count)
{
    Raise(winrt::NotifyCollectionChangedEventArgs(
        winrt::NotifyCollectionChangedAction::Remove,
        MakePlaceholderItems(0),
        MakePlaceholderItems(count),
        -1,
        static_cast<int32_t>(index)));
}

void ShapedCollectionView::NotifyAdded(uint32_t index, uint32_t count)
{
    Raise(winrt::NotifyCollectionChangedEventArgs(
        winrt::NotifyCollectionChangedAction::Add,
        MakePlaceholderItems(count),
        MakePlaceholderItems(0),
        static_cast<int32_t>(index),
        -1));
}

void ShapedCollectionView::NotifyReset()
{
    Raise(winrt::NotifyCollectionChangedEventArgs(
        winrt::NotifyCollectionChangedAction::Reset,
        MakePlaceholderItems(0),
        MakePlaceholderItems(0),
        -1,
        -1));
}

void ShapedCollectionView::Detach() noexcept
{
    m_rows = nullptr;
}

void ShapedCollectionView::Raise(winrt::NotifyCollectionChangedEventArgs const& args)
{
    m_collectionChanged(*this, args);
}
