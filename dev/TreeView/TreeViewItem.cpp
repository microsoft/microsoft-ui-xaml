﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TreeViewItem.h"
#include "TreeViewItemAutomationPeer.h"
#include "TreeViewList.h"
#include "TreeViewItemTemplateSettings.h"

TreeViewItem::TreeViewItem()
{
    SetDefaultStyleKey(this);
    SetValue(s_TreeViewItemTemplateSettingsProperty, winrt::make<::TreeViewItemTemplateSettings>());
}

TreeViewItem::~TreeViewItem()
{
    // We only need to use safe_get in the deconstruction loop
    RecycleEvents(true /* useSafeGet */);
}

// IControlOverrides
void TreeViewItem::OnKeyDown(winrt::KeyRoutedEventArgs const& e)
{
    if (const auto targetNode = TreeNode())
    {
        const auto treeView = AncestorTreeView();
        const auto key = e.Key();
        const auto originalKey = e.OriginalKey();
        
        // If in multi-selection and gamepad a is pressed
        if (originalKey == winrt::VirtualKey::GamepadA &&
            treeView->ListControl()->IsMultiselect())
        {
            HandleGamepadAInMultiselectMode(targetNode);
            e.Handled(true);
        }
        else if (IsInReorderMode(key))
        {
            HandleReorder(key);
            e.Handled(true);
        }
        // Collapse/Expand
        else if (IsExpandCollapse(key))
        {
            const auto handled = HandleExpandCollapse(key);
            if (handled)
            {
                e.Handled(true);
            }
        }
        else if (key == winrt::VirtualKey::Space &&
                 treeView->ListControl()->IsMultiselect())
        {
            const auto selectionBox = m_selectionBox.get();
            const bool isSelected = CheckBoxSelectionState(selectionBox) == TreeNodeSelectionState::Selected;
            selectionBox.IsChecked(!isSelected);
            e.Handled(true);
        }
    }

    // Call to base for all other key presses
    __super::OnKeyDown(e);
}

void TreeViewItem::OnDrop(winrt::DragEventArgs const& args)
{
    if (!args.Handled() && args.AcceptedOperation() == winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Move)
    {
        const winrt::TreeViewItem droppedOnItem = *this;
        const auto treeView = AncestorTreeView();
        if (treeView)
        {
            const auto treeViewList = treeView->ListControl();
            if (const auto droppedNode = treeViewList->DraggedTreeViewNode())
            {
                if (treeViewList->IsMutiSelectWithSelectedItems())
                {
                    const auto droppedOnNode = treeView->NodeFromContainer(droppedOnItem);
                    const auto selectedRoots = treeViewList->GetRootsOfSelectedSubtrees();
                    for (const auto node : selectedRoots)
                    {
                        const auto nodeIndex = treeViewList->FlatIndex(node);
                        if (treeViewList->IsFlatIndexValid(nodeIndex))
                        {
                            treeViewList->RemoveNodeFromParent(node);
                            winrt::get_self<TreeViewNodeVector>(droppedOnNode.Children())->Append(node);
                        }
                    }

                    args.Handled(true);
                    treeView->MutableListControl()->OnDrop(args);
                }
                else
                {
                    const auto droppedOnNode = treeView->NodeFromContainer(droppedOnItem);

                    // Remove the item that was dragged
                    unsigned int removeIndex;
                    droppedNode.Parent().Children().IndexOf(droppedNode, removeIndex);

                    if (droppedNode != droppedOnNode)
                    {
                        winrt::get_self<TreeViewNodeVector>(droppedNode.Parent().Children())->RemoveAt(removeIndex);

                        // Append the dragged dropped item as a child of the node it was dropped onto
                        winrt::get_self<TreeViewNodeVector>(droppedOnNode.Children())->Append(droppedNode);

                        // If not set to true then the Reorder code of listview will override what is being done here.
                        args.Handled(true);
                        treeView->MutableListControl()->OnDrop(args);
                    }
                    else
                    {
                        args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::None);
                    }
                    
                }
            }
        }
    }

    __super::OnDrop(args);
}

void TreeViewItem::OnDragOver(winrt::DragEventArgs const& args)
{
    const auto treeView = AncestorTreeView();
    if (treeView && !args.Handled())
    {
        const auto treeViewList = treeView->ListControl();
        winrt::TreeViewItem draggedOverItem = *this;
        winrt::TreeViewNode draggedOverNode = treeView->NodeFromContainer(draggedOverItem);
        winrt::TreeViewNode draggedNode = treeViewList->DraggedTreeViewNode();

        if (draggedNode && treeView->CanReorderItems())
        {
            if (treeViewList->IsMutiSelectWithSelectedItems())
            {
                if (treeViewList->IsSelected(draggedOverNode))
                {
                    args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::None);
                    treeView->MutableListControl()->SetDraggedOverItem(nullptr);
                }
                else
                {
                    args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Move);
                    treeView->MutableListControl()->SetDraggedOverItem(draggedOverItem);
                }
            }
            else
            {
                winrt::TreeViewNode walkNode = draggedOverNode.Parent();
                while (walkNode && walkNode != draggedNode)
                {
                    walkNode = walkNode.Parent();
                }

                const auto mutableTreeViewList = treeView->MutableListControl();
                if (walkNode != draggedNode && draggedNode != draggedOverNode)
                {
                    args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Move);
                    mutableTreeViewList->SetDraggedOverItem(draggedOverItem);
                }

                mutableTreeViewList->UpdateDropTargetDropEffect(false, false, nullptr);
            }
        }
    }

    __super::OnDragOver(args);
}

void TreeViewItem::OnDragEnter(winrt::DragEventArgs const& args)
{
    winrt::TreeViewItem draggedOverItem = *this;

    args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::None);
    args.DragUIOverride().IsGlyphVisible(true);

    const auto treeView = AncestorTreeView();
    if (treeView && treeView->CanReorderItems() && !args.Handled())
    {
        const auto treeViewList = treeView->ListControl();
        winrt::TreeViewNode draggedNode = treeViewList->DraggedTreeViewNode();
        if (draggedNode)
        {
            winrt::TreeViewNode draggedOverNode = treeView->NodeFromContainer(draggedOverItem);
            winrt::TreeViewNode walkNode = draggedOverNode.Parent();

            while (walkNode && walkNode != draggedNode)
            {
                walkNode = walkNode.Parent();
            }

            if (walkNode != draggedNode && draggedNode != draggedOverNode)
            {
                args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Move);
            }

            winrt::TreeViewNode droppedOnNode = TreeNode();
            if (droppedOnNode != draggedNode) // Don't expand if drag hovering on itself.
            {
                // Set up timer.
                if (!draggedOverNode.IsExpanded() && draggedOverNode.HasChildren())
                {
                    if (m_expandContentTimer)
                    {
                        const auto expandContentTimer = m_expandContentTimer.get();
                        expandContentTimer.Stop();
                        expandContentTimer.Start();
                    }
                    else
                    {
                        // Initialize timer.
                        const winrt::TimeSpan interval = winrt::TimeSpan::duration(c_dragOverInterval);
                        const auto expandContentTimer = winrt::DispatcherTimer();
                        m_expandContentTimer.set(expandContentTimer);
                        expandContentTimer.Interval(interval);
                        expandContentTimer.Tick({ this, &TreeViewItem::OnExpandContentTimerTick });
                        expandContentTimer.Start();
                    }
                }
            }
        }
    }

    __super::OnDragEnter(args);
}

void TreeViewItem::OnDragLeave(winrt::DragEventArgs const& args)
{
    if (!args.Handled())
    {
        if (auto treeView = AncestorTreeView())
        {
            const auto mutableTreeViewList = treeView->MutableListControl();
            mutableTreeViewList->SetDraggedOverItem(nullptr);
        }

        if (m_expandContentTimer)
        {
            m_expandContentTimer.get().Stop();
        }
    }

    __super::OnDragLeave(args);
}

// IUIElementOverrides
winrt::AutomationPeer TreeViewItem::OnCreateAutomationPeer()
{
    return winrt::make<TreeViewItemAutomationPeer>(*this);
}

// IFrameworkElementOverrides
void TreeViewItem::OnApplyTemplate()
{
    RecycleEvents();

    winrt::IControlProtected controlProtected = *this;
    m_selectionBox.set(GetTemplateChildT<winrt::CheckBox>(c_multiSelectCheckBoxName, controlProtected));
    RegisterPropertyChangedCallback(winrt::SelectorItem::IsSelectedProperty(), { this, &TreeViewItem::OnIsSelectedChanged });

    if (m_selectionBox)
    {
        m_checkedEventRevoker = m_selectionBox.get().Checked(winrt::auto_revoke, { this, &TreeViewItem::OnCheckToggle });
        m_uncheckedEventRevoker = m_selectionBox.get().Unchecked(winrt::auto_revoke, { this, &TreeViewItem::OnCheckToggle });
    }

    const auto chevron = GetTemplateChildT<winrt::UIElement>(c_expandCollapseChevronName, controlProtected);
    if (chevron)
    {
        m_expandCollapseChevronPointerPressedToken = chevron.PointerPressed({ this, &TreeViewItem::OnExpandCollapseChevronPointerPressed });
        m_expandCollapseChevron.set(chevron);
    }
    const auto node = TreeNode();
    if (node && IsInContentMode())
    {
        UpdateNodeIsExpandedAsync(node, IsExpanded());
        node.HasUnrealizedChildren(HasUnrealizedChildren());
    }

    __super::OnApplyTemplate();
}

template<typename T>
T TreeViewItem::GetAncestorView()
{
    winrt::DependencyObject treeViewItemAncestor = *this;
    T ancestorView = nullptr;
    while (treeViewItemAncestor && !ancestorView)
    {
        treeViewItemAncestor = winrt::VisualTreeHelper::GetParent(treeViewItemAncestor);
        ancestorView = treeViewItemAncestor.try_as<T>();
    }
    return ancestorView;
}

com_ptr<TreeView> TreeViewItem::AncestorTreeView()
{
    if (!m_ancestorTreeView.get())
    {
        m_ancestorTreeView = winrt::make_weak<winrt::TreeView>(GetAncestorView<winrt::TreeView>());
    }
    return winrt::get_self<TreeView>(m_ancestorTreeView.get())->get_strong();
}

void TreeViewItem::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (auto node = TreeNode()) 
    {
        if (property == s_IsExpandedProperty)
        {
            const bool value = unbox_value<bool>(args.NewValue());
            if (node.IsExpanded() != value)
            {
                UpdateNodeIsExpandedAsync(node, value);
            }
            RaiseExpandCollapseAutomationEvent(value);
        }
        else if (property == s_ItemsSourceProperty)
        {
            SetItemsSource(node, args.NewValue());
        }
        else if (property == s_HasUnrealizedChildrenProperty)
        {
            const bool value = unbox_value<bool>(args.NewValue());
            node.HasUnrealizedChildren(value);
        }
    }
}

void TreeViewItem::SetItemsSource(winrt::TreeViewNode const& node, winrt::IInspectable const& value)
{
    const auto treeViewNode = winrt::get_self<TreeViewNode>(node);
    treeViewNode->ItemsSource(value);
    if (IsInContentMode())
    {
        // The children have changed, validate and update GlyphOpacity
        const bool hasChildren = HasUnrealizedChildren() || treeViewNode->HasChildren();
        GlyphOpacity(hasChildren ? 1.0 : 0.0);
    }
}

void TreeViewItem::OnExpandContentTimerTick(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*e*/)
{
    if (m_expandContentTimer)
    {
        m_expandContentTimer.get().Stop();
    }
    
    if (auto draggedOverNode = TreeNode())
    {
        if (draggedOverNode && !draggedOverNode.IsExpanded())
        {
            draggedOverNode.IsExpanded(true);
        }
    }
}

void TreeViewItem::RaiseExpandCollapseAutomationEvent(bool isExpanded)
{
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        const auto expandState = isExpanded ? winrt::ExpandCollapseState::Expanded : winrt::ExpandCollapseState::Collapsed;
        if (const auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
        {
            const auto treeViewItemPeer = peer.as<winrt::TreeViewItemAutomationPeer>();
            winrt::get_self<TreeViewItemAutomationPeer>(treeViewItemPeer)->RaiseExpandCollapseAutomationEvent(expandState);
        }
    }
}

bool TreeViewItem::IsInReorderMode(winrt::VirtualKey key)
{
    const auto ctrlState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Control);
    const bool isControlPressed = (ctrlState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    const auto altState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Menu);
    const bool isAltPressed = (altState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    const auto shiftState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Shift);
    const bool isShiftPressed = (shiftState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    const bool isDirectionPressed = IsDirectionalKey(key);

    bool canReorderItems = false;
    if (auto treeView = AncestorTreeView())
    {
        canReorderItems = treeView->CanReorderItems();
    }

    return canReorderItems && isDirectionPressed && isShiftPressed && isAltPressed && !isControlPressed;
}

void TreeViewItem::UpdateTreeViewItemVisualState(TreeNodeSelectionState const& state)
{
    if (state == TreeNodeSelectionState::Selected)
    {
        winrt::VisualStateManager::GoToState(*this, L"TreeViewMultiSelectEnabledSelected", false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"TreeViewMultiSelectEnabledUnselected", false);
    }
}

void TreeViewItem::OnCheckToggle(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& /*args*/)
{
    if (const auto treeView = AncestorTreeView())
    {
        const auto listControl = treeView->ListControl();
        const int index = listControl->IndexFromContainer(*this);
        const auto selectionState = CheckBoxSelectionState(sender.as<winrt::CheckBox>());
        listControl->ListViewModel()->SelectByIndex(index, selectionState);
        UpdateTreeViewItemVisualState(selectionState);
        RaiseSelectionChangeEvents(selectionState == TreeNodeSelectionState::Selected);
    }
}

void TreeViewItem::RaiseSelectionChangeEvents(bool isSelected)
{
    if (winrt::AutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
    {
        const auto treeViewItemPeer = winrt::get_self<TreeViewItemAutomationPeer>(peer.as<winrt::TreeViewItemAutomationPeer>());

        const auto selectionEvent = isSelected ? winrt::AutomationEvents::SelectionItemPatternOnElementAddedToSelection : winrt::AutomationEvents::SelectionItemPatternOnElementRemovedFromSelection;
        treeViewItemPeer->RaiseAutomationEvent(selectionEvent);

        const auto isSelectedProperty = winrt::SelectionItemPatternIdentifiers::IsSelectedProperty();
        treeViewItemPeer->RaisePropertyChangedEvent(isSelectedProperty, box_value(!isSelected).as<winrt::IReference<bool>>(), box_value(isSelected).as<winrt::IReference<bool>>());
    }
}

void TreeViewItem::UpdateSelection(bool isSelected)
{
    if (const auto treeView = AncestorTreeView())
    {
        if (const auto node = TreeNode())
        {
            treeView->UpdateSelection(node, isSelected);
        }
    }
}

void TreeViewItem::UpdateSelectionVisual(TreeNodeSelectionState const& state)
{
    if (const auto treeView = AncestorTreeView())
    {
        if (const auto listControl = treeView->ListControl())
        {
            if (listControl->IsMultiselect())
            {
                UpdateMultipleSelection(state);
            }
            else
            {
                if (const auto node = TreeNode())
                {
                    const auto viewModel = listControl->ListViewModel();
                    const auto isNodeSelected = viewModel->IsNodeSelected(node);
                    if (isNodeSelected != IsSelected())
                    {
                        IsSelected(isNodeSelected);
                    }
                }
            }
        }
    }
}

void TreeViewItem::OnIsSelectedChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    UpdateSelection(unbox_value<bool>(GetValue(args)));
}

void TreeViewItem::UpdateMultipleSelection(TreeNodeSelectionState const& state)
{
    switch(state)
    {
    case TreeNodeSelectionState::Selected:
        m_selectionBox.get().IsChecked(true);
        break;
    case TreeNodeSelectionState::PartialSelected:
        m_selectionBox.get().IsChecked(nullptr);
        break;
    case TreeNodeSelectionState::UnSelected:
        m_selectionBox.get().IsChecked(false);
        break;
    }
    UpdateTreeViewItemVisualState(state);
}

bool TreeViewItem::IsSelectedInternal()
{
    // Check Selector::IsChecked for single selection since we use
    // ListView's single selection. In multiple selection we roll our own.
    bool isSelected = IsSelected(); 
    if (const auto treeView = AncestorTreeView())
    {
        const auto listControl = treeView->ListControl();
        if (listControl && listControl->IsMultiselect())
        {
            const TreeNodeSelectionState state = CheckBoxSelectionState(m_selectionBox.get());
            isSelected = state == TreeNodeSelectionState::Selected;
        }
    }

    return isSelected;
}

void TreeViewItem::UpdateIndentation(int depth)
{
    winrt::Thickness thickness;
    thickness.Left = depth * 16;
    thickness.Top = 0;
    thickness.Right = 0;
    thickness.Bottom = 0;

    const auto templateSettings = winrt::get_self<::TreeViewItemTemplateSettings>(TreeViewItemTemplateSettings());
    templateSettings->Indentation(thickness);
}

bool TreeViewItem::IsExpandCollapse(winrt::VirtualKey key)
{
    const auto ctrlState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Control);
    const bool isControlPressed = (ctrlState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    const auto altState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Menu);
    const bool isAltPressed = (altState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    const auto shiftState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Shift);
    const bool isShiftPressed = (shiftState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    const bool isDirectionPressed = IsDirectionalKey(key);

    return (isDirectionPressed && !isShiftPressed && !isAltPressed && !isControlPressed);
}

void TreeViewItem::ReorderItems(const winrt::ListView& listControl, const winrt::TreeViewNode& targetNode, int position, int childIndex, bool isForwards)
{
    const int positionModifier = isForwards ? 1 : -1;
    if (auto skippedItem = listControl.ContainerFromIndex(position + positionModifier))
    {
        skippedItem.as<winrt::TreeViewItem>().Focus(winrt::FocusState::Keyboard);
    }

    const auto parentNode = targetNode.Parent();
    const auto children = winrt::get_self<TreeViewNodeVector>(parentNode.Children());
    children->RemoveAt(childIndex);
    children->InsertAt(childIndex + positionModifier, targetNode);
    listControl.UpdateLayout();

    const auto treeView = AncestorTreeView();
    if (const auto lvi = treeView->ContainerFromNode(targetNode))
    {
        const auto targetItem = lvi.as<winrt::TreeViewItem>();
        targetItem.Focus(winrt::FocusState::Keyboard);
        winrt::get_self<TreeViewList>(listControl.as<winrt::TreeViewList>())->UpdateDropTargetDropEffect(false, false, targetItem);
        winrt::AutomationPeer ancestorPeer = winrt::FrameworkElementAutomationPeer::FromElement(listControl);
        ancestorPeer.RaiseAutomationEvent(winrt::AutomationEvents::Dropped);
    }
}

void TreeViewItem::HandleGamepadAInMultiselectMode(const winrt::TreeViewNode& node)
{
    if (node.HasChildren())
    {
        // Non-leaf node: cycle between selection and expansion
        if (!m_expansionCycled)
        {
            node.IsExpanded(!node.IsExpanded());
            m_expansionCycled = true;
        }
        else
        {
            m_expansionCycled = ToggleSelection();
        }
    }
    else
    {
        // Leaf node: toggle selection 
        ToggleSelection();
    }
}

bool TreeViewItem::ToggleSelection()
{
    const auto currentState = CheckBoxSelectionState(m_selectionBox.get());
    const auto newState = currentState == TreeNodeSelectionState::Selected ? TreeNodeSelectionState::UnSelected : TreeNodeSelectionState::Selected;
    UpdateMultipleSelection(newState);
    return newState == TreeNodeSelectionState::Selected;
}

void TreeViewItem::HandleReorder(winrt::VirtualKey key)
{
    winrt::TreeViewNode targetNode = TreeNode();

    winrt::TreeViewNode parentNode = targetNode.Parent();
    const auto listControl = AncestorTreeView()->ListControl();
    const unsigned int position = listControl->IndexFromContainer(*this);
    if (key == winrt::VirtualKey::Up || key == winrt::VirtualKey::Left && position != 0)
    {
        unsigned int childIndex;
        parentNode.Children().IndexOf(targetNode, childIndex);

        if (childIndex != 0)
        {
            if (targetNode.IsExpanded())
            {
                targetNode.IsExpanded(false);
            }

            ReorderItems(*listControl, targetNode, position, childIndex, false);
        }
    }
    else if ((key == winrt::VirtualKey::Down || key == winrt::VirtualKey::Right) && (position != listControl->Items().Size() - 1))
    {
        unsigned int childIndex;
        parentNode.Children().IndexOf(targetNode, childIndex);
        if (childIndex != parentNode.Children().Size() - 1)
        {
            if (targetNode.IsExpanded())
            {
                targetNode.IsExpanded(false);
            }

            ReorderItems(*listControl, targetNode, position, childIndex, true);
        }
    }
}

bool TreeViewItem::HandleExpandCollapse(winrt::VirtualKey key)
{
    const auto treeView = AncestorTreeView();
    const winrt::TreeViewNode targetNode = treeView->NodeFromContainer(*this);
    const auto flowDirectionReversed = (FlowDirection() == winrt::FlowDirection::RightToLeft);
    const bool isExpanded = targetNode.IsExpanded();
    bool handled = false;

    // Inputs for Collapse/Move to parent
    if ((key == winrt::VirtualKey::Left && !flowDirectionReversed) ||
        (key == winrt::VirtualKey::Right && flowDirectionReversed))
    {
        if (isExpanded)// Is expanded : need to collapse
        {
            targetNode.IsExpanded(false);
            const auto targetValue = treeView->ContainerFromNode(targetNode);
            if (targetValue)
            {
                const auto targetItem = targetValue.as<winrt::TreeViewItem>();
                targetItem.Focus(winrt::FocusState::Keyboard);
            }

            handled = true;
        }
        else // Is collapsed: need to select parent
        {
            if (auto parentNode = targetNode.Parent())
            {
                if (auto parentValue = treeView->ContainerFromNode(parentNode))
                {
                    const auto parentItem = parentValue.as<winrt::TreeViewItem>();
                    parentItem.Focus(winrt::FocusState::Keyboard);
                    handled = true;
                }
            }
        }
    }
    // Inputs for Expand/Move to first child
    else if ((key == winrt::VirtualKey::Right && !flowDirectionReversed) ||
             (key == winrt::VirtualKey::Left && flowDirectionReversed))
    {
        if (!isExpanded && targetNode.HasChildren()) // Is collapsed : need to expand
        {
            targetNode.IsExpanded(true);
            handled = true;
        }
        else if (targetNode.Children().Size() > 0) // Is expanded and has children: need to select first child (if applicable)
        {
            const auto childNode = targetNode.Children().GetAt(0).as<winrt::TreeViewNode>();
            const auto childValue = treeView->ContainerFromNode(childNode);
            if (childValue)
            {
                const auto childItem = childValue.as<winrt::TreeViewItem>();
                childItem.Focus(winrt::FocusState::Keyboard);
                handled = true;
            }
        }
    }

    return handled;
}

void TreeViewItem::OnExpandCollapseChevronPointerPressed(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    const winrt::TreeViewNode targetNode = TreeNode();
    const auto isExpanded = !targetNode.IsExpanded();
    targetNode.IsExpanded(isExpanded);

    args.Handled(true);
}

void TreeViewItem::RecycleEvents(bool useSafeGet)
{
    if (auto chevron = useSafeGet ? m_expandCollapseChevron.safe_get() : m_expandCollapseChevron.get())
    {
        if (m_expandCollapseChevronPointerPressedToken.value)
        {
            chevron.PointerCanceled(m_expandCollapseChevronPointerPressedToken);
            m_expandCollapseChevronPointerPressedToken.value = 0;
        }
    }
}

/* static */
bool constexpr TreeViewItem::IsDirectionalKey(winrt::VirtualKey key)
{
    return
        key == winrt::VirtualKey::Up ||
        key == winrt::VirtualKey::Down ||
        key == winrt::VirtualKey::Left ||
        key == winrt::VirtualKey::Right;
}

winrt::TreeViewNode TreeViewItem::TreeNode()
{
    if (auto treeView = AncestorTreeView())
    {
        return treeView->NodeFromContainer(*this);
    }

    return nullptr;
}

//Setting IsExpanded changes the itemssource collection on the listview, which cannot be done during layout.
//We schedule it on the dispatcher so that it runs after layout pass.
void TreeViewItem::UpdateNodeIsExpandedAsync(winrt::TreeViewNode const& node, bool isExpanded)
{
    const auto dispatcher = winrt::Window::Current().Dispatcher();
    const auto ignore = dispatcher.RunAsync(
        winrt::CoreDispatcherPriority::Normal,
        winrt::DispatchedHandler([node, isExpanded]()
    {
        node.IsExpanded(isExpanded);
    }));
}

bool TreeViewItem::IsInContentMode()
{
    const auto treeView = AncestorTreeView();
    const auto treeViewList = treeView->ListControl();
    return treeViewList->ListViewModel()->IsContentMode();
}

TreeNodeSelectionState TreeViewItem::CheckBoxSelectionState(winrt::CheckBox const& checkBox)
{
    winrt::IReference<bool> winrtBool = checkBox.IsChecked();
    if (winrtBool)
    {
        if (winrtBool.Value())
        {
            return TreeNodeSelectionState::Selected;
        }

        return TreeNodeSelectionState::UnSelected;
    }

    return TreeNodeSelectionState::PartialSelected;
}
