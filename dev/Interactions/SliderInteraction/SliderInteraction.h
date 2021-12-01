// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SliderInteraction.g.h"

class SliderInteraction :
    public ReferenceTracker<SliderInteraction, winrt::implementation::SliderInteractionT>
{
public:
    // ISliderInteraction
    winrt::Orientation Orientation();
    void Orientation(winrt::Orientation const& value);

    double Minimum();
    void Minimum(double value);

    double Maximum();
    void Maximum(double value);

    double Position();

    double SmallChange();
    void SmallChange(double value);

    double LargeChange();
    void LargeChange(double value);

    // INotifyPropertyChanged
    winrt::event_token PropertyChanged(winrt::PropertyChangedEventHandler const& value);
    void PropertyChanged(winrt::event_token const& token);

    winrt::IVectorView<winrt::RoutedEvent> GetSupportedEventsCore();

    void OnKeyDown(winrt::UIElement const& sender, winrt::KeyRoutedEventArgs const& args);

    void OnPointerMoved(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnPointerPressed(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnPointerReleased(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnPointerCanceled(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnPointerCaptureLost(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);

private:
    void ConfigureTargetElement(const winrt::UIElement& target);
    void UpdatePosition(double delta);
    void UpdateTransform();

    winrt::Orientation m_orientation{ winrt::Orientation::Horizontal };

    double m_minimum{ 0.0 };
    double m_maximum{ 100.0 };
    double m_position{ 0.0 };

    double m_smallChange{ 1.0 };
    double m_largeChange{ 10.0 };

    bool m_hasPointerCapture{ false };
    winrt::Point m_capturePosition{};

    tracker_ref<winrt::TranslateTransform> m_translateTransform{ this };

    event_source<winrt::PropertyChangedEventHandler> m_propertyChangedEventSource{ this };
};
