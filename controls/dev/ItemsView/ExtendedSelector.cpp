// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ExtendedSelector.h"

ExtendedSelector::ExtendedSelector()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

ExtendedSelector::~ExtendedSelector()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void ExtendedSelector::OnInteractedAction(winrt::IndexPath const &index, bool ctrl, bool shift)
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, ctrl, shift);

    auto selectionModel = GetSelectionModel();
    if (shift)
    {
        auto anchorIndex = selectionModel.AnchorIndex();
        if (anchorIndex)
        {
            selectionModel.ClearSelection();
            selectionModel.AnchorIndex(anchorIndex);
            selectionModel.SelectRangeFromAnchorTo(index);
        }
    }
    else if (ctrl)
    {
        if (IsSelected(index))
        {
            selectionModel.DeselectAt(index);
        }
        else
        {
            selectionModel.SelectAt(index);
        }
    }
    else
    {
        // Only clear selection if interacting with a different item.
        if (!IsSelected(index))
        {
            selectionModel.ClearSelection();
            selectionModel.SelectAt(index);
        }
    }
}

void ExtendedSelector::OnFocusedAction(winrt::IndexPath const &index, bool ctrl, bool shift)
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, ctrl, shift);

    auto selectionModel = GetSelectionModel();

    if (shift && ctrl)
    {
        if (selectionModel.AnchorIndex())
        {
            selectionModel.SelectRangeFromAnchorTo(index);
        }
    }
    else if (shift)
    {
        auto anchorIndex = selectionModel.AnchorIndex();
        if (anchorIndex)
        {
            selectionModel.ClearSelection();
            selectionModel.AnchorIndex(anchorIndex);
            selectionModel.SelectRangeFromAnchorTo(index);
        }
    }
    else if (!ctrl)
    {
        selectionModel.ClearSelection();
        selectionModel.SelectAt(index);
    }
}
