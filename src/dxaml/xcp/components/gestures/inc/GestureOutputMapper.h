// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <enumdefs.g.h>
#include <paltypes.h>
#include <Microsoft.UI.Xaml.coretypes.h>

class CUIElement;

// This class is responsible for taking the outputted data of a fired gesture event and making it into a message
// that can be understood and processed by XAML
class GestureOutputMapper
{
public:

    struct InertiaParameters;

    GestureOutputMapper();
    void Reset();

    bool IsInteractionStopping() const { return m_isInteractionStopping; }
    bool IsInteractionStopped() const { return m_isInteractionStopped; }

    void ResetProcessingOutput() { m_processingOutput = false; }
    bool IsProcessingOutput() const { return m_processingOutput; }

    bool IsManipulationStarting() const { return m_manipulationStarting; }
    bool IsManipulationStarted() const { return m_manipulationStarted; }

    bool IsInertial() const { return m_inertiaStarted; }

    void SetManipulationStarting(bool value) { m_manipulationStarting = value; }
    void SetManipulationStarted(bool value) { m_manipulationStarted = value; }

    void SetInteractionStopping(bool value) { m_isInteractionStopping = value; }
    void SetInteractionStopped(bool value) { m_isInteractionStopped = value; }

    void SetPointerInputType(XPointerInputType type) { m_pointerInputType = type; }

    void ResetInteraction();

    TouchInteractionMsg OnTapped(
        _In_ const wf::Point& position,
        _In_ UINT32 tapCount);
    TouchInteractionMsg OnSecondaryTapped(
        _In_ const wf::Point& position);
    TouchInteractionMsg OnHolding(
        _In_ const wf::Point& position,
        ixp::HoldingState holdingState);
    TouchInteractionMsg OnManipulationStarted(
        _In_ const wf::Point& position,
        _In_ const ixp::ManipulationDelta& cumulativeDelta,
        const float rasterizationScale = 1.0f);
    TouchInteractionMsg OnManipulationInertiaStarting(
        _In_ const wf::Point& position,
        _In_ const ixp::ManipulationDelta& delta,
        _In_ const ixp::ManipulationDelta& cumulativeDelta,
        _In_ const ixp::ManipulationVelocities& velocities,
        const float rasterizationScale = 1.0f);
    TouchInteractionMsg OnManipulationUpdated(
        _In_ const wf::Point& position,
        _In_ const ixp::ManipulationDelta& delta,
        _In_ const ixp::ManipulationDelta& cumulativeDelta,
        _In_ const ixp::ManipulationVelocities& velocities,
        const float rasterizationScale = 1.0f);
    TouchInteractionMsg OnManipulationCompleted(
        _In_ const wf::Point& position,
        _In_ DirectUI::ManipulationModes manipulationMode,
        _In_ const ixp::ManipulationDelta& cumulativeDelta,
        _In_ const ixp::ManipulationVelocities& velocities,
        _Out_ bool* interactionStopping,
        const float rasterizationScale = 1.0f);

    InertiaParameters GetInertiaParametersFromMsg(_In_ const TouchInteractionMsg& msg);

    struct InertiaParameters
    {
        float m_translationDeceleration = 0;
        float m_translationDisplacement = 0;
        float m_rotationDeceleration = 0;   // degrees
        float m_rotationAngle = 0;          // degrees
        float m_expansionDeceleration = 0;
        float m_expansionExpansion = 0;
    };

    bool ShouldProcessManipulationEvents() const { return !IsInteractionStopped() || IsInteractionStopping(); }

private:
    void OnTapped(_Inout_ TouchInteractionMsg& msg, _In_ const wf::Point& position);

    void SetStandardFields(_Inout_ TouchInteractionMsg& msg, _In_ const wf::Point& position) const;

    bool m_manipulationStarting : 1;
    bool m_isInteractionStopping : 1;
    bool m_isInteractionStopped : 1;
    bool m_processingOutput : 1;
    bool m_manipulationStarted : 1;
    bool m_inertiaStarted : 1;

    XPointerInputType m_pointerInputType = XcpPointerInputTypeTouch;
};
