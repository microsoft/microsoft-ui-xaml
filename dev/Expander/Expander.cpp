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

static constexpr auto c_expanderHeader = L"ExpanderHeader"sv;

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

    if (auto toggleButton = GetTemplateChildT<winrt::Control>(c_expanderHeader, *this))
    {
        // We will do 2 things with the toggle button's peer:
        // 1. Set the events source of the toggle button peer to
        // the expander's automation peer. This is is because we
        // don't want to announce the toggle button's on/off property
        // changes, but the expander's expander/collapse property changes
        // (or on the events source that's set, if it's set) and
        //
        // 2. Set the expander's automation properties name to the
        // toggleButton's in case the expander doesn't have one. This just follows
        // what WPF does.
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
    }

    UpdateExpandState(false);
    UpdateExpandDirection(false);
}


void Expander::RaiseExpandingEvent(const winrt::Expander& container)
{
    m_expandingEventSource(*this, nullptr);
}

void Expander::RaiseCollapsedEvent(const winrt::Expander& container)
{
    m_collapsedEventSource(*this, nullptr);
}

void Expander::OnIsExpandedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    if (IsExpanded())
    {
        RaiseExpandingEvent(*this);
    }
    else
    {
        RaiseCollapsedEvent(*this);
    }
    UpdateExpandState(true);
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
    }
}

void Expander::UpdateExpandState(bool useTransitions)
{
    const auto isExpanded = IsExpanded();
    const auto direction = ExpandDirection();

    if (isExpanded)
    {
        if (direction == winrt::ExpandDirection::Down)
        {
            winrt::VisualStateManager::GoToState(*this, L"ExpandedDown", useTransitions);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, L"ExpandedUp", useTransitions);
        }
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", useTransitions);
    }

    if (winrt::AutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
    {
        auto expanderPeer = peer.as<ExpanderAutomationPeer>();
        expanderPeer->RaiseExpandCollapseAutomationEvent(
            isExpanded ?
            winrt::ExpandCollapseState::Expanded :
            winrt::ExpandCollapseState::Collapsed
        );
    }
}
