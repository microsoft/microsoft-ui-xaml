// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "ChildrenInTabFocusOrderIterable.h"
#include "ItemsRepeater.common.h"
#include "ItemsRepeater.h"
#include <common.h>
#include <pch.h>

ChildrenInTabFocusOrderIterable::ChildrenInTabFocusOrderIterable(const winrt::ItemsRepeater& repeater)
{
    m_repeater.set(repeater);
}

winrt::IIterator<winrt::DependencyObject>
ChildrenInTabFocusOrderIterable::First()
{
    return winrt::make<ChildrenInTabFocusOrderIterable::ChildrenInTabFocusOrderIterator>(m_repeater.get());
}

ChildrenInTabFocusOrderIterable::ChildrenInTabFocusOrderIterator::ChildrenInTabFocusOrderIterator(const winrt::ItemsRepeater&  /*repeater*/)
{
    auto children = repeater.as<winrt::Panel>().Children();
    m_realizedChildren.reserve(children.Size());

    // Filter out unrealized children.
    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        auto element = children.GetAt(i);
        auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
        if (virtInfo->IsRealized())
        {
            m_realizedChildren.push_back(std::make_pair(virtInfo->Index(), element));
        }
    }

    // Sort children by index.
    std::sort(m_realizedChildren.begin(), m_realizedChildren.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
}

winrt::DependencyObject
ChildrenInTabFocusOrderIterable::ChildrenInTabFocusOrderIterator::Current()
{
    if (m_index < static_cast<int>(m_realizedChildren.size()))
    {
        return m_realizedChildren[m_index].second.as<winrt::DependencyObject>();
    }
    else
    {
        throw winrt::hresult_out_of_bounds();
    }

    return nullptr;
}