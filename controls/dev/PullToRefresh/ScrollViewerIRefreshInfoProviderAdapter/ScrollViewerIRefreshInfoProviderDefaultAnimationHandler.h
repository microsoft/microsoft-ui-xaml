// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CppWinRTHelpers.h"


class ScrollViewerIRefreshInfoProviderDefaultAnimationHandler
    : public ReferenceTracker<ScrollViewerIRefreshInfoProviderDefaultAnimationHandler,
    reference_tracker_implements_t<winrt::IAdapterAnimationHandler>::type>
{
public:
    ScrollViewerIRefreshInfoProviderDefaultAnimationHandler(const winrt::UIElement& container, const winrt::RefreshPullDirection& refreshPullDirection);
    ~ScrollViewerIRefreshInfoProviderDefaultAnimationHandler();

    //IAdapterAnimationHandler
    void InteractionTrackerAnimation(const winrt::UIElement& refreshVisualizer, const winrt::UIElement& infoProvider, const winrt::InteractionTracker& interactionTracker);
    void RefreshRequestedAnimation(const winrt::UIElement& refreshVisualizer, const winrt::UIElement& infoProvider, double executionRatio);
    void RefreshCompletedAnimation(const winrt::UIElement& refreshVisualizer, const winrt::UIElement& infoProvider);

private:
    //PrivateHelpers
    void ValidateAndStoreParameters(const winrt::UIElement& refreshVisualizer, const winrt::UIElement& infoProvider, const winrt::InteractionTracker& interactionTracker);
    void RefreshCompletedBatchCompleted(const winrt::IInspectable& sender, const winrt::CompositionBatchCompletedEventArgs& args);
    winrt::hstring getAnimatedPropertyName();

    bool IsOrientationVertical();

    winrt::RefreshPullDirection m_refreshPullDirection;

    tracker_ref<winrt::UIElement> m_refreshVisualizer{ this };
    tracker_ref<winrt::UIElement> m_infoProvider{ this };
    winrt::Visual m_refreshVisualizerVisual{ nullptr };
    winrt::Visual m_infoProviderVisual{ nullptr };
    winrt::InteractionTracker m_interactionTracker{ nullptr };
    winrt::Compositor m_compositor{ nullptr };

    bool m_interactionAnimationNeedsUpdating{ true };
    bool m_refreshRequestedAnimationNeedsUpdating{ true };
    bool m_refreshCompletedAnimationNeedsUpdating{ true };

    winrt::ExpressionAnimation m_refreshVisualizerVisualOffsetAnimation{ nullptr };
    winrt::ExpressionAnimation m_infoProviderOffsetAnimation{ nullptr };

    winrt::ScalarKeyFrameAnimation m_refreshVisualizerRefreshRequestedAnimation{ nullptr };
    winrt::ScalarKeyFrameAnimation m_infoProviderRefreshRequestedAnimation{ nullptr };

    winrt::ScalarKeyFrameAnimation m_refreshVisualizerRefreshCompletedAnimation{ nullptr };
    winrt::ScalarKeyFrameAnimation m_infoProviderRefreshCompletedAnimation{ nullptr };
    winrt::CompositionScopedBatch m_refreshCompletedScopedBatch{ nullptr };

    winrt::event_token m_compositionScopedBatchCompletedEventToken{ 0 };
};
