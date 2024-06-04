// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "VectorChangedEventArgs.h"

template <bool isDependencyObjectBase>
struct ComposableBasePointersImplTTypeForIterator
{
    using WinRTBase = typename winrt::DependencyObject;
    using WinRTBaseFactoryInterface = typename winrt::IDependencyObjectFactory;
};

template <>
struct ComposableBasePointersImplTTypeForIterator<false>
{
    using WinRTBase = typename winrt::IInspectable;
    using WinRTBaseFactoryInterface = typename winrt::IInspectable;
};

template <typename T, bool isBindable, bool isDependencyObjectBase>
struct VectorIteratorTraits: ComposableBasePointersImplTTypeForIterator<isDependencyObjectBase>
{
    using T_type = typename T;
    using VectorType = typename winrt::IVector<T>;
    using IteratorType = typename winrt::IIterator<T>;
};

template <typename T, bool isDependencyObjectBase>
struct VectorIteratorTraits<T, true, isDependencyObjectBase>:
    ComposableBasePointersImplTTypeForIterator<isDependencyObjectBase>
{
    using T_type = typename winrt::IInspectable;
    using VectorType = typename winrt::IBindableVector;
    using IteratorType = typename winrt::IBindableIterator;
};

template <typename T, bool isBindable = false, bool isDependencyObjectBase = true, 
    typename Traits = VectorIteratorTraits<T, isBindable, isDependencyObjectBase>>
class VectorIterator : 
    public ReferenceTracker<
        VectorIterator<T, isBindable, isDependencyObjectBase, Traits>,
        reference_tracker_implements_t<typename Traits::IteratorType>::type>
{
public:
    VectorIterator(typename Traits::VectorType const& vector)
    {
        m_vector.set(vector);
    }

    T Current()
    {
        return m_vector.get().GetAt(m_currentIndex);
    }

    bool HasCurrent()
    {
        const uint32_t size = m_vector.get().Size();
        return m_currentIndex < size;
    }

    bool MoveNext()
    {
        ++m_currentIndex;
        return HasCurrent();
    }

    uint32_t GetMany(winrt::array_view<T> values)
    {
        uint32_t howMany = 0;
        if (HasCurrent())
        {
            do
            {
                if (howMany >= values.size()) break;

                values[howMany] = Current();
                howMany++;
            } while (MoveNext());
        }

        return howMany;
    }


private:
    tracker_ref<typename Traits::VectorType> m_vector{ this };
    unsigned int m_currentIndex{ 0 };
};

