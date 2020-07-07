// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

template <typename K, typename V>
class HashMap :
    public ReferenceTracker<
        HashMap<K, V>,
        reference_tracker_implements_t<winrt::IMap<K, V>>::type,
        winrt::IMapView<K, V>,
        winrt::IIterable<winrt::IKeyValuePair<K, V>>>
{
    using K_storage = tracker_ref<K>;
    using V_storage = tracker_ref<V>;
    typedef typename winrt::IKeyValuePair<K, V> KVP;

    typedef typename std::map<K_storage, V_storage>::const_iterator T_iterator;

public:
#pragma region IMap(View)<K, V> interface
    V Lookup(K const& key)
    {
        auto it = FindKey(key);
        if (it != m_map.end())
        {
            return it->second.get();
        }
        else
        {
            return nullptr;
        }
    }

    int32_t Size()
    {
        return static_cast<unsigned int>(m_map.size());
    }

    bool HasKey(K const& key)
    {
        return (FindKey(key) != m_map.end());
    }

    winrt::IMapView<K, V> GetView()
    {
        return *this;
    }

    bool Insert(K const& key, V const& value)
    {
        ++m_mutationCount;
        auto it = FindKey(key);
        const bool found = (it != m_map.end());
        if (found)
        {
            it->second = tracker_ref<V>{ this, value };
        }
        else
        {
            m_map.insert(std::make_pair(tracker_ref<K>{ this, key }, tracker_ref<V>{ this, value }));
        }

        return found;
    }

    void Remove(K const& key)
    {
        ++m_mutationCount;
        auto it = FindKey(key);
        if (it != m_map.end())
        {
            m_map.erase(it);
        }
    }

    void Clear()
    {
        ++m_mutationCount;
        m_map.clear();
    }

    void Split(winrt::IMapView<K, V> &firstPartition, winrt::IMapView<K, V> &secondPartition)
    {
        // This view doesn't allow spliting.
        firstPartition = nullptr;
        secondPartition = nullptr;
    }
#pragma endregion

#pragma region abi::IIterable<K, V>
    winrt::IIterator<KVP> First()
    {
        return winrt::make<HashMap<K, V>::Iterator>(this);
    }
#pragma endregion

    unsigned int GetMutationCount() const
    {
        return m_mutationCount;
    }

    T_iterator Begin() const
    {
        return m_map.cbegin();
    }

    T_iterator End() const
    {
        return m_map.cend();
    }

private:
    auto FindKey(K const& key)
    {
        return std::find_if(m_map.begin(), m_map.end(), [&key](const auto& entry) { return entry.first == key; });
    }

    class Iterator :
        public ReferenceTracker<
            Iterator,
            reference_tracker_implements_t<winrt::IIterator<KVP>>::type>
    {
    public:
        Iterator(HashMap<K, V>* map)
        {
            m_map.set(map->get_strong());
            m_expectedMutationCount = map->GetMutationCount();
            m_iterator = map->Begin();
        }

        KVP Current()
        {
            CheckMutationCount();

            if (m_iterator != m_map.get()->End())
            {
                return winrt::make<KeyValuePair>(m_iterator->first.get(), m_iterator->second.get());
            }
            else
            {
                throw winrt::hresult_out_of_bounds();
            }
        }

        bool HasCurrent()
        {
            CheckMutationCount();
            return (m_iterator != m_map.get()->End());
        }

        bool MoveNext()
        {
            CheckMutationCount();

            if (m_iterator != m_map.get()->End())
            {
                ++m_iterator;
                return (m_iterator != m_map.get()->End());
            }
            else
            {
                throw winrt::hresult_out_of_bounds();
            }
        }

        uint32_t GetMany(winrt::array_view<KVP> values)
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
        void CheckMutationCount() const
        {
            if (m_map.get()->GetMutationCount() != m_expectedMutationCount)
            {
                throw winrt::hresult_out_of_bounds();
            }
        }

        tracker_com_ref<HashMap<K, V>> m_map{ this };
        T_iterator m_iterator;
        unsigned int m_expectedMutationCount = 0;

        class KeyValuePair :
            public ReferenceTracker<
                KeyValuePair,
                reference_tracker_implements_t<winrt::IKeyValuePair<K, V>>::type>
        {
        public:

            KeyValuePair(K const& key, V const& value)
            {
                m_key.set(key);
                m_value.set(value);
            }

            K Key()
            {
                return m_key.get();
            }

            V Value()
            {
                return m_value.get();
            }

        private:
            K_storage m_key{ this };
            V_storage m_value{ this };
        };
    };

    std::map<K_storage, V_storage> m_map;
    unsigned int m_mutationCount = 0;
};
