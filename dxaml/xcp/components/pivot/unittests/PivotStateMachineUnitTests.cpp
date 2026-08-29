// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PivotStateMachineUnitTests.h"
#include "StateMachineCallbackImplementor.h"
#include <XamlLogging.h>
#include <PivotStateMachine.h>

using namespace xaml_controls;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Pivot {

void PivotStateMachineUnitTests::ValidateInitialization()
{
    StateMachineCallbackImplementor callbackImplementor;
    PivotStateMachine psm(&callbackImplementor);
    VERIFY_ARE_EQUAL(psm.GetPivotState(), PivotStateMachine::PivotState::PivotState_Uninitialized);

    LOG_OUTPUT(L"Setting up PivotStateMachine");
    VERIFY_SUCCEEDED(psm.Initialize(0, 0, FALSE));
    VERIFY_SUCCEEDED(psm.ApplyTemplateEvent(TRUE));
    VERIFY_ARE_EQUAL(psm.GetPivotState(), PivotStateMachine::PivotState::PivotState_MeasurePending);

    LOG_OUTPUT(L"Firing Measure/Arrange events");
    wf::Size measureSize = {};
    measureSize.Height = 200.0;
    measureSize.Width = 100.0;

    VERIFY_SUCCEEDED(psm.MeasureEvent(measureSize));
    VERIFY_IS_TRUE(callbackImplementor.m_setPivotSectionWidthCalled);
    VERIFY_ARE_EQUAL(callbackImplementor.m_sectionWidth, 100.0);
    VERIFY_ARE_EQUAL(psm.GetPivotState(), PivotStateMachine::PivotState::PivotState_ArrangePending);
    callbackImplementor.m_setPivotSectionWidthCalled = false;


    VERIFY_SUCCEEDED(psm.ArrangeEvent(measureSize));
    VERIFY_IS_TRUE(callbackImplementor.m_setPivotOffsetCalled);
    VERIFY_IS_TRUE(callbackImplementor.m_setViewportOffsetCalled);
    VERIFY_IS_FALSE(callbackImplementor.m_setSelectedIndexCalled);
    VERIFY_ARE_EQUAL(psm.GetPivotState(), PivotStateMachine::PivotState::PivotState_Idle);

    VERIFY_ARE_EQUAL(callbackImplementor.m_sectionOffset, callbackImplementor.m_viewportOffset);
}

void PivotStateMachineUnitTests::ValidateCallingMeasureAndArrangeWithUnequalValues()
{
    // Validates the situation when Measure and Arrange are called with values that are not equal.
    // There is no guarantee that the values passed to Measure and Arrange will be equal, so the PivotStateMachine
    // should not depend on that.

    StateMachineCallbackImplementor callbackImplementor;
    PivotStateMachine psm(&callbackImplementor);
    VERIFY_ARE_EQUAL(psm.GetPivotState(), PivotStateMachine::PivotState::PivotState_Uninitialized);

    LOG_OUTPUT(L"Setting up PivotStateMachine");
    VERIFY_SUCCEEDED(psm.Initialize(0, 2, FALSE));
    VERIFY_SUCCEEDED(psm.ApplyTemplateEvent(TRUE));

    LOG_OUTPUT(L"Firing Measure/Arrange events");
    wf::Size measureSize = {100, 200};
    wf::Size arrangeSize = {110, 200};
    VERIFY_SUCCEEDED(psm.MeasureEvent(measureSize));
    VERIFY_ARE_EQUAL(psm.GetPivotState(), PivotStateMachine::PivotState::PivotState_ArrangePending);

    VERIFY_SUCCEEDED(psm.ArrangeEvent(arrangeSize));
    VERIFY_ARE_EQUAL(psm.GetPivotState(), PivotStateMachine::PivotState::PivotState_Idle);

    VERIFY_SUCCEEDED(psm.MeasureEvent(measureSize));
    VERIFY_ARE_EQUAL(psm.GetPivotState(), PivotStateMachine::PivotState::PivotState_Idle);
}

} } } } } }