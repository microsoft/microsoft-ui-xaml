// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class AnnotatedScrollBarPanningInfo
    : public ReferenceTracker<
        AnnotatedScrollBarPanningInfo,
        reference_tracker_implements_t<winrt::IScrollControllerPanningInfo>::type>
{
public:
    AnnotatedScrollBarPanningInfo();
    ~AnnotatedScrollBarPanningInfo();

#pragma region IScrollControllerPanningInfo
    bool IsRailEnabled();
    winrt::Orientation PanOrientation();
    winrt::UIElement PanningElementAncestor();

    void SetPanningElementExpressionAnimationSources(
        const winrt::CompositionPropertySet& propertySet,
        const winrt::hstring& minOffsetPropertyName,
        const winrt::hstring& maxOffsetPropertyName,
        const winrt::hstring& offsetPropertyName,
        const winrt::hstring& multiplierPropertyName);

    winrt::event_token Changed(const winrt::TypedEventHandler<winrt::IScrollControllerPanningInfo, winrt::IInspectable>& handler);
    void Changed(const winrt::event_token& token);

    winrt::event_token PanRequested(const winrt::TypedEventHandler<winrt::IScrollControllerPanningInfo, winrt::ScrollControllerPanRequestedEventArgs>& handler);
    void PanRequested(const winrt::event_token& token);
#pragma endregion

    void PanningFrameworkElement(const winrt::FrameworkElement& value);
    void PanningElementAncestor(const winrt::UIElement& value);
    void PanningElementOffsetMultiplier(float value);
    bool RaisePanRequested(const winrt::PointerPoint& pointerPoint);

private:
    void RaiseChanged();

    void EnsureThumbAnimation();
    void StartThumbAnimation();
    void StopThumbAnimation();
    void UpdateThumbExpression();

    void UpdatePanningElementOffsetMultiplier();

    tracker_ref<winrt::Visual> m_panningVisual{ this };
    tracker_ref<winrt::FrameworkElement> m_panningFrameworkElement{ this };
    tracker_ref<winrt::UIElement> m_panningElementAncestor{ this };
    winrt::CompositionPropertySet m_expressionAnimationSources{ nullptr };
    winrt::ExpressionAnimation m_thumbOffsetAnimation{ nullptr };
    winrt::hstring m_minOffsetPropertyName{ L"" };
    winrt::hstring m_maxOffsetPropertyName{ L"" };
    winrt::hstring m_offsetPropertyName{ L"" };
    winrt::hstring m_multiplierPropertyName{ L"" };
    float m_panningElementOffsetMultiplier{ 1.0f };

    event_source<winrt::TypedEventHandler<winrt::IScrollControllerPanningInfo, winrt::IInspectable>> m_changedEventSource{ this };
    event_source<winrt::TypedEventHandler<winrt::IScrollControllerPanningInfo, winrt::ScrollControllerPanRequestedEventArgs>> m_panRequestedEventSource{ this };
};
