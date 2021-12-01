// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewportManager.h"

class ItemsRepeater;

// Manages the virtualization windows (visible/realization). This class essentially is 
// does the equivalent behavior as ViewportManager class except that here we use 
// EffectiveViewport and ScrollAnchoring features added to the framework in RS5. 
// We also do not use the IRepeaterScrollingSurface internal API used by ViewManager.
// Not that this class is used when built in the OS build (under wuxc) and ViewManager
// is used when building in MUX to keep down level support.
class ViewportManagerWithPlatformFeatures : public ViewportManager
{
public:
    ViewportManagerWithPlatformFeatures(ItemsRepeater* owner);

    winrt::UIElement SuggestedAnchor() const override;

    double HorizontalCacheLength() const override { return m_maximumHorizontalCacheLength; }
    void HorizontalCacheLength(double value) override;

    double VerticalCacheLength() const override { return m_maximumVerticalCacheLength; }
    void VerticalCacheLength(double value) override;

    winrt::Rect GetLayoutVisibleWindow() const override;
    winrt::Rect GetLayoutRealizationWindow() const override;

    void SetLayoutExtent(winrt::Rect extent) override;
    winrt::Rect GetLayoutExtent() const override { return m_layoutExtent; }
    winrt::Point GetOrigin() const override { return winrt::Point(m_layoutExtent.X, m_layoutExtent.Y); }

    void OnLayoutChanged(bool isVirtualizing) override;
    void OnElementPrepared(const winrt::UIElement& element) override;
    void OnElementCleared(const winrt::UIElement& element) override;
    void OnOwnerMeasuring() override;
    void OnOwnerArranged() override;
    void OnMakeAnchor(const winrt::UIElement& anchor, const bool isAnchorOutsideRealizedRange) override;
    void OnBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs args) override;

    void ResetScrollers() override;

    winrt::UIElement MadeAnchor() const override { return m_makeAnchorElement.get(); }

private:
    struct ScrollerInfo;

    void OnCacheBuildActionCompleted();
    void OnEffectiveViewportChanged(winrt::FrameworkElement const& sender, winrt::EffectiveViewportChangedEventArgs const& args);
    void OnLayoutUpdated(winrt::IInspectable const& sender, winrt::IInspectable const& args);

    void EnsureScroller();
    bool HasScroller() const { return m_scroller != nullptr; }
    void UpdateViewport(winrt::Rect const& args);
    void ResetCacheBuffer();
    void ValidateCacheLength(double cacheLength);
    void RegisterCacheBuildWork();
    void TryInvalidateMeasure();
    winrt::Rect GetLayoutVisibleWindowDiscardAnchor() const;

    winrt::hstring GetLayoutId() const;
    void OnCompositionTargetRendering(winrt::IInspectable const& sender, winrt::IInspectable const& args);
    winrt::UIElement GetImmediateChildOfRepeater(winrt::UIElement const& descendant);

    ItemsRepeater* m_owner{ nullptr };

    bool m_ensuredScroller{ false };
    tracker_ref<winrt::Controls::IScrollAnchorProvider> m_scroller;

    tracker_ref<winrt::UIElement> m_makeAnchorElement;
    bool m_isAnchorOutsideRealizedRange{};  // Value is only valid when m_makeAnchorElement is set.

    tracker_ref<winrt::IAsyncAction> m_cacheBuildAction;

    winrt::Rect m_visibleWindow{};
    winrt::Rect m_layoutExtent{};
    // This is the expected shift by the layout.
    winrt::Point m_expectedViewportShift{};
    // This is what is pending and not been accounted for. 
    // Sometimes the scrolling surface cannot service a shift (for example
    // it is already at the top and cannot shift anymore.)
    winrt::Point m_pendingViewportShift{};
    // Unshiftable shift amount that this view manager can
    // handle on its own to fake it to the layout as if the shift
    // actually happened. This can happen in cases where no scrollviewer
    // in the parent chain can scroll in the shift direction.
    winrt::Point m_unshiftableShift{};


    // Realization window cache fields
    double m_maximumHorizontalCacheLength{ 2.0 };
    double m_maximumVerticalCacheLength{ 2.0 };
    double m_horizontalCacheBufferPerSide{};
    double m_verticalCacheBufferPerSide{};

    bool m_isBringIntoViewInProgress{false};
    // For non-virtualizing layouts, we do not need to keep
    // updating viewports and invalidating measure often. So when
    // a non virtualizing layout is used, we stop doing all that work.
    bool m_managingViewportDisabled{ false };

    // Event tokens
    winrt::FrameworkElement::EffectiveViewportChanged_revoker m_effectiveViewportChangedRevoker{};

    winrt::FrameworkElement::LayoutUpdated_revoker m_layoutUpdatedRevoker{};
    winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering_revoker m_renderingToken{};
};
