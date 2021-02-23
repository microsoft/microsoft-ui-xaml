// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "BindableVector.h"

#include "EllipsisItemsSource.h"

EllipsisItemsSource::EllipsisItemsSource()
{
    m_data = winrt::make<Vector<winrt::IInspectable>>();
}

EllipsisItemsSource::~EllipsisItemsSource()
{
}

void EllipsisItemsSource::ResetList()
{
    Clear();

    auto const& args = winrt::NotifyCollectionChangedEventArgs(winrt::NotifyCollectionChangedAction::Reset, *this, nullptr, 0, 0);
    m_collectionChangedEventSource(*this, args);
}

void EllipsisItemsSource::SetNewList(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource)
{
    Clear();

    for (uint32_t i = 0; i < ellipsisItemsSource.Size(); ++i)
    {
        m_data.Append(ellipsisItemsSource.GetAt(i));
    }

    auto const& args = winrt::NotifyCollectionChangedEventArgs(winrt::NotifyCollectionChangedAction::Reset, *this, nullptr, 0, 0);
    m_collectionChangedEventSource(*this, args);
}

winrt::event_token EllipsisItemsSource::CollectionChanged(winrt::NotifyCollectionChangedEventHandler const& value)
{
    return m_collectionChangedEventSource.add(value);
}

void EllipsisItemsSource::CollectionChanged(winrt::event_token const& token)
{
    m_collectionChangedEventSource.remove(token);
}

void EllipsisItemsSource::Clear()
{
    m_data.Clear();
}

winrt::IInspectable EllipsisItemsSource::GetAt(uint32_t const index) const
{
    return m_data.GetAt(index);
}

uint32_t EllipsisItemsSource::Size()
{
    return m_data.Size();
}

bool EllipsisItemsSource::IndexOf(winrt::IInspectable const& value, uint32_t& index) const
{
    return m_data.IndexOf(value, index);
}

void EllipsisItemsSource::SetAt(uint32_t const index, winrt::IInspectable const& value)
{
    return m_data.SetAt(index, value);
}

void EllipsisItemsSource::InsertAt(uint32_t const index, winrt::IInspectable const& value)
{
    return m_data.InsertAt(index, value);
}

void EllipsisItemsSource::RemoveAt(uint32_t const index)
{
    return m_data.RemoveAt(index);
}

void EllipsisItemsSource::Append(winrt::IInspectable const& value)
{
    return m_data.Append(value);
}

void EllipsisItemsSource::RemoveAtEnd()
{
    return m_data.RemoveAtEnd();
}

uint32_t EllipsisItemsSource::GetMany(uint32_t const startIndex, winrt::array_view<winrt::IInspectable> values) const
{
    return m_data.GetMany(startIndex, values);
}

void EllipsisItemsSource::ReplaceAll(winrt::array_view<winrt::IInspectable const> value)
{
    return m_data.ReplaceAll(value);
}

winrt::IIterator<winrt::IInspectable> EllipsisItemsSource::First()
{
    return m_data.First();
}

winrt::IBindableVectorView EllipsisItemsSource::GetView()
{
    return *this;
}
