// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RefreshVisualizer.g.h"
#include "RefreshVisualizer.properties.h"

class RefreshVisualizer :
    public ReferenceTracker<RefreshVisualizer, winrt::implementation::RefreshVisualizerT, winrt::IRefreshVisualizerPrivate>,
    public RefreshVisualizerProperties
{
public:
    RefreshVisualizer();
    ~RefreshVisualizer();

    // Property changed handler.
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // IFrameworkElementOverrides
    void OnApplyTemplate();

    // IRefreshVisualizer overrides
    void RequestRefresh();

    //Private Interface members
    winrt::IRefreshInfoProvider InfoProvider();
    void InfoProvider(winrt::IRefreshInfoProvider const& value);

    void SetInternalPullDirection(winrt::RefreshPullDirection const& value);

private:
    //////////////////////////////////////////////////////
    ////  OnDependencyPropertyChanged    handlers   //////
    //////////////////////////////////////////////////////
    void OnRefreshInfoProviderChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnOrientationChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnOrientationChangedImpl();

    void OnStateChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnContentChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnContentChangedImpl();

    ///////////////////////////////////////////////
    /////         Internal Methods            /////
    ///////////////////////////////////////////////
    void UpdateContent();
    void ExecuteInteractingAnimations();
    void ExecuteScaleUpAnimation();
    void ExecuteExecutingRotationAnimation();
    void UpdateRefreshState(const winrt::RefreshVisualizerState& newState);
    void RaiseRefreshStateChanged(const winrt::RefreshVisualizerState& oldState, const winrt::RefreshVisualizerState& newState);
    void RaiseRefreshRequested();    
    void RefreshCompleted();

    ///////////////////////////////////////////////
    ///// IRefreshInfoProvider Event Handlers /////
    ///////////////////////////////////////////////
    void RefreshInfoProvider_InteractingForRefreshChanged(const winrt::IInspectable& sender, const winrt::IInspectable& e);
    void RefreshInfoProvider_InteractionRatioChanged(const winrt::IRefreshInfoProvider& sender, const winrt::RefreshInteractionRatioChangedEventArgs& e);

    ///////////////////////////////////////////////
    ///// Private Dependency Property Setters /////
    ///////////////////////////////////////////////
    void put_State (const winrt::RefreshVisualizerState& value);

    //Helpers
    bool IsPullDirectionVertical();
    bool IsPullDirectionFar();

    //////////////////////////////////////////////////////
    ////             DependencyPropertyBackers      //////
    //////////////////////////////////////////////////////
    winrt::RefreshVisualizerOrientation m_orientation{ winrt::RefreshVisualizerOrientation::Auto };
    winrt::RefreshVisualizerState m_state{ winrt::RefreshVisualizerState::Idle };
    tracker_ref<winrt::IRefreshInfoProvider> m_refreshInfoProvider{ this };
    tracker_ref<winrt::UIElement> m_content{ this };

    ///////////////////////////////////////////////
    /////////   Event Tokens and Sources   ////////
    ///////////////////////////////////////////////
    winrt::event_token m_RefreshInfoProvider_InteractingForRefreshChangedToken{};
    winrt::event_token m_RefreshInfoProvider_InteractionRatioChangedToken{};

    ///////////////////////////////////////////////
    /////////   Internal Reference Vars   /////////
    ///////////////////////////////////////////////
    bool m_isInteractingForRefresh{ false };
    double m_executionRatio{ 0.8f };
    double m_interactionRatio{ 0.0f };
    tracker_ref<winrt::Compositor> m_compositor{ this };
    tracker_ref<winrt::Panel> m_containerPanel{ this };
    tracker_ref<winrt::Panel> m_root{ this };
    float m_startingRotationAngle{ 0.0f };
    winrt::RefreshPullDirection m_pullDirection{ winrt::RefreshPullDirection::TopToBottom };
};
