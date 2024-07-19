// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ItemsRepeaterScrollHost.h"
#include "DoubleUtil.h"
#include "RuntimeProfiler.h"

#include "ItemsRepeaterScrollHost.properties.cpp"

ItemsRepeaterScrollHost::ItemsRepeaterScrollHost()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ItemsRepeaterScrollHost);
}

winrt::UIElement ItemsRepeaterScrollHost::CurrentAnchor()
{
    return GetAnchorElement();
}

winrt::FxScrollViewer ItemsRepeaterScrollHost::ScrollViewer()
{
    winrt::FxScrollViewer value = nullptr;
    auto children = Children();
    if (children.Size() > 0)
    {
        value = children.GetAt(0).try_as<winrt::FxScrollViewer>();
    }

    return value;
}

void ItemsRepeaterScrollHost::ScrollViewer(winrt::FxScrollViewer const& value)
{
    m_scrollViewerViewChanging.revoke();
    m_scrollViewerViewChanged.revoke();
    m_scrollViewerSizeChanged.revoke();

    auto children = Children();
    children.Clear();
    children.Append(value);
}

#pragma region IFrameworkElementOverrides

winrt::Size ItemsRepeaterScrollHost::MeasureOverride(winrt::Size const& availableSize)
{
    winrt::Size desiredSize = {};
    if (auto scrollViewer = ScrollViewer())
    {
        scrollViewer.Measure(availableSize);
        desiredSize = scrollViewer.DesiredSize();
    }

    return desiredSize;
}

winrt::Size ItemsRepeaterScrollHost::ArrangeOverride(winrt::Size const& finalSize)
{
    const winrt::Size result = finalSize;
    if (auto scrollViewer = ScrollViewer())
    {
        scrollViewer.Arrange({ 0, 0, finalSize.Width, finalSize.Height });
    }

    return result;
}

#pragma endregion

#pragma region IScrollAnchorProvider

double ItemsRepeaterScrollHost::HorizontalAnchorRatio()
{
    return m_horizontalEdge;
}

void ItemsRepeaterScrollHost::HorizontalAnchorRatio(double value)
{
    m_horizontalEdge = value;
}

double ItemsRepeaterScrollHost::VerticalAnchorRatio()
{
    return m_verticalEdge;
}

void ItemsRepeaterScrollHost::VerticalAnchorRatio(double value)
{
    m_verticalEdge = value;
}

// TODO: this API should go on UIElement.
void ItemsRepeaterScrollHost::StartBringIntoView(
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

bool ItemsRepeaterScrollHost::IsHorizontallyScrollable()
{
    return true;
}

bool ItemsRepeaterScrollHost::IsVerticallyScrollable()
{
    return true;
}

winrt::UIElement ItemsRepeaterScrollHost::AnchorElement()
{
    return GetAnchorElement();
}


winrt::event_token ItemsRepeaterScrollHost::ViewportChanged(winrt::ViewportChangedEventHandler const& value)
{
    return m_viewportChanged.add(value);
}

void ItemsRepeaterScrollHost::ViewportChanged(winrt::event_token const& token)
{
    m_viewportChanged.remove(token);
}

winrt::event_token ItemsRepeaterScrollHost::PostArrange(winrt::PostArrangeEventHandler const& value)
{
    return m_postArrange.add(value);
}

void ItemsRepeaterScrollHost::PostArrange(winrt::event_token const& token)
{
    m_postArrange.remove(token);
}

winrt::event_token ItemsRepeaterScrollHost::ConfigurationChanged(
    winrt::ConfigurationChangedEventHandler const& /*value*/)
{
    return {};
}

void ItemsRepeaterScrollHost::ConfigurationChanged(
    winrt::event_token const& /*token*/)
{
}

void ItemsRepeaterScrollHost::RegisterAnchorCandidate(winrt::UIElement const& element)
{
    if (HorizontalAnchorRatio() != DoubleUtil::NaN || VerticalAnchorRatio() != DoubleUtil::NaN)
    {
        if (const auto scrollViewer = ScrollViewer())
        {

#ifdef DBG
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
#endif // DBG

            m_candidates.push_back(CandidateInfo(this /* owner */, element));
            m_isAnchorElementDirty = true;
        }
    }
}

void ItemsRepeaterScrollHost::UnregisterAnchorCandidate(winrt::UIElement const& element)
{
    const auto elem = element;
    const auto it = std::find_if(m_candidates.cbegin(), m_candidates.cend(), [&elem](const CandidateInfo& c) { return c.Element() == elem; });
    if (it != m_candidates.cend())
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Unregistered candidate:", it - m_candidates.begin());
        m_candidates.erase(it);
        m_isAnchorElementDirty = true;
    }
}

winrt::Rect ItemsRepeaterScrollHost::GetRelativeViewport(
    winrt::UIElement const& element)
{
    if (const auto scrollViewer = ScrollViewer())
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

        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"Pending shift:", m_pendingViewportShift);

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

void ItemsRepeaterScrollHost::ApplyPendingChangeView(const winrt::FxScrollViewer& scrollViewer)
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

    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"ItemsRepeaterScrollHost scroll to absolute offset:", changeViewOffset.X, changeViewOffset.Y);
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Animate:", bringIntoView.Animate());

    scrollViewer.ChangeView(
        box_value(static_cast<double>(changeViewOffset.X)).as<winrt::IReference<double>>(),
        box_value(static_cast<double>(changeViewOffset.Y)).as<winrt::IReference<double>>(),
        nullptr,
        !bringIntoView.Animate());

    m_pendingBringIntoView = std::move(bringIntoView);
}

double ItemsRepeaterScrollHost::TrackElement(const winrt::UIElement& element, winrt::Rect previousBounds, const winrt::FxScrollViewer& scrollViewer)
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
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"Viewport shift:", pendingViewportShift);
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

winrt::UIElement ItemsRepeaterScrollHost::GetAnchorElement(_Out_opt_ winrt::Rect* relativeBounds)
{
    if (m_isAnchorElementDirty)
    {
        if (const auto scrollViewer = ScrollViewer())
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

void ItemsRepeaterScrollHost::OnScrollViewerViewChanging(
    const winrt::IInspectable& sender,
    const winrt::ScrollViewerViewChangingEventArgs& args)
{
    m_viewportChanged(*this, false /* isFinal */);
}

void ItemsRepeaterScrollHost::OnScrollViewerViewChanged(
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

void ItemsRepeaterScrollHost::OnScrollViewerSizeChanged(
    const winrt::IInspectable& sender,
    const winrt::SizeChangedEventArgs& args)
{
    m_viewportChanged(*this, true /* isFinal */);
}
