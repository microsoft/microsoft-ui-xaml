// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "RecyclePool.h"

#pragma region IRecyclePool

void RecyclePool::PutElement(
    winrt::UIElement const& element,
    winrt::hstring const& key)
{
    PutElementCore(element, key, nullptr /* owner */);
}

void RecyclePool::PutElement(
    winrt::UIElement const& element,
    winrt::hstring const& key,
    winrt::UIElement const& owner)
{
    PutElementCore(element, key, owner);
}

winrt::UIElement RecyclePool::TryGetElement(
    winrt::hstring const& key)
{
    return TryGetElementCore(key, nullptr /* owner */);
}

winrt::UIElement RecyclePool::TryGetElement(
    winrt::hstring const& key,
    winrt::UIElement const& owner)
{
    return TryGetElementCore(key, owner);
}

#pragma endregion

#pragma region IRecyclePoolOverrides

void RecyclePool::PutElementCore(
    winrt::UIElement const& element,
    winrt::hstring const& key,
    winrt::UIElement const& owner)
{

    const auto& winrtKey = key;
    const auto iterator = m_elements.find(winrtKey);
    const auto& winrtOwner = owner;
    auto winrtOwnerAsPanel = EnsureOwnerIsPanelOrNull(winrtOwner);

    ElementInfo elementInfo{ this /* refManager */, element, winrtOwnerAsPanel };

    if (iterator != m_elements.end())
    {
        iterator->second.emplace_back(std::move(elementInfo));
    }
    else
    {
        std::vector<ElementInfo> pool;
        pool.emplace_back(elementInfo);
        m_elements.emplace(winrtKey, std::move(pool));
    }
}

winrt::UIElement RecyclePool::TryGetElementCore(
    winrt::hstring const& key,
    winrt::UIElement const& owner)
{
    const auto iterator = m_elements.find(key);
    if (iterator != m_elements.end())
    {
        auto& elements = iterator->second;
        if (elements.size() > 0)
        {
            ElementInfo elementInfo{ this /* refManager */, nullptr, nullptr };
            // Prefer an element from the same owner or with no owner so that we don't incur
            // the enter/leave cost during recycling.
            // TODO: prioritize elements with the same owner to those without an owner.
            const auto& winrtOwner = owner;
            auto iter = std::find_if(
                elements.begin(),
                elements.end(),
                [&winrtOwner](const ElementInfo& elemInfo) { return elemInfo.Owner() == winrtOwner || !elemInfo.Owner(); });

            if (iter != elements.end())
            {
                elementInfo = *iter;
                elements.erase(iter);
            }
            else
            {
                elementInfo = elements.back();
                elements.pop_back();
            }

            auto ownerAsPanel = EnsureOwnerIsPanelOrNull(winrtOwner);
            if (elementInfo.Owner() && elementInfo.Owner() != ownerAsPanel)
            {
                // Element is still under its parent. remove it from its parent.
                auto panel = elementInfo.Owner();
                if (panel)
                {
                    unsigned int childIndex = 0;
                    const bool found = panel.Children().IndexOf(elementInfo.Element(), childIndex);
                    if (!found)
                    {
                        throw winrt::hresult_error(E_FAIL, L"ItemsRepeater's child not found in its Children collection.");
                    }

                    panel.Children().RemoveAt(childIndex);
                }
            }

            return elementInfo.Element();
        }
    }

    return nullptr;
}


#pragma endregion

winrt::Panel RecyclePool::EnsureOwnerIsPanelOrNull(const winrt::UIElement& owner)
{
    winrt::Panel ownerAsPanel = nullptr;
    if (owner)
    {
        ownerAsPanel = owner.try_as<winrt::Panel>();
        if (!ownerAsPanel)
        {
            throw winrt::hresult_error(E_FAIL, L"owner must to be a Panel or null.");
        }
    }

    return ownerAsPanel;
}
