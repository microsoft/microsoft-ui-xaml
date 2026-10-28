// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>
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
#include "RowExpansionModel.h"
#include "LiveShapingTracker.h"

#include <winrt/Microsoft.UI.Dispatching.h>

class ShapedGroup;

// The engine behind TableViewSource3: rows are identified by an app-supplied key, and every shape
// (flat, grouped, hierarchical, grouped + hierarchical) is one pre-order row array with a depth
// per row.
//
// Group headers are synthetic parent rows, so a subtree, a group and a collapsed range are all the
// same thing: a contiguous range [pos, End) of that array. The presented rows are a scan over it
// that skips collapsed ranges, applied to one observable vector as a minimal diff.
//
// Per-source-row results of the app's delegates (key, filter, sort keys, group key, parent key)
// are cached, so expansion never calls app code and a source change evaluates only the rows it
// touched. A verb re-evaluates the whole source, as v1 does.
//
// Unlike ShapedItemsSource2, a source edit, a single expand or collapse and a live change update
// the row array in place: the changed row's subtree is cut out of its sibling run and spliced into
// its new one, and the presented vector receives the matching removes, inserts and restamps. A
// change whose effects are not local to that subtree (a new or emptied group, a row adopting
// existing rows, a filter context row losing its last match, a key change, a group reorder) falls
// back to the full rebuild, which yields the same rows.
//
// UI-thread affine, like ShapedItemsSource: the owner (TableViewSource3) is reference-tracked and
// marshals its teardown to the owning thread.
class ShapedItemsSource3 : public std::enable_shared_from_this<ShapedItemsSource3>
{
public:
    using IdentitySelector = RowIdentity::IdentitySelector;

    enum class ProjectionKind
    {
        Flat,
        Grouped,
        Hierarchical,
        GroupedHierarchical,
    };

    // One row of the pre-order array. A group header has Src == -1.
    struct Row
    {
        int32_t Src{ -1 };
        // Bucket index into the current groups; -1 when ungrouped.
        int32_t Group{ -1 };
        int32_t Depth{ 0 };
        // Exclusive end of this row's subtree.
        int32_t End{ 0 };
        int32_t ParentPos{ -1 };
        // Included children (a header: its roots or items).
        int32_t ChildCount{ 0 };
        // 1-based within the sibling run (a root: within its group); 0 for headers.
        uint32_t PosInSet{ 0 };
        uint32_t SizeOfSet{ 0 };
        // Presented expansion state (false for a row without children).
        bool Expanded{ false };
    };

    struct GroupInfo
    {
        winrt::hstring GroupKey;
        // Units (items, or roots under ParentBy) in the bucket.
        int32_t ItemCount{ 0 };
        int32_t HeaderPos{ -1 };
        // The presented header row (a GroupedEntry).
        winrt::IInspectable Entry{ nullptr };
    };

    ShapedItemsSource3(winrt::IInspectable const& items, IdentitySelector const& keySelector);
    ~ShapedItemsSource3();

    // Not in the constructor: needs weak_from_this. Throws when the source breaks the key contract.
    void Start();

    // --- Verbs. Each re-evaluates the source and republishes; a failure keeps the declaration. ---
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
    void SetParent(IdentitySelector const& parentSelector);
    void ClearParent();

    bool HasFilter() const noexcept { return m_pipeline.HasFilter(); }
    bool HasGroup() const noexcept { return static_cast<bool>(m_groupKey); }
    bool HasParent() const noexcept { return static_cast<bool>(m_parentSelector); }
    bool HasSortAxis(winrt::hstring const& token) const noexcept { return m_pipeline.HasSortAxis(token); }
    bool HasSortOtherThan(winrt::hstring const& token) const noexcept { return m_pipeline.HasSortOtherThan(token); }
    bool HasMatchingSort(winrt::hstring const& token, winrt::Windows::Foundation::IUnknown const& keyIdentity) const noexcept
    {
        return m_pipeline.HasMatchingSort(token, keyIdentity);
    }
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
    // After a verb republished: the owner re-reads the projection it cached.
    void SetProjectionRebuiltHandler(std::function<void()> handler) { m_projectionRebuilt = std::move(handler); }
    // After a verb or live restore rewrote the rows.
    void SetShapingChangedHandler(std::function<void(bool)> handler) { m_shapingChanged = std::move(handler); }

    // --- Projection ---
    ProjectionKind Kind() const noexcept;
    bool IsGrouped() const noexcept { return HasGroup(); }
    bool IsHierarchical() const noexcept { return HasParent(); }
    // The presented rows: data items and GroupedEntry headers. One vector for the engine's life.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& Rows() const noexcept { return m_presented; }
    uint32_t SourceRowCount() const noexcept { return static_cast<uint32_t>(m_src.size()); }
    // Data rows the filter keeps, regardless of expansion.
    uint32_t IncludedDataRowCount() const noexcept { return m_includedDataRows; }
    // True when `item` is a source row the filter keeps.
    bool ContainsItem(winrt::IInspectable const& item) const;

    // --- Row queries (for the metadata provider) ---
    std::vector<Row> const& PreOrderRows() const noexcept { return m_rows; }
    std::vector<GroupInfo> const& Groups() const noexcept { return m_groups; }
    // The pre-order position of a presented row object, or -1.
    int32_t PositionOfPresented(winrt::IInspectable const& row) const;
    // The presented index of a pre-order position, or -1 when it is hidden.
    int32_t PresentedIndexOf(int32_t pos) const;
    winrt::hstring const& KeyAt(int32_t pos) const;
    // The pre-order position of a row key, or -1.
    int32_t PositionOfKey(winrt::hstring const& key) const;
    // The pre-order position of a group header, or -1.
    int32_t PositionOfGroup(winrt::hstring const& groupKey) const;
    // True when the row is kept only because a descendant matches the filter.
    bool IsContextRow(int32_t pos) const;

    // --- Expansion. Each returns true when the presented state changed. ---
    bool SetNodeExpanded(winrt::hstring const& key, bool expanded);
    bool SetGroupExpanded(winrt::hstring const& groupKey, bool expanded);
    void SetAllGroupsExpanded(bool expanded);
    void SetAllNodesExpanded(bool expanded);
    void ExpandSubtree(winrt::hstring const& key);

private:
    enum class UpdateReason
    {
        Verb,
        Expansion,
        Source,
        Live,
    };

    struct SrcRow
    {
        winrt::IInspectable Item{ nullptr };
        winrt::hstring Key;
        winrt::hstring ParentKey;
        winrt::IInspectable GroupValue{ nullptr };
        winrt::hstring GroupKey;
        // Why GroupKey is unusable, or null.
        wchar_t const* GroupError{ nullptr };
        std::vector<winrt::IInspectable> SortKeys;
        bool SelfMatch{ true };
    };

    // What decides a presented row's metadata, compared to restamp rows whose object is unchanged.
    struct RowSignature
    {
        int32_t Depth{};
        int32_t ChildCount{};
        uint32_t PosInSet{};
        uint32_t SizeOfSet{};
        bool Expanded{};
        bool operator==(RowSignature const&) const = default;
    };

    // --- Source ---
    void Subscribe();
    void Unsubscribe();
    void OnSourceChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void OnSourceReset();
    void ReloadAll();

    // What a source edit left to do: reload the source, rebuild the structure, or nothing.
    enum class EditResult
    {
        Reload,
        Rebuild,
        Done,
    };
    EditResult ApplyInsert(uint32_t index, uint32_t count);
    EditResult ApplyRemove(uint32_t index, uint32_t count);
    EditResult ApplyReplace(uint32_t index, uint32_t count);
    void FinishSourceEdit(EditResult result);
    EditResult SourceEditResult();

    void RebuildLookups();
    winrt::hstring EvaluateKey(winrt::IInspectable const& item) const;
    void Evaluate(SrcRow& row);
    void EvaluateAll();
    void ReleaseRow(winrt::IInspectable const& item);

    // Source rows, with the lookups and parent references kept in step (no full rebuild).
    void InsertSrcRow(int32_t s, SrcRow&& row);
    void EraseSrcRow(int32_t s);
    void CommitRow(int32_t s, SrcRow&& fresh);
    void AddParentRef(winrt::hstring const& parentKey);
    void DropParentRef(winrt::hstring const& parentKey);
    void RebuildParentRefs();

    // --- Structure ---
    void Republish(UpdateReason reason);
    void BuildStructure();
    void ComputeExpansion();
    void ComputeVisible();
    void ApplyPresented(UpdateReason reason);
    winrt::IInspectable PresentedObjectAt(int32_t pos) const;
    winrt::hstring PresentedIdAt(int32_t pos) const;
    RowSignature SignatureAt(int32_t pos) const;
    bool ExpansionStateOf(int32_t pos) const;
    void UpdateGroupObjects(std::vector<winrt::IInspectable> const& bucketValues, std::vector<std::vector<int32_t>> const& bucketUnits);
    void UpdateGroupEntries();
    void ClearProjection();
    int CompareRows(int32_t a, int32_t b, std::vector<size_t> const& axes) const;
    int CompareKeys(SrcRow const& a, SrcRow const& b, std::vector<size_t> const& axes) const;
    void UpdateAxisSplits();
    void RaiseShapingChanged(bool reorderOnly);

    // --- Incremental structure ---
    enum class ReshapeKind
    {
        Added,
        Removed,
        Changed,
    };
    enum class ReshapeResult
    {
        // Nothing changed (a Changed row's cache is not committed): rebuild.
        NotLocal,
        Applied,
        // Applied and consistent, but the group order moved: rebuild.
        AppliedNeedsRebuild,
    };
    // Which order a sibling run is sorted in.
    enum class RunKind
    {
        TopLevel,
        Units,
        Children,
    };
    // Places, moves or removes source row `s` (and its subtree) in the row array. Changed commits
    // `fresh` into m_src unless the result is NotLocal.
    ReshapeResult TryReshape(int32_t s, ReshapeKind kind, SrcRow* fresh);
    bool TrySetExpandedInPlace(int32_t pos);
    bool RunLess(SrcRow const& a, int32_t aSrc, int32_t bSrc, RunKind kind) const;
    RunKind RunKindOf(int32_t parentPos) const;
    int32_t RunStart(int32_t parentPos) const noexcept { return parentPos < 0 ? 0 : parentPos + 1; }
    int32_t RunEnd(int32_t parentPos) const noexcept { return parentPos < 0 ? static_cast<int32_t>(m_rows.size()) : m_rows[parentPos].End; }
    // The first sibling in parentPos's run that source row s sorts before; RunEnd when none.
    int32_t FindSlot(int32_t parentPos, int32_t s) const;
    // Cuts [pos, End) out of the row array; the returned rows are relative to pos.
    std::vector<Row> Detach(int32_t pos);
    // Splices detached rows in at q as the last-placed child of parentPos, then presents them.
    void Attach(std::vector<Row>&& held, int32_t parentPos, int32_t q);
    void RenumberRun(int32_t parentPos);
    void SyncBucket(int32_t bucket);
    bool UnitLess(int32_t a, int32_t b) const;
    bool GroupOrderHolds(int32_t bucket) const;
    int32_t MinUnitOf(int32_t bucket) const;
    void VisibleScan(int32_t from, int32_t to, std::vector<int32_t>& out) const;
    bool IsShown(int32_t pos) const;
    // A parent identity that survives position shifts: a source index, -1 for the top level, or
    // -(bucket + 2) for a group header.
    int32_t ParentIdOf(int32_t parentPos) const;
    int32_t PosOfParentId(int32_t id) const;
    void RestampRun(int32_t parentId);
    void RestampRow(int32_t pos, size_t index);
    void InsertVisible(size_t at, std::vector<int32_t> const& shown);
    void EraseVisible(size_t at, size_t count);
    void RebuildEntryToPos();

    // A batch of presented edits. Recorded with the objects, so a batch can be flushed whatever
    // the row array's state; one ReplaceAll when it outgrows c_maxIncrementalEdits.
    struct PresentedOp
    {
        enum class Kind
        {
            Remove,
            Insert,
            Set,
        };
        Kind Op{ Kind::Set };
        uint32_t Index{ 0 };
        uint32_t Count{ 0 };
        std::vector<winrt::IInspectable> Objects;
    };
    void BeginEdit(UpdateReason reason);
    // The row array is consistent: apply the batch.
    void EndEdit();
    // The row array may not be consistent: apply the recorded ops, or leave presented out of sync.
    void AbortEdit();
    void RecordRemove(size_t at, size_t count);
    void RecordInsert(size_t at, std::vector<winrt::IInspectable>&& objects);
    void RecordSet(size_t at, winrt::IInspectable const& object);
    bool CountEdits(size_t count);

    // --- Live shaping ---
    void RefreshLiveSubscriptions();
    void OnLiveItemChanged(winrt::IInspectable const& item);
    bool RowChangedShape(SrcRow const& cached, SrcRow const& fresh) const;
    void MarkLiveDirty();
    void MarkLiveDirtyForItem();
    void QueueLiveItem(winrt::IInspectable const& item);
    bool TryRestoreLiveIncrementally();
    void ClearLivePending();
    void EnsureLiveRestore();
    void PostLiveRestore();
    void RestoreLive();
    void ClearLiveSubscriptions();

    winrt::IInspectable m_source{ nullptr };
    ShapingHelpers::CollectionAccessor m_accessor;
    IdentitySelector m_keySelector;

    ShapingHelpers::ShapingPipeline m_pipeline;
    // Active axes in precedence order, as evaluated into SrcRow::SortKeys.
    std::vector<ShapingHelpers::SortAxis> m_axes;
    // Indices into m_axes: all of them, and those declared before (pre) and after (post) GroupBy.
    std::vector<size_t> m_allAxes;
    std::vector<size_t> m_preAxes;
    std::vector<size_t> m_postAxes;
    ShapingHelpers::KeySelector m_groupKey{ nullptr };
    IdentitySelector m_groupIdentity{ nullptr };
    IdentitySelector m_parentSelector{ nullptr };

    std::vector<SrcRow> m_src;
    std::unordered_map<winrt::hstring, int32_t> m_keyToSrc;
    std::unordered_map<void const*, int32_t> m_itemToSrc;
    // Source index to pre-order position; -1 when the filter drops the row.
    std::vector<int32_t> m_srcToPos;

    std::vector<Row> m_rows;
    std::vector<GroupInfo> m_groups;
    std::unordered_map<winrt::hstring, int32_t> m_groupToBucket;
    struct CachedGroup
    {
        winrt::com_ptr<ShapedGroup> Group;
        winrt::IInspectable Entry{ nullptr };
        int32_t EntryCount{ -1 };
        bool EntryExpanded{ false };
    };
    // Group objects outlive a republish while their group lives, so a header keeps its object.
    std::unordered_map<winrt::hstring, CachedGroup> m_groupCache;
    std::unordered_map<void const*, int32_t> m_entryToPos;
    uint32_t m_includedDataRows{ 0 };

    std::vector<int32_t> m_visible;
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_presented;
    // Parallel to m_presented.
    std::vector<winrt::hstring> m_presentedIds;
    std::vector<RowSignature> m_presentedSignatures;

    ShapingHelpers::RowExpansionModel m_groupExpansion;
    ShapingHelpers::RowExpansionModel m_nodeExpansion;
    // Context rows (kept for a matching descendant) the user collapsed; reset when the filter changes.
    std::unordered_set<winrt::hstring> m_filterOverlayCollapsed;

    winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged::CollectionChanged_revoker m_collectionChangedRevoker{};
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>::VectorChanged_revoker m_vectorChangedRevoker{};
    winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector::VectorChanged_revoker m_bindableVectorChangedRevoker{};

    std::function<void()> m_projectionRebuilt;
    std::function<void(bool)> m_shapingChanged;

    winrt::Microsoft::UI::Dispatching::DispatcherQueue m_ownerQueue{ nullptr };
    DWORD m_ownerThreadId{};
    std::unique_ptr<LiveShapingTracker> m_liveShaping{ std::make_unique<LiveShapingTracker>() };
    bool m_liveShapingEnabled{ false };
    bool m_liveShapingDirty{ false };
    bool m_liveShapingHeld{ false };
    bool m_liveShapingRestorePosted{ false };
    std::shared_ptr<std::atomic<bool>> m_liveShapingChangeDropped{ std::make_shared<std::atomic<bool>>(false) };
    std::function<bool()> m_liveShapingHold;
    std::function<bool(winrt::IInspectable const&, bool)> m_liveItemChangedHook;
    std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> m_rowsReleasedHook;
    std::function<void()> m_liveItemReshape;

    // Rows whose cached parent key is the key: a row whose key is here would adopt them.
    std::unordered_map<winrt::hstring, int32_t> m_parentRefs;
    // m_rows matches m_src and its cached evaluation, so it can be edited in place.
    bool m_structureValid{ false };
    // m_presentedIds/m_presentedSignatures may not match m_presented; the next publish resets it.
    bool m_presentedOutOfSync{ false };

    // The open presented edit batch.
    UpdateReason m_editReason{ UpdateReason::Source };
    size_t m_editCount{ 0 };
    bool m_editReset{ false };
    std::vector<PresentedOp> m_editOps;

    // Items whose live change is applied by the next restore; m_liveFull re-reads every row instead.
    std::vector<winrt::IInspectable> m_livePending;
    bool m_liveFull{ false };

    // A failed source update left the cache out of step with the source; the next publish reloads.
    bool m_cacheInvalid{ false };
    // Bumped when m_src's rows move; the structure is valid only when built from the current rows.
    uint64_t m_srcGeneration{ 0 };
    uint64_t m_rowsGeneration{ 0 };
};
