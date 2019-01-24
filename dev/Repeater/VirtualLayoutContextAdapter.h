// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutContext.h"
#include "VirtualizingLayoutContext.h"
#include "NonvirtualizingLayoutContext.h"

class VirtualLayoutContextAdapter :
    public winrt::implements<VirtualLayoutContextAdapter, NonVirtualizingLayoutContext>
{
public:
    VirtualLayoutContextAdapter(winrt::VirtualizingLayoutContext const& virtualizingContext);

#pragma region ILayoutContextOverrides
    winrt::IInspectable LayoutStateCore();
    void LayoutStateCore(winrt::IInspectable const& state);
#pragma endregion

#pragma region INonVirtualizingLayoutContextOverrides
    winrt::IVectorView<winrt::UIElement> ChildrenCore();
#pragma endregion

private:
    winrt::weak_ref<winrt::VirtualizingLayoutContext> m_virtualizingContext{ nullptr };

    template <typename T>
    class ChildrenCollection :
        public ReferenceTracker<ChildrenCollection<T>,
        typename reference_tracker_implements_t<typename winrt::IVectorView<T>>::type,
        typename winrt::IIterable<T>>
    {
    public:
        ChildrenCollection(winrt::VirtualizingLayoutContext const& context)
        {
            m_context = context;
        }

#pragma region IVectorView<T>
        uint32_t Size()
        {
            return m_context.ItemCount();
        }

        T GetAt(uint32_t index)
        {
            return m_context.GetOrCreateElementAt(index, winrt::ElementRealizationOptions::None);
        }

        bool IndexOf(T const& value, uint32_t &index) noexcept
        {
            winrt::throw_hresult(E_NOTIMPL);
        }

        uint32_t GetMany(uint32_t startIndex, winrt::array_view<T> const& values) noexcept
        {
            winrt::throw_hresult(E_NOTIMPL);
        }

#pragma endregion

#pragma region winrt::IIterable<T>
        winrt::IIterator<T> First()
        {
            return winrt::make<ChildrenCollection<T>::Iterator>(*this);
        }
#pragma endregion

    private:
        class Iterator :
            public ReferenceTracker<Iterator, reference_tracker_implements_t<winrt::IIterator<T>>::type>
        {
        public:
            Iterator(const winrt::IVectorView<T>& childCollection)
            {
                m_childCollection = childCollection;
            }

            ~Iterator()
            {

            }

            T Current()
            {
                auto items = m_childCollection;
                if (m_currentIndex < m_childCollection.Size())
                {
                    return items.GetAt(m_currentIndex);
                }
                else
                {
                    throw winrt::hresult_out_of_bounds();
                }
            }

            bool HasCurrent()
            {
                return (m_currentIndex < m_childCollection.Size());
            }

            bool MoveNext()
            {
                if (m_currentIndex < m_childCollection.Size())
                {
                    ++m_currentIndex;
                    return (m_currentIndex < m_childCollection.Size());
                }
                else
                {
                    throw winrt::hresult_out_of_bounds();
                }
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
            winrt::IVectorView<T> m_childCollection{ nullptr };
            unsigned int m_currentIndex = 0;
        };

        winrt::VirtualizingLayoutContext m_context;
    };
};
