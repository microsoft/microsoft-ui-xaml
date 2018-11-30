// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ScrollAnchorProvider.h"
#include "DoubleUtil.h"

CppWinRTActivatableClassWithBasicFactory(ScrollAnchorProvider)

ScrollAnchorProvider::ScrollAnchorProvider()
{
    IsTabStop(false);
}

#pragma region IFrameworkElementOverrides

winrt::Size ScrollAnchorProvider::ArrangeOverride(winrt::Size const& finalSize)
{
    const auto scrollViewer = TryGetScrollViewer();    
    const auto shouldApplyPendingChangeView = scrollViewer && HasPendingBringIntoView() && !m_pendingBringIntoView.ChangeViewCalled();

    winrt::Rect anchorElementRelativeBounds{};
    const auto anchorElement =
        // BringIntoView takes precedence over tracking.
        shouldApplyPendingChangeView ?
        nullptr : 
        // Pick the best candidate depending on HorizontalAnchorRatio and VerticalAnchorRatio.
        // The best candidate is the element that's the closest to the edge of interest.
        GetAnchorElement(&anchorElementRelativeBounds);

    auto result = __super::ArrangeOverride(finalSize);

    m_pendingViewportShift = 0.0;

    
    if (shouldApplyPendingChangeView)
    {
        ApplyPendingChangeView(scrollViewer);
    }
    else if (anchorElement)
    {
        // The anchor element might have changed its position relative to us.
        // If that's the case, we should shift the viewport to follow it as much as possible.
        m_pendingViewportShift = TrackElement(anchorElement, anchorElementRelativeBounds, scrollViewer);
    }
    else if (!scrollViewer)
    {
        m_pendingBringIntoView.Reset();
    }

    m_candidates.clear();
    m_isAnchorElementDirty = true;

    m_postArrange(*this);

    return result;
}

#pragma endregion

#pragma region IScrollAnchorProvider

double ScrollAnchorProvider::HorizontalAnchorRatio()
{
    return m_horizontalEdge;
}

void ScrollAnchorProvider::HorizontalAnchorRatio(double value)
{
    m_horizontalEdge = value;
}

double ScrollAnchorProvider::VerticalAnchorRatio()
{
    return m_verticalEdge;
}

void ScrollAnchorProvider::VerticalAnchorRatio(double value)
{
    m_verticalEdge = value;
}

// TODO: this API should go on UIElement.
void ScrollAnchorProvider::StartBringIntoView(
    winrt::UIElement const& element,
    double alignmentX,
    double alignmentY,
    double offsetX,
    double offsetY,
    bool animate)
{

    m_pendingBringIntoView = {
        this /* owner */,
        element,
        alignmentX,
        alignmentY,
        offsetX,
        offsetY,
        !!animate };
}

bool ScrollAnchorProvider::IsHorizontallyScrollable()
{
    return true;
}

bool ScrollAnchorProvider::IsVerticallyScrollable()
{
    return true;
}

winrt::UIElement ScrollAnchorProvider::AnchorElement()
{
    return GetAnchorElement();
}

winrt::event_token ScrollAnchorProvider::ViewportChanged(winrt::ViewportChangedEventHandler const& value)
{
    return m_viewportChanged.add(value);
}

void ScrollAnchorProvider::ViewportChanged(winrt::event_token const& token)
{
    m_viewportChanged.remove(token);
}

winrt::event_token ScrollAnchorProvider::PostArrange(winrt::PostArrangeEventHandler const& value)
{
    return m_postArrange.add(value);
}

void ScrollAnchorProvider::PostArrange(winrt::event_token const& token)
{
    m_postArrange.remove(token);
}

winrt::event_token ScrollAnchorProvider::ConfigurationChanged(
    winrt::ConfigurationChangedEventHandler const& /*value*/)
{
    return {};
}

void ScrollAnchorProvider::ConfigurationChanged(
    winrt::event_token const& /*token*/)
{
}

void ScrollAnchorProvider::RegisterAnchorCandidate(winrt::UIElement const& element)
{
    if (HorizontalAnchorRatio() != DoubleUtil::NaN || VerticalAnchorRatio() != DoubleUtil::NaN)
    {
        const auto scrollViewer = TryGetScrollViewer();
        if (scrollViewer)
        {

#ifdef _DEBUG
            // We should not be registring the same element twice. Even through it is functionally ok,
            // we will end up spending more time during arrange than we must.
            // However checking if an element is already in the list every time a new element is registered is worse for perf.
            // So, I'm leaving an assert here to catch regression in our code but in release builds we run without the check.
            const auto elem = element;
            const auto it = std::find_if(m_candidates.cbegin(), m_candidates.cend(), [&elem](const CandidateInfo& c) { return c.Element() == elem; });
            if (it != m_candidates.cend())
            {
                MUX_ASSERT(false);
            }
#endif // _DEBUG

            m_candidates.push_back(CandidateInfo(this /* owner */, element));
            m_isAnchorElementDirty = true;
        }
    }
}

void ScrollAnchorProvider::UnregisterAnchorCandidate(winrt::UIElement const& element)
{
    const auto elem = element;
    const auto it = std::find_if(m_candidates.cbegin(), m_candidates.cend(), [&elem](const CandidateInfo& c) { return c.Element() == elem; });
    if (it != m_candidates.cend())
    {
        REPEATER_TRACE_INFO(L"Unregistered candidate %d\n", it - m_candidates.begin());
        m_candidates.erase(it);
        m_isAnchorElementDirty = true;
    }
}

winrt::Rect ScrollAnchorProvider::GetRelativeViewport(
    winrt::UIElement const& element)
{
    const auto scrollViewer = TryGetScrollViewer();
    if (scrollViewer)
    {
        const auto elem = element;
        const bool hasLockedViewport = HasPendingBringIntoView();
        const auto transformer = elem.TransformToVisual(hasLockedViewport ? scrollViewer.ContentTemplateRoot() : scrollViewer);
        const auto zoomFactor = static_cast<double>(scrollViewer.ZoomFactor());
        const double viewportWidth = scrollViewer.ViewportWidth() / zoomFactor;
        const double viewportHeight = scrollViewer.ViewportHeight() / zoomFactor;

        auto elementOffset = transformer.TransformPoint(winrt::Point{});

        elementOffset.X = -elementOffset.X;
        elementOffset.Y = -elementOffset.Y + static_cast<float>(m_pendingViewportShift);

        REPEATER_TRACE_INFO(L"Pending Shift - %lf\n", m_pendingViewportShift);

        if (hasLockedViewport)
        {
            elementOffset.X += m_pendingBringIntoView.ChangeViewOffset().X;
            elementOffset.Y += m_pendingBringIntoView.ChangeViewOffset().Y;
        }

        return { elementOffset.X, elementOffset.Y, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight) };
    }

    return {};
}

#pragma endregion

void ScrollAnchorProvider::ApplyPendingChangeView(const winrt::ScrollViewer& scrollViewer)
{
    auto bringIntoView = m_pendingBringIntoView;
    MUX_ASSERT(!bringIntoView.ChangeViewCalled());

    bringIntoView.ChangeViewCalled(true);

    const auto layoutSlot = CachedVisualTreeHelpers::GetLayoutSlot(bringIntoView.TargetElement().as<winrt::FrameworkElement>());

    // Arrange bounds are absolute.
    const auto arrangeBounds = bringIntoView
        .TargetElement()
        .TransformToVisual(scrollViewer.ContentTemplateRoot())
        .TransformBounds(winrt::Rect{ 0, 0, layoutSlot.Width, layoutSlot.Height });

    const auto scrollableArea = winrt::Point(
        static_cast<float>(scrollViewer.ViewportWidth() - arrangeBounds.Width),
        static_cast<float>(scrollViewer.ViewportHeight() - arrangeBounds.Height));

    // Calculate the target offset based on the alignment and offset parameters.
    // Make sure that we are constrained to the ScrollViewer's extent.
    const auto changeViewOffset = winrt::Point{
        std::max(0.0f, static_cast<float>(std::min(
            arrangeBounds.X + bringIntoView.OffsetX() - scrollableArea.X * bringIntoView.AlignmentX(),
            scrollViewer.ExtentWidth() - scrollViewer.ViewportWidth()))),
        std::max(0.0f, static_cast<float>(std::min(
            arrangeBounds.Y + bringIntoView.OffsetY() - scrollableArea.Y * bringIntoView.AlignmentY(),
            scrollViewer.ExtentHeight() - scrollViewer.ViewportHeight()))) };
    bringIntoView.ChangeViewOffset(changeViewOffset);

    REPEATER_TRACE_INFO(L"ScrollAnchorProvider scroll to absolute offset (%.0f, %.0f) \n", changeViewOffset.X, changeViewOffset.Y);
    scrollViewer.ChangeView(
        box_value(static_cast<double>(changeViewOffset.X)).as<winrt::IReference<double>>(),
        box_value(static_cast<double>(changeViewOffset.Y)).as<winrt::IReference<double>>(),
        nullptr,
        !bringIntoView.Animate());

    m_pendingBringIntoView = std::move(bringIntoView);
}

double ScrollAnchorProvider::TrackElement(const winrt::UIElement& element, winrt::Rect previousBounds, const winrt::ScrollViewer& scrollViewer)
{
    const auto bounds = winrt::LayoutInformation::GetLayoutSlot(element.as<winrt::FrameworkElement>());
    const auto transformer = element.TransformToVisual(scrollViewer.ContentTemplateRoot());
    const auto newBounds = transformer.TransformBounds(winrt::Rect{
        0.0f,
        0.0f,
        bounds.Width,
        bounds.Height });

    const auto oldEdgeOffset = previousBounds.Y + HorizontalAnchorRatio() * previousBounds.Height;
    const auto newEdgeOffset = newBounds.Y + HorizontalAnchorRatio() * newBounds.Height;
    
    const auto unconstrainedPendingViewportShift = newEdgeOffset - oldEdgeOffset;
    auto pendingViewportShift = unconstrainedPendingViewportShift;

    // ScrollViewer.ChangeView is not synchronous, so we need to account for the pending ChangeView call
    // and make sure we are locked on the target viewport.
    const auto verticalOffset =
        HasPendingBringIntoView() && !m_pendingBringIntoView.Animate() ?
        m_pendingBringIntoView.ChangeViewOffset().Y :
        scrollViewer.VerticalOffset();

    // Constrain the viewport shift to the extent
    if (verticalOffset + pendingViewportShift < 0)
    {
        pendingViewportShift = -verticalOffset;
    }
    else if (verticalOffset + scrollViewer.ViewportHeight() + pendingViewportShift > scrollViewer.ExtentHeight())
    {
        pendingViewportShift = scrollViewer.ExtentHeight() - scrollViewer.ViewportHeight() - verticalOffset;
    }

    if (std::abs(pendingViewportShift) > 1)
    {
        // TODO: do we need to account for the zoom factor?
        // BUG:
        //  Unfortunately, if we have to correct while animating, we almost never
        //  update the ongoing animation correctly and we end up missing our target
        //  viewport. We should address that when building element tracking as part
        //  of the framework.
        REPEATER_TRACE_INFO(L"Viewport shift:%.0f. \n", pendingViewportShift);
        scrollViewer.ChangeView(
            nullptr,
            box_value(verticalOffset + pendingViewportShift).as<winrt::IReference<double>>(),
            nullptr,
            true /* disableAnimation */);
    }
    else
    {
        pendingViewportShift = 0.0;

        // We can't shift the viewport to follow the tracked element. The viewport relative
        // to the tracked element will have changed. We need to raise ViewportChanged to make
        // sure the repeaters will get a second layout pass to fill any empty space they have.
        if (std::abs(unconstrainedPendingViewportShift) > 1)
        {
            m_viewportChanged(*this, true /* isFinal */);
        }
    }

    return pendingViewportShift;
}

winrt::ScrollViewer ScrollAnchorProvider::TryGetScrollViewer()
{
    if (!m_scrollViewer)
    {
        // PERF: This operation is expensive especially since it gets invoked every time
        // CalculateDistance is called.
        m_scrollViewer.set(Content().try_as<winrt::ScrollViewer>());

        if (m_scrollViewer)
        {
            m_scrollViewer.get().ViewChanging({ this, &ScrollAnchorProvider::OnScrollViewerViewChanging });
            m_scrollViewer.get().ViewChanged({ this, &ScrollAnchorProvider::OnScrollViewerViewChanged });
            m_scrollViewer.get().SizeChanged({ this, &ScrollAnchorProvider::OnScrollViewerSizeChanged });
        }
    }
    return m_scrollViewer.get();
}

winrt::UIElement ScrollAnchorProvider::GetAnchorElement(_Out_opt_ winrt::Rect* relativeBounds)
{
    if (m_isAnchorElementDirty)
    {
        winrt::ScrollViewer scrollViewer = TryGetScrollViewer();

        if (scrollViewer)
        {
            // ScrollViewer.ChangeView is not synchronous, so we need to account for the pending ChangeView call
            // and make sure we are locked on the target viewport.
            const auto verticalOffset =
                HasPendingBringIntoView() && !m_pendingBringIntoView.Animate() ?
                m_pendingBringIntoView.ChangeViewOffset().Y :
                scrollViewer.VerticalOffset();
            const double viewportEdgeOffset = verticalOffset + HorizontalAnchorRatio() * scrollViewer.ViewportHeight() + m_pendingViewportShift;

            const CandidateInfo* bestCandidate{ nullptr };
            double bestCandidateDistance = std::numeric_limits<float>::max();

            for (auto& candidate : m_candidates)
            {
                const auto element = candidate.Element();

                if (!candidate.IsRelativeBoundsSet())
                {
                    const auto bounds = winrt::LayoutInformation::GetLayoutSlot(element.as<winrt::FrameworkElement>());
                    const auto transformer = element.TransformToVisual(scrollViewer.ContentTemplateRoot());
                    candidate.RelativeBounds(transformer.TransformBounds(winrt::Rect{
                        0.0f,
                        0.0f,
                        bounds.Width,
                        bounds.Height }));
                }

                const double elementEdgeOffset = candidate.RelativeBounds().Y + HorizontalAnchorRatio() * candidate.RelativeBounds().Height;
                const double candidateDistance = std::abs(elementEdgeOffset - viewportEdgeOffset);
                if (candidateDistance < bestCandidateDistance)
                {
                    bestCandidate = &candidate;
                    bestCandidateDistance = candidateDistance;
                }
            }
            
            if (bestCandidate)
            {
                m_anchorElement.set(bestCandidate->Element());
                m_anchorElementRelativeBounds = bestCandidate->RelativeBounds();
            }
            else
            {
                m_anchorElement.set(nullptr);
                m_anchorElementRelativeBounds = CandidateInfo::InvalidBounds;
            }
        }

        m_isAnchorElementDirty = false;
    }

    if (relativeBounds)
    {
        *relativeBounds = m_anchorElementRelativeBounds;
    }

    return m_anchorElement.get();
}

void ScrollAnchorProvider::OnScrollViewerViewChanging(
    const winrt::IInspectable& sender,
    const winrt::ScrollViewerViewChangingEventArgs& args)
{
    m_viewportChanged(*this, false /* isFinal */);
}

void ScrollAnchorProvider::OnScrollViewerViewChanged(
    winrt::IInspectable const&,
    winrt::ScrollViewerViewChangedEventArgs const& args)
{
    const bool isFinal = !args.IsIntermediate();

    if (isFinal)
    {
        m_pendingViewportShift = 0.0;

        if (HasPendingBringIntoView() &&
            m_pendingBringIntoView.ChangeViewCalled() == true)
        {
            m_pendingBringIntoView.Reset();
        }
    }

    m_viewportChanged(*this, isFinal);
}

void ScrollAnchorProvider::OnScrollViewerSizeChanged(
    const winrt::IInspectable& sender,
    const winrt::SizeChangedEventArgs& args)
{
    m_viewportChanged(*this, true /* isFinal */);
}