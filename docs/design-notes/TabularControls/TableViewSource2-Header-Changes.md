# TableViewSource2 header and IDL changes and design drift during implementation

This log tracks every non-comment change made to a header (`.h`) or IDL (`.idl`) file after the
reviewed TableViewSource2 design was committed (baseline `5f6617e792`, "TableViewSource2: review
fixes for headers and spec"). Comment-only edits are not listed.

Add an entry whenever an implementation commit changes a declaration, signature, member, base
class, include or inline definition in a header or IDL. Each entry gives the change, the reason,
and the commit that made it.

It also tracks implementation decisions that drift from what the headers or the spec
(`Shaping-Architecture-Redesign.md`) define, or that settle behaviour the spec leaves open. See
[Decisions that drift from the spec or headers](#decisions-that-drift-from-the-spec-or-headers).

## Decisions that drift from the spec or headers

From 2026-10-29 (after `96297adaf5`) the spec is frozen: implementation does not edit
`Shaping-Architecture-Redesign.md`. Every decision that departs from it, or from a declaration in
the reviewed headers, is recorded here instead, with what the spec or header says, what was done,
why, and the commit. Earlier entries (M1, M2) are listed for completeness; the two M1 entries
marked "spec revised" were also written back into the spec before the freeze.

| # | Spec / header says | Implementation does | Why | Commit |
|---|---|---|---|---|
| 1 | D1: `ShapedCollectionView` derives from `ItemsSourceView` and `View()` returns it as is. | Native read-only `IBindableVector` + `INotifyCollectionChanged` + `IKeyIndexMapping`; `TableViewSource2::GetItemsSourceView()` wraps it once in `winrt::ItemsSourceView`. (Spec revised.) | `ItemsSourceView`'s implementation is in the Controls DLL and exposes no overridable interface, so it cannot be subclassed from the Tabular DLL. | `8ebf18bcbe` (M1) |
| 2 | D3: change args carry a bindable `Vector` of k `nullptr`s. | Count-only read-only `PlaceholderItems` (`IBindableVector`, `GetAt` returns `nullptr`, mutators throw), O(1) per event; unused side is an empty list, never null. (Spec revised.) | `Collections/Vector.h` is not on the Tabular DLL include path. | `8ebf18bcbe` (M1) |
| 3 | `ShapingManager::SetOwner` rejects owner changes while a commit is applying. | `SetOwner(nullptr)` is allowed during a commit; owner-released waiters are deferred to commit end via `SetCommitEndedHandler`. | An owner must be able to release from inside its own callbacks. | `6cc4e48ed4` (M1) |
| 4 | Spec is silent on a hold ending inside a freeze. | `EndHoldResets` performs its deferred reset only when no freeze is open (`m_freezeDepth == 0`); otherwise `EndFreeze` reports it. | Symmetric with `EndFreeze`; avoids a reset raised mid-freeze. | `b979366dcb` (M2) |
| 5 | Global constraint: app exceptions propagate. | `SubscribeRecordPropertyChanged` is `noexcept`: a subscribe failure marks the item not observable, with no retry. | A throw inside a commit would unwind with freed records left in the lookups. Cost: that item's property edits are not live-shaped. | `5f3bba7ebc` (M2) |
| 6 | Spec covers off-thread item `PropertyChanged` (marshalled) but not an off-thread root collection notification. | A bound table throws `hresult_wrong_thread` (and asserts in debug) for a root `CollectionChanged` / `VectorChanged` raised on another thread, before taking any lock. | Matches XAML `ItemsControl` behaviour; marshalling an index-based change would apply it against a moved collection. | `f9f7b0326c` (M2) |
| 7 | Spec is silent on a failed posted re-read. | The posted re-read callback swallows a `Refresh` throw and does not re-post; the table stays owed until the next notification. | A posted callback has no caller to propagate to. Deferred: revisit with M6 error handling. | `b3ae0903cc` (M2) |
| 8 | `SourceHierarchy::OnInserted(host, record)` / `OnRemoving(host, record) noexcept` fill the change's tree fields and "may add Moves to raise after it". | `bool OnInserted(host, record, std::exception_ptr& error)`, new `AfterInserted(host, record)`, and `bool OnRemoving(host, record, removed)` (not `noexcept`). `false` means the record is not linked (ParentBy: excluded), so the caller raises no Add / Remove. The strategy raises every other tree change itself, through the host, as its own SourceChange: a Move per moved record, or one Reset when an insert adopts more than one orphan. `OnInserted` captures a selector or key error and the caller rethrows it after raising. `OnRemoving` leaves the record linked so its orphans' Moves are raised while it is still in the shaped tree; the caller then unlinks it and raises the Remove. | The caller needs to know whether to raise; adoption must follow the Add (spec Orphan adoption); a ChildrenBy subtree must reach `removed`. | this task's commit |
| 9 | Spec: the verb throws on a bad key or a cycle; it leaves open what `Refresh()` does with keys a live edit made bad. | `ParentKeyHierarchy::Prepare` is strict at the verb (and for a parked verb). When `Refresh()` re-reads the committed strategy it is tolerant, matching live-edit placement: a null or empty key is excluded; of two records with one key the record holding it keeps it (else the first in source order) and the other is excluded; a self-parent goes to the top level; a cycle is broken at a record already forced to the top level, else at the member earliest in source order. A key or parent-key selector throw still propagates (the table is untouched and `m_diverged` set). | Every Refresh after a bad live edit would otherwise throw forever; the spec says excluded records are re-evaluated on Refresh. | this task's commit |
| 10 | Spec Bulk threshold covers orphan adoption (more than one adopted is a bulk change). | Orphaning (a parent removed, or its key changed or lost) raises one Move per child to the top level, whatever the count. | The spec names a threshold only for adoption; removals stay surgical. | this task's commit |
| 11 | Spec Misbehaving records: a null, empty or duplicate key excludes the record; it does not say what a throwing key selector does. | A key-selector throw also excludes the record (a parent-key selector throw puts it at the top level, as the spec says). An excluded record stays in the table and in no sibling list; becoming excluded raises its Remove while the record stays alive in `m_records` and the lookups (nothing is destroyed), becoming included again raises its Add. | No key means it cannot be placed or adopt children; exclusion is the spec's nearest rule. | this task's commit |
| 12 | Spec is silent on null hierarchy selectors. | `ParentBy` and `ChildrenBy` throw `hresult_invalid_argument` for a null selector. | A null would fault later, inside a commit. | this task's commit |
| 13 | Header: `DestroyChildFeed` "releases the feed's records and their subtrees through the host"; `Release` frees everything. | `DestroyChildFeed` (and so `Release`) does not call `host.ReleaseRecord`. `ReleaseRecord` reads `Item.get()`, which is not allowed on a teardown path. Non-teardown removal goes through `TakeFeed` (which calls `ReleaseRecord`), and `SourceTable::Clear()` clears the lookups wholesale. | Teardown never calls `get()`. | this task's commit |
| 14 | Header: the child lambdas check `AdmitChange` before calling the entry points. | The lambdas check the thread, lock the host and compare `CurrentHierarchy()`. Each entry point classifies the change against its feed and then calls `AdmitChange(isReset)` itself. When that returns false, nothing more is touched. | Classification needs the feed, which only the strategy owns. This mirrors the root handlers. | this task's commit |
| 15 | Spec leaves the per-kind handling of child changes, and what `Refresh()` does with a throwing children selector, open. | Only a single Add, or a single Remove whose old item is the record at that index, applies positionally; Replace, Move, multi-item and Reset changes become `AdmitChange(true)` (a whole-tree re-read). `Commit` always rebinds each feed to the collection `Prepare` read. After the verb, `CollectIncoming` treats a throwing selector as a leaf and `BuildChildFeed` calls the selector again: a throw there makes a leaf (spec) and is rethrown after the raise; an item that now fails validation is skipped with `RequestReset` (spec duplicate rule). In `Refresh()` (`Prepare`), a selector throw propagates and the table is untouched (`m_diverged`), as at the verb. A collection that refuses the handler stays unobserved. | Smallest correct set for Milestone 3; Milestone 5 makes child Replace / Move surgical. A Refresh that turned a throwing selector into a leaf would silently drop a subtree that the app can fix. | this task's commit |
| 16 | Header: "A replace is OnRemoving then OnInserted." | Under a hierarchy, `ReplaceAt` validates the incoming item (and its ChildrenBy subtree) against the leaving record and, under ChildrenBy, its leaving subtree (a replacement may share children with the item it replaces), then runs `RemoveAt` and `InsertAt`: the view sees a Remove and an Add (plus any orphan Moves), not one Replace. `InsertAt` validates again, so the children selector may run twice. | Under ParentBy a replace can re-key, adopt and orphan in one step; two hook calls keep one path for each. | this task's commit |
| 17 | Header: `Reevaluate` runs for "a property the selectors read". | ParentBy re-keys on every `PropertyChanged` (any name), gated like a single-item change (`AdmitChange(false)`). A changed key or parent key is placed as spec Misbehaving records says and raised surgically (Moves, a Remove / Add on exclusion, one Reset when more than one orphan is adopted); a record whose keys did not change raises nothing and throws nothing. ChildrenBy `Reevaluate` is a no-op until Milestone 6. | The selectors are opaque delegates, so the read set is unknown. | this task's commit |
| 18 | Spec: `Level` is "group levels + `Depth`" (0-based). | When the source is hierarchical, `RowInfo.Level` adds 1, so top-level records are Level 1, as `TableViewSource` (v1) and the TableView tree presenter expect. Flat rows keep 0. | The TableView indent and toggle logic and the existing tree test helpers treat Level 0 as "not a tree row". | this task's commit |
| 19 | Roadmap: expansion verbs are Milestone 7. | `ShapingManager::SetAllExpanded(ExpansionAxis::Rows, ...)` and `ShapedTree::SetAllRowsExpanded` ship in Milestone 3, so `TableView.ExpandAllRows` / `CollapseAllRows` work on a hierarchical source. `Groups`, `SetExpanded`, `ExpandSubtree` and `Reveal` still throw `not_implemented`. | Hierarchy rows start collapsed; without one expansion verb no child row can ever be shown or tested. | this task's commit |
| 20 | Roadmap: surgical inserts, removes and moves are Milestone 5. | Under the empty spec, `ShapingEngine::ApplySourceChange` links or unlinks the one node for a source Add, Remove or Move (flat and hierarchical), with the first definitions of `ShapedTree::Link`, `Unlink`, `PropagateCount`, `Renumber` and the raise helpers, plus an `Unlink(node, parent, index, isMove)` overload. A Replace, or any non-empty spec, still rebuilds. Inside a move within one source-owned list (a ParentBy sibling reorder), a listener that reads the tree from the `Remove` event can see the list in its new order. The same window exists across lists: a ParentBy reparent has already put the node into the new parent's source-owned list when `Remove` is raised, so a listener that calls `GetAt` from that event reads shifted rows. `Unlink` still reports the old flat index correctly, also when the new parent is an ancestor of the old one (it corrects the stale ancestor sums for the moved row). Milestone 5's engine-owned lists close the window. | Spec §6 makes single-item hierarchy changes (adoption of one orphan, reparent, a removed parent's children) surgical; they cannot be without the link primitives. | this task's commit |
| 21 | Spec O3 / `IShapingOwner.h`: `OnShapingApplied` is called when `Changes` is not `None` or `Retired` is non-empty; P9: the owner re-reads realized chrome there. | `ShapingManager` also calls it after every surgical source change (`CommitState::chromeDirty`), with `Changes` `None` when no row moved. `TableView`'s `SourceOwnerSink` does not re-read synchronously: it queues the existing v1 restamp (`QueueRefreshRealizedRowHierarchyState`, one dispatcher turn later, coalesced), and skips a `Reset` (already restamped) and a drained table. | A first child linked under a collapsed parent raises no row event, so under O3 the owner never heard of it and the parent kept a stale (missing) expander; the deferred restamp is the path v1 already uses and runs after the repeater has applied the commit's row events. | this task's commit |

## IDL

No IDL changes so far.

## Headers

### `controls/dev/TableViewSource2/View/ShapedCollectionView.h`

| Change | Reason | Commit |
|---|---|---|
| No longer derives from `ItemsSourceView` (dropped `#include "ItemsSourceView.h"` and `ForwardRefToBaseReferenceTracker`). Now `winrt::implements<ShapedCollectionView, IBindableVector, IBindableVectorView, IBindableIterable, INotifyCollectionChanged, IKeyIndexMapping>`. | `ItemsSourceView`'s implementation type lives in the Controls DLL; TableViewSource2 builds into the Tabular DLL and cannot override its `*Core` virtuals across the DLL boundary. Option A kept (wrap with the public `winrt::ItemsSourceView{ collection }` constructor); alternatives (implement `IItemsSourceView` natively, move the view into the Controls DLL) were considered and deferred. | `8ebf18bcbe` (M1) |
| `ItemsSourceView` overrides replaced by interface members: `GetSizeCore` → `uint32_t Size()`, `GetAtCore(int)` → `GetAt(uint32_t)`, `IndexOfCore` → `bool IndexOf(value, uint32_t&)`, `KeyFromIndexCore` → `KeyFromIndex(int32_t)`, `IndexFromKeyCore` → `int32_t IndexFromKey(hstring)`. `HasKeyIndexMappingCore` removed (implied by implementing `IKeyIndexMapping`). | Follows the base-class change. | `8ebf18bcbe` (M1) |
| Added `GetView()`, `First()`, the read-only mutators `SetAt`, `InsertAt`, `RemoveAt`, `Append`, `RemoveAtEnd`, `Clear` (each throws `E_ILLEGAL_METHOD_CALL`), and the `CollectionChanged` add/remove pair. | Required by `IBindableVector` / `IBindableIterable` / `INotifyCollectionChanged`. | `8ebf18bcbe` (M1) |
| Constructor is now `noexcept`. | It only stores the back-reference. | `8ebf18bcbe` (M1) |
| Added private `Raise(args)` and `winrt::event<NotifyCollectionChangedEventHandler> m_collectionChanged`. | Event storage previously came from the `ItemsSourceView` base. | `8ebf18bcbe` (M1) |

### `controls/dev/TableViewSource2/View/ShapedRowQueries.h`

| Change | Reason | Commit |
|---|---|---|
| Added `winrt::hstring KeyAt(uint32_t index) const` and `static winrt::hstring KeyOf(ShapedNode const& node)`. | One row-key helper shared by `ShapedCollectionView::KeyFromIndex` and `ShapedRowMetadata::GetIdentity`, so the two key spaces cannot drift. | `8ebf18bcbe` (M1) |

### `controls/dev/TableViewSource2/Shaping/ShapingManager.h`

| Change | Reason | Commit |
|---|---|---|
| `View()` returns `winrt::Microsoft::UI::Xaml::Interop::IBindableVector` instead of `winrt::ItemsSourceView`; `m_view` is `std::optional<tracker_ref<IBindableVector>>`. | Follows the `ShapedCollectionView` base-class change; `TableViewSource2` does the `ItemsSourceView` wrap. | `8ebf18bcbe` (M1) |
| Added `ShapedCollectionView* m_viewImpl`. | Lets the `noexcept` tree listener reach the implementation without a QI. | `987774dc29` (M1) |
| Added private forward declaration `class CommitScope;`. | RAII scope around `BeginCommit` / `EndCommit`, defined in the `.cpp`. | `987774dc29` (M1) |
| Added `void SetCommitEndedHandler(std::function<void()>) noexcept`, `bool IsApplying() const noexcept` and member `std::function<void()> m_commitEnded`. `SetOwner(nullptr)` no longer throws during a commit. | Lets an owner release from inside its own callbacks; owner-released waiters run at commit end. | `6cc4e48ed4` (M1) |
| Removed `bool m_resyncPosted`. | The posted retry now goes through `SourceTable::PostReread`, guarded by its `m_rereadPosted`. | `db396c693c` (M2) |
| `CommitState` gains `bool chromeDirty{ false }`; the `EndCommit` comment lists it as a third reason to call `OnShapingApplied`. | A surgical source change with no row event can still change a staying row's expander (drift #21). | this task's commit |
| Added private `bool BeginVerb(wchar_t const* verb) const`: the shared preamble of ParentBy, ChildrenBy and ClearHierarchy (throws while an apply runs; false when detached). | One guard instead of three copies. | this task's commit |

### `controls/dev/TableViewSource2/Source/SourceTable.h`

| Change | Reason | Commit |
|---|---|---|
| `void PostReread() noexcept` moved from private to public. | `ShapingManager::ResyncSource` uses it for its posted retry. | `b3ae0903cc` (M2) |
| Added private `void PostRereadIfOwed() noexcept`. | `Refresh()` epilogue: posts a re-read owed by its own dispatch unless a freeze or hold will report it. | `b3ae0903cc` (M2) |
| `SubscribeRecordPropertyChanged` is now `noexcept`. | A subscribe failure must not unwind a commit and leave freed records in the lookups; the item is marked not observable instead. | `5f3bba7ebc` (M2) |
| Added private `void RaiseEdit(SourceChangeKind, uint32_t, uint32_t, std::vector<SourceRecord*> const&, std::vector<SourceRecordPtr>&)`. | The edits' `RaiseSourceChanged`; a listener throw still posts any owed re-read before rethrowing. | `f9f7b0326c` (M2) |
| Added private `static void ThrowIfOffBoundThread(ThreadBinding const&)`. | A bound table rejects a root collection notification raised on another thread with `hresult_wrong_thread`. | `f9f7b0326c` (M2) |
| `RaiseEdit` gains `SourceRecord* oldParent, SourceRecord* newParent` after `kind`. | Edits under a hierarchy raise parented changes. | this task's commit |
| `DrainPendingChanges()` is now an `ISourceHierarchyHost` override; added `std::function<void()> MakeThreadCheck() const override`. | Child-collection handlers end with it, and their lambdas apply the root's thread rule. | this task's commit |
| Added private `void ReplaceHierarchy(std::unique_ptr<SourceHierarchy> next)`. | One body for `SetParentKeyHierarchy`, `SetChildCollectionHierarchy` and `ClearHierarchy` (park, or prepare, release, commit and raise one Reset). | this task's commit |
| `void UpdateDepths(SourceRecord*)` is not `noexcept`. | The linear rewrite uses a `std::vector` work stack, which can throw on growth. | this task's fix commit |
| `PostRereadIfOwed()` is also called from `RaiseSourceChanged`'s listener-fault path (comment only; no signature change). | Hierarchy strategies raise through the host and have no `RaiseEdit`; the owed re-read must still be posted (idempotent with `RaiseEdit`). | this task's commit |
| Added private `std::vector<winrt::Windows::Foundation::IInspectable> CollectIncomingItems(IInspectable const&) const`. | One builder of the incoming item plus its ChildrenBy subtree for `InsertAt` and `ReplaceAt`. | this task's fix commit |
| Added private `bool IsLeavingWith(SourceRecord const*, SourceRecord const*) const noexcept`. | `ValidateIncoming` exempts the leaving root's ChildrenBy subtree on a replace. | this task's fix commit |
| Added private `std::vector<SourceRecordPtr> m_zombies`. | A listener fault during a hierarchy remove leaves the record in the engine's tree; it is parked here and handed to the next re-read's Reset so no pointer outlives its owner. | this task's fix commit |

### `controls/dev/TableViewSource2/TableViewSource2.h`

| Change | Reason | Commit |
|---|---|---|
| Added `#include <optional>`, internal `winrt::ItemsSourceView GetItemsSourceView()`, and cache members `std::optional<tracker_ref<winrt::ItemsSourceView>> m_itemsSourceView` and `void* m_itemsSourceViewCollection`. | Wraps `ShapingManager::View()` once in a `winrt::ItemsSourceView` for TableView's rows repeater, as v1 `TableViewSource::GetItemsSourceView()`. | `987774dc29` (M1) |
| Added private `void RunOwnerReleasedWaiters()`. `TrySetOwner` no longer removes the claiming owner's waiter (the caller does). | A release during a commit defers waiters to the commit's end. | `6cc4e48ed4` (M1) |

### `controls/dev/TableViewSource2/Source/SourceRecord.h`

| Change | Reason | Commit |
|---|---|---|
| Added `static ObjectAddress AddressOf(IInspectable const& item)`. | Canonical COM identity (the `IUnknown` address) for lookups when a managed item arrives through a different interface pointer. | `dbc625e395` (M1) |

### `controls/dev/TableViewSource2/Engine/FenwickTree.h`

| Change | Reason | Commit |
|---|---|---|
| Added the inline definition of `template <typename CountOf> void FenwickTree::Build(uint32_t, CountOf&&)` (linear O(n) build). | Template body must live in the header. | `4dfb4995a8` (M1) |

### `controls/dev/TableView/TableView.h`

| Change | Reason | Commit |
|---|---|---|
| Added `#include "Shaping/IShapingOwner.h"`, `tracker_ref<winrt::TableViewSource2> m_activeSource2`, nested `struct SourceOwnerSink final : IShapingOwner` (empty callbacks) and member `m_sourceOwnerSink`. | TableView accepts a `TableViewSource2` as `ItemsSource` and registers itself as its owner. | `1e83acaf9c` (M1) |
| `SourceOwnerSink` gains `explicit SourceOwnerSink(TableView* owner) noexcept` and a private `TableView* m_owner`; `OnShapingApplied` is declared and defined out of line in `TableView.cpp`; the member is initialized as `m_sourceOwnerSink{ this }`; the comment no longer says the callbacks are empty until Milestone 9. | The owner re-reads realized rows' level and expander in `OnShapingApplied` (P9; drift #21), which needs the table. | this task's commit |
| Added `bool m_activeSource2Released`, `winrt::weak_ref<winrt::TableViewSource2> m_awaitedSource2`, `void StopAwaitingSource2()` and `void AdoptTableViewSource2(winrt::TableViewSource2)`. | Owner arbitration for a shared `TableViewSource2`, mirroring the v1 `m_activeSource` members. | `1e83acaf9c` (M1) |

### `controls/dev/TableViewSource2/Source/SourceHierarchy.h`

| Change | Reason | Commit |
|---|---|---|
| Added `#include <exception>` and `#include <functional>`. | `std::exception_ptr` and `std::function` in the new signatures. | this task's commit |
| `ISourceHierarchyHost`: added `MakeThreadCheck() const` and `DrainPendingChanges()`. | Child-collection lambdas check the bound thread; child handlers end like root handlers. | this task's commit |
| `SourceHierarchy::OnInserted` / `OnRemoving` now return `bool`; `OnInserted` takes `std::exception_ptr& error`; `OnRemoving` takes `std::vector<SourceRecordPtr>& removed` and is no longer `noexcept`; added pure `AfterInserted(host, record)` (both derived classes match). | Drift #8. | this task's commit |
| `ParentKeyHierarchy`: added `#include <unordered_set>`; private placement helpers `Keys`, `PlaceParent`, `MoveTo`, `Exclude`, `Include`, `WaitingFor`, `Adopt`, `OrphanChildren`, `RestoreForcedTop`, `Unkey`; members `m_excluded`, `m_forcedTop`, `m_pendingAdoptions`, `m_committed`; `NodeKeys` moved above them. | Spec placement after the verb (exclusion, top level, cycles), surgical Moves, strict-verb / tolerant-Refresh (drifts #9-#11). | this task's commit |
| Added pure `ForEachChildRecord(std::function<void(SourceRecordPtr const&)> const&) const`. | `SetLiveShaping` subscribes child records too. | this task's commit |
| Added free functions `AppendSourceSubtree` and `IndexInSourceSiblings`. | `Added` lists and sibling indexes for parented changes. | this task's commit |
| `SourceHierarchy`: added `Generation()` (unique per strategy, private `m_generation` / `s_lastGeneration`); `ChildCollectionsHierarchy::InsertChild` returns `std::exception_ptr` instead of `void`. | Child event lambdas compare the generation, not a raw pointer that a later strategy could reuse; the caller drains pending changes before rethrowing a selector error. | this task's commit |
| `ChildCollectionsHierarchy`: `BuildChildFeed(host, SourceRecordPtr const&, std::exception_ptr&)`; added `TakeFeed`, `InsertChild`, `RemoveChild` and `ITrackerHandleManager const* m_trackerOwner`. | The feed owner's weak pointer; a throwing selector makes a leaf (spec) and the selector re-runs (drift #15); incremental child edits; the `const` `CollectIncoming` enumerates without a host. | this task's commit |

### `controls/dev/TableViewSource2/Engine/ShapedTree.h`

| Change | Reason | Commit |
|---|---|---|
| Added `void Unlink(ShapedNode* node, ShapedNode* parent, uint32_t index, bool isMove) noexcept;`, which takes the old place explicitly; the two-argument `Unlink` forwards to it. | `ShapingEngine::ApplySourceChange` runs after SourceTable has re-parented or erased the record, so `ShapedParent()` and `IndexInParent` no longer name the old place (drift #20). | this task's commit |

### `controls/dev/TableViewSource2/Shaping/IShapingOwner.h`

| Change | Reason | Commit |
|---|---|---|
| Comment only: `OnShapingApplied` is also called after a surgical source change that raised no row event. | Documents drift #21 at the contract. | this task's commit |
