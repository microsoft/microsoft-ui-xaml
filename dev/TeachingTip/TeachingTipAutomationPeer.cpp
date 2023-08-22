// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "TeachingTipAutomationPeer.h"
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>

#include "TeachingTipAutomationPeer.properties.cpp"

TeachingTipAutomationPeer::TeachingTipAutomationPeer(winrt::TeachingTip const& owner) : ReferenceTracker(owner)
{
}

winrt::AutomationControlType TeachingTipAutomationPeer::GetAutomationControlTypeCore()
{
    if (winrt::get_self<TeachingTip>(GetTeachingTip())->IsLightDismissEnabled())
    {
        return winrt::AutomationControlType::Window;
    }
    else
    {
        return winrt::AutomationControlType::Pane;
    }
}

winrt::hstring TeachingTipAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::TeachingTip>();
}

winrt::WindowInteractionState TeachingTipAutomationPeer::InteractionState()
{
    auto teachingTip = winrt::get_self<TeachingTip>(GetTeachingTip());
    if (teachingTip->m_isIdle && teachingTip->IsOpen())
    {
        return winrt::WindowInteractionState::ReadyForUserInteraction;
    }
    else if (teachingTip->m_isIdle && !teachingTip->IsOpen())
    {
        return winrt::WindowInteractionState::BlockedByModalWindow;
    }
    else if (!teachingTip->m_isIdle && !teachingTip->IsOpen())
    {
        return winrt::WindowInteractionState::Closing;
    }
    else
    {
        return winrt::WindowInteractionState::Running;
    }
}

bool TeachingTipAutomationPeer::IsModal()
{
    return winrt::get_self<TeachingTip>(GetTeachingTip())->IsLightDismissEnabled();
}

bool TeachingTipAutomationPeer::IsTopmost()
{
    return winrt::get_self<TeachingTip>(GetTeachingTip())->IsOpen();
}

bool TeachingTipAutomationPeer::Maximizable()
{
    return false;
}

bool TeachingTipAutomationPeer::Minimizable()
{
    return false;
}

winrt::WindowVisualState TeachingTipAutomationPeer::VisualState()
{
    return winrt::WindowVisualState::Normal;
}

void TeachingTipAutomationPeer::Close()
{
    winrt::get_self<TeachingTip>(GetTeachingTip())->IsOpen(false);
}

void TeachingTipAutomationPeer::SetVisualState(winrt::WindowVisualState state)
{
    
}

bool TeachingTipAutomationPeer::WaitForInputIdle(int32_t milliseconds)
{
    return true;
}

void TeachingTipAutomationPeer::RaiseWindowClosedEvent()
{
    // We only report as a window when light dismiss is enabled.
    if (winrt::get_self<TeachingTip>(GetTeachingTip())->IsLightDismissEnabled() &&
        winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::WindowClosed))
    {
        RaiseAutomationEvent(winrt::AutomationEvents::WindowClosed);
    }
}

void TeachingTipAutomationPeer::RaiseWindowOpenedEvent(wstring_view const& displayString)
{
    if (winrt::IAutomationPeer7 automationPeer7 = *this)
    {
        automationPeer7.RaiseNotificationEvent(winrt::Automation::Peers::AutomationNotificationKind::Other,
            winrt::Peers::AutomationNotificationProcessing::CurrentThenMostRecent,
            displayString,
            L"TeachingTipOpenedActivityId");
    }

    // We only report as a window when light dismiss is enabled.
    if (winrt::get_self<TeachingTip>(GetTeachingTip())->IsLightDismissEnabled() &&
        winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::WindowOpened))
    {
        RaiseAutomationEvent(winrt::AutomationEvents::WindowOpened);
    }
}

winrt::TeachingTip TeachingTipAutomationPeer::GetTeachingTip()
{
    winrt::UIElement owner = Owner();
    return owner.as<winrt::TeachingTip>();
}
