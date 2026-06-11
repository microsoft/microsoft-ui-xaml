// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlLogging.h>

#include "GestureOutputMapper.h"
#include <GestureOutputMapperUnitTests.h>

#include <xcpmath.h>

using namespace Microsoft::WRL;
using namespace WEX::TestExecution;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Gestures {

void GestureOutputMapperUnitTests::VerifyTap()
{
    GestureOutputMapper mapper;
    const wf::Point position = { 50, 65 };

    TouchInteractionMsg msg = mapper.OnTapped(position, 0);
    VERIFY_IS_TRUE(msg.m_msgID == XCP_NULL);

    msg = mapper.OnTapped(position, 1);
    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);
    VERIFY_IS_TRUE(msg.m_msgID == XCP_GESTURETAP);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.x, position.X);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.y, position.Y);

    msg = mapper.OnTapped(position, 2);
    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);
    VERIFY_IS_TRUE(msg.m_msgID == XCP_GESTUREDOUBLETAP);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.x, position.X);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.y, position.Y);

    VERIFY_IS_FALSE(mapper.IsManipulationStarting());
    VERIFY_IS_FALSE(mapper.IsInteractionStopping());
    VERIFY_IS_FALSE(mapper.IsInteractionStopped());
}

void GestureOutputMapperUnitTests::VerifySecondaryTap()
{
    GestureOutputMapper mapper;
    const wf::Point position = { 50, 65 };

    TouchInteractionMsg msg = mapper.OnSecondaryTapped(position);

    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);
    VERIFY_IS_TRUE(msg.m_msgID == XCP_GESTURERIGHTTAP);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.x, position.X);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.y, position.Y);
}

void GestureOutputMapperUnitTests::VerifyPointerInputTypeReflectedInMessage()
{
    GestureOutputMapper mapper;

    TouchInteractionMsg msg = mapper.OnTapped({ 50, 65 }, 1);
    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);

    mapper.SetPointerInputType(XcpPointerInputTypePen);

    msg = mapper.OnTapped({ 50, 65 }, 1);
    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypePen);
}

void GestureOutputMapperUnitTests::VerifyHolding()
{
    GestureOutputMapper mapper;
    const wf::Point position = { 50, 65 };

    TouchInteractionMsg msg = mapper.OnHolding(position, ixp::HoldingState_Started);

    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);
    VERIFY_ARE_EQUAL(msg.m_msgID, XCP_GESTUREHOLD);
    VERIFY_IS_TRUE(msg.m_holdingState == XcpHoldingStateStarted);

    VERIFY_ARE_EQUAL(msg.m_pointInteraction.x, position.X);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.y, position.Y);
}

void GestureOutputMapperUnitTests::VerifyHoldingStateStarted()
{
    GestureOutputMapper mapper;

    mapper.SetManipulationStarting(true);
    mapper.SetInteractionStopping(true);
    mapper.SetInteractionStopped(true);

    VERIFY_IS_TRUE(mapper.IsManipulationStarting());

    TouchInteractionMsg msg = mapper.OnHolding({ 100, 200 }, ixp::HoldingState_Started);

    VERIFY_ARE_EQUAL(msg.m_holdingState, XcpHoldingStateStarted);

    // HoldingState_Started sholuld not change any manipulation data
    VERIFY_IS_TRUE(mapper.IsManipulationStarting());
    VERIFY_IS_TRUE(mapper.IsInteractionStopping());
    VERIFY_IS_TRUE(mapper.IsInteractionStopped());
}

void GestureOutputMapperUnitTests::VerifyHoldingStateCompleted()
{
    GestureOutputMapper mapper;

    mapper.SetManipulationStarting(true);
    mapper.SetInteractionStopping(true);

    VERIFY_IS_TRUE(mapper.IsManipulationStarting());

    TouchInteractionMsg msg = mapper.OnHolding({ 100, 200 }, ixp::HoldingState_Completed);

    VERIFY_ARE_EQUAL(msg.m_holdingState, XcpHoldingStateCompleted);

    VERIFY_IS_FALSE(mapper.IsManipulationStarting());
    VERIFY_IS_FALSE(mapper.IsInteractionStopping());
    VERIFY_IS_FALSE(mapper.IsInteractionStopped());
}

void GestureOutputMapperUnitTests::VerifyHoldingStateCanceled()
{
    GestureOutputMapper mapper;

    mapper.SetManipulationStarting(true);
    mapper.SetInteractionStopping(true);

    VERIFY_IS_TRUE(mapper.IsManipulationStarting());

    TouchInteractionMsg msg = mapper.OnHolding({ 100, 200 }, ixp::HoldingState_Canceled);

    VERIFY_ARE_EQUAL(msg.m_holdingState, XcpHoldingStateCanceled);

    VERIFY_IS_FALSE(mapper.IsManipulationStarting());
    VERIFY_IS_FALSE(mapper.IsInteractionStopping());
    VERIFY_IS_FALSE(mapper.IsInteractionStopped());
}

void GestureOutputMapperUnitTests::VerifySendManipulationStartingEventAfterGesture()
{
    GestureOutputMapper mapper;

    mapper.SetManipulationStarting(true);
    VERIFY_IS_TRUE(mapper.IsManipulationStarting());

    mapper.OnTapped({ 100, 200 }, 1);
    VERIFY_IS_FALSE(mapper.IsManipulationStarting()); // This should be reset on versions > 6.3

    mapper.SetManipulationStarting(true);
    mapper.OnHolding({ 100, 200 }, ixp::HoldingState_Canceled);
    VERIFY_IS_FALSE(mapper.IsManipulationStarting()); // This should be reset on versions > 6.3

    mapper.SetManipulationStarting(true);
    mapper.OnHolding({ 100, 200 }, ixp::HoldingState_Started);
    VERIFY_IS_TRUE(mapper.IsManipulationStarting()); // We don't modify the value if the interaction has not ended
}

void GestureOutputMapperUnitTests::VerifyCallbackNotCalled()
{
    GestureOutputMapper mapper;

    ixp::ManipulationDelta delta;
    ixp::ManipulationDelta cumulativeDelta;
    ixp::ManipulationVelocities velocities;

    mapper.SetInteractionStopping(true);
    mapper.SetInteractionStopped(true);

    TouchInteractionMsg msg = mapper.OnTapped({ 100, 200 }, 1);
    VERIFY_ARE_EQUAL(msg.m_msgID, XCP_NULL);
    VERIFY_IS_FALSE(mapper.IsInteractionStopping());
    VERIFY_IS_FALSE(mapper.IsInteractionStopped());

    mapper.SetInteractionStopping(true);
    mapper.SetInteractionStopped(true);

    msg = mapper.OnHolding({ 100, 200 }, ixp::HoldingState_Canceled);
    VERIFY_ARE_EQUAL(msg.m_msgID, XCP_NULL);
    VERIFY_IS_FALSE(mapper.IsInteractionStopping());
    VERIFY_IS_FALSE(mapper.IsInteractionStopped());

    mapper.SetInteractionStopping(true);
    mapper.SetInteractionStopped(true);

    msg = mapper.OnHolding({ 100, 200 }, ixp::HoldingState_Started);
    VERIFY_ARE_EQUAL(msg.m_msgID, XCP_NULL);
    VERIFY_IS_TRUE(mapper.IsInteractionStopping());
    VERIFY_IS_TRUE(mapper.IsInteractionStopped());

    mapper.SetInteractionStopping(true);
    mapper.SetInteractionStopped(true);

    msg = mapper.OnManipulationStarted({ 100, 200 }, cumulativeDelta);
    VERIFY_ARE_EQUAL(msg.m_msgID, XCP_NULL);
    VERIFY_IS_TRUE(mapper.IsInteractionStopping());
    VERIFY_IS_TRUE(mapper.IsInteractionStopped());

    mapper.SetInteractionStopping(true);
    mapper.SetInteractionStopped(true);

    bool interactionStopping = false;
    msg = mapper.OnManipulationCompleted({ 100, 200 }, DirectUI::ManipulationModes::Rotate, cumulativeDelta, velocities, &interactionStopping);
    VERIFY_ARE_EQUAL(msg.m_msgID, XCP_NULL);
    VERIFY_IS_FALSE(mapper.IsInteractionStopping());
    VERIFY_IS_FALSE(mapper.IsInteractionStopped());

    mapper.SetInteractionStopping(false);
    mapper.SetInteractionStopped(false);

    msg = mapper.OnManipulationCompleted({ 100, 200 }, DirectUI::ManipulationModes::None, cumulativeDelta, velocities, &interactionStopping);
    VERIFY_ARE_EQUAL(msg.m_msgID, XCP_NULL);
    VERIFY_IS_FALSE(mapper.IsInteractionStopping());
    VERIFY_IS_FALSE(mapper.IsInteractionStopped());

    msg = mapper.OnManipulationInertiaStarting({ 100, 200 }, delta, cumulativeDelta, velocities);
    msg = mapper.OnManipulationInertiaStarting({ 100, 200 }, delta, cumulativeDelta, velocities);
    VERIFY_ARE_EQUAL(msg.m_msgID, XCP_NULL);
}

void VerifyManipulationDelta(_In_ const ixp::ManipulationDelta& delta, _In_ const ManipulationTransform& transform)
{
    VERIFY_ARE_EQUAL(transform.m_pointTranslation.x, delta.Translation.X);
    VERIFY_ARE_EQUAL(transform.m_pointTranslation.y, delta.Translation.Y);
    VERIFY_ARE_EQUAL(transform.m_floatScale, delta.Scale);
    VERIFY_ARE_EQUAL(transform.m_floatExpansion, delta.Expansion);
    VERIFY_ARE_EQUAL(transform.m_floatRotation, delta.Rotation);
}

void VerifyManipulationVelocities(_In_ const ixp::ManipulationVelocities& velocities, _In_ const ManipulationVelocity& manipVelocities)
{
    VERIFY_ARE_EQUAL(manipVelocities.m_pointLinear.x, velocities.Linear.X);
    VERIFY_ARE_EQUAL(manipVelocities.m_pointLinear.y, velocities.Linear.Y);
    VERIFY_ARE_EQUAL(manipVelocities.m_floatExpansion, velocities.Expansion);
    VERIFY_ARE_EQUAL(manipVelocities.m_floatAngular, velocities.Angular);
}

void GestureOutputMapperUnitTests::VerifyManipulationStarted()
{
    const wf::Point position = { 200, 100 };

    ixp::ManipulationDelta cumulativeDelta;
    cumulativeDelta.Translation.X = 100;
    cumulativeDelta.Translation.Y = 50;
    cumulativeDelta.Scale = 1.25;
    cumulativeDelta.Expansion = 50;
    cumulativeDelta.Rotation = 90;

    const float rasterizationScale = 2.0f;

    ixp::ManipulationDelta expected = cumulativeDelta;

    expected.Translation.X *= rasterizationScale;
    expected.Translation.Y *= rasterizationScale;

    GestureOutputMapper mapper;
    TouchInteractionMsg msg = mapper.OnManipulationStarted(position, cumulativeDelta, rasterizationScale);

    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);

    VERIFY_IS_TRUE(msg.m_msgID == XCP_MANIPULATIONSTARTED);

    VERIFY_ARE_EQUAL(msg.m_pointInteraction.x, position.X);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.y, position.Y);

    VERIFY_IS_FALSE(msg.m_bInertial);

    VerifyManipulationDelta(expected, msg.m_cumulative);

    VERIFY_IS_TRUE(mapper.IsManipulationStarted());
    VERIFY_IS_FALSE(mapper.IsInertial());
}

void GestureOutputMapperUnitTests::VerifyManipulationUpdated()
{
    const wf::Point position = { 200, 100 };

    ixp::ManipulationDelta delta;
    delta.Translation.X = 25;
    delta.Translation.Y = 25;
    delta.Scale = .75;
    delta.Expansion = 25;
    delta.Rotation = 45;

    ixp::ManipulationDelta cumulativeDelta;
    cumulativeDelta.Translation.X = 100;
    cumulativeDelta.Translation.Y = 50;
    cumulativeDelta.Scale = 1.25;
    cumulativeDelta.Expansion = 50;
    cumulativeDelta.Rotation = 90;

    ixp::ManipulationVelocities velocities;
    velocities.Linear.X = 75;
    velocities.Linear.Y = 50;
    velocities.Expansion = 50;
    velocities.Angular = 270;

    const float rasterizationScale = 2.0f;

    ixp::ManipulationDelta expectedDelta = delta;
    ixp::ManipulationDelta expectedCumulativeDelta = cumulativeDelta;

    expectedDelta.Translation.X *= rasterizationScale;
    expectedDelta.Translation.Y *= rasterizationScale;
    expectedCumulativeDelta.Translation.X *= rasterizationScale;
    expectedCumulativeDelta.Translation.Y *= rasterizationScale;

    GestureOutputMapper mapper;
    TouchInteractionMsg msg = mapper.OnManipulationUpdated(position, delta, cumulativeDelta, velocities, rasterizationScale);

    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);

    VERIFY_ARE_EQUAL(msg.m_pointInteraction.x, position.X);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.y, position.Y);

    VERIFY_IS_FALSE(msg.m_bInertial);

    VERIFY_IS_TRUE(msg.m_msgID == XCP_MANIPULATIONDELTA);

    VerifyManipulationDelta(expectedDelta, msg.m_delta);
    VerifyManipulationDelta(expectedCumulativeDelta, msg.m_cumulative);
    VerifyManipulationVelocities(velocities, msg.m_velocity);
}

void GestureOutputMapperUnitTests::VerifyOnManipulationInertiaStarting()
{
    const wf::Point position = { 200, 100 };

    ixp::ManipulationDelta delta;
    delta.Translation.X = 25;
    delta.Translation.Y = 25;
    delta.Scale = .75;
    delta.Expansion = 25;
    delta.Rotation = 45;

    ixp::ManipulationDelta cumulativeDelta;
    cumulativeDelta.Translation.X = 100;
    cumulativeDelta.Translation.Y = 50;
    cumulativeDelta.Scale = 1.25;
    cumulativeDelta.Expansion = 50;
    cumulativeDelta.Rotation = 90;

    ixp::ManipulationVelocities velocities;
    velocities.Linear.X = 75;
    velocities.Linear.Y = 50;
    velocities.Expansion = 50;
    velocities.Angular = 270;

    const float rasterizationScale = 2.0f;

    ixp::ManipulationDelta expectedDelta = delta;
    ixp::ManipulationDelta expectedCumulativeDelta = cumulativeDelta;

    expectedDelta.Translation.X *= rasterizationScale;
    expectedDelta.Translation.Y *= rasterizationScale;
    expectedCumulativeDelta.Translation.X *= rasterizationScale;
    expectedCumulativeDelta.Translation.Y *= rasterizationScale;

    GestureOutputMapper mapper;
    TouchInteractionMsg msg = mapper.OnManipulationInertiaStarting(position, delta, cumulativeDelta, velocities, rasterizationScale);

    VERIFY_IS_TRUE(msg.m_msgID == XCP_MANIPULATIONINERTIASTARTING);
    VERIFY_IS_TRUE(msg.m_bInertial);

    VERIFY_ARE_NOT_EQUAL(_isnan(msg.m_fInertiaTranslationDeceleration),0);
    VERIFY_ARE_NOT_EQUAL(_isnan(msg.m_fInertiaTranslationDisplacement),0);
    VERIFY_ARE_NOT_EQUAL(_isnan(msg.m_fInertiaRotationDeceleration),0);
    VERIFY_ARE_NOT_EQUAL(_isnan(msg.m_fInertiaRotationAngle),0);
    VERIFY_ARE_NOT_EQUAL(_isnan(msg.m_fInertiaExpansionDeceleration),0);
    VERIFY_ARE_NOT_EQUAL(_isnan(msg.m_fInertiaExpansionExpansion),0);

    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);

    VERIFY_ARE_EQUAL(msg.m_pointInteraction.x, position.X);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.y, position.Y);

    VerifyManipulationDelta(expectedDelta, msg.m_delta);
    VerifyManipulationDelta(expectedCumulativeDelta, msg.m_cumulative);
    VerifyManipulationVelocities(velocities, msg.m_velocity);
}

void GestureOutputMapperUnitTests::VerifyManipulationInertiaStarting()
{
    const wf::Point position = { 200, 100 };

    ixp::ManipulationDelta delta;
    delta.Translation.X = 25;
    delta.Translation.Y = 25;
    delta.Scale = .75;
    delta.Expansion = 25;
    delta.Rotation = 45;

    ixp::ManipulationDelta cumulativeDelta;
    cumulativeDelta.Translation.X = 100;
    cumulativeDelta.Translation.Y = 50;
    cumulativeDelta.Scale = 1.25;
    cumulativeDelta.Expansion = 50;
    cumulativeDelta.Rotation = 90;

    ixp::ManipulationVelocities velocities;
    velocities.Linear.X = 75;
    velocities.Linear.Y = 50;
    velocities.Expansion = 50;
    velocities.Angular = 270;

    GestureOutputMapper::InertiaParameters inertiaParams;
    inertiaParams.m_translationDeceleration = 5;
    inertiaParams.m_translationDisplacement = 7;
    inertiaParams.m_rotationDeceleration = 90;
    inertiaParams.m_rotationAngle = 45;
    inertiaParams.m_expansionDeceleration = 15;
    inertiaParams.m_expansionExpansion = 3;

    const float rasterizationScale = 2.0f;

    ixp::ManipulationDelta expectedDelta = delta;
    ixp::ManipulationDelta expectedCumulativeDelta = cumulativeDelta;

    expectedDelta.Translation.X *= rasterizationScale;
    expectedDelta.Translation.Y *= rasterizationScale;
    expectedCumulativeDelta.Translation.X *= rasterizationScale;
    expectedCumulativeDelta.Translation.Y *= rasterizationScale;

    GestureOutputMapper mapper;

    TouchInteractionMsg intertiaMessage;
    intertiaMessage.m_fInertiaTranslationDeceleration = inertiaParams.m_translationDeceleration;
    intertiaMessage.m_fInertiaTranslationDisplacement = inertiaParams.m_translationDisplacement;
    intertiaMessage.m_fInertiaRotationDeceleration = inertiaParams.m_rotationDeceleration;
    intertiaMessage.m_fInertiaRotationAngle = inertiaParams.m_rotationAngle;
    intertiaMessage.m_fInertiaExpansionDeceleration = inertiaParams.m_expansionDeceleration;
    intertiaMessage.m_fInertiaExpansionExpansion = inertiaParams.m_expansionExpansion;

    TouchInteractionMsg msg = mapper.OnManipulationInertiaStarting(position, delta, cumulativeDelta, velocities, rasterizationScale);
    GestureOutputMapper::InertiaParameters params = mapper.GetInertiaParametersFromMsg(intertiaMessage);

    VERIFY_ARE_EQUAL(params.m_translationDeceleration, inertiaParams.m_translationDeceleration);
    VERIFY_ARE_EQUAL(params.m_translationDisplacement, inertiaParams.m_translationDisplacement);
    VERIFY_ARE_EQUAL(params.m_rotationDeceleration, inertiaParams.m_rotationDeceleration);
    VERIFY_ARE_EQUAL(params.m_rotationAngle, inertiaParams.m_rotationAngle);
    VERIFY_ARE_EQUAL(params.m_expansionDeceleration, inertiaParams.m_expansionDeceleration);
    VERIFY_ARE_EQUAL(params.m_expansionExpansion, inertiaParams.m_expansionExpansion);

    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);

    VERIFY_ARE_EQUAL(msg.m_pointInteraction.x, position.X);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.y, position.Y);

    VerifyManipulationDelta(expectedDelta, msg.m_delta);
    VerifyManipulationDelta(expectedCumulativeDelta, msg.m_cumulative);
    VerifyManipulationVelocities(velocities, msg.m_velocity);
}

void GestureOutputMapperUnitTests::VerifyManipulationInertiaStartingFiresOnce()
{
    ixp::ManipulationDelta delta;
    ixp::ManipulationDelta cumulativeDelta;
    ixp::ManipulationVelocities velocities;

    GestureOutputMapper mapper;
    TouchInteractionMsg msg = mapper.OnManipulationInertiaStarting({ 200, 100 }, delta, cumulativeDelta, velocities);

    VERIFY_IS_TRUE(msg.m_msgID == XCP_MANIPULATIONINERTIASTARTING);
    VERIFY_IS_TRUE(mapper.IsInertial());

    msg = mapper.OnManipulationInertiaStarting({ 200, 100 }, delta, cumulativeDelta, velocities);

    VERIFY_IS_TRUE(msg.m_msgID == XCP_NULL);
    VERIFY_IS_TRUE(mapper.IsInertial());
}

void GestureOutputMapperUnitTests::VerifyManipulationUpdateIsInertiaUntilManipulationComplete()
{
    ixp::ManipulationDelta delta;
    ixp::ManipulationDelta cumulativeDelta;
    ixp::ManipulationVelocities velocities;

    GestureOutputMapper mapper;
    TouchInteractionMsg msg = mapper.OnManipulationUpdated({ 200, 100 }, delta, cumulativeDelta, velocities);
    VERIFY_IS_FALSE(mapper.IsInertial());

    msg = mapper.OnManipulationInertiaStarting({ 200, 100 }, delta, cumulativeDelta, velocities);
    VERIFY_IS_TRUE(mapper.IsInertial());

    VERIFY_IS_TRUE(msg.m_msgID == XCP_MANIPULATIONINERTIASTARTING);
    VERIFY_IS_TRUE(mapper.IsInertial());

    msg = mapper.OnManipulationUpdated({ 200, 100 }, delta, cumulativeDelta, velocities);
    VERIFY_IS_TRUE(mapper.IsInertial());

    msg = mapper.OnManipulationUpdated({ 200, 100 }, delta, cumulativeDelta, velocities);
    VERIFY_IS_TRUE(mapper.IsInertial());

    bool interactionStopping = false;
    msg = mapper.OnManipulationCompleted({ 200, 100 }, DirectUI::ManipulationModes::Rotate, cumulativeDelta, velocities, &interactionStopping);

    msg = mapper.OnManipulationUpdated({ 200, 100 }, delta, cumulativeDelta, velocities);
    VERIFY_IS_FALSE(mapper.IsInertial());
}

void GestureOutputMapperUnitTests::VerifyManipulationCompleted()
{
    const wf::Point position = { 200, 100 };

    ixp::ManipulationDelta cumulativeDelta;
    cumulativeDelta.Translation.X = 100;
    cumulativeDelta.Translation.Y = 50;
    cumulativeDelta.Scale = 1.25;
    cumulativeDelta.Expansion = 50;
    cumulativeDelta.Rotation = 90;

    ixp::ManipulationVelocities velocities;
    velocities.Linear.X = 75;
    velocities.Linear.Y = 50;
    velocities.Expansion = 50;
    velocities.Angular = 270;

    GestureOutputMapper mapper;
    mapper.SetInteractionStopping(true);

    bool interactionStopping = false;
    TouchInteractionMsg msg = mapper.OnManipulationCompleted(position, DirectUI::ManipulationModes::Rotate, cumulativeDelta, velocities, &interactionStopping);

    VERIFY_IS_TRUE(interactionStopping);

    VERIFY_ARE_EQUAL(msg.m_pointerInputType, XcpPointerInputTypeTouch);

    VERIFY_ARE_EQUAL(msg.m_pointInteraction.x, position.X);
    VERIFY_ARE_EQUAL(msg.m_pointInteraction.y, position.Y);

    VERIFY_IS_TRUE(msg.m_msgID == XCP_MANIPULATIONCOMPLETED);

    VerifyManipulationDelta(cumulativeDelta, msg.m_cumulative);
    VerifyManipulationVelocities(velocities, msg.m_velocity);

    VERIFY_IS_FALSE(mapper.IsInertial());

    VERIFY_IS_FALSE(mapper.IsManipulationStarting());
    VERIFY_IS_FALSE(mapper.IsManipulationStarted());
}

} } } } }
