// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResizeGripper.h"
#include "ResizeGripperAutomationPeer.h"
#include "ResizeGripperDragDeltaEventArgs.h"
#include "ResizeGripperDragCompletedEventArgs.h"
#include "RuntimeProfiler.h"
#include <cmath>

namespace
{
    // Sub-pixel jitter should not reach the host. DIPs.
    constexpr double c_dragDeadband{ 0.5 };

    // Shift takes a coarser step. Kept as a constant rather than a second property: KeyboardIncrement
    // already lets a host choose the base step.
    constexpr double c_largeIncrementMultiplier{ 4.0 };

    // Matches the KeyboardIncrement default; used when a host supplies an unusable one.
    constexpr double c_defaultKeyboardIncrement{ 8.0 };

    bool IsShiftKeyDown()
    {
        return (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Shift) &
            winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
    }
}

ResizeGripper::ResizeGripper()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ResizeGripper);

    SetDefaultStyleKey(this);
    // ProtectedCursor is deliberately NOT set here - see UpdateResizeCursor.
    // Set here rather than in the default style: without it no manipulation is ever raised, so a
    // host whose style failed to load would have no pointer resize at all.
    UpdateManipulationMode();

    IsEnabledChanged({ this, &ResizeGripper::OnIsEnabledChanged });
    Unloaded({ this, &ResizeGripper::OnUnloaded });
}

// Detached mid-gesture - the host rebuilt the subtree we live in - so no manipulation event will
// arrive to complete the drag and IsDragging would stay true for the rest of this instance's life.
void ResizeGripper::OnUnloaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    m_isPointerOver = false;
    EndDrag(true /* canceled */);
}

void ResizeGripper::OnIsEnabledChanged(const winrt::IInspectable&, const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateVisualState();
}

void ResizeGripper::OnApplyTemplate()
{
    __super::OnApplyTemplate();
    m_templateApplied = true;
    UpdateResizeCursor();
    UpdateOrientationVisualState();
    UpdateVisualState();
}

void ResizeGripper::OnPointerEntered(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerEntered(args);
    m_isPointerOver = true;
    // A cursor assigned before the element is live does not stick (microsoft-ui-xaml#7062).
    UpdateResizeCursor();
    UpdateVisualState();
}

void ResizeGripper::OnPointerExited(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerExited(args);
    m_isPointerOver = false;
    UpdateVisualState();
}

// Pressed stays on for the whole drag even if the pointer leaves the gripper, because the host
// captures the pointer and the column keeps resizing.
void ResizeGripper::UpdateVisualState()
{
    auto const state =
        !IsEnabled() ? L"Disabled" :
        IsDragging() ? L"Pressed" :
        m_isPointerOver ? L"PointerOver" : L"Normal";

    winrt::VisualStateManager::GoToState(*this, state, true /* useTransitions */);
}

void ResizeGripper::UpdateOrientationVisualState()
{
    winrt::VisualStateManager::GoToState(*this,
        (DragOrientation() == winrt::Orientation::Vertical) ? L"Vertical" : L"Horizontal",
        true /* useTransitions */);
}

// Owned by the primitive so every host gets the right shape, keyed off the direction of travel.
// Two guards, both of which cause a visibly flickering cursor when missed:
//  - nothing before OnApplyTemplate. Assigning ProtectedCursor that early does not stick reliably
//    (microsoft-ui-xaml#7062), so the shape gets re-resolved while the pointer is already over us.
//  - never reassign the shape already in effect. Handing the input system a brand new
//    InputSystemCursor makes the pointer visibly reset even when the shape is identical.
void ResizeGripper::UpdateResizeCursor()
{
    if (!m_templateApplied)
    {
        return;
    }

    const auto shape = (DragOrientation() == winrt::Orientation::Vertical)
        ? winrt::InputSystemCursorShape::SizeNorthSouth
        : winrt::InputSystemCursorShape::SizeWestEast;

    if (auto const current = ProtectedCursor().try_as<winrt::InputSystemCursor>();
        current && current.CursorShape() == shape)
    {
        return;
    }

    ProtectedCursor(winrt::InputSystemCursor::Create(shape).as<winrt::InputCursor>());
}

// One axis only: the cross-axis translation is noise, and leaving it out lets an ancestor
// ScrollViewer keep panning on the axis we do not resize.
void ResizeGripper::UpdateManipulationMode()
{
    ManipulationMode((DragOrientation() == winrt::Orientation::Vertical)
        ? winrt::ManipulationModes::TranslateY
        : winrt::ManipulationModes::TranslateX);
}

void ResizeGripper::OnManipulationStarting(winrt::ManipulationStartingRoutedEventArgs const& args)
{
    __super::OnManipulationStarting(args);

    // Default container is this element, so the two frames agree by definition.
    m_containerIsRightToLeft = FlowDirection() == winrt::FlowDirection::RightToLeft;

    // A host-designated frame wins: it knows which of its ancestors stays put while the drag
    // resizes something, and it is not subject to any scale applied above it.
    auto container = ManipulationContainer();
    if (!container)
    {
        // The gripper travels with the edge it drags, so the default container - itself - measures
        // the pointer against a frame the drag is moving, and the gesture stalls. The XamlRoot
        // content does not move while a host resizes.
        if (auto const xamlRoot = XamlRoot())
        {
            container = xamlRoot.Content();
        }
    }

    if (container)
    {
        args.Container(container);

        // Cumulative().Translation is expressed in the CONTAINER's space, and RTL mirrors that
        // space. Record the container's direction so the delta is mirrored against the frame it
        // was actually measured in, not against this element's own direction.
        if (auto const containerElement = container.try_as<winrt::FrameworkElement>())
        {
            m_containerIsRightToLeft = containerElement.FlowDirection() == winrt::FlowDirection::RightToLeft;
        }
    }
}

void ResizeGripper::OnManipulationStarted(winrt::ManipulationStartedRoutedEventArgs const& args)
{
    __super::OnManipulationStarted(args);

    BeginDrag();
    args.Handled(true);
}

void ResizeGripper::OnManipulationDelta(winrt::ManipulationDeltaRoutedEventArgs const& args)
{
    __super::OnManipulationDelta(args);

    const bool isHorizontal = DragOrientation() != winrt::Orientation::Vertical;
    const auto translation = args.Cumulative().Translation;
    double totalDelta = isHorizontal ? translation.X : translation.Y;

    // Report a logical delta, so positive always grows in reading order and both input paths agree.
    // Mirror only when this element and the measurement frame disagree; mirroring on the element's
    // own direction inverts the drag in a fully-RTL app, where the container is mirrored too.
    if (isHorizontal && (FlowDirection() == winrt::FlowDirection::RightToLeft) != m_containerIsRightToLeft)
    {
        totalDelta = -totalDelta;
    }

    TryDrag(totalDelta);
    args.Handled(true);
}

void ResizeGripper::OnManipulationCompleted(winrt::ManipulationCompletedRoutedEventArgs const& args)
{
    __super::OnManipulationCompleted(args);

    EndDrag(false /* canceled */);
    args.Handled(true);
}

// Manipulation never marks the press handled, so without this the press bubbles to whatever the
// host put above us - for a header cell that means taking focus mid-gesture, and the resulting
// bring-into-view scroll moves the gripper out from under a stationary pointer. Marking it handled
// does NOT suppress the manipulation: that is driven by ManipulationMode on this element.
void ResizeGripper::OnPointerPressed(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerPressed(args);

    if (IsEnabled())
    {
        args.Handled(true);
    }
}

// A contact the system takes away (palm rejection, the contact leaving the digitizer) is not a
// release: the user never committed, and no ManipulationCompleted follows a canceled contact.
void ResizeGripper::OnPointerCanceled(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerCanceled(args);

    m_isPointerOver = false;
    EndDrag(true /* canceled */);
    UpdateVisualState();
}

void ResizeGripper::BeginDrag()
{
    // A disabled control takes no pointer input from the framework, but BeginDrag is callable
    // directly (that is how the keyboard path drives it), so gate here too.
    if (!IsEnabled() || IsDragging())
    {
        return;
    }
    m_lastRaisedDelta = 0.0;
    SetValue(s_IsDraggingProperty, box_value(true));
    UpdateVisualState();
    m_dragStartedEventSource(*this, nullptr);
}

bool ResizeGripper::TryDrag(double totalDelta)
{
    if (!IsDragging() || !std::isfinite(totalDelta))
    {
        return false;
    }

    if (std::abs(totalDelta - m_lastRaisedDelta) < c_dragDeadband)
    {
        return false;
    }

    const double delta = totalDelta - m_lastRaisedDelta;
    m_lastRaisedDelta = totalDelta;

    auto eventArgs = winrt::make<::ResizeGripperDragDeltaEventArgs>(delta, totalDelta);
    m_dragDeltaEventSource(*this, eventArgs);
    return true;
}

void ResizeGripper::EndDrag(bool canceled)
{
    if (!IsDragging())
    {
        return;
    }

    const double totalDelta = m_lastRaisedDelta;

    // Clear the flag before raising: a throwing handler would otherwise leave IsDragging stuck
    // true, wedging every later BeginDrag.
    SetValue(s_IsDraggingProperty, box_value(false));
    UpdateVisualState();

    auto eventArgs = winrt::make<::ResizeGripperDragCompletedEventArgs>(totalDelta, canceled);
    m_dragCompletedEventSource(*this, eventArgs);
}

void ResizeGripper::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.Property() == s_DragOrientationProperty)
    {
        UpdateResizeCursor();
        UpdateOrientationVisualState();
        UpdateManipulationMode();
    }
}

// A host-settable step needs sanitising before it reaches TryDrag: a negative one would resize
// backwards, and a non-finite or sub-deadband one would be swallowed by TryDrag - leaving the
// keyboard silently dead while still raising a start/complete pair, which a host announces to a
// screen reader as an unchanged width on every arrow press.
double ResizeGripper::EffectiveKeyboardIncrement()
{
    const double raw = std::abs(KeyboardIncrement());
    if (!std::isfinite(raw) || raw == 0.0)
    {
        return c_defaultKeyboardIncrement;
    }
    return std::max(raw, c_dragDeadband);
}

// One implementation of "an arrow key is a one-step drag", callable so a host that owns focus
// itself (and therefore never lets the gripper see the key) still gets the same step size, Shift
// multiplier and RTL mirror. Returns false when the key is not one this gripper acts on.
bool ResizeGripper::TryKeyboardStep(winrt::VirtualKey key)
{
    const bool isHorizontal = DragOrientation() == winrt::Orientation::Horizontal;

    double direction = 0.0;
    switch (key)
    {
        case winrt::VirtualKey::Left:  direction = isHorizontal ? -1.0 : 0.0; break;
        case winrt::VirtualKey::Right: direction = isHorizontal ? 1.0 : 0.0; break;
        case winrt::VirtualKey::Up:    direction = isHorizontal ? 0.0 : -1.0; break;
        case winrt::VirtualKey::Down:  direction = isHorizontal ? 0.0 : 1.0; break;
        default: break;
    }

    if (direction == 0.0)
    {
        return false;
    }

    // A pointer drag is already in flight: a keyboard step would retarget its anchor and end it.
    // Reported as handled so the key cannot also move focus mid-gesture.
    if (IsDragging())
    {
        return true;
    }

    // Mirror the axis under RTL so Left always shrinks visually. FlowDirection does not mirror y.
    if (isHorizontal && FlowDirection() == winrt::FlowDirection::RightToLeft)
    {
        direction *= -1.0;
    }

    const double step = IsShiftKeyDown()
        ? EffectiveKeyboardIncrement() * c_largeIncrementMultiplier
        : EffectiveKeyboardIncrement();

    // No ManipulationCompleted follows a keyboard step, so a throw would strand IsDragging.
    try
    {
        BeginDrag();
        TryDrag(direction * step);
    }
    catch (...)
    {
        EndDrag(true /* canceled */);
        throw;
    }

    EndDrag(false /* canceled */);
    return true;
}

winrt::AutomationPeer ResizeGripper::OnCreateAutomationPeer()
{
    return winrt::make<ResizeGripperAutomationPeer>(*this);
}
