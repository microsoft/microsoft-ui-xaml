// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs.h"

LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs::LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs(
    winrt::LinedFlowLayoutInvalidationTrigger const& invalidationTrigger)
{
    m_invalidationTrigger = invalidationTrigger;
}

#pragma region ILayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs

winrt::LinedFlowLayoutInvalidationTrigger LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs::InvalidationTrigger()
{
    return m_invalidationTrigger;
}

#pragma endregion
