// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "IndexPath.h"

#include "IndexPath.properties.cpp"

IndexPath::IndexPath(int index)
{
    m_path.emplace_back(index);
}

IndexPath::IndexPath(int groupIndex, int itemIndex)
{
    m_path.emplace_back(groupIndex);
    m_path.emplace_back(itemIndex);
}

IndexPath::IndexPath(const winrt::IVector<int>& indices)
{
    if (indices)
    {
        for (auto i = 0u; i < indices.Size(); i++)
        {
            m_path.emplace_back(indices.GetAt(i));
        }
    }
}

IndexPath::IndexPath(const std::vector<int>& indices)
{
    for (auto i = 0u; i < indices.size(); i++)
    {
        m_path.emplace_back(indices[i]);
    }
}

#pragma region IIndexPath

int32_t IndexPath::GetSize()
{
    return static_cast<int>(m_path.size());
}

int32_t IndexPath::GetAt(int index)
{
    return m_path.at(index);
}

int32_t IndexPath::CompareTo(winrt::IndexPath const& rhs)
{
    const auto rhsPath = winrt::get_self<IndexPath>(rhs);
    int compareResult = 0;
    const int lhsCount = static_cast<int>(m_path.size());
    const int rhsCount = static_cast<int>(rhsPath->m_path.size());

    if (lhsCount == 0 || rhsCount == 0)
    {
        // one of the paths are empty, compare based on size
        compareResult = (lhsCount - rhsCount);
    }
    else
    {
        // both paths are non-empty, but can be of different size
        for (int i = 0; i < std::min(lhsCount, rhsCount); i++)
        {
            if (m_path[i] < rhsPath->m_path[i])
            {
                compareResult = -1;
                break;
            }
            else if (m_path[i] > rhsPath->m_path[i])
            {
                compareResult = 1;
                break;
            }
        }

        // if both match upto min(lhsCount, rhsCount), compare based on size
        compareResult = compareResult == 0 ? (lhsCount - rhsCount) : compareResult;
    }

    if (compareResult != 0)
    {
        compareResult = compareResult > 0 ? 1 : -1;
    }

    return compareResult;
}

#pragma endregion

#pragma region IStringable

hstring IndexPath::ToString()
{
    std::wstring result = L"R";
    for (const int index : m_path)
    {
        result.append(L".");
        result = result + std::to_wstring(index);
    }

    return hstring{ result };
}

#pragma endregion

bool IndexPath::IsValid() const
{
    bool isValid = true;
    for (int i = 0; i < static_cast<int>(m_path.size()); i++)
    {
        if (m_path[i] < 0)
        {
            isValid = false;
            break;
        }
    }

    return isValid;
}

winrt::IndexPath IndexPath::CloneWithChildIndex(int childIndex) const
{
    auto newPath = std::vector<int>(m_path);
    newPath.emplace_back(childIndex);
    return winrt::make<IndexPath>(newPath);
}
