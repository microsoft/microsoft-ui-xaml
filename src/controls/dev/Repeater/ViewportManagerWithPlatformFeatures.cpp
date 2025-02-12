﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ViewportManagerWithPlatformFeatures.h"
#include "ItemsRepeater.h"
#include "Layout.h"

// Pixel delta by which to inflate the cache buffer on each side.  Rather than fill the entire
// cache buffer all at once, we chunk the work to make the UI thread more responsive.  We inflate
// the cache buffer from 0 to a max value determined by the Maximum[Horizontal,Vertical]CacheLength
// properties.
constexpr double CacheBufferPerSideInflationPixelDelta = 40.0;

ViewportManagerWithPlatformFeatures::ViewportManagerWithPlatformFeatures(ItemsRepeater* owner) :
    m_owner(owner),
    m_scroller(owner),
    m_makeAnchorElement(owner)
{
    // ItemsRepeater is not fully constructed yet. Don't interact with it.
}

winrt::UIElement ViewportManagerWithPlatformFeatures::SuggestedAnchor() const
{
    // The element generated during the ItemsRepeater.MakeAnchor call has precedence over the next tick.
    winrt::UIElement suggestedAnchor = m_makeAnchorElement.get();

    if (!suggestedAnchor)
    {
        const auto anchorElement =
            m_scroller ?
            m_scroller.get().CurrentAnchor() :
            nullptr;

        if (anchorElement)
        {
            // We can't simply return anchorElement because, in case of nested ItemsRepeaters, it may not
            // be a direct child of ours, or even an indirect child. We need to walk up the tree starting
            // from anchorElement to figure out what child of ours (if any) to use as the suggested element.
            winrt::UIElement owner = *m_owner;
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

            ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_PTR, METH_NAME, this, GetLayoutId().data(), L"SuggestedAnchor returns IScrollAnchorProvider.CurrentAnchor.", suggestedAnchor);
        }
#ifdef DBG
        else
        {
            ITEMSREPEATER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this, GetLayoutId().data(), L"SuggestedAnchor returns null.");
        }
#endif // DBG
    }
#ifdef DBG
    else
    {
        // Since we have an anchor element, we do not want the scroll anchor provider to start anchoring some other element.
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_PTR, METH_NAME, this, GetLayoutId().data(), L"SuggestedAnchor returns non-null m_makeAnchorElement.", m_makeAnchorElement);
    }
#endif // DBG
    return suggestedAnchor;
}

void ViewportManagerWithPlatformFeatures::HorizontalCacheLength(double value)
{
    if (m_maximumHorizontalCacheLength != value)
    {
        ValidateCacheLength(value);
        m_maximumHorizontalCacheLength = value;
        ResetCacheBuffer();
    }
}

void ViewportManagerWithPlatformFeatures::VerticalCacheLength(double value)
{
    if (m_maximumVerticalCacheLength != value)
    {
        ValidateCacheLength(value);
        m_maximumVerticalCacheLength = value;
        ResetCacheBuffer();
    }
}

winrt::Rect ViewportManagerWithPlatformFeatures::GetLayoutVisibleWindowDiscardAnchor() const
{
    auto visibleWindow = m_visibleWindow;

    if (HasScroller())
    {
        visibleWindow.X += m_layoutExtent.X + m_expectedViewportShift.X + m_unshiftableShift.X;
        visibleWindow.Y += m_layoutExtent.Y + m_expectedViewportShift.Y + m_unshiftableShift.Y;
    }

    return visibleWindow;
}

winrt::Rect ViewportManagerWithPlatformFeatures::GetLayoutVisibleWindow() const
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
    else if (HasScroller())
    {
        visibleWindow.X += m_layoutExtent.X + m_expectedViewportShift.X + m_unshiftableShift.X;
        visibleWindow.Y += m_layoutExtent.Y + m_expectedViewportShift.Y + m_unshiftableShift.Y;
    }

    return visibleWindow;
}

winrt::Rect ViewportManagerWithPlatformFeatures::GetLayoutRealizationWindow() const
{
    auto realizationWindow = GetLayoutVisibleWindow();
    if (HasScroller())
    {
        realizationWindow.X -= static_cast<float>(m_horizontalCacheBufferPerSide);
        realizationWindow.Y -= static_cast<float>(m_verticalCacheBufferPerSide);
        realizationWindow.Width += static_cast<float>(m_horizontalCacheBufferPerSide) * 2.0f;
        realizationWindow.Height += static_cast<float>(m_verticalCacheBufferPerSide) * 2.0f;
    }

    return realizationWindow;
}

void ViewportManagerWithPlatformFeatures::SetVisibleWindow(const winrt::Rect& visibleWindow)
{
    if (visibleWindow.X != m_visibleWindow.X || visibleWindow.Y != m_visibleWindow.Y ||
        visibleWindow.Width != m_visibleWindow.Width || visibleWindow.Height != m_visibleWindow.Height)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_visibleWindow set:",
            visibleWindow.X, visibleWindow.Y, visibleWindow.Width, visibleWindow.Height);

        m_visibleWindow = visibleWindow;
    }
}

void ViewportManagerWithPlatformFeatures::SetLastLayoutRealizationWindow(const winrt::Rect& layoutRealizationWindow)
{
    if (layoutRealizationWindow.X != m_lastLayoutRealizationWindow.X || layoutRealizationWindow.Y != m_lastLayoutRealizationWindow.Y ||
        layoutRealizationWindow.Width != m_lastLayoutRealizationWindow.Width || layoutRealizationWindow.Height != m_lastLayoutRealizationWindow.Height)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_lastLayoutRealizationWindow set:",
            layoutRealizationWindow.X, layoutRealizationWindow.Y, layoutRealizationWindow.Width, layoutRealizationWindow.Height);

        m_lastLayoutRealizationWindow = layoutRealizationWindow;
    }
}

void ViewportManagerWithPlatformFeatures::SetPendingViewportShift(const winrt::Point& pendingViewportShift)
{
    if (pendingViewportShift.X != m_pendingViewportShift.X || pendingViewportShift.Y != m_pendingViewportShift.Y)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_pendingViewportShift set:",
            pendingViewportShift.X, pendingViewportShift.Y);

        m_pendingViewportShift = pendingViewportShift;
    }
}

void ViewportManagerWithPlatformFeatures::SetExpectedViewportShift(float expectedViewportShiftX, float expectedViewportShiftY)
{
    if (expectedViewportShiftX != m_expectedViewportShift.X || expectedViewportShiftY != m_expectedViewportShift.Y)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_expectedViewportShift set:",
            expectedViewportShiftX, expectedViewportShiftY);

        m_expectedViewportShift.X = expectedViewportShiftX;
        m_expectedViewportShift.Y = expectedViewportShiftY;
    }
}

void ViewportManagerWithPlatformFeatures::SetUnshiftableShift(float unshiftableShiftX, float unshiftableShiftY)
{
    if (unshiftableShiftX != m_unshiftableShift.X || unshiftableShiftY != m_unshiftableShift.Y)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_unshiftableShift set:",
            unshiftableShiftX, unshiftableShiftY);

        m_unshiftableShift.X = unshiftableShiftX;
        m_unshiftableShift.Y = unshiftableShiftY;
    }
}

void ViewportManagerWithPlatformFeatures::SetLastScrollPresenterViewChangeCorrelationId(int correlationId)
{
    if (correlationId != m_lastScrollPresenterViewChangeCorrelationId)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_INT, METH_NAME, this,
            GetLayoutId().data(), L"m_lastScrollPresenterViewChangeCorrelationId set:", correlationId);

        m_lastScrollPresenterViewChangeCorrelationId = correlationId;
    }
}

void ViewportManagerWithPlatformFeatures::SetLayoutExtent(const winrt::Rect& layoutExtent)
{
#ifdef DBG
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
        GetLayoutId().data(), L"layoutExtent:", layoutExtent.X, layoutExtent.Y);

    TraceFieldsDbg();
#endif // DBG

    const float expectedViewportShiftX = m_expectedViewportShift.X + m_layoutExtent.X - layoutExtent.X;
    const float expectedViewportShiftY = m_expectedViewportShift.Y + m_layoutExtent.Y - layoutExtent.Y;

    SetExpectedViewportShift(expectedViewportShiftX, expectedViewportShiftY);

    // We tolerate viewport imprecisions up to 1 pixel to avoid invaliding layout too much.
    if (std::abs(m_expectedViewportShift.X) > 1.f || std::abs(m_expectedViewportShift.Y) > 1.f)
    {
        // There are cases where we might be expecting a shift but not get it. We will
        // be waiting for the effective viewport event but if the scroller is not able
        // to perform the shift (perhaps because it cannot scroll in negative offset),
        // then we will end up not realizing elements in the visible window.
        // To avoid this, we register to layout updated for this layout pass. If we 
        // get an effective viewport, we know we have a new viewport and we unregister from
        // layout updated. If we get the layout updated handler, then we know that the 
        // scroller was unable to perform the shift and we invalidate measure and unregister
        // from the layout updated event.
        if (!m_layoutUpdatedRevoker)
        {
            m_layoutUpdatedRevoker = m_owner->LayoutUpdated(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnLayoutUpdated });
        }
    }

    if (layoutExtent.X != m_layoutExtent.X || layoutExtent.Y != m_layoutExtent.Y ||
        layoutExtent.Width != m_layoutExtent.Width || layoutExtent.Height != m_layoutExtent.Height)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_layoutExtent set:",
            layoutExtent.X, layoutExtent.Y);

        m_layoutExtent = layoutExtent;
    }

    SetPendingViewportShift(m_expectedViewportShift);

    if (m_scroller)
    {
        // We just finished a measure pass and have a new extent.
        // Let's make sure the scroller will run its arrange so that it tracks the anchor.
        m_scroller.as<winrt::UIElement>().InvalidateArrange();
    }
}

void ViewportManagerWithPlatformFeatures::ResetLayoutExtent()
{
    if (m_layoutExtent.X != 0.0f || m_layoutExtent.Y != 0.0f ||
        m_layoutExtent.Width != 0.0f || m_layoutExtent.Height != 0.0f)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_layoutExtent reset:",
            m_layoutExtent.X, m_layoutExtent.Y, m_layoutExtent.Width, m_layoutExtent.Height);

        m_layoutExtent = {};
    }
}

void ViewportManagerWithPlatformFeatures::ResetVisibleWindow()
{
    if (m_visibleWindow.X != 0.0f || m_visibleWindow.Y != 0.0f ||
        m_visibleWindow.Width != 0.0f || m_visibleWindow.Height != 0.0f)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_visibleWindow reset:",
            m_visibleWindow.X, m_visibleWindow.Y, m_visibleWindow.Width, m_visibleWindow.Height);

        m_visibleWindow = {};
    }
}

void ViewportManagerWithPlatformFeatures::ResetLastLayoutRealizationWindow()
{
    if (m_lastLayoutRealizationWindow.X != 0.0f || m_lastLayoutRealizationWindow.Y != 0.0f ||
        m_lastLayoutRealizationWindow.Width != 0.0f || m_lastLayoutRealizationWindow.Height != 0.0f)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_lastLayoutRealizationWindow reset:",
            m_lastLayoutRealizationWindow.X, m_lastLayoutRealizationWindow.Y, m_lastLayoutRealizationWindow.Width, m_lastLayoutRealizationWindow.Height);

        m_lastLayoutRealizationWindow = {};
    }
}

void ViewportManagerWithPlatformFeatures::ResetExpectedViewportShift()
{
    if (m_expectedViewportShift.X != 0.0f || m_expectedViewportShift.Y != 0.0f)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_expectedViewportShift reset:",
            m_expectedViewportShift.X, m_expectedViewportShift.Y);

        m_expectedViewportShift = {};
    }
}

void ViewportManagerWithPlatformFeatures::ResetPendingViewportShift()
{
    if (m_pendingViewportShift.X != 0.0f || m_pendingViewportShift.Y != 0.0f)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_pendingViewportShift reset:",
            m_pendingViewportShift.X, m_pendingViewportShift.Y);

        m_pendingViewportShift = {};
    }
}

void ViewportManagerWithPlatformFeatures::ResetUnshiftableShift()
{
    if (m_unshiftableShift.X != 0.0f || m_unshiftableShift.Y != 0.0f)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"m_unshiftableShift reset:",
            m_unshiftableShift.X, m_unshiftableShift.Y);

        m_unshiftableShift = {};
    }
}

void ViewportManagerWithPlatformFeatures::ResetLastScrollPresenterViewChangeCorrelationId()
{
    if (m_lastScrollPresenterViewChangeCorrelationId != -1)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_INT, METH_NAME, this,
            GetLayoutId().data(), L"m_lastScrollPresenterViewChangeCorrelationId reset:",
            m_lastScrollPresenterViewChangeCorrelationId);

        m_lastScrollPresenterViewChangeCorrelationId = -1;
    }
}

#ifdef DBG
void ViewportManagerWithPlatformFeatures::OnScrollViewerViewChangingDbg(winrt::IInspectable const& /*sender*/, winrt::ScrollViewerViewChangingEventArgs const& args)
{
    const winrt::ScrollViewerView nextViewDbg = args.NextView();

    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_DBL_DBL, METH_NAME, this,
        GetLayoutId().data(), L"ScrollViewerViewChangingEventArgs nextView:", nextViewDbg.HorizontalOffset(), nextViewDbg.VerticalOffset());
}

void ViewportManagerWithPlatformFeatures::OnScrollViewerViewChangedDbg(winrt::IInspectable const& /*sender*/, winrt::ScrollViewerViewChangedEventArgs const& args)
{
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_INT, METH_NAME, this,
        GetLayoutId().data(), L"ScrollViewerViewChangedEventArgs.IsIntermediate", args.IsIntermediate());

    TraceScrollerDbg();
    TraceFieldsDbg();
}
#endif // DBG

void ViewportManagerWithPlatformFeatures::OnLayoutChanged(bool isVirtualizing)
{
    ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        GetLayoutId().data(), isVirtualizing);

    m_managingViewportDisabled = !isVirtualizing;

    ResetLayoutExtent();
    ResetExpectedViewportShift();
    ResetPendingViewportShift();

    if (m_managingViewportDisabled)
    {
        m_effectiveViewportChangedRevoker.revoke();
    }
    else if (!m_effectiveViewportChangedRevoker)
    {
        m_effectiveViewportChangedRevoker = m_owner->EffectiveViewportChanged(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnEffectiveViewportChanged });
    }

    ResetUnshiftableShift();
    ResetCacheBuffer();
}

void ViewportManagerWithPlatformFeatures::OnElementPrepared(const winrt::UIElement& element)
{
    ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    // The newly prepared element is not registered as an anchor candidate right away. It first needs to be arranged at least once so its position
    // tracked by the scroller is valid.
    // The element is first put into a list of prepared elements. Then once arranged it is put into a list of prepared+arranged elements. Then finally
    // at the beginning of the following measure pass, it is registered as an anchor candidate.

    const auto itPreparedElement = std::find_if(m_preparedElements.cbegin(), m_preparedElements.cend(), [&element](const tracker_ref<winrt::UIElement>& preparedElement) { return preparedElement.get() == element; });

    if (itPreparedElement == m_preparedElements.cend())
    {
        m_preparedElements.push_back(tracker_ref<winrt::UIElement>{ m_owner, element });
    }
}

void ViewportManagerWithPlatformFeatures::OnElementCleared(const winrt::UIElement& element)
{
    ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    // Remove the element from the prepared and prepared+arranged lists so it no longer gets registered as an anchor candidate for the scroller
    // after the next arrange pass.

    const auto itPreparedElement = std::find_if(m_preparedElements.cbegin(), m_preparedElements.cend(),
        [&element](const tracker_ref<winrt::UIElement>& preparedElement) { return preparedElement.get() == element; });

    if (itPreparedElement != m_preparedElements.cend())
    {
        m_preparedElements.erase(itPreparedElement);
    }

    const auto itPreparedAndArrangedElement = std::find_if(m_preparedAndArrangedElements.cbegin(), m_preparedAndArrangedElements.cend(),
        [&element](const tracker_ref<winrt::UIElement>& preparedAndArrangedElement) { return preparedAndArrangedElement.get() == element; });

    if (itPreparedAndArrangedElement != m_preparedAndArrangedElements.cend())
    {
        m_preparedAndArrangedElements.erase(itPreparedAndArrangedElement);
    }

    if (element.CanBeScrollAnchor())
    {
        // Unregister the element as an anchor candidate since it was already declared as such.
        element.CanBeScrollAnchor(false);
    }
}

void ViewportManagerWithPlatformFeatures::OnOwnerMeasuring()
{
    // This is because of a bug that causes effective viewport to not 
    // fire if you register during arrange.
    // Bug 17411076: EffectiveViewport: registering for effective viewport in arrange should invalidate viewport
    EnsureScroller();

    winrt::Rect currentLayoutRealizationWindow = GetLayoutRealizationWindow();

    if ((m_horizontalCacheBufferPerSide != 0.0 || m_verticalCacheBufferPerSide != 0.0) &&
        (m_lastLayoutRealizationWindow.Width <= 0.0f || m_lastLayoutRealizationWindow.Height <= 0.0f ||
         currentLayoutRealizationWindow.Width <= 0.0f || currentLayoutRealizationWindow.Height <= 0.0f ||
         !SharedHelpers::DoRectsIntersect(m_lastLayoutRealizationWindow, currentLayoutRealizationWindow)))
    {
        // Two consecutive measure passes use disconnected realization windows.
        // Reset the potential cache buffer so that it regrows from scratch.
        ResetLayoutRealizationWindowCacheBuffer();
    }

    SetLastLayoutRealizationWindow(currentLayoutRealizationWindow);

    if (m_skipScrollAnchorRegistrationsDuringNextMeasurePass)
    {
        ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"m_skipScrollAnchorRegistrationsDuringNextMeasurePass reset.");

        m_skipScrollAnchorRegistrationsDuringNextMeasurePass = false;
    }
    else if (!m_skipScrollAnchorRegistrationsDuringNextArrangePass)
    {
        // Now that a new measure pass is starting, register the previously arranged elements as anchor candidates for the scroller.
        RegisterPreparedAndArrangedElementsAsScrollAnchorCandidates();
    }

#ifdef DBG
    TraceScrollerDbg();
#endif // DBG
}

void ViewportManagerWithPlatformFeatures::OnOwnerArranged()
{
#ifdef DBG
    ITEMSREPEATER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, GetLayoutId().data());

    TraceScrollerDbg();
    TraceFieldsDbg();
#endif // DBG

    if (!m_skipScrollAnchorRegistrationsDuringNextMeasurePass)
    {
        if (m_skipScrollAnchorRegistrationsDuringNextArrangePass)
        {
            ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"m_skipScrollAnchorRegistrationsDuringNextArrangePass reset.");

            m_skipScrollAnchorRegistrationsDuringNextArrangePass = false;
        }
        else
        {
            // Now that an arrange pass completed, register the prepared elements as arranged. They will be registered as anchor candidates
            // during the next measure pass.
            RegisterPreparedElementsAsArranged();
        }
    }

    ResetExpectedViewportShift();

    if (m_unshiftableShift.X != 0.0f || m_unshiftableShift.Y != 0.0f)
    {
        double horizontalOffset{};
        double verticalOffset{};

        if (const auto scrollViewer = m_scroller.try_as<winrt::Controls::IScrollViewer>())
        {
            horizontalOffset = scrollViewer.HorizontalOffset();
            verticalOffset = scrollViewer.VerticalOffset();
        }
        else if (const auto scrollPresenter = m_scroller.try_as<winrt::Controls::Primitives::IScrollPresenter>())
        {
            horizontalOffset = scrollPresenter.HorizontalOffset();
            verticalOffset = scrollPresenter.VerticalOffset();
        }

        if ((m_unshiftableShift.X != 0.0f && std::abs(m_visibleWindow.X - horizontalOffset) < 1.0) ||
            (m_unshiftableShift.Y != 0.0f && std::abs(m_visibleWindow.Y - verticalOffset) < 1.0))
        {
            // Reset m_unshiftableShift & trigger measure path. This situation arises for example when the first item
            // in a collection is removed and the layout origin is increased.
            ResetUnshiftableShift();
            TryInvalidateMeasure();
        }
    }

    if (!m_managingViewportDisabled)
    {
        // This is because of a bug that causes effective viewport to not 
        // fire if you register during arrange.
        // Bug 17411076: EffectiveViewport: registering for effective viewport in arrange should invalidate viewport
        // EnsureScroller();

        if (HasScroller())
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
}

void ViewportManagerWithPlatformFeatures::OnLayoutUpdated(winrt::IInspectable const& sender, winrt::IInspectable const& args)
{
    ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, GetLayoutId().data());

    m_layoutUpdatedRevoker.revoke();

    if (m_managingViewportDisabled)
    {
        return;
    }

    // We were expecting a viewport shift but we never got one and we are not going to in this
    // layout pass. We likely will never get this shift, so lets assume that we are never going to get it and
    // adjust our expected shift to track that. One case where this can happen is when there is no scroller
    // that can scroll in the direction where the shift is expected.
    if (m_pendingViewportShift.X != 0.0f || m_pendingViewportShift.Y != 0.0f)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
            GetLayoutId().data(), L"Layout Updated with pending shift - invalidating measure",
            m_pendingViewportShift.X,
            m_pendingViewportShift.Y);

        // Assume this is never going to come.
        SetUnshiftableShift(m_unshiftableShift.X + m_pendingViewportShift.X, m_unshiftableShift.Y + m_pendingViewportShift.Y);
        ResetPendingViewportShift();
        ResetExpectedViewportShift();

        TryInvalidateMeasure();
    }
}

void ViewportManagerWithPlatformFeatures::OnMakeAnchor(const winrt::UIElement& anchor, const bool isAnchorOutsideRealizedRange)
{
#ifdef DBG
    if (anchor == nullptr)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_PTR, METH_NAME, this, GetLayoutId().data(), L"Resets m_makeAnchorElement.", m_makeAnchorElement);
    }
    else
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_PTR, METH_NAME, this, GetLayoutId().data(), L"Sets m_makeAnchorElement.", anchor);
    }
#endif // DBG

    m_makeAnchorElement.set(anchor);
    m_isAnchorOutsideRealizedRange = isAnchorOutsideRealizedRange;
}

void ViewportManagerWithPlatformFeatures::OnBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs& args)
{
    ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, GetLayoutId().data());

    if (!m_managingViewportDisabled)
    {
        // We do not animate bring-into-view operations where the anchor is disconnected because
        // it doesn't look good (the blank space is obvious because the layout can't keep track
        // of two realized ranges while the animation is going on).
        if (m_isAnchorOutsideRealizedRange)
        {
            args.AnimationDesired(false);
        }

        // During the time between a bring into view request and the element coming into view we do not
        // want the anchor provider to pick some anchor and jump to it. Instead we want to anchor on the
        // element that is being brought into view. We can do this by making just that element as a potential
        // anchor candidate and ensure no other element of this repeater is an anchor candidate.
        // Once the layout pass is done and we render the frame, the element will be in frame and we can
        // switch back to letting the anchor provider pick a suitable anchor.

        // get the targetChild - i.e the immediate child of this repeater that is being brought into view.
        // Note that the element being brought into view could be a descendant.
        const auto targetChild = GetImmediateChildOfRepeater(args.TargetElement());

        // Make sure that only the target child can be the anchor during the bring into view operation.
        UnregisterScrollAnchorCandidates(targetChild /*exceptionElement*/, false /*registerAsPreparedAndArrangedElements*/);

        // Register to rendering event to go back to how things were before where any child can be the anchor.
        m_isBringIntoViewInProgress = true;
        if (!m_renderingToken)
        {
            winrt::Microsoft::UI::Xaml::Media::CompositionTarget compositionTarget{ nullptr };
            m_renderingToken = compositionTarget.Rendering(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnCompositionTargetRendering });
        }
    }
}

winrt::UIElement ViewportManagerWithPlatformFeatures::GetImmediateChildOfRepeater(winrt::UIElement const& descendant)
{
    winrt::UIElement targetChild = descendant;
    winrt::UIElement parent = CachedVisualTreeHelpers::GetParent(descendant).as<winrt::UIElement>();
    while (parent != nullptr && parent != static_cast<winrt::DependencyObject>(*m_owner))
    {
        targetChild = parent;
        parent = CachedVisualTreeHelpers::GetParent(parent).as<winrt::UIElement>();
    }

    if (!parent)
    {
        throw winrt::hresult_error(E_FAIL, L"OnBringIntoViewRequested called with args.target element not under the ItemsRepeater that recieved the call");
    }

    return targetChild;
}

void ViewportManagerWithPlatformFeatures::OnCompositionTargetRendering(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_PTR, METH_NAME, this, GetLayoutId().data(), L"Resets m_makeAnchorElement.", m_makeAnchorElement);

    MUX_ASSERT(!m_managingViewportDisabled);

    m_renderingToken.revoke();

    m_isBringIntoViewInProgress = false;

    if (m_makeAnchorElement)
    {
        m_makeAnchorElement.set(nullptr);

        if (m_isAnchorOutsideRealizedRange)
        {
            m_isAnchorOutsideRealizedRange = false;

            // During the bring-into-view operation, the layout anchor was positioned at the top/left
            // of the viewport (see ViewportManagerWithPlatformFeatures::GetLayoutVisibleWindow()).
            // Now it may move within the viewport and require different items to be generated given
            // its final position. Thus a new measure pass is requested.
            TryInvalidateMeasure();
        }
    }

    // Now that the item has been brought into view, we can let the anchor provider pick a new anchor.
    for (const auto& child : m_owner->Children())
    {
        if (!child.CanBeScrollAnchor())
        {
            auto info = ItemsRepeater::GetVirtualizationInfo(child);
            if (info->IsRealized() && info->IsHeldByLayout())
            {
                child.CanBeScrollAnchor(true);
            }
        }
    }
}

void ViewportManagerWithPlatformFeatures::ResetScrollers()
{
#ifdef DBG
    m_scrollViewerViewChangingRevokerDbg.revoke();
    m_scrollViewerViewChangedRevokerDbg.revoke();
#endif // DBG
    m_scroller.set(nullptr);
    m_scrollPresenterScrollStartingRevoker.revoke();
    m_scrollPresenterScrollCompletedRevoker.revoke();
    m_scrollPresenterZoomStartingRevoker.revoke();
    m_scrollPresenterZoomCompletedRevoker.revoke();
    m_effectiveViewportChangedRevoker.revoke();
    m_isAnchorOutsideRealizedRange = false;
    m_skipScrollAnchorRegistrationsDuringNextMeasurePass = false;
    m_skipScrollAnchorRegistrationsDuringNextArrangePass = false;
    m_ensuredScroller = false;
    ResetExpectedViewportShift();
    ResetPendingViewportShift();
    ResetUnshiftableShift();
    ResetLastScrollPresenterViewChangeCorrelationId();
}

void ViewportManagerWithPlatformFeatures::OnCacheBuildActionCompleted()
{
    if (m_cacheBuildActionOutstanding)
    {
        ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this, GetLayoutId().data(), L"m_cacheBuildActionOutstanding reset.");

        m_cacheBuildActionOutstanding = false;
    }

    if (!m_managingViewportDisabled)
    {
        m_owner->InvalidateMeasure();
    }
}

// Detect an imminent ScrollPresenter view change triggered by a ScrollTo, ScrollBy, ZoomTo, or ZoomBy call (which is likely originated from a ScrollBar interaction)
// in order to generate items around the destination area. This is to avoid the ScrollPresenter showing blank content because the Compositor thread moved the ScrollPresenter.Content
// visual before the ItemsRepeater was measured and arranged for the destination view.
// This event handler checks if that anticipated view, without UI re-generation, would create a blank region in the ScrollPresenter. In the interest of performance, less than
// 5% blank is tolerated and will not trigger a re-generation. Beyond that, UpdateViewport is called with the anticipated realization window. This is trigger an ItemsRepeater
// measure/arrange pass based on the anticipated view.
void ViewportManagerWithPlatformFeatures::OnScrollPresenterScrollStarting(winrt::ScrollPresenter const& scrollPresenter, winrt::ScrollingScrollStartingEventArgs const& args)
{
    OnScrollPresenterViewChangeStarting(scrollPresenter, args.CorrelationId(), args.HorizontalOffset(), args.VerticalOffset(), args.ZoomFactor());
}

void ViewportManagerWithPlatformFeatures::OnScrollPresenterZoomStarting(winrt::ScrollPresenter const& scrollPresenter, winrt::ScrollingZoomStartingEventArgs const& args)
{
    OnScrollPresenterViewChangeStarting(scrollPresenter, args.CorrelationId(), args.HorizontalOffset(), args.VerticalOffset(), args.ZoomFactor());
}

void ViewportManagerWithPlatformFeatures::OnScrollPresenterViewChangeStarting(winrt::ScrollPresenter const& scrollPresenter, int correlationId, double horizontalOffset, double verticalOffset, float zoomFactor)
{
    // No UI re-generation is performed if the last recorded realization window, m_lastLayoutRealizationWindow, is empty. This avoids UI generation when the actual effective viewport is empty.
    if (m_lastLayoutRealizationWindow.Width == 0.0f || m_lastLayoutRealizationWindow.Height == 0.0f)
    {
        return;
    }

    const float anticipatedHorizontalOffset = static_cast<float>(horizontalOffset);
    const float anticipatedVerticalOffset = static_cast<float>(verticalOffset);
    const float anticipatedZoomFactor = zoomFactor;
    const float anticipatedExtentWidth = static_cast<float>(scrollPresenter.ExtentWidth() * anticipatedZoomFactor);
    const float anticipatedExtentHeight = static_cast<float>(scrollPresenter.ExtentHeight() * anticipatedZoomFactor);
    const float viewportWidth = static_cast<float>(scrollPresenter.ViewportWidth());
    const float viewportHeight = static_cast<float>(scrollPresenter.ViewportHeight());
    const float unzoomedAnticipatedHorizontalOffset = anticipatedHorizontalOffset / anticipatedZoomFactor;
    const float unzoomedAnticipatedVerticalOffset = anticipatedVerticalOffset / anticipatedZoomFactor;
    const float scaledViewportWidth = viewportWidth / anticipatedZoomFactor;
    const float scaledViewportHeight = viewportHeight / anticipatedZoomFactor;
    const winrt::UIElement owner = *m_owner;
    const winrt::FrameworkElement ownerAsFE = owner.as<winrt::FrameworkElement>();

#ifdef DBG
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_INT, METH_NAME, this, GetLayoutId().data(),
        L"CorrelationId:", correlationId);
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this, GetLayoutId().data(),
        L"Anticipated HorizontalOffset, VerticalOffset:", anticipatedHorizontalOffset, anticipatedVerticalOffset);
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this, GetLayoutId().data(),
        L"Anticipated ZoomFactor:", anticipatedZoomFactor);

    TraceFieldsDbg();
#endif // DBG

    winrt::Rect anticipatedLayoutRealizationWindow = {
        unzoomedAnticipatedHorizontalOffset,
        unzoomedAnticipatedVerticalOffset,
        scaledViewportWidth,
        scaledViewportHeight };

    if (anticipatedExtentWidth < viewportWidth)
    {
        const winrt::HorizontalAlignment horizontalAlignment = ownerAsFE.HorizontalAlignment();

        if (horizontalAlignment == winrt::HorizontalAlignment::Center || horizontalAlignment == winrt::HorizontalAlignment::Stretch)
        {
            anticipatedLayoutRealizationWindow.X -= (viewportWidth - anticipatedExtentWidth) / 2.0f / anticipatedZoomFactor;
        }
        else if (horizontalAlignment == winrt::HorizontalAlignment::Right)
        {
            anticipatedLayoutRealizationWindow.X -= (viewportWidth - anticipatedExtentWidth) / anticipatedZoomFactor;
        }
    }

    if (anticipatedExtentHeight < viewportHeight)
    {
        const winrt::VerticalAlignment verticalAlignment = ownerAsFE.VerticalAlignment();

        if (verticalAlignment == winrt::VerticalAlignment::Center || verticalAlignment == winrt::VerticalAlignment::Stretch)
        {
            anticipatedLayoutRealizationWindow.Y -= (viewportHeight - anticipatedExtentHeight) / 2.0f / anticipatedZoomFactor;
        }
        else if (verticalAlignment == winrt::VerticalAlignment::Bottom)
        {
            anticipatedLayoutRealizationWindow.Y -= (viewportHeight - anticipatedExtentHeight) / anticipatedZoomFactor;
        }
    }

    // Using the smallest effective viewport, viewportWidth x viewportHeight, to cover the ScrollPresenter's viewport. It may still be too large if a part of the scroller is not displayed on screen.
    const bool invalidatedMeasure = UpdateViewport(anticipatedLayoutRealizationWindow);

    if (invalidatedMeasure)
    {
        UnregisterScrollAnchorCandidates(nullptr /*exceptionElement*/, true /*registerAsPreparedAndArrangedElements*/);

        ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"m_skipScrollAnchorRegistrationsDuringNextMeasurePass/m_skipScrollAnchorRegistrationsDuringNextArrangePass set.");

        // During such a UI re-generation, scroll anchoring is momentarily turned off to avoid unwanted anchoring-driven scrolls.
        m_skipScrollAnchorRegistrationsDuringNextMeasurePass = true;
        m_skipScrollAnchorRegistrationsDuringNextArrangePass = true;

        SetLastScrollPresenterViewChangeCorrelationId(correlationId);
    }
}

void ViewportManagerWithPlatformFeatures::OnScrollPresenterScrollCompleted(winrt::ScrollPresenter const& scrollPresenter, winrt::ScrollingScrollCompletedEventArgs const& args)
{
#ifdef DBG
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, GetLayoutId().data(), args.CorrelationId());

    TraceScrollerDbg();
#endif // DBG

    OnScrollPresenterViewChangeCompleted(args.CorrelationId());
}

void ViewportManagerWithPlatformFeatures::OnScrollPresenterZoomCompleted(winrt::ScrollPresenter const& scrollPresenter, winrt::ScrollingZoomCompletedEventArgs const& args)
{
#ifdef DBG
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, GetLayoutId().data(), args.CorrelationId());

    TraceScrollerDbg();
#endif // DBG

    OnScrollPresenterViewChangeCompleted(args.CorrelationId());
}

void ViewportManagerWithPlatformFeatures::OnScrollPresenterViewChangeCompleted(int correlationId)
{
    if (correlationId == m_lastScrollPresenterViewChangeCorrelationId && m_lastScrollPresenterViewChangeCorrelationId != -1)
    {
        ResetLastScrollPresenterViewChangeCorrelationId();
    }
}

void ViewportManagerWithPlatformFeatures::OnEffectiveViewportChanged(winrt::FrameworkElement const& sender, winrt::EffectiveViewportChangedEventArgs const& args)
{
    const winrt::Rect effectiveViewport = args.EffectiveViewport();

#ifdef DBG
    MUX_ASSERT(!m_managingViewportDisabled);

    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this, GetLayoutId().data(),
        L"EffectiveViewport:", effectiveViewport.X, effectiveViewport.Y, effectiveViewport.Width, effectiveViewport.Height);

    TraceScrollerDbg();
#endif // DBG

    if (m_lastScrollPresenterViewChangeCorrelationId != -1)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_INT, METH_NAME, this, GetLayoutId().data(), L"Early exit during ScrollPresenter view change.", m_lastScrollPresenterViewChangeCorrelationId);

        return;
    }

    bool invalidateMeasure = false;
    const bool invalidatedMeasure = UpdateViewport(effectiveViewport);
    const winrt::Point emptyPoint = {};
    const winrt::Rect emptyRect = {};

    if (m_pendingViewportShift != emptyPoint)
    {
        ResetPendingViewportShift();
        invalidateMeasure = true;
    }

    if (m_unshiftableShift != emptyPoint)
    {
        ResetUnshiftableShift();
        invalidateMeasure = true;
    }

    if (m_visibleWindow == emptyRect && m_layoutExtent != emptyRect)
    {
        // We got cleared.
        ResetLayoutExtent();
        invalidateMeasure = true;
    }

    if (invalidateMeasure && !invalidatedMeasure)
    {
        TryInvalidateMeasure();
    }

    // We got a new viewport, we don't need to wait for layout updated anymore to 
    // see if our request for a pending shift was handled.
    m_layoutUpdatedRevoker.revoke();
}

void ViewportManagerWithPlatformFeatures::EnsureScroller()
{
    if (!m_ensuredScroller)
    {
        ResetScrollers();

        auto parent = CachedVisualTreeHelpers::GetParent(*m_owner);
        while (parent)
        {
            if (const auto scroller = parent.try_as<winrt::Controls::IScrollAnchorProvider>())
            {
                m_scroller.set(scroller);
                break;
            }

            parent = CachedVisualTreeHelpers::GetParent(parent);
        }

        if (!m_managingViewportDisabled)
        {
            // When the ItemsRepeater is hosted in a Popup, m_scroller is null, but ItemsRepeater::EffectiveViewportChanged will still get raised.
            m_effectiveViewportChangedRevoker = m_owner->EffectiveViewportChanged(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnEffectiveViewportChanged });
        }

        if (const auto scrollPresenter = m_scroller.try_as<winrt::IScrollPresenter>())
        {
            if (const auto scrollPresenter2 = m_scroller.try_as<winrt::IScrollPresenter2>())
            {
                m_scrollPresenterScrollStartingRevoker = scrollPresenter2.ScrollStarting(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnScrollPresenterScrollStarting });
                m_scrollPresenterScrollCompletedRevoker = scrollPresenter.ScrollCompleted(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnScrollPresenterScrollCompleted });
                m_scrollPresenterZoomStartingRevoker = scrollPresenter2.ZoomStarting(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnScrollPresenterZoomStarting });
                m_scrollPresenterZoomCompletedRevoker = scrollPresenter.ZoomCompleted(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnScrollPresenterZoomCompleted });
            }
        }
#ifdef DBG
        else if (const auto scrollViewer = m_scroller.try_as<winrt::Controls::IScrollViewer>())
        {
            m_scrollViewerViewChangingRevokerDbg = scrollViewer.ViewChanging(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnScrollViewerViewChangingDbg });
            m_scrollViewerViewChangedRevokerDbg = scrollViewer.ViewChanged(winrt::auto_revoke, { this, &ViewportManagerWithPlatformFeatures::OnScrollViewerViewChangedDbg });
        }
#endif // DBG

        m_ensuredScroller = true;
    }
}

// Returns True when m_visibleWindow changes and TryInvalidateMeasure is invoked.
bool ViewportManagerWithPlatformFeatures::UpdateViewport(winrt::Rect const& effectiveViewport)
{
#ifdef DBG
    MUX_ASSERT(!m_managingViewportDisabled);

    const auto previousVisibleWindowDbg = m_visibleWindow;

    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this, GetLayoutId().data(),
        L"Previous Effective Viewport:", previousVisibleWindowDbg.X, previousVisibleWindowDbg.Y, previousVisibleWindowDbg.Width, previousVisibleWindowDbg.Height);
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this, GetLayoutId().data(),
        L"Current Effective Viewport:", effectiveViewport.X, effectiveViewport.Y, effectiveViewport.Width, effectiveViewport.Height);
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this, GetLayoutId().data(),
        L"Effective Viewport shift:", effectiveViewport.X - previousVisibleWindowDbg.X, effectiveViewport.Y - previousVisibleWindowDbg.Y, effectiveViewport.Width - previousVisibleWindowDbg.Width, effectiveViewport.Height - previousVisibleWindowDbg.Height);
#endif // DBG

    if (-effectiveViewport.X <= ItemsRepeater::ClearedElementsArrangePosition.X &&
        -effectiveViewport.Y <= ItemsRepeater::ClearedElementsArrangePosition.Y)
    {
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this, GetLayoutId().data(), L"Viewport is invalid. Visible window cleared.");

        // We got cleared.
        ResetVisibleWindow();
    }
    else
    {
        const float roundingTolerance = 0.01f;

        if (std::abs(m_visibleWindow.X - effectiveViewport.X) > roundingTolerance ||
            std::abs(m_visibleWindow.Y - effectiveViewport.Y) > roundingTolerance ||
            std::abs(m_visibleWindow.Width - effectiveViewport.Width) > roundingTolerance ||
            std::abs(m_visibleWindow.Height - effectiveViewport.Height) > roundingTolerance)
        {
            SetVisibleWindow(effectiveViewport);

            TryInvalidateMeasure();
            return true;
        }
    }

    return false;
}

void ViewportManagerWithPlatformFeatures::ResetLayoutRealizationWindowCacheBuffer()
{
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, GetLayoutId().data());

    ResetCacheBuffer(false /*registerCacheBuildWork*/);
}

void ViewportManagerWithPlatformFeatures::ResetCacheBuffer(bool registerCacheBuildWork)
{
#ifdef DBG
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_INT, METH_NAME, this,
        GetLayoutId().data(), L"registerCacheBuildWork:", registerCacheBuildWork);

    if (m_horizontalCacheBufferPerSide != 0.0)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_DBL, METH_NAME, this,
            GetLayoutId().data(), L"m_horizontalCacheBufferPerSide reset:", m_horizontalCacheBufferPerSide);
    }
    if (m_verticalCacheBufferPerSide != 0.0)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_DBL, METH_NAME, this,
            GetLayoutId().data(), L"m_verticalCacheBufferPerSide reset:", m_verticalCacheBufferPerSide);
    }
#endif // DBG

    m_horizontalCacheBufferPerSide = 0.0;
    m_verticalCacheBufferPerSide = 0.0;

    if (!m_managingViewportDisabled && registerCacheBuildWork)
    {
        // We need to start building the realization buffer again.
        RegisterCacheBuildWork();
    }
}

void ViewportManagerWithPlatformFeatures::ValidateCacheLength(double cacheLength)
{
    if (cacheLength < 0.0 || std::isinf(cacheLength) || std::isnan(cacheLength))
    {
        throw winrt::hresult_invalid_argument(L"The maximum cache length must be equal or superior to zero.");
    }
}

void ViewportManagerWithPlatformFeatures::RegisterPreparedElementsAsArranged()
{
    ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_preparedElements.size:", m_preparedElements.size());

    if (m_preparedElements.size() == 0)
    {
        return;
    }

    for (tracker_ref<winrt::UIElement> preparedElementTracker : m_preparedElements)
    {
        const winrt::UIElement preparedElement = preparedElementTracker.get();

        const auto it = std::find_if(m_preparedAndArrangedElements.cbegin(), m_preparedAndArrangedElements.cend(),
            [&preparedElement](const tracker_ref<winrt::UIElement>& preparedAndArrangedElement) { return preparedAndArrangedElement.get() == preparedElement; });

        if (it == m_preparedAndArrangedElements.cend())
        {
            m_preparedAndArrangedElements.push_back(tracker_ref<winrt::UIElement>{ m_owner, preparedElement });
        }
    }

    m_preparedElements.clear();
}

void ViewportManagerWithPlatformFeatures::RegisterPreparedAndArrangedElementsAsScrollAnchorCandidates()
{
    ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_preparedAndArrangedElements.size:", m_preparedAndArrangedElements.size());

    if (m_preparedAndArrangedElements.size() == 0)
    {
        return;
    }

    for (tracker_ref<winrt::UIElement> anchorCandidateTracker : m_preparedAndArrangedElements)
    {
        const winrt::UIElement anchorCandidate = anchorCandidateTracker.get();

        if (!anchorCandidate.CanBeScrollAnchor())
        {
            const auto info = ItemsRepeater::GetVirtualizationInfo(anchorCandidate);

            if (info->IsRealized() && info->IsHeldByLayout())
            {
                anchorCandidate.CanBeScrollAnchor(true);
            }
        }
    }

    m_preparedAndArrangedElements.clear();
}

void ViewportManagerWithPlatformFeatures::RegisterCacheBuildWork()
{
    MUX_ASSERT(!m_managingViewportDisabled);

    if (!m_cacheBuildActionOutstanding)
    {
        // We capture 'owner' (a strong reference on ItemsRepeater) to make sure ItemsRepeater is still around
        // when the async action completes. By protecting ItemsRepeater, we also ensure that this instance
        // of ViewportManager (referenced by 'this' pointer) is valid because the lifetime of ItemsRepeater
        // and ViewportManager is the same (see ItemsRepeater::m_viewportManager).
        // We can't simply hold a strong reference on ViewportManager because it's not a COM object.
        auto strongOwner = m_owner->get_strong();
        m_cacheBuildActionOutstanding = 
            m_owner->DispatcherQueue().TryEnqueue([this, strongOwner](auto&&...)
            {
                OnCacheBuildActionCompleted();
            });

        ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_STR_INT, METH_NAME, this,
            GetLayoutId().data(), L"m_cacheBuildActionOutstanding set:", m_cacheBuildActionOutstanding);
    }
#ifdef DBG
    else
    {
        ITEMSREPEATER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this,
            GetLayoutId().data(), L"m_cacheBuildActionOutstanding already set.");
    }
#endif // DBG
}

void ViewportManagerWithPlatformFeatures::TryInvalidateMeasure()
{
    // Don't invalidate measure if we have an invalid window.
    if (m_visibleWindow != winrt::Rect())
    {
        // We invalidate measure instead of just invalidating arrange because
        // we don't invalidate measure in UpdateViewport if the view is changing to
        // avoid layout cycles.
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this, GetLayoutId().data(), L"Invalidating measure due to viewport change.");

        m_owner->InvalidateMeasure();
    }
}

void ViewportManagerWithPlatformFeatures::UnregisterScrollAnchorCandidates(winrt::UIElement const& exceptionElement, bool registerAsPreparedAndArrangedElements)
{
    ITEMSREPEATER_TRACE_VERBOSE_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"registerAsPreparedAndArrangedElements:", registerAsPreparedAndArrangedElements);

    for (const winrt::UIElement& child : m_owner->Children())
    {
        if (child.CanBeScrollAnchor() && child != exceptionElement)
        {
            child.CanBeScrollAnchor(false);

            if (registerAsPreparedAndArrangedElements)
            {
                m_preparedAndArrangedElements.push_back(tracker_ref<winrt::UIElement>{ m_owner, child });
            }
        }
    }
}

winrt::hstring ViewportManagerWithPlatformFeatures::GetLayoutId() const
{
    if (auto layout = m_owner->Layout())
    {
        return layout.as<Layout>()->LayoutId();
    }

    return winrt::hstring{};
}

#ifdef DBG
void ViewportManagerWithPlatformFeatures::TraceFieldsDbg()
{
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
        GetLayoutId().data(), L"m_expectedViewportShift:", m_expectedViewportShift.X, m_expectedViewportShift.Y);
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
        GetLayoutId().data(), L"m_pendingViewportShift:", m_pendingViewportShift.X, m_pendingViewportShift.Y);
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
        GetLayoutId().data(), L"m_unshiftableShift:", m_unshiftableShift.X, m_unshiftableShift.Y);
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this,
        GetLayoutId().data(), L"m_visibleWindow:", m_visibleWindow.X, m_visibleWindow.Y, m_visibleWindow.Width, m_visibleWindow.Height);
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this,
        GetLayoutId().data(), L"m_lastLayoutRealizationWindow:", m_lastLayoutRealizationWindow.X, m_lastLayoutRealizationWindow.Y, m_lastLayoutRealizationWindow.Width, m_lastLayoutRealizationWindow.Height);
    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT, METH_NAME, this,
        GetLayoutId().data(), L"m_layoutExtent:", m_layoutExtent.X, m_layoutExtent.Y, m_layoutExtent.Width, m_layoutExtent.Height);    
}

void ViewportManagerWithPlatformFeatures::TraceScrollerDbg()
{
    if (const auto scrollViewerDbg = m_scroller.try_as<winrt::Controls::IScrollViewer>())
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_DBL_DBL, METH_NAME, this,
            GetLayoutId().data(), L"ScrollViewer Offsets:", scrollViewerDbg.HorizontalOffset(), scrollViewerDbg.VerticalOffset());

        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_DBL_DBL, METH_NAME, this,
            GetLayoutId().data(), L"ScrollViewer Scrollable Sizes:", scrollViewerDbg.ScrollableWidth(), scrollViewerDbg.ScrollableHeight());
    }
    else if (const auto scrollPresenterDbg = m_scroller.try_as<winrt::Controls::Primitives::IScrollPresenter>())
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_DBL_DBL, METH_NAME, this,
            GetLayoutId().data(), L"ScrollPresenter Offsets:", scrollPresenterDbg.HorizontalOffset(), scrollPresenterDbg.VerticalOffset());

        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_STR_DBL_DBL, METH_NAME, this,
            GetLayoutId().data(), L"ScrollPresenter Scrollable Sizes:", scrollPresenterDbg.ScrollableWidth(), scrollPresenterDbg.ScrollableHeight());
    }
}
#endif // DBG
