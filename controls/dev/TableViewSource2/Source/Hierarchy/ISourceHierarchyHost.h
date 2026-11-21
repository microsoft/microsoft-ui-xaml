// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "Source/SourceRecord.h"
#include "Source/ThreadBinding.h"

// The host side of the source-tree strategies (declarations only); see Source/Hierarchy/SourceHierarchy.h.

class SourceHierarchy;

// What a strategy may ask of its SourceTable. SourceTable implements it privately. Every member
// runs on the UI thread.
struct ISourceHierarchyHost
{
    // The tracker owner every app object a strategy holds is bound to. Null once detached.
    virtual ITrackerHandleManager const* TrackerOwner() const noexcept = 0;

    // Weak self for event lambdas (child collections). Locked only on the UI thread (SourceTable
    // Lifetime rule); an expired host drops the notification.
    virtual std::weak_ptr<ISourceHierarchyHost> WeakHost() noexcept = 0;

    // The current strategy, or nullptr when flat. A child-collection lambda checks it is still the
    // strategy that subscribed before routing a change to it.
    virtual SourceHierarchy* CurrentHierarchy() noexcept = 0;

    // Root-collection records, in source order.
    virtual std::vector<SourceRecordPtr> const& RootRecords() const noexcept = 0;

    // Records: create one (added to the lookups and, when live shaping is on, subscribed), or take
    // one out of the lookups before the caller destroys it. Release walks no children.
    virtual SourceRecordPtr CreateRecord(winrt::Windows::Foundation::IInspectable const& item) = 0;
    virtual void ReleaseRecord(SourceRecord* record) noexcept = 0;

    // SourceTable::ValidateIncoming: throws hresult_invalid_argument on a null, a duplicate within
    // items, or an item already in the table outside replacing and its subtree.
    virtual void ValidateIncoming(
        std::vector<winrt::Windows::Foundation::IInspectable> const& items,
        SourceRecord const* replacing) const = 0;

    // Shared tree operations (see SourceTable). SiblingsOf(nullptr) is the top level.
    virtual std::vector<SourceRecord*>& SiblingsOf(SourceRecord* parent) noexcept = 0;
    virtual void Reparent(SourceRecord* record, SourceRecord* newParent, uint32_t index) = 0;

    // Re-entrancy gate for child-collection notifications (#19); the same table as the root
    // collection (see SourceTable). isReset: a child Reset or a multi-item change reported as one.
    // True: apply positionally (an incremental change, table not frozen and not diverged). False:
    // the host has handled the notification and the strategy must not touch anything:
    //   - frozen, or held and (diverged or isReset): skipped (m_diverged and m_rereadDue set);
    //   - free and (diverged or isReset): the host re-reads now (Refresh()); a validation throw
    //     propagates into the app's Add / Insert / setter and the table stays diverged.
    virtual bool AdmitChange(bool isReset) = 0;

    // SourceTable::RaiseSourceChanged: raise the change inside a freeze, then destroy removed.
    virtual void RaiseSourceChanged(
        SourceChangeKind kind,
        SourceRecord* oldParent,
        SourceRecord* newParent,
        uint32_t oldIndex,
        uint32_t newIndex,
        std::vector<SourceRecord*> const& added,
        std::vector<SourceRecordPtr>& removed) = 0;

    // A child change was rejected (null/duplicate) after the child collection had already changed.
    // Decided (no subtree-scoped Reset): sets SourceTable::m_diverged (not m_rereadDue), exactly
    // as a rejected root change does, so the next notification (root or any child collection) is
    // handled as a whole-tree re-read (Refresh(): one Reset, unchanged records reused per feed)
    // until one validates. Raises nothing itself. Only for rejections: a child Reset or
    // multi-item change goes through AdmitChange(true).
    virtual void RequestReset() noexcept = 0;

    // The table's thread binding (decided, A2). Child-collection lambdas capture it and call
    // ThrowIfOffBoundThread(*binding) before locking the host, as the root-collection lambdas do.
    // Shared, not copied: ThreadBinding holds an atomic.
    virtual std::shared_ptr<ThreadBinding const> Binding() const noexcept = 0;

    // SourceTable::DrainPendingChanges (decided, A3): the end of a child notification handler
    // that applied a change (never after AdmitChange returned false).
    virtual void DrainPendingChanges() = 0;

protected:
    ~ISourceHierarchyHost() = default;
};
