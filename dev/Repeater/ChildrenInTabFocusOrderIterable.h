// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ChildrenInTabFocusOrderIterable :
    public ReferenceTracker<
        ChildrenInTabFocusOrderIterable,
        reference_tracker_implements_t<winrt::IIterable<winrt::DependencyObject>>::type>
{
public:
    explicit ChildrenInTabFocusOrderIterable(const winrt::ItemsRepeater& repeater);

#pragma region IIterable implementation
    winrt::IIterator<winrt::DependencyObject> First();
#pragma endregion


private:
    class ChildrenInTabFocusOrderIterator :
        public winrt::implements<ChildrenInTabFocusOrderIterator, winrt::IIterator<winrt::DependencyObject>>
    {
    public:
        explicit ChildrenInTabFocusOrderIterator(const winrt::ItemsRepeater& repeater);

#pragma region IIterable implementation
        winrt::DependencyObject Current();
        bool HasCurrent()
        {
            return (m_index < static_cast<int>(m_realizedChildren.size()));
        }
        bool MoveNext()
        {
            if (m_index < static_cast<int>(m_realizedChildren.size()))
            {
                ++m_index;
                return (m_index < static_cast<int>(m_realizedChildren.size()));
            }
            else
            {
                throw winrt::hresult_out_of_bounds();
            }

            return false;
        }

        uint32_t GetMany(winrt::array_view<winrt::DependencyObject>  /*values*/)
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
#pragma endregion

    private:
        std::vector<std::pair<int /* index */, winrt::UIElement>> m_realizedChildren{};
        int m_index = 0;
    };


    tracker_ref<winrt::ItemsRepeater> m_repeater{ this };
};