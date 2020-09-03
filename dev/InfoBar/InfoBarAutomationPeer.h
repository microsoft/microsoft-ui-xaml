#pragma once
#include "InfoBar.h"
#include "InfoBarAutomationPeer.g.h"

class InfoBarAutomationPeer :
    public ReferenceTracker<InfoBarAutomationPeer, winrt::implementation::InfoBarAutomationPeerT> // ###, winrt::IWindowProvider>
{

public:
    InfoBarAutomationPeer(winrt::InfoBar const& owner);

    // IAutomationPeerOverrides 
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetClassNameCore();

    /*winrt::WindowInteractionState InteractionState();
    bool IsModal();
    bool IsTopmost();
    bool Maximizable();
    bool Minimizable();
    winrt::WindowVisualState VisualState();
    void Close();
    void SetVisualState(winrt::WindowVisualState state);
    bool WaitForInputIdle(int32_t milliseconds);*/

    void RaiseWindowClosedEvent(wstring_view const& displayString);
    void RaiseWindowOpenedEvent(wstring_view const& displayString);

private:
    winrt::InfoBar GetInfoBar();
};
