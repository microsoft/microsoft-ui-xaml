// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ViewModel.h"

#include "TreeViewList.g.h"

class TreeViewList :
    public ReferenceTracker<TreeViewList, winrt::implementation::TreeViewListT>
{
public:
    TreeViewList();

    void OnDragItemsStarting(const winrt::IInspectable& sender, const winrt::DragItemsStartingEventArgs& args);
    void OnDragItemsCompleted(const winrt::IInspectable& sender, const winrt::DragItemsCompletedEventArgs& args);
    void OnContainerContentChanging(const winrt::IInspectable& sender, const winrt::ContainerContentChangingEventArgs& args);

    // IControlOverrides
    void OnDrop(winrt::DragEventArgs const& e);
    void OnDragOver(winrt::DragEventArgs const& e);
    void OnDragEnter(winrt::DragEventArgs const& e);
    void OnDragLeave(winrt::DragEventArgs const& e);

    // IItemsControlOverrides
    void PrepareContainerForItemOverride(winrt::DependencyObject const& element, winrt::IInspectable const& item);
    winrt::DependencyObject GetContainerForItemOverride();

    // IFrameworkElementOverrides
    void OnApplyTemplate();

    // IUIElementOverrides
    winrt::AutomationPeer OnCreateAutomationPeer();

    winrt::hstring GetDropTargetDropEffect();
    void SetDraggedOverItem(winrt::TreeViewItem newDraggedOverItem);
    void UpdateDropTargetDropEffect(bool forceUpdate, bool isLeaving, winrt::TreeViewItem keyboardReorderedContainer);    
    void EnableMultiselect(bool isEnabled);
    bool IsMultiselect() const;

    bool IsMutiSelectWithSelectedItems() const;
    bool IsSelected(const winrt::TreeViewNode& node) const;
    std::vector<winrt::TreeViewNode> GetRootsOfSelectedSubtrees() const;
    int FlatIndex(const winrt::TreeViewNode& node) const;
    bool IsFlatIndexValid(int index) const;
    unsigned int RemoveNodeFromParent(const winrt::TreeViewNode& node) const;
    winrt::TreeViewNode NodeFromContainer(winrt::DependencyObject const& container) const;
    winrt::DependencyObject ContainerFromNode(winrt::TreeViewNode const& node) const;
    winrt::TreeViewNode NodeFromItem(winrt::IInspectable const& item) const;
    winrt::IInspectable ItemFromNode(winrt::TreeViewNode const& node) const;
    com_ptr<ViewModel> ListViewModel() const;
    void ListViewModel(com_ptr<ViewModel> viewModel);
    winrt::TreeViewNode DraggedTreeViewNode() const;
    void DraggedTreeViewNode(winrt::TreeViewNode const& node);
    bool IsContentMode() const;

private:
    bool IsIndexValid(int index) const;
    hstring GetAutomationName(int index) const;
    hstring BuildEffectString(hstring priorString, hstring afterString, hstring dragString, hstring dragOverString) const;
    unsigned int IndexInParent(const winrt::TreeViewNode& node) const;
    winrt::TreeViewNode NodeAtFlatIndex(int index) const;
    winrt::TreeViewNode GetRootOfSelection(const winrt::TreeViewNode& node) const;
    void MoveNodeInto(winrt::TreeViewNode const& node, winrt::TreeViewNode const& insertAtNode) const;

    tracker_ref<winrt::TreeViewItem> m_draggedOverItem{ this };
    winrt::hstring m_dropTargetDropEffectString;
    int m_emptySlotIndex{ 0 };
    bool m_itemsSourceAttached{ false };
    bool m_isMultiselectEnabled{ false };
    tracker_com_ref<ViewModel> m_viewModel{ this };
    tracker_ref<winrt::TreeViewNode> m_draggedTreeViewNode{ this };
};

