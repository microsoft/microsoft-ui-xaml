// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingPresenter.h"
#include "ScrollingPresenterTestHooksAnchorEvaluatedEventArgs.h"
#include "ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs.h"
#include "ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs.h"
#include "ScrollingPresenterTestHooks.g.h"
#include "RegUtil.h"

class ScrollingPresenterTestHooks :
    public winrt::implementation::ScrollingPresenterTestHooksT<ScrollingPresenterTestHooks>
{
public:
    ScrollingPresenterTestHooks();

    static com_ptr<ScrollingPresenterTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<ScrollingPresenterTestHooks> EnsureGlobalTestHooks();

    static bool AreAnchorNotificationsRaised();
    static void AreAnchorNotificationsRaised(bool areAnchorNotificationsRaised);
    static bool AreInteractionSourcesNotificationsRaised();
    static void AreInteractionSourcesNotificationsRaised(bool areInteractionSourcesNotificationsRaised);
    static bool AreExpressionAnimationStatusNotificationsRaised();
    static void AreExpressionAnimationStatusNotificationsRaised(bool areExpressionAnimationStatusNotificationsRaised);
    static bool IsInteractionTrackerPointerWheelRedirectionEnabled();
    static void IsInteractionTrackerPointerWheelRedirectionEnabled(bool isInteractionTrackerPointerWheelRedirectionEnabled);
    static winrt::IReference<bool> IsAnimationsEnabledOverride();
    static void IsAnimationsEnabledOverride(winrt::IReference<bool> isAnimationsEnabledOverride);
    static int MouseWheelDeltaForVelocityUnit();
    static void MouseWheelDeltaForVelocityUnit(int mouseWheelDeltaForVelocityUnit);
    static int MouseWheelScrollLines();
    static void MouseWheelScrollLines(int mouseWheelScrollLines);
    static int MouseWheelScrollChars();
    static void MouseWheelScrollChars(int mouseWheelScrollChars);
    static float MouseWheelInertiaDecayRate();
    static void MouseWheelInertiaDecayRate(float mouseWheelInertiaDecayRate);
    static void GetOffsetsChangeVelocityParameters(int& millisecondsPerUnit, int& minMilliseconds, int& maxMilliseconds);
    static void SetOffsetsChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds);
    static void GetZoomFactorChangeVelocityParameters(int& millisecondsPerUnit, int& minMilliseconds, int& maxMilliseconds);
    static void SetZoomFactorChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds);
    static void GetContentLayoutOffsetX(const winrt::ScrollingPresenter& scrollingPresenter, float& contentLayoutOffsetX);
    static void SetContentLayoutOffsetX(const winrt::ScrollingPresenter& scrollingPresenter, float contentLayoutOffsetX);
    static void GetContentLayoutOffsetY(const winrt::ScrollingPresenter& scrollingPresenter, float& contentLayoutOffsetY);
    static void SetContentLayoutOffsetY(const winrt::ScrollingPresenter& scrollingPresenter, float contentLayoutOffsetY);
    static winrt::float2 GetArrangeRenderSizesDelta(const winrt::ScrollingPresenter& scrollingPresenter);
    static winrt::InteractionTracker GetInteractionTracker(const winrt::ScrollingPresenter& scrollingPresenter);
    static winrt::float2 GetMinPosition(const winrt::ScrollingPresenter& scrollingPresenter);
    static winrt::float2 GetMaxPosition(const winrt::ScrollingPresenter& scrollingPresenter);
    static winrt::ScrollingPresenterViewChangeResult GetScrollCompletedResult(const winrt::ScrollingScrollCompletedEventArgs& scrollCompletedEventArgs);
    static winrt::ScrollingPresenterViewChangeResult GetZoomCompletedResult(const winrt::ScrollingZoomCompletedEventArgs& zoomCompletedEventArgs);

    static void NotifyAnchorEvaluated(const winrt::ScrollingPresenter& sender, const winrt::UIElement& anchorElement, double viewportAnchorPointHorizontalOffset, double viewportAnchorPointVerticalOffset);
    static winrt::event_token AnchorEvaluated(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::ScrollingPresenterTestHooksAnchorEvaluatedEventArgs> const& value);
    static void AnchorEvaluated(winrt::event_token const& token);

    static void NotifyInteractionSourcesChanged(const winrt::ScrollingPresenter& sender, const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources);
    static winrt::event_token InteractionSourcesChanged(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs> const& value);
    static void InteractionSourcesChanged(winrt::event_token const& token);

    static void NotifyExpressionAnimationStatusChanged(const winrt::ScrollingPresenter& sender, bool isExpressionAnimationStarted, wstring_view const& propertyName);
    static winrt::event_token ExpressionAnimationStatusChanged(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs> const& value);
    static void ExpressionAnimationStatusChanged(winrt::event_token const& token);

    static void NotifyContentLayoutOffsetXChanged(const winrt::ScrollingPresenter& sender);
    static winrt::event_token ContentLayoutOffsetXChanged(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::IInspectable> const& value);
    static void ContentLayoutOffsetXChanged(winrt::event_token const& token);

    static void NotifyContentLayoutOffsetYChanged(const winrt::ScrollingPresenter& sender);
    static winrt::event_token ContentLayoutOffsetYChanged(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::IInspectable> const& value);
    static void ContentLayoutOffsetYChanged(winrt::event_token const& token);

    static winrt::IVector<winrt::ScrollSnapPointBase> GetConsolidatedHorizontalScrollSnapPoints(const winrt::ScrollingPresenter& scrollingPresenter);
    static winrt::IVector<winrt::ScrollSnapPointBase> GetConsolidatedVerticalScrollSnapPoints(const winrt::ScrollingPresenter& scrollingPresenter);
    static winrt::IVector<winrt::ZoomSnapPointBase> GetConsolidatedZoomSnapPoints(const winrt::ScrollingPresenter& scrollingPresenter);
    static winrt::float2 GetHorizontalSnapPointActualApplicableZone(
        const winrt::ScrollingPresenter& scrollingPresenter,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static winrt::float2 GetVerticalSnapPointActualApplicableZone(
        const winrt::ScrollingPresenter& scrollingPresenter,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static winrt::float2 GetZoomSnapPointActualApplicableZone(
        const winrt::ScrollingPresenter& scrollingPresenter,
        const winrt::ZoomSnapPointBase& zoomSnapPoint);
    static int GetHorizontalSnapPointCombinationCount(
        const winrt::ScrollingPresenter& scrollingPresenter,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static int GetVerticalSnapPointCombinationCount(
        const winrt::ScrollingPresenter& scrollingPresenter,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static int GetZoomSnapPointCombinationCount(
        const winrt::ScrollingPresenter& scrollingPresenter,
        const winrt::ZoomSnapPointBase& zoomSnapPoint);
    static winrt::Color GetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint);
    static void SetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint, const winrt::Color& color);

private:
    static winrt::ScrollingPresenterViewChangeResult TestHooksViewChangeResult(ScrollingPresenterViewChangeResult result);

    static com_ptr<ScrollingPresenterTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::ScrollingPresenterTestHooksAnchorEvaluatedEventArgs>> m_anchorEvaluatedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs>> m_interactionSourcesChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs>> m_expressionAnimationStatusChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::IInspectable>> m_contentLayoutOffsetXChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::IInspectable>> m_contentLayoutOffsetYChangedEventSource;
    bool m_areAnchorNotificationsRaised{ false };
    bool m_areInteractionSourcesNotificationsRaised{ false };
    bool m_areExpressionAnimationStatusNotificationsRaised{ false };
    bool m_isInteractionTrackerPointerWheelRedirectionEnabled{ true };
    winrt::IReference<bool> m_isAnimationsEnabledOverride{ nullptr };
    int m_offsetsChangeMsPerUnit{ ScrollingPresenter::s_offsetsChangeMsPerUnit };
    int m_offsetsChangeMinMs{ ScrollingPresenter::s_offsetsChangeMinMs };
    int m_offsetsChangeMaxMs{ ScrollingPresenter::s_offsetsChangeMaxMs };
    int m_zoomFactorChangeMsPerUnit{ ScrollingPresenter::s_zoomFactorChangeMsPerUnit };
    int m_zoomFactorChangeMinMs{ ScrollingPresenter::s_zoomFactorChangeMinMs };
    int m_zoomFactorChangeMaxMs{ ScrollingPresenter::s_zoomFactorChangeMaxMs };
    int m_mouseWheelDeltaForVelocityUnit{ ScrollingPresenter::s_mouseWheelDeltaForVelocityUnit };
    int m_mouseWheelScrollLines{ RegUtil::s_defaultMouseWheelScrollLines };
    int m_mouseWheelScrollChars{ RegUtil::s_defaultMouseWheelScrollChars };
    float m_mouseWheelInertiaDecayRate{ 0.0f };
};
