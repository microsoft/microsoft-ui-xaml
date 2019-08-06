// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"
#include "ScrollerTestHooksAnchorEvaluatedEventArgs.h"
#include "ScrollerTestHooksInteractionSourcesChangedEventArgs.h"
#include "ScrollerTestHooksExpressionAnimationStatusChangedEventArgs.h"
#include "ScrollerTestHooks.g.h"
#include "RegUtil.h"

class ScrollerTestHooks :
    public winrt::implementation::ScrollerTestHooksT<ScrollerTestHooks>
{
public:
    ScrollerTestHooks();

    static com_ptr<ScrollerTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<ScrollerTestHooks> EnsureGlobalTestHooks();

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
    static void GetContentLayoutOffsetX(const winrt::Scroller& scroller, float& contentLayoutOffsetX);
    static void SetContentLayoutOffsetX(const winrt::Scroller& scroller, float contentLayoutOffsetX);
    static void GetContentLayoutOffsetY(const winrt::Scroller& scroller, float& contentLayoutOffsetY);
    static void SetContentLayoutOffsetY(const winrt::Scroller& scroller, float contentLayoutOffsetY);
    static winrt::float2 GetArrangeRenderSizesDelta(const winrt::Scroller& scroller);
    static winrt::float2 GetMinPosition(const winrt::Scroller& scroller);
    static winrt::float2 GetMaxPosition(const winrt::Scroller& scroller);
    static winrt::ScrollerViewChangeResult GetScrollCompletedResult(const winrt::ScrollCompletedEventArgs& scrollCompletedEventArgs);
    static winrt::ScrollerViewChangeResult GetZoomCompletedResult(const winrt::ZoomCompletedEventArgs& zoomCompletedEventArgs);

    static void NotifyAnchorEvaluated(const winrt::Scroller& sender, const winrt::UIElement& anchorElement, double viewportAnchorPointHorizontalOffset, double viewportAnchorPointVerticalOffset);
    static winrt::event_token AnchorEvaluated(winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksAnchorEvaluatedEventArgs> const& value);
    static void AnchorEvaluated(winrt::event_token const& token);

    static void NotifyInteractionSourcesChanged(const winrt::Scroller& sender, const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources);
    static winrt::event_token InteractionSourcesChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksInteractionSourcesChangedEventArgs> const& value);
    static void InteractionSourcesChanged(winrt::event_token const& token);

    static void NotifyExpressionAnimationStatusChanged(const winrt::Scroller& sender, bool isExpressionAnimationStarted, wstring_view const& propertyName);
    static winrt::event_token ExpressionAnimationStatusChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksExpressionAnimationStatusChangedEventArgs> const& value);
    static void ExpressionAnimationStatusChanged(winrt::event_token const& token);

    static void NotifyContentLayoutOffsetXChanged(const winrt::Scroller& sender);
    static winrt::event_token ContentLayoutOffsetXChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable> const& value);
    static void ContentLayoutOffsetXChanged(winrt::event_token const& token);

    static void NotifyContentLayoutOffsetYChanged(const winrt::Scroller& sender);
    static winrt::event_token ContentLayoutOffsetYChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable> const& value);
    static void ContentLayoutOffsetYChanged(winrt::event_token const& token);

    static winrt::IVector<winrt::ScrollSnapPointBase> GetConsolidatedHorizontalScrollSnapPoints(const winrt::Scroller& scroller);
    static winrt::IVector<winrt::ScrollSnapPointBase> GetConsolidatedVerticalScrollSnapPoints(const winrt::Scroller& scroller);
    static winrt::IVector<winrt::ZoomSnapPointBase> GetConsolidatedZoomSnapPoints(const winrt::Scroller& scroller);
    static winrt::float2 GetHorizontalSnapPointActualApplicableZone(
        const winrt::Scroller& scroller,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static winrt::float2 GetVerticalSnapPointActualApplicableZone(
        const winrt::Scroller& scroller,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static winrt::float2 GetZoomSnapPointActualApplicableZone(
        const winrt::Scroller& scroller,
        const winrt::ZoomSnapPointBase& zoomSnapPoint);
    static int GetHorizontalSnapPointCombinationCount(
        const winrt::Scroller& scroller,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static int GetVerticalSnapPointCombinationCount(
        const winrt::Scroller& scroller,
        const winrt::ScrollSnapPointBase& scrollSnapPoint);
    static int GetZoomSnapPointCombinationCount(
        const winrt::Scroller& scroller,
        const winrt::ZoomSnapPointBase& zoomSnapPoint);
    static winrt::Color GetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint);
    static void SetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint, const winrt::Color& color);

private:
    static winrt::ScrollerViewChangeResult TestHooksViewChangeResult(ScrollerViewChangeResult result);

    static com_ptr<ScrollerTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksAnchorEvaluatedEventArgs>> m_anchorEvaluatedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksInteractionSourcesChangedEventArgs>> m_interactionSourcesChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksExpressionAnimationStatusChangedEventArgs>> m_expressionAnimationStatusChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable>> m_contentLayoutOffsetXChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable>> m_contentLayoutOffsetYChangedEventSource;
    bool m_areAnchorNotificationsRaised{ false };
    bool m_areInteractionSourcesNotificationsRaised{ false };
    bool m_areExpressionAnimationStatusNotificationsRaised{ false };
    bool m_isInteractionTrackerPointerWheelRedirectionEnabled{ true };
    winrt::IReference<bool> m_isAnimationsEnabledOverride{ nullptr };
    int m_offsetsChangeMsPerUnit{ Scroller::s_offsetsChangeMsPerUnit };
    int m_offsetsChangeMinMs{ Scroller::s_offsetsChangeMinMs };
    int m_offsetsChangeMaxMs{ Scroller::s_offsetsChangeMaxMs };
    int m_zoomFactorChangeMsPerUnit{ Scroller::s_zoomFactorChangeMsPerUnit };
    int m_zoomFactorChangeMinMs{ Scroller::s_zoomFactorChangeMinMs };
    int m_zoomFactorChangeMaxMs{ Scroller::s_zoomFactorChangeMaxMs };
    int m_mouseWheelDeltaForVelocityUnit{ Scroller::s_mouseWheelDeltaForVelocityUnit };
    int m_mouseWheelScrollLines{ RegUtil::s_defaultMouseWheelScrollLines };
    int m_mouseWheelScrollChars{ RegUtil::s_defaultMouseWheelScrollChars };
    float m_mouseWheelInertiaDecayRate{ 0.0f };
};
