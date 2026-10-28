// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pch.h"
#include "common.h"

#include "ShapingPipeline.h"
#include "ShapingHelpers.h"
#include "RowIdentity.h"
#include "LiveShapingTracker.h"

#include <winrt/Microsoft.UI.Dispatching.h>

class ShapedGroup;
class VisibleRowList6;

namespace v6
{
    // A binary indexed tree over the visible-row counts of one sibling list.
    class Fenwick
    {
    public:
        template <typename F>
        void Assign(size_t count, F&& valueAt)
        {
            m_tree.assign(count + 1, 0);
            m_total = 0;
            for (size_t i = 1; i <= count; ++i)
            {
                auto const value = static_cast<int32_t>(valueAt(i - 1));
                m_total += value;
                m_tree[i] += value;
                auto const j = i + (i & (~i + 1));
                if (j <= count)
                {
                    m_tree[j] += m_tree[i];
                }
            }
            m_highBit = 1;
            while ((m_highBit << 1) <= count)
            {
                m_highBit <<= 1;
            }
        }

        void Add(int32_t index, int32_t delta)
        {
            m_total += delta;
            for (auto i = static_cast<size_t>(index) + 1; i < m_tree.size(); i += i & (~i + 1))
            {
                m_tree[i] += delta;
            }
        }

        // The sum of the first `count` values.
        int32_t Prefix(int32_t count) const
        {
            int32_t sum = 0;
            for (auto i = static_cast<size_t>(count); i > 0; i -= i & (~i + 1))
            {
                sum += m_tree[i];
            }
            return sum;
        }

        int32_t Total() const noexcept { return m_total; }

        // The index whose range holds `offset` (0 <= offset < Total()); `offset` becomes the offset
        // within that index's range.
        int32_t Find(int32_t& offset) const
        {
            size_t pos = 0;
            for (auto step = m_highBit; step > 0; step >>= 1)
            {
                if (pos + step < m_tree.size() && m_tree[pos + step] <= offset)
                {
                    pos += step;
                    offset -= m_tree[pos];
                }
            }
            return static_cast<int32_t>(pos);
        }

    private:
        std::vector<int32_t> m_tree;
        int32_t m_total{ 0 };
        size_t m_highBit{ 0 };
    };

    // An observed collection: the bound source, or the collection ChildrenWith returned for a node.
    struct ObservedCollection
    {
        ShapingHelpers::CollectionAccessor Accessor;
        winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged::CollectionChanged_revoker CollectionChangedRevoker{};
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>::VectorChanged_revoker VectorChangedRevoker{};
        winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector::VectorChanged_revoker BindableVectorChangedRevoker{};

        void Revoke() noexcept;
    };

    // One wrapper per item of the model, plus one per group header and the virtual root. Holds the
    // links, the expansion state and the count of visible rows in its subtree.
    struct Node
    {
        // The data item; a GroupedEntry for a header; null for the root.
        winrt::IInspectable Item{ nullptr };
        void const* Id{ nullptr };
        // The presentation parent: the data parent, a group header, or the root.
        Node* Parent{ nullptr };
        // The node whose children collection holds this one; null for top-level rows.
        Node* DataParent{ nullptr };
        // Position in Parent->Kids; -1 when not attached (excluded by the filter).
        int32_t IndexInParent{ -1 };
        // Position in the owning collection.
        int32_t SrcIndex{ -1 };
        // 0 for top-level rows.
        int32_t Depth{ 0 };
        // This row plus its visible descendants (0 + descendants for the root).
        int32_t Visible{ 1 };

        // Children in source order.
        std::vector<Node*> All;
        // Included children in shaped order; KidVisible counts their Visible.
        std::vector<Node*> Kids;
        Fenwick KidVisible;

        std::vector<winrt::IInspectable> SortKeys;
        winrt::IInspectable GroupValue{ nullptr };
        winrt::hstring GroupKey;
        wchar_t const* GroupError{ nullptr };

        std::unique_ptr<ObservedCollection> Children;

        // Header only.
        winrt::com_ptr<ShapedGroup> Group;
        int32_t EntryCount{ -1 };
        bool EntryExpanded{ false };

        bool IsHeader{ false };
        // Expansion intent (headers: the presented state).
        bool Expanded{ false };
        // A context row the user collapsed while the filter is active.
        bool OverlayCollapsed{ false };
        // The presented expansion state.
        bool Open{ false };
        bool Materialized{ false };
        bool HasChildrenHint{ false };
        bool SelfMatch{ true };
        bool Included{ true };
    };
}

// The engine behind TableViewSource6. Rows are identified by object identity and kept in the
// model's own hierarchy: one v6::Node per item, each counting its visible descendants, with a
// Fenwick tree per sibling list. The presented rows are never flattened: GetAt(i) walks down the
// tree, skipping whole branches by their counts.
//
// UI-thread affine, like the other engines: the owner (TableViewSource6) is reference-tracked and
// marshals its teardown to the owning thread.
class ShapedItemsSource6 : public std::enable_shared_from_this<ShapedItemsSource6>
{
public:
    using IdentitySelector = RowIdentity::IdentitySelector;
    using ChildrenSelector = std::function<winrt::IInspectable(winrt::IInspectable const&)>;
    using HasChildrenSelector = std::function<bool(winrt::IInspectable const&)>;
    using Node = v6::Node;

    explicit ShapedItemsSource6(winrt::IInspectable const& items);
    ~ShapedItemsSource6();

    // Not in the constructor: needs weak_from_this. Throws when the source breaks the identity contract.
    void Start();

    // --- Verbs. Each re-evaluates every node and raises Reset. ---
    void SetFilter(ShapingHelpers::Predicate const& predicate);
    void ClearFilter();
    void SetSort(
        winrt::hstring const& previousAxisToken,
        winrt::hstring const& axisToken,
        ShapingHelpers::KeySelector const& key,
        winrt::Windows::Foundation::IUnknown const& keyIdentity,
        winrt::hstring const& sortMemberPath,
        winrt::SortDirection direction);
    void ClearSorts();
    void ClearSort(winrt::hstring const& axisToken);
    void ClearSortsExcept(winrt::hstring const& axisToken);
    void SetGroup(ShapingHelpers::KeySelector const& key, IdentitySelector const& groupIdentity);
    void ClearGroup();
    void SetChildren(ChildrenSelector const& selector, HasChildrenSelector const& hasChildren);
    void ClearChildren();

    bool HasFilter() const noexcept { return m_pipeline.HasFilter(); }
    bool HasGroup() const noexcept { return static_cast<bool>(m_groupKey); }
    bool HasChildren() const noexcept { return static_cast<bool>(m_childrenSelector); }
    std::vector<ShapingHelpers::SortAxis> ActiveSortAxes() const { return m_pipeline.ActiveSortAxes(-1, -1); }

    // --- Live shaping ---
    void SetLiveShaping(bool enabled);
    bool IsLiveShapingEnabled() const noexcept { return m_liveShapingEnabled; }
    void SetLiveShapingHold(std::function<bool()> isHeld) { m_liveShapingHold = std::move(isHeld); }
    void ResumeHeldLiveShaping();
    void SetLiveItemChangedHook(std::function<bool(winrt::IInspectable const&, bool)> hook) { m_liveItemChangedHook = std::move(hook); }
    void SetRowsReleasedHook(std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> hook) { m_rowsReleasedHook = std::move(hook); }
    void SetLiveItemReshapeHandler(std::function<void()> handler) { m_liveItemReshape = std::move(handler); }

    // --- Owner notifications ---
    void SetProjectionRebuiltHandler(std::function<void()> handler) { m_projectionRebuilt = std::move(handler); }
    void SetShapingChangedHandler(std::function<void(bool)> handler) { m_shapingChanged = std::move(handler); }
    // Raised when a row's metadata (expansion, child count) changed without a collection change.
    void SetRowsMetadataChangedHook(std::function<void()> hook) { m_rowsMetadataChanged = std::move(hook); }

    // --- Projection ---
    bool IsGrouped() const noexcept { return HasGroup(); }
    bool IsHierarchical() const noexcept { return HasChildren(); }
    // The presented rows. One list for the engine's life.
    winrt::IInspectable Rows() const;
    int32_t Size() const noexcept { return m_root.Visible; }
    // The node presented at `index`; throws out of bounds.
    Node* NodeAt(int32_t index) const;
    // The presented index of a node, or -1 when it is excluded or under a collapsed row.
    int32_t IndexOf(Node const* node) const;
    // The node for a presented object (a data item or a header entry), or null.
    Node* NodeOfPresented(winrt::IInspectable const& value) const;
    bool ContainsItem(winrt::IInspectable const& item) const;

    // --- Row queries (for the metadata provider) ---
    winrt::hstring IdentityOf(Node const* node) const;
    Node* NodeOfIdentity(winrt::hstring const& identity) const;
    bool IsExpandable(Node const* node) const noexcept;
    int32_t ChildCountOf(Node const* node) const noexcept;
    bool IsRoot(Node const* node) const noexcept { return node == &m_root; }

    // --- Expansion. Each returns true when the state changed. ---
    bool SetExpanded(Node* node, bool expanded);
    void SetAllGroupsExpanded(bool expanded);
    void SetAllNodesExpanded(bool expanded);
    void ExpandSubtree(Node* node);

private:
    // --- Nodes ---
    Node* AllocateNode();
    void FreeNode(Node* node);
    Node* CreateNode(winrt::IInspectable const& item, Node* dataParent);
    void ReleaseSubtree(Node* node, bool notify);
    void ReleaseHeader(Node* header);
    void ReleaseAllChildren();
    std::vector<Node*>& AllOf(Node* owner) { return owner ? owner->All : m_top; }
    v6::ObservedCollection& CollectionOf(Node* owner) { return owner ? *owner->Children : m_topCollection; }
    template <typename F>
    void ForEachDataNode(F&& f)
    {
        std::vector<Node*> stack(m_top.rbegin(), m_top.rend());
        while (!stack.empty())
        {
            auto const node = stack.back();
            stack.pop_back();
            f(node);
            stack.insert(stack.end(), node->All.rbegin(), node->All.rend());
        }
    }

    // --- Evaluation and materialization ---
    void Evaluate(Node* node);
    void EvaluateHint(Node* node);
    void EvaluateAll(bool clearOverlay);
    bool NeedsFullMaterialization() const noexcept;
    void MaterializeOne(Node* node);
    void MaterializeDeep(Node* node);
    void ReloadTop();

    // --- Verbs ---
    void ApplyVerb(bool clearOverlay);
    void Rebuild(bool reload, bool clearOverlay, bool raise);

    // --- Shape ---
    void RefreshAxes();
    int CompareNodes(Node const* a, Node const* b, std::vector<size_t> const& axes) const;
    std::vector<size_t> const& AxesFor(Node const* parent) const;
    void SortKids(Node* parent);
    void SortKidsInPlace(Node* parent);
    bool IsGroupExpanded(winrt::hstring const& groupKey) const;
    bool ComputeOpen(Node const* node) const;
    bool IsContextRow(Node const* node) const noexcept;
    void Recount(Node* node);
    void ShapeNode(Node* node);
    void ShapeSubtree(Node* node);
    void ShapeTop();
    void Shape();
    void RecountAll();
    void ClearProjection();
    void RefreshHeaderEntry(Node* header, bool raise);

    // --- Incremental structure ---
    void PropagateFrom(Node* node, int32_t delta);
    void AfterKidsChanged(Node* parent);
    void AttachCore(Node* node, Node* parent);
    void DetachCore(Node* node);
    Node* PresentationParentFor(Node const* node) const;
    bool Attach(Node* node, bool refreshHeader);
    void Detach(Node* node, bool prune);
    void RemoveHeader(Node* header);
    Node* NextVisible(Node* node) const;
    bool IsAttached(Node const* node) const;
    void InvalidateCursor() const noexcept { m_cursorNode = nullptr; m_cursorIndex = -1; }

    // --- Source ---
    void Subscribe(Node* owner);
    void OnCollectionChanged(Node* owner, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnVectorChanged(Node* owner, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void HandleSourceChange(std::function<void()> const& apply);
    bool PostDeferredReload();
    void InsertItems(Node* owner, uint32_t index, std::vector<winrt::IInspectable> const& items);
    void RemoveItems(Node* owner, uint32_t index, uint32_t count);
    void ResetOwner(Node* owner);
    std::vector<winrt::IInspectable> SnapshotPresented() const;
    void RaisePresentedDiff(std::vector<winrt::IInspectable> const& before);

    // --- Publishing ---
    void RaiseAdded(int32_t index, int32_t count);
    void RaiseRemoved(int32_t index, int32_t count);
    void RaiseReplaced(int32_t index);
    void RaiseReset();
    void RaiseShapingChanged(bool reorderOnly);
    void NotifyRowsMetadataChanged();

    // --- Live shaping ---
    void RefreshLiveSubscriptions();
    void ClearLiveSubscriptions();
    bool ShapeChanged(Node const* node, Node const& fresh, bool& needsFull) const;
    void OnLiveItemChanged(winrt::IInspectable const& item);
    void MarkLiveDirty();
    void MarkLiveDirtyForItem();
    void EnsureLiveRestore();
    void PostLiveRestore();
    void RestoreLive();
    bool TryResplice(std::vector<winrt::IInspectable> const& items);
    winrt::IInspectable m_source{ nullptr };
    v6::ObservedCollection m_topCollection;

    ShapingHelpers::ShapingPipeline m_pipeline;
    std::vector<ShapingHelpers::SortAxis> m_axes;
    std::vector<size_t> m_allAxes;
    // Sorts declared before GroupBy order the headers; later ones order the rows in each group.
    std::vector<size_t> m_preAxes;
    // Rows within a group: the later sorts, then the earlier ones.
    std::vector<size_t> m_headerKidAxes;
    ShapingHelpers::KeySelector m_groupKey{ nullptr };
    IdentitySelector m_groupIdentity{ nullptr };
    ChildrenSelector m_childrenSelector{ nullptr };
    HasChildrenSelector m_hasChildren{ nullptr };

    std::deque<Node> m_pool;
    std::vector<Node*> m_free;
    Node m_root;
    // Top-level data rows, in source order.
    std::vector<Node*> m_top;
    std::unordered_map<void const*, Node*> m_byItem;
    std::unordered_map<void const*, Node*> m_byEntry;
    std::unordered_map<winrt::hstring, Node*> m_headers;
    // A group is expanded when the default differs from its presence in the toggled set.
    bool m_groupDefaultExpanded{ true };
    std::unordered_set<winrt::hstring> m_groupToggled;
    bool m_nodeDefaultExpanded{ false };
    bool m_deferEvaluate{ false };

    winrt::com_ptr<VisibleRowList6> m_list;
    // Serves sequential GetAt calls.
    mutable Node* m_cursorNode{ nullptr };
    mutable int32_t m_cursorIndex{ -1 };

    std::function<void()> m_projectionRebuilt;
    std::function<void(bool)> m_shapingChanged;
    std::function<void()> m_rowsMetadataChanged;

    winrt::Microsoft::UI::Dispatching::DispatcherQueue m_ownerQueue{ nullptr };
    DWORD m_ownerThreadId{};
    std::unique_ptr<LiveShapingTracker> m_liveShaping{ std::make_unique<LiveShapingTracker>() };
    bool m_liveShapingEnabled{ false };
    bool m_liveShapingDirty{ false };
    bool m_liveShapingHeld{ false };
    bool m_liveShapingRestorePosted{ false };
    // A change that cannot be applied by moving its row (filter or group) needs a full reshape.
    bool m_liveFullRestore{ false };
    std::vector<winrt::IInspectable> m_liveDirtyItems;
    std::shared_ptr<std::atomic<bool>> m_liveShapingChangeDropped{ std::make_shared<std::atomic<bool>>(false) };
    std::function<bool()> m_liveShapingHold;
    std::function<bool(winrt::IInspectable const&, bool)> m_liveItemChangedHook;
    std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> m_rowsReleasedHook;
    std::function<void()> m_liveItemReshape;

    // A failed source update left the nodes out of step with the source; the next verb reloads.
    bool m_cacheInvalid{ false };
};
