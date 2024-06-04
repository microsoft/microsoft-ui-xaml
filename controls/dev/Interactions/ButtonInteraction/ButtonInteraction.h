// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ButtonInteraction.g.h"
#include "ButtonInteraction.properties.h"

class ButtonInteraction :
    public ReferenceTracker<ButtonInteraction, winrt::implementation::ButtonInteractionT>,
    public ButtonInteractionProperties
{
public:
    // IButtonInteraction
    winrt::ButtonInteractionInvokeMode InvokeMode();
    void InvokeMode(winrt::ButtonInteractionInvokeMode const& value);

    bool IsHovering();
    bool IsPressing();

    // INotifyPropertyChanged
    winrt::event_token PropertyChanged(winrt::PropertyChangedEventHandler const& value);
    void PropertyChanged(winrt::event_token const& token);

    winrt::IVectorView<winrt::RoutedEvent> GetSupportedEventsCore();

    void OnKeyDown(winrt::UIElement const& sender, winrt::KeyRoutedEventArgs const& args);
    void OnKeyUp(winrt::UIElement const& sender, winrt::KeyRoutedEventArgs const& args);

    void OnPointerEntered(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnPointerExited(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnPointerPressed(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnPointerReleased(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnPointerCanceled(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnPointerCaptureLost(winrt::UIElement const& sender, winrt::PointerRoutedEventArgs const& args);

private:
    void RaiseInvoked(winrt::UIElement target);
    void UpdateIsHovering(bool isHovering);
    void UpdateIsPressing(bool isPressing);

    winrt::ButtonInteractionInvokeMode m_invokeMode{ winrt::ButtonInteractionInvokeMode::Release };

    bool m_isHovering{ false };
    bool m_isPressing{ false };
    bool m_hasPointerCapture{ false };

    event_source<winrt::PropertyChangedEventHandler> m_propertyChangedEventSource{ this };
};
