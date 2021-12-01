// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "IndexRange.h"

class SelectionModel;

enum class SelectionState
{
    Selected,
    NotSelected,
    PartiallySelected
};

// SelectionNode in the internal tree data structure that we keep track of for selection in 
// a nested scenario. This would map to one ItemsSourceView/Collection. This node reacts
// to collection changes and keeps the selected indices up to date.
// This can either be a leaf node or a non leaf node.
class SelectionNode final: public std::enable_shared_from_this<SelectionNode>
{
public:
    SelectionNode(SelectionModel* manager, SelectionNode* parent);
    ~SelectionNode();

    winrt::IInspectable Source();
    void Source(const winrt::IInspectable& value);

    winrt::ItemsSourceView ItemsSourceView();
    int DataCount();
    int ChildrenNodeCount();
    int RealizedChildrenNodeCount();
    int AnchorIndex();
    void AnchorIndex(int value);
    std::shared_ptr<SelectionNode> GetAt(int index, bool realizeChild);

    int SelectedCount();
    winrt::IReference<bool> IsSelectedWithPartial();
    winrt::IReference<bool> IsSelectedWithPartial(int index);
    bool IsSelected(int index);
    int SelectedIndex();
    void SelectedIndex(int value);
    std::vector<int> SelectedIndices();
    bool Select(int index, bool select);
    bool ToggleSelect(int index);
    void SelectAll();
    void Clear();
    bool SelectRange(const IndexRange& range, bool select);
    SelectionState EvaluateIsSelectedBasedOnChildrenNodes();
    static winrt::IReference<bool> ConvertToNullableBool(SelectionState isSelected);
    winrt::IndexPath IndexPath();

private:
    void HookupCollectionChangedHandler();
    void UnhookCollectionChangedHandler();
    bool IsValidIndex(int index);
    void AddRange(const IndexRange& addRange, bool raiseOnSelectionChanged);
    void RemoveRange(const IndexRange& removeRange, bool raiseOnSelectionChanged);
    void ClearSelection();
    bool Select(int index, bool select, bool raiseOnSelectionChanged);
    void OnSourceListChanged(const winrt::IInspectable& dataSource, const winrt::NotifyCollectionChangedEventArgs& args);
    bool OnItemsAdded(int index, int count);
    bool OnItemsRemoved(int index, int count);
    void OnSelectionChanged();

    SelectionModel* m_manager;

    // Note that a node can contain children who are leaf as well as 
    // chlidren containing leaf entries.

    // For inner nodes (any node whose children are data sources)
    std::vector<std::shared_ptr<SelectionNode>> m_childrenNodes;
    // Don't take a ref.
    SelectionNode* m_parent { nullptr };

    // For parents of leaf nodes (any node whose children are not data sources)
    std::vector<IndexRange> m_selected;
    
    tracker_ref<winrt::IInspectable> m_source;
    tracker_ref<winrt::ItemsSourceView> m_dataSource;
    winrt::ItemsSourceView::CollectionChanged_revoker m_itemsSourceViewChanged{};

    int m_selectedCount{ 0 };
    std::vector<int> m_selectedIndicesCached;
    bool m_selectedIndicesCacheIsValid = false;
    int m_anchorIndex{ -1 };
    int m_realizedChildrenNodeCount{ 0 };
};
