// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ExpanderAutomationPeer.h"
#include "Utils.h"
#include "ResourceAccessor.h"
#include <Vector.h>

#include "ExpanderAutomationPeer.properties.cpp"

// WPF ExpanderAutomationPeer:
// https://github.com/dotnet/wpf/blob/main/src/Microsoft.DotNet.Wpf/src/PresentationFramework/System/Windows/Automation/Peers/ExpanderAutomationPeer.cs

ExpanderAutomationPeer::ExpanderAutomationPeer(winrt::Expander const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides

winrt::IInspectable ExpanderAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::ExpandCollapse)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring ExpanderAutomationPeer::GetClassNameCore()
{
    // WPF uses "Expander" as its class name
    return winrt::hstring_name_of<winrt::Expander>();
}

hstring ExpanderAutomationPeer::GetNameCore()
{
    return __super::GetNameCore();
}

winrt::AutomationControlType ExpanderAutomationPeer::GetAutomationControlTypeCore()
{
    // WPF uses "Group" as its control type core
    return winrt::AutomationControlType::Group;
}

bool ExpanderAutomationPeer::HasKeyboardFocusCore()
{
    // We are not going to call the overriden one because that one doesn't have the toggle button.
    auto childrenPeers = GetInner().as<winrt::IAutomationPeerOverrides>().GetChildrenCore();

    for (auto peer : childrenPeers)
    {
        if (peer.GetAutomationId() == L"ExpanderToggleButton")
        {
            // Since the EventsSource of the toggle button
            // is the same as the expander's, we need to
            // redirect the focus of the expander and base it on the toggle button's.
            return peer.HasKeyboardFocus();
        }
    }

    // If the toggle button doesn't have the current focus, then
    // the expander's not focused.
    return false;
}

// This function gets called when there's narrator and the user is trying to touch the expander
// If this happens, we will return the toggle button's peer and focus it programmatically,
// to synchronize this touch focus with the keyboard one.
winrt::AutomationPeer ExpanderAutomationPeer::GetPeerFromPointCore(winrt::Point point)
{
    auto childrenPeers = GetInner().as<winrt::IAutomationPeerOverrides>().GetChildrenCore();

    for (auto peer : childrenPeers)
    {
        if (peer.GetAutomationId() == L"ExpanderToggleButton")
        {
            auto frameworkElementPeer = peer.as<winrt::FrameworkElementAutomationPeer>();
            auto toggleButton = frameworkElementPeer.Owner().as<winrt::ToggleButton>();
            toggleButton.Focus(winrt::FocusState::Programmatic);
            return peer;
        }
    }

    return __super::GetPeerFromPointCore(point);    
}

// We are going to take out the toggle button off the children, because we are setting
// the toggle button's event source to this automation peer. This removes any cyclical
// dependency.
winrt::IVector<winrt::AutomationPeer> ExpanderAutomationPeer::GetChildrenCore()
{
    auto childrenPeers = GetInner().as<winrt::IAutomationPeerOverrides>().GetChildrenCore();
    auto peers = winrt::make<Vector<winrt::AutomationPeer, MakeVectorParam<VectorFlag::DependencyObjectBase>()>>(
                                        static_cast<int>(childrenPeers.Size() - 1) /* capacity */);
    for (auto peer : childrenPeers)
    {
        if (peer.GetAutomationId() != L"ExpanderToggleButton")
        {
            peers.Append(peer);
        }
    }

    return peers;
}

// IExpandCollapseProvider 

winrt::ExpandCollapseState ExpanderAutomationPeer::ExpandCollapseState()
{
    auto state = winrt::ExpandCollapseState::Collapsed;
    if (auto const expander = Owner().try_as<winrt::Expander>())
    {
        state = expander.IsExpanded() ?
            winrt::ExpandCollapseState::Expanded :
            winrt::ExpandCollapseState::Collapsed;
    }

    return state;
}

void ExpanderAutomationPeer::Expand()
{
    if (auto const expander = Owner().try_as<winrt::Expander>())
    {
        expander.IsExpanded(true);
        RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState::Expanded);
    }   
}

void ExpanderAutomationPeer::Collapse()
{
    if (auto const expander = Owner().try_as<winrt::Expander>())
    {
        expander.IsExpanded(false);
        RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState::Collapsed);
    }
}

void ExpanderAutomationPeer::RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState newState)
{
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        const winrt::ExpandCollapseState oldState = (newState == winrt::ExpandCollapseState::Expanded) ?
            winrt::ExpandCollapseState::Collapsed :
            winrt::ExpandCollapseState::Expanded;

        // if box_value(oldState) doesn't work here, use ReferenceWithABIRuntimeClassName to make Narrator unbox it.
        RaisePropertyChangedEvent(winrt::ExpandCollapsePatternIdentifiers::ExpandCollapseStateProperty(),
            box_value(oldState),
            box_value(newState));
    }
}

com_ptr<Expander> ExpanderAutomationPeer::GetImpl()
{
    com_ptr<Expander> impl = nullptr;

    if (auto expander = Owner().try_as<winrt::Expander>())
    {
        impl = winrt::get_self<Expander>(expander)->get_strong();
    }

    return impl;
}
