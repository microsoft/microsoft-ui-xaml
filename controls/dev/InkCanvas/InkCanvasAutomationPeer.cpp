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

namespace
{
    winrt::Rect IntersectRects(winrt::Rect const& a, winrt::Rect const& b)
    {
        const float left = std::max(a.X, b.X);
        const float top = std::max(a.Y, b.Y);
        const float right = std::min(a.X + a.Width, b.X + b.Width);
        const float bottom = std::min(a.Y + a.Height, b.Y + b.Height);

        if (right <= left || bottom <= top)
        {
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        }

        return { left, top, right - left, bottom - top };
    }
}

// InkCanvas draws entirely through a child composition visual and has no XAML children, so the
// framework finds no rendered content to measure and reports an empty rect. Derive the bounds from
// the layout size instead, then re-apply the clipping that CFrameworkElementAutomationPeer would
// have applied, so a canvas scrolled out of view or pushed off the window still reports accurately.
winrt::Rect InkCanvasAutomationPeer::GetClippedBoundsInRoot()
{
    auto const owner = Owner().try_as<winrt::InkCanvas>();
    if (!owner || owner.Visibility() != winrt::Visibility::Visible)
    {
        return { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    const winrt::Rect localBounds{
        0.0f,
        0.0f,
        static_cast<float>(owner.ActualWidth()),
        static_cast<float>(owner.ActualHeight()) };

    if (localBounds.Width <= 0.0f || localBounds.Height <= 0.0f)
    {
        return { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    auto bounds = owner.TransformToVisual(nullptr).TransformBounds(localBounds);

    for (auto parent = winrt::VisualTreeHelper::GetParent(owner); parent; parent = winrt::VisualTreeHelper::GetParent(parent))
    {
        auto const ancestor = parent.try_as<winrt::FrameworkElement>();
        if (!ancestor)
        {
            continue;
        }

        if (ancestor.Visibility() != winrt::Visibility::Visible)
        {
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        }

        if (auto const clip = ancestor.Clip())
        {
            bounds = IntersectRects(bounds, ancestor.TransformToVisual(nullptr).TransformBounds(clip.Rect()));
        }

        // Scroll presenters clip to their viewport without setting UIElement.Clip.
        if (ancestor.try_as<winrt::ScrollContentPresenter>() || ancestor.try_as<winrt::ScrollPresenter>())
        {
            const winrt::Rect viewport{
                0.0f,
                0.0f,
                static_cast<float>(ancestor.ActualWidth()),
                static_cast<float>(ancestor.ActualHeight()) };

            bounds = IntersectRects(bounds, ancestor.TransformToVisual(nullptr).TransformBounds(viewport));
        }

        if (bounds.Width <= 0.0f || bounds.Height <= 0.0f)
        {
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        }
    }

    if (auto const xamlRoot = owner.XamlRoot())
    {
        const auto rootSize = xamlRoot.Size();
        bounds = IntersectRects(bounds, { 0.0f, 0.0f, rootSize.Width, rootSize.Height });
    }

    return bounds;
}

winrt::Rect InkCanvasAutomationPeer::GetBoundingRectangleCore()
{
    const auto bounds = GetClippedBoundsInRoot();
    if (bounds.Width <= 0.0f || bounds.Height <= 0.0f)
    {
        return { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    if (auto const owner = Owner().try_as<winrt::InkCanvas>())
    {
        return SharedHelpers::ConvertDipsToPhysical(owner, bounds);
    }

    return { 0.0f, 0.0f, 0.0f, 0.0f };
}

// Same reason: with no rendered XAML content the framework treats the canvas as offscreen, which
// makes assistive technology skip it entirely. Matching the framework, empty clipped bounds mean
// offscreen, so a partially visible canvas is still reported as on screen.
bool InkCanvasAutomationPeer::IsOffscreenCore()
{
    const auto bounds = GetClippedBoundsInRoot();
    return bounds.Width <= 0.0f || bounds.Height <= 0.0f;
}
