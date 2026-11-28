// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ViewModel.h"
#include "TreeView.h"
#include "TreeViewItem.h"
#include "VectorChangedEventArgs.h"
#include "TreeViewList.h"
#include <HashMap.h>

// Need to update node selection states on UI before vector changes.
// Listen on vector change events don't solve the problem because the event already happened when the event handler gets called.
// i.e. the node is already gone when we get to ItemRemoved callback.
#pragma region SelectedTreeNodeVector

bool SelectedTreeNodeVector::IndexOf_OptimizedForRemove(winrt::TreeViewNode const& node, uint32_t& index)
{
    return GetVectorInnerImpl()->IndexOf_OptimizedForRemove(node, index);
}

void SelectedTreeNodeVector::UpdateSelection(winrt::TreeViewNode const& node, TreeNodeSelectionState state)
{
    if (winrt::get_self<TreeViewNode>(node)->SelectionState() != state)
    {
        if (auto viewModel = m_viewModel.get())
        {
            viewModel->UpdateSelection(node, state);
            viewModel->NotifyContainerOfSelectionChange(node, winrt::get_self<TreeViewNode>(node)->SelectionState());
        }
    }
}

void SelectedTreeNodeVector::SetViewModel(ViewModel& viewModel)
{
    m_viewModel = viewModel.get_weak();
}

void SelectedTreeNodeVector::Append(winrt::TreeViewNode const& node)
{
    InsertAt(Size(), node);
}

void SelectedTreeNodeVector::InsertAt(unsigned int index, winrt::TreeViewNode const& node)
{
    // UpdateSelection will call InsertAtCore_NoContainsCheck
    UpdateSelection(node, TreeNodeSelectionState::Selected);
}

void SelectedTreeNodeVector::SetAt(unsigned int index, winrt::TreeViewNode const& node)
{
    RemoveAt(index);
    InsertAt(index, node);
}

void SelectedTreeNodeVector::RemoveAt(unsigned int index)
{
    auto inner = GetVectorInnerImpl();
    auto oldNode = winrt::get_self<TreeViewNode>(inner->GetAt(index));
    // UpdateSelection will call RemoveAtCore
    UpdateSelection(*oldNode, TreeNodeSelectionState::UnSelected);
}

void SelectedTreeNodeVector::RemoveAtEnd()
{
    RemoveAt(Size() - 1);
}

void SelectedTreeNodeVector::ReplaceAll(winrt::array_view<winrt::TreeViewNode const> nodes)
{
    Clear();

    for (auto const& node : nodes)
    {
        Append(node);
    }
}

void SelectedTreeNodeVector::Clear()
{
    while (Size() > 0)
    {
        RemoveAtEnd();
    }
}

bool SelectedTreeNodeVector::Contains(winrt::TreeViewNode const& node)
{
    uint32_t index;
    return GetVectorInnerImpl()->IndexOf(node, index);
}

namespace
{
    struct ScopedSelectionVectorNotification
    {
        explicit ScopedSelectionVectorNotification(com_ptr<ViewModel> const& viewModel)
            : m_viewModel(viewModel)
        {
            if (m_viewModel)
            {
                m_viewModel->EnterSelectionVectorNotification();
            }
        }

        ~ScopedSelectionVectorNotification()
        {
            if (m_viewModel)
            {
                m_viewModel->ExitSelectionVectorNotification();
            }
        }

        ScopedSelectionVectorNotification(ScopedSelectionVectorNotification const&) = delete;
        ScopedSelectionVectorNotification& operator=(ScopedSelectionVectorNotification const&) = delete;

    private:
        com_ptr<ViewModel> m_viewModel;
    };
}

// Default write methods will trigger TreeView visual updates.
// If you want to update vector content without notifying TreeViewNodes, use "core" version of the methods.
void SelectedTreeNodeVector::InsertAtCore_NoContainsCheck(unsigned int index, winrt::TreeViewNode const& node)
{
    MUX_ASSERT(!Contains(node));

    const auto viewModel = m_viewModel.get();
    const bool alreadyInsertedInSelectedItems = viewModel && viewModel->ConsumePendingInsertedItemFromSelectedItems(node);
    const bool isNestedNotification = viewModel && viewModel->IsInSelectionVectorNotification();
    const uint64_t versionBefore = viewModel ? viewModel->IncrementSelectionMutationVersion() : 0;

    {
        ScopedSelectionVectorNotification notificationGuard(viewModel);
        GetVectorInnerImpl()->InsertAt(index, node);
    }

    // Keep SelectedItems and SelectedNodes in sync.
    if (viewModel)
    {
        auto selectedItems = viewModel->GetSelectedItems();
        auto selectedItemsImpl = winrt::get_self<SelectedItemsVector>(selectedItems);

        if (!isNestedNotification && viewModel->SelectionMutationVersion() == versionBefore)
        {
            // Fast path: no re-entrant selection mutation occurred during VectorChanged.
            if (!alreadyInsertedInSelectedItems)
            {
                if (auto item = viewModel->ItemFromNode(node))
                {
                    if (!viewModel->IsContentMode() || !selectedItemsImpl->Contains(item))
                    {
                        const uint32_t insertIndex = std::min(index, selectedItemsImpl->Size());
                        const uint64_t itemVersionBefore = viewModel->SelectionMutationVersion();
                        selectedItemsImpl->InsertAt_NoContainsCheck(insertIndex, item, /*syncSelectedNodes*/ false);
                        if (viewModel->SelectionMutationVersion() == itemVersionBefore + 1 || selectedItemsImpl->Contains(item))
                        {
                            viewModel->TrackItemSelected(item);
                        }
                    }
                }
            }
            return;
        }

        // Re-entrant callback mutated selection: reconcile by identity if node is still selected.
        uint32_t currentIndex = index;
        if ((currentIndex < Size() && GetVectorInnerImpl()->GetAt(currentIndex) == node) ||
            GetVectorInnerImpl()->IndexOf(node, currentIndex))
        {
            if (auto item = viewModel->ItemFromNode(node))
            {
                if (!selectedItemsImpl->Contains(item))
                {
                    const uint32_t insertIndex = std::min(currentIndex, selectedItemsImpl->Size());
                    selectedItemsImpl->InsertAt_NoContainsCheck(insertIndex, item, /*syncSelectedNodes*/ false);
                }
                if (selectedItemsImpl->Contains(item))
                {
                    viewModel->TrackItemSelected(item);
                }
            }
        }
    }
}

void SelectedTreeNodeVector::RemoveAtCore(unsigned int index)
{
    const auto viewModel = m_viewModel.get();
    const auto removedNode = GetVectorInnerImpl()->GetAt(index);
    const bool alreadyRemovedFromSelectedItems = viewModel && viewModel->ConsumePendingRemovedItemFromSelectedItems(removedNode);
    const bool isNestedNotification = viewModel && viewModel->IsInSelectionVectorNotification();
    const uint64_t versionBefore = viewModel ? viewModel->IncrementSelectionMutationVersion() : 0;

    {
        ScopedSelectionVectorNotification notificationGuard(viewModel);
        GetVectorInnerImpl()->RemoveAt(index);
    }

    // Keep SelectedItems and SelectedNodes in sync.
    if (viewModel)
    {
        auto selectedItems = viewModel->GetSelectedItems();
        auto selectedItemsImpl = winrt::get_self<SelectedItemsVector>(selectedItems);
        const bool noReentrantMutation = !isNestedNotification && viewModel->SelectionMutationVersion() == versionBefore;

        if (noReentrantMutation && alreadyRemovedFromSelectedItems)
        {
            return;
        }

        const auto item = viewModel->ItemFromNode(removedNode);
        if (!item)
        {
            return;
        }

        if (noReentrantMutation)
        {
            // Fast path: verify item identity in O(1) at index (or back() when m_originNode shifts Content-mode indices).
            const uint32_t originOffset = (viewModel->IsContentMode() && Size() > 0 && GetVectorInnerImpl()->GetAt(0) == viewModel->OriginNode()) ? 1u : 0u;
            if (!viewModel->IsContentMode() || selectedItems.Size() == Size() + 1 - originOffset)
            {
                uint32_t candidateIndex = UINT32_MAX;
                if (index < selectedItems.Size() && selectedItems.GetAt(index) == item)
                {
                    candidateIndex = index;
                }
                else if (selectedItems.Size() > 0 && selectedItems.GetAt(selectedItems.Size() - 1) == item)
                {
                    candidateIndex = selectedItems.Size() - 1;
                }

                if (candidateIndex != UINT32_MAX)
                {
                    const uint64_t itemVersionBefore = viewModel->SelectionMutationVersion();
                    selectedItemsImpl->RemoveAtCore(candidateIndex, /*syncSelectedNodes*/ false);
                    if (viewModel->SelectionMutationVersion() == itemVersionBefore + 1 || !selectedItemsImpl->Contains(item))
                    {
                        viewModel->TrackItemUnselected(item);
                    }
                    return;
                }
            }
        }

        // Slow path (re-entrant mutation, order drift, or duplicate Content in Content mode): reconcile by identity.
        if (noReentrantMutation || !Contains(removedNode))
        {
            bool itemStillSelected = false;
            if (viewModel->IsContentMode())
            {
                for (uint32_t i = 0; i < Size(); ++i)
                {
                    if (viewModel->ItemFromNode(GetVectorInnerImpl()->GetAt(i)) == item)
                    {
                        itemStillSelected = true;
                        break;
                    }
                }
            }

            if (!itemStillSelected)
            {
                uint32_t itemIndex = 0;
                if (selectedItemsImpl->IndexOf_OptimizedForRemove(item, itemIndex))
                {
                    const uint64_t itemVersionBefore = viewModel->SelectionMutationVersion();
                    selectedItemsImpl->RemoveAtCore(itemIndex, /*syncSelectedNodes*/ false);
                    if (viewModel->SelectionMutationVersion() == itemVersionBefore + 1 || !selectedItemsImpl->Contains(item))
                    {
                        viewModel->TrackItemUnselected(item);
                    }
                }
            }
        }
    }
}

#pragma endregion

// Similar to SelectedNodesVector above, we need to make decisions before the item is inserted or removed.
// we can't use vector change events because the event already happened when event hander gets called.
#pragma region SelectedItemsVector

bool SelectedItemsVector::IndexOf_OptimizedForRemove(winrt::IInspectable const& node, uint32_t& index)
{
    return GetVectorInnerImpl()->IndexOf_OptimizedForRemove(node, index);
}

void SelectedItemsVector::SetViewModel(ViewModel& viewModel)
{
    m_viewModel = viewModel.get_weak();
}

void SelectedItemsVector::Append(winrt::IInspectable const& item)
{
    InsertAt(Size(), item);
}

void SelectedItemsVector::InsertAt(unsigned int index, winrt::IInspectable const& item)
{
    if (!Contains(item))
    {
        InsertAt_NoContainsCheck(index, item);
    }
}

void SelectedItemsVector::InsertAt_NoContainsCheck(unsigned int index, winrt::IInspectable const& item, bool syncSelectedNodes)
{
    MUX_ASSERT(!Contains(item));

    const auto viewModel = m_viewModel.get();
    const bool isNestedNotification = viewModel && viewModel->IsInSelectionVectorNotification();
    const uint64_t versionBefore = viewModel ? viewModel->IncrementSelectionMutationVersion() : 0;

    {
        ScopedSelectionVectorNotification notificationGuard(viewModel);
        GetVectorInnerImpl()->InsertAt(index, item);
    }

    // Keep SelectedNodes and SelectedItems in sync.
    if (viewModel && syncSelectedNodes)
    {
        auto selectedNodes = viewModel->GetSelectedNodes();
        auto insertNode = [&viewModel, &selectedNodes, &item](uint32_t targetIndex, winrt::TreeViewNode const& node)
        {
            viewModel->SetPendingInsertedItemFromSelectedItems(item);
            auto clearPending = gsl::finally([&viewModel]()
            {
                viewModel->SetPendingInsertedItemFromSelectedItems(nullptr);
            });
            selectedNodes.InsertAt(std::min(targetIndex, selectedNodes.Size()), node);
        };

        if (!isNestedNotification && viewModel->SelectionMutationVersion() == versionBefore)
        {
            if (auto node = viewModel->NodeFromItem(item))
            {
                auto selectedNodesImpl = winrt::get_self<SelectedTreeNodeVector>(selectedNodes);
                if (!selectedNodesImpl->Contains(node))
                {
                    insertNode(index, node);
                }
            }
            return;
        }

        uint32_t currentIndex = index;
        if ((currentIndex < Size() && GetVectorInnerImpl()->GetAt(currentIndex) == item) ||
            GetVectorInnerImpl()->IndexOf(item, currentIndex))
        {
            if (auto node = viewModel->NodeFromItem(item))
            {
                auto selectedNodesImpl = winrt::get_self<SelectedTreeNodeVector>(selectedNodes);
                if (!selectedNodesImpl->Contains(node))
                {
                    insertNode(currentIndex, node);
                }
            }
        }
    }
}

void SelectedItemsVector::SetAt(unsigned int index, winrt::IInspectable const& item)
{
    RemoveAt(index);
    InsertAt(index, item);
}

void SelectedItemsVector::RemoveAt(unsigned int index)
{
    RemoveAtCore(index, /*syncSelectedNodes*/ true);
}

void SelectedItemsVector::RemoveAtCore(unsigned int index, bool syncSelectedNodes)
{
    const auto viewModel = m_viewModel.get();
    const auto removedItem = GetVectorInnerImpl()->GetAt(index);
    const bool isNestedNotification = viewModel && viewModel->IsInSelectionVectorNotification();
    const uint64_t versionBefore = viewModel ? viewModel->IncrementSelectionMutationVersion() : 0;

    {
        ScopedSelectionVectorNotification notificationGuard(viewModel);
        GetVectorInnerImpl()->RemoveAt(index);
    }

    // Keep SelectedNodes and SelectedItems in sync.
    if (viewModel && syncSelectedNodes)
    {
        auto selectedNodes = viewModel->GetSelectedNodes();
        auto removeNodeAt = [&viewModel, &selectedNodes, &removedItem](uint32_t targetIndex)
        {
            viewModel->SetPendingRemovedItemFromSelectedItems(removedItem);
            auto clearPending = gsl::finally([&viewModel]()
            {
                viewModel->SetPendingRemovedItemFromSelectedItems(nullptr);
            });
            selectedNodes.RemoveAt(targetIndex);
        };

        const bool noReentrantMutation = !isNestedNotification && viewModel->SelectionMutationVersion() == versionBefore;
        if (noReentrantMutation)
        {
            // Fast path: verify node identity in O(1) at index (or back() during reverse Clear()).
            if (index < selectedNodes.Size() && viewModel->ItemFromNode(selectedNodes.GetAt(index)) == removedItem)
            {
                removeNodeAt(index);
                return;
            }
            if (selectedNodes.Size() > 0 && viewModel->ItemFromNode(selectedNodes.GetAt(selectedNodes.Size() - 1)) == removedItem)
            {
                removeNodeAt(selectedNodes.Size() - 1);
                return;
            }
        }

        if (noReentrantMutation || !Contains(removedItem))
        {
            if (auto node = viewModel->NodeFromItem(removedItem))
            {
                auto selectedNodesImpl = winrt::get_self<SelectedTreeNodeVector>(selectedNodes);
                uint32_t nodeIndex = 0;
                if (selectedNodesImpl->IndexOf_OptimizedForRemove(node, nodeIndex))
                {
                    removeNodeAt(nodeIndex);
                }
            }
            else
            {
                for (uint32_t i = selectedNodes.Size(); i > 0; --i)
                {
                    if (viewModel->ItemFromNode(selectedNodes.GetAt(i - 1)) == removedItem)
                    {
                        removeNodeAt(i - 1);
                        break;
                    }
                }
            }
        }
    }
}

void SelectedItemsVector::RemoveAtEnd()
{
    RemoveAt(Size() - 1);
}

void SelectedItemsVector::ReplaceAll(winrt::array_view<winrt::IInspectable const> items)
{
    Clear();

    for (auto const& node : items)
    {
        Append(node);
    }
}

void SelectedItemsVector::Clear()
{
    while (Size() > 0)
    {
        RemoveAtEnd();
    }
}

bool SelectedItemsVector::Contains(winrt::IInspectable const& item)
{
    uint32_t index;
    return GetVectorInnerImpl()->IndexOf(item, index);
}

#pragma endregion

ViewModel::ViewModel()
{
    auto selectedNodes = winrt::make_self<SelectedTreeNodeVector>();
    selectedNodes->SetViewModel(*this);
    m_selectedNodes.set(*selectedNodes);

    auto selectedItems = winrt::make_self<SelectedItemsVector>();
    selectedItems->SetViewModel(*this);
    m_selectedItems.set(*selectedItems);
}

ViewModel::~ViewModel()
{
    if (m_rootNodeChildrenChangedEventToken.value != 0)
    {
        if (auto origin = m_originNode.safe_get())
        {
            winrt::get_self<TreeViewNode>(origin)->ChildrenChanged(m_rootNodeChildrenChangedEventToken);
        }

        ClearEventTokenVectors();
    }
}

void ViewModel::ExpandNode(const winrt::TreeViewNode& value)
{
    value.IsExpanded(true);
}

void ViewModel::CollapseNode(const winrt::TreeViewNode& value)
{
    value.IsExpanded(false);
}

winrt::event_token ViewModel::NodeExpanding(const winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IInspectable>& handler)
{
    return m_nodeExpandingEventSource.add(handler);
}

void ViewModel::NodeExpanding(const winrt::event_token token)
{
    m_nodeExpandingEventSource.remove(token);
}

winrt::event_token ViewModel::NodeCollapsed(const winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IInspectable>& handler)
{
    return m_nodeCollapsedEventSource.add(handler);
}

void ViewModel::NodeCollapsed(const winrt::event_token token)
{
    m_nodeCollapsedEventSource.remove(token);
}

void ViewModel::SelectAll()
{
    auto trackSelection = gsl::finally([this]() { EndSelectionChanges(); });
    BeginSelectionChanges();

    UpdateSelection(m_originNode.get(), TreeNodeSelectionState::Selected);
}

void ViewModel::SelectSingleItem(winrt::IInspectable const& item)
{
    auto trackSelection = gsl::finally([this]() { EndSelectionChanges(); });
    BeginSelectionChanges();

    auto selectedNodes = GetSelectedNodes();
    if (selectedNodes.Size() > 0)
    {
        selectedNodes.Clear();
    }
    auto selectedItems = GetSelectedItems();
    if (selectedItems.Size() > 0)
    {
        selectedItems.Clear();
    }
    if (item)
    {
        selectedItems.Append(item);
    }
}

void ViewModel::SelectNode(const winrt::TreeViewNode& node, bool isSelected)
{
    auto trackSelection = gsl::finally([this]() { EndSelectionChanges(); });
    BeginSelectionChanges();

    auto selectedNodes = GetSelectedNodes();
    if (isSelected)
    {
        if (IsInSingleSelectionMode() && selectedNodes.Size() > 0)
        {
            selectedNodes.Clear();
        }
        selectedNodes.Append(node);
    }
    else
    {
        unsigned int index;
        if (selectedNodes.IndexOf(node, index))
        {
            selectedNodes.RemoveAt(index);
        }
    }
}

void ViewModel::SelectByIndex(int index, TreeNodeSelectionState const& state)
{
    auto trackSelection = gsl::finally([this]() { EndSelectionChanges(); });
    BeginSelectionChanges();

    auto targetNode = GetNodeAt(index);
    UpdateSelection(targetNode, state);
}

void ViewModel::BeginSelectionChanges()
{
    if (!IsInSingleSelectionMode())
    {
        m_selectionTrackingCounter++;
        if (m_selectionTrackingCounter == 1) {
            m_addedSelectedItems.clear();
            m_removedSelectedItems.clear();
        }
    }
}

void ViewModel::EndSelectionChanges()
{
    if (!IsInSingleSelectionMode())
    {
        m_selectionTrackingCounter--;
        if (m_selectionTrackingCounter == 0 && (m_addedSelectedItems.size() > 0 || m_removedSelectedItems.size() > 0)) {
            const auto treeView = m_TreeView.get();
            if (!treeView)
            {
                return;
            }

            auto added = winrt::make<Vector<winrt::IInspectable>>();
            for (unsigned int i = 0; i < m_addedSelectedItems.size(); i++)
            {
                added.Append(m_addedSelectedItems.at(i).get());
            }
            auto removed = winrt::make<Vector<winrt::IInspectable>>();
            for (unsigned int i = 0; i < m_removedSelectedItems.size(); i++)
            {
                removed.Append(m_removedSelectedItems.at(i));
            }
            winrt::get_self<TreeView>(treeView)->RaiseSelectionChanged(added, removed);
        }
    }
}

uint32_t ViewModel::Size()
{
    return m_items.size();
}

winrt::IInspectable ViewModel::GetAt(uint32_t index)
{
    winrt::TreeViewNode node = GetNodeAt(index);
    return IsContentMode() ? node.Content() : node;
}

bool ViewModel::IndexOf(winrt::IInspectable const& value, uint32_t& index)
{
    if (auto indexOfFunction = GetCustomIndexOfFunction())
    {
        return indexOfFunction(value, index);
    }
    else
    {
        return IndexOfItem(value, index);
    }
}

uint32_t ViewModel::GetMany(uint32_t const startIndex, winrt::array_view<winrt::IInspectable> values)
{
    if (IsContentMode())
    {
        auto vector = winrt::make<Vector<winrt::IInspectable>>();
        const int size = Size();
        for (int i = 0; i < size; i++)
        {
            vector.Append(GetNodeAt(i).Content());
        }
        return vector.GetMany(startIndex, values);
    }

    uint32_t count = 0;
    m_items.for_each(startIndex, values.size(), [&](const FlatListItem& row) { values[count++] = row.item.safe_get(false); });
    return count;
}

winrt::IVectorView<winrt::IInspectable> ViewModel::GetView()
{
    throw winrt::hresult_not_implemented();
}

winrt::TreeViewNode ViewModel::GetNodeAt(uint32_t index)
{
    return ItemAt(index).as<winrt::TreeViewNode>();
}

void ViewModel::SetAt(uint32_t index, winrt::IInspectable const& value)
{
    auto current = GetNodeAt(index);
    m_items.replace(index, MakeFlatListItem(value));
    RaiseVectorChanged(winrt::CollectionChange::ItemChanged, index);

    winrt::TreeViewNode newNode = value.as<winrt::TreeViewNode>();

    auto tvnCurrent = winrt::get_self<TreeViewNode>(current);
    tvnCurrent->ChildrenChanged(m_collectionChangedEventTokenVector[index]);
    tvnCurrent->RemoveExpandedChanged(m_IsExpandedChangedEventTokenVector[index]);

    // Hook up events and replace tokens
    auto tvnNewNode = winrt::get_self<TreeViewNode>(newNode);
    m_collectionChangedEventTokenVector[index] = tvnNewNode->ChildrenChanged({ this, &ViewModel::TreeViewNodeVectorChanged });
    m_IsExpandedChangedEventTokenVector[index] = tvnNewNode->AddExpandedChanged({ this, &ViewModel::TreeViewNodePropertyChanged });
}

void ViewModel::InsertAt(uint32_t index, winrt::IInspectable const& value)
{
    if (index > m_items.size())
    {
        throw winrt::hresult_out_of_bounds();
    }
    m_items.insert(index, MakeFlatListItem(value));
    RaiseVectorChanged(winrt::CollectionChange::ItemInserted, index);
    winrt::TreeViewNode newNode = value.as<winrt::TreeViewNode>();

    // Hook up events and save tokens
    auto tvnNewNode = winrt::get_self<TreeViewNode>(newNode);
    m_collectionChangedEventTokenVector.insert(index, tvnNewNode->ChildrenChanged({ this, &ViewModel::TreeViewNodeVectorChanged }));
    m_IsExpandedChangedEventTokenVector.insert(index, tvnNewNode->AddExpandedChanged({ this, &ViewModel::TreeViewNodePropertyChanged }));
}

void ViewModel::RemoveAt(uint32_t index)
{
    auto current = GetNodeAt(index);
    m_items.erase(index);
    RaiseVectorChanged(winrt::CollectionChange::ItemRemoved, index);

    // Unhook event handlers
    auto tvnCurrent = winrt::get_self<TreeViewNode>(current);
    tvnCurrent->ChildrenChanged(m_collectionChangedEventTokenVector[index]);
    tvnCurrent->RemoveExpandedChanged(m_IsExpandedChangedEventTokenVector[index]);

    // Remove tokens from vectors
    m_collectionChangedEventTokenVector.erase(index);
    m_IsExpandedChangedEventTokenVector.erase(index);
}

void ViewModel::Append(winrt::IInspectable const& value)
{
    m_items.push_back(MakeFlatListItem(value));
    RaiseVectorChanged(winrt::CollectionChange::ItemInserted, m_items.size() - 1);
    winrt::TreeViewNode newNode = value.as<winrt::TreeViewNode>();

    // Hook up events and save tokens
    auto tvnNewNode = winrt::get_self<TreeViewNode>(newNode);
    m_collectionChangedEventTokenVector.push_back(tvnNewNode->ChildrenChanged({ this, &ViewModel::TreeViewNodeVectorChanged }));
    m_IsExpandedChangedEventTokenVector.push_back(tvnNewNode->AddExpandedChanged({ this, &ViewModel::TreeViewNodePropertyChanged }));
}

void ViewModel::RemoveAtEnd()
{
    auto current = GetNodeAt(Size() - 1);
    m_items.pop_back();
    RaiseVectorChanged(winrt::CollectionChange::ItemRemoved, m_items.size());

    // Unhook events
    auto tvnCurrent = winrt::get_self<TreeViewNode>(current);
    tvnCurrent->ChildrenChanged(m_collectionChangedEventTokenVector.back());
    tvnCurrent->RemoveExpandedChanged(m_IsExpandedChangedEventTokenVector.back());

    // Remove tokens
    m_collectionChangedEventTokenVector.pop_back();
    m_IsExpandedChangedEventTokenVector.pop_back();
}

void ViewModel::Clear()
{
    // Don't call GetVectorInnerImpl()->Clear() directly because we need to remove hooked events
    unsigned int count = Size();
    while (count != 0)
    {
        RemoveAtEnd();
        count--;
    }
}

void ViewModel::ReplaceAll(winrt::array_view<winrt::IInspectable const> items)
{
    m_items.clear();
    RaiseVectorChanged(winrt::CollectionChange::Reset, 0u);
    for (auto value : items)
    {
        m_items.push_back(MakeFlatListItem(value));
        RaiseVectorChanged(winrt::CollectionChange::ItemInserted, m_items.size() - 1);
    }
}

// The rows are in m_items. Implement_Vector_External's inner vector stays empty and only raises the flat list's
// VectorChanged event.
void ViewModel::RaiseVectorChanged(winrt::CollectionChange change, uint32_t index)
{
    GetVectorInnerImpl()->RaiseChildrenChanged(change, index);
}

ViewModel::FlatListItem ViewModel::MakeFlatListItem(winrt::IInspectable const& value)
{
    // The same identity that operator== compares for winrt objects.
    const auto identity = value ? winrt::get_abi(value.try_as<winrt::Windows::Foundation::IUnknown>()) : nullptr;
    return FlatListItem{ tracker_ref<winrt::IInspectable>{ this, value }, identity };
}

winrt::IInspectable ViewModel::ItemAt(uint32_t index, bool useSafeGet)
{
    if (index >= m_items.size())
    {
        throw winrt::hresult_out_of_bounds();
    }
    return m_items[index].item.safe_get(useSafeGet);
}

bool ViewModel::IndexOfItem(winrt::IInspectable const& value, uint32_t& index)
{
    const auto identity = value ? winrt::get_abi(value.try_as<winrt::Windows::Foundation::IUnknown>()) : nullptr;
    const auto found = m_items.find_first(identity);
    index = found.value_or(0);
    return found.has_value();
}

// Helper function
void ViewModel::PrepareView(const winrt::TreeViewNode& originNode)
{
    // Remove any existing RootNode events/children
    if (auto existingOriginNode = m_originNode.get())
    {
        for (int i = (existingOriginNode.Children().Size() - 1); i >= 0; i--)
        {
            auto removeNode = existingOriginNode.Children().GetAt(i).as<winrt::TreeViewNode>();
            RemoveNodeAndDescendantsFromView(removeNode);
        }

        if (m_rootNodeChildrenChangedEventToken.value != 0)
        {
            existingOriginNode.Children().as<winrt::IObservableVector<winrt::TreeViewNode>>().VectorChanged(m_rootNodeChildrenChangedEventToken);
        }
    }

    // Add new RootNode & children
    m_originNode.set(originNode);
    m_rootNodeChildrenChangedEventToken = winrt::get_self<TreeViewNode>(originNode)->ChildrenChanged({ this, &ViewModel::TreeViewNodeVectorChanged });
    originNode.IsExpanded(true);

    int allOpenedDescendantsCount = 0;
    for (unsigned int i = 0; i < originNode.Children().Size(); i++)
    {
        auto addNode = originNode.Children().GetAt(i).as<winrt::TreeViewNode>();
        AddNodeToView(addNode, i + allOpenedDescendantsCount);
        allOpenedDescendantsCount = AddNodeDescendantsToView(addNode, i, allOpenedDescendantsCount);
    }
}

void ViewModel::SetOwners(winrt::TreeViewList const& owningList, winrt::TreeView const& owningTreeView)
{
    m_TreeViewList = winrt::make_weak(owningList);
    m_TreeView = winrt::make_weak(owningTreeView);
}

bool ViewModel::IsInSingleSelectionMode()
{
    if (const auto listControl = m_TreeViewList.get())
    {
        m_isInSingleSelectionMode = listControl.SelectionMode() == winrt::ListViewSelectionMode::Single;
    }
    return m_isInSingleSelectionMode;
}

// Private helpers
void ViewModel::AddNodeToView(const winrt::TreeViewNode& value, unsigned int index)
{
    InsertAt(index, value);
}

int ViewModel::AddNodeDescendantsToView(const winrt::TreeViewNode& value, unsigned int index, int offset)
{
    if (value.IsExpanded())
    {
        unsigned int size = value.Children().Size();
        for (unsigned int i = 0; i < size; i++)
        {
            auto childNode = value.Children().GetAt(i).as<winrt::TreeViewNode>();
            offset++;
            AddNodeToView(childNode, offset + index);
            offset = AddNodeDescendantsToView(childNode, index, offset);
        }

        return offset;
    }

    return offset;
}

void ViewModel::RemoveNodeAndDescendantsFromView(const winrt::TreeViewNode& value)
{
    UINT32 valueIndex;
    if (value.IsExpanded())
    {
        unsigned int size = value.Children().Size();
        for (unsigned int i = 0; i < size; i++)
        {
            auto childNode = value.Children().GetAt(i).as<winrt::TreeViewNode>();
            RemoveNodeAndDescendantsFromView(childNode);
        }
    }

    const bool containsValue = IndexOfNode(value, valueIndex);
    if (containsValue)
    {
        RemoveAt(valueIndex);
    }
}

void ViewModel::RemoveNodesAndDescendentsWithFlatIndexRange(unsigned int lowIndex, unsigned int highIndex)
{
    MUX_ASSERT(lowIndex <= highIndex);

    for (int i = static_cast<int>(highIndex); i >= static_cast<int>(lowIndex); i--)
    {
        RemoveNodeAndDescendantsFromView(GetNodeAt(i));
    }
}

int ViewModel::GetNextIndexInFlatTree(const winrt::TreeViewNode& node)
{
    unsigned int index = 0;
    const bool isNodeInFlatList = IndexOfNode(node, index);

    if (isNodeInFlatList)
    {
        index++;
    }
    else
    {
        // node is Root node, so next index in flat tree is 0
        index = 0;
    }
    return index;
}

// When ViewModel receives a event, it only includes the sender(parent TreeViewNode) and index.
// We can't use sender[index] directly because it is already updated/removed
// To find the removed TreeViewNode:
//   calculate allOpenedDescendantsCount in sender[0..index-1] first
//   then add offset and finally return TreeViewNode by looking up the flat tree.
winrt::TreeViewNode ViewModel::GetRemovedChildTreeViewNodeByIndex(winrt::TreeViewNode const& node, unsigned int childIndex)
{
    unsigned int allOpenedDescendantsCount = 0;
    for (unsigned int i = 0; i < childIndex; i++)
    {
        winrt::TreeViewNode calcNode = node.Children().GetAt(i).as<winrt::TreeViewNode>();
        if (calcNode.IsExpanded())
        {
            allOpenedDescendantsCount += GetExpandedDescendantCount(calcNode);
        }
    }

    const unsigned int childIndexInFlatTree = GetNextIndexInFlatTree(node) + childIndex + allOpenedDescendantsCount;
    return GetNodeAt(childIndexInFlatTree);
}

// Returns the flat tree index for the child that was just inserted at childIndex of an expanded parent.
// The child goes right after its previous sibling and that sibling's visible descendants. Children are
// often added one at a time, for example when an ItemsSource is filled, so this avoids walking all the
// earlier siblings for each new child.
unsigned int ViewModel::GetInsertedChildIndexInFlatTree(winrt::TreeViewNode const& parentNode, unsigned int childIndex)
{
    const unsigned int firstChildIndex = GetNextIndexInFlatTree(parentNode);
    if (childIndex > 0)
    {
        auto previousSibling = parentNode.Children().GetAt(childIndex - 1);
        unsigned int previousSiblingIndex = 0;
        // App code can move a node while its old row is still in the view, for example from a HasChildren callback,
        // and IndexOfNode returns the first of its rows. Its row under this parent comes after the parent's row.
        if (IndexOfNode(previousSibling, previousSiblingIndex) && previousSiblingIndex >= firstChildIndex)
        {
            const unsigned int previousSiblingDescendantCount = previousSibling.IsExpanded() ? CountDescendants(previousSibling) : 0;
            return previousSiblingIndex + 1 + previousSiblingDescendantCount;
        }
    }

    // There is no previous sibling, or its row is not under this parent. Count from the parent instead.
    unsigned int flatIndex = firstChildIndex + childIndex;
    for (unsigned int i = 0; i < childIndex; i++)
    {
        auto childNode = parentNode.Children().GetAt(i);
        if (childNode.IsExpanded())
        {
            flatIndex += CountDescendants(childNode);
        }
    }

    return flatIndex;
}

int ViewModel::CountDescendants(const winrt::TreeViewNode& value)
{
    int descendantCount = 0;
    unsigned int size = value.Children().Size();
    for (unsigned int i = 0; i < size; i++)
    {
        auto childNode = value.Children().GetAt(i).as<winrt::TreeViewNode>();
        descendantCount++;
        if (childNode.IsExpanded())
        {
            descendantCount = descendantCount + CountDescendants(childNode);
        }
    }

    return descendantCount;
}

unsigned int ViewModel::IndexOfNextSibling(winrt::TreeViewNode const& childNode)
{
    auto child = childNode;
    auto parentNode = child.Parent();
    unsigned int stopIndex;
    bool isLastRelativeChild = true;
    while (parentNode && isLastRelativeChild)
    {
        unsigned int relativeIndex;
        parentNode.Children().IndexOf(child, relativeIndex);
        if (parentNode.Children().Size() - 1 != relativeIndex)
        {
            isLastRelativeChild = false;
        }
        else
        {
            child = parentNode;
            parentNode = parentNode.Parent();
        }
    }

    if (parentNode)
    {
        unsigned int siblingIndex;
        parentNode.Children().IndexOf(child, siblingIndex);
        auto siblingNode = parentNode.Children().GetAt(siblingIndex + 1);
        IndexOfNode(siblingNode, stopIndex);
    }
    else
    {
        stopIndex = Size();
    }

    return stopIndex;
}

unsigned int ViewModel::GetExpandedDescendantCount(winrt::TreeViewNode const& parentNode)
{
    unsigned int allOpenedDescendantsCount = 0;
    for (unsigned int i = 0; i < parentNode.Children().Size(); i++)
    {
        auto childNode = parentNode.Children().GetAt(i).as<winrt::TreeViewNode>();
        allOpenedDescendantsCount++;
        if (childNode.IsExpanded())
        {
            allOpenedDescendantsCount += CountDescendants(childNode);
        }
    }
    return allOpenedDescendantsCount;
}

bool ViewModel::IsNodeSelected(winrt::TreeViewNode const& targetNode)
{
    unsigned int index;
    return m_selectedNodes.get().IndexOf(targetNode, index);
}

TreeNodeSelectionState ViewModel::NodeSelectionState(winrt::TreeViewNode const& targetNode)
{
    return winrt::get_self<TreeViewNode>(targetNode)->SelectionState();
}

void ViewModel::UpdateNodeSelection(winrt::TreeViewNode const& selectNode, TreeNodeSelectionState const& selectionState)
{
    auto node = winrt::get_self<TreeViewNode>(selectNode);
    if (selectionState != node->SelectionState())
    {
        node->SelectionState(selectionState);
        auto selectedNodes = winrt::get_self<SelectedTreeNodeVector>(m_selectedNodes.get());
        switch (selectionState)
        {
        case TreeNodeSelectionState::Selected:
            // Store the ChildrenChanged token before raising VectorChanged so a re-entrant removal finds its token.
            m_selectedNodeChildrenChangedEventTokenVector.push_back(node->ChildrenChanged({ this, &ViewModel::SelectedNodeChildrenChanged }));
            selectedNodes->InsertAtCore_NoContainsCheck(selectedNodes->Size(), selectNode);
            break;

        case TreeNodeSelectionState::PartialSelected:
        case TreeNodeSelectionState::UnSelected:
            unsigned int index;
            if (selectedNodes->IndexOf(selectNode, index))
            {
                const auto token = m_selectedNodeChildrenChangedEventTokenVector[index];
                m_selectedNodeChildrenChangedEventTokenVector.erase(m_selectedNodeChildrenChangedEventTokenVector.begin() + index);
                node->ChildrenChanged(token);
                selectedNodes->RemoveAtCore(index);
            }
            break;
        }
    }
}

void ViewModel::UpdateSelection(winrt::TreeViewNode const& selectNode, TreeNodeSelectionState const& selectionState)
{
    if(NodeSelectionState(selectNode) != selectionState)
    {
        UpdateNodeSelection(selectNode, selectionState);

        if (!IsInSingleSelectionMode())
        {
            UpdateSelectionStateOfDescendants(selectNode, selectionState);
            UpdateSelectionStateOfAncestors(selectNode);
        }
    }
}

void ViewModel::UpdateSelectionStateOfDescendants(winrt::TreeViewNode const& targetNode, TreeNodeSelectionState const& selectionState)
{
    if (selectionState == TreeNodeSelectionState::PartialSelected) return;

    // For unselecting items, we process the list backwards. This prevents an item from being repeatedly shuffled
    // towards the front of the list only to be removed at the end anyway.
    if (selectionState == TreeNodeSelectionState::UnSelected)
    {
        for (int i = targetNode.Children().Size() - 1; i >= 0; i--)
        {
            const auto& childNode = targetNode.Children().GetAt(i);
            UpdateSelectionStateOfDescendants(childNode, selectionState);
            UpdateNodeSelection(childNode, selectionState);
            NotifyContainerOfSelectionChange(childNode, selectionState);
        }
    }
    else
    {
        for (auto const& childNode : targetNode.Children())
        {
            UpdateNodeSelection(childNode, selectionState);
            UpdateSelectionStateOfDescendants(childNode, selectionState);
            NotifyContainerOfSelectionChange(childNode, selectionState);
        }
    }
}

void ViewModel::UpdateSelectionStateOfAncestors(winrt::TreeViewNode const& targetNode)
{
    if (auto parentNode = targetNode.Parent())
    {
        // no need to update m_originalNode since it's the logical root for TreeView and not accessible to users
        if (parentNode != m_originNode.safe_get())
        {
            const auto previousState = NodeSelectionState(parentNode);
            const auto selectionState = SelectionStateBasedOnChildren(parentNode);

            if (previousState != selectionState)
            {
                UpdateNodeSelection(parentNode, selectionState);
                NotifyContainerOfSelectionChange(parentNode, selectionState);
                UpdateSelectionStateOfAncestors(parentNode);
            }
        }
    }
}

TreeNodeSelectionState ViewModel::SelectionStateBasedOnChildren(winrt::TreeViewNode const& node)
{
    bool hasSelectedChildren{ false };
    bool hasUnSelectedChildren{ false };

    for (auto const& childNode : node.Children())
    {
        const auto state = NodeSelectionState(childNode);
        if (state == TreeNodeSelectionState::Selected)
        {
            hasSelectedChildren = true;
        }
        else if (state == TreeNodeSelectionState::UnSelected)
        {
            hasUnSelectedChildren = true;
        }

        if ((hasSelectedChildren && hasUnSelectedChildren) ||
            state == TreeNodeSelectionState::PartialSelected)
        {
            return TreeNodeSelectionState::PartialSelected;
        }
    }

    return hasSelectedChildren ? TreeNodeSelectionState::Selected : TreeNodeSelectionState::UnSelected;
}

void ViewModel::NotifyContainerOfSelectionChange(winrt::TreeViewNode const& targetNode, TreeNodeSelectionState const& selectionState)
{
    if (const auto listControl = m_TreeViewList.get())
    {
        auto container = winrt::get_self<TreeViewList>(listControl)->ContainerFromNode(targetNode);
        if (container)
        {
            winrt::TreeViewItem targetItem = container.as<winrt::TreeViewItem>();
            winrt::get_self<TreeViewItem>(targetItem)->UpdateSelectionVisual(selectionState);
        }
    }
}

winrt::IVector<winrt::TreeViewNode> ViewModel::GetSelectedNodes()
{
    return m_selectedNodes.get();
}

winrt::IVector<winrt::IInspectable> ViewModel::GetSelectedItems()
{
    return m_selectedItems.get();
}

void ViewModel::TrackItemSelected(winrt::IInspectable item)
{
    if (m_selectionTrackingCounter > 0 && item != m_originNode.safe_get())
    {
        m_addedSelectedItems.push_back(winrt::make_weak(item));
    }
}

void ViewModel::TrackItemUnselected(winrt::IInspectable item)
{
    if (m_selectionTrackingCounter > 0 && item != m_originNode.safe_get())
    {
        m_removedSelectedItems.push_back(item);
    }
}

winrt::TreeViewNode ViewModel::GetAssociatedNode(winrt::IInspectable item)
{
    // Look in the visible rows first. They are the tree's nodes in tree order without the collapsed parts, so an item
    // on screen is found without walking the collapsed parts before it. An item that is not visible falls back to the
    // search of the whole tree below.
    if (auto node = GetVisibleNodeWithContent(item))
    {
        return node;
    }

    if(auto treeView = m_TreeView.get())
    {
        if(auto treeViewImpl = winrt::get_self<TreeView>(treeView))
        {
            return treeViewImpl->GetAssociatedNode(item);
        }
    }

    return nullptr;
}

winrt::TreeViewNode ViewModel::GetVisibleNodeWithContent(winrt::IInspectable const& item)
{
    if (!item)
    {
        return nullptr;
    }

    const auto origin = m_originNode.safe_get();
    const auto size = Size();
    for (uint32_t i = 0; i < size; i++)
    {
        auto node = GetNodeAt(i);
        if (node.Content() == item)
        {
            // A row can briefly outlive its node's removal while app code runs from a change notification. Like the
            // tree search, count only a node that is still in this tree.
            auto ancestor = node.Parent();
            while (ancestor && ancestor != origin)
            {
                ancestor = ancestor.Parent();
            }
            if (ancestor)
            {
                return node;
            }
        }
    }

    return nullptr;
}

winrt::IInspectable ViewModel::ItemFromNode(winrt::TreeViewNode const& node)
{
    return (m_isContentMode && node) ? node.Content() : static_cast<winrt::IInspectable>(node);
}

winrt::TreeViewNode ViewModel::NodeFromItem(winrt::IInspectable const& item)
{
    return m_isContentMode ? GetAssociatedNode(item) : item.try_as<winrt::TreeViewNode>();
}

bool ViewModel::ConsumePendingInsertedItemFromSelectedItems(winrt::TreeViewNode const& node)
{
    if (m_pendingInsertedItemFromSelectedItems && ItemFromNode(node) == m_pendingInsertedItemFromSelectedItems)
    {
        m_pendingInsertedItemFromSelectedItems = nullptr;
        return true;
    }
    return false;
}

bool ViewModel::ConsumePendingRemovedItemFromSelectedItems(winrt::TreeViewNode const& node)
{
    if (m_pendingRemovedItemFromSelectedItems && ItemFromNode(node) == m_pendingRemovedItemFromSelectedItems)
    {
        m_pendingRemovedItemFromSelectedItems = nullptr;
        return true;
    }
    return false;
}

bool ViewModel::IndexOfNode(winrt::TreeViewNode const& targetNode, uint32_t& index)
{
    return IndexOfItem(targetNode, index);
}

void ViewModel::TreeViewNodeVectorChanged(winrt::TreeViewNode const& sender, winrt::IInspectable const& args)
{
    winrt::CollectionChange collectionChange = args.as<winrt::IVectorChangedEventArgs>().CollectionChange();
    unsigned int index = args.as<winrt::IVectorChangedEventArgs>().Index();

    switch (collectionChange)
    {
        // Reset case, commonly seen when a TreeNode is cleared.
        // removes all nodes that need removing then
        // toggles a collapse / expand to ensure order.
    case (winrt::CollectionChange::Reset):
    {
        auto resetNode = sender.as<winrt::TreeViewNode>();
        if (resetNode.IsExpanded())
        {
            //The lowIndex is the index of the first child, while the high index is the index of the last descendant in the list.
            const unsigned int lowIndex = GetNextIndexInFlatTree(resetNode);
            const unsigned int highIndex = IndexOfNextSibling(resetNode) - 1;
            RemoveNodesAndDescendentsWithFlatIndexRange(lowIndex, highIndex);

            // reset the status of resetNodes children
            CollapseNode(resetNode);
            ExpandNode(resetNode);
        }

        break;
    }

    // If the parent is expanded, add the new child and its visible descendants to the view.
    case (winrt::CollectionChange::ItemInserted):
    {
        auto targetNode = sender.as<winrt::TreeViewNode>().Children().GetAt(index).as<winrt::TreeViewNode>();

        auto parentNode = targetNode.Parent();
        if (parentNode.IsExpanded())
        {
            const unsigned int targetIndex = GetInsertedChildIndexInFlatTree(parentNode, index);
            AddNodeToView(targetNode, targetIndex);
            AddNodeDescendantsToView(targetNode, targetIndex, 0);
        }

        break;
    }

    // Removes a node from the ViewModel when a TreeNode
    // removes a child.
    case (winrt::CollectionChange::ItemRemoved):
    {
        auto removingNodeParent = sender.as<winrt::TreeViewNode>();
        if (removingNodeParent.IsExpanded())
        {
            auto removedNode = GetRemovedChildTreeViewNodeByIndex(removingNodeParent, index);
            RemoveNodeAndDescendantsFromView(removedNode);
        }

        break;
    }

    // Triggered by a replace such as SetAt.
    // Updates the TreeNode that changed in the ViewModel.
    case (winrt::CollectionChange::ItemChanged):
    {
        auto targetNode = sender.as<winrt::TreeViewNode>().Children().GetAt(index).as<winrt::TreeViewNode>();
        auto changingNodeParent = sender.as<winrt::TreeViewNode>();
        if (changingNodeParent.IsExpanded())
        {
            auto removedNode = GetRemovedChildTreeViewNodeByIndex(changingNodeParent, index);
            [[gsl::suppress(con)]]
            {
                unsigned int removedNodeIndex = 0;
                MUX_ASSERT(IndexOfNode(removedNode, removedNodeIndex));

                RemoveNodeAndDescendantsFromView(removedNode);
                InsertAt(removedNodeIndex, targetNode.as<winrt::IInspectable>());
            }

        }

        break;
    }
    }
}

void ViewModel::RemoveDetachedSelectedNodes()
{
    auto selectedNodes = winrt::get_self<SelectedTreeNodeVector>(m_selectedNodes.get());
    for (unsigned int i = 0; i < selectedNodes->Size(); i++)
    {
        auto selectNode = selectedNodes->GetAt(i);
        auto ancestorNode = selectNode.Parent();
        while (ancestorNode && ancestorNode.Parent())
        {
            ancestorNode = ancestorNode.Parent();
        }

        if (ancestorNode != m_originNode.get())
        {
            const auto token = m_selectedNodeChildrenChangedEventTokenVector[i];
            m_selectedNodeChildrenChangedEventTokenVector.erase(m_selectedNodeChildrenChangedEventTokenVector.begin() + i);
            winrt::get_self<TreeViewNode>(selectNode)->ChildrenChanged(token);
            selectedNodes->RemoveAtCore(i);

            // We've removed the node at position i, so we should keep i at the same position
            // because the new node at position i is a different one.
            i--;
        }
    }
}

void ViewModel::SelectedNodeChildrenChanged(winrt::TreeViewNode const& sender, winrt::IInspectable const& args)
{
    winrt::CollectionChange collectionChange = args.as<winrt::IVectorChangedEventArgs>().CollectionChange();
    unsigned int index = args.as<winrt::IVectorChangedEventArgs>().Index();
    auto changingChildrenNode = sender.as<winrt::TreeViewNode>();

    switch (collectionChange)
    {
    case (winrt::CollectionChange::ItemInserted):
    {
        auto newNode = changingChildrenNode.Children().GetAt(index);

        // If we are in multi select, we want the new child items to be also selected.
        if (!IsInSingleSelectionMode())
        {
            UpdateNodeSelection(newNode, NodeSelectionState(changingChildrenNode));
        }
        break;
    }

    case (winrt::CollectionChange::ItemChanged):
    {
        auto newNode = changingChildrenNode.Children().GetAt(index);
        if (!IsInSingleSelectionMode())
        {
            UpdateNodeSelection(newNode, NodeSelectionState(changingChildrenNode));
        }

        RemoveDetachedSelectedNodes();
        break;
    }

    case (winrt::CollectionChange::ItemRemoved):
    case (winrt::CollectionChange::Reset):
    {
        //This checks if there are still children, then re-evaluates parents selection based on current state of remaining children
        //If a node has 2 children selected, and 1 unselected, and the unselected is removed, we then change the parent node to selected.
        //If the last child is removed, we preserve the current selection state of the parent, and this code need not execute.
        if (!IsInSingleSelectionMode() && changingChildrenNode.Children().Size() > 0)
        {
            auto firstChildNode = changingChildrenNode.Children().GetAt(0);
            UpdateSelectionStateOfAncestors(firstChildNode);
        }

        RemoveDetachedSelectedNodes();
        break;
    }

    }
}

void ViewModel::TreeViewNodePropertyChanged(winrt::TreeViewNode const& sender, winrt::IDependencyPropertyChangedEventArgs const& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (property == TreeViewNode::s_IsExpandedProperty)
    {
        TreeViewNodeIsExpandedPropertyChanged(sender, args);
    }
    else if (property == TreeViewNode::s_HasChildrenProperty)
    {
        TreeViewNodeHasChildrenPropertyChanged(sender, args);
    }
}

void ViewModel::TreeViewNodeIsExpandedPropertyChanged(winrt::TreeViewNode const& sender, winrt::IDependencyPropertyChangedEventArgs const& args)
{
    auto targetNode = sender.as<winrt::TreeViewNode>();
    if (targetNode.IsExpanded())
    {
        if (targetNode.Children().Size() != 0)
        {
            int openedDescendantOffset = 0;
            unsigned int index;
            IndexOfNode(targetNode, index);
            index = index + 1;
            for (unsigned int i = 0; i < targetNode.Children().Size(); i++)
            {
                winrt::TreeViewNode childNode{ nullptr };
                childNode = targetNode.Children().GetAt(i).as<winrt::TreeViewNode>();
                AddNodeToView(childNode, index + i + openedDescendantOffset);
                openedDescendantOffset = AddNodeDescendantsToView(childNode, index + i, openedDescendantOffset);
            }
        }

        //Notify TreeView that a node is being expanded.
        m_nodeExpandingEventSource(targetNode, nullptr);
    }
    else
    {
        for (unsigned int i = 0; i < targetNode.Children().Size(); i++)
        {
            winrt::TreeViewNode childNode{ nullptr };
            childNode = targetNode.Children().GetAt(i).as<winrt::TreeViewNode>();
            RemoveNodeAndDescendantsFromView(childNode);
        }

        //Notify TreeView that a node is being collapsed
        m_nodeCollapsedEventSource(targetNode, nullptr);
    }
}

void ViewModel::TreeViewNodeHasChildrenPropertyChanged(winrt::TreeViewNode const& sender, winrt::IDependencyPropertyChangedEventArgs const& args)
{
    if (const auto listControl = m_TreeViewList.get())
    {
        auto targetNode = sender.as<winrt::TreeViewNode>();
        auto container = winrt::get_self<TreeViewList>(listControl)->ContainerFromNode(targetNode);
        if (container)
        {
            winrt::TreeViewItem targetItem = container.as<winrt::TreeViewItem>();
            targetItem.GlyphOpacity(targetNode.HasChildren() ? 1.0 : 0.0);
        }
    }
}

void ViewModel::IsContentMode(const bool value)
{
    m_isContentMode = value;
}

bool ViewModel::IsContentMode()
{
    return m_isContentMode;
}

void ViewModel::ClearEventTokenVectors()
{
    // Remove ChildrenChanged and ExpandedChanged events
    for (uint32_t i =0 ; i < Size(); i++)
    {
        if (auto current = ItemAt(i, true /* useSafeGet */))
        {
            auto tvnCurrent = winrt::get_self<TreeViewNode>(current.as<winrt::TreeViewNode>());
            tvnCurrent->ChildrenChanged(m_collectionChangedEventTokenVector[i]);
            tvnCurrent->RemoveExpandedChanged(m_IsExpandedChangedEventTokenVector[i]);
        }
    }

    // Remove SelectedNodeChildrenChangedEvent
    if (auto selectedNodes = m_selectedNodes.safe_get())
    {
        for (uint32_t i = 0; i < selectedNodes.Size(); i++)
        {
            if (auto current = selectedNodes.GetAt(i))
            {
                auto node = winrt::get_self<TreeViewNode>(current.as<winrt::TreeViewNode>());
                node->ChildrenChanged(m_selectedNodeChildrenChangedEventTokenVector[i]);
            }
        }
    }

    // Clear token vectors
    m_collectionChangedEventTokenVector.clear();
    m_IsExpandedChangedEventTokenVector.clear();
    m_selectedNodeChildrenChangedEventTokenVector.clear();
}
