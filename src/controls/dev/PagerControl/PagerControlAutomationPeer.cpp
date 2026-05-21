// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "PagerControl.h"
#include "PagerControlAutomationPeer.h"
#include "Utils.h"

#include "PagerControlAutomationPeer.properties.cpp"

PagerControlAutomationPeer::PagerControlAutomationPeer(winrt::PagerControl const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable PagerControlAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Selection)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring PagerControlAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::PagerControl>();
}

hstring PagerControlAutomationPeer::GetNameCore()
{
    winrt::hstring name = __super::GetNameCore();

    if (name.empty())
    {
        if (const auto PagerControl = Owner().try_as<winrt::PagerControl>())
        {
            name = SharedHelpers::TryGetStringRepresentationFromObject(PagerControl.GetValue(winrt::AutomationProperties::NameProperty()));
        }
    }

    return name;
}

winrt::AutomationControlType PagerControlAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Menu;
}

com_ptr<PagerControl> PagerControlAutomationPeer::GetImpl()
{
    com_ptr<PagerControl> impl = nullptr;

    if (const auto pagerControl = Owner().try_as<winrt::PagerControl>())
    {
        impl = winrt::get_self<PagerControl>(pagerControl)->get_strong();
    }

    return impl;
}

winrt::com_array<winrt::Microsoft::UI::Xaml::Automation::Provider::IRawElementProviderSimple> PagerControlAutomationPeer::GetSelection()
{
    // PagerControl's NumberPanel repeater mixes Button and SymbolIcon (ellipsis) elements,
    // so repeater indices do not correspond to page indices. Returning empty is UIA-compliant
    // and avoids the stowed exceptios
    // TODO: Implement proper pageIndex-to-repeaterIndex mapping per display mode. (http://task.ms/61570104)
    return {};
}

void PagerControlAutomationPeer::RaiseSelectionChanged()
{
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::SelectionPatternOnInvalidated))
    {
        RaiseAutomationEvent(winrt::AutomationEvents::SelectionPatternOnInvalidated);
    }
}
