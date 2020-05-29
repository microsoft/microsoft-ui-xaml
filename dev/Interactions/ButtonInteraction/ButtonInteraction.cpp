// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "ButtonInteraction.h"
#include "ButtonInteractionInvokedEventArgs.h"

#include "ButtonInteraction.properties.cpp"

winrt::ButtonInteractionInvokeMode ButtonInteraction::InvokeMode()
{
    return m_invokeMode;
}

void ButtonInteraction::InvokeMode(winrt::ButtonInteractionInvokeMode const& value)
{

    auto invokeMode = value;
    if (invokeMode != m_invokeMode)
    {
        m_invokeMode = value;
        m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(L"InvokeMode"));
    }
}

bool ButtonInteraction::IsHovering()
{
    return m_isHovering;
}

bool ButtonInteraction::IsPressing()
{
    return m_isPressing;
}

winrt::event_token ButtonInteraction::PropertyChanged(winrt::PropertyChangedEventHandler const& value)
{
    return m_propertyChangedEventSource.add(value);
}

void ButtonInteraction::PropertyChanged(winrt::event_token const& token)
{
    m_propertyChangedEventSource.remove(token);
}

winrt::IVectorView<winrt::RoutedEvent> ButtonInteraction::GetSupportedEventsCore()
{
    std::vector<winrt::RoutedEvent> supportedEvents;

    // Build the list of events that we'll respond to.
    supportedEvents.push_back(winrt::UIElement::KeyDownEvent());
    supportedEvents.push_back(winrt::UIElement::KeyUpEvent());
    supportedEvents.push_back(winrt::UIElement::PointerExitedEvent());
    supportedEvents.push_back(winrt::UIElement::PointerEnteredEvent());
    supportedEvents.push_back(winrt::UIElement::PointerExitedEvent());
    supportedEvents.push_back(winrt::UIElement::PointerPressedEvent());
    supportedEvents.push_back(winrt::UIElement::PointerReleasedEvent());
    supportedEvents.push_back(winrt::UIElement::PointerCanceledEvent());
    supportedEvents.push_back(winrt::UIElement::PointerCaptureLostEvent());

    return winrt::single_threaded_vector(std::move(supportedEvents)).GetView();
}

void ButtonInteraction::OnKeyDown(winrt::UIElement const& sender, winrt::KeyRoutedEventArgs const& args)
{
    // Only process key events if the pointer is not currently being pressed.
    if (!m_hasPointerCapture)
    {
        auto target = sender;
        auto keyArgs = args;

        switch (keyArgs.Key())
        {
        case winrt::VirtualKey::Space:
        case winrt::VirtualKey::Enter:
            UpdateIsPressing(true);

            if (m_invokeMode == winrt::ButtonInteractionInvokeMode::Press)
            {
                RaiseInvoked(target);
            }
            break;
        }
    }
}

void ButtonInteraction::OnKeyUp(winrt::UIElement const& sender, winrt::KeyRoutedEventArgs const& args)
{
    // Only process key events if the pointer is not currently being pressed.
    if (!m_hasPointerCapture)
    {
        auto target = sender;
        auto keyArgs = args;

        switch (keyArgs.Key())
        {
        case winrt::VirtualKey::Space:
        case winrt::VirtualKey::Enter:
            UpdateIsPressing(false);

            if (m_invokeMode == winrt::ButtonInteractionInvokeMode::Release)
            {
                RaiseInvoked(target);
            }

            break;
        }
    }
}

void ButtonInteraction::OnPointerEntered(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    auto target = sender;
    auto pointerArgs = args;

    UpdateIsHovering(true);

    if (m_invokeMode == winrt::ButtonInteractionInvokeMode::Hover)
    {
        UpdateIsPressing(true);
        RaiseInvoked(target);
    }
}

void ButtonInteraction::OnPointerExited(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    auto target = sender;

    UpdateIsHovering(false);

    if (m_invokeMode == winrt::ButtonInteractionInvokeMode::Hover || m_hasPointerCapture)
    {
        UpdateIsPressing(false);
    }
}

void ButtonInteraction::OnPointerPressed(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    auto target = sender;
    auto pointerArgs = args;

    UpdateIsPressing(true);

    if (m_invokeMode == winrt::ButtonInteractionInvokeMode::Press)
    {
        RaiseInvoked(target);
    }

    m_hasPointerCapture = target.CapturePointer(pointerArgs.Pointer());
}

void ButtonInteraction::OnPointerReleased(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    auto target = sender;
    auto pointerArgs = args;

    const bool wasPressing = m_isPressing;

    UpdateIsPressing(false);

    if (wasPressing && m_invokeMode == winrt::ButtonInteractionInvokeMode::Release)
    {
        RaiseInvoked(target);
    }

    if (m_hasPointerCapture)
    {
        target.ReleasePointerCapture(pointerArgs.Pointer());
    }
}

void ButtonInteraction::OnPointerCanceled(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    auto target = sender;
    auto pointerArgs = args;

    if (m_hasPointerCapture)
    {
        target.ReleasePointerCapture(pointerArgs.Pointer());
    }

    UpdateIsPressing(false);
}

void ButtonInteraction::OnPointerCaptureLost(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args)
{
    auto target = sender;

    m_hasPointerCapture = false;
    UpdateIsPressing(false);
}

void ButtonInteraction::RaiseInvoked(winrt::UIElement target)
{
    auto eventArgs = winrt::make_self<ButtonInteractionInvokedEventArgs>(target);
    m_invokedEventSource(*this, *eventArgs);
}

void ButtonInteraction::UpdateIsHovering(bool isHovering)
{
    if (isHovering != m_isHovering)
    {
        m_isHovering = isHovering;
        m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(L"IsHovering"));
    }
}

void ButtonInteraction::UpdateIsPressing(bool isPressing)
{
    if (isPressing != m_isPressing)
    {
        m_isPressing = isPressing;
        m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(L"IsPressing"));
    }
}
