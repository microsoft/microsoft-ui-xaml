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

void InfoBarAutomationPeer::RaiseOpenedEvent(wstring_view const& displayString)
{
    if (winrt::IAutomationPeer7 automationPeer7 = *this)
    {
        automationPeer7.RaiseNotificationEvent(
            winrt::Automation::Peers::AutomationNotificationKind::Other,
            winrt::Peers::AutomationNotificationProcessing::CurrentThenMostRecent,
            displayString,
            L"InfoBarOpenedActivityId");
    }
}

void InfoBarAutomationPeer::RaiseClosedEvent(wstring_view const& displayString)
{
    if (winrt::IAutomationPeer7 automationPeer7 = *this)
    {
        automationPeer7.RaiseNotificationEvent(
            winrt::Automation::Peers::AutomationNotificationKind::Other,
            winrt::Peers::AutomationNotificationProcessing::CurrentThenMostRecent,
            displayString,
            L"InfoBarClosedActivityId");
    }
}

winrt::InfoBar InfoBarAutomationPeer::GetInfoBar()
{
    winrt::UIElement owner = Owner();
    return owner.as<winrt::InfoBar>();
}
