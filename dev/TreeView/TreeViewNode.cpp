// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TreeViewNode.h"
#include "Vector.h"
#include "VectorChangedEventArgs.h"

using TreeNodeSelectionState = TreeViewNode::TreeNodeSelectionState;

TreeViewNode::TreeViewNode()
{
    EnsureProperties();
    auto collection = winrt::make_self<TreeViewNodeVector>();
    collection->SetParent(*this);
    m_children.set(*collection);
    m_children.get().as<winrt::IObservableVector<winrt::TreeViewNode>>().VectorChanged({ this, &TreeViewNode::ChildVectorChanged });
}

winrt::TreeViewNode TreeViewNode::Parent()
{
    return get_ParentImpl();
}

winrt::TreeViewNode TreeViewNode::get_ParentImpl()
{
    return m_parentNode ? m_parentNode.get() : nullptr;
}

void TreeViewNode::put_ParentImpl(winrt::TreeViewNode const& value)
{
    if (value != nullptr)
    {
        m_parentNode = winrt::make_weak(value);
    }
    else
    {
        m_parentNode = nullptr;
    }

    //A parentless node has a depth of -1, and the first level of visible nodes in
    // the tree view will have a depth of 0 (-1 + 1);
    UpdateDepth(value ? value.Depth() + 1 : -1);
}

bool TreeViewNode::HasUnrealizedChildren()
{
    return m_HasUnrealizedChildren;
}

void TreeViewNode::HasUnrealizedChildren(bool value)
{
    m_HasUnrealizedChildren = value;
    UpdateHasChildren();
}

winrt::IVector<winrt::TreeViewNode> TreeViewNode::Children()
{
    auto x = m_children.get();
    return x;
}

bool TreeViewNode::IsContentMode()
{
    return m_isContentMode;
}

void TreeViewNode::IsContentMode(bool value)
{
    m_isContentMode = value;
}

TreeNodeSelectionState TreeViewNode::SelectionState()
{
    return m_multiSelectionState;
}

void TreeViewNode::SelectionState(TreeNodeSelectionState const& state)
{
    m_multiSelectionState = state;
}

void TreeViewNode::UpdateDepth(int depth)
{
    // Update our depth
    SetValue(s_DepthProperty, box_value(depth));

    // Update children's depth
    for (auto const& childInspectable : Children())
    {
        auto childNode = childInspectable.as<winrt::TreeViewNode>();
        winrt::get_self<TreeViewNode>(childNode)->UpdateDepth(depth + 1);
    }
}

void TreeViewNode::UpdateHasChildren()
{
    bool hasChildren = ((Children().Size() != 0) || m_HasUnrealizedChildren);
    SetValue(s_HasChildrenProperty, box_value(hasChildren));
}

void TreeViewNode::ChildVectorChanged(winrt::IObservableVector<winrt::TreeViewNode> const& sender, winrt::IInspectable const& args)
{
    const auto wArgs = args.as<winrt::IVectorChangedEventArgs>();
    const winrt::CollectionChange collectionChange = wArgs.CollectionChange();
    const unsigned int index = args.as<winrt::IVectorChangedEventArgs>().Index();
    UpdateHasChildren();
    RaiseChildrenChanged(collectionChange, index);
}

void TreeViewNode::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    winrt::IDependencyProperty property = args.Property();
    m_propertyChangedEventSource(*this, args);
}

winrt::event_token TreeViewNode::AddExpandedChanged(winrt::TypedEventHandler<winrt::TreeViewNode, winrt::DependencyPropertyChangedEventArgs> const& value)
{
    winrt::event_token token = m_propertyChangedEventSource.add(value);
    return token;
}

void TreeViewNode::RemoveExpandedChanged(winrt::event_token token)
{
    m_propertyChangedEventSource.remove(token);
}

winrt::event_token TreeViewNode::ChildrenChanged(winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IVectorChangedEventArgs> const& value)
{
    winrt::event_token token = m_childrenChangedSource.add(value);
    return token;
}

void TreeViewNode::ChildrenChanged(winrt::event_token token)
{
    m_childrenChangedSource.remove(token);
}

void TreeViewNode::RaiseChildrenChanged(winrt::CollectionChange CC, unsigned int index)
{
    auto args = winrt::make<VectorChangedEventArgs>(CC, index);
    m_childrenChangedSource(*this, args);
}

winrt::IInspectable TreeViewNode::ItemsSource()
{
    return m_itemsSource.get();
}

void TreeViewNode::ItemsSource(winrt::IInspectable const& value)
{
    m_itemItemsSourceViewChangedRevoker.revoke();
    m_itemsSource.set(value);
    m_itemsDataSource = value ? winrt::ItemsSourceView(value) : nullptr;
    if (m_itemsDataSource)
    {
        m_itemItemsSourceViewChangedRevoker = m_itemsDataSource.CollectionChanged(winrt::auto_revoke, { this, &TreeViewNode::OnItemsSourceChanged });
    }
    SyncChildrenNodesWithItemsSource();
}

void TreeViewNode::OnItemsSourceChanged(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args)
{
    switch (args.Action())
    {
    case winrt::NotifyCollectionChangedAction::Add:
    {
        // TreeViewNode and ItemsSource will update each other when data changes.
        // For ItemsSource -> TreeViewNode changes, m_itemsDataSource.Count() > Children().Size()
        // We'll add the new node to children collection.
        // For TreeViewNode -> ItemsSource changes, m_itemsDataSource.Count() == Children().Size()
        // the node is already in children collection, we don't want to update TreeViewNode again here.
        if (m_itemsDataSource.Count() != static_cast<int>(Children().Size()))
        {
            AddToChildrenNodes(args.NewStartingIndex(), args.NewItems().Size());
        }
        break;
    }

    case winrt::NotifyCollectionChangedAction::Remove:
    {
        // TreeViewNode and ItemsSource will update each other when data changes.
        // For ItemsSource -> TreeViewNode changes, m_itemsDataSource.Count() < Children().Size()
        // We'll remove the node from children collection.
        // For TreeViewNode -> ItemsSource changes, m_itemsDataSource.Count() == Children().Size()
        // the node is already removed, we don't want to update TreeViewNode again here.
        if (m_itemsDataSource.Count() != static_cast<int>(Children().Size()))
        {
            RemoveFromChildrenNodes(args.OldStartingIndex(), args.OldItems().Size());
        }
        break;
    }

    case winrt::NotifyCollectionChangedAction::Reset:
    {
        SyncChildrenNodesWithItemsSource();
        break;
    }

    case winrt::NotifyCollectionChangedAction::Replace:
    {
        RemoveFromChildrenNodes(args.OldStartingIndex(), args.OldItems().Size());
        AddToChildrenNodes(args.NewStartingIndex(), args.NewItems().Size());
        break;
    }
    }
}

void TreeViewNode::AddToChildrenNodes(int index, int count)
{
    for (int i = index + count - 1; i >= index; i--)
    {
        auto item = m_itemsDataSource.GetAt(i);
        auto node = winrt::make_self<TreeViewNode>();
        node->Content(item);
        winrt::get_self<TreeViewNodeVector>(Children())->InsertAt(index, *node, false /* updateItemsSource */);
    }
}

void TreeViewNode::RemoveFromChildrenNodes(int index, int count)
{
    for (int i = 0; i < count; i++)
    {
        winrt::get_self<TreeViewNodeVector>(Children())->RemoveAt(index, false /* updateItemsSource */);
    }
}

void TreeViewNode::SyncChildrenNodesWithItemsSource()
{
    if (!AreChildrenNodesEqualToItemsSource())
    {
        const auto children = winrt::get_self<TreeViewNodeVector>(Children());
        children->Clear(false /* updateItemsSource */, false /* updateIsExpanded */);

        const auto size = m_itemsDataSource ? m_itemsDataSource.Count() : 0;
        for (auto i = 0; i < size; i++)
        {
            const auto item = m_itemsDataSource.GetAt(i);
            const auto node = winrt::make_self<TreeViewNode>();
            node->Content(item);
            node->IsContentMode(true);
            children->Append(*node, false /* updateItemsSource */);
        }
    }
}

bool TreeViewNode::AreChildrenNodesEqualToItemsSource()
{
    const auto children = Children();
    const UINT32 childrenCount = children ? children.Size() : 0;
    const UINT32 itemsSourceCount = m_itemsDataSource ? m_itemsDataSource.Count() : 0;

    if (childrenCount != itemsSourceCount)
    {
        return false;
    }

    // Compare the actual content in collections when counts are equal
    for (UINT32 i = 0; i < itemsSourceCount; i++)
    {
        if (children.GetAt(i).Content() != m_itemsDataSource.GetAt(i))
        {
            return false;
        }
    }

    return true;
}

hstring TreeViewNode::GetContentAsString()
{
    if (auto content = Content())
    {
        if (auto result = content.try_as<winrt::ICustomPropertyProvider>())
        {
            return result.GetStringRepresentation();
        }

        if (auto result = content.try_as<winrt::IStringable>())
        {
            return result.ToString();
        }

        return winrt::unbox_value_or<hstring>(content, Type().Name);
    }

    return Type().Name;
}

#pragma region ICustomPropertyProvider

winrt::TypeName TreeViewNode::Type()
{
    auto outer = get_strong().as<winrt::IInspectable>();
    winrt::TypeName typeName;
    typeName.Kind = winrt::TypeKind::Metadata;
    typeName.Name = winrt::get_class_name(outer);
    return typeName;
}

winrt::ICustomProperty TreeViewNode::GetCustomProperty(hstring const& name)
{
    return nullptr;
}

winrt::ICustomProperty TreeViewNode::GetIndexedProperty(hstring const& name, winrt::TypeName const& type)
{
    return nullptr;
}

hstring TreeViewNode::GetStringRepresentation()
{
    return GetContentAsString();
}

#pragma endregion

#pragma region IStringable
hstring TreeViewNode::ToString()
{
    return GetContentAsString();
}
#pragma endregion

#pragma region TreeViewNodeVector

TreeViewNodeVector::TreeViewNodeVector()
{
}

TreeViewNodeVector::TreeViewNodeVector(unsigned int capacity)
{
    GetVectorInnerImpl()->reserve(capacity);
}

void TreeViewNodeVector::SetParent(winrt::TreeViewNode value)
{
    m_parent = winrt::make_weak(value);
}

TreeViewNode* TreeViewNodeVector::Parent()
{
    return winrt::get_self<TreeViewNode>(m_parent.get());
}

winrt::IBindableVector TreeViewNodeVector::GetWritableParentItemsSource()
{
    winrt::IBindableVector parentItemsSource = nullptr;

    if (auto parent = Parent())
    {
        if (auto itemsSource = parent->ItemsSource())
        {
            parentItemsSource = itemsSource.try_as<winrt::IBindableVector>();
        }
    }
    return parentItemsSource;
}

void TreeViewNodeVector::Append(winrt::TreeViewNode const& item, bool updateItemsSource)
{
    InsertAt(Size(), item, updateItemsSource);
}

void TreeViewNodeVector::InsertAt(unsigned int index, winrt::TreeViewNode const& item, bool updateItemsSource)
{
    auto inner = GetVectorInnerImpl();
    MUX_ASSERT(m_parent.get());
    MUX_ASSERT(index <= inner->Size());
    winrt::get_self<TreeViewNode>(item)->put_ParentImpl(m_parent.get());

    inner->InsertAt(index, item);

    if (updateItemsSource)
    {
        if (auto itemsSource = GetWritableParentItemsSource())
        {
            itemsSource.InsertAt(index, item.Content());
        }
    }
}

void TreeViewNodeVector::SetAt(unsigned int index, winrt::TreeViewNode const& item, bool updateItemsSource)
{
    RemoveAt(index, updateItemsSource,false /* updateIsExpanded */);
    InsertAt(index, item, updateItemsSource);
}

void TreeViewNodeVector::RemoveAt(unsigned int index, bool updateItemsSource,bool updateIsExpanded)
{
    auto inner = GetVectorInnerImpl();
    auto targetNode = inner->GetAt(index);
    winrt::get_self<TreeViewNode>(targetNode)->put_ParentImpl(nullptr);

    inner->RemoveAt(index);

    if (updateItemsSource)
    {
        if (auto source = GetWritableParentItemsSource())
        {
            source.RemoveAt(index);
        }
    }

    // No children, so close parent if not requested otherwise
    if (updateIsExpanded && inner->Size() == 0)
    {
        if (auto&& ownerNode = m_parent.get())
        {
            // Only set IsExpanded to false if we are not the root node
            if (auto&& ownerParent = ownerNode.Parent())
            {
                ownerNode.IsExpanded(false);
            }
        }
    }
}

void TreeViewNodeVector::RemoveAtEnd(bool updateItemsSource)
{
    const auto index = GetVectorInnerImpl()->Size() - 1;
    RemoveAt(updateItemsSource);
}

void TreeViewNodeVector::ReplaceAll(winrt::array_view<winrt::TreeViewNode const> values, bool updateItemsSource)
{
    const auto inner = GetVectorInnerImpl();

    const auto count = inner->Size();
    if (count > 0)
    {
        Clear(updateItemsSource);

        auto itemsSource = GetWritableParentItemsSource();
        // Set parent on new elements
        MUX_ASSERT(m_parent.get());
        for (auto& value : values)
        {
            winrt::get_self<TreeViewNode>(value)->put_ParentImpl(m_parent.get());
            if (itemsSource)
            {
                itemsSource.Append(value.Content());
            }
        }

        inner->ReplaceAll(values);
    }
}

void TreeViewNodeVector::Clear(bool updateItemsSource,bool updateIsExpanded)
{
    const auto inner = GetVectorInnerImpl();
    const auto count = inner->Size();

    if (count > 0)
    {
        for (unsigned int i = 0; i < count; i++)
        {
            auto node = inner->GetAt(i);
            winrt::get_self<TreeViewNode>(node)->put_ParentImpl(nullptr);
        }

        inner->Clear();

        if (updateItemsSource)
        {
            if (auto itemsSource = GetWritableParentItemsSource())
            {
                itemsSource.Clear();
            }
        }
    }

    if (updateIsExpanded)
    {
        if(auto&& ownerNode = m_parent.get())
        {
            // Only set IsExpanded to false if we are not the root node
            if (auto&& ownerParent = ownerNode.Parent())
            {
                ownerNode.IsExpanded(false);
            }
        }
    }
}

#pragma endregion
