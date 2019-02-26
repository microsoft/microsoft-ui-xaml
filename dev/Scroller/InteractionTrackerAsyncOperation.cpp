// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "InteractionTrackerAsyncOperation.h"

InteractionTrackerAsyncOperation::InteractionTrackerAsyncOperation(
    InteractionTrackerAsyncOperationType operationType,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    bool isDelayed,
    std::shared_ptr<ViewChangeBase> viewChangeBase)
    : m_operationType(operationType)
    , m_operationTrigger(operationTrigger)
    , m_isDelayed(isDelayed)
    , m_viewChangeBase(viewChangeBase)
{
    SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](operationType: %s, operationTrigger: %s, isDelayed: %d, viewChange: 0x%p)\n",
        METH_NAME, this, TypeLogging::InteractionTrackerAsyncOperationTypeToString(operationType).c_str(),
        TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str(), isDelayed, viewChangeBase);

    if ((operationType == InteractionTrackerAsyncOperationType::TryUpdatePosition ||
         operationType == InteractionTrackerAsyncOperationType::TryUpdatePositionBy) &&
        SharedHelpers::IsRS5OrHigher())
    {
        // Starting with RS5, the number of UI thread ticks elapsed before a queued operation gets processed for TryUpdatePosition
        // or TryUpdatePositionBy is reduced to the minimum value 1 because of the use of the InteractionTrackerClampingOption::Disabled value.
        // This maintains the asynchronous aspect of the view change requests for behavior and API compatibility reasons.
        m_preProcessingTicksCountdown = m_queuedOperationTicks = 1;
    }
    else
    {
        // Number of UI thread ticks elapsed before a queued operation gets processed to allow any pending size
        // changes to be propagated to the InteractionTracker.
        m_preProcessingTicksCountdown = m_queuedOperationTicks = c_queuedOperationTicks;
    }

    if (!IsAnimated())
    {
        m_postProcessingTicksCountdown = c_maxNonAnimatedOperationTicks;
    }
}

InteractionTrackerAsyncOperation::~InteractionTrackerAsyncOperation()
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}
