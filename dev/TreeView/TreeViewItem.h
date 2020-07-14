// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "TreeViewItemTemplateSettings.h"
#include "TreeView.h"
#include "TreeViewNode.h"
#include "DispatcherHelper.h"

#include "TreeViewItem.g.h"
#include "TreeViewItem.properties.h"

using TreeNodeSelectionState = TreeViewNode::TreeNodeSelectionState;

// When dragging an item over a collapsed folder, we expand the folder after 1s hovering.
static int64_t const c_dragOverInterval = 1000 * 10000;

class TreeViewItem :
    public ReferenceTracker<TreeViewItem, winrt::implementation::TreeViewItemT>,
    public TreeViewItemProperties
{
public:
    TreeViewItem();
    ~TreeViewItem();

    void UpdateMultipleSelection(TreeNodeSelectionState const& state);
    void UpdateIndentation(int depth);
    bool IsSelectedInternal();

    // IControlOverrides
    void OnKeyDown(winrt::KeyRoutedEventArgs const& e);
    void OnDrop(winrt::DragEventArgs const& e);
    void OnDragOver(winrt::DragEventArgs const& e);
    void OnDragEnter(winrt::DragEventArgs const& e);
    void OnDragLeave(winrt::DragEventArgs const& e);

    // IUIElementOverrides
    winrt::AutomationPeer OnCreateAutomationPeer();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void SetItemsSource(winrt::TreeViewNode const& node, winrt::IInspectable const& value);

public:
    // IFrameworkElementOverrides
    void OnApplyTemplate();
    void UpdateSelection(bool isSelected);
    void UpdateSelectionVisual(TreeNodeSelectionState const& state);

private:
    template <typename T> T GetAncestorView();
    com_ptr<TreeView> AncestorTreeView();
    winrt::weak_ref<winrt::TreeView> m_ancestorTreeView{ nullptr };
    tracker_ref<winrt::CheckBox> m_selectionBox{ this };
    tracker_ref<winrt::DispatcherTimer> m_expandContentTimer{ this };
    winrt::CheckBox::Checked_revoker m_checkedEventRevoker{};
    winrt::CheckBox::Unchecked_revoker m_uncheckedEventRevoker{};
    bool m_expansionCycled{ false };
    tracker_ref<winrt::UIElement> m_expandCollapseChevron{ this };
    winrt::event_token m_expandCollapseChevronPointerPressedToken{};

    static constexpr PCWSTR c_multiSelectCheckBoxName = L"MultiSelectCheckBox";
    static constexpr PCWSTR c_expandCollapseChevronName = L"ExpandCollapseChevron";

    void OnExpandContentTimerTick(const winrt::IInspectable& sender, const winrt::IInspectable& e);
    void RaiseExpandCollapseAutomationEvent(bool isExpanded);
    void OnCheckToggle(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnIsSelectedChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/);
    bool IsInReorderMode(winrt::VirtualKey keys);
    bool IsExpandCollapse(winrt::VirtualKey key);
    void ReorderItems(const winrt::ListView& listControl, const winrt::TreeViewNode& targetNode, int position, int childIndex, bool isForwards);
    void HandleReorder(winrt::VirtualKey key);
    bool HandleExpandCollapse(winrt::VirtualKey key);
    void HandleGamepadAInMultiselectMode(const winrt::TreeViewNode& node);
    bool TreeViewItem::ToggleSelection();
    void OnExpandCollapseChevronPointerPressed(
        const winrt::IInspectable& /*sender*/,
        const winrt::PointerRoutedEventArgs& args);
    void RecycleEvents(bool useSafeGet = false);
    winrt::TreeViewNode TreeNode();
    void UpdateNodeIsExpandedAsync(winrt::TreeViewNode const& node, bool isExpanded);
    bool IsInContentMode();
    TreeNodeSelectionState CheckBoxSelectionState(winrt::CheckBox const& checkBox);
    void UpdateTreeViewItemVisualState(TreeNodeSelectionState const& state);
    void RaiseSelectionChangeEvents(bool isSelected);

    static bool constexpr IsDirectionalKey(winrt::VirtualKey key);

    DispatcherHelper m_dispatcherHelper{ *this };
};

