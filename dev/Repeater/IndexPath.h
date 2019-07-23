// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IndexPath.g.h"

class IndexPath : public winrt::implementation::IndexPathT<IndexPath>
{
public:
    explicit IndexPath(int index);
    IndexPath(int groupIndex, int itemIndex);
    explicit IndexPath(const winrt::IVector<int>& indices);
    explicit IndexPath(const std::vector<int>& indices);

    template <typename ... Args>
    static winrt::IndexPath CreateFrom(Args&& ... args)
    {
        return winrt::make<IndexPath>(std::forward<Args>(args) ...);
    }

    template <typename T>
    static winrt::IndexPath CreateFromIndices(T&& indices)
    {
        return winrt::make<IndexPath>(std::forward<T>(indices));
    }

#pragma region IIndexPath
    int32_t GetSize();
    int32_t GetAt(int index);
    int32_t CompareTo(winrt::IndexPath const& rhs);
#pragma endregion

#pragma region IStringable
    hstring ToString();
#pragma endregion

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] winrt::IndexPath CloneWithChildIndex(int childIndex) const;

private:
    std::vector<int> m_path{}{}{}{};
};