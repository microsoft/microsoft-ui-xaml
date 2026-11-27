// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      The tree of a hierarchical CollectionView (VectorCollectionView_Hierarchy.cpp): a node per source item, linked
//      by key and parent key, and the public node objects (ICollectionViewNode) created on demand.

#pragma once

#include "VectorCollectionView_ShapingInternal.h"
#include <functional>
#include <memory>

namespace DirectUI::ShapingEngine
{
    using NodeId = UINT;
    constexpr NodeId c_noNode = UINT_MAX;
    constexpr UINT c_noTrackerSlot = UINT_MAX;

    // A key compared by type and value: Int32 5 and Int64 5 differ, strings compare ordinally, other objects by identity.
    struct NodeKey
    {
        wf::PropertyType type = wf::PropertyType_Empty;
        bool isObject = false;
        UINT64 bits = 0;
        UINT64 bits2 = 0;
        std::wstring text;

        bool IsEmpty() const
        {
            return type == wf::PropertyType_Empty && !isObject;
        }

        bool operator==(const NodeKey& other) const
        {
            return type == other.type && isObject == other.isObject && bits == other.bits && bits2 == other.bits2 && text == other.text;
        }

        bool operator!=(const NodeKey& other) const
        {
            return !(*this == other);
        }
    };

    struct NodeKeyHash
    {
        size_t operator()(const NodeKey& key) const
        {
            size_t hash = key.text.empty() ? std::hash<UINT64>()(key.bits) : std::hash<std::wstring>()(key.text);
            hash ^= std::hash<UINT64>()(key.bits2) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            return hash * 31 + static_cast<size_t>(key.type) * 2 + (key.isObject ? 1 : 0);
        }
    };

    // *pIsObject: the key is an object compared by identity, which the store must keep alive.
    _Check_return_ HRESULT MakeNodeKey(_In_opt_ IInspectable* pValue, _Out_ NodeKey* pKey);

    struct HierarchyNode
    {
        UINT slot = SortKeyStore::c_noSlot;
        NodeId parent = c_noNode;
        // Children by parent key, in no order; the kept ones, sorted.
        std::vector<NodeId> children;
        std::vector<NodeId> kept;
        // Kept nodes in the subtree, this one included, while this one is kept.
        UINT keptSubtree = 0;
        // Descendants that pass the filter.
        UINT matchingBelow = 0;
        NodeKey key;
        NodeKey parentKey;
        UINT keyObjectSlot = c_noTrackerSlot;
        UINT parentKeyObjectSlot = c_noTrackerSlot;
        UINT projectionSlot = c_noTrackerSlot;
        class CollectionViewNode* pProjection = nullptr;
        bool isAllocated = false;
        bool isMatch = false;
        bool isKept = false;
        bool isOwner = false;
        bool isWaiting = false;
        bool isDuplicate = false;
        bool isCycleBroken = false;

        bool IsKeptByRule() const
        {
            return isMatch || matchingBelow > 0;
        }
    };

    class HierarchyStore;

    // CollectionNodes and a node's Children: a read-only view of a kept list of the store, so Size costs nothing and a
    // child's public node is created when it is read. The store raises the changes, each after the list changed.
    class __declspec(novtable) NodeListVector :
        public wfc::IObservableVector<IInspectable*>,
        public wfc::IVector<IInspectable*>,
        public wfc::IIterable<IInspectable*>,
        public ctl::WeakReferenceSource
    {
    protected:
        NodeListVector() = default;

        BEGIN_INTERFACE_MAP(NodeListVector, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(NodeListVector, wfc::IObservableVector<IInspectable*>)
            INTERFACE_ENTRY(NodeListVector, wfc::IVector<IInspectable*>)
            INTERFACE_ENTRY(NodeListVector, wfc::IIterable<IInspectable*>)
        END_INTERFACE_MAP(NodeListVector, ctl::WeakReferenceSource)

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
        void OnReferenceTrackerWalk(INT walkType) override;

    public:
        // owner: the node whose kept children this is, or c_noNode for the roots.
        void Bind(_In_ const std::shared_ptr<HierarchyStore>& spStore, NodeId owner);
        // The list leaves the store and is empty from then on; isRaised: a Reset tells its handlers.
        _Check_return_ HRESULT Detach(bool isRaised);
        _Check_return_ HRESULT RaiseChanged(wfc::CollectionChange change, UINT index);

        IFACEMETHODIMP add_VectorChanged(_In_ wfc::VectorChangedEventHandler<IInspectable*>* pHandler, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHODIMP remove_VectorChanged(EventRegistrationToken token) override;
        IFACEMETHODIMP GetAt(UINT index, _Outptr_ IInspectable** ppItem) override;
        IFACEMETHODIMP get_Size(_Out_ UINT* pSize) override;
        IFACEMETHODIMP GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable*>** ppView) override;
        IFACEMETHODIMP IndexOf(_In_opt_ IInspectable* pValue, _Out_ UINT* pIndex, _Out_ BOOLEAN* pFound) override;
        IFACEMETHODIMP SetAt(UINT index, _In_opt_ IInspectable* pItem) override;
        IFACEMETHODIMP InsertAt(UINT index, _In_opt_ IInspectable* pItem) override;
        IFACEMETHODIMP RemoveAt(UINT index) override;
        IFACEMETHODIMP Append(_In_opt_ IInspectable* pItem) override;
        IFACEMETHODIMP RemoveAtEnd() override;
        IFACEMETHODIMP Clear() override;
        IFACEMETHODIMP First(_Outptr_ wfc::IIterator<IInspectable*>** ppIterator) override;

    private:
        // A copy of the list, for GetView and First.
        _Check_return_ HRESULT Snapshot(_Outptr_ InternalList** ppList);

        std::weak_ptr<HierarchyStore> m_store;
        NodeId m_owner = c_noNode;
        TrackerEventSource<wfc::VectorChangedEventHandler<IInspectable*>, wfc::IObservableVector<IInspectable*>, wfc::IVectorChangedEventArgs> m_handlers;
    };

    // The public node (WPF has none; modelled on CollectionViewGroupInternal). Children is created on first read and
    // follows the store from then on. The node leaves the store when its item is removed or filtered out, or when the
    // view recomputes; it then keeps its last values and Parent is null.
    class __declspec(novtable) CollectionViewNode :
        public xaml_data::ICollectionViewNode,
        public xaml_data::ICustomPropertyProvider,
        public xaml_data::INotifyPropertyChanged,
        public ctl::WeakReferenceSource
    {
    protected:
        CollectionViewNode() = default;

        BEGIN_INTERFACE_MAP(CollectionViewNode, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(CollectionViewNode, xaml_data::ICollectionViewNode)
            INTERFACE_ENTRY(CollectionViewNode, xaml_data::ICustomPropertyProvider)
            INTERFACE_ENTRY(CollectionViewNode, xaml_data::INotifyPropertyChanged)
        END_INTERFACE_MAP(CollectionViewNode, ctl::WeakReferenceSource)

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
        void OnReferenceTrackerWalk(INT walkType) override;

    public:
        _Check_return_ HRESULT Initialize(_In_ const std::shared_ptr<HierarchyStore>& spStore, NodeId id, _In_opt_ IInspectable* pItem, _In_opt_ IInspectable* pKey, bool isContext);

        // The store lets go of the node; isRaised: its Children tells its handlers it emptied.
        void Detach(bool isRaised);
        NodeId Id() const
        {
            return m_id;
        }
        NodeListVector* ChildrenNoRef() const
        {
            return m_tpChildren.Get();
        }
        _Check_return_ HRESULT SetItem(_In_opt_ IInspectable* pItem);
        _Check_return_ HRESULT SetIsContext(bool isContext);

        // ICollectionViewNode
        IFACEMETHODIMP get_Item(_Outptr_result_maybenull_ IInspectable** ppValue) override;
        IFACEMETHODIMP get_Key(_Outptr_result_maybenull_ IInspectable** ppValue) override;
        IFACEMETHODIMP get_Parent(_Outptr_result_maybenull_ xaml_data::ICollectionViewNode** ppValue) override;
        IFACEMETHODIMP get_Children(_Outptr_ wfc::IObservableVector<IInspectable*>** ppValue) override;
        IFACEMETHODIMP get_IsContext(_Out_ BOOLEAN* pValue) override;

        // INotifyPropertyChanged
        IFACEMETHODIMP add_PropertyChanged(_In_ xaml_data::IPropertyChangedEventHandler* pHandler, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHODIMP remove_PropertyChanged(EventRegistrationToken token) override;

        // ICustomPropertyProvider ({Binding Item.Name} on a node)
        IFACEMETHODIMP get_Type(_Out_ wxaml_interop::TypeName* pValue) override;
        IFACEMETHODIMP GetCustomProperty(_In_ HSTRING name, _Outptr_ xaml_data::ICustomProperty** ppValue) override;
        IFACEMETHODIMP GetIndexedProperty(_In_ HSTRING name, _In_ wxaml_interop::TypeName type, _Outptr_ xaml_data::ICustomProperty** ppValue) override;
        IFACEMETHODIMP GetStringRepresentation(_Out_ HSTRING* pValue) override;

    private:
        static CollectionViewNode* FromTarget(_In_ IInspectable* pTarget);
        _Check_return_ HRESULT RaisePropertyChanged(_In_z_ const WCHAR* pName);

        std::weak_ptr<HierarchyStore> m_store;
        NodeId m_id = c_noNode;
        TrackerPtr<IInspectable> m_tpItem;
        TrackerPtr<IInspectable> m_tpKey;
        TrackerPtr<NodeListVector> m_tpChildren;
        bool m_isContext = false;
        TrackerEventSource<xaml_data::IPropertyChangedEventHandler, IInspectable, xaml_data::IPropertyChangedEventArgs> m_propertyChangedHandlers;
    };

    // Where a tree change is published: the view's flat list (pre-order) and CollectionNodes.
    struct HierarchyOutput
    {
        InternalList* pFlat = nullptr;
        // Null while nobody has read CollectionNodes.
        NodeListVector* pRoots = nullptr;
    };

    // Data errors the view logs once per recompute.
    struct HierarchyDiagnostics
    {
        UINT duplicateCount = 0;
        UINT cycleCount = 0;
    };

    // The view's tree: every source item has a node (filtered or not), so a filtered-out parent never orphans its
    // children. Kept nodes (those that pass the filter, and their ancestors as context) form sorted sibling lists.
    // The public nodes are created on demand.
    class HierarchyStore : public std::enable_shared_from_this<HierarchyStore>
    {
    public:
        // A list is published with a vector change per row up to this many rows, else as one Reset.
        static constexpr UINT c_maxFlatEvents = 64;

        std::shared_ptr<ShapingSortState> sort;
        ShapingKeySelector keySelector;
        ShapingKeySelector parentKeySelector;
        std::vector<HierarchyNode> nodes;
        // By node: its source index (apart from the nodes, so a shift touches 4 bytes a node).
        std::vector<UINT> sourceIndexOf;
        std::vector<NodeId> freeNodes;
        std::vector<NodeId> bySource;
        std::unordered_map<NodeKey, NodeId, NodeKeyHash> owners;
        // Nodes whose parent key no item has, by that key; and nodes whose key an earlier item owns.
        std::unordered_map<NodeKey, std::vector<NodeId>, NodeKeyHash> waiting;
        std::unordered_map<NodeKey, std::vector<NodeId>, NodeKeyHash> duplicates;
        std::vector<NodeId> roots;
        UINT cycleBrokenCount = 0;
        bool isDuplicateLogged = false;
        // A source item that changed sort keys since it was placed; not compared when another is placed.
        std::function<bool(UINT)> isSortSkipped;

        ~HierarchyStore();

        void ReferenceTrackerWalk(EReferenceTrackerWalkType walkType);

        _Check_return_ HRESULT GetItem(NodeId id, _Outptr_result_maybenull_ IInspectable** ppItem)
        {
            return m_items.GetAt(id, ppItem);
        }

        // A new node for a source item, not linked.
        _Check_return_ HRESULT AllocateNode(_In_opt_ IInspectable* pItem, UINT sourceIndex, NodeKey&& key, _In_opt_ IInspectable* pKeyObject, NodeKey&& parentKey, _In_opt_ IInspectable* pParentKeyObject, bool isMatch, SortKeys&& keys, _Out_ NodeId* pId);
        _Check_return_ HRESULT FreeNode(NodeId id);
        void Reserve(UINT count);

        // Full build: owners, links, broken cycles, filter counts, sorted kept lists.
        _Check_return_ HRESULT BuildTree(_Out_ HierarchyDiagnostics* pDiagnostics);
        // The kept nodes in pre-order.
        void CollectPreOrder(std::vector<NodeId>& order) const;

        std::vector<NodeId>& KeptList(NodeId parent)
        {
            return (parent == c_noNode) ? roots : nodes[parent].kept;
        }

        // The position of a kept node among its kept siblings.
        UINT PositionOf(NodeId id);
        // The index of a kept, attached node in the flat pre-order list.
        UINT FlatIndexOf(NodeId id);

        // Incremental changes. Each publishes what it changes; the store then matches every event raised.
        _Check_return_ HRESULT InsertNode(NodeId id, const HierarchyOutput& output);
        _Check_return_ HRESULT RemoveNode(NodeId id, const HierarchyOutput& output);
        // The node's item, filter result or sort keys changed (same key and parent key).
        _Check_return_ HRESULT UpdateNode(NodeId id, _In_opt_ IInspectable* pItem, bool isMatch, _In_opt_ const SortKeys* pKeys, const HierarchyOutput& output);

        // Shifts the source indices for an insert (delta 1) or a remove (delta -1) at index.
        void ShiftSourceIndices(UINT index, int delta);

        // Whether an item with these keys inserted at sourceIndex needs a recompute (it would take a key from a later
        // duplicate, or close a cycle).
        bool IsInsertComplex(UINT sourceIndex, const NodeKey& key, const NodeKey& parentKey) const;
        bool IsRemoveComplex(NodeId id) const;

        _Check_return_ HRESULT GetProjection(NodeId id, _Outptr_ xaml_data::ICollectionViewNode** ppNode);
        // The kept list of a node (c_noNode: the roots) as a NodeListVector reads it; empty for a node no longer kept.
        UINT KeptCount(NodeId owner) const;
        NodeId KeptAt(NodeId owner, UINT index) const;
        _Check_return_ HRESULT GetParentProjection(NodeId id, _Outptr_result_maybenull_ xaml_data::ICollectionViewNode** ppNode);
        _Check_return_ HRESULT FindKeptNode(_In_opt_ IInspectable* pItem, _Out_ NodeId* pId);
        _Check_return_ HRESULT FlatItems(const std::vector<NodeId>& order, _Out_ std::vector<ctl::ComPtr<IInspectable>>* pItems);

        // Detaches every public node (the view recomputed or stopped being hierarchical).
        void DetachProjections(bool isRaised);

    private:
        _Check_return_ HRESULT CompareNodes(NodeId left, NodeId right, _Out_ int* pResult);
        _Check_return_ HRESULT CompareWithKeys(NodeId id, const SortKeys& keys, NodeId other, _Out_ int* pResult);
        _Check_return_ HRESULT SortKept(std::vector<NodeId>& list);
        _Check_return_ HRESULT FindInsertPosition(NodeId id, const std::vector<NodeId>& list, _Out_ UINT* pPosition);
        _Check_return_ HRESULT Attach(NodeId id, const HierarchyOutput& output);
        _Check_return_ HRESULT Detach(NodeId id, const HierarchyOutput& output);
        _Check_return_ HRESULT PromoteAncestors(NodeId child, const HierarchyOutput& output);
        _Check_return_ HRESULT DemoteAncestors(NodeId from, const HierarchyOutput& output);
        void AddMatches(NodeId from, int delta);
        void ReleaseProjection(NodeId id);
        _Check_return_ HRESULT RepublishFlat(const HierarchyOutput& output);
        _Check_return_ HRESULT SetIsContext(NodeId id);
        UINT AllocateKeyObject(_In_opt_ IInspectable* pObject);
        void FreeKeyObject(UINT slot);
        _Check_return_ HRESULT AppendNodeVector(NodeId parent, UINT position, NodeId id, const HierarchyOutput& output);
        NodeListVector* NodeVector(NodeId parent, const HierarchyOutput& output) const;

        TrackerPtrVector<IInspectable> m_items;
        TrackerPtrVector<IInspectable> m_keyObjects;
        std::vector<UINT> m_freeKeyObjects;
        TrackerPtrVector<IInspectable> m_projections;
        std::vector<UINT> m_freeProjections;
    };
}
