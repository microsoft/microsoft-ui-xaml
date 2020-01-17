// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollPresenter.h"
#include "ScrollPresenterTestHooksAnchorEvaluatedEventArgs.h"
#include "ScrollPresenterTestHooksInteractionSourcesChangedEventArgs.h"
#include "ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs.h"
#include "ScrollPresenterTestHooks.g.h"
#include "RegUtil.h"

class ScrollPresenterTestHooks :
    public winrt::implementation::ScrollPresenterTestHooksT<ScrollPresenterTestHooks>
{
public:
    ScrollPresenterTestHooks();

    static com_ptr<ScrollPresenterTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<ScrollPresenterTestHooks> EnsureGlobalTestHooks();

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
    static void GetContentLayoutOffsetX(const winrt::ScrollPresenter& scrollPresenter, float& contentLayoutOffsetX);
    static void SetContentLayoutOffsetX(const winrt::ScrollPresenter& scrollPresenter, float contentLayoutOffsetX);
    static void GetContentLayoutOffsetY(const winrt::ScrollPresenter& scrollPresenter, float& contentLayoutOffsetY);
    static void SetContentLayoutOffsetY(const winrt::ScrollPresenter& scrollPresenter, float contentLayoutOffsetY);
    static winrt::float2 GetArrangeRenderSizesDelta(const winrt::ScrollPresenter& scrollPresenter);
    static winrt::float2 GetMinPosition(const winrt::ScrollPresenter& scrollPresenter);
    static winrt::float2 GetMaxPosition(const winrt::ScrollPresenter& scrollPresenter);
    static winrt::ScrollPresenterViewChangeResult GetScrollCompletedResult(const winrt::ScrollingScrollCompletedEventArgs& scrollCompletedEventArgs);
    static winrt::ScrollPresenterViewChangeResult GetZoomCompletedResult(const winrt::ScrollingZoomCompletedEventArgs& zoomCompletedEventArgs);

    static void NotifyAnchorEvaluated(const winrt::ScrollPresenter& sender, const winrt::UIElement& anchorElement, double viewportAnchorPointHorizontalOffset, double viewportAnchorPointVerticalOffset);
    static winrt::event_token AnchorEvaluated(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::ScrollPresenterTestHooksAnchorEvaluatedEventArgs> const& value);
    static void AnchorEvaluated(winrt::event_token const& token);

    static void NotifyInteractionSourcesChanged(const winrt::ScrollPresenter& sender, const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources);
    static winrt::event_token InteractionSourcesChanged(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::ScrollPresenterTestHooksInteractionSourcesChangedEventArgs> const& value);
    static void InteractionSourcesChanged(winrt::event_token const& token);

    static void NotifyExpressionAnimationStatusChanged(const winrt::ScrollPresenter& sender, bool isExpressionAnimationStarted, wstring_view const& propertyName);
    static winrt::event_token ExpressionAnimationStatusChanged(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs> const& value);
    static void ExpressionAnimationStatusChanged(winrt::event_token const& token);

    static void NotifyContentLayoutOffsetXChanged(const winrt::ScrollPresenter& sender);
    static winrt::event_token ContentLayoutOffsetXChanged(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::IInspectable> const& value);
    static void ContentLayoutOffsetXChanged(winrt::event_token const& token);

    static void NotifyContentLayoutOffsetYChanged(const winrt::ScrollPresenter& sender);
    static winrt::event_token ContentLayoutOffsetYChanged(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::IInspectable> const& value);
    static void ContentLayoutOffsetYChanged(winrt::event_token const& token);

    static winrt::IVector<winrt::ScrollSnapPointBase> GetConsolidatedHorizontalScrollSnapPoints(const winrt::ScrollPresenter& scrollPresenter);
    static winrt::IVector<winrt::ScrollSnapPointBase> GetConsolidatedVerticalScrollSnapPoints(const winrt::ScrollPresenter& scrollPresenter);
    static winrt::IVector<winrt::ZoomSnapPointBase> GetConsolidatedZoomSnapPoints(const winrt::ScrollPresenter& scrollPresenter);
    static winrt::float2 GetHorizontalSnapPointActualApplicableZone(
        const winrt::ScrollPresenter& scrollPresenter,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static winrt::float2 GetVerticalSnapPointActualApplicableZone(
        const winrt::ScrollPresenter& scrollPresenter,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static winrt::float2 GetZoomSnapPointActualApplicableZone(
        const winrt::ScrollPresenter& scrollPresenter,
        const winrt::ZoomSnapPointBase& zoomSnapPoint);
    static int GetHorizontalSnapPointCombinationCount(
        const winrt::ScrollPresenter& scrollPresenter,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static int GetVerticalSnapPointCombinationCount(
        const winrt::ScrollPresenter& scrollPresenter,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static int GetZoomSnapPointCombinationCount(
        const winrt::ScrollPresenter& scrollPresenter,
        const winrt::ZoomSnapPointBase& zoomSnapPoint);
    static winrt::Color GetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint);
    static void SetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint, const winrt::Color& color);

private:
    static winrt::ScrollPresenterViewChangeResult TestHooksViewChangeResult(ScrollPresenterViewChangeResult result);

    static com_ptr<ScrollPresenterTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::ScrollPresenterTestHooksAnchorEvaluatedEventArgs>> m_anchorEvaluatedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::ScrollPresenterTestHooksInteractionSourcesChangedEventArgs>> m_interactionSourcesChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs>> m_expressionAnimationStatusChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::IInspectable>> m_contentLayoutOffsetXChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::IInspectable>> m_contentLayoutOffsetYChangedEventSource;
    bool m_areAnchorNotificationsRaised{ false };
    bool m_areInteractionSourcesNotificationsRaised{ false };
    bool m_areExpressionAnimationStatusNotificationsRaised{ false };
    bool m_isInteractionTrackerPointerWheelRedirectionEnabled{ true };
    winrt::IReference<bool> m_isAnimationsEnabledOverride{ nullptr };
    int m_offsetsChangeMsPerUnit{ ScrollPresenter::s_offsetsChangeMsPerUnit };
    int m_offsetsChangeMinMs{ ScrollPresenter::s_offsetsChangeMinMs };
    int m_offsetsChangeMaxMs{ ScrollPresenter::s_offsetsChangeMaxMs };
    int m_zoomFactorChangeMsPerUnit{ ScrollPresenter::s_zoomFactorChangeMsPerUnit };
    int m_zoomFactorChangeMinMs{ ScrollPresenter::s_zoomFactorChangeMinMs };
    int m_zoomFactorChangeMaxMs{ ScrollPresenter::s_zoomFactorChangeMaxMs };
    int m_mouseWheelDeltaForVelocityUnit{ ScrollPresenter::s_mouseWheelDeltaForVelocityUnit };
    int m_mouseWheelScrollLines{ RegUtil::s_defaultMouseWheelScrollLines };
    int m_mouseWheelScrollChars{ RegUtil::s_defaultMouseWheelScrollChars };
    float m_mouseWheelInertiaDecayRate{ 0.0f };
};
