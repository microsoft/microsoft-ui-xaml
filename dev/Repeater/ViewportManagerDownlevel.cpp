// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ViewportManagerDownLevel.h"
#include "ItemsRepeater.h"
#include "layout.h"

// Pixel delta by which to inflate the cache buffer on each side.  Rather than fill the entire
// cache buffer all at once, we chunk the work to make the UI thread more responsive.  We inflate
// the cache buffer from 0 to a max value determined by the Maximum[Horizontal,Vertical]CacheLength
// properties.
static double CacheBufferPerSideInflationPixelDelta = 40.0;

ViewportManagerDownLevel::ViewportManagerDownLevel(ItemsRepeater* owner) :
    m_owner(owner),
    m_horizontalScroller(owner),
    m_verticalScroller(owner),
    m_innerScrollableScroller(owner),
    m_makeAnchorElement(owner),
    m_cacheBuildAction(owner)
{
    // ItemsRepeater is not fully constructed yet. Don't interact with it.
}

winrt::UIElement ViewportManagerDownLevel::SuggestedAnchor() const
{
    // The element generated during the ItemsRepeater.MakeAnchor call has precedence over the next tick.
    winrt::UIElement suggestedAnchor = m_makeAnchorElement.get();
    winrt::UIElement owner = *m_owner;

    if (!suggestedAnchor)
    {
        // We only care about what the first scrollable scroller is tracking (i.e. inner most).
        // A scroller is considered scrollable if IRepeaterScrollingSurface.IsHorizontallyScrollable
        // or IsVerticallyScroller is true.
        const auto anchorElement =
            m_innerScrollableScroller ?
            m_innerScrollableScroller.get().AnchorElement() :
            nullptr;

        if (anchorElement)
        {
            // We can't simply return anchorElement because, in case of nested Repeaters, it may not
            // be a direct child of ours, or even an indirect child. We need to walk up the tree starting
            // from anchorElement to figure out what child of ours (if any) to use as the suggested element.

            auto child = anchorElement;
            auto parent = CachedVisualTreeHelpers::GetParent(child).as<winrt::UIElement>();
            while (parent)
            {
                if (parent == owner)
                {
                    suggestedAnchor = child;
                    break;
                }

                child = parent;
                parent = CachedVisualTreeHelpers::GetParent(parent).as<winrt::UIElement>();
            }
        }
    }

    return suggestedAnchor;
}

void ViewportManagerDownLevel::HorizontalCacheLength(double value)
{
    if (m_maximumHorizontalCacheLength != value)
    {
        ValidateCacheLength(value);
        m_maximumHorizontalCacheLength = value;
        ResetCacheBuffer();
    }
}

void ViewportManagerDownLevel::VerticalCacheLength(double value)
{
    if (m_maximumVerticalCacheLength != value)
    {
        ValidateCacheLength(value);
        m_maximumVerticalCacheLength = value;
        ResetCacheBuffer();
    }
}

winrt::Rect ViewportManagerDownLevel::GetLayoutVisibleWindow() const
{
    auto visibleWindow = m_visibleWindow;

    if (m_makeAnchorElement && m_isAnchorOutsideRealizedRange)
    {
        // The anchor is not necessarily laid out yet. Its position should default
        // to zero and the layout origin is expected to change once layout is done.
        // Until then, we need a window that's going to protect the anchor from
        // getting recycled.

        // Also, we only want to mess with the realization rect iff the anchor is not inside it.
        // If we fiddle with an anchor that is already inside the realization rect,
        // shifting the realization rect results in repeater, layout and scroller thinking that it needs to act upon StartBringIntoView.
        // We do NOT want that!
        visibleWindow.X = 0.0f;
        visibleWindow.Y = 0.0f;
    }
    else if (HasScrollers())
    {
        visibleWindow.X += m_layoutExtent.X + m_expectedViewportShift.X;
        visibleWindow.Y += m_layoutExtent.Y + m_expectedViewportShift.Y;
    }

    return visibleWindow;
}

winrt::Rect ViewportManagerDownLevel::GetLayoutRealizationWindow() const
{
    auto realizationWindow = GetLayoutVisibleWindow();
    if (HasScrollers())
    {
        realizationWindow.X -= static_cast<float>(m_horizontalCacheBufferPerSide);
        realizationWindow.Y -= static_cast<float>(m_verticalCacheBufferPerSide);
        realizationWindow.Width += static_cast<float>(m_horizontalCacheBufferPerSide) * 2.0f;
        realizationWindow.Height += static_cast<float>(m_verticalCacheBufferPerSide) * 2.0f;
    }

    return realizationWindow;
}

void ViewportManagerDownLevel::SetLayoutExtent(winrt::Rect extent)
{
    m_expectedViewportShift.X += m_layoutExtent.X - extent.X;
    m_expectedViewportShift.Y += m_layoutExtent.Y - extent.Y;

    // We tolerate viewport imprecisions up to 1 pixel to avoid invaliding layout too much.
    if (std::abs(m_expectedViewportShift.X) > 1.f || std::abs(m_expectedViewportShift.Y) > 1.f)
    {
        REPEATER_TRACE_INFO(L"%ls: \tExpecting viewport shift of (%.0f,%.0f) \n",
            GetLayoutId().data(), m_expectedViewportShift.X, m_expectedViewportShift.Y);
    }

    m_layoutExtent = extent;

    // We just finished a measure pass and have a new extent.
    // Let's make sure the scrollers will run its arrange so that they track the anchor.
    const auto outerScroller = GetOuterScroller();
    if (outerScroller) { outerScroller.as<winrt::UIElement>().InvalidateArrange(); }
    if (m_horizontalScroller && m_horizontalScroller != outerScroller) { m_horizontalScroller.as<winrt::UIElement>().InvalidateArrange(); }
    if (m_verticalScroller && m_verticalScroller != outerScroller) { m_verticalScroller.as<winrt::UIElement>().InvalidateArrange(); }
}

void ViewportManagerDownLevel::OnLayoutChanged(bool isVirtualizing)
{
    m_managingViewportDisabled = !isVirtualizing;
    m_layoutExtent = {};
    m_expectedViewportShift = {};
    ResetCacheBuffer();
}

void ViewportManagerDownLevel::OnElementCleared(const winrt::UIElement& element)
{
    if (m_horizontalScroller)
    {
        m_horizontalScroller.get().UnregisterAnchorCandidate(element);
    }

    if (m_verticalScroller && m_verticalScroller != m_horizontalScroller)
    {
        m_verticalScroller.get().UnregisterAnchorCandidate(element);
    }
}

void ViewportManagerDownLevel::OnOwnerArranged()
{
    if (m_managingViewportDisabled)
    {
        return;
    }

    m_expectedViewportShift = {};

    EnsureScrollers();

    if (HasScrollers())
    {
        const double maximumHorizontalCacheBufferPerSide = m_maximumHorizontalCacheLength * m_visibleWindow.Width / 2.0;
        const double maximumVerticalCacheBufferPerSide = m_maximumVerticalCacheLength * m_visibleWindow.Height / 2.0;

        const bool continueBuildingCache =
            m_horizontalCacheBufferPerSide < maximumHorizontalCacheBufferPerSide ||
            m_verticalCacheBufferPerSide < maximumVerticalCacheBufferPerSide;

        if (continueBuildingCache)
        {
            m_horizontalCacheBufferPerSide += CacheBufferPerSideInflationPixelDelta;
            m_verticalCacheBufferPerSide += CacheBufferPerSideInflationPixelDelta;

            m_horizontalCacheBufferPerSide = std::min(m_horizontalCacheBufferPerSide, maximumHorizontalCacheBufferPerSide);
            m_verticalCacheBufferPerSide = std::min(m_verticalCacheBufferPerSide, maximumVerticalCacheBufferPerSide);

            // Since we grow the cache buffer at the end of the arrange pass,
            // we need to register work even if we just reached cache potential.
            RegisterCacheBuildWork();
        }
    }
}

void ViewportManagerDownLevel::OnMakeAnchor(const winrt::UIElement& anchor, const bool isAnchorOutsideRealizedRange)
{
    m_makeAnchorElement.set(anchor);
    m_isAnchorOutsideRealizedRange = isAnchorOutsideRealizedRange;
}

void ViewportManagerDownLevel::OnBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs args)
{
    if (!m_managingViewportDisabled)
    {
        // We do not animate bring-into-view operations where the anchor is disconnected because
        // it doesn't look good (the blank space is obvious because the layout can't keep track
        // of two realized ranges while the animation is going on).
        if (m_isAnchorOutsideRealizedRange)
        {
            args.AnimationDesired(false);
        }
    }
}

void ViewportManagerDownLevel::ResetScrollers()
{
    m_parentScrollers.clear();
    m_horizontalScroller.set(nullptr);
    m_verticalScroller.set(nullptr);
    m_innerScrollableScroller.set(nullptr);

    m_ensuredScrollers = false;
}

void ViewportManagerDownLevel::OnCacheBuildActionCompleted()
{
    m_cacheBuildAction.set(nullptr);
    m_owner->InvalidateMeasure();
}

void ViewportManagerDownLevel::OnViewportChanged(const winrt::IRepeaterScrollingSurface&, const bool isFinal)
{
    if (!m_managingViewportDisabled)
    {
        if (isFinal)
        {
            // Note that isFinal will never be true for input based manipulations.
            m_makeAnchorElement.set(nullptr);
            m_isAnchorOutsideRealizedRange = false;
        }

        TryInvalidateMeasure();
    }
}

void ViewportManagerDownLevel::OnPostArrange(const winrt::IRepeaterScrollingSurface&)
{
    if (!m_managingViewportDisabled)
    {
        UpdateViewport();

        if (m_visibleWindow == winrt::Rect())
        {
            // We got cleared.
            m_layoutExtent = {};
        }
        else
        {
            // Register our non-recycled children as candidates for element tracking.
            if (m_horizontalScroller || m_verticalScroller)
            {
                auto children = m_owner->Children();
                for (unsigned i = 0u; i < children.Size(); ++i)
                {
                    const auto element = children.GetAt(i);
                    const auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
                    if (virtInfo->IsHeldByLayout())
                    {
                        if (m_horizontalScroller)
                        {
                            m_horizontalScroller.get().RegisterAnchorCandidate(element);
                        }

                        if (m_verticalScroller && m_verticalScroller != m_horizontalScroller)
                        {
                            m_verticalScroller.get().RegisterAnchorCandidate(element);
                        }
                    }
                }
            }
        }
    }
}

void ViewportManagerDownLevel::OnConfigurationChanged(const winrt::IRepeaterScrollingSurface& sender)
{
    m_ensuredScrollers = false;
    TryInvalidateMeasure();
}

void ViewportManagerDownLevel::EnsureScrollers()
{
    if (!m_ensuredScrollers)
    {
        ResetScrollers();

        auto parent = CachedVisualTreeHelpers::GetParent(*m_owner);
        while (parent)
        {
            const auto scroller = parent.try_as<winrt::IRepeaterScrollingSurface>();
            if (scroller && AddScroller(scroller))
            {
                break;
            }

            parent = CachedVisualTreeHelpers::GetParent(parent);
        }

        if (m_parentScrollers.empty())
        {
            // We usually update the viewport in the post arrange handler. But, since we don't have
            // a scroller, let's do it now.
            UpdateViewport();
        }
        else
        {
            auto& outerScrollerInfo = m_parentScrollers.back();
            outerScrollerInfo.PostArrangeToken = outerScrollerInfo.Scroller().PostArrange(winrt::auto_revoke, { this, &ViewportManagerDownLevel::OnPostArrange });
        }

        m_ensuredScrollers = true;
    }
}

bool ViewportManagerDownLevel::AddScroller(const winrt::IRepeaterScrollingSurface& scroller)
{
    MUX_ASSERT(!(m_horizontalScroller && m_verticalScroller));

    const bool isHorizontallyScrollable = scroller.IsHorizontallyScrollable();
    const bool isVerticallyScrollable = scroller.IsVerticallyScrollable();
    const bool allScrollersSet = (m_horizontalScroller || isHorizontallyScrollable) && (m_verticalScroller || isVerticallyScrollable);
    const bool setHorizontalScroller = !m_horizontalScroller && isHorizontallyScrollable;
    const bool setVerticalScroller = !m_verticalScroller && isVerticallyScrollable;
    const bool setInnerScrollableScroller = !m_innerScrollableScroller && (setHorizontalScroller || setVerticalScroller);

    if (setHorizontalScroller) { m_horizontalScroller.set(scroller); }
    if (setVerticalScroller) { m_verticalScroller.set(scroller); }
    if (setInnerScrollableScroller) { m_innerScrollableScroller.set(scroller); }

    auto scrollerInfo = ScrollerInfo(
        m_owner,
        scroller);

    scrollerInfo.ConfigurationChangedToken = scroller.ConfigurationChanged(winrt::auto_revoke, { this, &ViewportManagerDownLevel::OnConfigurationChanged });
    if (setHorizontalScroller || setVerticalScroller)
    {
        scrollerInfo.ViewportChangedToken = scroller.ViewportChanged(winrt::auto_revoke, { this, &ViewportManagerDownLevel::OnViewportChanged });
    }

    m_parentScrollers.push_back(std::move(scrollerInfo));
    return allScrollersSet;
}

void ViewportManagerDownLevel::UpdateViewport()
{
    assert(!m_managingViewportDisabled);

    const auto previousVisibleWindow = m_visibleWindow;
    const auto horizontalVisibleWindow =
        m_horizontalScroller ?
        m_horizontalScroller.get().GetRelativeViewport(*m_owner) :
        winrt::Rect();
    const auto verticalVisibleWindow =
        m_verticalScroller ?
        (m_verticalScroller.get() == m_horizontalScroller.get() ?
            horizontalVisibleWindow :
            m_verticalScroller.get().GetRelativeViewport(*m_owner)) :
        winrt::Rect();
    const auto currentVisibleWindow = 
        HasScrollers() ?
        winrt::Rect
        {
            m_horizontalScroller ? horizontalVisibleWindow.X : verticalVisibleWindow.X,
            m_verticalScroller ? verticalVisibleWindow.Y : horizontalVisibleWindow.Y,
            m_horizontalScroller ? horizontalVisibleWindow.Width : verticalVisibleWindow.Width,
            m_verticalScroller ? verticalVisibleWindow.Height : horizontalVisibleWindow.Height
        } :
        winrt::Rect{ 0.0f, 0.0f, std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };

    if (-currentVisibleWindow.X <= ItemsRepeater::ClearedElementsArrangePosition.X &&
        -currentVisibleWindow.Y <= ItemsRepeater::ClearedElementsArrangePosition.Y)
    {
        // We got cleared.
        m_visibleWindow = {};
    }
    else
    {
        REPEATER_TRACE_INFO(L"%ls: \tViewport: (%.0f,%.0f,%.0f,%.0f)->(%.0f,%.0f,%.0f,%.0f). \n",
            GetLayoutId().data(),
            previousVisibleWindow.X, previousVisibleWindow.Y, previousVisibleWindow.Width, previousVisibleWindow.Height,
            currentVisibleWindow.X, currentVisibleWindow.Y, currentVisibleWindow.Width, currentVisibleWindow.Height);
        m_visibleWindow = currentVisibleWindow;
    }

    const bool viewportChanged =
        std::abs(m_visibleWindow.X - previousVisibleWindow.X) > 1 ||
        std::abs(m_visibleWindow.Y - previousVisibleWindow.Y) > 1 ||
        m_visibleWindow.Width != previousVisibleWindow.Width ||
        m_visibleWindow.Height != previousVisibleWindow.Height;

    if (viewportChanged)
    {
        TryInvalidateMeasure();
    }
}

void ViewportManagerDownLevel::ResetCacheBuffer()
{
    m_horizontalCacheBufferPerSide = 0.0;
    m_verticalCacheBufferPerSide = 0.0;

    if (!m_managingViewportDisabled)
    {
        // We need to start building the realization buffer again.
        RegisterCacheBuildWork();
    }
}

void ViewportManagerDownLevel::ValidateCacheLength(double cacheLength)
{
    if (cacheLength < 0.0 || std::isinf(cacheLength) || std::isnan(cacheLength))
    {
        throw winrt::hresult_invalid_argument(L"The maximum cache length must be equal or superior to zero.");
    }
}

void ViewportManagerDownLevel::RegisterCacheBuildWork()
{
    if (m_owner->Layout() &&
        !m_cacheBuildAction)
    {
        auto strongOwner = m_owner->get_strong();
        m_cacheBuildAction.set(m_owner
            ->Dispatcher()
            // We capture 'owner' (a strong refernce on ItemsRepeater) to make sure ItemsRepeater is still around
            // when the async action completes. By protecting ItemsRepeater, we also ensure that this instance
            // of ViewportManager (referenced by 'this' pointer) is valid because the lifetime of ItemsRepeater
            // and ViewportManager is the same (see ItemsRepeater::m_viewportManager).
            // We can't simply hold a strong reference on ViewportManager because it's not a COM object.
            .RunIdleAsync([this, strongOwner](const winrt::IdleDispatchedHandlerArgs&)
            {
                OnCacheBuildActionCompleted();
            }));
    }
}

void ViewportManagerDownLevel::TryInvalidateMeasure()
{
    // Don't invalidate measure if we have an invalid window.
    if (m_visibleWindow != winrt::Rect())
    {
        // We invalidate measure instead of just invalidating arrange because
        // we don't invalidate measure in UpdateViewport if the view is changing to
        // avoid layout cycles.
        REPEATER_TRACE_INFO(L"%ls: \tInvalidating measure due to viewport change. \n", GetLayoutId().data());
        m_owner->InvalidateMeasure();
    }
}

winrt::IRepeaterScrollingSurface ViewportManagerDownLevel::GetOuterScroller() const
{
    winrt::IRepeaterScrollingSurface scroller = nullptr;

    if (!m_parentScrollers.empty())
    {
        scroller = m_parentScrollers.back().Scroller();
    }

    return scroller;
}

winrt::hstring ViewportManagerDownLevel::GetLayoutId()
{
    winrt::hstring layoutId{};
    if (auto layout = m_owner->Layout())
    {
        layoutId = layout.as<Layout>()->LayoutId();
    }

    return layoutId;
}
