// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/ShapedTree.h"
#include "Source/SourceTable.h"
#include "Spec/PathEvaluator.h"
#include "Spec/ShapeDelegates.h"
#include "Spec/ShapeSpec.h"

// Source tree + applied spec -> shaped tree (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Shaping engine".
//
// Owns the applied ShapeSpec, the ShapedTree (and through it the shaped root and every GroupNode),
// and writes the engine-owned ShapeState slot on each record. Reads SourceTable (TopLevel(),
// SourceRecord::Parent / Children / Depth); never reads the app collection. Makes every structural
// change through ShapedTree (Link / Unlink, BeginReset / EndReset), which keeps the counts and
// raises the row events (P8, P10).
//
// Compute, then commit (decided): every entry point first runs all app code (filter, keys, group
// key, comparer-driven positions) without writing anything, then writes Shape, groups and counts
// through the tree. The write phase calls no app code of its own and cannot throw; the tree's
// listener may run handlers, and catches (see ShapingManager). A throw from the compute phase
// propagates with nothing written and no event raised.
//
// Readable between tree calls (P8): a listener may read the tree inside an event, so the write
// phase builds a new group chain or subtree while it is detached (plain writes) and links its
// topmost node once; removal mirrors it. A commit either resets or links, never both: Reset vs
// surgical is decided before the write phase.
//
// Routing stays in ShapingManager: the engine never sees a range event (k > c_bulkThreshold); the
// manager calls Reapply for those.
//
// Reference holding: GroupNode::Key and ShapeState::SortKeys are strong references (Header-Changes
// drift #73), not tracker_refs, so the engine needs no tracker owner.
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
        SourceTable const& source,
        ShapeDelegates& delegates,
        PathEvaluator& paths);

    ShapingEngine(ShapingEngine const&) = delete;
    ShapingEngine& operator=(ShapingEngine const&) = delete;

    // ---- State (read) -----------------------------------------------------------------------

    // The spec the shaped tree reflects. Diff(Applied(), desired) picks the work; column sort
    // indicators read it. Unchanged when an apply throws.
    ShapeSpec const& Applied() const noexcept;

    // The shaped tree: root, counts, reads and expansion (P10). ShapingManager reads rows and calls
    // the expansion verbs through it, never through the engine.
    ShapedTree& Tree() noexcept;
    ShapedTree const& Tree() const noexcept;

    // Every live group by Identity (m_groupsByIdentity). ShapingManager resolves a group row key
    // through it (V3); Rebuild hands it to Tree().EndReset, which prunes group expansion (F4).
    GroupMap const& GroupsByIdentity() const noexcept;

    // The node is in the shaped tree: a group (emptied groups are pruned, E9), or a record for which
    // InShapedParent holds at every record level up to its top-level record. O(depth). A record that
    // fails the filter and is no ancestor of a match is not. ShapingManager::Reveal checks it
    // before expanding anything (#20).
    static bool InShapedTree(ShapedNode* node) noexcept;

    // ---- Entry points -----------------------------------------------------------------------
    // Each is one unit of work. Called inside a commit scope opened by ShapingManager.

    // A spec verb: Rebuild(desired, delta) (decided, E6): only the parts `delta`
    // (Diff(Applied(), desired)) names re-run app code; the others reuse ShapeState and the
    // groups. Always one reset (a stable Reset), never surgical edits. On success
    // Applied() == desired.
    void Apply(ShapeSpec const& desired, ShapeDelta delta);

    // Full rebuild from SourceTable with `desired`: Rebuild(desired, ShapeDelta::All). Evaluates
    // every record into a staging buffer, builds new ShapedChildren and groups aside, then moves
    // them in under the same root between Tree().BeginReset and EndReset. Used for a SourceTable
    // Reset, a range event and a failed-apply retry.
    void Reapply(ShapeSpec const& desired);

    // Reapply for a source change that has already happened (a SourceTable Reset, or a change the
    // engine cannot apply surgically). Departed records always leave the tree: when the compute
    // phase throws, UnlinkOnly rebuilds the tree from the current records without app code (one
    // Reset), then the exception is rethrown. `arrivals` are the change's new records
    // (SourceChange::Added), hidden by that fallback.
    void ReapplyForSource(ShapeSpec const& desired, std::span<SourceRecord* const> arrivals);

    // True after UnlinkOnly: cached filter results, keys and placements may be stale, so the next
    // Rebuild re-reads everything (ShapeDelta::All) even when Diff(Applied(), desired) is empty.
    bool IsStale() const noexcept;

    // One source notification with k <= c_bulkThreshold items (any subtree size): places, removes,
    // moves or replaces the record and its subtree with Tree().Unlink / Link, under any applied
    // spec. While grouped it also creates missing groups and prunes emptied ones (E9; see
    // StageGroupChain and PruneEmptyGroups). A reparenting Move under a non-empty spec, and a stale
    // engine, rebuild instead (drift #35). A compute failure falls back to UnlinkOnly and rethrows
    // (drift #33).
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

    // Live shaping for more than ShapingManager::c_liveBulkThreshold coalesced records (decided,
    // E7 Y2): Rebuild(Applied(), Filter | GroupKey | Sort, changes). The re-reads run app code only
    // for `changes` (each entry re-reads every part, unrouted: drift #43); every other record reuses its cached
    // ShapeState. The re-sorts then run over cached keys. One reset (a stable Reset the owner
    // re-anchors). App code runs O(k) times, not O(n). Every entry's record is live (the manager
    // erases removed records).
    void ApplyItemsChanged(std::span<LiveChange const> changes);

    // ---- Lifetime ---------------------------------------------------------------------------

    // Calls Tree().Clear() (unlinks the root's Children), then drops every GroupNode (and its
    // Key) by clearing m_groupsByIdentity and m_retired, every record's SortKeys and
    // ShapedChildren, and resets Applied() to the empty spec. Called by ShapingManager::Clear()
    // before SourceTable::Clear(). Idempotent.
    void Clear();

    // Destroys the groups pruned since the last call (m_retired). ShapingManager calls it after
    // OnShapingApplied, and from Clear(). Until then a pruned group stays alive, so a GroupNode*
    // that a handler took from an earlier read stays valid for the whole commit (P8).
    void ReleaseRetired() noexcept;

private:
    // Staging types (E4), defined at the end of the class.
    struct StagedRecord;
    struct StagedEdit;
    struct StagedLeave;
    struct StagedReapply;

    // Rebuild runs these parts in order (decided, E6), driven by a ShapeDelta (see its comment for
    // which flags imply which re-sort):
    //   1. Filter      (Filter)                    predicates -> PassesFilter; ancestors of matches
    //                                              follow from ShapedChildren; a full pass (`only`
    //                                              empty) also clears OverlayCollapsed (F5)
    //   2. Group keys  (GroupKey)                  key per top-level record and level -> segments
    //   3. Sort keys   (Sort)                      keys of changed levels; unchanged levels keep theirs
    //   4. Sort rows   (Sort | GroupKey | Filter)  comparer over rows in each parent / bottom group
    //   5. Sort groups (GroupOrder | GroupKey | Filter) group comparer over each group's subgroups
    // A clear part reuses the cached result. Structural work (bucketing, building ShapedChildren
    // and group Children) always runs from the results. Records that become visible with no valid
    // cached keys (SortKeysValid clear, or never bucketed) are evaluated whatever the delta.

    // True when the record belongs in its shaped parent's list (a parent's ShapedChildren, or a
    // group's Children for a top-level record): it passes the filter, or it is an ancestor of a
    // match (ChildrenShaped set and ShapedChildren non-empty). Derived, never stored (decided, E8).
    // Surgical edits keep it current with an O(depth) walk: inserting into an empty list of a
    // failing parent inserts that parent into its own shaped parent, and so on up; emptying the
    // list removes it the same way. A full rebuild builds the lists bottom-up, so it holds.
    // Tree calls (decided, F1): one per edit, at the topmost node that enters or leaves. Insert:
    // build the whole chain detached, then one Tree().Link of its topmost node. Remove: one
    // Tree().Unlink of the topmost node that leaves, then take the chain apart (plain writes).
    static bool InShapedParent(SourceRecord const& record) noexcept;

    // Surgical source edits (Milestone 5). Each edit computes everything first (every app call,
    // every allocation) into a StagedEdit / StagedLeave, then a noexcept write phase commits it
    // with one Tree().Link and / or one Tree().Unlink of the topmost node that enters or leaves
    // (F1). ApplySourceChange catches a compute failure and falls back to UnlinkOnly (drift #33).

    // ApplySourceChange under the empty spec for an Add, Remove or Move (including a reparent):
    // the source position is the shaped position, so it links or unlinks that one node (drift #20).
    // Only the root list's reservation can throw, before any write.
    void ApplyUnshapedSourceChange(SourceChange const& change);

    // Insert compute, part 1: stages `added` (a subtree in pre-order; filter, sort keys, shaped
    // children, flags) into edit.Staged / edit.Descendants. When the root enters the shaped tree it
    // also stages the E8 chain of `parent`'s ancestors that enter with it (edit.Chain, bottom-up),
    // sets edit.Parent to the shaped parent the topmost staged record links under and, when that
    // record is top-level while grouped, stages its group chain (StageGroupChain). edit.Parent
    // stays null when the root does not enter. Returns the deepest existing group on the topmost
    // record's group chain (the root while ungrouped), or null when nothing enters or the topmost
    // record is nested.
    GroupNode* StageInsert(StagedEdit& edit, std::span<SourceRecord* const> added, SourceRecord* parent);

    // Insert compute, part 2: places the topmost node that enters (PlacementIndex /
    // GroupPlacementIndex) into edit.ShapedIndex, as if `skip` (a node about to leave the same
    // list, or null) were not in edit.Parent's list, and reserves the slot the write phase takes.
    // `sourceIndex` is the root's SourceChange::NewIndex; a chain ancestor finds its own.
    void StagePlacement(StagedEdit& edit, std::optional<uint32_t> sourceIndex, ShapedNode const* skip);

    // Insert write phase: commits every staged record, links the detached chain (new groups, chain
    // ancestors) under its topmost node, inserts the new map nodes, adds 1 to ItemCount up the
    // existing group chain, then one Tree().Link at edit.ShapedIndex.
    void CommitInsert(StagedEdit& edit) noexcept;

    // Leave compute (runs no app code): finds the topmost node that leaves with `record` (the record,
    // the E8 chain of its failing ancestors whose only shaped child it is, then the highest group
    // with no other member, never one on `keep`'s chain) and its place. `oldParent` and `oldIndex`
    // are the record's place before the source change (a removed record's Parent is already null).
    // Reserves m_retired for the pruned groups.
    StagedLeave StageLeave(SourceRecord* record, SourceRecord* oldParent, uint32_t oldIndex, GroupNode const* keep);

    // Leave write phase: subtracts 1 from ItemCount up the old group chain, one Tree().Unlink of
    // the topmost node that leaves, takes the leaving chain apart and prunes the emptied groups.
    void CommitLeave(StagedLeave& leave) noexcept;

    // Group lifecycle in surgical edits (decided, E9). Compute: walks the identities built from
    // `keys` (one key object per level) down from the root through m_groupsByIdentity, allocates
    // the missing tail of the chain into edit.NewGroupEntries (top-down; keys tracked, each new
    // group's Parent set, every vector's capacity reserved, m_groupsByIdentity reserved for the new
    // entries), sets the topmost staged record's Shape.Group to the bottom-level group and
    // edit.Parent to the deepest existing group, so the write phase cannot throw. Returns that
    // deepest existing group. Placement of a new group among its siblings (GroupPlacementIndex):
    // by key comparison for Ascending / Descending; appended for None (sticky: existing groups
    // never move on a surgical edit, the next rebuild restores exact first-appearance order).
    GroupNode* StageGroupChain(
        StagedEdit& edit,
        std::vector<winrt::Windows::Foundation::IInspectable> const& keys);

    // Surgical placement (decided, E5 option B: no stored source order). Compute phase: returns the
    // index `record` takes in `parent`'s shaped list, taken without `skip` (the record itself for a
    // move, a leaving node for a replace), so the result equals what a rebuild's stable sort gives:
    // equal keys end in source order. `shape` holds the record's (staged) flags and sort keys.
    //   - Sort active: binary-search the run of siblings whose keys tie with the staged keys
    //     (the only app code, through Comparer levels). Empty run: its start. Otherwise set
    //     ShapeFlags::PlacementMark on the run, walk the record's source siblings backwards
    //     from its source position to the first marked one and return the index after it (the
    //     run start if none), then clear the marks.
    //   - Filter or GroupBy only (shaped order is source order): walk back to the first source
    //     sibling in the same shaped list (InShapedParent, and the same ShapedParent), then return
    //     its index in the list plus one (0 if none).
    //   - Neither (a source-owned list, ChildrenShaped clear): the source position.
    // `sourceIndex` is SourceChange::NewIndex; empty to find the record in its source sibling list.
    // Reads only, besides the marks. O(log n + t + walk), worst case O(n).
    uint32_t PlacementIndex(
        SourceRecord* record,
        ShapeState const& shape,
        ShapedNode* parent,
        ShapedNode const* skip,
        std::optional<uint32_t> sourceIndex);

    // The index new group `group` takes among `parent`'s Children (taken without `skip`): upper
    // bound by key for Ascending / Descending, appended for None.
    uint32_t GroupPlacementIndex(GroupNode const* group, ShapedNode* parent, ShapedNode const* skip) const;

    // Row order under the applied sort: < 0 when (a, aShape) sorts before (b, bShape). Keys come
    // from the shapes' SortKeys; Comparer levels call the app's comparer (may throw).
    int32_t CompareRows(
        winrt::Windows::Foundation::IInspectable const& a,
        ShapeState const& aShape,
        winrt::Windows::Foundation::IInspectable const& b,
        ShapeState const& bShape) const;

    // Write phase, after Tree().Unlink of the topmost leaving node: extracts every group from
    // `bottom` up to and including `top` (all emptied; null `top` prunes nothing) from
    // m_groupsByIdentity into m_retired (capacity reserved by StageLeave). Unlinking `top` already
    // erased their expansion entries (F4). Stops at the shaped root.
    void PruneEmptyGroups(GroupNode* bottom, GroupNode* top) noexcept;

    // The one rebuild routine behind Apply, Reapply and ApplyItemsChanged: compute into a
    // StagedReapply, then the noexcept write phase calls Tree().BeginReset, swaps it in under the
    // root and calls Tree().EndReset.
    // Closes with Tree().EndReset(levels, m_groupsByIdentity, newGroupSelector), where
    // newGroupSelector is true when a group level present in both Applied() and `desired` has a
    // different Key (a level only appended or removed keeps the other identities' meaning).
    // `only` empty: the re-reads `delta` names (Filter, GroupKey, Sort) cover every record.
    // `only` non-empty (live shaping, E7): they cover only those records, routed by PropertyName,
    // re-reading every sort level; the re-sorts still run over all cached keys.
    void Rebuild(ShapeSpec const& desired, ShapeDelta delta, std::span<LiveChange const> only = {});

    // Rebuild's write phase, shared with UnlinkOnly: Tree().BeginReset, moves `staged` into every
    // record in `records` (breadth-first; the first topCount are top-level) and under the root,
    // swaps the group map, sets Applied() to `applied`, Tree().EndReset. Records keep the `kept`
    // bits of their flags; keysRead marks the records whose SortKeys were read into `staged`.
    // Clears m_stale.
    void CommitStaged(
        std::vector<SourceRecord*> const& records,
        uint32_t topCount,
        StagedReapply& staged,
        std::vector<uint8_t> const& keysRead,
        ShapeSpec&& applied,
        uint16_t levels,
        bool newGroupSelector,
        ShapeFlags kept) noexcept;

    // Fallback after a failed compute in ReapplyForSource. Runs no app code: rebuilds the tree
    // for the current records with the applied spec from cached state only. Records keep their
    // cached PassesFilter and their previous relative order; records that left the source are
    // dropped; `arrivals`, and records with no cached place (a top-level record with no group
    // while grouped), are hidden; emptied groups are dropped. One Reset. Sets m_stale.
    // noexcept: an allocation failure fails fast rather than leaving freed records linked.
    void     UnlinkOnly(std::span<SourceRecord* const> arrivals) noexcept;

    SourceTable const& m_source;
    ShapeDelegates& m_delegates;
    PathEvaluator& m_paths;

    ShapeSpec m_applied;

    // Set by UnlinkOnly, cleared by CommitStaged (see IsStale).
    bool m_stale{ false };

    // Owns the shaped root (no other group). Stable for the engine's lifetime.
    ShapedTree m_tree;

    // Every live group by GroupNode::Identity (decided, E3), and their owner (E2). A record's
    // group at each level is found by extending the parent's Identity with the key's
    // ValueKey::ToObjectLookupKey segment (value types by typed value, reference types by IUnknown
    // identity, null as "null:"), the same string a new group gets as its Identity. Also serves
    // Identity -> node for expansion restore and reveal. Entries are added when a group is created
    // and erased in the same write phase that drops it; a rebuild builds a new map aside and swaps
    // it in (the old groups are destroyed with the old map when the commit returns). Empty while
    // ungrouped. The shaped root is not in it.
    GroupMap m_groupsByIdentity;

    // Groups pruned by surgical edits since the last ReleaseRetired. Unlinked from the tree and the
    // map, so no read reaches them; kept alive until the commit ends because a listener may hold a
    // node pointer from an earlier read (P8). A rebuild's old groups are not retired (P5): they are
    // destroyed after EndReset, when the owner has seen the Reset.
    std::vector<std::unique_ptr<GroupNode>> m_retired;

    // Staging (decided, E4): compute results live in a transient object local to one entry point
    // (a stack local, never an engine member), never in a record's own ShapeState, and are moved in
    // by the noexcept write phase. Freed when the call returns. The write phase never allocates:
    // the staged state is a ShapeState (a commit is vector moves, pointer stores and flag bits),
    // and compute reserves every container the write phase inserts into (m_groupsByIdentity for
    // NewGroupEntries, a group's Children, a parent's ShapedChildren), so no insert can rehash or
    // grow.
    //   - StagedRecord: one record's new shape. The only per-record type a rebuild stages.
    //   - StagedEdit: one surgical edit (ApplySourceChange insert / replace / move,
    //     ApplyItemChanged): a StagedRecord plus the edit-only fields.
    //   - StagedLeave: one record leaving (ApplySourceChange remove / replace); no app code runs.
    //   - StagedReapply: a whole rebuild (Apply, Reapply, ApplyItemsChanged).

    // One record's compute results.
    struct StagedRecord
    {
        SourceRecord* Record{ nullptr };
        // The record's new shape state, same type as SourceRecord::Shape:
        //   - Flags: only the compute bits (PassesFilter, ChildrenShaped, SortKeysValid).
        //   - SortKeys: one slot per level (empty for Comparer levels); empty unless a sort is
        //     active and the delta re-reads it.
        //   - ShapedChildren: the new filtered and sorted children; empty for a leaf (no
        //     allocation). Built bottom-up by a post-order pass over the pre-order list.
        //   - Group: top-level records while grouped, the bottom-level group resolved during
        //     compute (an existing group, the bottom new group in StagedEdit::NewGroupEntries, or a
        //     group in StagedReapply::GroupsByIdentity); null otherwise. The key segments and key
        //     objects used to find or create it are compute-local and not staged.
        // Committed field by field, never by whole assignment (`Shape = std::move(...)`):
        //   - Flags: compute bits from here; Expanded and PlacementMark kept from the record;
        //     OverlayCollapsed kept unless the delta runs the full Filter stage (then cleared, F5).
        //   - SortKeys: moved only when the delta re-reads them; otherwise the record's are kept.
        //   - ShapedChildren and Group: moved and stored.
        ShapeState Shape;
    };

    // One surgical edit. One per edit; the write phase makes one Tree().Link at the topmost node
    // that enters (F1).
    struct StagedEdit
    {
        // The inserted, replaced, moved or changed record.
        StagedRecord Staged;
        // Subtree insert only: the inserted record's descendants in pre-order (they enter inside
        // its subtree, so they need no group fields and no placement). Empty otherwise.
        std::vector<StagedRecord> Descendants;
        // E8: the inserted record's ancestors that enter the shaped tree with it (failing the
        // filter, no shaped child before), bottom-up; each one's staged ShapedChildren holds only
        // the node below it. The back is the topmost record that enters. Empty otherwise.
        std::vector<StagedRecord> Chain;
        // The shaped parent the topmost node that enters links under (a record, a group or the
        // root). Null when nothing enters (the record stays out of the shaped tree).
        ShapedNode* Parent{ nullptr };
        // Grouped insert or group-key change (E9): the owning map nodes of the missing tail of the
        // record's group chain, top-down (front is the topmost new group; its Parent is the
        // existing group it links under, the root for a new top-level group). Inserted into
        // m_groupsByIdentity by the write phase. Empty when every level already exists.
        std::vector<GroupMap::node_type> NewGroupEntries;
        // From StagePlacement (E5): the index of the topmost node that enters, in edit.Parent's
        // list. The topmost new group among its Parent's Children when NewGroupEntries is non-empty
        // (the record then goes at 0 in its new bottom-level group); otherwise the topmost record,
        // in its shaped parent list taken without the node that leaves it.
        uint32_t ShapedIndex{ 0 };
    };

    // One record leaving the shaped tree (a remove, or the old record of a replace). Computed
    // without app code by StageLeave, committed by CommitLeave.
    struct StagedLeave
    {
        // The leaving record.
        SourceRecord* Record{ nullptr };
        // The topmost node that leaves (the record, a chain ancestor or a group) and its place in
        // its shaped parent list. Top is null when the record is not in the shaped tree.
        ShapedNode* Top{ nullptr };
        ShapedNode* Parent{ nullptr };
        uint32_t Index{ 0 };
        // E8: the record's ancestors that leave with it, bottom-up.
        std::vector<SourceRecord*> Chain;
        // The leaving top-level record's bottom-level group while grouped (null otherwise), and the
        // topmost group pruned with it (null when none empties).
        GroupNode* Group{ nullptr };
        GroupNode* TopGroup{ nullptr };
    };

    // Everything Reapply builds aside: per-record results in pre-order (each carrying its new
    // ShapedChildren), the new root Children and the new (owning) group map. Commit calls
    // Tree().BeginReset (the owner captures anchors in OnResetting), commits every record in one
    // pass, swaps the root Children and the map, calls Tree().EndReset, then destroys the old
    // groups (the old map ends up in here) when the commit returns.
    struct StagedReapply
    {
        std::vector<StagedRecord> Records;
        std::vector<ShapedNode*> RootChildren;
        GroupMap GroupsByIdentity;
    };
};
