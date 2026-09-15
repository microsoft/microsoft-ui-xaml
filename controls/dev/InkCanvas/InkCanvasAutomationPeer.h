#pragma once

#include "InkCanvas.h"

#include "InkCanvasAutomationPeer.g.h"

class InkCanvasAutomationPeer :
    public ReferenceTracker<InkCanvasAutomationPeer, winrt::implementation::InkCanvasAutomationPeerT>
{
public:
    InkCanvasAutomationPeer(winrt::InkCanvas const& owner);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::Rect GetBoundingRectangleCore();
    bool IsOffscreenCore();

private:
    // Bounds in root coordinates, clipped the way the framework clips them. Empty when not visible.
    winrt::Rect GetClippedBoundsInRoot();
};
