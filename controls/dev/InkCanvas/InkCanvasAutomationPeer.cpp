#include "pch.h"
#include "common.h"
#include "InkCanvasAutomationPeer.h"
#include "InkCanvas.h"

#include "InkCanvasAutomationPeer.properties.cpp"

InkCanvasAutomationPeer::InkCanvasAutomationPeer(winrt::InkCanvas const& owner)
    : ReferenceTracker(owner)
{
}

hstring InkCanvasAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::InkCanvas>();
}

winrt::AutomationControlType InkCanvasAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Pane;
}
