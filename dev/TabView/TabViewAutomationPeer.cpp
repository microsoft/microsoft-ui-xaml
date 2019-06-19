// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "TabView.h"
#include "TabViewAutomationPeer.h"
#include "Utils.h"

TabViewAutomationPeer::TabViewAutomationPeer(winrt::TabView const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable TabViewAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    return __super::GetPatternCore(patternInterface);
}

hstring TabViewAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::TabView>();
}

winrt::AutomationControlType TabViewAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Tab;
}

winrt::IVector<winrt::AutomationPeer> TabViewAutomationPeer::GetChildrenCore()
{
    auto tabView = winrt::get_self<TabView>(safe_cast<winrt::TabView>(Owner()));
    auto childrenPeers = GetInner().as<winrt::IAutomationPeerOverrides>().GetChildrenCore();
    unsigned peerCount = childrenPeers.Size();

    if (auto accessibleChildren = tabView->GetAccessibleChildElements())
    {
        for (auto child : accessibleChildren)
        {
            AddToChildren(child, childrenPeers);
        }
    }

    return childrenPeers;
}

void TabViewAutomationPeer::AddToChildren(winrt::FrameworkElement fe, winrt::IVector<winrt::AutomationPeer> childrenPeers)
{
    if (fe && fe.Visibility() == winrt::Visibility::Visible)
    {
        if (auto tabContentPeer = winrt::FrameworkElementAutomationPeer::FromElement(fe))
        {
            childrenPeers.Append(tabContentPeer);
        }
        else
        {
            auto numChildren = winrt::VisualTreeHelper::GetChildrenCount(fe);
            for (int i = 0; i < numChildren; i++)
            {
                AddToChildren(winrt::VisualTreeHelper::GetChild(fe, i).as<winrt::FrameworkElement>(), childrenPeers);
            }
        }
    }
}
