#pragma once
#include "InfoBar.h"
#include "InfoBarAutomationPeer.g.h"

class InfoBarAutomationPeer :
    public ReferenceTracker<InfoBarAutomationPeer, winrt::implementation::InfoBarAutomationPeerT>
{

public:
    InfoBarAutomationPeer(winrt::InfoBar const& owner);

    // IAutomationPeerOverrides 
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetClassNameCore();

    void RaiseClosedEvent(wstring_view const& displayString);
    void RaiseOpenedEvent(wstring_view const& displayString);

private:
    winrt::InfoBar GetInfoBar();
};
