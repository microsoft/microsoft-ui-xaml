// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "SliderInteraction.h"

#include "SliderInteraction.properties.cpp"

winrt::Orientation SliderInteraction::Orientation()
{
    return m_orientation;
}

void SliderInteraction::Orientation(winrt::Orientation const& value)
{
    const auto orientation = value;
    if (orientation != m_orientation)
    {
        m_orientation = orientation;
        UpdateTransform();

        m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(L"Orientation"));
    }
}

double SliderInteraction::Minimum()
{
    return m_minimum;
}

void SliderInteraction::Minimum(double value)
{

    if (value > m_maximum)
    {
        throw winrt::hresult_invalid_argument(L"Minimum cannot be larger than Maximum.");
    }

    if (value != m_minimum)
    {
        m_minimum = value;
        UpdatePosition(std::max(m_minimum, m_position) - m_position);

        m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(L"Minimum"));
    }
}

double SliderInteraction::Maximum()
{
    return m_maximum;
}

void SliderInteraction::Maximum(double value)
{

    if (value < m_minimum)
    {
        throw winrt::hresult_invalid_argument(L"Maximum cannot be smaller than Minimum.");
    }

    if (value != m_maximum)
    {
        m_maximum = value;
        UpdatePosition(std::min(m_maximum, m_position) - m_position);

        m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(L"Maximum"));
    }
}

double SliderInteraction::Position()
{
    return m_position;
}

void SliderInteraction::SmallChange(double value)
{

    if (value > m_largeChange)
    {
        throw winrt::hresult_invalid_argument(L"SmallChange value cannot be larger than LargeChange value.");
    }

    if (value != m_smallChange)
    {
        m_smallChange = value;
        m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(L"SmallChange"));
    }
}

double SliderInteraction::SmallChange()
{
    return m_smallChange;
}

void SliderInteraction::LargeChange(double value)
{

    if (value < m_smallChange)
    {
        throw winrt::hresult_invalid_argument(L"LargeChange value cannot be smaller than SmallChange value.");
    }

    if (value != m_largeChange)
    {
        m_largeChange = value;
        m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(L"LargeChange"));
    }
}

double SliderInteraction::LargeChange()
{
    return m_largeChange;
}

winrt::event_token SliderInteraction::PropertyChanged(winrt::PropertyChangedEventHandler const& value)
{
    return m_propertyChangedEventSource.add(value);
}

void SliderInteraction::PropertyChanged(winrt::event_token const& token)
{
    m_propertyChangedEventSource.remove(token);
}

winrt::IVectorView<winrt::RoutedEvent> SliderInteraction::GetSupportedEventsCore()
{
    std::vector<winrt::RoutedEvent> supportedEvents;

    // Build the list of events that we'll respond to.
    supportedEvents.push_back(winrt::UIElement::KeyDownEvent());
    supportedEvents.push_back(winrt::UIElement::PointerMovedEvent());
    supportedEvents.push_back(winrt::UIElement::PointerPressedEvent());
    supportedEvents.push_back(winrt::UIElement::PointerReleasedEvent());
    supportedEvents.push_back(winrt::UIElement::PointerCanceledEvent());
    supportedEvents.push_back(winrt::UIElement::PointerCaptureLostEvent());

    return winrt::single_threaded_vector(std::move(supportedEvents)).GetView();
}

void SliderInteraction::OnKeyDown(winrt::UIElement const& sender, winrt::KeyRoutedEventArgs const& args)
{
    const auto& target = sender;
    const auto& keyArgs = args;

    ConfigureTargetElement(target);

    switch (keyArgs.Key())
    {
    case winrt::VirtualKey::Left:
    case winrt::VirtualKey::Up:
        UpdatePosition(-m_smallChange);
        break;

    case winrt::VirtualKey::Right:
    case winrt::VirtualKey::Down:
        UpdatePosition(m_smallChange);
        break;

    case winrt::VirtualKey::PageUp:
        UpdatePosition(-m_largeChange);
        break;

    case winrt::VirtualKey::PageDown:
        UpdatePosition(m_largeChange);
        break;

    case winrt::VirtualKey::Home:
        UpdatePosition(m_minimum - m_position);
        break;

    case winrt::VirtualKey::End:
        UpdatePosition(m_maximum - m_position);
        break;
    }
}

void SliderInteraction::OnPointerMoved(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    if (m_hasPointerCapture)
    {
        const auto& target = sender;
        const auto& pointerArgs = args;
        const auto position = pointerArgs.GetCurrentPoint(target).Position();

        const winrt::Point delta(m_capturePosition.X - position.X, m_capturePosition.Y - position.Y);

        UpdatePosition(m_orientation == winrt::Orientation::Horizontal ? -delta.X : -delta.Y);
    }
}

void SliderInteraction::OnPointerPressed(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    const auto& target = sender;
    const auto& pointerArgs = args;

    ConfigureTargetElement(target);

    m_hasPointerCapture = target.CapturePointer(pointerArgs.Pointer());
    m_capturePosition = pointerArgs.GetCurrentPoint(target).Position();
}

void SliderInteraction::OnPointerReleased(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    const auto& target = sender;
    const auto& pointerArgs = args;

    if (m_hasPointerCapture)
    {
        target.ReleasePointerCapture(pointerArgs.Pointer());
    }
}

void SliderInteraction::OnPointerCanceled(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    const auto& target = sender;
    const auto& pointerArgs = args;

    if (m_hasPointerCapture)
    {
        target.ReleasePointerCapture(pointerArgs.Pointer());
    }
}

void SliderInteraction::OnPointerCaptureLost(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    m_hasPointerCapture = false;
    m_capturePosition = winrt::Point();
}

void SliderInteraction::ConfigureTargetElement(const winrt::UIElement& target)
{
    if (!m_translateTransform)
    {
        winrt::TranslateTransform translateTransform;

        if (m_orientation == winrt::Orientation::Horizontal)
        {
            translateTransform.X(m_position);
            translateTransform.Y(0.0);
        }
        else
        {
            translateTransform.X(0.0);
            translateTransform.Y(m_position);
        }

        target.RenderTransform(translateTransform);

        m_translateTransform.set(translateTransform);
    }
}

void SliderInteraction::UpdatePosition(double delta)
{
    auto position = std::clamp(m_position + delta, m_minimum, m_maximum);
    if (position != m_position)
    {
        m_position = position;

        UpdateTransform();

        m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(L"Position"));
    }
}

void SliderInteraction::UpdateTransform()
{
    if (m_translateTransform)
    {
        if (m_orientation == winrt::Orientation::Horizontal)
        {
            m_translateTransform.get().X(m_position);
            m_translateTransform.get().Y(0.0);
        }
        else
        {
            m_translateTransform.get().X(0.0);
            m_translateTransform.get().Y(m_position);
        }
    }
}
