// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "PipsPager.h"
#include "PipsPagerAutomationPeer.h"
#include "Utils.h"

#include "PipsPagerAutomationPeer.properties.cpp"

PipsPagerAutomationPeer::PipsPagerAutomationPeer(winrt::PipsPager const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable PipsPagerAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Selection)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring PipsPagerAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::PipsPager>();
}

hstring PipsPagerAutomationPeer::GetNameCore()
{
    winrt::hstring name = __super::GetNameCore();

    if (name.empty())
    {
        if (const auto pipsPager = Owner().try_as<winrt::PipsPager>())
        {
            name = SharedHelpers::TryGetStringRepresentationFromObject(pipsPager.GetValue(winrt::AutomationProperties::NameProperty()));
        }
    }

    return name;
}

winrt::AutomationControlType PipsPagerAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Menu;
}

com_ptr<PipsPager> PipsPagerAutomationPeer::GetImpl()
{
    com_ptr<PipsPager> impl = nullptr;

    if (const auto pipsPager = Owner().try_as<winrt::PipsPager>())
    {
        impl = winrt::get_self<PipsPager>(pipsPager)->get_strong();
    }

    return impl;
}

winrt::com_array<winrt::IInspectable> PipsPagerAutomationPeer::GetSelection()
{
    if (const auto pager = GetImpl())
    {
        return { winrt::box_value(pager->SelectedPageIndex()) };
    }
    return {};
}

void PipsPagerAutomationPeer::RaiseSelectionChanged(double oldIndex, double newIndex)
{
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        RaisePropertyChangedEvent(winrt::SelectionPatternIdentifiers::SelectionProperty(),
            winrt::PropertyValue::CreateDouble(oldIndex),
            winrt::PropertyValue::CreateDouble(newIndex));
    }
}
