#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "InfoBarAutomationPeer.h"
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>

#include "InfoBarAutomationPeer.properties.cpp"

InfoBarAutomationPeer::InfoBarAutomationPeer(winrt::InfoBar const& owner) : ReferenceTracker(owner)
{
}

winrt::AutomationControlType InfoBarAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::StatusBar;
}

winrt::hstring InfoBarAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::InfoBar>();
}

/*
winrt::WindowInteractionState InfoBarAutomationPeer::InteractionState()
{
    auto InfoBar = winrt::get_self<InfoBar>(GetInfoBar());
    if (InfoBar->m_isIdle && InfoBar->IsOpen())
    {
        return winrt::WindowInteractionState::ReadyForUserInteraction;
    }
    else if (InfoBar->m_isIdle && !InfoBar->IsOpen())
    {
        return winrt::WindowInteractionState::BlockedByModalWindow;
    }
    else if (!InfoBar->m_isIdle && !InfoBar->IsOpen())
    {
        return winrt::WindowInteractionState::Closing;
    }
    else
    {
        return winrt::WindowInteractionState::Running;
    }
}

bool InfoBarAutomationPeer::IsModal()
{
    return winrt::get_self<InfoBar>(GetInfoBar())->IsLightDismissEnabled();
}

bool InfoBarAutomationPeer::IsTopmost()
{
    return winrt::get_self<InfoBar>(GetInfoBar())->IsOpen();
}

bool InfoBarAutomationPeer::Maximizable()
{
    return false;
}

bool InfoBarAutomationPeer::Minimizable()
{
    return false;
}

winrt::WindowVisualState InfoBarAutomationPeer::VisualState()
{
    return winrt::WindowVisualState::Normal;
}

void InfoBarAutomationPeer::Close()
{
    winrt::get_self<InfoBar>(GetInfoBar())->IsOpen(false);
}

void InfoBarAutomationPeer::SetVisualState(winrt::WindowVisualState state)
{
    
}

bool InfoBarAutomationPeer::WaitForInputIdle(int32_t milliseconds)
{
    return true;
}*/

void InfoBarAutomationPeer::RaiseWindowOpenedEvent(wstring_view const& displayString)
{
    if (winrt::IAutomationPeer7 automationPeer7 = *this)
    {
        automationPeer7.RaiseNotificationEvent(
            winrt::Automation::Peers::AutomationNotificationKind::Other,
            winrt::Peers::AutomationNotificationProcessing::CurrentThenMostRecent,
            displayString,
            L"InfoBarOpenedActivityId");
    }

    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::WindowOpened))
    {
        RaiseAutomationEvent(winrt::AutomationEvents::WindowOpened);
    }
}

void InfoBarAutomationPeer::RaiseWindowClosedEvent(wstring_view const& displayString)
{
    if (winrt::IAutomationPeer7 automationPeer7 = *this)
    {
        automationPeer7.RaiseNotificationEvent(
            winrt::Automation::Peers::AutomationNotificationKind::Other,
            winrt::Peers::AutomationNotificationProcessing::CurrentThenMostRecent,
            displayString,
            L"InfoBarClosedActivityId");
    }

    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::WindowClosed))
    {
        RaiseAutomationEvent(winrt::AutomationEvents::WindowClosed);
    }
}

winrt::InfoBar InfoBarAutomationPeer::GetInfoBar()
{
    winrt::UIElement owner = Owner();
    return owner.as<winrt::InfoBar>();
}
