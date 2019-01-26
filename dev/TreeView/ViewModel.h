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
    void ModifySelectByIndex(int index, TreeNodeSelectionState const& state);
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
    void SetOwningList(winrt::ListView const& owningList);
    bool IsInSingleSelectionMode();
    bool IsNodeSelected(winrt::TreeViewNode const& targetNode);
    TreeNodeSelectionState NodeSelectionState(winrt::TreeViewNode const& targetNode);
    void UpdateSelection(winrt::TreeViewNode const& selectNode, TreeNodeSelectionState const& selectionState);
    winrt::IVector<winrt::TreeViewNode> GetSelectedNodes();
    void NotifyContainerOfSelectionChange(winrt::TreeViewNode const& targetNode, TreeNodeSelectionState const& selectionState);

private:
    tracker_ref<winrt::IVector<winrt::TreeViewNode>> m_selectedNodes{ this };
    event_source<winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IInspectable>> m_nodeExpandingEventSource{ this };
    event_source<winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IInspectable>> m_nodeCollapsedEventSource{ this };
    std::vector<winrt::event_token> m_collectionChangedEventTokenVector;
    std::vector<winrt::event_token> m_selectedNodeChildrenChangedEventTokenVector;
    std::vector<winrt::event_token> m_IsExpandedChangedEventTokenVector;
    winrt::event_token m_rootNodeChildrenChangedEventToken;
    winrt::weak_ref<winrt::ListView> m_listView{ nullptr };
    tracker_ref<winrt::TreeViewNode> m_originNode{ this };
    bool m_isContentMode{ false };

    // Methods
    winrt::TreeViewNode GetRemovedChildTreeViewNodeByIndex(winrt::TreeViewNode const& node, unsigned int childIndex);
    int CountDescendants(const winrt::TreeViewNode& value);
    void AddNodeToView(const winrt::TreeViewNode& value, unsigned int index);
    int AddNodeDescendantsToView(const winrt::TreeViewNode& value, unsigned int index, int offset);
    void RemoveNodeAndDescendantsFromView(const winrt::TreeViewNode& value);
    void RemoveNodesAndDescendentsWithFlatIndexRange(unsigned int startIndex, unsigned int stopIndex);
    int GetNextIndexInFlatTree(const winrt::TreeViewNode& indexNode);
    unsigned int IndexOfNextSibling(winrt::TreeViewNode& childNode);
    unsigned int GetExpandedDescendantCount(winrt::TreeViewNode& parentNode);
    void UpdateNodeSelection(winrt::TreeViewNode const& selectNode, TreeNodeSelectionState const& selectionState);
    void UpdateSelectionStateOfDescendants(winrt::TreeViewNode const& targetNode, TreeNodeSelectionState const& selectionState);
    void UpdateSelectionStateOfAncestors(winrt::TreeViewNode const& targetNode);
    TreeNodeSelectionState SelectionStateBasedOnChildren(winrt::TreeViewNode const& node);
    void ClearEventTokenVectors();
};
