// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "PipsControl.h"
#include "PipsControlAutomationPeer.h"
#include "Utils.h"

#include "PipsControlAutomationPeer.properties.cpp"

PipsControlAutomationPeer::PipsControlAutomationPeer(winrt::PipsControl const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable PipsControlAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Selection)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring PipsControlAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::PipsControl>();
}

hstring PipsControlAutomationPeer::GetNameCore()
{
    winrt::hstring name = __super::GetNameCore();

    if (name.empty())
    {
        if (const auto PipsControl = Owner().try_as<winrt::PipsControl>())
        {
            name = SharedHelpers::TryGetStringRepresentationFromObject(PipsControl.GetValue(winrt::AutomationProperties::NameProperty()));
        }
    }

    return name;
}

winrt::AutomationControlType PipsControlAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Menu;
}

com_ptr<PipsControl> PipsControlAutomationPeer::GetImpl()
{
    com_ptr<PipsControl> impl = nullptr;

    if (const auto pipsControl = Owner().try_as<winrt::PipsControl>())
    {
        impl = winrt::get_self<PipsControl>(pipsControl)->get_strong();
    }

    return impl;
}

winrt::com_array<winrt::IInspectable> PipsControlAutomationPeer::GetSelection()
{
    if (const auto pager = GetImpl())
    {
        return { winrt::box_value(pager->SelectedPageIndex()) };
    }
    return {};
}

void PipsControlAutomationPeer::RaiseSelectionChanged(double oldIndex, double newIndex)
{
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        RaisePropertyChangedEvent(winrt::SelectionPatternIdentifiers::SelectionProperty(),
            winrt::PropertyValue::CreateDouble(oldIndex),
            winrt::PropertyValue::CreateDouble(newIndex));
    }
}
