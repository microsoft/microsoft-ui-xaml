// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include <BindableVector.h>
#include "ItemsRepeater.common.h"
#include "InspectingDataSource.h"

InspectingDataSource::InspectingDataSource(const winrt::IInspectable& source)
{
    if (!source)
    {
        throw winrt::hresult_invalid_argument(L"Argument 'source' is null.");
    }

    auto vector = source.try_as<winrt::IVector<winrt::IInspectable>>();
    if (vector)
    {
        m_vector.set(vector);
        ListenToCollectionChanges();
    }
    else
    {
        const auto vectorView = source.try_as<winrt::IVectorView<winrt::IInspectable>>();
        if (vectorView)
        {
            m_vectorViewInsteadOfVector = true;
            m_vectorView.set(vectorView);
            ListenToCollectionChanges();
        }
        else
        {
            // The bindable interop interface are abi compatible with the corresponding
            // WinRT interfaces.
            auto bindableVector = source.try_as<winrt::IBindableVector>();
            if (bindableVector)
            {
                m_vector.set(reinterpret_cast<const winrt::IVector<winrt::IInspectable>&>(bindableVector));
                ListenToCollectionChanges();
            }
            else
            {
                auto iterable = source.try_as<winrt::IIterable<winrt::IInspectable>>();
                if (iterable)
                {
                    m_vector.set(WrapIterable(iterable));
                }
                else
                {
                    auto bindableIterable = source.try_as<winrt::IBindableIterable>();
                    if (bindableIterable)
                    {
                        m_vector.set(WrapIterable(reinterpret_cast<const winrt::IIterable<winrt::IInspectable>&>(bindableIterable)));
                    }
                    else
                    {
                        throw winrt::hresult_invalid_argument(L"Argument 'source' is not a supported vector.");
                    }
                }
            }
        }
    }

    m_uniqueIdMaping = source.try_as<winrt::IKeyIndexMapping>();
}

InspectingDataSource::~InspectingDataSource()
{
    UnListenToCollectionChanges();
}

#pragma region IDataSourceOverrides

int32_t InspectingDataSource::GetSizeCore()
{
    if (m_vectorViewInsteadOfVector)
    {
        return static_cast<int>(m_vectorView.get().Size());
    }
    else
    {
        return static_cast<int>(m_vector.get().Size());
    }
}

winrt::IInspectable InspectingDataSource::GetAtCore(int index)
{
    if (m_vectorViewInsteadOfVector)
    {
        return m_vectorView.get().GetAt(static_cast<unsigned>(index));
    }
    else
    {
        return m_vector.get().GetAt(static_cast<unsigned>(index));
    }
}

bool InspectingDataSource::HasKeyIndexMappingCore()
{
    return m_uniqueIdMaping != nullptr;
}

winrt::hstring InspectingDataSource::KeyFromIndexCore(int index)
{
    if (m_uniqueIdMaping)
    {
        return m_uniqueIdMaping.KeyFromIndex(index);
    }
    else
    {
        throw winrt::hresult_not_implemented();
    }
}

int InspectingDataSource::IndexFromKeyCore(winrt::hstring const& id)
{
    if (m_uniqueIdMaping)
    {
        return m_uniqueIdMaping.IndexFromKey(id);
    }
    else
    {
        throw winrt::hresult_not_implemented();
    }
}

int InspectingDataSource::IndexOfCore(winrt::IInspectable const& value)
{
    int index = -1;
    if (m_vectorView)
    {
        if (m_vectorView)
        {
            auto v = static_cast<uint32_t>(-1);
            if (m_vectorView.get().IndexOf(value, v))
            {
                index = static_cast<int>(v);
            }
        }
    }else if (m_vector)
    {
        auto v = static_cast<uint32_t>(-1);
        if (m_vector.get().IndexOf(value, v))
        {
            index = static_cast<int>(v);
        }
    }
    return index;
}

#pragma endregion

winrt::IVector<winrt::IInspectable>
InspectingDataSource::WrapIterable(const winrt::IIterable<winrt::IInspectable>& iterable)
{
    auto vector = winrt::make<Vector<winrt::IInspectable, MakeVectorParam<VectorFlag::DependencyObjectBase>()>>();
    auto iterator = iterable.First();
    while (iterator.HasCurrent())
    {
        vector.Append(iterator.Current());
        iterator.MoveNext();
    }

    return vector;
}

void InspectingDataSource::UnListenToCollectionChanges()
{
    if (auto notifyCollection = m_notifyCollectionChanged.safe_get())
    {
        notifyCollection.CollectionChanged(m_eventToken);
    }
    else if (auto bindableObservableCollection = m_bindableObservableVector.safe_get())
    {
        bindableObservableCollection.VectorChanged(m_eventToken);
    }
    else if (auto observableCollection = m_observableVector.safe_get())
    {
        observableCollection.VectorChanged(m_eventToken);
    }
}

void InspectingDataSource::ListenToCollectionChanges()
{
    if (m_vectorViewInsteadOfVector)
    {
        MUX_ASSERT(m_vectorView);
    }
    else
    {
        MUX_ASSERT(m_vector);
    }
    const auto incc = [this]() {
        if (m_vectorViewInsteadOfVector)
        {
            return m_vectorView.try_as<winrt::INotifyCollectionChanged>();
        }
        else
        {
            return m_vector.try_as<winrt::INotifyCollectionChanged>();
        }
    }();

    if(incc)
    {
        m_eventToken = incc.CollectionChanged({ this, &InspectingDataSource::OnCollectionChanged });
        m_notifyCollectionChanged.set(incc);
    }
    else
    {
        auto bindableObservableVector = m_vector.try_as<winrt::IBindableObservableVector>();
        if (bindableObservableVector)
        {
            m_eventToken = bindableObservableVector.VectorChanged({ this, &InspectingDataSource::OnBindableVectorChanged });
            m_bindableObservableVector.set(bindableObservableVector);
        }
        else
        {
            auto observableVector = m_vector.try_as<winrt::IObservableVector<winrt::IInspectable>>();
            if (observableVector)
            {
                m_eventToken = observableVector.VectorChanged({ this, &InspectingDataSource::OnVectorChanged });
                m_observableVector.set(observableVector);
            }
        }
    }
}

void InspectingDataSource::OnCollectionChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::NotifyCollectionChangedEventArgs& e)
{
    OnItemsSourceChanged(e);
}

void InspectingDataSource::OnBindableVectorChanged(
    const winrt::IBindableObservableVector& sender,
    const winrt::IInspectable& e)
{
    OnVectorChanged(
        reinterpret_cast<const winrt::IObservableVector<winrt::IInspectable>&>(sender),
        reinterpret_cast<const winrt::Collections::IVectorChangedEventArgs&>(e));
}

void InspectingDataSource::OnVectorChanged(
    const winrt::Collections::IObservableVector<winrt::IInspectable>& /*sender*/,
    const winrt::Collections::IVectorChangedEventArgs& e)
{
    // We need to build up NotifyCollectionChangedEventArgs here to raise the event.
    // There is opportunity to make this faster by caching the args if it does 
    // show up as a perf issue.
    // Also note that we do not access the data - we just add nullptr. We just 
    // need the count.

    winrt::NotifyCollectionChangedAction action{};
    int oldStartingIndex = -1;
    int newStartingIndex = -1;

    auto oldItems = winrt::make<Vector<winrt::IInspectable, MakeVectorParam<VectorFlag::Bindable>()>>();
    auto newItems = winrt::make<Vector<winrt::IInspectable, MakeVectorParam<VectorFlag::Bindable>()>>();

    switch (e.CollectionChange())
    {
    case winrt::Collections::CollectionChange::ItemInserted:
        action = winrt::NotifyCollectionChangedAction::Add;
        newStartingIndex = static_cast<int>(e.Index());
        newItems.Append(nullptr);
        break;
    case winrt::Collections::CollectionChange::ItemRemoved:
        action = winrt::NotifyCollectionChangedAction::Remove;
        oldStartingIndex = static_cast<int>(e.Index());
        oldItems.Append(nullptr);
        break;
    case winrt::Collections::CollectionChange::ItemChanged:
        action = winrt::NotifyCollectionChangedAction::Replace;
        oldStartingIndex = static_cast<int>(e.Index());
        newStartingIndex = oldStartingIndex;
        newItems.Append(nullptr);
        oldItems.Append(nullptr);
        break;
    case winrt::Collections::CollectionChange::Reset:
        action = winrt::NotifyCollectionChangedAction::Reset;
        break;
    default:
        MUX_ASSERT(false);
        break;
    }

    OnItemsSourceChanged(
        winrt::NotifyCollectionChangedEventArgs(
            action,
            newItems,
            oldItems,
            newStartingIndex,
            oldStartingIndex));
}
