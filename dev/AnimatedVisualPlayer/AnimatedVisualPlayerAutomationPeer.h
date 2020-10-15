#pragma once

#include "AnimatedVisualPlayer.h"

#include "AnimatedVisualPlayerAutomationPeer.g.h"

class AnimatedVisualPlayerAutomationPeer :
    public ReferenceTracker<AnimatedVisualPlayerAutomationPeer, winrt::implementation::AnimatedVisualPlayerAutomationPeerT>
{
public:
    AnimatedVisualPlayerAutomationPeer(winrt::AnimatedVisualPlayer const& owner);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
};
