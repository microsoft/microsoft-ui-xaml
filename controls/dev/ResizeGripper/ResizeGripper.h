// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// A templated Control providing a resize affordance on either axis. Its default template supplies
// the hit-testable surface; the consumer positions it, handles DragStarted / DragDelta /
// DragCompleted, and decides what the reported drag distance means. Mirrors Thumb.
//
// The gesture itself is owned here: pointer input arrives as manipulation events, so capture,
// touch and pen contacts, and multi-pointer arbitration come from the framework's gesture
// recognizer rather than from each host. BeginDrag / TryDrag / EndDrag stay callable so a host
// can drive the same gesture from its own keyboard handling.

#pragma once

#include "pch.h"
#include "common.h"

#include "ResizeGripper.g.h"
#include "ResizeGripper.properties.h"

class ResizeGripper :
    public ReferenceTracker<ResizeGripper, winrt::implementation::ResizeGripperT>,
    public ResizeGripperProperties
{
public:
    ResizeGripper();

    void BeginDrag();
    bool TryDrag(double totalDelta);
    void EndDrag(bool canceled);
    bool TryKeyboardStep(winrt::VirtualKey key);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // IControl overrides
    void OnApplyTemplate();
    void OnPointerEntered(winrt::PointerRoutedEventArgs const& args);
    void OnPointerExited(winrt::PointerRoutedEventArgs const& args);
    void OnPointerPressed(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCanceled(winrt::PointerRoutedEventArgs const& args);
    void OnManipulationStarting(winrt::ManipulationStartingRoutedEventArgs const& args);
    void OnManipulationStarted(winrt::ManipulationStartedRoutedEventArgs const& args);
    void OnManipulationDelta(winrt::ManipulationDeltaRoutedEventArgs const& args);
    void OnManipulationCompleted(winrt::ManipulationCompletedRoutedEventArgs const& args);

    // IUIElement overrides
    winrt::AutomationPeer OnCreateAutomationPeer();

private:
    void UpdateResizeCursor();
    void UpdateManipulationMode();
    void UpdateVisualState();
    void UpdateOrientationVisualState();
    double EffectiveKeyboardIncrement();
    void OnIsEnabledChanged(const winrt::IInspectable& sender, const winrt::DependencyPropertyChangedEventArgs& args);
    void OnUnloaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    double m_lastRaisedDelta{ 0.0 };
    bool m_isPointerOver{ false };
    bool m_templateApplied{ false };
    bool m_containerIsRightToLeft{ false };
};
