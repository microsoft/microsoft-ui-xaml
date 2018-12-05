#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "TeachingTipAutomationPeer.h"
#include "Utils.h"

TeachingTipAutomationPeer::TeachingTipAutomationPeer(winrt::TeachingTip const& owner) : ReferenceTracker(owner)
{
}

winrt::AutomationControlType TeachingTipAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Window;
}

winrt::hstring TeachingTipAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::TeachingTip>();
}