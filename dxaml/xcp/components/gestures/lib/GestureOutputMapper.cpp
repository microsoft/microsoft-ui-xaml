// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "GestureOutputMapper.h"
#include "GestureConfigurationBuilder.h"
#include <xcpmath.h>

GestureOutputMapper::GestureOutputMapper() 
{ 
    Reset();
}

void GestureOutputMapper::Reset()
{
    m_manipulationStarting = false;
    m_isInteractionStopping = false;
    m_isInteractionStopped = false;
    m_processingOutput = false;
    m_manipulationStarted = false;
    m_inertiaStarted = false;

    m_pointerInputType = XcpPointerInputTypeTouch;
}

void GestureOutputMapper::SetStandardFields(_Inout_ TouchInteractionMsg& msg, _In_ const wf::Point& position) const
{
    msg.m_pointerInputType = m_pointerInputType;
    msg.m_pointInteraction.x = position.X;
    msg.m_pointInteraction.y = position.Y;
}

ManipulationTransform PackManipulationTransform(const float rasterizationScale, _In_ const ixp::ManipulationDelta& delta)
{
    ManipulationTransform manipTransform;

    // GestureOutputMapper is used to wrap values from manipulation event args in Xaml's own data structures before
    // injecting them back into CCoreServices/CInputServices::ProcessTouchInteractionCallback. ProcessTouchInteractionCallback
    // expects values in world space and will divide out the rasterization scale. The values returned in the manipulation event 
    // args already have the rasterization scale divided out, so we'll be double-counting the rasterization scale if we inject 
    // the point right back into ProcessTouchInteractionCallback. So we multiply in the rasterization scale before injecting the points.
    manipTransform.m_pointTranslation.x = delta.Translation.X * rasterizationScale;
    manipTransform.m_pointTranslation.y = delta.Translation.Y * rasterizationScale;
    
    manipTransform.m_floatScale = delta.Scale; // Don't scale m_floatScale. It represents the change in scale.
    manipTransform.m_floatExpansion = delta.Expansion;
    manipTransform.m_floatRotation = delta.Rotation;

    return manipTransform;
}

ManipulationVelocity PackManipulationVelocity(_In_ const ixp::ManipulationVelocities& velocities)
{
    ManipulationVelocity manipVelocities;

    manipVelocities.m_pointLinear.x = velocities.Linear.X;
    manipVelocities.m_pointLinear.y = velocities.Linear.Y;
    manipVelocities.m_floatExpansion = velocities.Expansion;
    manipVelocities.m_floatAngular = velocities.Angular;

    return manipVelocities;
}

void GestureOutputMapper::ResetInteraction()
{
    SetInteractionStopping(false);
    SetInteractionStopped(false);
}

TouchInteractionMsg GestureOutputMapper::OnTapped(_In_ const wf::Point& position, _In_ UINT32 tapCount)
{
    TouchInteractionMsg msg;

    if (IsInteractionStopping() && IsInteractionStopped()) 
    {
        ResetInteraction();
        return msg;
    }

    m_processingOutput = true;
    SetStandardFields(msg, position);

    if (tapCount == 1) { msg.m_msgID = XCP_GESTURETAP; }
    else if (tapCount == 2) { msg.m_msgID = XCP_GESTUREDOUBLETAP; }

    SetManipulationStarting(false);
    
    OnTapped(msg, position);

    return msg;
}

TouchInteractionMsg GestureOutputMapper::OnSecondaryTapped(_In_ const wf::Point& position)
{
    TouchInteractionMsg msg;
    msg.m_msgID = XCP_GESTURERIGHTTAP;

    if (IsInteractionStopping() && IsInteractionStopped())
    {
        ResetInteraction();
        return msg;
    }

    OnTapped(msg, position);

    return msg;
}

void GestureOutputMapper::OnTapped(_Inout_ TouchInteractionMsg& msg, _In_ const wf::Point& position)
{
    m_processingOutput = true;
    SetStandardFields(msg, position);

    SetManipulationStarting(false);

    ResetInteraction();
}


TouchInteractionMsg GestureOutputMapper::OnHolding(_In_ const wf::Point& position, ixp::HoldingState holdingState)
{
    TouchInteractionMsg msg;
    const bool interactionEnd = holdingState != ixp::HoldingState_Started;

    if (IsInteractionStopping() && IsInteractionStopped()) 
    {
        if(interactionEnd) { ResetInteraction(); }
        return msg;
    }

    m_processingOutput = true;
    SetStandardFields(msg, position);

    msg.m_msgID = XCP_GESTUREHOLD;

    switch (holdingState)
    {
        case ixp::HoldingState_Started:
            msg.m_holdingState = XcpHoldingStateStarted;
            break;
        case ixp::HoldingState_Completed:
            msg.m_holdingState = XcpHoldingStateCompleted;
            break;
        case ixp::HoldingState_Canceled:
            msg.m_holdingState = XcpHoldingStateCanceled;
            break;
    }

    if (interactionEnd) 
    { 
        SetManipulationStarting(false); 
        ResetInteraction();
    }

    return msg;
}

TouchInteractionMsg GestureOutputMapper::OnManipulationStarted(
    _In_ const wf::Point& position,
    _In_ const ixp::ManipulationDelta& cumulativeDelta,
    const float rasterizationScale)
{
    TouchInteractionMsg msg;

    if (IsInteractionStopping() && IsInteractionStopped()) { return msg; }

    m_processingOutput = true;
    SetStandardFields(msg, position);

    msg.m_msgID = XCP_MANIPULATIONSTARTED;
    msg.m_cumulative = PackManipulationTransform(rasterizationScale, cumulativeDelta);

    msg.m_bInertial = false;
    m_inertiaStarted = false;

    SetManipulationStarted(true);

    return msg;
}

GestureOutputMapper::InertiaParameters GestureOutputMapper::GetInertiaParametersFromMsg(_In_ const TouchInteractionMsg& msg)
{
    GestureOutputMapper::InertiaParameters params;

    if (!_isnan(msg.m_fInertiaTranslationDeceleration) && msg.m_fInertiaTranslationDeceleration > 0) { params.m_translationDeceleration = msg.m_fInertiaTranslationDeceleration; }
    if (!_isnan(msg.m_fInertiaTranslationDisplacement) && msg.m_fInertiaTranslationDisplacement > 0) { params.m_translationDisplacement = msg.m_fInertiaTranslationDisplacement; }
    if (!_isnan(msg.m_fInertiaRotationDeceleration) && msg.m_fInertiaRotationDeceleration > 0) { params.m_rotationDeceleration = msg.m_fInertiaRotationDeceleration; }
    if (!_isnan(msg.m_fInertiaRotationAngle) && msg.m_fInertiaRotationAngle > 0) { params.m_rotationAngle = msg.m_fInertiaRotationAngle; }
    if (!_isnan(msg.m_fInertiaExpansionDeceleration) && msg.m_fInertiaExpansionDeceleration > 0) { params.m_expansionDeceleration = msg.m_fInertiaExpansionDeceleration; }
    if (!_isnan(msg.m_fInertiaExpansionExpansion) && msg.m_fInertiaExpansionExpansion > 0) { params.m_expansionExpansion = msg.m_fInertiaExpansionExpansion; }

    return params;
}

TouchInteractionMsg GestureOutputMapper::OnManipulationInertiaStarting(
    _In_ const wf::Point& position,
    _In_ const ixp::ManipulationDelta& delta,
    _In_ const ixp::ManipulationDelta& cumulativeDelta,
    _In_ const ixp::ManipulationVelocities& velocities,
    const float rasterizationScale)
{
    TouchInteractionMsg msg;

    m_processingOutput = true;
    SetStandardFields(msg, position);

    msg.m_delta = PackManipulationTransform(rasterizationScale, delta);
    msg.m_cumulative = PackManipulationTransform(rasterizationScale, cumulativeDelta);
    msg.m_velocity = PackManipulationVelocity(velocities);

    if (IsInertial() == false)
    {
        msg.m_msgID = XCP_MANIPULATIONINERTIASTARTING;

        msg.m_fInertiaTranslationDeceleration = static_cast<XFLOAT>(XDOUBLE_NAN);
        msg.m_fInertiaTranslationDisplacement = static_cast<XFLOAT>(XDOUBLE_NAN);
        msg.m_fInertiaRotationDeceleration = static_cast<XFLOAT>(XDOUBLE_NAN);
        msg.m_fInertiaRotationAngle = static_cast<XFLOAT>(XDOUBLE_NAN);
        msg.m_fInertiaExpansionDeceleration = static_cast<XFLOAT>(XDOUBLE_NAN);
        msg.m_fInertiaExpansionExpansion = static_cast<XFLOAT>(XDOUBLE_NAN);

        msg.m_bInertial = true;
        m_inertiaStarted = true;
    }

    return msg;
}

TouchInteractionMsg GestureOutputMapper::OnManipulationUpdated(
    _In_ const wf::Point& position,
    _In_ const ixp::ManipulationDelta& delta,
    _In_ const ixp::ManipulationDelta& cumulativeDelta,
    _In_ const ixp::ManipulationVelocities& velocities,
    const float rasterizationScale)
{
    TouchInteractionMsg msg;

    if (IsInteractionStopping() && IsInteractionStopped()) { return msg; }

    m_processingOutput = true;
    SetStandardFields(msg, position);

    msg.m_msgID = XCP_MANIPULATIONDELTA;
    msg.m_delta = PackManipulationTransform(rasterizationScale, delta);
    msg.m_cumulative = PackManipulationTransform(rasterizationScale, cumulativeDelta);
    msg.m_velocity = PackManipulationVelocity(velocities);
    msg.m_bInertial = m_inertiaStarted;

    return msg;
}

TouchInteractionMsg GestureOutputMapper::OnManipulationCompleted(
    _In_ const wf::Point& position,
    _In_ DirectUI::ManipulationModes manipulationMode,
    _In_ const ixp::ManipulationDelta& cumulativeDelta,
    _In_ const ixp::ManipulationVelocities& velocities,
    _Out_ bool* interactionStopping,
    const float rasterizationScale)
{
    TouchInteractionMsg msg;

    if (manipulationMode == DirectUI::ManipulationModes::None || (IsInteractionStopping() && IsInteractionStopped()))
    {
        ResetInteraction();
        return msg;
    }

    m_processingOutput = true;
    SetStandardFields(msg, position);

    msg.m_msgID = XCP_MANIPULATIONCOMPLETED;
    msg.m_cumulative = PackManipulationTransform(rasterizationScale, cumulativeDelta);
    msg.m_velocity = PackManipulationVelocity(velocities);

    m_inertiaStarted = false;

    SetManipulationStarting(false);
    SetManipulationStarted(false);

    // We want to store the value of interactionStopping BEFORE it is reset to false
    *interactionStopping = IsInteractionStopping();

    return msg;
}
