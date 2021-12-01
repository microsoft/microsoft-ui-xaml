// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Vector.h>
#include "TreeViewNode.h"

using TreeNodeSelectionState = TreeViewNode::TreeNodeSelectionState;
using ViewModelVectorOptions = typename VectorOptionsFromFlag<winrt::IInspectable, MakeVectorParam<VectorFlag::Observable, VectorFlag::DependencyObjectBase>()>;

class ViewModel : 
    public ReferenceTracker<
        ViewModel,
        reference_tracker_implements_t<typename ViewModelVectorOptions::VectorType>::type,
            typename ViewModelVectorOptions::IterableType,
            typename ViewModelVectorOptions::ObservableVectorType>,
    public ViewModelVectorOptions::IVectorOwner
{
    Implement_IObservable(ViewModelVectorOptions)
    Implement_Vector_External(ViewModelVectorOptions)
    Implement_IIterator(ViewModelVectorOptions)

public:
    ViewModel();
    ~ViewModel();

    void ExpandNode(const winrt::TreeViewNode& value);
    void CollapseNode(const winrt::TreeViewNode& value);
    winrt::event_token NodeExpanding(const winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IInspectable>& handler);
    void NodeExpanding(const winrt::event_token token);
    winrt::event_token NodeCollapsed(const winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IInspectable>& handler);
    void NodeCollapsed(const winrt::event_token token);
    void SelectAll();
    void SelectSingleItem(winrt::IInspectable const& item);
    void SelectNode(const winrt::TreeViewNode& node, bool isSelected);
    void SelectByIndex(int index, TreeNodeSelectionState const& state);
    winrt::TreeViewNode GetNodeAt(uint32_t index);
    bool IndexOfNode(winrt::TreeViewNode const& targetNode, uint32_t& index);
    void IsContentMode(const bool value);
    bool IsContentMode();

public:

    // IVector functions
    uint32_t Size();
    winrt::IInspectable GetAt(uint32_t index);
    bool IndexOf(winrt::IInspectable const& value, uint32_t& index);
    uint32_t GetMany(uint32_t const startIndex, winrt::array_view<winrt::IInspectable> values);
    winrt::IVectorView<winrt::IInspectable> ViewModel::GetView();
    void SetAt(uint32_t index, winrt::IInspectable const& value);
    void InsertAt(uint32_t index, winrt::IInspectable const& value);
    void RemoveAt(uint32_t index);
    void Append(winrt::IInspectable const& value);
    void RemoveAtEnd();
    void Clear();
    void ReplaceAll(winrt::array_view<winrt::IInspectable const> items);

public:
    void TreeViewNodeVectorChanged(const winrt::TreeViewNode& sender, const winrt::IInspectable& args);
    void SelectedNodeChildrenChanged(const winrt::TreeViewNode& sender, const winrt::IInspectable& args);
    void TreeViewNodePropertyChanged(winrt::TreeViewNode const& sender, winrt::IDependencyPropertyChangedEventArgs const& args);
    void TreeViewNodeIsExpandedPropertyChanged(winrt::TreeViewNode const& sender, winrt::IDependencyPropertyChangedEventArgs const& args);
    void TreeViewNodeHasChildrenPropertyChanged(winrt::TreeViewNode const& sender, winrt::IDependencyPropertyChangedEventArgs const& args);

    // Helper functions
    void PrepareView(winrt::TreeViewNode const& originNode);
    void SetOwners(winrt::TreeViewList const& owningList, winrt::TreeView const& owningTreeView);
    winrt::TreeViewList ListControl();
    bool IsInSingleSelectionMode();
    bool IsNodeSelected(winrt::TreeViewNode const& targetNode);
    TreeNodeSelectionState NodeSelectionState(winrt::TreeViewNode const& targetNode);
    void UpdateSelection(winrt::TreeViewNode const& selectNode, TreeNodeSelectionState const& selectionState);
    winrt::IVector<winrt::TreeViewNode> GetSelectedNodes();
    winrt::IVector<winrt::IInspectable> GetSelectedItems();
    void TrackItemSelected(winrt::IInspectable item);
    void TrackItemUnselected(winrt::IInspectable item);
    void NotifyContainerOfSelectionChange(winrt::TreeViewNode const& targetNode, TreeNodeSelectionState const& selectionState);

    winrt::TreeViewNode GetAssociatedNode(winrt::IInspectable item);

private:
    tracker_ref<winrt::IVector<winrt::TreeViewNode>> m_selectedNodes{ this };
    event_source<winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IInspectable>> m_nodeExpandingEventSource{ this };
    event_source<winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IInspectable>> m_nodeCollapsedEventSource{ this };
    std::vector<winrt::event_token> m_collectionChangedEventTokenVector;
    std::vector<winrt::event_token> m_selectedNodeChildrenChangedEventTokenVector;
    std::vector<winrt::event_token> m_IsExpandedChangedEventTokenVector;
    winrt::event_token m_rootNodeChildrenChangedEventToken;
    winrt::weak_ref<winrt::TreeViewList> m_TreeViewList{ nullptr };
    winrt::weak_ref<winrt::TreeView> m_TreeView{ nullptr };
    tracker_ref<winrt::TreeViewNode> m_originNode{ this };
    bool m_isContentMode{ false };
    tracker_ref<winrt::IVector<winrt::IInspectable>> m_selectedItems{ this };
    std::vector<winrt::weak_ref<winrt::IInspectable>> m_addedSelectedItems;
    std::vector<winrt::IInspectable> m_removedSelectedItems;
    tracker_ref<winrt::IMap<winrt::IInspectable, winrt::TreeViewNode>> m_itemToNodeMap{ this };
    uint32_t m_selectionTrackingCounter{ 0 };

    // Methods
    winrt::TreeViewNode GetRemovedChildTreeViewNodeByIndex(winrt::TreeViewNode const& node, unsigned int childIndex);
    int CountDescendants(const winrt::TreeViewNode& value);
    void AddNodeToView(const winrt::TreeViewNode& value, unsigned int index);
    int AddNodeDescendantsToView(const winrt::TreeViewNode& value, unsigned int index, int offset);
    void RemoveNodeAndDescendantsFromView(const winrt::TreeViewNode& value);
    void RemoveNodesAndDescendentsWithFlatIndexRange(unsigned int startIndex, unsigned int stopIndex);
    int GetNextIndexInFlatTree(winrt::TreeViewNode const& indexNode);
    unsigned int IndexOfNextSibling(winrt::TreeViewNode const& childNode);
    unsigned int GetExpandedDescendantCount(winrt::TreeViewNode const& parentNode);
    void UpdateNodeSelection(winrt::TreeViewNode const& selectNode, TreeNodeSelectionState const& selectionState);
    void UpdateSelectionStateOfDescendants(winrt::TreeViewNode const& targetNode, TreeNodeSelectionState const& selectionState);
    void UpdateSelectionStateOfAncestors(winrt::TreeViewNode const& targetNode);
    TreeNodeSelectionState SelectionStateBasedOnChildren(winrt::TreeViewNode const& node);
    void ClearEventTokenVectors();
    void BeginSelectionChanges();
    void EndSelectionChanges();
};
