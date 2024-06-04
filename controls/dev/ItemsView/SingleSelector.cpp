// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SingleSelector.h"

SingleSelector::SingleSelector()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

SingleSelector::~SingleSelector()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void SingleSelector::FollowFocus(bool followFocus)
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, followFocus);

    m_followFocus = followFocus;
}

void SingleSelector::OnInteractedAction(winrt::IndexPath const& index, bool ctrl, bool shift)
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, ctrl, shift);

    auto selectionModel = GetSelectionModel();
    selectionModel.SingleSelect(true);

    if (!ctrl)
    {
        selectionModel.SelectAt(index);
    }
    else if (!IsSelected(index))
    {
        selectionModel.SelectAt(index);
    }
    else
    {
        selectionModel.DeselectAt(index);
    }
}

void SingleSelector::OnFocusedAction(winrt::IndexPath const& index, bool ctrl, bool shift)
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, ctrl, shift);

    auto selectionModel = GetSelectionModel();
    selectionModel.SingleSelect(true);

    if (!ctrl && m_followFocus)
    {
        selectionModel.SelectAt(index);
    }
}
