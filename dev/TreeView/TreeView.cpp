// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TreeViewNode.h"
#include "TreeView.h"
#include "TreeViewList.h"
#include "TreeViewItem.h"
#include "TreeViewItemAutomationPeer.h"
#include "ViewModel.h"
#include "TreeViewNode.h"
#include "RuntimeProfiler.h"
#include "InspectingDataSource.h"

static constexpr auto c_listControlName = L"ListControl"sv;

TreeView::TreeView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TreeView);

    SetDefaultStyleKey(this);

    m_rootNode.set(winrt::TreeViewNode());
    m_pendingSelectedNodes.set(winrt::make<Vector<winrt::TreeViewNode>>());
}

winrt::IVector<winrt::TreeViewNode> TreeView::RootNodes()
{
    auto x = m_rootNode.get().Children();
    return x;
}

TreeViewList* TreeView::ListControl()
{
    return winrt::get_self<TreeViewList>(m_listControl.get());
}

winrt::IInspectable TreeView::ItemFromContainer(winrt::DependencyObject const& container)
{
    return ListControl() ? ListControl()->ItemFromContainer(container) : nullptr;
}

winrt::DependencyObject TreeView::ContainerFromItem(winrt::IInspectable const& item)
{
    return ListControl() ? ListControl()->ContainerFromItem(item) : nullptr;
}

winrt::TreeViewNode TreeView::NodeFromContainer(winrt::DependencyObject const& container)
{
    return ListControl() ? ListControl()->NodeFromContainer(container) : nullptr;
}

winrt::DependencyObject TreeView::ContainerFromNode(winrt::TreeViewNode const& node)
{
    return ListControl() ? ListControl()->ContainerFromNode(node) : nullptr;
}

void TreeView::SelectedNode(winrt::TreeViewNode const& node)
{
    auto selectedNodes = SelectedNodes();
    if (selectedNodes.Size() > 0)
    {
        selectedNodes.Clear();
    }
    if (node)
    {
        selectedNodes.Append(node);
    }
}

winrt::TreeViewNode TreeView::SelectedNode()
{
    auto nodes = SelectedNodes();
    return nodes.Size() > 0 ? nodes.GetAt(0) : nullptr;
}


winrt::IVector<winrt::TreeViewNode> TreeView::SelectedNodes()
{
    if (auto listControl = ListControl())
    {
        if (auto vm = listControl->ListViewModel())
        {
            return vm->GetSelectedNodes();
        }
    }
    
    // we'll treat the pending selected nodes as SelectedNodes value if we don't have a list control or a view model
    return m_pendingSelectedNodes.get();
}

winrt::IVector<winrt::IInspectable> TreeView::SelectedItems()
{
    if (auto listControl = ListControl())
    {
        if (auto viewModel = listControl->ListViewModel())
        {
            return viewModel->GetSelectedItems();
        }
    }

    return nullptr;
}

void TreeView::UpdateSelection(winrt::TreeViewNode const& node, bool isSelected)
{
    if (auto listControl = ListControl())
    {
        if (auto viewModel = listControl->ListViewModel())
        {
            if (isSelected != viewModel->IsNodeSelected(node))
            {
                viewModel->SelectNode(node, isSelected);
            }
        }
    }
}

void TreeView::Expand(winrt::TreeViewNode const& value)
{
    auto vm = ListControl()->ListViewModel();
    vm->ExpandNode(value);
}

void TreeView::Collapse(winrt::TreeViewNode const& value)
{
    auto vm = ListControl()->ListViewModel();
    vm->CollapseNode(value);
}

void TreeView::SelectAll()
{
    auto vm = ListControl()->ListViewModel();
    vm->SelectAll();
}


void TreeView::OnItemClick(const winrt::IInspectable& /*sender*/, const winrt::Windows::UI::Xaml::Controls::ItemClickEventArgs& args)
{
    auto itemInvokedArgs = winrt::make_self<TreeViewItemInvokedEventArgs>();
    itemInvokedArgs->InvokedItem(args.ClickedItem());
    m_itemInvokedEventSource(*this, *itemInvokedArgs);
}

void TreeView::OnContainerContentChanging(const winrt::IInspectable& sender, const winrt::Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs& args)
{
    m_containerContentChangedSource(sender.as<winrt::ListView>(), args);
}

void TreeView::OnNodeExpanding(const winrt::TreeViewNode& sender, const winrt::IInspectable& /*args*/)
{
    auto treeViewExpandingEventArgs = winrt::make_self<TreeViewExpandingEventArgs>();
    treeViewExpandingEventArgs->Node(sender);

    if (m_listControl)
    {
        if (auto expandingTVI = ContainerFromNode(sender).try_as<winrt::TreeViewItem>())
        {
            //Update TVI properties
            if (!expandingTVI.IsExpanded())
            {
                expandingTVI.IsExpanded(true);
            }

            //Update TemplateSettings properties
            auto templateSettings = winrt::get_self<TreeViewItemTemplateSettings>(expandingTVI.TreeViewItemTemplateSettings());
            templateSettings->ExpandedGlyphVisibility(winrt::Visibility::Visible);
            templateSettings->CollapsedGlyphVisibility(winrt::Visibility::Collapsed);
        }
        m_expandingEventSource(*this, *treeViewExpandingEventArgs);
    }
}

void TreeView::OnNodeCollapsed(const winrt::TreeViewNode& sender, const winrt::IInspectable& /*args*/)
{
    auto treeViewCollapsedEventArgs = winrt::make_self<TreeViewCollapsedEventArgs>();
    treeViewCollapsedEventArgs->Node(sender);

    if (m_listControl)
    {
        if (auto collapsedTVI = ContainerFromNode(sender).try_as<winrt::TreeViewItem>())
        {
            //Update TVI properties
            if (collapsedTVI.IsExpanded())
            {
                collapsedTVI.IsExpanded(false);
            }

            //Update TemplateSettings properties
            auto templateSettings = winrt::get_self<TreeViewItemTemplateSettings>(collapsedTVI.TreeViewItemTemplateSettings());
            templateSettings->ExpandedGlyphVisibility(winrt::Visibility::Collapsed);
            templateSettings->CollapsedGlyphVisibility(winrt::Visibility::Visible);
        }
        m_collapsedEventSource(*this, *treeViewCollapsedEventArgs);
    }
}

void TreeView::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_SelectionModeProperty && m_listControl)
    {
        winrt::TreeViewSelectionMode value = SelectionMode();
        switch (value)
        {
            case winrt::TreeViewSelectionMode::None:
            {
                m_listControl.get().SelectionMode(winrt::ListViewSelectionMode::None);
                UpdateItemsSelectionMode(false);
            }
            break;

            case winrt::TreeViewSelectionMode::Single:
            {
                m_listControl.get().SelectionMode(winrt::ListViewSelectionMode::Single);
                UpdateItemsSelectionMode(false);
            }
            break;

            case winrt::TreeViewSelectionMode::Multiple:
            {
                m_listControl.get().SelectionMode(winrt::ListViewSelectionMode::None);
                UpdateItemsSelectionMode(true);
            }
            break;

        }
    }
    else if (property == s_ItemsSourceProperty)
    {
        winrt::get_self<TreeViewNode>(m_rootNode.get())->IsContentMode(true);

        if (auto listControl = ListControl())
        {
            auto viewModel = listControl->ListViewModel();
            viewModel->IsContentMode(true);
        }

        winrt::get_self<TreeViewNode>(m_rootNode.get())->ItemsSource(ItemsSource());
    }
    else if (property == s_SelectedItemProperty)
    {

        const auto items = SelectedItems();
        const auto selected = items.Size() > 0 ? items.GetAt(0) : nullptr;
        // Checking if new value is different to the currently internally selected item
        if (args.NewValue() != selected)
        {
            if (const auto listControl = ListControl())
            {
                if (const auto viewModel = listControl->ListViewModel())
                {
                    viewModel->SelectSingleItem(args.NewValue());
                }
            }
        }
    }
}

void TreeView::OnListControlDragItemsStarting(const winrt::IInspectable& sender, const winrt::DragItemsStartingEventArgs& args)
{
    const auto treeViewArgs = winrt::make_self<TreeViewDragItemsStartingEventArgs>(args);
    m_dragItemsStartingEventSource(*this, *treeViewArgs);
}

void TreeView::OnListControlDragItemsCompleted(const winrt::IInspectable& sender, const winrt::DragItemsCompletedEventArgs& args)
{
    const auto newParent = [items = args.Items(), listControl = ListControl(), rootNode = m_rootNode.get()]()
    {
        if (listControl && items && items.Size() > 0)
        {
            if (const auto draggedNode = listControl->NodeFromItem(items.GetAt(0)))
            {
                const auto parentNode = draggedNode.Parent();
                if (parentNode && parentNode != rootNode)
                {
                    return listControl->ItemFromNode(parentNode);
                }
            }
        }
        return static_cast<winrt::IInspectable>(nullptr);
    }();

    const auto treeViewArgs = winrt::make_self<TreeViewDragItemsCompletedEventArgs>(args, newParent);
    m_dragItemsCompletedEventSource(*this, *treeViewArgs);
}

void TreeView::OnListControlSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args)
{
    if (SelectionMode() == winrt::TreeViewSelectionMode::Single)
    {
        RaiseSelectionChanged(args.AddedItems(), args.RemovedItems());

        const auto newSelectedItem = [args]() {
            if(const auto& newItems = args.AddedItems())
            {
                if (newItems.Size() > 0)
                {
                    return newItems.GetAt(0);
                }
                else
                {
                    return (winrt::IInspectable)nullptr;
                }
            }
            else
            {
                return (winrt::IInspectable)nullptr;
            }
        }();

        if (SelectedItem() != newSelectedItem)
        {
            SelectedItem(newSelectedItem);
        }
    }
}

void TreeView::UpdateItemsSelectionMode(bool isMultiSelect)
{
    auto listControl = ListControl();
    listControl->EnableMultiselect(isMultiSelect);

    auto viewModel = listControl->ListViewModel();
    int size = viewModel->Size();

    for (int i = 0; i < size; i++)
    {
        auto updateContainer = listControl->ContainerFromIndex(i).as<winrt::TreeViewItem>();
        if (updateContainer)
        {
            if (isMultiSelect)
            {
                if (auto targetNode = viewModel->GetNodeAt(i))
                {
                    if (viewModel->IsNodeSelected(targetNode))
                    {
                        winrt::VisualStateManager::GoToState(updateContainer, L"TreeViewMultiSelectEnabledSelected", false);
                    }
                    else
                    {
                        winrt::VisualStateManager::GoToState(updateContainer, L"TreeViewMultiSelectEnabledUnselected", false);
                    }
                }
            }
            else
            {
                winrt::VisualStateManager::GoToState(updateContainer, L"TreeViewMultiSelectDisabled", false);
            }
        }
    }
}

void TreeView::RaiseSelectionChanged(const winrt::IVector<winrt::IInspectable> addedItems, const winrt::IVector<winrt::IInspectable> removedItems)
{
    const auto treeViewArgs = winrt::make_self<TreeViewSelectionChangedEventArgs>(addedItems, removedItems);
    m_selectionChangedEventSource(*this, *treeViewArgs);
}

void TreeView::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected = *this;
    m_listControl.set(GetTemplateChildT<winrt::TreeViewList>(c_listControlName, controlProtected));

    if (auto listControl = m_listControl.get())
    {
        auto listPtr = winrt::get_self<TreeViewList>(m_listControl.get());
        auto viewModel = listPtr->ListViewModel();
        if (!m_rootNode.get())
        {
            m_rootNode.set(winrt::TreeViewNode());
        }

        if (ItemsSource())
        {
            viewModel->IsContentMode(true);
        }
        viewModel->PrepareView(m_rootNode.get());
        viewModel->SetOwners(listControl, *this);
        viewModel->NodeExpanding({ this, &TreeView::OnNodeExpanding });
        viewModel->NodeCollapsed({ this, &TreeView::OnNodeCollapsed });

        auto selectionMode = SelectionMode();
        if (selectionMode == winrt::TreeViewSelectionMode::Single)
        {
            listControl.SelectionMode(winrt::ListViewSelectionMode::Single);
        }
        else
        {
            listControl.SelectionMode(winrt::ListViewSelectionMode::None);
            if (selectionMode == winrt::TreeViewSelectionMode::Multiple)
            {
                UpdateItemsSelectionMode(true);
            }
        }

        m_itemClickRevoker = listControl.ItemClick(winrt::auto_revoke, { this, &TreeView::OnItemClick });
        m_containerContentChangingRevoker = listControl.ContainerContentChanging(winrt::auto_revoke, { this, &TreeView::OnContainerContentChanging });
        m_dragItemsStartingRevoker = listControl.DragItemsStarting(winrt::auto_revoke, { this, &TreeView::OnListControlDragItemsStarting });
        m_dragItemsCompletedRevoker = listControl.DragItemsCompleted(winrt::auto_revoke, { this, &TreeView::OnListControlDragItemsCompleted });
        m_selectionChangedRevoker = listControl.SelectionChanged(winrt::auto_revoke, { this, &TreeView::OnListControlSelectionChanged });

        if (m_pendingSelectedNodes && m_pendingSelectedNodes.get().Size() > 0)
        {
            auto selectedNodes = viewModel->GetSelectedNodes();
            for (auto const& node : m_pendingSelectedNodes.get())
            {
                selectedNodes.Append(node);
            }
            m_pendingSelectedNodes.get().Clear();
        }
    }
}
