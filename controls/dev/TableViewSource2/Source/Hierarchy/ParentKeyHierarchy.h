// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <exception>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "Source/SourceRecord.h"
#include "SourceHierarchy.h"

// ParentBy strategy; see Source/Hierarchy/SourceHierarchy.h.

// ParentBy(keySelector, parentKeySelector). Every item is in the root collection; the tree is an
// index over Records(). Key rules follow v1 (Hierarchical-Data-Rows-Parent-Key-Design.md section 3):
// value keys compare by value, object keys by reference; a null/empty parent key or an orphan makes
// a top-level record. Siblings follow source order.
//
// Validation (decided, B1; spec Misbehaving records):
//   - Verb (and a verb parked under DeferRefresh, prepared at the flush): strict. A null/empty key,
//     a duplicate key, a self-parent or a cycle throws before any write.
//   - Refresh() of the committed strategy: tolerant, since the source has already changed. A
//     null/empty key excludes the record; a duplicate key stays with the record holding it (else
//     the first in source order) and the other is excluded; a self-parent is top-level; a cycle
//     is broken at a record forced to the top level before, else at the earliest in root order.
//   - Live (an edit, or a re-key under live shaping): the change is applied, then the first error
//     propagates. A key-selector throw, a null/empty key or a duplicate key excludes the record (it
//     stays in the table, in no sibling list, like a filtered-out record; B3); a parent-key
//     selector throw, a self-parent or a would-be cycle puts it at the top level (forced), and it
//     returns to its parent once that is valid again.
//   - A selector throw always propagates; a null selector is rejected by the verb (B4).
class ParentKeyHierarchy final : public SourceHierarchy
{
public:
    ParentKeyHierarchy(
        ITrackerHandleManager const* owner,
        winrt::TableViewKeySelector const& keySelector,
        winrt::TableViewKeySelector const& parentKeySelector);

    void Prepare(
        ISourceHierarchyHost& host,
        std::vector<winrt::Windows::Foundation::IInspectable> const& rootItems) override;
    void MatchForRefresh(
        ISourceHierarchyHost& host,
        std::vector<SourceRecord*> const& rootMatches,
        std::vector<SourceRecordPtr>& removed) override;
    void Commit(ISourceHierarchyHost& host, std::vector<SourceRecord*>& added) override;
    void CollectIncoming(
        winrt::Windows::Foundation::IInspectable const& item,
        std::vector<winrt::Windows::Foundation::IInspectable>& items) override;
    void DiscardIncoming() noexcept override {}
    void OnInserted(ISourceHierarchyHost& host, SourceRecord* record) override;
    void OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>& removed) override;
    void OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t fromIndex, uint32_t toIndex) override;
    void ForEachChildRecord(std::function<void(SourceRecordPtr const&)> const& visit) const override;
    bool Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const& propertyName) override;
    void TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed) override;
    void Release(ISourceHierarchyHost& host) noexcept override;

private:
    // Each record's cached keys. Object keys are held while the tree is current so their addresses
    // cannot be reused (v1 section 3.1).
    struct NodeKeys
    {
        winrt::hstring Key;
        winrt::hstring ParentKey;
        // Object keys only (value keys need only Key); tracked like Item.
        std::optional<tracker_ref<winrt::Windows::Foundation::IInspectable>> KeyObject;
    };

    // Prepare(), strict: throws when a prepared record is not reachable from the top level (it is
    // on or below a cycle).
    static void ValidateNoCycles(
        std::unordered_map<winrt::hstring, uint32_t> const& keyIndex,
        std::vector<NodeKeys> const& nodeKeys);

    // The live rules. Evaluates both selectors for record into keys; the first failure goes to
    // error (kept when already set). False when record cannot be keyed (excluded): the key selector
    // threw, or the key is null/empty or held by another record. A parent-key throw leaves
    // ParentKey empty (top level).
    bool Keys(ISourceHierarchyHost& host, SourceRecord const* record, NodeKeys& keys, std::exception_ptr& error) const;

    // The parent for a keyed record, given the orphans it is about to adopt. A parent at or below
    // record or one of the orphans would close a cycle: then nullptr, record is forced to the top
    // level and error is set. Otherwise record is no longer forced.
    SourceRecord* PlaceParent(SourceRecord* record, std::vector<SourceRecord*> const& orphans, std::exception_ptr& error);

    // Top-level records waiting for key (their parent key), in source order, other than record and
    // the forced ones (RestoreForcedTop places those).
    std::vector<SourceRecord*> WaitingFor(ISourceHierarchyHost& host, winrt::hstring const& key, SourceRecord const* record) const;

    // A newly linked record (insert, or re-inclusion after exclusion): places it, raises its Add
    // and adopts its orphans (one: a Move; more: one Reset with Added = record and no Add; spec
    // Bulk threshold), then RestoreForcedTop. A fault while raising requests a re-read.
    void LinkAndAdopt(ISourceHierarchyHost& host, SourceRecord* record, std::exception_ptr& error);

    // An already linked record takes orphans: one Move, or one Reset for more than one.
    void Adopt(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecord*> const& orphans);

    // Moves record under newParent at its source-order position and raises the Move; nothing when
    // it is already there.
    void MoveTo(ISourceHierarchyHost& host, SourceRecord* record, SourceRecord* newParent);

    // Takes a linked leaf record out of the tree (not the table) and raises its Remove.
    void Exclude(ISourceHierarchyHost& host, SourceRecord* record);

    // record's children to the top level, one Move each (record is still linked).
    void OrphanChildren(ISourceHierarchyHost& host, SourceRecord* record);

    // Returns each forced record whose parent no longer closes a cycle to it, one Move each, until
    // none can move.
    void RestoreForcedTop(ISourceHierarchyHost& host);

    // Drops record's keys, and its key from m_keyIndex when it holds it.
    void Unkey(SourceRecord const* record);

    // record's source-order index among parent's linked children (excluded records are in no
    // list). While OnRemoving runs, the leaving record is still linked but no longer in
    // RootRecords(); it is counted when it comes before the position.
    uint32_t SourceOrderPosition(ISourceHierarchyHost& host, SourceRecord const* record, SourceRecord* parent) const;

    // App delegates, tracked (see SourceTable Reference tracking). Optional so Release() can
    // destroy them while the tracker owner is alive.
    std::optional<tracker_ref<winrt::TableViewKeySelector>> m_keySelector;
    std::optional<tracker_ref<winrt::TableViewKeySelector>> m_parentKeySelector;

    // Node key -> record, and record -> keys (keyed records only). Prepare() stages by root
    // position (under Refresh() the records are not matched yet; an excluded position has an empty
    // Key); Commit() resolves each position through host.RootRecords() and rebuilds these.
    std::unordered_map<winrt::hstring, SourceRecord*> m_keyIndex;
    std::unordered_map<SourceRecord const*, NodeKeys> m_nodeKeys;
    std::unordered_map<winrt::hstring, uint32_t> m_stagedKeyIndex;  // node key -> root position
    std::vector<NodeKeys> m_stagedNodeKeys;                         // by root position

    // Root records in no sibling list: not keyed (B1, B3).
    std::unordered_set<SourceRecord const*> m_excluded;

    // Records at the top level only because their parent would close a cycle (or did, at a
    // tolerant re-read). Preferred as the break point at the next Commit.
    std::unordered_set<SourceRecord*> m_forcedTop;

    // OnRemoving: the record leaving, still linked while its children are orphaned.
    SourceRecord const* m_leaving{ nullptr };

    // False until the first Commit(): Prepare() is strict before it, tolerant after.
    bool m_committed{ false };
};
