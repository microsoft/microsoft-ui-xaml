// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "AnimatedIcon.g.h"
#include "AnimatedIcon.properties.h"

class AnimatedIcon :
    public ReferenceTracker<AnimatedIcon, DeriveFromPathIconHelper_base, winrt::AnimatedIcon>,
    public AnimatedIconProperties
{
    friend class AnimatedIconProperties;

public:
    AnimatedIcon();
    ~AnimatedIcon();

    // void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    winrt::Composition::CompositionObject ProgressObject();
    void Pause();
    winrt::IAsyncAction PlayAsync(double fromProgress, double toProgress, bool looped);
    void Resume();
    void SetProgress(double progress);
    void Stop();


    // FrameworkElement overrides
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

    // IUIElement / IUIElementOverridesHelper
    // winrt::AutomationPeer OnCreateAutomationPeer();

private:
    //
    // An awaitable object that is completed when an animation play is completed.
    //
    struct AnimationPlay final : public Awaitable
    {
        AnimationPlay(
            AnimatedIcon& owner,
            float fromProgress,
            float toProgress,
            bool looped);

        float FromProgress();

        bool IsCurrentPlay();

        // Sets the playback rate of the animation.
        void SetPlaybackRate(float value);

        void Start();

        void Pause();

        void Resume();

        void OnHiding();
        void OnUnhiding();

        // Called to indicate that the play has been completed. Unblocks awaiters.
        void Complete();

    private:
        AnimatedIcon& m_owner;
        const float m_fromProgress{};
        const float m_toProgress{};
        const bool m_looped{};
        winrt::TimeSpan m_playDuration{};

        winrt::Composition::AnimationController m_controller{ nullptr };
        bool m_isPaused{ false };
        bool m_isPausedBecauseHidden{ false };
        winrt::event_token m_batchCompletedToken{ 0 };
        winrt::Composition::CompositionScopedBatch m_batch{ nullptr };
    };

    void OnAutoPlayPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    void OnFallbackContentPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    void OnPlaybackRatePropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    void OnSourcePropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    void OnStretchPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    void UpdateContent();
    void UnloadContent();

    void LoadFallbackContent();
    void UnloadFallbackContent();

    void SetFallbackContent(winrt::UIElement const& uiElement);

    void OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnHiding();
    void OnUnhiding();

    //
    // Initialized by the constructor.
    //
    // A Visual used for clipping and for parenting of m_animatedVisualRoot.
    winrt::Composition::SpriteVisual m_rootVisual{ nullptr };
    // The property set that contains the Progress property that will be used to
    // set the progress of the animated visual.
    winrt::Composition::CompositionPropertySet m_progressPropertySet{ nullptr };
    // Revokers for events that we are subscribed to.
    winrt::Application::Suspending_revoker m_suspendingRevoker{};
    winrt::Application::Resuming_revoker m_resumingRevoker{};
    winrt::CoreWindow::VisibilityChanged_revoker m_visibilityChangedRevoker{};
    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};
    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};

    //
    // Player mutable state state.
    //
    tracker_ref<winrt::IAnimatedVisual> m_animatedVisual{ this };
    // The native size of the current animated visual. Only valid if m_animatedVisual is not nullptr.
    winrt::float2 m_animatedVisualSize;
    winrt::Composition::Visual m_animatedVisualRoot{ nullptr };
    int m_playAsyncVersion{ 0 };
    double m_currentPlayFromProgress{ 0 };
    // The play that will be stopped when Stop() is called.
    std::shared_ptr<AnimationPlay> m_nowPlaying{ nullptr };
    winrt::IDynamicAnimatedVisualSource::AnimatedVisualInvalidated_revoker  m_dynamicAnimatedVisualInvalidatedRevoker{};

    // Set true if an animated visual has failed to load and set false the next time an animated
    // visual loads with non-null content. When this is true the fallback content (if any) will
    // be displayed.
    bool m_isFallenBack{ false };

    // Set true when FrameworkElement::Unloaded is fired, then set false when FrameworkElement::Loaded is fired.
    // This is used to differentiate the first Loaded event (when the element has never been
    // unloaded) from later Loaded events.
    bool m_isUnloaded{ false };
};
