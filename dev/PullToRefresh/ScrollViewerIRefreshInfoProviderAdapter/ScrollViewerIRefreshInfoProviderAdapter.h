// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RefreshInfoProviderImpl.h"
#include "ScrollViewerIRefreshInfoProviderDefaultAnimationHandler.h"

#include "ScrollViewerIRefreshInfoProviderAdapter.g.h"

class ScrollViewerIRefreshInfoProviderAdapter :
    public ReferenceTracker<ScrollViewerIRefreshInfoProviderAdapter, winrt::implementation::ScrollViewerIRefreshInfoProviderAdapterT, winrt::composable, winrt::composing>
{
public:
    ScrollViewerIRefreshInfoProviderAdapter(winrt::RefreshPullDirection const& refreshPullDirection, winrt::IAdapterAnimationHandler const& animationHandler);
    ~ScrollViewerIRefreshInfoProviderAdapter();

    winrt::IRefreshInfoProvider AdaptFromTree(winrt::UIElement const& root, winrt::Size const& size);
    winrt::IRefreshInfoProvider Adapt(winrt::ScrollViewer const& adaptee, winrt::Size const& size);
    void SetAnimations(winrt::UIElement const& refreshVisualizerContainer);

private:
    void OnRefreshStarted(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnRefreshCompleted(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    void OnScrollViewerLoaded(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnScrollViewerDirectManipulationCompleted(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnScrollViewerViewChanging(const winrt::IInspectable& sender, const winrt::ScrollViewerViewChangingEventArgs args);

    bool IsWithinOffsetThreshold();
    void CleanupScrollViewer();
    void CleanupIRefreshInfoProvider();

    bool IsOrientationVertical();
    winrt::UIElement GetScrollContent();
    winrt::ScrollViewer AdaptFromTreeRecursiveHelper(winrt::DependencyObject, int depth);
    void MakeInteractionSource(winrt::UIElement contentParent);

    tracker_com_ref<RefreshInfoProviderImpl> m_infoProvider{ this };
    tracker_ref<winrt::IAdapterAnimationHandler> m_animationHandler{ this };
    tracker_ref<winrt::ScrollViewer> m_scrollViewer{ this };
    winrt::RefreshPullDirection m_refreshPullDirection{ winrt::RefreshPullDirection::TopToBottom };
    tracker_ref<winrt::InteractionTracker> m_interactionTracker{ this };
    tracker_ref<winrt::VisualInteractionSource> m_visualInteractionSource{ this };
    bool m_visualInteractionSourceIsAttached{ false };

    winrt::event_token m_scrollViewer_LoadedToken{};
    winrt::event_token m_scrollViewer_PointerPressedToken{};
    winrt::event_token m_scrollViewer_DirectManipulationCompletedToken{};
    winrt::event_token m_scrollViewer_ViewChangingToken{};
    winrt::event_token m_infoProvider_RefreshStartedToken{};
    winrt::event_token m_infoProvider_RefreshCompletedToken{};

    tracker_ref<winrt::IInspectable> m_boxedPointerPressedEventHandler{ this };
};

