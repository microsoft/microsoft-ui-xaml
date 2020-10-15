#pragma once
#include "TeachingTip.h"
#include "TeachingTipAutomationPeer.g.h"

class TeachingTipAutomationPeer :
    public ReferenceTracker<TeachingTipAutomationPeer, winrt::implementation::TeachingTipAutomationPeerT, winrt::IWindowProvider>
{

public:
    TeachingTipAutomationPeer(winrt::TeachingTip const& owner);

    // IAutomationPeerOverrides 
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetClassNameCore();

    winrt::WindowInteractionState InteractionState();
    bool IsModal();
    bool IsTopmost();
    bool Maximizable();
    bool Minimizable();
    winrt::WindowVisualState VisualState();
    void Close();
    void SetVisualState(winrt::WindowVisualState state);
    bool WaitForInputIdle(int32_t milliseconds);

    void RaiseWindowClosedEvent();
    void RaiseWindowOpenedEvent(wstring_view const& displayString);

private:
    winrt::TeachingTip GetTeachingTip();
};
