// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs.g.h"

class LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs :
    public winrt::implementation::LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgsT<LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs>
{
public:
    LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs(
        winrt::LinedFlowLayoutInvalidationTrigger const& invalidationTrigger);

    // ILayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs overrides
    winrt::LinedFlowLayoutInvalidationTrigger InvalidationTrigger();

private:
    winrt::LinedFlowLayoutInvalidationTrigger m_invalidationTrigger;
};
