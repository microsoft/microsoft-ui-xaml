// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ViewportManager.h"

class ItemsRepeater;

// Manages virtualization windows (visible/realization). 
// This class does the equivalent behavior as ViewportManagerWithPlatformFeatures class
// except that here we do not use EffectiveViewport and ScrollAnchoring features added to the framework in RS5. 
// Instead we use the IRepeaterScrollingSurface internal API. This class is used when building in MUX and 
// should work down-level.
class ViewportManagerDownLevel : public ViewportManager
{
public:
    ViewportManagerDownLevel(ItemsRepeater* owner);

    winrt::UIElement SuggestedAnchor() const override;

    double HorizontalCacheLength() const override { return m_maximumHorizontalCacheLength; }
    void HorizontalCacheLength(double value) override;

    double VerticalCacheLength() const override { return m_maximumVerticalCacheLength; }
    void VerticalCacheLength(double value) override;

    winrt::Rect GetLayoutVisibleWindow() const override;
    winrt::Rect GetLayoutRealizationWindow() const override;

    void SetLayoutExtent(winrt::Rect extent) override;
    winrt::Rect GetLayoutExtent() const override { return m_layoutExtent; }
    winrt::Point GetOrigin() const override{ return winrt::Point(m_layoutExtent.X, m_layoutExtent.Y); }

    void OnLayoutChanged(bool isVirtualizing) override;
    void OnElementPrepared(const winrt::UIElement& element) override {}
    void OnElementCleared(const winrt::UIElement& element) override;
    void OnOwnerMeasuring() override {};
    void OnOwnerArranged() override;
    void OnMakeAnchor(const winrt::UIElement& anchor, const bool isAnchorOutsideRealizedRange) override;
    void OnBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs args) override;

    void ResetScrollers() override;

    winrt::UIElement MadeAnchor() const override { return m_makeAnchorElement.get(); }

private:
    struct ScrollerInfo;

    void OnCacheBuildActionCompleted();
    void OnViewportChanged(const winrt::IRepeaterScrollingSurface& sender, const bool isFinal);
    void OnPostArrange(const winrt::IRepeaterScrollingSurface& sender);
    void OnConfigurationChanged(const winrt::IRepeaterScrollingSurface& sender);

    void EnsureScrollers();
    bool HasScrollers() const { return !!m_horizontalScroller || !!m_verticalScroller; }
    bool AddScroller(const winrt::IRepeaterScrollingSurface& scroller);
    void UpdateViewport();
    void ResetCacheBuffer();
    void ValidateCacheLength(double cacheLength);
    void RegisterCacheBuildWork();
    void TryInvalidateMeasure();
    winrt::IRepeaterScrollingSurface GetOuterScroller() const;

    winrt::hstring GetLayoutId();

    ItemsRepeater* m_owner{ nullptr };

    // List of parent scrollers.
    // The list stops when we reach the root scroller OR when both m_horizontalScroller
    // and m_verticalScroller are set. In the latter case, we don't care about the other
    // scroller that we haven't reached yet.
    bool m_ensuredScrollers{ false };
    std::vector<ScrollerInfo> m_parentScrollers;

    // In order to support the Store scenario (vertical list of horizontal lists),
    // we need to build a synthetic virtualization window by taking the horizontal and
    // vertical components of the viewport from two different scrollers.
    tracker_ref<winrt::IRepeaterScrollingSurface> m_horizontalScroller;
    tracker_ref<winrt::IRepeaterScrollingSurface> m_verticalScroller;
    // Invariant: !m_innerScrollableScroller || m_horizontalScroller == m_innerScrollableScroller || m_verticalScroller == m_innerScrollableScroller.
    tracker_ref<winrt::IRepeaterScrollingSurface> m_innerScrollableScroller;

    tracker_ref<winrt::UIElement> m_makeAnchorElement;
    bool m_isAnchorOutsideRealizedRange{};  // Value is only valid when m_makeAnchorElement is set.

    tracker_ref<winrt::IAsyncAction> m_cacheBuildAction;

    winrt::Rect m_visibleWindow{};
    winrt::Rect m_layoutExtent{};
    winrt::Point m_expectedViewportShift{};

    // Realization window cache fields
    double m_maximumHorizontalCacheLength{ 2.0 };
    double m_maximumVerticalCacheLength{ 2.0 };
    double m_horizontalCacheBufferPerSide{};
    double m_verticalCacheBufferPerSide{};

    // For non-virtualizing layouts, we do not need to keep
    // updating viewports and invalidating measure often. So when
    // a non virtualizing layout is used, we stop doing all that work.
    bool m_managingViewportDisabled{ false };

    // Event tokens
    winrt::IRepeaterScrollingSurface::PostArrange_revoker m_postArrangeToken;

    // Stores information about a parent scrolling surface.
    // We subscribe to...
    // - ViewportChanged only on scrollers that are scrollable in at least one direction.
    // - ConfigurationChanged on all scrollers.
    // - PostArrange only on the outer most scroller, because we need to wait for that one
    //   to arrange its children before we can reliably figure out our relative viewport.
    struct ScrollerInfo
    {
        ScrollerInfo(
            const ITrackerHandleManager* owner,
            winrt::IRepeaterScrollingSurface scroller) :
            m_scroller(owner, scroller)
        { }

        winrt::IRepeaterScrollingSurface Scroller() const
        {
            return m_scroller.get();
        }

        winrt::IRepeaterScrollingSurface::ViewportChanged_revoker ViewportChangedToken{};
        winrt::IRepeaterScrollingSurface::PostArrange_revoker PostArrangeToken{};
        winrt::IRepeaterScrollingSurface::ConfigurationChanged_revoker ConfigurationChangedToken{};

    private:
        tracker_ref<winrt::IRepeaterScrollingSurface> m_scroller;       
    };
};
