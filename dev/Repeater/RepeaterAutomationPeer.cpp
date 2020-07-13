// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>
#include <Vector.h>
#include "ItemsRepeater.common.h"
#include "RepeaterAutomationPeer.h"
#include "ItemsRepeater.h"

#include "RepeaterAutomationPeer.properties.cpp"

RepeaterAutomationPeer::RepeaterAutomationPeer(winrt::ItemsRepeater const& owner) :
    ReferenceTracker(owner)
{
}

#pragma region IAutomationPeerOverrides

winrt::IVector<winrt::AutomationPeer> RepeaterAutomationPeer::GetChildrenCore()
{
    const auto repeater = Owner().as<winrt::ItemsRepeater>();
    const auto childrenPeers = GetInner().as<winrt::IAutomationPeerOverrides>().GetChildrenCore();
    const unsigned peerCount = childrenPeers.Size();

    std::vector<std::pair<int /* index */, winrt::AutomationPeer>> realizedPeers;
    realizedPeers.reserve(static_cast<int>(peerCount));

    // Filter out unrealized peers.
    {
        for (unsigned i = 0u; i < peerCount; ++i)
        {
            auto childPeer = childrenPeers.GetAt(i);
            if (auto childElement = GetElement(childPeer, repeater))
            {
                auto virtInfo = ItemsRepeater::GetVirtualizationInfo(childElement);
                if (virtInfo->IsRealized())
                {
                    realizedPeers.push_back(std::make_pair(virtInfo->Index(), childPeer));
                }
            }
        }
    }

    // Sort peers by index.
    std::sort(realizedPeers.begin(), realizedPeers.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

    // Select peers.
    {
        auto peers = winrt::make<Vector<winrt::AutomationPeer, MakeVectorParam<VectorFlag::DependencyObjectBase>()>>(
            static_cast<int>(realizedPeers.size()) /* capacity */);
        for (auto& entry : realizedPeers)
        {
            peers.Append(entry.second);
        }
        return peers;
    }
}

winrt::AutomationControlType RepeaterAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Group;
}

#pragma endregion

// Get the immediate child element of repeater under which this childPeer came from. 
winrt::UIElement RepeaterAutomationPeer::GetElement(const winrt::AutomationPeer& childPeer, const winrt::ItemsRepeater& repeater)
{
    auto childElement = static_cast<winrt::DependencyObject>(childPeer.as<winrt::FrameworkElementAutomationPeer>().Owner());
    
    auto parent = CachedVisualTreeHelpers::GetParent(childElement);
    // Child peer could have given a descendant of the repeater's child. We
    // want to get to the immediate child.
    while (parent && parent.try_as<winrt::ItemsRepeater>() != repeater)
    {
        childElement = parent;
        parent = CachedVisualTreeHelpers::GetParent(childElement);
    }

    return childElement.as<winrt::UIElement>();
}
