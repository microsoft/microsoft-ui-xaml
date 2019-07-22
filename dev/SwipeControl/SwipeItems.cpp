// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SwipeItems.h"
#include "SwipeItem.h"
#include "SwipeControl.h"
#include "Vector.h"
#include "VectorIterator.h"

SwipeItems::SwipeItems()
{
    // create the Collection
    auto collection = winrt::make<Vector<winrt::SwipeItem, MakeVectorParam<VectorFlag::DependencyObjectBase>()>>();

    put_Items(collection);
}

SwipeItems::~SwipeItems()
= default;

void SwipeItems::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.Property() == s_ModeProperty)
    {
        if (winrt::unbox_value<winrt::SwipeMode>(args.NewValue()) == winrt::SwipeMode::Execute && m_items.get().Size() > 1)
        {
            throw winrt::hresult_invalid_argument(L"Execute items should only have one item.");
        }
    }
}

void SwipeItems::put_Items(
    const winrt::Collections::IVector<winrt::SwipeItem>& value)
{
    if (Mode() == winrt::SwipeMode::Execute && value.Size() > 1)
    {
        throw winrt::hresult_invalid_argument(L"Execute items should only have one item.");
    }

    m_items.set(value);
    m_vectorChangedEventSource(*this, nullptr);
}

winrt::SwipeItem SwipeItems::GetAt(uint32_t index)
{
    if (index >= m_items.get().Size())
    {
        throw winrt::hresult_out_of_bounds();
    }
    return m_items.get().GetAt(index);
}

uint32_t SwipeItems::Size()
{
    return m_items.get().Size();
}

bool SwipeItems::IndexOf(winrt::SwipeItem const& value, uint32_t& index)
{
    if (index >= m_items.get().Size())
    {
        throw winrt::hresult_out_of_bounds();
    }
    return m_items.get().IndexOf(value, index);
}

void SwipeItems::SetAt(uint32_t index, winrt::SwipeItem const& value)
{
    if (index >= m_items.get().Size())
    {
        throw winrt::hresult_out_of_bounds();
    }
    m_items.get().SetAt(index, value);
    m_vectorChangedEventSource(*this, nullptr);
}

void SwipeItems::InsertAt(uint32_t index, winrt::SwipeItem const& value)
{
    if (Mode() == winrt::SwipeMode::Execute && m_items.get().Size() > 0)
    {
        throw winrt::hresult_invalid_argument(L"Execute items should only have one item.");
    }
    if (index > m_items.get().Size())
    {
        throw winrt::hresult_out_of_bounds();
    }

    m_items.get().InsertAt(index, value);
    m_vectorChangedEventSource(*this, nullptr);
}

void SwipeItems::RemoveAt(uint32_t index)
{
    if (index >= m_items.get().Size())
    {
        throw winrt::hresult_out_of_bounds();
    }
    m_items.get().RemoveAt(index);
    m_vectorChangedEventSource(*this, nullptr);
}

void SwipeItems::Append(winrt::SwipeItem const& value)
{
    if (Mode() == winrt::SwipeMode::Execute && m_items.get().Size() > 0)
    {
        throw winrt::hresult_invalid_argument(L"Execute items should only have one item.");
    }
    
    m_items.get().Append(value);
    m_vectorChangedEventSource(*this, nullptr);
}

void SwipeItems::RemoveAtEnd()
{
    m_items.get().RemoveAtEnd();
    m_vectorChangedEventSource(*this, nullptr);
}

void SwipeItems::Clear()
{
    m_items.get().Clear();
    m_vectorChangedEventSource(*this, nullptr);
}

winrt::IVectorView<winrt::SwipeItem> SwipeItems::GetView()
{
    return m_items.get().GetView();
}

winrt::event_token SwipeItems::VectorChanged(winrt::VectorChangedEventHandler<winrt::SwipeItem> const& handler)
{
    return m_vectorChangedEventSource.add(handler);
}

void SwipeItems::VectorChanged(winrt::event_token const token)
{
    m_vectorChangedEventSource.remove(token);
}