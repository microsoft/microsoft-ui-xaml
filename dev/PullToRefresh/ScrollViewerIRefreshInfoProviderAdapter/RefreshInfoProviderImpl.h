// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define DEFAULT_EXECUTION_RATIO 0.8


class RefreshInfoProviderImpl
    : public ReferenceTracker<
        RefreshInfoProviderImpl,
        reference_tracker_implements_t<winrt::IRefreshInfoProvider>::type,
        winrt::IInteractionTrackerOwner>
{
public:
    RefreshInfoProviderImpl();
    ~RefreshInfoProviderImpl();
    RefreshInfoProviderImpl(const winrt::RefreshPullDirection& refreshPullDirection, const winrt::Size& refreshVisualizerSize, const winrt::Compositor& compositor);
    void UpdateIsInteractingForRefresh(bool value);

    //IInteractionTrackerOwner;
    void ValuesChanged(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerValuesChangedArgs& args);
    void RequestIgnored(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerRequestIgnoredArgs& args);
    void InteractingStateEntered(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerInteractingStateEnteredArgs& args);
    void InertiaStateEntered(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerInertiaStateEnteredArgs& args);
    void IdleStateEntered(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerIdleStateEnteredArgs& args);
    void CustomAnimationStateEntered(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerCustomAnimationStateEnteredArgs& args);

    //IRefreshInfoProvider
    void OnRefreshStarted();
    void OnRefreshCompleted();
    winrt::event_token InteractionRatioChanged(const winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::RefreshInteractionRatioChangedEventArgs>& handler);
    void InteractionRatioChanged(const winrt::event_token& token);
    winrt::event_token IsInteractingForRefreshChanged(const winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::IInspectable>& handler);
    void IsInteractingForRefreshChanged(const winrt::event_token& token);
    winrt::event_token RefreshStarted(const winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::IInspectable>& handler);
    void RefreshStarted(const winrt::event_token& token);
    winrt::event_token RefreshCompleted(const winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::IInspectable>& handler);
    void RefreshCompleted(const winrt::event_token& token);
    double ExecutionRatio();
    winrt::hstring InteractionRatioCompositionProperty();
    winrt::CompositionPropertySet CompositionProperties();
    bool IsInteractingForRefresh();

    //private helpers
    void RaiseInteractionRatioChanged(double interactionRatio);
    void RaiseIsInteractingForRefreshChanged();
    void RaiseRefreshStarted();
    void RaiseRefreshCompleted();
    void SetPeekingMode(bool peeking);

    bool AreClose(double interactionRatio, double target);

private:
    winrt::RefreshPullDirection m_refreshPullDirection{ winrt::RefreshPullDirection::TopToBottom };
    winrt::Size m_refreshVisualizerSize{ 1.0F, 1.0F };
    bool m_isInteractingForRefresh{ false };
    int m_interactionRatioChangedCount{ 0 };
    winrt::CompositionPropertySet m_compositionProperties{ nullptr };
    PCWSTR m_interactionRatioCompositionProperty = L"InteractionRatio";
    double m_executionRatio{ DEFAULT_EXECUTION_RATIO };
    bool m_peeking{ false };

    event_source<winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::IInspectable>> m_IsInteractingForRefreshChangedEventSource{ this };
    event_source<winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::RefreshInteractionRatioChangedEventArgs>> m_InteractionRatioChangedEventSource{ this };
    event_source<winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::IInspectable>> m_RefreshStartedEventSource{ this };
    event_source<winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::IInspectable>> m_RefreshCompletedEventSource{ this };
};