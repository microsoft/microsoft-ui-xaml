#pragma once
#include "TitleBar.h"
#include "TitleBarAutomationPeer.g.h"

class TitleBarAutomationPeer :
    public ReferenceTracker<TitleBarAutomationPeer, winrt::implementation::TitleBarAutomationPeerT>
{

public:
    TitleBarAutomationPeer(winrt::TitleBar const& owner);

    // IAutomationPeerOverrides 
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetClassNameCore();
    winrt::hstring GetNameCore();
};
