#pragma once
#include "TeachingTip.h"
#include "TeachingTipAutomationPeer.g.h"

class TeachingTipAutomationPeer :
    public ReferenceTracker<TeachingTipAutomationPeer, winrt::implementation::TeachingTipAutomationPeerT>
{

public:
    TeachingTipAutomationPeer(winrt::TeachingTip const& owner);

    // IAutomationPeerOverrides 
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetClassNameCore();
};

CppWinRTActivatableClassWithBasicFactory(TeachingTipAutomationPeer)