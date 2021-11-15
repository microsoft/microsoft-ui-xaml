#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "TitleBarAutomationPeer.h"
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>
#include "TitleBarAutomationPeer.properties.cpp"

TitleBarAutomationPeer::TitleBarAutomationPeer(winrt::TitleBar const& owner) : ReferenceTracker(owner)
{
}

winrt::AutomationControlType TitleBarAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::TitleBar;
}

winrt::hstring TitleBarAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::TitleBar>();
}

winrt::hstring TitleBarAutomationPeer::GetNameCore()
{
    winrt::hstring name = __super::GetNameCore();

    if (name.empty())
    {
        if (auto titleBar = Owner().try_as<winrt::TitleBar>())
        {
            name = titleBar.Title();
        }
    }

    return name;
}
