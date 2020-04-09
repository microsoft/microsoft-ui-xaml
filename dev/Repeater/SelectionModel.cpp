// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "SelectionNode.h"
#include "SelectionModel.h"
#include "SelectionTreeHelper.h"
#include "IndexPath.h"
#include "SelectionModelSelectionChangedEventArgs.h"
#include "SelectionModelChildrenRequestedEventArgs.h"
#include "Vector.h"
#include "SelectedItems.h"
#include "CustomProperty.h"

#include "SelectionModel.properties.cpp"

SelectionModel::SelectionModel()
{
    // Parent is null for root node.
    m_rootNode = std::make_shared<SelectionNode>(this, nullptr /* parent */);
    // Parent is null for leaf node since it is shared. This is ok since we just
    // use the leaf as a placeholder and never ask stuff of it.
    m_leafNode = std::make_shared<SelectionNode>(this, nullptr /* parent */);
}

SelectionModel::~SelectionModel()
{
    ClearSelection(false /*resetAnchor*/, false /*raiseSelectionChanged*/);
    m_rootNode = nullptr;
    m_leafNode = nullptr;
    m_selectedIndicesCached = nullptr;
    m_selectedItemsCached = nullptr;
}

#pragma region ISelectionModel

winrt::IInspectable SelectionModel::Source()
{
    return m_rootNode->Source();
}

void SelectionModel::Source(winrt::IInspectable const& value)
{
    ClearSelection(true /* resetAnchor */, false /* raiseSelectionChanged */);
    m_rootNode->Source(value);
    OnSelectionChanged();
    RaisePropertyChanged(L"Source");
}

bool SelectionModel::SingleSelect()
{
    return m_singleSelect;
}

void SelectionModel::SingleSelect(bool value)
{
    if (m_singleSelect != !!value)
    {
        m_singleSelect = value;
        auto selectedIndices = SelectedIndices();
        if (value && selectedIndices && selectedIndices.Size() > 0)
        {
            // We want to be single select, so make sure there is only 
            // one selected item.
            auto firstSelectionIndexPath = selectedIndices.GetAt(0);
            ClearSelection(true /* resetAnchor */, false /*raiseSelectionChanged */);
            SelectWithPathImpl(firstSelectionIndexPath, true /* select */, false /* raiseSelectionChanged */);
            // Setting SelectedIndex will raise SelectionChanged event.
            SelectedIndex(firstSelectionIndexPath);
        }

        RaisePropertyChanged(L"SingleSelect");
    }
}

winrt::IndexPath SelectionModel::AnchorIndex()
{
    winrt::IndexPath anchor = nullptr;
    if (m_rootNode->AnchorIndex() >= 0)
    {
        std::vector<int> path;
        auto current = m_rootNode;
        while (current && current->AnchorIndex() >= 0)
        {
            path.emplace_back(current->AnchorIndex());
            current = current->GetAt(current->AnchorIndex(), false);
        }

        anchor = winrt::make<IndexPath>(path);
    }

    return anchor;
}

void SelectionModel::AnchorIndex(winrt::IndexPath const& value)
{
    if (value)
    {
        SelectionTreeHelper::TraverseIndexPath(
            m_rootNode,
            value,
            true, /* realizeChildren */
            [](std::shared_ptr<SelectionNode> currentNode, const winrt::IndexPath& path, int depth, int childIndex)
        {
            currentNode->AnchorIndex(path.GetAt(depth));
        }
        );
    }
    else
    {
        m_rootNode->AnchorIndex(-1);
    }

    RaisePropertyChanged(L"AnchorIndex");
}

winrt::IndexPath SelectionModel::SelectedIndex()
{
    winrt::IndexPath selectedIndex = nullptr;
    auto selectedIndices = SelectedIndices();
    if (selectedIndices && selectedIndices.Size() > 0)
    {
        selectedIndex = selectedIndices.GetAt(0);
    }

    return selectedIndex;
}

void SelectionModel::SelectedIndex(winrt::IndexPath const& value)
{
    auto isSelected = IsSelectedAt(value);
    if (!isSelected || !isSelected.Value())
    {
        ClearSelection(true /* resetAnchor */, false /*raiseSelectionChanged */);
        SelectWithPathImpl(value, true /* select */, false /* raiseSelectionChanged */);
        OnSelectionChanged();
    }
}

winrt::IInspectable SelectionModel::SelectedItem()
{
    winrt::IInspectable item = nullptr;
    auto selectedItems = SelectedItems();
    if (selectedItems && selectedItems.Size() > 0)
    {
        item = selectedItems.GetAt(0);
    }

    return item;
}

winrt::IVectorView<winrt::IInspectable> SelectionModel::SelectedItems()
{
    if (!m_selectedItemsCached)
    {
        std::vector<SelectedItemInfo> selectedInfos;
        if (m_rootNode.get()->Source())
        {
            SelectionTreeHelper::Traverse(
                m_rootNode,
                false, /* realizeChildren */
                [&selectedInfos](const SelectionTreeHelper::TreeWalkNodeInfo& currentInfo)
            {
                if (currentInfo.Node->SelectedCount() > 0)
                {
                    selectedInfos.emplace_back(SelectedItemInfo{ currentInfo.Node, currentInfo.Path });
                }
            });
        }

        // Instead of creating a dumb vector that takes up the space for all the selected items,
        // we create a custom VectorView implimentation that calls back using a delegate to find 
        // the selected item at a particular index. This avoid having to create the storage and copying
        // needed in a dumb vector. This also allows us to expose a tree of selected nodes into an 
        // easier to consume flat vector view of objects.
        auto selectedItems = winrt::make<::SelectedItems<winrt::IInspectable>>(
            selectedInfos,
            [](const std::vector<SelectedItemInfo>& infos, unsigned int index) // callback for GetAt(index)
        {
            unsigned int currentIndex = 0;
            winrt::IInspectable item{ nullptr };
            for (auto& info : infos)
            {
                if (auto node = info.Node.lock())
                {
                    unsigned int currentCount = node->SelectedCount();
                    if (index >= currentIndex && index < currentIndex + currentCount)
                    {
                        int targetIndex = node->SelectedIndices().at(index - currentIndex);
                        item = node->ItemsSourceView().GetAt(targetIndex);
                        break;
                    }

                    currentIndex += currentCount;
                }
                else
                {
                    throw winrt::hresult_error(E_FAIL, L"selection has changed since SelectedItems property was read.");
                }
            }

            return item;
        });
        m_selectedItemsCached = selectedItems;
    }

    return m_selectedItemsCached;
}

winrt::IVectorView<winrt::IndexPath> SelectionModel::SelectedIndices()
{
    if (!m_selectedIndicesCached)
    {
        std::vector<SelectedItemInfo> selectedInfos;
        SelectionTreeHelper::Traverse(
            m_rootNode,
            false, /* realizeChildren */
            [&selectedInfos](const SelectionTreeHelper::TreeWalkNodeInfo& currentInfo)
        {
            if (currentInfo.Node->SelectedCount() > 0)
            {
                selectedInfos.emplace_back(SelectedItemInfo{ currentInfo.Node, currentInfo.Path });
            }
        });

        // Instead of creating a dumb vector that takes up the space for all the selected indices,
        // we create a custom VectorView implimentation that calls back using a delegate to find 
        // the IndexPath at a particular index. This avoid having to create the storage and copying
        // needed in a dumb vector. This also allows us to expose a tree of selected nodes into an 
        // easier to consume flat vector view of IndexPaths.
        auto indices = winrt::make<::SelectedItems<winrt::IndexPath>>(
            selectedInfos,
            [](const std::vector<SelectedItemInfo>& infos, unsigned int index) // callback for GetAt(index)
        {
            unsigned int currentIndex = 0;
            winrt::IndexPath path{ nullptr };
            for (auto& info : infos)
            {
                if (auto node = info.Node.lock())
                {
                    unsigned int currentCount = node->SelectedCount();
                    if (index >= currentIndex && index < currentIndex + currentCount)
                    {
                        int targetIndex = node->SelectedIndices().at(index - currentIndex);
                        path = winrt::get_self<IndexPath>(info.Path)->CloneWithChildIndex(targetIndex);
                        break;
                    }

                    currentIndex += currentCount;
                }
                else
                {
                    throw winrt::hresult_error(E_FAIL, L"selection has changed since SelectedIndices property was read.");
                }
            }

            return path;
        });
        m_selectedIndicesCached = indices;
    }

    return m_selectedIndicesCached;
}

void SelectionModel::SetAnchorIndex(int32_t index)
{
    AnchorIndex(winrt::make<IndexPath>(index));
}

void SelectionModel::SetAnchorIndex(int groupIndex, int itemIndex)
{
    AnchorIndex(winrt::make<IndexPath>(groupIndex, itemIndex));
}

void SelectionModel::Select(int32_t index)
{
    SelectImpl(index, true /* select */);
}

void SelectionModel::Select(int groupIndex, int itemIndex)
{
    SelectWithGroupImpl(groupIndex, itemIndex, true /* select */);
}

void SelectionModel::SelectAt(winrt::IndexPath const& index)
{
    SelectWithPathImpl(index, true /* select */, true /* raiseSelectionChanged */);
}

void SelectionModel::Deselect(int32_t index)
{
    SelectImpl(index, false /* select */);
}

void SelectionModel::Deselect(int groupIndex, int itemIndex)
{
    SelectWithGroupImpl(groupIndex, itemIndex, false /* select */);
}

void SelectionModel::DeselectAt(winrt::IndexPath const& index)
{
    SelectWithPathImpl(index, false /* select */, true /* raiseSelectionChanged */);
}

winrt::IReference<bool> SelectionModel::IsSelected(int index)
{
    MUX_ASSERT(index >= 0);
    auto isSelected = m_rootNode->IsSelectedWithPartial(index);
    return isSelected;
}

winrt::IReference<bool> SelectionModel::IsSelected(int groupIndex, int itemIndex)
{
    MUX_ASSERT(groupIndex >= 0 && itemIndex >= 0);
    winrt::IReference<bool> isSelected = false;
    auto childNode = m_rootNode->GetAt(groupIndex, false /*realizeChild*/);
    if (childNode)
    {
        isSelected = childNode->IsSelectedWithPartial(itemIndex);
    }

    return isSelected;
}

winrt::IReference<bool> SelectionModel::IsSelectedAt(winrt::IndexPath const& index)
{
    auto path = index;
    MUX_ASSERT(winrt::get_self<IndexPath>(path)->IsValid());
    bool isRealized = true;
    auto node = m_rootNode;
    for (int i = 0; i < path.GetSize() - 1; i++)
    {
        auto childIndex = path.GetAt(i);
        node = node->GetAt(childIndex, false /* realizeChild */);
        if (!node)
        {
            isRealized = false;
            break;
        }
    }

    winrt::IReference<bool> isSelected = false;
    if (isRealized)
    {
        auto size = path.GetSize();
        if (size == 0)
        {
            isSelected = SelectionNode::ConvertToNullableBool(node->EvaluateIsSelectedBasedOnChildrenNodes());
        }
        else
        {
            isSelected = node->IsSelectedWithPartial(path.GetAt(size - 1));
        }
    }

    return isSelected;
}

void SelectionModel::SelectRangeFromAnchor(int32_t index)
{
    SelectRangeFromAnchorImpl(index, true /* select */ );
}

void SelectionModel::SelectRangeFromAnchor(int endGroupIndex, int endItemIndex)
{
    SelectRangeFromAnchorWithGroupImpl(endGroupIndex, endItemIndex, true /* select */);
}

void SelectionModel::SelectRangeFromAnchorTo(winrt::IndexPath const& index)
{
    SelectRangeImpl(AnchorIndex(), index, true /* select */);
}

void SelectionModel::DeselectRangeFromAnchor(int32_t index)
{
    SelectRangeFromAnchorImpl(index, false /* select */);
}

void SelectionModel::DeselectRangeFromAnchor(int endGroupIndex, int endItemIndex)
{
    SelectRangeFromAnchorWithGroupImpl(endGroupIndex, endItemIndex, false /* select */);
}

void SelectionModel::DeselectRangeFromAnchorTo(winrt::IndexPath const& index)
{
    SelectRangeImpl(AnchorIndex(), index, false /* select */);
}


void SelectionModel::SelectRange(winrt::IndexPath const& start, winrt::IndexPath const& end)
{
    SelectRangeImpl(start, end, true /* select */);
}

void SelectionModel::DeselectRange(winrt::IndexPath const& start, winrt::IndexPath const& end)
{
    SelectRangeImpl(start, end, false /* select */);
}

void SelectionModel::SelectAll()
{
    SelectionTreeHelper::Traverse(
        m_rootNode,
        true, /* realizeChildren */
        [](const SelectionTreeHelper::TreeWalkNodeInfo& info)
    {
        if (info.Node->DataCount() > 0)
        {
            info.Node->SelectAll();
        }
    });

    OnSelectionChanged();
}

void SelectionModel::ClearSelection()
{
    ClearSelection(true /*resetAnchor*/, true /* raiseSelectionChanged */);
}

#pragma endregion

#pragma region ICustomPropertyProvider

winrt::TypeName SelectionModel::Type()
{
    auto outer = get_strong().as<winrt::IInspectable>();
    winrt::TypeName typeName;
    typeName.Kind = winrt::TypeKind::Metadata;
    typeName.Name = winrt::get_class_name(outer);
    return typeName;
}

winrt::ICustomProperty SelectionModel::GetCustomProperty(hstring const& name)
{
    // Exposing SelectedItem through ICustomPropertyProvider so that Binding can work 
    // for SelectedItem. This is requried since SelectedItem is not a dependency proeprty and
    // is evaluated when requested.
    if (name == L"SelectedItem")
    {
        auto selectedItemCustomProperty = winrt::make<CustomProperty>(
            L"SelectedItem" /* name */,
            winrt::xaml_typename<winrt::IInspectable>() /* typeName */,
            [](winrt::IInspectable const& target) { return target.as<winrt::SelectionModel>().SelectedItem(); } /* getter */,
            nullptr /* setter */);
        return selectedItemCustomProperty;
    }

    return nullptr;
}

winrt::ICustomProperty SelectionModel::GetIndexedProperty(hstring const& name, winrt::TypeName const& type)
{
    // No indexed properties exposed via ICustomPropertyProvider
    return nullptr;
}

hstring SelectionModel::GetStringRepresentation()
{
    return L"SelectionModel";
}

#pragma endregion

#pragma region INotifyPropertyChanged

winrt::event_token SelectionModel::PropertyChanged(winrt::PropertyChangedEventHandler const& value)
{
    return m_propertyChangedEventSource.add(value);
}

void SelectionModel::PropertyChanged(winrt::event_token const& token)
{
    m_propertyChangedEventSource.remove(token);
}

#pragma endregion

#pragma region ISelectionModelProtected

void SelectionModel::OnPropertyChanged(winrt::hstring const& propertyName)
{
    RaisePropertyChanged(propertyName);
}

#pragma endregion

void SelectionModel::RaisePropertyChanged(std::wstring_view const& name)
{
    m_propertyChangedEventSource(*this, winrt::PropertyChangedEventArgs(name));
}

void SelectionModel::OnSelectionInvalidatedDueToCollectionChange()
{
    OnSelectionChanged();
}

winrt::IInspectable SelectionModel::ResolvePath(const winrt::IInspectable& data, const winrt::IndexPath& dataIndexPath)
{
    winrt::IInspectable resolved = nullptr;
    // Raise ChildrenRequested event if there is a handler
    if (m_childrenRequestedEventSource)
    {
        if (!m_childrenRequestedEventArgs)
        {
            m_childrenRequestedEventArgs = tracker_ref<winrt::SelectionModelChildrenRequestedEventArgs>(this, winrt::make<SelectionModelChildrenRequestedEventArgs>(data, dataIndexPath, false /*throwOnAccess*/));
        }
        else
        {
            winrt::get_self<SelectionModelChildrenRequestedEventArgs>(m_childrenRequestedEventArgs.get())->Initialize(data, dataIndexPath, false /*throwOnAccess*/);
        }

        m_childrenRequestedEventSource(*this, m_childrenRequestedEventArgs.get());
        resolved = m_childrenRequestedEventArgs.get().Children();

        // Clear out the values in the args so that it cannot be used after the event handler call.
        winrt::get_self<SelectionModelChildrenRequestedEventArgs>(m_childrenRequestedEventArgs.get())->Initialize(nullptr, nullptr, true /*throwOnAccess*/);
    }
    else
    {
        // No handlers for ChildrenRequested event. If data is of type ItemsSourceView
        // or a type that can be used to create a ItemsSourceView using ItemsSourceView::CreateFrom, then we can
        // auto-resolve that as the child. If not, then we consider the value as a leaf. This is to 
        // avoid having to provide the event handler for the most common scenarios. If the app dev does
        // not want this default behavior, they can provide the handler to override.
        if (data.try_as<winrt::ItemsSourceView>() || 
            data.try_as<winrt::IBindableVector>() ||
            data.try_as<winrt::IIterable<winrt::IInspectable>>() ||
            data.try_as<winrt::IBindableIterable>())
        {
            resolved = data;
        }
    }

    return resolved;
}

void SelectionModel::ClearSelection(bool resetAnchor, bool raiseSelectionChanged)
{
    SelectionTreeHelper::Traverse(
        m_rootNode,
        false, /* realizeChildren */
        [](const SelectionTreeHelper::TreeWalkNodeInfo& info)
    {
        info.Node->Clear();
    });

    if (resetAnchor)
    {
        AnchorIndex(nullptr);
    }

    if (raiseSelectionChanged)
    {
        OnSelectionChanged();
    }
}

void SelectionModel::OnSelectionChanged()
{
    m_selectedIndicesCached = nullptr;
    m_selectedItemsCached = nullptr;

    // Raise SelectionChanged event
    if (m_selectionChangedEventSource)
    {
        if (!m_selectionChangedEventArgs)
        {
            m_selectionChangedEventArgs = tracker_ref<winrt::SelectionModelSelectionChangedEventArgs>(this, winrt::make<SelectionModelSelectionChangedEventArgs>());
        }

        m_selectionChangedEventSource(*this, m_selectionChangedEventArgs.get());
    }

    RaisePropertyChanged(L"SelectedIndex");
    RaisePropertyChanged(L"SelectedIndices");
    if (m_rootNode->Source())
    {
        RaisePropertyChanged(L"SelectedItem");
        RaisePropertyChanged(L"SelectedItems");
    }
}

void SelectionModel::SelectImpl(int index, bool select)
{
    if (m_rootNode->IsSelected(index) != select)
    {
        if (m_singleSelect)
        {
            ClearSelection(true /*resetAnchor*/, false /* raiseSelectionChanged */);
        }
        auto selected = m_rootNode->Select(index, select);
        if (selected)
        {
            AnchorIndex(winrt::make<IndexPath>(index));
        }
        OnSelectionChanged();
    }
}

void SelectionModel::SelectWithGroupImpl(int groupIndex, int itemIndex, bool select)
{
    if (m_singleSelect)
    {
        ClearSelection(true /*resetAnchor*/, false /* raiseSelectionChanged */);
    }

    auto childNode = m_rootNode->GetAt(groupIndex, true /* realize */);
    auto selected = childNode->Select(itemIndex, select);
    if (selected)
    {
        AnchorIndex(winrt::make<IndexPath>(groupIndex, itemIndex));
    }

    OnSelectionChanged();
}

void SelectionModel::SelectWithPathImpl(const winrt::IndexPath& index, bool select, bool raiseSelectionChanged)
{
    bool newSelection = true;

    // Handle single select differently as comparing indexpaths is faster
    if (m_singleSelect)
    {
        if (auto const selectedIndex = SelectedIndex())
        {
            // If paths are equal and we want to select, skip everything and do nothing
            if (select && selectedIndex.CompareTo(index) == 0)
            {
                newSelection = false;
            }
        }
        else
        {
            // If we are in single select and selectedIndex is null, deselecting is not a new change.
            // Selecting something is a new change, so set flag to appropriate value here.
            newSelection = select;
        }
    }

    // Selection is actually different from previous one, so update.
    if (newSelection)
    {
        bool selected = false;
        // If we unselect something, raise event any way, otherwise changedSelection is false
        bool changedSelection = false;

        if (m_singleSelect)
        {
            ClearSelection(true /*resetAnchor*/, false /* raiseSelectionChanged */);
        }

        SelectionTreeHelper::TraverseIndexPath(
            m_rootNode,
            index,
            true, /* realizeChildren */
            [&selected, &select, &changedSelection](std::shared_ptr<SelectionNode> currentNode, const winrt::IndexPath& path, int depth, int childIndex)
            {
                if (depth == path.GetSize() - 1)
                {
                    if (currentNode->IsSelected(childIndex) != select)
                    {
                        // Node has different value then we want to set, so lets update!
                        changedSelection = true;
                    }
                    selected = currentNode->Select(childIndex, select);
                }
            }
        );

        if (selected)
        {
            AnchorIndex(index);
        }

        // The walk tree operation can change the indices, and the next time it get's read,
        // we would throw an exception. That's what we are preventing with next two lines
        m_selectedIndicesCached = nullptr;
        m_selectedItemsCached = nullptr;

        if (raiseSelectionChanged && changedSelection)
        {
            OnSelectionChanged();
        }
    }
}

void SelectionModel::SelectRangeFromAnchorImpl(int index, bool select)
{
    int anchorIndex = 0;
    auto anchor = AnchorIndex();
    if (anchor)
    {
        MUX_ASSERT(anchor.GetSize() == 1);
        anchorIndex = anchor.GetAt(0);
    }

    bool selected = m_rootNode->SelectRange(IndexRange(anchorIndex, index), select);
    if (selected)
    {
        OnSelectionChanged();
    }
}

void SelectionModel::SelectRangeFromAnchorWithGroupImpl(int endGroupIndex, int endItemIndex, bool select)
{
    int startGroupIndex = 0;
    int startItemIndex = 0;
    auto anchorIndex = AnchorIndex();
    if (anchorIndex)
    {
        MUX_ASSERT(anchorIndex.GetSize() == 2);
        startGroupIndex = anchorIndex.GetAt(0);
        startItemIndex = anchorIndex.GetAt(1);
    }

    // Make sure start > end
    if (startGroupIndex > endGroupIndex ||
        (startGroupIndex == endGroupIndex && startItemIndex > endItemIndex))
    {
        int temp = startGroupIndex;
        startGroupIndex = endGroupIndex;
        endGroupIndex = temp;
        temp = startItemIndex;
        startItemIndex = endItemIndex;
        endItemIndex = temp;
    }

    bool selected = false;
    for (int groupIdx = startGroupIndex; groupIdx <= endGroupIndex; groupIdx++)
    {
        auto groupNode = m_rootNode->GetAt(groupIdx, true /* realizeChild */);
        int startIndex = groupIdx == startGroupIndex ? startItemIndex : 0;
        int endIndex = groupIdx == endGroupIndex ? endItemIndex : groupNode->DataCount() - 1;
        selected |= groupNode->SelectRange(IndexRange(startIndex, endIndex), select);
    }

    if (selected)
    {
        OnSelectionChanged();
    }
}

void SelectionModel::SelectRangeImpl(const winrt::IndexPath& start, const winrt::IndexPath& end, bool select)
{
    auto winrtStart = start;
    auto winrtEnd = end;

    // Make sure start <= end 
    if (winrtEnd.CompareTo(winrtStart) == -1)
    {
        auto temp = winrtStart;
        winrtStart = winrtEnd;
        winrtEnd = temp;
    }

    // Note: Since we do not know the depth of the tree, we have to walk to each leaf
    SelectionTreeHelper::TraverseRangeRealizeChildren(
        m_rootNode,
        winrtStart,
        winrtEnd,
        [select](const SelectionTreeHelper::TreeWalkNodeInfo& info)
    {
        if (info.Node->DataCount() == 0)
        {
            // Select only leaf nodes
            info.ParentNode->Select(info.Path.GetAt(info.Path.GetSize() - 1), select);
        }
    });

    OnSelectionChanged();
}
