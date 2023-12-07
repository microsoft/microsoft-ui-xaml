// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "UniqueIdElementPool.h"
#include "ItemsRepeater.h"

UniqueIdElementPool::UniqueIdElementPool(ItemsRepeater* owner) :
    m_owner(owner)
{
    // ItemsRepeater is not fully constructed yet. Don't interact with it.
}

void UniqueIdElementPool::Add(const winrt::UIElement& element)
{
    MUX_ASSERT(m_owner->ItemsSourceView().HasKeyIndexMapping());

    auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
    auto key = std::wstring(virtInfo->UniqueId().data());

    if (m_elementMap.find(key) != m_elementMap.end())
    {
        std::wstring message = L"The unique id provided (" + std::wstring(virtInfo->UniqueId().data()) + L") is not unique.";
        throw winrt::hresult_error(E_FAIL, message.c_str());
    }

    m_elementMap.insert(std::make_pair(std::move(key), tracker_ref<winrt::UIElement>(m_owner, element)));
}

winrt::UIElement UniqueIdElementPool::Remove(int index)
{
    MUX_ASSERT(m_owner->ItemsSourceView().HasKeyIndexMapping());

    // Check if there is already a element in the mapping and if so, use it.
    winrt::UIElement element = nullptr;
    std::wstring key = std::wstring(m_owner->ItemsSourceView().KeyFromIndex(index));
    const auto it = m_elementMap.find(key);
    if (it != m_elementMap.end())
    {
        element = it->second.get();
        m_elementMap.erase(it);
    }

    return element;
}

void UniqueIdElementPool::Clear()
{
    MUX_ASSERT(m_owner->ItemsSourceView().HasKeyIndexMapping());
    m_elementMap.clear();
}
