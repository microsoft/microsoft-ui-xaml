// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <common.h>
#include "ItemsRepeater.common.h"
#include "SelectionNode.h"
#include "SelectionModel.h"
#include "IndexPath.h"

SelectionNode::SelectionNode(SelectionModel* manager, SelectionNode* parent) :
    m_manager(manager), m_parent(parent), m_source(manager), m_dataSource(manager)
{
    m_source.set(nullptr);
    m_dataSource.set(nullptr);
}

SelectionNode::~SelectionNode()
{
    UnhookCollectionChangedHandler();
}

winrt::IInspectable SelectionNode::Source()
{
    return m_source.get();
}

void SelectionNode::Source(const winrt::IInspectable& value)
{
    if (m_source.get() != value)
    {
        ClearSelection();
        UnhookCollectionChangedHandler();

        m_source.set(value);

        // Setup ItemsSourceView
        auto newDataSource = value.try_as<winrt::ItemsSourceView>();
        if (value && !newDataSource)
        {
            newDataSource = winrt::ItemsSourceView(value);
        }

        m_dataSource.set(newDataSource);

        HookupCollectionChangedHandler();
        OnSelectionChanged();
    }
}

winrt::ItemsSourceView SelectionNode::ItemsSourceView()
{
    return m_dataSource.get();
}

int SelectionNode::DataCount()
{
    return m_dataSource == nullptr ? 0 : m_dataSource.get().Count();
}

int SelectionNode::ChildrenNodeCount()
{
    return static_cast<int>(m_childrenNodes.size());
}

int SelectionNode::RealizedChildrenNodeCount()
{
    return m_realizedChildrenNodeCount;
}

int SelectionNode::AnchorIndex()
{
    return m_anchorIndex;
}

void SelectionNode::AnchorIndex(int value)
{
    m_anchorIndex = value;
}

winrt::IndexPath SelectionNode::IndexPath()
{
    std::vector<int> path;
    auto parent = m_parent;
    auto child = this;
    while (parent != nullptr)
    {
        auto childNodes = parent->m_childrenNodes;
        auto it = std::find_if(childNodes.cbegin(), childNodes.cend(), [&child](const auto& item) {return item.get() == child;});
        const auto index = static_cast<int>(distance(childNodes.cbegin(), it));
        assert(index >= 0);
        // we are walking up to the parent, so the path will be backwards
        path.insert(path.begin(), index);
        child = parent;
        parent = parent->m_parent;
    }

    return winrt::make<::IndexPath>(path);
}

// For a genuine tree view, we dont know which node is leaf until we 
// actually walk to it, so currently the tree builds up to the leaf. I don't 
// create a bunch of leaf node instances - instead i use the same instance m_leafNode to avoid 
// an explosion of node objects. However, I'm still creating the m_childrenNodes 
// collection unfortunately.
std::shared_ptr<SelectionNode> SelectionNode::GetAt(int index, bool realizeChild)
{
    std::shared_ptr<SelectionNode> child = nullptr;
    if (realizeChild)
    {
        if (m_childrenNodes.size() == 0)
        {
            if (m_dataSource != nullptr)
            {
                for (int i = 0; i < m_dataSource.get().Count(); i++)
                {
                    m_childrenNodes.emplace_back(nullptr);
                }
            }
        }

        MUX_ASSERT(0 <= index && index <= static_cast<int>(m_childrenNodes.size()));

        if (m_childrenNodes.at(index) == nullptr)
        {
            auto childData = m_dataSource.get().GetAt(index);
            if (childData != nullptr)
            {
                auto const childDataIndexPath = winrt::get_self<class IndexPath>(IndexPath())->CloneWithChildIndex(index);
                auto resolvedChild = m_manager->ResolvePath(childData, childDataIndexPath);
                if (resolvedChild != nullptr)
                {
                    child = std::make_shared<SelectionNode>(m_manager, this /* parent */);
                    child->Source(resolvedChild);
                }
                else
                {
                    child = m_manager->SharedLeafNode();
                }
            }
            else
            {
                child = m_manager->SharedLeafNode();
            }

            m_childrenNodes[index] = child;
            m_realizedChildrenNodeCount++;
        }
        else
        {
            child = m_childrenNodes[index];
        }
    }
    else
    {
        if (m_childrenNodes.size() > 0)
        {
            MUX_ASSERT(0 <= index && index <= static_cast<int>(m_childrenNodes.size()));
            child = m_childrenNodes[index];
        }
    }

    return child;
}

int SelectionNode::SelectedCount()
{
    return m_selectedCount;
}

bool SelectionNode::IsSelected(int index)
{
    bool isSelected = false;
    for (auto& range : m_selected)
    {
        if (range.Contains(index))
        {
            isSelected = true;
            break;
        }
    }

    return isSelected;
}

// True  -> Selected
// False -> Not Selected
// Null  -> Some descendents are selected and some are not
winrt::IReference<bool> SelectionNode::IsSelectedWithPartial()
{
    auto isSelected = winrt::PropertyValue::CreateBoolean(false).as<winrt::IReference<bool>>();
    if (m_parent)
    {
        auto parentsChildren = m_parent->m_childrenNodes;
        const auto it = std::find_if(parentsChildren.cbegin(), parentsChildren.cend(), [this](const std::shared_ptr<SelectionNode>& node) { return node.get() == this; });
        if (it != parentsChildren.end())
        {
            auto myIndexInParent = static_cast<int>(it - parentsChildren.begin());
            isSelected = m_parent->IsSelectedWithPartial(myIndexInParent);
        }
    }

    return isSelected;
}

// True  -> Selected
// False -> Not Selected
// Null  -> Some descendents are selected and some are not
winrt::IReference<bool> SelectionNode::IsSelectedWithPartial(int index)
{
    SelectionState selectionState = SelectionState::NotSelected;
    MUX_ASSERT(index >= 0);

    if (m_childrenNodes.size() == 0 || // no nodes realized
        static_cast<int>(m_childrenNodes.size()) <= index || // target node is not realized 
        !m_childrenNodes[index] || // target node is not realized
        m_childrenNodes[index] == m_manager->SharedLeafNode())  // target node is a leaf node.
    {
        // Ask parent if the target node is selected.
        selectionState = IsSelected(index) ? SelectionState::Selected : SelectionState::NotSelected;
    }
    else
    {
        // targetNode is the node representing the index. This node is the parent. 
        // targetNode is a non-leaf node, containing one or many children nodes. Evaluate 
        // based on children of targetNode.
        auto targetNode = m_childrenNodes[index];
        selectionState = targetNode->EvaluateIsSelectedBasedOnChildrenNodes();
    }

    return ConvertToNullableBool(selectionState);
}

int SelectionNode::SelectedIndex()
{
    return SelectedCount() > 0 ? SelectedIndices().at(0) : -1;
}

void SelectionNode::SelectedIndex(int value)
{
    if (IsValidIndex(value) && (SelectedCount() != 1 || !IsSelected(value)))
    {
        ClearSelection();

        if (value != -1)
        {
            Select(value, true);
        }
    }
}

std::vector<int> SelectionNode::SelectedIndices()
{
    if (!m_selectedIndicesCacheIsValid)
    {
        m_selectedIndicesCacheIsValid = true;
        for (auto& range : m_selected)
        {
            for (int index = range.Begin(); index <= range.End(); index++)
            {
                // Avoid duplicates
                if (std::find(m_selectedIndicesCached.begin(), m_selectedIndicesCached.end(), index) == m_selectedIndicesCached.end())
                {
                    m_selectedIndicesCached.emplace_back(index);
                }
            }
        }

        // Sort the list for easy consumption
        std::sort(m_selectedIndicesCached.begin(), m_selectedIndicesCached.end());
    }

    return m_selectedIndicesCached;
}

bool SelectionNode::Select(int index, bool select)
{
    return Select(index, select, true /* raiseOnSelectionChanged */);
}

bool SelectionNode::ToggleSelect(int index)
{
    return Select(index, !IsSelected(index));
}

void SelectionNode::SelectAll()
{
    if (m_dataSource)
    {
        auto size = m_dataSource.get().Count();
        if (size > 0)
        {
            SelectRange(IndexRange(0, size - 1), true /* select */);
        }
    }
}

void SelectionNode::Clear()
{
    ClearSelection();
}

bool SelectionNode::SelectRange(const IndexRange& range, bool select)
{
    if (IsValidIndex(range.Begin()) && IsValidIndex(range.End()))
    {
        if (select)
        {
            AddRange(range, true /* raiseOnSelectionChanged */);
        }
        else
        {
            RemoveRange(range, true /* raiseOnSelectionChanged */);
        }

        return true;
    }

    return false;
}

void SelectionNode::HookupCollectionChangedHandler()
{
    if (m_dataSource)
    {
        m_itemsSourceViewChanged = m_dataSource.get().CollectionChanged(winrt::auto_revoke, { this, &SelectionNode::OnSourceListChanged });
    }
}

void SelectionNode::UnhookCollectionChangedHandler()
{
        m_itemsSourceViewChanged.revoke();
}

bool SelectionNode::IsValidIndex(int index)
{
    return (ItemsSourceView() == nullptr || (index >= 0 && index < ItemsSourceView().Count()));
}

void SelectionNode::AddRange(const IndexRange& addRange, bool raiseOnSelectionChanged)
{
    // TODO: Check for duplicates (Task 14107720)
    // TODO: Optimize by merging adjacent ranges (Task 14107720)

    int oldCount = SelectedCount();

    for (int i = addRange.Begin(); i <= addRange.End(); i++)
    {
        if (!IsSelected(i))
        {
            m_selectedCount++;
        }
    }

    if (oldCount != m_selectedCount)
    {
        m_selected.emplace_back(addRange);

        if (raiseOnSelectionChanged)
        {
            OnSelectionChanged();
        }
    }
}

void SelectionNode::RemoveRange(const IndexRange& removeRange, bool raiseOnSelectionChanged)
{
    int oldCount = m_selectedCount;

    // TODO: Prevent overlap of Ranges in _selected (Task 14107720)
    for (int i = removeRange.Begin(); i <= removeRange.End(); i++)
    {
        if (IsSelected(i))
        {
            m_selectedCount--;
        }
    }

    if (oldCount != m_selectedCount)
    {
        // Build up a both a list of Ranges to remove and ranges to add
        std::vector<IndexRange> toRemove;
        std::vector<IndexRange> toAdd;

        for (IndexRange& range : m_selected)
        {
            // If this range intersects the remove range, we have to do something
            if (removeRange.Intersects(range))
            {
                IndexRange before(-1, -1);
                IndexRange cut(-1, -1);
                IndexRange after(-1, -1);

                // Intersection with the beginning of the range
                //  Anything to the left of the point (exclusive) stays
                //  Anything to the right of the point (inclusive) gets clipped
                if (range.Contains(removeRange.Begin() - 1))
                {
                    range.Split(removeRange.Begin() - 1, before, cut);
                    toAdd.emplace_back(before);
                }

                // Intersection with the end of the range
                //  Anything to the left of the point (inclusive) gets clipped
                //  Anything to the right of the point (exclusive) stays
                if (range.Contains(removeRange.End()))
                {
                    if (range.Split(removeRange.End(), cut, after))
                    {
                        toAdd.emplace_back(after);
                    }
                }

                // Remove this Range from the collection
                // New ranges will be added for any remaining subsections
                toRemove.emplace_back(range);
            }
        }

        bool change = ((toRemove.size() > 0) || (toAdd.size() > 0));

        if (change)
        {
            // Remove tagged ranges
            for (IndexRange& remove : toRemove)
            {
                auto iter = std::find(m_selected.begin(), m_selected.end(), remove);
                m_selected.erase(iter);
            }

            // Add new ranges
            for (IndexRange& add : toAdd)
            {
                m_selected.emplace_back(add);
            }

            if (raiseOnSelectionChanged)
            {
                OnSelectionChanged();
            }
        }
    }
}

void SelectionNode::ClearSelection()
{
    // Deselect all items
    if (m_selected.size() > 0)
    {
        m_selected.clear();
        OnSelectionChanged();
    }

    m_selectedCount = 0;
    AnchorIndex(-1);

    // This will throw away all the children SelectionNodes
    // causing them to be unhooked from their data source. This
    // essentially cleans up the tree.
    m_childrenNodes.clear();
}

bool SelectionNode::Select(int index, bool select, bool raiseOnSelectionChanged)
{
    if (IsValidIndex(index))
    {
        // Ignore duplicate selection calls
        if (IsSelected(index) == select)
        {
            return true;
        }

        auto range = IndexRange(index, index);

        if (select)
        {
            AddRange(range, raiseOnSelectionChanged);
        }
        else
        {
            RemoveRange(range, raiseOnSelectionChanged);
        }

        return true;
    }

    return false;
}

void SelectionNode::OnSourceListChanged(const winrt::IInspectable& dataSource, const winrt::NotifyCollectionChangedEventArgs& args)
{
    bool selectionInvalidated = false;
    switch (args.Action())
    {
        case winrt::NotifyCollectionChangedAction::Add:
        {
            selectionInvalidated = OnItemsAdded(args.NewStartingIndex(), args.NewItems().Size());
            break;
        }

        case winrt::NotifyCollectionChangedAction::Remove:
        {
            selectionInvalidated = OnItemsRemoved(args.OldStartingIndex(), args.OldItems().Size());
            break;
        }

        case winrt::NotifyCollectionChangedAction::Reset:
        {
            ClearSelection();
            selectionInvalidated = true;
            break;
        }

        case winrt::NotifyCollectionChangedAction::Replace:
        {
            selectionInvalidated = OnItemsRemoved(args.OldStartingIndex(), args.OldItems().Size());
            selectionInvalidated |= OnItemsAdded(args.NewStartingIndex(), args.NewItems().Size());
            break;
        }
    }

    if (selectionInvalidated)
    {
        OnSelectionChanged();
        m_manager->OnSelectionInvalidatedDueToCollectionChange();
    }
}

bool SelectionNode::OnItemsAdded(int index, int count)
{
    bool selectionInvalidated = false;
    // Update ranges for leaf items
    std::vector<IndexRange> toAdd;
    for (int i = 0; i < static_cast<int>(m_selected.size()); i++)
    {
        auto range = m_selected[i];

        // The range is after the inserted items, need to shift the range right
        if (range.End() >= index)
        {
            int begin = range.Begin();
            // If the index left of newIndex is inside the range,
            // Split the range and remember the left piece to add later
            if (range.Contains(index - 1))
            {
                IndexRange before(-1, -1), after(-1, -1);
                range.Split(index - 1, before, after);
                toAdd.emplace_back(before);
                begin = index;
            }

            // Shift the range to the right
            m_selected[i] = IndexRange(begin + count, range.End() + count);
            selectionInvalidated = true;
        }
    }

    if (toAdd.size() > 0)
    {
        // Add the left sides of the split ranges
        for (auto& add : toAdd)
        {
            m_selected.emplace_back(add);
        }
    }

    // Update for non-leaf if we are tracking non-leaf nodes
    if (m_childrenNodes.size() > 0)
    {
        selectionInvalidated = true;
        for (int i = 0; i < count; i++)
        {
            m_childrenNodes.insert(m_childrenNodes.begin() + index, nullptr);
        }
    }

    //Adjust the anchor
    if (AnchorIndex() >= index)
    {
        AnchorIndex(AnchorIndex() + count);
    }

    // Check if adding a node invalidated an ancestors
    // selection state. For example if parent was selected before
    // adding a new item makes the parent partially selected now.
    if (!selectionInvalidated)
    {
        auto parent = m_parent;
        while (parent)
        {
            auto isSelected = parent->IsSelectedWithPartial();
            // If a parent is selected, then it will become partially selected.
            // If it is not selected or partially selected - there is no change.
            if (isSelected && isSelected.Value())
            {
                selectionInvalidated = true;
                break;
            }

            parent = parent->m_parent;
        }
    }

    return selectionInvalidated;
}

bool SelectionNode::OnItemsRemoved(int index, int count)
{
    bool selectionInvalidated = false;
    // Remove the items from the selection for leaf
    if (ItemsSourceView().Count() > 0)
    {
        bool isSelected = false;
        for (int i = index; i <= index + count - 1; i++)
        {
            if (IsSelected(i))
            {
                isSelected = true;
                break;
            }
        }

        if (isSelected)
        {
            RemoveRange(IndexRange(index, index + count - 1), false /* raiseOnSelectionChanged */);
            selectionInvalidated = true;
        }

        for (int i = 0; i < static_cast<int>(m_selected.size()); i++)
        {
            auto range = m_selected[i];

            // The range is after the removed items, need to shift the range left
            if (range.End() > index)
            {
                MUX_ASSERT(!range.Contains(index));

                // Shift the range to the left
                m_selected[i] = IndexRange(range.Begin() - count, range.End() - count);
                selectionInvalidated = true;
            }
        }

        // Update for non-leaf if we are tracking non-leaf nodes
        if (m_childrenNodes.size() > 0)
        {
            selectionInvalidated = true;
            for (int i = 0; i < count; i++)
            {
                if (m_childrenNodes[index])
                {
                    m_realizedChildrenNodeCount--;
                }
                m_childrenNodes.erase(m_childrenNodes.begin() + index);
            }
        }

        //Adjust the anchor
        if (AnchorIndex() >= index)
        {
            AnchorIndex(AnchorIndex() - count);
        }
    }
    else
    {
        // No more items in the list, clear
        ClearSelection();
        m_realizedChildrenNodeCount = 0;
        selectionInvalidated = true;
    }

    // Check if removing a node invalidated an ancestors
    // selection state. For example if parent was partially selected before
    // removing an item, it could be selected now.
    if (!selectionInvalidated)
    {
        auto parent = m_parent;
        while (parent)
        {
            auto isSelected = parent->IsSelectedWithPartial();
            // If a parent is partially selected, then it will become selected.
            // If it is selected or not selected - there is no change.
            if (!isSelected)
            {
                selectionInvalidated = true;
                break;
            }

            parent = parent->m_parent;
        }
    }

    return selectionInvalidated;
}

void SelectionNode::OnSelectionChanged()
{
    m_selectedIndicesCacheIsValid = false;
    m_selectedIndicesCached.clear();
}

/* static */
winrt::IReference<bool> SelectionNode::ConvertToNullableBool(SelectionState isSelected)
{
    winrt::IReference<bool> result = nullptr; // PartialySelected
    if (isSelected == SelectionState::Selected)
    {
        result = winrt::PropertyValue::CreateBoolean(true).as<winrt::IReference<bool>>();
    }
    else if (isSelected == SelectionState::NotSelected)
    {
        result = winrt::PropertyValue::CreateBoolean(false).as<winrt::IReference<bool>>();
    }

    return result;
}

SelectionState SelectionNode::EvaluateIsSelectedBasedOnChildrenNodes()
{
    SelectionState selectionState = SelectionState::NotSelected;
    int realizedChildrenNodeCount = RealizedChildrenNodeCount();
    int selectedCount = SelectedCount();

    if (realizedChildrenNodeCount != 0 || selectedCount != 0)
    {
        // There are realized children or some selected leaves.
        int dataCount = DataCount();
        if (realizedChildrenNodeCount == 0 && selectedCount > 0)
        {
            // All nodes are leaves under it - we didn't create children nodes as an optimization.
            // See if all/some or none of the leaves are selected.
            selectionState = dataCount != selectedCount ?
                SelectionState::PartiallySelected :
                dataCount == selectedCount ? SelectionState::Selected : SelectionState::NotSelected;
        }
        else
        {
            // There are child nodes, walk them individually and evaluate based on each child
            // being selected/not selected or partially selected.
            bool isSelected = false;
            selectedCount = 0;
            int notSelectedCount = 0;
            for (int i = 0; i < ChildrenNodeCount(); i++)
            {
                if (auto child = GetAt(i, false /* realizeChild */))
                {
                    // child is realized, ask it.
                    auto isChildSelected = IsSelectedWithPartial(i);
                    if (isChildSelected == nullptr)
                    {
                        selectionState = SelectionState::PartiallySelected;
                        break;
                    }
                    else if (isChildSelected.Value())
                    {
                        selectedCount++;
                    }
                    else
                    {
                        notSelectedCount++;
                    }
                }
                else
                {
                    // not realized.
                    if (IsSelected(i))
                    {
                        selectedCount++;
                    }
                    else
                    {
                        notSelectedCount++;
                    }
                }

                if (selectedCount > 0 && notSelectedCount > 0)
                {
                    selectionState = SelectionState::PartiallySelected;
                    break;
                }
            }

            if (selectionState != SelectionState::PartiallySelected)
            {
                if (selectedCount != 0 && selectedCount != dataCount)
                {
                    selectionState = SelectionState::PartiallySelected;
                }
                else
                {
                    selectionState = selectedCount == dataCount ? SelectionState::Selected : SelectionState::NotSelected;
                }
            }
        }
    }

    return selectionState;
}
