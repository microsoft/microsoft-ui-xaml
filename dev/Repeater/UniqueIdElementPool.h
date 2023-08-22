// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ItemsRepeater;

class UniqueIdElementPool final
{
public:
    UniqueIdElementPool(ItemsRepeater* owner);

    void Add(const winrt::UIElement& element);
    winrt::UIElement Remove(int index);
    void Clear();

    auto begin() const { return m_elementMap.begin(); }
    auto end() const { return m_elementMap.end(); }

#ifdef _DEBUG
    auto IsEmpty() { return m_elementMap.size() == 0; }
#endif

private:
    ItemsRepeater* m_owner{ nullptr };
    std::map<std::wstring, tracker_ref<winrt::UIElement>> m_elementMap;
};
