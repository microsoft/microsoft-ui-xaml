// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "Source/SourceRecord.h"
#include "SourceHierarchy.h"

// ParentBy strategy (declarations only); see Source/Hierarchy/SourceHierarchy.h.

// ParentBy(keySelector, parentKeySelector). Every item is in the root collection; the tree is an
// index over Records(). Key rules follow v1 (Hierarchical-Data-Rows-Parent-Key-Design.md section 3):
// value keys compare by value, object keys by reference; a null/empty parent key or an orphan makes
// a top-level record; a null/empty key, a duplicate key, a self-parent or a cycle (found by counting
// records reachable from the top level) throws before any write.
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
        std::vector<winrt::Windows::Foundation::IInspectable>& items) const override;
    void OnInserted(ISourceHierarchyHost& host, SourceRecord* record) override;
    void OnRemoving(ISourceHierarchyHost& host, SourceRecord* record) noexcept override;
    void OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t fromIndex, uint32_t toIndex) override;
    bool Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const& propertyName) override;
    void TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed) override;
    void Release(ISourceHierarchyHost& host) noexcept override;

private:
    // Lookup form of a selector result (v1 MakeNodeKey): value types by value, objects by
    // reference. Empty for null.
    static winrt::hstring MakeNodeKey(winrt::Windows::Foundation::IInspectable const& key);

    // Counts records reachable from the top level of the prepared links; any unreached record is
    // on or below a cycle. Throws.
    void ValidateNoCycles(ISourceHierarchyHost const& host) const;

    // App delegates, tracked (see SourceTable Reference tracking). Optional so Release() can
    // destroy them while the tracker owner is alive.
    std::optional<tracker_ref<winrt::TableViewKeySelector>> m_keySelector;
    std::optional<tracker_ref<winrt::TableViewKeySelector>> m_parentKeySelector;

    // Each record's cached keys. Object keys are held while the tree is current so their addresses
    // cannot be reused (v1 section 3.1).
    struct NodeKeys
    {
        winrt::hstring Key;
        winrt::hstring ParentKey;
        // Object keys only (value keys need only Key); tracked like Item.
        std::optional<tracker_ref<winrt::Windows::Foundation::IInspectable>> KeyObject;
    };

    // Node key -> record, and record -> keys. Prepare() stages by root position (under Refresh()
    // the records are not matched yet); Commit() resolves each position through
    // host.RootRecords() and rebuilds these.
    std::unordered_map<winrt::hstring, SourceRecord*> m_keyIndex;
    std::unordered_map<SourceRecord const*, NodeKeys> m_nodeKeys;
    std::unordered_map<winrt::hstring, uint32_t> m_stagedKeyIndex;  // node key -> root position
    std::vector<NodeKeys> m_stagedNodeKeys;                         // by root position
};
