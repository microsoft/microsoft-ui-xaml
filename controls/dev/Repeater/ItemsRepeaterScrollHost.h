// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsRepeaterScrollHost.g.h"

// TODO: move to framework level element tracking.
class ItemsRepeaterScrollHost :
    public ReferenceTracker<ItemsRepeaterScrollHost, DeriveFromPanelHelper_base, winrt::ItemsRepeaterScrollHost, winrt::cloaked<winrt::IRepeaterScrollingSurface>>
{
public:
    ItemsRepeaterScrollHost();

#pragma region IFrameworkElementOverrides

    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

#pragma endregion

#pragma region IScrollAnchorProvider

    double HorizontalAnchorRatio();

    void HorizontalAnchorRatio(double value);

    double VerticalAnchorRatio();

    void VerticalAnchorRatio(double value);

    winrt::UIElement CurrentAnchor();

    winrt::FxScrollViewer ScrollViewer();

    void ScrollViewer(winrt::FxScrollViewer const& value);

#pragma endregion

#pragma region IRepeaterScrollingSurface

    bool IsHorizontallyScrollable();

    bool IsVerticallyScrollable();

    winrt::UIElement AnchorElement();

    winrt::event_token ViewportChanged(winrt::ViewportChangedEventHandler const& value);

    void ViewportChanged(winrt::event_token const& token);

    winrt::event_token PostArrange(winrt::PostArrangeEventHandler const& value);

    void PostArrange(winrt::event_token const& token);

    winrt::event_token ConfigurationChanged(winrt::ConfigurationChangedEventHandler const& value);

    void ConfigurationChanged(winrt::event_token const& token);

    void RegisterAnchorCandidate(winrt::UIElement const& element);

    void UnregisterAnchorCandidate(winrt::UIElement const& element);

    winrt::Rect GetRelativeViewport(
        winrt::UIElement const& element);

#pragma endregion

    void StartBringIntoView(
        winrt::UIElement const& element,
        double alignmentX,
        double alignmentY,
        double offsetX,
        double offsetY,
        bool animate);

private:
    void ApplyPendingChangeView(const winrt::FxScrollViewer& scrollViewer);
    double TrackElement(const winrt::UIElement& element, winrt::Rect previousBounds, const winrt::FxScrollViewer& scrollViewer);
    winrt::UIElement GetAnchorElement(_Out_opt_ winrt::Rect* relativeBounds = nullptr);

    void OnScrollViewerViewChanging(const winrt::IInspectable& sender, const winrt::ScrollViewerViewChangingEventArgs& args);
    void OnScrollViewerViewChanged(winrt::IInspectable const& sender, winrt::ScrollViewerViewChangedEventArgs const& args);
    void OnScrollViewerSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);

    bool HasPendingBringIntoView() const { return !!m_pendingBringIntoView.TargetElement(); }

    struct CandidateInfo
    {
        CandidateInfo(
            const ITrackerHandleManager* owner,
            winrt::UIElement element) :
            m_element(owner, element)
        { }

        winrt::UIElement Element() const { return m_element.get(); }
        winrt::Rect RelativeBounds() const { return m_relativeBounds; }
        void RelativeBounds(winrt::Rect bounds) { m_relativeBounds = bounds; }
        bool IsRelativeBoundsSet() const { return m_relativeBounds != InvalidBounds; }

        static constexpr winrt::Rect InvalidBounds{ -1.0f, -1.0f, -1.0f, -1.0f };
    private:
        tracker_ref<winrt::UIElement> m_element;
        winrt::Rect m_relativeBounds{ InvalidBounds };
    };

    struct BringIntoViewState
    {
        BringIntoViewState(const ITrackerHandleManager* owner)
            : m_targetElement(owner)
        { }
        BringIntoViewState(
            const ITrackerHandleManager* owner,
            winrt::UIElement targetElement,
            double alignmentX,
            double alignmentY,
            double offsetX,
            double offsetY,
            bool animate) :
            m_targetElement(owner, targetElement),
            m_alignmentX(alignmentX),
            m_alignmentY(alignmentY),
            m_offsetX(offsetX),
            m_offsetY(offsetY),
            m_animate(animate)
        { }

        winrt::UIElement TargetElement() const { return m_targetElement.get(); }
        double AlignmentX() const { return m_alignmentX; }
        double AlignmentY() const { return m_alignmentY; }
        double OffsetX() const { return m_offsetX; }
        double OffsetY() const { return m_offsetY; }
        bool Animate() const { return m_animate; }
        bool ChangeViewCalled() const { return m_changeViewCalled; }
        void ChangeViewCalled(bool value) { m_changeViewCalled = value; }
        winrt::Point ChangeViewOffset() const { return m_changeViewOffset; }
        void ChangeViewOffset(winrt::Point value) { m_changeViewOffset = value; }

        void Reset()
        {
            m_targetElement.set(nullptr);
            m_alignmentX = m_alignmentY = m_offsetX = m_offsetY = 0.0;
            m_animate = m_changeViewCalled = false;
            m_changeViewOffset = {};
        }

    private:
        tracker_ref<winrt::UIElement> m_targetElement;
        double m_alignmentX{};
        double m_alignmentY{};
        double m_offsetX{};
        double m_offsetY{};
        bool m_animate{};
        bool m_changeViewCalled{};
        winrt::Point m_changeViewOffset{};
    };

    std::vector<CandidateInfo> m_candidates;

    tracker_ref<winrt::UIElement> m_anchorElement{ this };
    winrt::Rect m_anchorElementRelativeBounds{};
    // Whenever the m_candidates list changes, we set this to true.
    bool m_isAnchorElementDirty{ true };

    double m_horizontalEdge{};
    double m_verticalEdge{};    // Not used in this temporary implementation.

    // We can only bring an element into view after it got arranged and
    // we know its bounds as well as the viewport (so that we can account
    // for alignment and offset).
    // The BringIntoView call can however be made at any point, even
    // in the constructor of a page (deserialization scenario) so we
    // need to hold on the parameter that are passed in BringIntoViewOperation.
    BringIntoViewState m_pendingBringIntoView{ this };

    // A ScrollViewer.ChangeView operation, even if not animated, is not synchronous.
    // In other words, right after the call, ScrollViewer.[Vertical|Horizontal]Offset and
    // TransformToVisual are not going to reflect the new viewport. We need to keep
    // track of the pending viewport shift until the ChangeView operation completes
    // asynchronously.
    double m_pendingViewportShift{};

    event_source<winrt::ViewportChangedEventHandler> m_viewportChanged{ this };
    event_source<winrt::PostArrangeEventHandler> m_postArrange{ this };

    winrt::FxScrollViewer::ViewChanging_revoker m_scrollViewerViewChanging{};
    winrt::FxScrollViewer::ViewChanged_revoker m_scrollViewerViewChanged{};
    winrt::FxScrollViewer::SizeChanged_revoker m_scrollViewerSizeChanged{};

};
