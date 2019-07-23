#include "pch.h"
#include "AnimatedVisualPlayerAutomationPeer.h"
#include "common.h"

#include "AnimatedVisualPlayer.h"

AnimatedVisualPlayerAutomationPeer::AnimatedVisualPlayerAutomationPeer(winrt::AnimatedVisualPlayer  /*unused*/const& owner)
    : ReferenceTracker(owner)
{
}

hstring AnimatedVisualPlayerAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::AnimatedVisualPlayer>();
}

winrt::AutomationControlType AnimatedVisualPlayerAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Image;
}
