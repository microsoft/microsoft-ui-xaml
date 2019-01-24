// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"
#include "ScrollerTestHooksAnchorEvaluatedEventArgs.h"
#include "ScrollerTestHooksInteractionSourcesChangedEventArgs.h"

#include "ScrollerTestHooks.g.h"

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
    static bool IsInteractionTrackerMouseWheelZoomingEnabled();
    static void IsInteractionTrackerMouseWheelZoomingEnabled(bool isInteractionTrackerMouseWheelZoomingEnabled);
    static int MouseWheelDeltaForVelocityUnit();
    static void MouseWheelDeltaForVelocityUnit(int mouseWheelDeltaForVelocityUnit);
    static float MouseWheelInertiaDecayRate();
    static void MouseWheelInertiaDecayRate(float mouseWheelInertiaDecayRate);
    static void GetOffsetsChangeVelocityParameters(_Out_ int& millisecondsPerUnit, _Out_ int& minMilliseconds, _Out_ int& maxMilliseconds);
    static void SetOffsetsChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds);
    static void GetZoomFactorChangeVelocityParameters(_Out_ int& millisecondsPerUnit, _Out_ int& minMilliseconds, _Out_ int& maxMilliseconds);
    static void SetZoomFactorChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds);
    static void GetContentLayoutOffsetX(const winrt::Scroller& scroller, _Out_ float& contentLayoutOffsetX);
    static void SetContentLayoutOffsetX(const winrt::Scroller& scroller, float contentLayoutOffsetX);
    static void GetContentLayoutOffsetY(const winrt::Scroller& scroller, _Out_ float& contentLayoutOffsetY);
    static void SetContentLayoutOffsetY(const winrt::Scroller& scroller, float contentLayoutOffsetY);

    static void NotifyAnchorEvaluated(const winrt::Scroller& sender, const winrt::UIElement& anchorElement, double viewportAnchorPointHorizontalOffset, double viewportAnchorPointVerticalOffset);
    static winrt::event_token AnchorEvaluated(winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksAnchorEvaluatedEventArgs> const& value);
    static void AnchorEvaluated(winrt::event_token const& token);
    static void NotifyInteractionSourcesChanged(const winrt::Scroller& sender, const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources);
    static winrt::event_token InteractionSourcesChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksInteractionSourcesChangedEventArgs> const& value);
    static void InteractionSourcesChanged(winrt::event_token const& token);

    static void NotifyContentLayoutOffsetXChanged(const winrt::Scroller& sender);
    static winrt::event_token ContentLayoutOffsetXChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable> const& value);
    static void ContentLayoutOffsetXChanged(winrt::event_token const& token);

    static void NotifyContentLayoutOffsetYChanged(const winrt::Scroller& sender);
    static winrt::event_token ContentLayoutOffsetYChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable> const& value);
    static void ContentLayoutOffsetYChanged(winrt::event_token const& token);

    static winrt::IVector<winrt::ScrollerSnapPointBase> GetConsolidatedSnapPoints(const winrt::Scroller& scroller, const winrt::ScrollerSnapPointDimension& dimension);
    static winrt::float2 GetSnapPointActualApplicableZone(const winrt::ScrollerSnapPointBase& snapPoint);
    static int GetSnapPointCombinationCount(const winrt::ScrollerSnapPointBase& snapPoint);
    static winrt::Color GetSnapPointVisualizationColor(const winrt::ScrollerSnapPointBase& snapPoint);
    static void SetSnapPointVisualizationColor(const winrt::ScrollerSnapPointBase& snapPoint, const winrt::Color& color);
private:
    static com_ptr<ScrollerTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksAnchorEvaluatedEventArgs>> m_anchorEvaluatedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksInteractionSourcesChangedEventArgs>> m_interactionSourcesChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable>> m_contentLayoutOffsetXChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable>> m_contentLayoutOffsetYChangedEventSource;
    bool m_areAnchorNotificationsRaised{ false };
    bool m_areInteractionSourcesNotificationsRaised{ false };
    bool m_isInteractionTrackerMouseWheelZoomingEnabled{ true };
    int m_offsetsChangeMsPerUnit{ 0 };
    int m_offsetsChangeMinMs{ 0 };
    int m_offsetsChangeMaxMs{ 0 };
    int m_zoomFactorChangeMsPerUnit{ 0 };
    int m_zoomFactorChangeMinMs{ 0 };
    int m_zoomFactorChangeMaxMs{ 0 };
    int m_mouseWheelDeltaForVelocityUnit{ 0 };
    float m_mouseWheelInertiaDecayRate{ 0.0f };
};
