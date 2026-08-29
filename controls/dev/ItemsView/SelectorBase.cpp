// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SelectorBase.h"

SelectorBase::SelectorBase()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

SelectorBase::~SelectorBase()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void SelectorBase::SetSelectionModel(winrt::SelectionModel const& selectionModel)
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, selectionModel);

    m_selectionModel = selectionModel;
}

void SelectorBase::DeselectWithAnchorPreservation(int index)
{
    MUX_ASSERT(index != -1);

    if (m_selectionModel != nullptr)
    {
        auto const& anchorIndexPath = m_selectionModel.AnchorIndex();

        MUX_ASSERT(anchorIndexPath == nullptr || anchorIndexPath.GetSize() == 1);

        const int anchorIndex = anchorIndexPath == nullptr ? -1 : anchorIndexPath.GetAt(0);

        m_selectionModel.Deselect(index);

        if (anchorIndex != -1)
        {
            m_selectionModel.SetAnchorIndex(anchorIndex);
        }
    }
}

bool SelectorBase::IsSelected(winrt::IndexPath const& index)
{
    bool isSelected = false;

    if (m_selectionModel != nullptr)
    {
        winrt::IReference<bool> isSelectedNullable = m_selectionModel.IsSelectedAt(index);

        if (isSelectedNullable != nullptr)
        {
            isSelected = isSelectedNullable.Value();
        }
    }

    return isSelected;
}

bool SelectorBase::CanSelect(winrt::IndexPath const& index)
{
    return m_selectionModel != nullptr;
}

void SelectorBase::SelectAll()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (m_selectionModel != nullptr)
    {
        m_selectionModel.SelectAll();
    }
}

void SelectorBase::Clear()
{
    ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (m_selectionModel != nullptr)
    {
        m_selectionModel.ClearSelection();
    }
}
