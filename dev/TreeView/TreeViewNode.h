// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "Vector.h"

#include "TreeViewNode.g.h"
#include "TreeViewNode.properties.h"

class TreeViewNode :
    public ReferenceTracker<TreeViewNode, winrt::implementation::TreeViewNodeT, winrt::Windows::UI::Xaml::Data::ICustomPropertyProvider, winrt::Windows::Foundation::IStringable>,
    public TreeViewNodeProperties
{
public:

    enum class TreeNodeSelectionState
    {
        UnSelected,
        PartialSelected,
        Selected
    };

    TreeViewNode();

    winrt::TreeViewNode Parent();
    bool HasUnrealizedChildren();
    void HasUnrealizedChildren(bool value);
    winrt::IVector<winrt::TreeViewNode> Children();
    bool IsContentMode();
    void IsContentMode(bool value);

    void ChildVectorChanged(winrt::IObservableVector<winrt::TreeViewNode> const& sender, winrt::IInspectable const& args);
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    winrt::event_token AddExpandedChanged(winrt::TypedEventHandler<winrt::TreeViewNode, winrt::DependencyPropertyChangedEventArgs> const& value);
    void RemoveExpandedChanged(winrt::event_token);
    winrt::event_token ChildrenChanged(winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IVectorChangedEventArgs> const& value);
    void ChildrenChanged(winrt::event_token);

    winrt::IInspectable ItemsSource();
    void ItemsSource(winrt::IInspectable const& value);
    TreeNodeSelectionState SelectionState();
    void SelectionState(TreeNodeSelectionState const& state);

// Enable "ToString" on TreeViewNode to show stringable data correctly
#pragma region ICustomPropertyProvider
    winrt::TypeName Type();
    winrt::ICustomProperty GetCustomProperty(hstring const& name);
    winrt::ICustomProperty GetIndexedProperty(hstring const& name, winrt::TypeName const& type);
    hstring GetStringRepresentation();
#pragma endregion

#pragma region IStringable
    hstring ToString();
#pragma endregion

private:
    // Variables
    winrt::weak_ref<winrt::TreeViewNode> m_parentNode{ nullptr };
    event_source<winrt::TypedEventHandler<winrt::TreeViewNode, winrt::IVectorChangedEventArgs>> m_childrenChangedSource{ this };
    event_source<winrt::TypedEventHandler<winrt::TreeViewNode, winrt::DependencyPropertyChangedEventArgs>> m_propertyChangedEventSource{ this };
    winrt::ItemsSourceView::CollectionChanged_revoker m_itemItemsSourceViewChangedRevoker{};
    bool m_HasUnrealizedChildren{ false };
    bool m_isExpanded{ false };
    tracker_ref<winrt::IVector<winrt::TreeViewNode>> m_children{ this };
    tracker_ref<winrt::IInspectable> m_itemsSource{ this };
    winrt::ItemsSourceView m_itemsDataSource{ nullptr };
    void OnItemsSourceChanged(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args);
    void SyncChildrenNodesWithItemsSource();
    bool AreChildrenNodesEqualToItemsSource();
    void AddToChildrenNodes(int index, int count);
    void RemoveFromChildrenNodes(int index, int count);
    bool m_isContentMode{ false };
    TreeNodeSelectionState m_multiSelectionState{ TreeNodeSelectionState::UnSelected };
    hstring GetContentAsString();

public:
    // Impls and Helpers
    winrt::TreeViewNode get_ParentImpl();
    void put_ParentImpl(winrt::TreeViewNode const& value);
    void UpdateDepth(int depth);
    void UpdateHasChildren();
    void RaiseChildrenChanged(winrt::CollectionChange CC, unsigned int index);
};

typedef typename VectorOptionsFromFlag<winrt::TreeViewNode, MakeVectorParam<VectorFlag::Observable, VectorFlag::DependencyObjectBase>()> TreeViewNodeVectorOptions;

class TreeViewNodeVector :
    public ReferenceTracker<
    TreeViewNodeVector,
    reference_tracker_implements_t<typename TreeViewNodeVectorOptions::VectorType>::type,
    typename TreeViewNodeVectorOptions::IterableType,
    typename TreeViewNodeVectorOptions::ObservableVectorType>,
    public TreeViewNodeVectorOptions::IVectorOwner
{
    Implement_Vector_Read(TreeViewNodeVectorOptions)

private:
    winrt::weak_ref<winrt::TreeViewNode> m_parent{ nullptr };

public:
    
    TreeViewNodeVector();
    TreeViewNodeVector(unsigned int capacity);

    TreeViewNode* Parent();
    void SetParent(winrt::TreeViewNode value);
    winrt::IBindableVector GetWritableParentItemsSource();

    void Append(winrt::TreeViewNode const& item, bool updateItemsSource = true);   
    void InsertAt(unsigned int index, winrt::TreeViewNode const& item, bool updateItemsSource = true);
    void SetAt(unsigned int index, winrt::TreeViewNode const& item, bool updateItemsSource = true);   
    void RemoveAt(unsigned int index, bool updateItemsSource = true, bool updateIsExpanded = true);
    void RemoveAtEnd(bool updateItemsSource = true);
    void ReplaceAll(winrt::array_view<winrt::TreeViewNode const> values, bool updateItemsSource = true);    
    void Clear(bool updateItemsSource = true, bool updateIsExpanded = true);
};
