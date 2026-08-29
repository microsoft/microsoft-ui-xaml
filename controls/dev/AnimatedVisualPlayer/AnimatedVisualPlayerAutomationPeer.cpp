#include "pch.h"
#include "common.h"
#include "AnimatedVisualPlayerAutomationPeer.h"
#include "AnimatedVisualPlayer.h"

#include "AnimatedVisualPlayerAutomationPeer.properties.cpp"

AnimatedVisualPlayerAutomationPeer::AnimatedVisualPlayerAutomationPeer(winrt::AnimatedVisualPlayer const& owner)
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
