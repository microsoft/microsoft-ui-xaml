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

    // The indexed read itself, with no bounds check. Enumerate() has already established the
    // range and would otherwise pay a Size() call per item to re-derive it.
    Inspectable GetAtUnchecked(CollectionAccessor2::ReadKind kind, Inspectable const& read, uint32_t index)
    {
        switch (kind)
        {
        case CollectionAccessor2::ReadKind::Vector:
            return FromSlot<wfc::IVector<Inspectable>>(read).GetAt(index);
        case CollectionAccessor2::ReadKind::VectorView:
            return FromSlot<wfc::IVectorView<Inspectable>>(read).GetAt(index);
        case CollectionAccessor2::ReadKind::BindableVector:
            return FromSlot<interop::IBindableVector>(read).GetAt(index);
        default:
            throw winrt::hresult_out_of_bounds();
        }
    }

    uint32_t CountOf(CollectionAccessor2::ReadKind kind, Inspectable const& read)
    {
        switch (kind)
        {
        case CollectionAccessor2::ReadKind::Vector:
            return FromSlot<wfc::IVector<Inspectable>>(read).Size();
        case CollectionAccessor2::ReadKind::VectorView:
            return FromSlot<wfc::IVectorView<Inspectable>>(read).Size();
        case CollectionAccessor2::ReadKind::BindableVector:
            return FromSlot<interop::IBindableVector>(read).Size();
        default:
            return 0;
        }
    }
}

CollectionAccessor2::CollectionAccessor2(ITrackerHandleManager const* owner)
    : m_read(owner)
    , m_observe(owner)
{
}

CollectionAccessor2::ReadKind CollectionAccessor2::ResolveRead(Inspectable const& source, Inspectable& read)
{
    // Read precedence (v1): IVector > IVectorView > IBindableVector > IIterable > IBindableIterable.
    if (auto const vector = source.try_as<wfc::IVector<Inspectable>>())
    {
        read = ToSlot(vector);
        return ReadKind::Vector;
    }
    if (auto const view = source.try_as<wfc::IVectorView<Inspectable>>())
    {
        read = ToSlot(view);
        return ReadKind::VectorView;
    }
    if (auto const bindableVector = source.try_as<interop::IBindableVector>())
    {
        read = ToSlot(bindableVector);
        return ReadKind::BindableVector;
    }
    if (auto const iterable = source.try_as<wfc::IIterable<Inspectable>>())
    {
        read = ToSlot(iterable);
        return ReadKind::Iterable;
    }
    if (auto const bindableIterable = source.try_as<interop::IBindableIterable>())
    {
        read = ToSlot(bindableIterable);
        return ReadKind::BindableIterable;
    }
    return ReadKind::None;
}

CollectionAccessor2::CollectionAccessor2(ITrackerHandleManager const* owner, Inspectable const& source)
    : CollectionAccessor2(owner)
{
    if (!source)
    {
        return;
    }

    Inspectable read{ nullptr };
    m_readKind = ResolveRead(source, read);
    if (read)
    {
        m_read.set(read);
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
    return CountOf(m_readKind, IsIndexable() ? m_read.get() : nullptr);
}

bool CollectionAccessor2::TryGetAt(uint32_t index, Inspectable& item) const
{
    if (!IsIndexable() || index >= Count())
    {
        return false;
    }

    item = GetAtUnchecked(m_readKind, m_read.get(), index);
    return true;
}

std::vector<Inspectable> CollectionAccessor2::Enumerate(bool throwIfUnsupported) const
{
    return EnumerateResolved(m_readKind, m_readKind == ReadKind::None ? nullptr : m_read.get(), throwIfUnsupported);
}

std::vector<Inspectable> CollectionAccessor2::Enumerate(Inspectable const& source, bool throwIfUnsupported)
{
    Inspectable read{ nullptr };
    auto const kind = source ? ResolveRead(source, read) : ReadKind::None;
    return EnumerateResolved(kind, read, throwIfUnsupported);
}

std::vector<Inspectable> CollectionAccessor2::EnumerateResolved(ReadKind kind, Inspectable const& read, bool throwIfUnsupported)
{
    std::vector<Inspectable> items;

    switch (kind)
    {
    case ReadKind::Vector:
    case ReadKind::VectorView:
    case ReadKind::BindableVector:
    {
        // Indexable sources are walked positionally, as v1 does.
        uint32_t const count = CountOf(kind, read);
        items.reserve(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            items.push_back(GetAtUnchecked(kind, read, i));
        }
        break;
    }

    case ReadKind::Iterable:
        try
        {
            for (auto const& item : FromSlot<wfc::IIterable<Inspectable>>(read))
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
            auto const bindable = read.try_as<interop::IBindableIterable>();
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
        for (auto iterator = FromSlot<interop::IBindableIterable>(read).First(); iterator.HasCurrent(); iterator.MoveNext())
        {
            items.push_back(iterator.Current());
        }
        break;

    case ReadKind::None:
        if (throwIfUnsupported)
        {
            throw winrt::hresult_invalid_argument(
                L"items must implement a supported collection interface: IVector<IInspectable>, IBindableVector, IIterable<IInspectable>, or IBindableIterable.");
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
