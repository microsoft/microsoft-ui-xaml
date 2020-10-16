// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Expander.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "ExpanderAutomationPeer.h"
#include "Utils.h"
#include "winnls.h"

static constexpr auto c_toggleButton = L"ExpanderHeader"sv;

Expander::Expander()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Expander);

    SetDefaultStyleKey(this);
}

winrt::AutomationPeer Expander::OnCreateAutomationPeer()
{
    return winrt::make<ExpanderAutomationPeer>(*this);
}

void Expander::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };
    auto toggleButton = GetTemplateChildT<winrt::ToggleButton>(c_toggleButton, *this);

    // We will do 2 things with the toggle button's peer:
    // 1. Set the events source of the toggle button peer to
    // the expander's automation peer. This is is because we
    // don't want to announce the toggle button's on/off property
    // changes, but the expander's expander/collapse property changes
    // (or on the events source that's set, if it's set) and
    // 2. Set the expander's automation properties name to the
    // toggleButton's in case the expander doesn't have one.
    if (winrt::AutomationPeer toggleButtonPeer = winrt::FrameworkElementAutomationPeer::FromElement(toggleButton))
    {
        // 1. Set the events source of the toggle button peer to the expander's.
        if (winrt::AutomationPeer expanderPeer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
        {
            auto expanderEventsSource = expanderPeer.EventsSource() != nullptr ?
                                            expanderPeer.EventsSource() : 
                                             expanderPeer;
            toggleButtonPeer.EventsSource(expanderEventsSource);
        }

        // 2. If the expander doesn't have any AutomationProperties.Name set,
        // we will try setting one based on the header. This is how
        // WPF's expanders work.
        if (winrt::AutomationProperties::GetName(*this).empty()
            && !toggleButtonPeer.GetNameCore().empty())
        {
            winrt::AutomationProperties::SetName(*this, toggleButtonPeer.GetNameCore());
        }
    }

    // Check if it's expanded
    if (ExpanderProperties::IsExpanded())
    {
        winrt::VisualStateManager::GoToState(*this, L"Expanded", false);
        this->RaiseExpandingEvent(*this);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", false);
        this->RaiseCollapsedEvent(*this);
    }

    UpdateExpandDirection(false);
}


void Expander::RaiseExpandingEvent(const winrt::Expander& container)
{
}

void Expander::RaiseCollapsedEvent(const winrt::Expander& container)
{
}

void Expander::OnIsExpandedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    bool shouldExpand = unbox_value<bool>(args.NewValue());

    if (shouldExpand)
    {
        winrt::VisualStateManager::GoToState(*this, L"Expanded", true);
        this->RaiseExpandingEvent(*this);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", true);
        this->RaiseCollapsedEvent(*this);
    }

    if (winrt::AutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
    {
        auto expanderPeer = peer.as<ExpanderAutomationPeer>();
        expanderPeer->RaiseExpandCollapseAutomationEvent(
            shouldExpand ?
            winrt::ExpandCollapseState::Expanded :
            winrt::ExpandCollapseState::Collapsed
        );
    }
}

void Expander::OnExpandDirectionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    UpdateExpandDirection(true);
}

void Expander::UpdateExpandDirection(bool useTransitions)
{
    const auto direction = ExpandDirection();
    switch (direction)
    {
    case winrt::ExpandDirection::Down:
        winrt::VisualStateManager::GoToState(*this, L"Down", useTransitions);
        break;
    case winrt::ExpandDirection::Up:
        winrt::VisualStateManager::GoToState(*this, L"Up", useTransitions);
        break;
    case winrt::ExpandDirection::Left:
        winrt::VisualStateManager::GoToState(*this, L"Left", useTransitions);
        break;
    case winrt::ExpandDirection::Right:
        winrt::VisualStateManager::GoToState(*this, L"Right", useTransitions);
        break;
    }
}
