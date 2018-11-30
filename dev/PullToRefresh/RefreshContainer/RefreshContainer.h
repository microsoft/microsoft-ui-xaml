// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RefreshContainer.g.h"
#include "RefreshContainer.properties.h"

class RefreshContainer :
    public ReferenceTracker<RefreshContainer, winrt::implementation::RefreshContainerT, winrt::IRefreshContainerPrivate>,
    public RefreshContainerProperties
{
public:
    RefreshContainer();
    ~RefreshContainer();

    // Property changed handler.
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // IFrameworkElementOverrides
    void OnApplyTemplate();

    void RequestRefresh();

    winrt::IRefreshInfoProviderAdapter RefreshInfoProviderAdapter();
    void RefreshInfoProviderAdapter(winrt::IRefreshInfoProviderAdapter const& value);

private:
    // Property changed event handler.
    static void OnPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args);

    winrt::IRefreshInfoProvider SearchTreeForIRefreshInfoProvider(); 
    winrt::IRefreshInfoProvider SearchTreeForIRefreshInfoProviderRecursiveHelper(winrt::DependencyObject root, int depth);

    void OnRefreshVisualizerChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnPullDirectionChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnRefreshVisualizerChangedImpl();
    void OnPullDirectionChangedImpl();

    void OnRefreshInfoProviderAdapterChanged();
    void OnRefreshInfoProviderAdapterChangedImpl();

    void OnVisualizerSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnVisualizerRefreshRequested(const winrt::IInspectable& sender, const winrt::RefreshRequestedEventArgs& args);

    void RaiseRefreshRequested();
    void RefreshCompleted();

    tracker_ref<winrt::Panel> m_root{ this };
    tracker_ref<winrt::Panel> m_refreshVisualizerPresenter{ this };
    tracker_ref<winrt::RefreshVisualizer> m_refreshVisualizer{ this };
    tracker_ref<winrt::Deferral> m_visualizerRefreshCompletedDeferral{ this };
    winrt::RefreshPullDirection m_refreshPullDirection{ winrt::RefreshPullDirection::TopToBottom };
    tracker_ref<winrt::IRefreshInfoProviderAdapter> m_refreshInfoProviderAdapter{ this };
    winrt::event_token m_refreshVisualizerSizeChangedToken{ 0 };

    bool m_hasDefaultRefreshVisualizer{ false };
    bool m_hasDefaultRefreshInfoProviderAdapter{ false };
};