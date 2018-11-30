// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ItemsRepeater;

// Manages the virtualization windows (visible/realization). This class essentially is 
// does the equivalent behavior as ViewportManager class except that here we use 
// EffectiveViewport and ScrollAnchoring features added to the framework in RS5. 
// We also do not use the IRepeaterScrollingSurface internal API used by ViewManager.
// Not that this class is used when built in the OS build (under wuxc) and ViewManager
// is used when building in MUX to keep down level support.
class ViewportManagerWithPlatformFeatures final
{
public:
    ViewportManagerWithPlatformFeatures(ItemsRepeater* owner);

    winrt::UIElement SuggestedAnchor() const;

    double HorizontalCacheLength() const { return m_maximumHorizontalCacheLength; }
    void HorizontalCacheLength(double value);

    double VerticalCacheLength() const { return m_maximumVerticalCacheLength; }
    void VerticalCacheLength(double value);

    winrt::Rect GetLayoutVisibleWindow() const;
    winrt::Rect GetLayoutRealizationWindow() const;

    void SetLayoutExtent(winrt::Rect extent);
    winrt::Point GetOrigin() const { return winrt::Point(m_layoutExtent.X, m_layoutExtent.Y); }

    void OnLayoutChanged();
    void OnElementPrepared(const winrt::UIElement& element);
    void OnElementCleared(const winrt::UIElement& element);
    void OnOwnerMeasuring();
    void OnOwnerArranged();
    void OnMakeAnchor(const winrt::UIElement& anchor, const bool isAnchorOutsideRealizedRange);
    void OnBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs args);

    void ResetScrollers();
    void EnsureScroller();

    winrt::UIElement MadeAnchor() const { return m_makeAnchorElement.get(); }

private:
    struct ScrollerInfo;

    void OnCacheBuildActionCompleted();
    void OnEffectiveViewportChanged(winrt::FrameworkElement const& sender, winrt::EffectiveViewportChangedEventArgs const& args);
    void OnLayoutUpdated(winrt::IInspectable const& sender, winrt::IInspectable const& args);
    
    bool HasScroller() const { return m_scroller != nullptr; }
    void UpdateViewport(winrt::Rect const& args);
    void ResetCacheBuffer();
    void ValidateCacheLength(double cacheLength);
    void RegisterCacheBuildWork();
    void TryInvalidateMeasure();
    
    winrt::hstring GetLayoutId() const;

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

    // Realization window cache fields
    double m_maximumHorizontalCacheLength{ 2.0 };
    double m_maximumVerticalCacheLength{ 2.0 };
    double m_horizontalCacheBufferPerSide{};
    double m_verticalCacheBufferPerSide{};

    // Event tokens
    winrt::FrameworkElement::EffectiveViewportChanged_revoker m_effectiveViewportChangedRevoker{};

    winrt::FrameworkElement::LayoutUpdated_revoker m_layoutUpdatedRevoker{};
};