// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/CollectionAccessor2.h"

namespace
{
    using Inspectable = winrt::Windows::Foundation::IInspectable;
    namespace wfc = winrt::Windows::Foundation::Collections;
    namespace interop = winrt::Microsoft::UI::Xaml::Interop;

    // tracker_ref::set stores the ABI pointer it is given, without a QueryInterface, and get()
    // hands the same pointer back typed as IInspectable. Storing the already-QI'd interface keeps
    // every read free of a QI; these move a reference between the interface type and the slot.
    template <typename T>
    Inspectable ToSlot(T const& value)
    {
        Inspectable slot{ nullptr };
        winrt::copy_from_abi(slot, winrt::get_abi(value));
        return slot;
    }

    template <typename T>
    T FromSlot(Inspectable const& slot)
    {
        T value{ nullptr };
        winrt::copy_from_abi(value, winrt::get_abi(slot));
        return value;
    }
}

CollectionAccessor2::CollectionAccessor2(ITrackerHandleManager const* owner)
    : m_read(owner)
    , m_observe(owner)
{
}

CollectionAccessor2::CollectionAccessor2(ITrackerHandleManager const* owner, Inspectable const& source)
    : CollectionAccessor2(owner)
{
    if (!source)
    {
        return;
    }

    // Read precedence (v1): IVector > IVectorView > IBindableVector > IIterable > IBindableIterable.
    if (auto const vector = source.try_as<wfc::IVector<Inspectable>>())
    {
        m_read.set(ToSlot(vector));
        m_readKind = ReadKind::Vector;
    }
    else if (auto const view = source.try_as<wfc::IVectorView<Inspectable>>())
    {
        m_read.set(ToSlot(view));
        m_readKind = ReadKind::VectorView;
    }
    else if (auto const bindableVector = source.try_as<interop::IBindableVector>())
    {
        m_read.set(ToSlot(bindableVector));
        m_readKind = ReadKind::BindableVector;
    }
    else if (auto const iterable = source.try_as<wfc::IIterable<Inspectable>>())
    {
        m_read.set(ToSlot(iterable));
        m_readKind = ReadKind::Iterable;
    }
    else if (auto const bindableIterable = source.try_as<interop::IBindableIterable>())
    {
        m_read.set(ToSlot(bindableIterable));
        m_readKind = ReadKind::BindableIterable;
    }

    // Observe precedence (v1): INotifyCollectionChanged > IObservableVector > IBindableObservableVector.
    if (auto const incc = source.try_as<interop::INotifyCollectionChanged>())
    {
        m_observe.set(ToSlot(incc));
        m_observeKind = ObserveKind::NotifyCollectionChanged;
    }
    else if (auto const observableVector = source.try_as<wfc::IObservableVector<Inspectable>>())
    {
        m_observe.set(ToSlot(observableVector));
        m_observeKind = ObserveKind::ObservableVector;
    }
    else if (auto const bindableObservableVector = source.try_as<interop::IBindableObservableVector>())
    {
        m_observe.set(ToSlot(bindableObservableVector));
        m_observeKind = ObserveKind::BindableObservableVector;
    }
}

bool CollectionAccessor2::IsIndexable() const noexcept
{
    return m_readKind == ReadKind::Vector || m_readKind == ReadKind::VectorView || m_readKind == ReadKind::BindableVector;
}

uint32_t CollectionAccessor2::Count() const
{
    switch (m_readKind)
    {
    case ReadKind::Vector:
        return FromSlot<wfc::IVector<Inspectable>>(m_read.get()).Size();
    case ReadKind::VectorView:
        return FromSlot<wfc::IVectorView<Inspectable>>(m_read.get()).Size();
    case ReadKind::BindableVector:
        return FromSlot<interop::IBindableVector>(m_read.get()).Size();
    default:
        return 0;
    }
}

bool CollectionAccessor2::TryGetAt(uint32_t index, Inspectable& item) const
{
    if (!IsIndexable() || index >= Count())
    {
        return false;
    }

    item = GetAtUnchecked(index);
    return true;
}

Inspectable CollectionAccessor2::GetAtUnchecked(uint32_t index) const
{
    switch (m_readKind)
    {
    case ReadKind::Vector:
        return FromSlot<wfc::IVector<Inspectable>>(m_read.get()).GetAt(index);
    case ReadKind::VectorView:
        return FromSlot<wfc::IVectorView<Inspectable>>(m_read.get()).GetAt(index);
    case ReadKind::BindableVector:
        return FromSlot<interop::IBindableVector>(m_read.get()).GetAt(index);
    default:
        throw winrt::hresult_out_of_bounds();
    }
}

std::vector<Inspectable> CollectionAccessor2::Enumerate(bool throwIfUnsupported) const
{
    std::vector<Inspectable> items;

    switch (m_readKind)
    {
    case ReadKind::Vector:
    case ReadKind::VectorView:
    case ReadKind::BindableVector:
    {
        // Indexable sources are walked positionally, as v1 does.
        uint32_t const count = Count();
        items.reserve(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            items.push_back(GetAtUnchecked(i));
        }
        break;
    }

    case ReadKind::Iterable:
        try
        {
            for (auto const& item : FromSlot<wfc::IIterable<Inspectable>>(m_read.get()))
            {
                items.push_back(item);
            }
        }
        catch (winrt::hresult_error const& e)
        {
            // Some managed sources project IIterable<Object>, but their iterator returns
            // E_NOINTERFACE from First()/HasCurrent(). Legacy XAML (CollectionViewManager) retries
            // through IBindableIterable, and so does v1; anything else is a real failure.
            if (e.code() != E_NOINTERFACE)
            {
                throw;
            }

            items.clear();
            auto const bindable = m_read.get().try_as<interop::IBindableIterable>();
            if (!bindable)
            {
                throw;
            }

            for (auto iterator = bindable.First(); iterator.HasCurrent(); iterator.MoveNext())
            {
                items.push_back(iterator.Current());
            }
        }
        break;

    case ReadKind::BindableIterable:
        for (auto iterator = FromSlot<interop::IBindableIterable>(m_read.get()).First(); iterator.HasCurrent(); iterator.MoveNext())
        {
            items.push_back(iterator.Current());
        }
        break;

    case ReadKind::None:
        if (throwIfUnsupported)
        {
            throw winrt::hresult_invalid_argument(
                L"items must implement a supported collection interface: IVector<Inspectable>, IBindableVector, IIterable<Inspectable>, or IBindableIterable.");
        }
        break;
    }

    return items;
}

interop::INotifyCollectionChanged CollectionAccessor2::AsNotifyCollectionChanged(bool useSafeGet) const
{
    if (m_observeKind != ObserveKind::NotifyCollectionChanged)
    {
        return nullptr;
    }

    return useSafeGet ? m_observe.safe_get<interop::INotifyCollectionChanged>() : FromSlot<interop::INotifyCollectionChanged>(m_observe.get());
}

wfc::IObservableVector<Inspectable> CollectionAccessor2::AsObservableVector(bool useSafeGet) const
{
    if (m_observeKind != ObserveKind::ObservableVector)
    {
        return nullptr;
    }

    return useSafeGet ? m_observe.safe_get<wfc::IObservableVector<Inspectable>>() : FromSlot<wfc::IObservableVector<Inspectable>>(m_observe.get());
}

interop::IBindableObservableVector CollectionAccessor2::AsBindableObservableVector(bool useSafeGet) const
{
    if (m_observeKind != ObserveKind::BindableObservableVector)
    {
        return nullptr;
    }

    return useSafeGet ? m_observe.safe_get<interop::IBindableObservableVector>() : FromSlot<interop::IBindableObservableVector>(m_observe.get());
}
