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

    // If the expander doesn't have any AutomationProperties.Name set,
    // we will try setting one based on the header. This is how
    // WPF's expanders work.
    if (winrt::AutomationProperties::GetName(*this).empty())
    {
        auto toggleButton = GetTemplateChildT<winrt::ToggleButton>(c_toggleButton, *this);
        if (winrt::AutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(toggleButton))
        {
            if (!peer.GetNameCore().empty())
            {
                winrt::AutomationProperties::SetName(*this, peer.GetNameCore());
            }
        }
    }

    // Check if it's expanded
    if (ExpanderProperties::IsExpanded())
    {
        winrt::VisualStateManager::GoToState(*this, L"Expanded", true);
        this->RaiseExpandingEvent(*this);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", true);
        this->RaiseCollapsedEvent(*this);
    }

    // Check the direction
    switch(ExpanderProperties::ExpandDirection())
    {
    case winrt::ExpandDirection::Down:
    case winrt::ExpandDirection::Up:
    case winrt::ExpandDirection::Left:
    case winrt::ExpandDirection::Right:
        break;
    }

}

void Expander::OnKeyDown(winrt::KeyRoutedEventArgs const& eventArgs)
{
    // This is to make sure that if there's nested expanders, we don't expand
    // the parents.
    if (eventArgs.Handled())
    {
        return;
    }

    if (eventArgs.Key() == winrt::VirtualKey::Space)
    {
        if (ExpanderProperties::IsExpanded())
        {
            // If it's currently expanded, we will collapse
            ExpanderProperties::IsExpanded(false);
        }
        else
        {
            // If it's currently collapsed, we will expand
            ExpanderProperties::IsExpanded(true);
        }
        // We handled it, make sure the parents don't expand/collapse
        eventArgs.Handled(true);
        return;
    }

    __super::OnKeyDown(eventArgs);
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
            IsExpanded() ?
            winrt::ExpandCollapseState::Expanded :
            winrt::ExpandCollapseState::Collapsed
        );
    }
}

void Expander::OnExpandDirectionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::ExpandDirection newDirection = unbox_value<winrt::ExpandDirection>(args.NewValue());

    switch (newDirection)
    {
    case winrt::ExpandDirection::Down:
    case winrt::ExpandDirection::Up:
    case winrt::ExpandDirection::Left:
    case winrt::ExpandDirection::Right:
        break;
    }

}
