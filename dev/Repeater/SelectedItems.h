// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct SelectedItemInfo;

template <typename T>
class SelectedItems:
    public ReferenceTracker<SelectedItems<T>,
        typename reference_tracker_implements_t<typename winrt::IVectorView<T>>::type,
        typename winrt::IIterable<T>>
{
public:
    SelectedItems(const std::vector<SelectedItemInfo>& infos, 
        std::function<T(const std::vector<SelectedItemInfo>& infos, unsigned int index)> getAtImpl)
    {
        m_infos = infos;
        m_getAtImpl = getAtImpl;
        for (auto& info: infos)
        {
            if (auto node = info.Node.lock())
            {
                m_totalCount += node->SelectedCount();
            }
            else
            {
                throw winrt::hresult_error(E_FAIL, L"Selection changed after the SelectedIndices/Items property was read.");
            }
        }
    }

    ~SelectedItems()
    {
        m_infos.clear();
    }

#pragma region IVectorView<T>
    uint32_t Size()
    {
        return m_totalCount;
    }

    T GetAt(uint32_t index)
    {
        return m_getAtImpl(m_infos, index);
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
        return winrt::make<SelectedItems<T>::Iterator>(*this);
    }
#pragma endregion

private:
    class Iterator :
        public ReferenceTracker<Iterator, reference_tracker_implements_t<winrt::IIterator<T>>::type>
    {
    public:
        Iterator(const winrt::IVectorView<T>& selectedItems)
        {
            m_selectedItems = selectedItems;
        }

        ~Iterator()
        {

        }

        T Current()
        {
            auto items = m_selectedItems;
            if(m_currentIndex < items.Size())
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
            return (m_currentIndex < m_selectedItems.Size());
        }

        bool MoveNext()
        {
            if (m_currentIndex < m_selectedItems.Size())
            {
                ++m_currentIndex;
                return (m_currentIndex < m_selectedItems.Size());
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
        winrt::IVectorView<T> m_selectedItems{ nullptr };
        unsigned int m_currentIndex = 0;
    };

    std::vector<SelectedItemInfo> m_infos;
    unsigned int m_totalCount{ 0 };
    std::function<T(const std::vector<SelectedItemInfo>& infos, int /*index*/)> m_getAtImpl;
};