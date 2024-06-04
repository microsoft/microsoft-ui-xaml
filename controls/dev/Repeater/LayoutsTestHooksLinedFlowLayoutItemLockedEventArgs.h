// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs.g.h"

class LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs :
    public winrt::implementation::LayoutsTestHooksLinedFlowLayoutItemLockedEventArgsT<LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs>
{
public:
    LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs(
        int itemIndex,
        int lineIndex);

    // ILayoutsTestHooksLinedFlowLayoutItemLockedEventArgs overrides
    int ItemIndex();
    int LineIndex();

private:
    int m_itemIndex;
    int m_lineIndex;
};
