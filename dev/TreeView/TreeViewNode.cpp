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
    auto wArgs = args.as<winrt::IVectorChangedEventArgs>();
    winrt::CollectionChange collectionChange = wArgs.CollectionChange();
    unsigned int index = args.as<winrt::IVectorChangedEventArgs>().Index();
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
            OnItemsAdded(args.NewStartingIndex(), args.NewItems().Size());
            break;
        }

        case winrt::NotifyCollectionChangedAction::Remove:
        {
            OnItemsRemoved(args.OldStartingIndex(), args.OldItems().Size());
            break;
        }

        case winrt::NotifyCollectionChangedAction::Reset:
        {
            SyncChildrenNodesWithItemsSource();
            break;
        }

        case winrt::NotifyCollectionChangedAction::Replace:
        {
            OnItemsRemoved(args.OldStartingIndex(), args.OldItems().Size());
            OnItemsAdded(args.NewStartingIndex(), args.NewItems().Size());
            break;
        }
    }
}

void TreeViewNode::OnItemsAdded(int index, int count)
{
    for (int i = index + count - 1; i >= index; i--)
    {
        auto item = m_itemsDataSource.GetAt(i);
        auto node = winrt::make_self<TreeViewNode>();
        node->Content(item);
        winrt::get_self<TreeViewNodeVector>(Children())->InsertAtCore(index, *node);
    }
}

void TreeViewNode::OnItemsRemoved(int index, int count)
{
    for (int i = 0; i < count; i++)
    {
        winrt::get_self<TreeViewNodeVector>(Children())->RemoveAtCore(index);
    }
}

void TreeViewNode::SyncChildrenNodesWithItemsSource()
{
    auto children = winrt::get_self<TreeViewNodeVector>(Children());
    children->ClearCore();

    if (m_itemsDataSource)
    {
        int size = m_itemsDataSource.Count();
        for (int i = 0; i < size; i++)
        {
            auto item = m_itemsDataSource.GetAt(i);
            auto node = winrt::make_self<TreeViewNode>();
            node->Content(item);
            node->IsContentMode(true);
            // Required to create the whole tree when used in NavigationView Markup
            if (auto nvi = item.try_as<winrt::NavigationViewItem>())
            {
                if (nvi.MenuItems().Size() > 0)
                {
                    node->ItemsSource(nvi.MenuItems());
                }
            }
            children->AppendCore(*node);
        }
    }
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

// Check if parent node is in "content mode".
// We don't want users to use ItemsSource and modify TreeViewNode at the same time since that might cause some unexpected behaviors.
// This method is used to check what "mode" is treeview currently in.
bool TreeViewNodeVector::IsParentInContentMode()
{
    return winrt::get_self<TreeViewNode>(m_parent.get())->IsContentMode();
}

TreeViewNodeVector::TreeViewNodeVector()
{
}

TreeViewNodeVector::TreeViewNodeVector(unsigned int capacity)
{
    GetVectorInnerImpl()->reserve(capacity);
}

void TreeViewNodeVector::SetParent(winrt::TreeViewNode value) { m_parent = winrt::make_weak(value); }

void TreeViewNodeVector::AppendCore(winrt::TreeViewNode const& item)
{
    auto inner = GetVectorInnerImpl();

    winrt::get_self<TreeViewNode>(item)->put_ParentImpl(m_parent.get());

    inner->Append(item);
}

void TreeViewNodeVector::InsertAtCore(unsigned int index, winrt::TreeViewNode const& item)
{
    auto inner = GetVectorInnerImpl();

    MUX_ASSERT(m_parent.get());
    winrt::get_self<TreeViewNode>(item)->put_ParentImpl(m_parent.get());

    inner->InsertAt(index, item);
}

void TreeViewNodeVector::SetAtCore(unsigned int index, winrt::TreeViewNode const& item)
{
    auto inner = GetVectorInnerImpl();

    auto oldNode = inner->GetAt(index);
    winrt::get_self<TreeViewNode>(oldNode)->put_ParentImpl(nullptr);

    MUX_ASSERT(m_parent.get());
    winrt::get_self<TreeViewNode>(item)->put_ParentImpl(m_parent.get());

    inner->SetAt(index, item);
}

void TreeViewNodeVector::RemoveAtCore(unsigned int index)
{
    auto inner = GetVectorInnerImpl();
    auto targetNode = inner->GetAt(index);

    winrt::get_self<TreeViewNode>(targetNode)->put_ParentImpl(nullptr);

    inner->RemoveAt(index);
}

void TreeViewNodeVector::RemoveAtEndCore()
{
    auto inner = GetVectorInnerImpl();

    auto index = inner->Size() - 1;
    auto targetNode = inner->GetAt(index);

    winrt::get_self<TreeViewNode>(targetNode)->put_ParentImpl(nullptr);

    inner->RemoveAtEnd();
}

void TreeViewNodeVector::ReplaceAllCore(winrt::array_view<winrt::TreeViewNode const> values)
{
    auto inner = GetVectorInnerImpl();

    auto count = inner->Size();

    if (count > 0)
    {
        // Clear parent on outgoing
        for (unsigned int i = 0; i < count; i++)
        {
            auto targetNode = inner->GetAt(i);

            winrt::get_self<TreeViewNode>(targetNode)->put_ParentImpl(nullptr);
        }

        // Set parent on new elements
        MUX_ASSERT(m_parent.get());
        for (auto& value : values)
        {
            winrt::get_self<TreeViewNode>(value)->put_ParentImpl(m_parent.get());
        }

        inner->ReplaceAll(values);
    }
}

void TreeViewNodeVector::ClearCore()
{
    auto inner = GetVectorInnerImpl();

    auto count = inner->Size();

    if (count > 0)
    {
        for (unsigned int i = 0; i < count; i++)
        {
            auto targetNode = inner->GetAt(i);

            winrt::get_self<TreeViewNode>(targetNode)->put_ParentImpl(nullptr);
        }

        inner->Clear();
    }
}

void TreeViewNodeVector::Append(winrt::TreeViewNode const& item)
{
    if (!IsParentInContentMode())
    {
        AppendCore(item);
    }
    else
    {
        throwIllegalMethodCallException();
    }
}
void TreeViewNodeVector::InsertAt(unsigned int index, winrt::TreeViewNode const& item)
{
    if (!IsParentInContentMode())
    {
        InsertAtCore(index, item);
    }
    else
    {
        throwIllegalMethodCallException();
    }
}

void TreeViewNodeVector::SetAt(unsigned int index, winrt::TreeViewNode const& item)
{
    if (!IsParentInContentMode())
    {
        SetAtCore(index, item);
    }
    else
    {
        throwIllegalMethodCallException();
    }
}

void TreeViewNodeVector::RemoveAt(unsigned int index)
{
    if (!IsParentInContentMode())
    {
        RemoveAtCore(index);
    }
    else
    {
        throwIllegalMethodCallException();
    }
}

void TreeViewNodeVector::RemoveAtEnd()
{
    if (!IsParentInContentMode())
    {
        RemoveAtEndCore();
    }
    else
    {
        throwIllegalMethodCallException();
    }
}

void TreeViewNodeVector::ReplaceAll(winrt::array_view<winrt::TreeViewNode const> values)
{
    if (!IsParentInContentMode())
    {
        ReplaceAllCore(values);
    }
    else
    {
        throwIllegalMethodCallException();
    }
}

void TreeViewNodeVector::Clear()
{
    if (!IsParentInContentMode())
    {
        ClearCore();
    }
    else
    {
        throwIllegalMethodCallException();
    }
}

#pragma endregion

void throwIllegalMethodCallException()
{
    winrt::throw_hresult(E_ILLEGAL_METHOD_CALL);
}
