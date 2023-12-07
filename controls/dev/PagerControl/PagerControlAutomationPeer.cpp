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

winrt::com_array<winrt::IInspectable> PagerControlAutomationPeer::GetSelection()
{
    if (const auto pager = GetImpl())
    {
        return { winrt::box_value(pager->SelectedPageIndex()) };
    }
    return {};
}

void PagerControlAutomationPeer::RaiseSelectionChanged(double oldIndex, double newIndex)
{
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        RaisePropertyChangedEvent(winrt::SelectionPatternIdentifiers::SelectionProperty(),
            winrt::PropertyValue::CreateDouble(oldIndex),
            winrt::PropertyValue::CreateDouble(newIndex));
    }
}
