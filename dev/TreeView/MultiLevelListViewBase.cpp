// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MultiLevelListViewBase.h"

MultiLevelListViewBase::MultiLevelListViewBase(const ITrackerHandleManager* m_owner, const winrt::ListView& lv)
    : m_viewModel(m_owner), m_listView(m_owner)
{
    m_listView.set(lv);
}

MultiLevelListViewBase::~MultiLevelListViewBase()
{
}

com_ptr<ViewModel> MultiLevelListViewBase::ListViewModel() const
{
    return m_viewModel.get();
}

void MultiLevelListViewBase::ListViewModel(com_ptr<ViewModel> viewModel)
{
    m_viewModel.set(viewModel);
}

winrt::TreeViewNode MultiLevelListViewBase::NodeAtFlatIndex(int index) const
{
    return ListViewModel()->GetNodeAt(index);
}

winrt::TreeViewNode MultiLevelListViewBase::NodeFromContainer(winrt::DependencyObject const& container)
{
    int index = m_listView.get().IndexFromContainer(container);
    if (index >= 0 && index < static_cast<int32_t>(ListViewModel()->Size()))
    {
        return NodeAtFlatIndex(index);
    }
    return nullptr;
}

winrt::DependencyObject MultiLevelListViewBase::ContainerFromNode(winrt::TreeViewNode const& node)
{
    return m_listView.get().ContainerFromItem(node.Content());
}
