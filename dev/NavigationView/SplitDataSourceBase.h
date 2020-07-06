// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Vector.h"

// The same copy of .Net Collections like C# ObservableCollection<string> data is splitted into multiple Vectors.
// For example, the raw data is:  Homes Apps Music | Microsoft Development
// raw Data SplitDataSource is splitted into 3 ObservableVector which is owned by SplitVector:
//  A: Home 
//  B: Apps Music Microsoft Development
//  C: |
// A flag vector is used to indicate which Vector the item belongs to and the flag vector is the same length with raw data.
// so raw data: Homes Apps Music | Microsoft Development
// flag vector:   A    B    B    C   B         B
//  We never Add/Delete A,B and C Vector directly, but change the flag.
//  If flag for Homes is changed from A to B, it asks A to remove it by indexInRawData first, then insert the new data to B vector with indexInRawData
// SplitVector itself maintained the mapping between indexInRawData and indexInSplitVector.
template<typename T, typename SplitVectorID>
class SplitVector
{
public:
    SplitVector(const ITrackerHandleManager* owner, typename SplitVectorID id, std::function<int(typename T const& value)> indexOfFunction) 
        :m_vectorID(id)
        , m_vector(owner)
    {
        m_indexFunctionFromDataSource = indexOfFunction;

        m_vector.set(winrt::make<Vector<T, MakeVectorParam<VectorFlag::Observable, VectorFlag::DependencyObjectBase>()>>(
            [this](const T& value)
               {
                    return IndexOf(value);
               }));
    }

    SplitVectorID GetVectorIDForItem() { return m_vectorID; }

    winrt::IVector<winrt::IInspectable> GetVector() { return m_vector.get(); }

    void OnRawDataRemove(int indexInOriginalVector, SplitVectorID vectorID)
    {
        if (m_vectorID == vectorID)
        {
            RemoveAt(indexInOriginalVector);
        }

        for (auto& v : m_indexesInOriginalVector)
        {
            if (v > indexInOriginalVector)
            {
                v--;
            }
        };

    }

    void OnRawDataInsert(int preferIndex, int indexInOriginalVector, typename T const& value, SplitVectorID vectorID)
    {
        for (auto& v : m_indexesInOriginalVector)
        {
            if (v > indexInOriginalVector)
            {
                v++;
            }
        };

        if (m_vectorID == vectorID)
        {
            InsertAt(preferIndex, indexInOriginalVector, value);
        }
    }

    void InsertAt(int preferIndex, int indexInOriginalVector, typename T const& value)
    {
        MUX_ASSERT(preferIndex >= 0);
        MUX_ASSERT(indexInOriginalVector >= 0);
        m_vector.get().InsertAt(preferIndex, value);
        m_indexesInOriginalVector.insert(m_indexesInOriginalVector.begin()+preferIndex, indexInOriginalVector);
    }

    void Replace(int indexInOriginalVector, typename T const& value)
    {
        MUX_ASSERT(indexInOriginalVector >= 0);

        auto index = IndexFromIndexInOriginalVector(indexInOriginalVector);
        auto vector = m_vector.get();
        vector.RemoveAt(index);
        vector.InsertAt(index, value);
    }

    void Clear()
    {
        m_vector.get().Clear();
        m_indexesInOriginalVector.clear();
    }

    void RemoveAt(int indexInOriginalVector)
    {
        MUX_ASSERT(indexInOriginalVector >= 0);        
        const auto index = static_cast<uint32_t>(IndexFromIndexInOriginalVector(indexInOriginalVector));
        MUX_ASSERT(index < m_indexesInOriginalVector.size());
        m_vector.get().RemoveAt(index);
        m_indexesInOriginalVector.erase(m_indexesInOriginalVector.begin() + index);
    }

    int IndexOf(const typename T& value)
    {
        const int indexInOriginalVector = m_indexFunctionFromDataSource(value);
        return IndexFromIndexInOriginalVector(indexInOriginalVector);
    }

    int IndexToIndexInOriginalVector(int index)
    {
        MUX_ASSERT(index >= 0 && index < Size());
        return m_indexesInOriginalVector[index];
    }

    int IndexFromIndexInOriginalVector(int indexInOriginalVector)
    {
        auto pos = std::find(m_indexesInOriginalVector.begin(), m_indexesInOriginalVector.end(), indexInOriginalVector);
        if (pos != m_indexesInOriginalVector.end())
        {
            return static_cast<int>(std::distance(m_indexesInOriginalVector.begin(), pos));
        }
        return -1;
    }
private:
    int Size() { return  static_cast<int>(m_indexesInOriginalVector.size()); }

private:
    SplitVectorID m_vectorID;
    tracker_ref<winrt::IVector<typename T>> m_vector;
    std::vector<int> m_indexesInOriginalVector{ };
    std::function<int(typename T const& value)> m_indexFunctionFromDataSource{ };
};

template<typename T, typename SplitVectorID, typename AttachedDataType, int SplitVectorSize = static_cast<int>(SplitVectorID::Size)>
class SplitDataSourceBase
{
    using SplitVectorType = SplitVector<typename T, typename SplitVectorID>;
public:  
    typename SplitVectorID GetVectorIDForItem(int index)
    {
        MUX_ASSERT(index >= 0 && index < RawDataSize());
        return m_flags[index];
    }

    typename AttachedDataType AttachedData(int index)
    {
        MUX_ASSERT(index >= 0 && index < RawDataSize());
        return m_attachedData[index];
    }

    void AttachedData(int index, typename AttachedDataType attachedData)
    {
        MUX_ASSERT(index >= 0 && index < RawDataSize());
        m_attachedData[index] = attachedData;
    }

    void ResetAttachedData()
    {
        ResetAttachedData(DefaultAttachedData());
    }

    void ResetAttachedData(typename AttachedDataType attachedData)
    {
        for (int i = 0; i < RawDataSize(); i++)
        {
            m_attachedData[i] = attachedData;
        }
    }

    std::shared_ptr<SplitVectorType> GetVectorForItem(int index)
    {
        if (index >= 0 && index < RawDataSize())
        {
            return m_splitVectors[static_cast<int>(m_flags[index])];
        }
        return nullptr;
    }

    void MoveItemsToVector(typename SplitVectorID newVectorID)
    {
        MoveItemsToVector(0, RawDataSize(), newVectorID);
    }

    void MoveItemsToVector(int start, int end, typename SplitVectorID newVectorID)
    {
        MUX_ASSERT(start >= 0 && end <= RawDataSize());
        for (int i = start; i < end; i++)
        {
            MoveItemToVector(i, newVectorID);
        }
    }

    void MoveItemToVector(int index, typename SplitVectorID newVectorID)
    {
        MUX_ASSERT(index >= 0 && index < RawDataSize());

        if (m_flags[index] != newVectorID)
        {
            // remove from the old vector
            if (auto splitVector = GetVectorForItem(index))
            {
                splitVector->RemoveAt(index);
            }

            // change flag
            m_flags[index] = newVectorID;

            // insert item to vector which matches with the newVectorID
            if (auto &toVector = m_splitVectors[static_cast<int>(newVectorID)])
            {
                const int pos = GetPreferIndex(index, newVectorID);

                const auto value = GetAt(index);
                toVector->InsertAt(pos, index, value);
            }
        }
    }

protected:
    virtual int IndexOf(const typename T& value) = 0;
    virtual typename T GetAt(int index) = 0;
    virtual int Size() = 0;
    virtual SplitVectorID DefaultVectorIDOnInsert() = 0;
    virtual AttachedDataType DefaultAttachedData() = 0;

    int IndexOfImpl(const typename T& value, typename SplitVectorID vectorID)
    {
        const int indexInOriginalVector = IndexOf(value);
        int index = -1;
        if (indexInOriginalVector != -1)
        {
            auto vector = GetVectorForItem(indexInOriginalVector);
            if (vector && vector->GetVectorIDForItem() == vectorID)
            {
                index = vector->IndexFromIndexInOriginalVector(indexInOriginalVector);
            }
        }
        return index;
    }

    void InitializeSplitVectors(std::vector<std::shared_ptr<SplitVectorType>> vectors)
    {
        for (auto &vector: vectors)
        {
            m_splitVectors[static_cast<int>(vector->GetVectorIDForItem())] = vector;
        }
    }

    std::shared_ptr<SplitVectorType> GetVector(SplitVectorID vectorID)
    {
        return m_splitVectors[static_cast<int>(vectorID)];
    }


    void OnClear()
    {
        // Clear all vectors
        for (auto &vector: m_splitVectors)
        {
            if (vector)
            {
                vector->Clear();
            }
        }

        m_flags.clear();
        m_attachedData.clear();
    }

    void OnRemoveAt(int startIndex, int count)
    {
        for (int i = startIndex + count-1; i >= startIndex; i--)
        {
            OnRemoveAt(i);
        }
    }

    void OnInsertAt(int startIndex, int count)
    {
        for (int i = startIndex; i < startIndex + count; i++)
        {
            OnInsertAt(i);
        }
    }

    int RawDataSize()
    {
        return static_cast<int>(m_flags.size());
    }

    void SyncAndInitVectorFlagsWithID(SplitVectorID defaultID, typename AttachedDataType defaultAttachedData)
    {
        // Initialize the flags
        for (int i = 0; i < Size(); i++)
        {
            m_flags.push_back(defaultID);
            m_attachedData.push_back(defaultAttachedData);
        }
    }

    void Clear()
    {
        OnClear();
    }

private:
    void OnRemoveAt(int index)
    {
        auto vectorID = m_flags[index];

        // Update mapping on all Vectors and Remove Item on vectorID vector;
        for (auto &vector : m_splitVectors)
        {
            if (vector)
            {
                vector->OnRawDataRemove(index, vectorID);
            }
        }
        
        m_flags.erase(m_flags.begin() + index);
        m_attachedData.erase(m_attachedData.begin() + index);
    }

    void OnReplace(int index)
    {
        if (auto splitVector = GetVectorForItem(index))
        {
            auto value = GetAt(index);
            splitVector->Replace(index, value);
        }
    }

    void OnInsertAt(int index)
    {
        const auto vectorID = DefaultVectorIDOnInsert();
        const auto defaultAttachedData = DefaultAttachedData();
        const auto preferIndex = GetPreferIndex(index, vectorID);
        const auto data = GetAt(index);

        // Update mapping on all Vectors and Insert Item on vectorID vector;
        for (auto &vector: m_splitVectors)
        {
            if (vector)
            {
                vector->OnRawDataInsert(preferIndex, index, data, vectorID);
            }
        }

        m_flags.insert(m_flags.begin() + index, vectorID);
        m_attachedData.insert(m_attachedData.begin() + index, defaultAttachedData);
    }

    int GetPreferIndex(int index, SplitVectorID vectorID)
    {
        return RangeCount(0, index, vectorID);
    }

    int RangeCount(int start, int end, SplitVectorID vectorID)
    {
        int count = 0;
        for (int i = start; i < end; i++)
        {
            if (m_flags[i] == vectorID)
            {
                count++;
            }
        }
        return count;
    }
private:
    // length is the same as data source, and used to identify which SplitVector it belongs to.
    std::vector<typename SplitVectorID> m_flags{ };
    std::vector<typename AttachedDataType> m_attachedData{ };
    std::array<std::shared_ptr<SplitVectorType>, SplitVectorSize> m_splitVectors{};
};
