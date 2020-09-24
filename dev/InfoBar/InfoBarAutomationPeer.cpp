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

void InfoBarAutomationPeer::RaiseOpenedEvent(winrt::InfoBarSeverity severity, wstring_view const& displayString)
{
    if (winrt::IAutomationPeer7 automationPeer7 = *this)
    {
        automationPeer7.RaiseNotificationEvent(
            winrt::Automation::Peers::AutomationNotificationKind::Other,
            GetProcessingForSeverity(severity),
            displayString,
            L"InfoBarOpenedActivityId");
    }
}

void InfoBarAutomationPeer::RaiseClosedEvent(winrt::InfoBarSeverity severity, wstring_view const& displayString)
{
    winrt::Peers::AutomationNotificationProcessing processing = winrt::Peers::AutomationNotificationProcessing::CurrentThenMostRecent;


    if (winrt::IAutomationPeer7 automationPeer7 = *this)
    {
        automationPeer7.RaiseNotificationEvent(
            winrt::Automation::Peers::AutomationNotificationKind::Other,
            GetProcessingForSeverity(severity),
            displayString,
            L"InfoBarClosedActivityId");
    }
}


winrt::Peers::AutomationNotificationProcessing InfoBarAutomationPeer::GetProcessingForSeverity(winrt::InfoBarSeverity severity)
{
    winrt::Peers::AutomationNotificationProcessing processing = winrt::Peers::AutomationNotificationProcessing::CurrentThenMostRecent;

    if (severity == winrt::InfoBarSeverity::Critical
        || severity == winrt::InfoBarSeverity::Warning)
    {
        processing = winrt::Peers::AutomationNotificationProcessing::ImportantAll;
    }

    return processing;
}

winrt::InfoBar InfoBarAutomationPeer::GetInfoBar()
{
    winrt::UIElement owner = Owner();
    return owner.as<winrt::InfoBar>();
}
