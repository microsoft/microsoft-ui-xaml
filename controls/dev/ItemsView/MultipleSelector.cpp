// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MultipleSelector.h"

MultipleSelector::MultipleSelector()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

MultipleSelector::~MultipleSelector()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void MultipleSelector::OnInteractedAction(winrt::IndexPath const &index, bool ctrl, bool shift)
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, ctrl, shift);

    auto selectionModel = GetSelectionModel();
    if (shift)
    {
        auto anchorIndex = selectionModel.AnchorIndex();
        if (anchorIndex)
        {
            winrt::IReference<bool> isAnchorSelectedNullable = selectionModel.IsSelectedAt(anchorIndex);
            bool isAnchorSelected = false;
            if (isAnchorSelectedNullable != nullptr)
            {
                isAnchorSelected = isAnchorSelectedNullable.Value();
            }

            winrt::IReference<bool> isIndexSelectedNullable = selectionModel.IsSelectedAt(index);
            bool isIndexSelected = false;
            if (isIndexSelectedNullable != nullptr)
            {
                isIndexSelected = isIndexSelectedNullable.Value();
            }

            if (isAnchorSelected != isIndexSelected)
            {
                if (isAnchorSelected)
                {
                    selectionModel.SelectRangeFromAnchorTo(index);
                }
                else
                {
                    selectionModel.DeselectRangeFromAnchorTo(index);
                }
            }
        }
    }
    else if (IsSelected(index))
    {
        selectionModel.DeselectAt(index);
    }
    else
    {
        selectionModel.SelectAt(index);
    }
}

void MultipleSelector::OnFocusedAction(winrt::IndexPath const &index, bool ctrl, bool shift)
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, ctrl, shift);

    if (shift)
    {
        auto selectionModel = GetSelectionModel();
        auto anchorIndex = selectionModel.AnchorIndex();
        if (anchorIndex)
        {
            winrt::IReference<bool> isAnchorSelectedNullable = selectionModel.IsSelectedAt(anchorIndex);
            bool isAnchorSelected = false;
            if (isAnchorSelectedNullable != nullptr)
            {
                isAnchorSelected = isAnchorSelectedNullable.Value();
            }

            if (isAnchorSelected)
            {
                selectionModel.SelectRangeFromAnchorTo(index);
            }
            else
            {
                selectionModel.DeselectRangeFromAnchorTo(index);
            }
        }
    }
}
