// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerTypeLogging.h"
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

    if (!IsAnimated())
    {
        m_postProcessingTicksCountdown = c_maxNonAnimatedOperationTicks;
    }
}

InteractionTrackerAsyncOperation::~InteractionTrackerAsyncOperation()
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}
