// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/ShapedEdit.h"
#include "Engine/ShapedTree.h"
#include "Source/SourceTable.h"
#include "Spec/PathEvaluator.h"
#include "Spec/ShapeDelegates.h"
#include "Spec/ShapeSpec.h"

// Source tree + applied spec -> shaped tree (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Shaping engine".
//
// Owns the applied ShapeSpec, the stable shaped root and every GroupNode, and writes the
// engine-owned ShapeState slot on each record. Reads SourceTable (TopLevel(), SourceRecord::Parent /
// Children / Depth); never reads the app collection. Reports structural changes to the flattener
// through IShapedEditSink only.
//
// Compute, then commit (decided): every entry point first runs all app code (filter, keys, group
// key, comparer-driven positions) without writing anything, then writes Shape, groups and counts
// and calls the sink. The write phase calls no app code and cannot throw. A throw from the compute
// phase propagates with nothing written and no sink call.
//
// Commit bracketing (decided: the caller brackets): ShapingManager calls Flattener::BeginCommit
// before an entry point and EndCommit after it, through a scope that calls Flattener::AbortCommit
// if an exception escapes. The engine never sees BeginCommit / EndCommit / FlatDelta.
//
// Routing stays in ShapingManager: the engine never sees a range event (k > c_bulkThreshold); the
// manager calls Reapply for those.
//
// Reference tracking: GroupNode::Key and ShapeState::SortKeys are tracker_refs bound to
// trackerOwner (the owning TableViewSource2). Clear() must run while that owner is alive.
//
// Threading: UI-thread affine.

// One coalesced live-shaping entry (E7): a record and the property that changed. An empty
// PropertyName means "re-read everything" (the record raised PropertyChanged with an empty name,
// or with several different names in the same dispatcher turn).
struct LiveChange
{
    SourceRecord* Record{ nullptr };
    winrt::hstring PropertyName;
};

class ShapingEngine final
{
public:
    ShapingEngine(
        ITrackerHandleManager const* trackerOwner,
        SourceTable const& source,
        ShapeDelegates& delegates,
        PathEvaluator& paths,
        IShapedEditSink& sink);

    ShapingEngine(ShapingEngine const&) = delete;
    ShapingEngine& operator=(ShapingEngine const&) = delete;

    // ---- State (read) -----------------------------------------------------------------------

    // The spec the shaped tree reflects. Diff(Applied(), desired) picks the work; column sort
    // indicators read it. Unchanged when an apply throws.
    ShapeSpec const& Applied() const noexcept;

    // The stable shaped root: same object for the engine's lifetime (see "Root lifetime").
    GroupNode& Root() noexcept;

    // Every live group by Identity (m_groupsByIdentity). ShapingManager prunes GroupExpansion
    // against it after each rebuild (F4).
    std::unordered_map<winrt::hstring, GroupNode*> const& GroupsByIdentity() const noexcept;

    // ---- Entry points -----------------------------------------------------------------------
    // Each is one unit of work. Called inside a flattener commit opened by ShapingManager.

    // A spec verb: Rebuild(desired, StagesFor(delta)) (decided, E6): only the stages `delta`
    // (Diff(Applied(), desired)) needs re-run app code; the others reuse ShapeState and the
    // groups. Always one OnRebuilt (a stable Reset), never surgical edits. On success
    // Applied() == desired.
    void Apply(ShapeSpec const& desired, ShapeDelta delta);

    // Full rebuild from SourceTable with `desired`: Rebuild(desired, RebuildStage::All). Evaluates
    // every record into a staging buffer, builds new ShapedChildren and groups aside, then moves
    // them in under the same root and calls OnRebuilt. Used for a SourceTable Reset, a range event, a failed-apply retry and
    // ItemChanged(nullptr) recovery.
    void Reapply(ShapeSpec const& desired);

    // One source notification with k <= c_bulkThreshold items (any subtree size): places, removes,
    // moves or replaces the record and its subtree with OnRemoving / OnInserted. While grouped it
    // also creates missing groups and prunes emptied ones (E9; see StageGroupChain and
    // PruneEmptyGroups).
    void ApplySourceChange(SourceChange const& change);

    // Live shaping for one record (k == 1 after coalescing, E7). Routing (decided: re-evaluate and
    // diff): re-reads what `propertyName` may affect and diffs against the cached ShapeState:
    //   - filter: a delegate predicate is opaque, so it is always re-run;
    //   - a path sort key or group level: re-read only when the path's first segment equals
    //     `propertyName` (a nested object's change is not reported by the row item); delegate
    //     keys are always re-read;
    //   - an empty `propertyName` re-reads everything.
    // No change in PassesFilter, SortKeys or group Identity: nothing is emitted (the row's
    // bindings update on their own). Otherwise the record is removed and re-inserted surgically:
    // a filter flip walks up while a ShapedChildren list changes between empty and non-empty
    // under an ancestor failing the filter (E8); a group change may prune and create groups (E9).
    void ApplyItemChanged(SourceRecord* record, winrt::hstring const& propertyName);

    // Live shaping for k > 1 coalesced records (decided, E7 Y2): Rebuild(Applied(), the re-read
    // stages | SortRows | SortGroups, changes). The re-read stages (Filter, GroupKeys, SortKeys,
    // routed per entry as in ApplyItemChanged) run app code only for `changes`; every other
    // record reuses its cached ShapeState. The structural stages then run over cached keys. One
    // OnRebuilt (a stable Reset the owner re-anchors), matching c_bulkThreshold. App code runs
    // O(k) times, not O(n). Every entry's record is live (the manager erases removed records).
    void ApplyItemsChanged(std::span<LiveChange const> changes);

    // ---- Lifetime ---------------------------------------------------------------------------

    // Drops every GroupNode (and its tracked Key), every record's SortKeys and ShapedChildren,
    // empties the root and resets Applied() to the empty spec. Called by ShapingManager::Clear()
    // after Flattener::Clear() and before SourceTable::Clear(). Idempotent.
    void Clear();

    // Destroys the groups pruned since the last call (m_retired). ShapingManager calls it after
    // Publish, and from Clear(). Until then a pruned group stays alive, so the visible list can
    // still hand out its ref between the events of the delta that removes it (V4).
    void ReleaseRetired() noexcept;

private:
    // Stages of the one rebuild routine, in order (decided, E6). A set stage re-runs its app code
    // for every record it covers; a clear stage reuses the cached result. Structural work
    // (bucketing, building ShapedChildren and group Children) always runs from the stage results.
    enum class RebuildStage : uint8_t
    {
        None = 0x0,
        Filter = 0x1,       // predicates -> PassesFilter (ancestors of matches follow from ShapedChildren);
                            // a full pass (`only` empty) also clears OverlayCollapsed (F5)
        GroupKeys = 0x2,    // group key per top-level record and level -> segments, bucketing
        SortKeys = 0x4,     // keys of changed sort levels (unchanged levels keep theirs)
        SortRows = 0x8,     // comparer over rows within each parent / bottom-level group
        SortGroups = 0x10,  // group comparer over each group's subgroups
        All = 0x1F,
    };
    friend constexpr RebuildStage operator|(RebuildStage a, RebuildStage b) noexcept
    {
        return static_cast<RebuildStage>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    // Sort -> SortKeys | SortRows; GroupOrder -> SortGroups; GroupKey -> GroupKeys | SortRows |
    // SortGroups; Filter -> Filter | SortRows | SortGroups (new members and new groups must be
    // ordered; the comparer runs over cached keys). Flags combine. Records that become visible with no valid cached keys
    // (SortKeysValid clear, or never bucketed) are evaluated regardless of the stages.
    static RebuildStage StagesFor(ShapeDelta delta) noexcept;

    // True when the record belongs in its shaped parent's list (a parent's ShapedChildren, or a
    // group's Children for a top-level record): it passes the filter, or it is an ancestor of a
    // match (ChildrenShaped set and ShapedChildren non-empty). Derived, never stored (decided, E8).
    // Surgical edits keep it current with an O(depth) walk: inserting into an empty list of a
    // failing parent inserts that parent into its own shaped parent, and so on up; emptying the
    // list removes it the same way. A full rebuild builds the lists bottom-up, so it holds.
    // Sink (decided, F1): one call per edit, at the topmost node that enters or leaves. Insert:
    // link the whole chain, then one OnInserted for the topmost newly shown node. Remove: one
    // OnRemoving for the topmost node that leaves (old tree intact), then unlink the chain.
    static bool InShapedParent(SourceRecord const& record) noexcept;

    // Group lifecycle in surgical edits (decided, E9). Compute: walks the staged GroupSegments down
    // from m_root through m_groupsByIdentity and allocates the missing tail of the chain (keys
    // tracked, map nodes and every vector's capacity reserved) into the StagedRecord, so the write
    // phase cannot throw. Write: links NewGroupChain under NewGroupParent (one OnInserted for its
    // subtree), inserts the map nodes, and adds 1 to ItemCount up the chain. Placement among
    // siblings: by key comparison for Ascending / Descending; appended for None (sticky: existing
    // groups never move on a surgical edit, the next rebuild restores exact first-appearance order).
    void StageGroupChain(StagedRecord& staged);

    // Write phase, when a top-level record leaves bottom-level group `group`: finds the topmost
    // node that leaves (the highest ancestor group with no other member, else the record) and
    // calls OnRemoving once for it while the old tree is intact (F1). Then unlinks the record,
    // subtracts 1 from ItemCount up the chain, and erases each emptied group from
    // m_groupsByIdentity and from its parent's Children and Subgroups (moving it to m_retired, V4). Stops at
    // the shaped root. The expansion entry of a dropped
    // group is erased by the flattener (prune rules).
    void PruneEmptyGroups(GroupNode* group) noexcept;

    // The one rebuild routine behind Apply, Reapply and ApplyItemsChanged: compute into a
    // StagedReapply, then the noexcept write phase swaps it in under m_root and calls OnRebuilt.
    // `only` empty: the set re-read stages (Filter, GroupKeys, SortKeys) cover every record.
    // `only` non-empty (live shaping, E7): they cover only those records, routed by PropertyName;
    // SortRows and SortGroups still run over all cached keys.
    void Rebuild(ShapeSpec const& desired, RebuildStage stages, std::span<LiveChange const> only = {});

    ITrackerHandleManager const* m_trackerOwner{ nullptr };
    SourceTable const& m_source;
    ShapeDelegates& m_delegates;
    PathEvaluator& m_paths;
    IShapedEditSink& m_sink;

    ShapeSpec m_applied;

    // Stable for the engine's lifetime (decided). A rebuild swaps Children and Subgroups under it.
    // Owns the top-level groups through Subgroups, and so (transitively) every GroupNode (E2).
    GroupNode m_root;

    // Every live group by GroupNode::Identity (decided, E3). Non-owning: Subgroups owns. A record's
    // group at each level is found by extending the parent's Identity with the key's
    // ValueKey::ToObjectLookupKey segment (value types by typed value, reference types by IUnknown
    // identity, null as "null:"), the same string a new group gets as its Identity. Also serves
    // Identity -> node for expansion restore and reveal. Entries are added when a group is created
    // and erased in the same write phase that drops it; a rebuild builds a new map aside and swaps
    // it in with the new groups. Empty while ungrouped. The shaped root is not in it.
    std::unordered_map<winrt::hstring, GroupNode*> m_groupsByIdentity;

    // Groups pruned by surgical edits since the last ReleaseRetired (decided, V4). Unlinked from
    // the tree and the map, so the engine never reaches them; kept alive only because m_rows may
    // still hold their refs until the delta that removes them is published. A rebuild needs no
    // retiring: it rewrites m_rows before destroying the old set.
    std::vector<std::unique_ptr<GroupNode>> m_retired;

    // Staging (decided, E4): compute results live in a transient object local to one entry point,
    // never in ShapeState, and are moved in by the noexcept write phase. Freed when the call returns.

    // One record's compute results. Surgical paths stage 1..k of these.
    struct StagedRecord
    {
        SourceRecord* Record{ nullptr };
        bool PassesFilter{ false };
        // Empty unless a sort is active; moved into ShapeState::SortKeys on commit.
        std::vector<tracker_ref<winrt::Windows::Foundation::IInspectable>> SortKeys;
        // Top-level records while grouped: one ValueKey::ToObjectLookupKey segment per level, and the
        // key objects (for a GroupNode::Key on a new group).
        std::vector<winrt::hstring> GroupSegments;
        std::vector<winrt::Windows::Foundation::IInspectable> GroupKeys;
        // Surgical insert while grouped (E9): the missing tail of the record's group chain,
        // allocated during compute. NewGroupChain is the topmost new group (it owns the rest
        // through Subgroups); NewGroupParent is the existing group it is linked under (m_root for a
        // new top-level group). NewGroupEntries are the map nodes for the new groups. All empty
        // when every level already exists.
        std::unique_ptr<GroupNode> NewGroupChain;
        GroupNode* NewGroupParent{ nullptr };
        std::vector<std::unordered_map<winrt::hstring, GroupNode*>::node_type> NewGroupEntries;
    };

    // Everything Reapply builds aside: per-record results in pre-order, every parent's new
    // ShapedChildren, the new root Children / Subgroups and the new group map. Commit swaps these
    // in under m_root, calls OnRebuilt, then destroys the old groups (which end up in here).
    struct StagedReapply
    {
        std::vector<StagedRecord> Records;
        std::vector<std::pair<SourceRecord*, std::vector<SourceRecord*>>> ShapedChildren;
        std::vector<ShapedRef> RootChildren;
        std::vector<std::unique_ptr<GroupNode>> RootSubgroups;
        std::unordered_map<winrt::hstring, GroupNode*> GroupsByIdentity;
    };
};
