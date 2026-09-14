#include "pch.h"
#include "common.h"
#include "SharedHelpers.h"
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

// InkCanvas draws entirely through a child composition visual and has no XAML children, so the
// framework finds no rendered content to measure and reports an empty rect. Derive the bounds from
// the layout size instead, matching ColorSpectrum.
winrt::Rect InkCanvasAutomationPeer::GetBoundingRectangleCore()
{
    if (auto const owner = Owner().try_as<winrt::InkCanvas>())
    {
        const winrt::Rect localBounds{
            0.0f,
            0.0f,
            static_cast<float>(owner.ActualWidth()),
            static_cast<float>(owner.ActualHeight()) };

        const auto globalBounds = owner.TransformToVisual(nullptr).TransformBounds(localBounds);
        return SharedHelpers::ConvertDipsToPhysical(owner, globalBounds);
    }

    return { 0.0f, 0.0f, 0.0f, 0.0f };
}

// Same reason: with no rendered XAML content the framework treats the canvas as offscreen, which
// makes assistive technology skip it entirely.
bool InkCanvasAutomationPeer::IsOffscreenCore()
{
    auto const owner = Owner().try_as<winrt::InkCanvas>();
    if (!owner || owner.Visibility() != winrt::Visibility::Visible)
    {
        return true;
    }

    const winrt::Rect localBounds{
        0.0f,
        0.0f,
        static_cast<float>(owner.ActualWidth()),
        static_cast<float>(owner.ActualHeight()) };

    if (localBounds.Width <= 0.0f || localBounds.Height <= 0.0f)
    {
        return true;
    }

    auto const xamlRoot = owner.XamlRoot();
    if (!xamlRoot)
    {
        return true;
    }

    // Treat the canvas as offscreen when it is scrolled or positioned outside the content area.
    const auto bounds = owner.TransformToVisual(nullptr).TransformBounds(localBounds);
    const auto rootSize = xamlRoot.Size();

    return bounds.X + bounds.Width <= 0.0f
        || bounds.Y + bounds.Height <= 0.0f
        || bounds.X >= rootSize.Width
        || bounds.Y >= rootSize.Height;
}
