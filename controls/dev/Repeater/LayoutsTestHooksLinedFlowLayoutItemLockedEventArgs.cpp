// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs.h"

LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs::LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs(
    int itemIndex,
    int lineIndex)
{
    m_itemIndex = itemIndex;
    m_lineIndex = lineIndex;
}

#pragma region ILayoutsTestHooksLinedFlowLayoutItemLockedEventArgs

int LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs::ItemIndex()
{
    return m_itemIndex;
}

int LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs::LineIndex()
{
    return m_lineIndex;
}

#pragma endregion
