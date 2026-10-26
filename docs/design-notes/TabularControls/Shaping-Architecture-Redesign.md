# TableView shaping redesign — TableViewSource2

**Status:** In progress. This doc grows section by section as each part of the design is agreed.
It covers only what exists in `controls/dev/TableViewSource2` today. Everything is declared; nothing
is implemented, and every verb still throws `hresult_not_implemented`.

**Related:**

- [TableView-Data-Pipeline-Current-Structure.md](TableView-Data-Pipeline-Current-Structure.md)
  describes the current (v1) pipeline this replaces.
- [TableView-Row-Identity-Reuse-Spec.md](TableView-Row-Identity-Reuse-Spec.md) is the source of the
  `SourceRecord` design.
- [Shaping-Architecture-Redesign-Original.md](Shaping-Architecture-Redesign-Original.md) is the
  original interface draft (`ShapeSpec`, `ShapePlan`, coordinator, fast paths). Kept unchanged as a
  reference; this doc records what is actually adopted.

## Directory layout

`controls/dev/TableViewSource2` is the domain root. Each layer below the runtime class lives in its
own subdirectory. Includes are relative to the root (the `.vcxitems` adds `$(MSBuildThisFileDirectory)`
to the include path), e.g. `#include "Source/SourceTable.h"`; files inside a layer include siblings
by bare name.

```text
controls/dev/TableViewSource2/
  TableViewSource2.idl / .h / .cpp / .vcxitems   public runtime class (verbs, Refresh, IsLiveShaping)
  Source/                                        source layer: input collection + record table
    CollectionAccessor2.h
    SourceRecord.h                               SourceRecord, SourceChange
    SourceTable.h                                SourceTable (records + source tree), HierarchyKind
  Spec/                                          shape spec: verbs in force (data) + delegate registry + path evaluator
    ShapeSpec.h                                  ShapeSpec, FilterSpec, SortSpec, GroupSpec, DelegateRef, ShapeDelta
    PathEvaluator.h                              PathEvaluator (cached SortMemberPathResolver per sort path)
    ShapeDelegates.h                             ShapeDelegates (tracker_ref'd app delegates)
  Engine/                                        shaping engine (node types and change contract so far)
    ShapedTree.h                                 ShapedRef, ShapedRefHash, GroupNode
    ShapeState.h                                 ShapeState, ShapeFlags (engine state inline on SourceRecord)
    ShapedEdit.h                                 ShapedEditKind, ShapedEdit, IShapedEditSink (engine -> flattener)
  View/                                          shaped tree -> visible list
    FlatDelta.h                                  FlatSplice, RowStateField, RowStateEdit, FlatDelta
    Flattener.h                                  Flattener (IShapedEditSink; visible list, counts, expansion)
    GroupExpansion.h                             GroupExpansion (group expansion model, keyed by GroupNode::Identity)
  Shaping/                                       owns and wires the layers above
    ShapingManager.h                             ShapingManager (the coordinator)
```

The WinRT visible list the repeater binds to will also live in `View/` when it is declared.

## Why redesign

The v1 pipeline (`TableViewSource` → `ShapedItemsSource` → adapters) has two structural costs:

1. **It re-reads the source on almost every change.** Each filter, group and hierarchy change, and
   every collection change outside two narrow single-item paths, calls `Refresh()`. That enumerates
   the whole source again (`Materialize`). Frameworks that keep a cached full set (AG Grid,
   DevExtreme, WPF `EnumerableCollectionView`, DynamicData) reshape from that cache instead.
2. **Row identity is computed over and over.** The same object's address and identity string are
   derived several times per rebuild, by several components, and each component keeps its own
   copy.

The redesign keeps one record per source row, kept current by change notifications. That record
table serves as both the shared identity store and the cached full set that shaping runs over.

## Components so far

| # | Component | Files | State |
|---|---|---|---|
| 1 | Public verb API | `TableViewSource2.idl` | Declared |
| 2 | `TableViewSource2` runtime class | `TableViewSource2.h/.cpp` | Verbs are stubs; `Refresh` and `IsLiveShaping` are declared |
| 3 | Source collection resolution: `CollectionAccessor2` | `Source/CollectionAccessor2.h` | Declared, not implemented |
| 4 | Source tracking and source tree: `SourceRecord`, `SourceTable`, `SourceChange` | `Source/SourceRecord.h`, `Source/SourceTable.h` | Declared, not implemented |
| 5 | Shape spec: `ShapeSpec`, `ShapeDelegates`, `PathEvaluator` | `Spec/ShapeSpec.h`, `Spec/ShapeDelegates.h`, `Spec/PathEvaluator.h` | Declared, not implemented; ownership decided (desired in `ShapingManager`, applied in engine) |
| 6 | Shaping engine: shaped tree node types `ShapedRef`, `GroupNode`; per-record `ShapeState`; change contract `ShapedEdit`, `IShapedEditSink` | `Engine/ShapedTree.h`, `Engine/ShapeState.h`, `Engine/ShapedEdit.h` | Node types, record state and change contract declared; engine not started |
| 7 | Flattener and visible list: `FlatDelta`, `Flattener`; the WinRT visible list and row metadata, wrapped in the `ItemsSourceView` that `ShapingManager::View()` returns | `View/FlatDelta.h`, `View/Flattener.h`, `View/GroupExpansion.h` | Flattener and delta types declared, not implemented; WinRT visible list and row metadata not started |
| 8 | Coordinator: `ShapingManager` | `Shaping/ShapingManager.h` | Ownership decided; verbs, handlers, `View()`, flattener and group expansion declared; engine and visible list pending |

## Class diagram

```mermaid
classDiagram
    direction TB

    class TableViewSource2 {
        <<WinRT runtimeclass, ReferenceTracker>>
        -tracker_ref~IInspectable~ m_items
        -unique_ptr~ShapingManager~ m_shapingManager
        +From(items)$ TableViewSource2
        +Filter(predicate) TableViewSource2
        +GroupBy(keySelector, groupDirection) TableViewSource2
        +Sort(sortMemberPath, direction) TableViewSource2
        +Sort(keySelector, direction) TableViewSource2
        +Sort(comparer) TableViewSource2
        +ParentBy(keySelector, parentKeySelector) TableViewSource2
        +ChildrenBy(childrenSelector) TableViewSource2
        +ClearFilter() TableViewSource2
        +ClearGroupBy() TableViewSource2
        +ClearSort() TableViewSource2
        +ClearHierarchy() TableViewSource2
        +Refresh()
        +IsLiveShaping : bool
    }

    class CollectionAccessor2 {
        -tracker_ref~IInspectable~ m_source
        -tracker_ref~IVector~IInspectable~~ m_vector
        -tracker_ref~IVectorView~IInspectable~~ m_vectorView
        -tracker_ref~IBindableVector~ m_bindableVector
        -tracker_ref~INotifyCollectionChanged~ m_notifyCollectionChanged
        -tracker_ref~IObservableVector~IInspectable~~ m_observableVector
        -tracker_ref~IBindableObservableVector~ m_bindableObservableVector
        +IsIndexable() bool
        +IsObservable() bool
        +Count() uint32
        +TryGetAt(index, item) bool
        +Enumerate(throwIfUnsupported) vector~IInspectable~
        +AsNotifyCollectionChanged(useSafeGet) INotifyCollectionChanged
        +AsObservableVector(useSafeGet) IObservableVector~IInspectable~
        +AsBindableObservableVector(useSafeGet) IBindableObservableVector
    }

    class SourceChange {
        +NotifyCollectionChangedAction Kind
        +SourceRecord* OldParent
        +SourceRecord* NewParent
        +uint32 OldIndex
        +uint32 NewIndex
        +vector~SourceRecord*~ Added
        +vector~SourceRecord*~ Removed
    }

    class ShapingManager {
        <<native coordinator>>
        -shared_ptr~SourceTable~ m_sourceTable
        -ShapeSpec m_desired
        -ShapeDelegates m_delegates
        -PathEvaluator m_paths
        -optional~tracker_ref~ItemsSourceView~~ m_view
        -bool m_isApplying
        +View() ItemsSourceView
        +Filter/GroupBy/Sort/ParentBy/ChildrenBy/Clear*()
        +Refresh()
        +IsLiveShaping : bool
        +Clear()
        -OnSourceChanged(change)
        -OnItemChanged(record, propertyName)
        -ApplyDesiredSpec()
        -Reapply()
    }

    class SourceTable {
        -ITrackerHandleManager* m_trackerOwner
        -optional~CollectionAccessor2~ m_sourceAccessor
        -event_token m_sourceEventToken
        -vector~SourceRecordPtr~ m_records
        -vector~SourceRecord*~ m_topLevel
        -HierarchyKind m_hierarchyKind
        -map~hstring, SourceRecord*~ m_keyIndex
        -map~SourceRecord*, ChildFeed~ m_childFeeds
        -map~void*, SourceRecord*~ m_rawLookup
        -map~ObjectAddress, SourceRecord*~ m_addressLookup
        -bool m_identityIndexEnabled
        -map~hstring, SourceRecord*~ m_identityIndex
        -bool m_liveShapingEnabled
        -DispatcherQueue m_ownerQueue
        -DWORD m_ownerThreadId
        -shared_ptr~atomic~bool~~ m_itemChangeDropped
        -SourceChangedHandler m_sourceChanged
        -ItemChangedHandler m_itemChanged
        -bool m_isDispatching
        -bool m_pendingReset
        -vector~SourceRecordPtr~ m_pendingRemoved
        -bool m_outOfSync
        +Create(owner, source)$ shared_ptr~SourceTable~
        +Refresh()
        +Clear()
        +IsDetached() bool
        +Size() uint32
        +At(index) SourceRecord*
        +Records() vector~SourceRecordPtr~
        +TopLevel() vector~SourceRecord*~
        +SetParentKeyHierarchy(key, parentKey)
        +SetChildCollectionHierarchy(children)
        +ClearHierarchy()
        +SetSourceChangedHandler(handler)
        +SetItemChangedHandler(handler)
        +SetLiveShaping(enabled)
        +IsLiveShaping() bool
        +TryFindByRawPointer(item) SourceRecord*
        +TryFindByAddress(address) SourceRecord*
        +TryFindByItem(item) SourceRecord*
        +EnableIdentityIndex()
        +DisableIdentityIndex()
        +HasIdentityIndex() bool
        +TryFindByIdentity(identity) SourceRecord*
        -SubscribeToSourceChanges()
        -UnsubscribeFromSourceChanges()
        -OnSourceCollectionChanged(args)
        -OnSourceVectorChanged(args)
        -OnSourceBindableVectorChanged()
        -Reconcile(items)
        -InsertAt / RemoveAt / ReplaceAt / Move
        -RaiseSourceChanged(kind, oldIndex, newIndex, added, removed)
        -DrainPendingChanges()
        -SubscribeRecordPropertyChanged(record)
        -UnsubscribeRecordPropertyChanged(record)
        -OnRecordPropertyChanged(weakRecord, propertyName)
    }

    class SourceRecord {
        +tracker_ref~IInspectable~ Item
        +ObjectAddress Address
        +event_token PropertyChangedToken
        +WatchState Watch
        +SourceRecord* Parent
        +vector~SourceRecord*~ Children
        +uint16 Depth
        +HasChildren() bool
        -hstring m_identity
        +Identity() hstring
    }

    class WatchState {
        <<enum>>
        Unchecked
        NotObservable
        Subscribed
    }

    class TableViewPredicate2 {
        <<delegate>>
        Boolean(Object item)
    }
    class TableViewKeySelector2 {
        <<delegate>>
        Object(Object item)
    }
    class TableViewComparer2 {
        <<delegate>>
        Int32(Object x, Object y)
    }
    class TableViewChildrenSelector2 {
        <<delegate>>
        Object(Object item)
    }
    class SortDirection2 {
        <<enum>>
        None
        Ascending
        Descending
    }

    TableViewSource2 *-- ShapingManager : unique_ptr (forwards verbs)
    ShapingManager *-- SourceTable : shared_ptr (shareable with consumers)
    SourceTable *-- CollectionAccessor2 : owns source accessor (optional, reset on Clear)
    SourceTable ..> SourceChange : raises (single handler)
    SourceTable "1" *-- "0..*" SourceRecord : sole strong shared_ptr per source position
    SourceRecord --> WatchState
    TableViewSource2 ..> TableViewPredicate2
    TableViewSource2 ..> TableViewKeySelector2
    TableViewSource2 ..> TableViewComparer2
    TableViewSource2 ..> TableViewChildrenSelector2
    TableViewSource2 ..> SortDirection2
```

## 1. Public verb API

`TableViewSource2.idl` declares its own types, so v2 does not depend on v1's IDL:

| Type | Shape | Use |
|---|---|---|
| `TableViewPredicate2` | `Boolean(Object item)` | `Filter` |
| `TableViewKeySelector2` | `Object(Object item)` | `GroupBy`, `Sort`, `ParentBy` |
| `TableViewComparer2` | `Int32(Object x, Object y)`: negative when `x` sorts first, zero when equal, positive when after | `Sort` |
| `TableViewChildrenSelector2` | `Object(Object item)` returning a child collection, or `null` for a leaf | `ChildrenBy` |
| `SortDirection2` | `None`, `Ascending`, `Descending` | `Sort`, `GroupBy` |

Verbs are fluent: each returns the same `TableViewSource2` so calls can be chained.

| Verb | Meaning |
|---|---|
| `From(items)` | Wraps any collection `TableViewSource.From` accepts. Throws `E_INVALIDARG` when `items` is null. |
| `Filter(predicate)` | Keeps only the items the predicate accepts. A second call replaces the first, as in v1. |
| `GroupBy(keySelector, groupDirection)` | Groups by key. `groupDirection` orders the groups by key (`None` = order of first appearance in the filtered source). `Sort` orders only the rows inside each group, never the groups. There is one `GroupBy`; v1's overload taking a group identity selector was removed. |
| `Sort(sortMemberPath, direction)` | Sorts by property path. Projected as `SortByPath`. |
| `Sort(keySelector, direction)` | Sorts by key selector. This is the default overload. |
| `Sort(comparer)` | Sorts by comparing whole items (like WPF's `CustomSort`). The comparer defines the full order of its level, so there is no direction. Projected as `SortWithComparer`. |
| `ParentBy(keySelector, parentKeySelector)` | Hierarchy over a flat source. Each item has a unique key and its parent's key (`null` for a root). |
| `ChildrenBy(childrenSelector)` | Hierarchy over a nested source. Each item returns its child collection. |
| `ClearFilter` / `ClearGroupBy` / `ClearSort` | Remove the matching verb. |
| `ClearHierarchy` | Removes the hierarchy declared by `ParentBy` or `ChildrenBy`. |
| `Refresh()` | Re-reads the source and rebuilds. It is for sources that do not raise change notifications. |
| `IsLiveShaping` | A get/set property, `false` by default. When true, an item's `PropertyChanged` re-applies the active verbs to that item. When false, property edits leave rows in place until the next collection change, verb or `Refresh`. |

Each `Sort` call appends a lower-priority level after the existing ones (`Sort(a).Sort(b)` sorts
by `a`, then `b`), unless it matches an existing level, in which case it replaces that level in
place. A path matches by string; a key selector or comparer matches only when the same delegate
object is passed again. `SortDirection2::None` removes the matching level. `ClearSort` removes
every level. Unlike v1, group order no longer depends on
whether `Sort` was declared before or after `GroupBy`.

## 2. `TableViewSource2` runtime class

- **Not activatable.** It is created only through `From(items)`.
- **A `ReferenceTracker`, like v1.** `final_release` runs on the owning UI thread, and `tracker_ref`
  members let the garbage collector walk references to app objects.
- **Members:**
  - `m_items` is the app collection, held as a `tracker_ref`.
  - `m_shapingManager` is a `unique_ptr<ShapingManager>`, the coordinator (§6). It owns the
    `shared_ptr<SourceTable>`; consumers (the row metadata provider and adapters) may share the
    table past this object's lifetime. `~TableViewSource2()` calls `m_shapingManager->Clear()`, which
    empties the visible list first (see *Teardown order*) and then detaches the table (see *Reference tracking and teardown*), so a lingering table is empty and
    holds no `tracker_ref` bound to the dead owner.
  - Verbs validate their arguments, forward to `m_shapingManager`, and return `*this`.
  - No live-shaping flag. `IsLiveShaping` get/set forward through `m_shapingManager` to
    `SourceTable::IsLiveShaping()` / `SetLiveShaping()`, so `SourceTable::m_liveShapingEnabled` is
    the single source of truth. Implementation note: until `m_shapingManager` exists, the getter
    returns `false` and the setter has nothing to apply; Phase 1.4 constructs it in `From(items)`
    so this window does not occur in practice.
- **`Refresh()`** re-reads the source and rebuilds. It is for sources that do not raise change
  notifications, which the record table cannot otherwise detect.
- **Not included:** `DeferRefresh` (batching verbs into one rebuild) is postponed to a later design
  session; see *Later design sessions: `DeferRefresh`*.

## 3. Source collection resolution: `CollectionAccessor2`

`CollectionAccessor2` (`Source/CollectionAccessor2.h`) is the v2 copy of `ShapingHelpers::CollectionAccessor`.
It probes an `IInspectable` source once at construction for both read (`IVector<IInspectable>` >
`IVectorView<IInspectable>` > `IBindableVector`) and change-notification (`INotifyCollectionChanged`,
`IObservableVector<IInspectable>`, `IBindableObservableVector`) interfaces so all indexed reads,
enumerations, and subscriptions agree by construction. Unlike v1's `CollectionAccessor`, every
interface member is stored in a `tracker_ref` bound to the owning `TableViewSource2`
(`ITrackerHandleManager const*`), matching `InspectingDataSource` so .NET GC can trace cycles through
the collection.

## 4. Source tracking: `SourceRecord` and `SourceTable`

### `SourceRecord`

There is one record per source position. It holds only values that depend on the object itself
and that live exactly as long as the item stays in the source.

| Member | Purpose |
|---|---|
| `Item` | A `tracker_ref<IInspectable>` bound to the owning `TableViewSource2` (matching `Vector<IInspectable>`). Keeps the item alive while in the table while letting the .NET GC collect cycles when an item references its owner. `get_abi(Item.get())` is the raw-pointer lookup key. `Item.get()` is used only on live paths; teardown paths use `Item.safe_get()` (see *Reference tracking and teardown*). |
| `Address` (`ObjectAddress` = `uintptr_t`) | The canonical `IUnknown` pointer: the object's physical identity. It costs one QI, at creation. |
| `Identity()` | The logical identity string (`"0x…"` today). It is formatted on first use and cached; this accessor is the only way to read it. |
| `PropertyChangedToken`, `Watch` | Live-shaping subscription state, revoked in `~SourceRecord()`. `NotObservable` records that an item has no `INotifyPropertyChanged`, so it is not probed again. |
| `Parent`, `Children`, `Depth`, `HasChildren()` | Source-tree links, written only by `SourceTable`. Non-owning; flat sources leave every record top-level (`Parent == nullptr`) with no children. See *Hierarchy: the source tree in `SourceTable`*. |
| `ShapeState Shape` | Engine state stored inline (`Engine/ShapeState.h`): filter flags, cached sort keys, filtered and sorted children, visible-descendant count. Written only by the engine and the flattener. See *Engine state storage*. |

Records are not copyable. `SourceTable` holds the sole strong `shared_ptr<SourceRecord>` (`SourceRecordPtr`)
for each record so that `PropertyChanged` callbacks can capture `weak_ptr<SourceRecord>`
(`SourceRecordWeakPtr`) and call `.lock()` before use (returning `nullptr` once removed, with no
`Retired` flag needed). Callbacks raised off the UI thread are marshalled to the owning
`DispatcherQueue` before any weak pointer is locked (see *Threading* below). Downstream stages hold non-owning `SourceRecord*`
pointers and purge them synchronously from the `SourceChanged` handler before `SourceRecord` is destroyed—ensuring
`Item` stays alive until after all consumers drop the record, preventing COM address reuse (ABA).

### `SourceTable`

`SourceTable` owns the `CollectionAccessor2` and the source-change `event_token` for the input source,
maintaining the source records, addresses, identities, and lookups automatically. It also owns the
per-item `INotifyPropertyChanged` subscriptions used by live shaping, so it is the single place
where any external event (collection or item) enters the pipeline. Output projection lives outside
`SourceTable`.

```text
m_sourceAccessor   resolved read + change-notification interfaces for the bound source
Records()          root-collection records, one per source position, in source order (shared_ptr)
TopLevel()         top-level records of the source tree, in source order (flat: same as Records())
raw lookup         get_abi(item) -> SourceRecord*   always on; pipeline map-back, no QI
address lookup     Address       -> SourceRecord*   always on; duplicate check + COM identity lookup
identity index     Identity      -> SourceRecord*   shaped only; selection, focus, expansion
```

**Public lifecycle & internal synchronization:**

| Operation | Visibility | Behaviour |
|---|---|---|
| `Create(owner, source)` | Public (static) | The only way to build a table. `make_shared<SourceTable>(PrivateToken{}, owner)`, then private `AttachSource(source)`: resolves `CollectionAccessor2(owner, source)`, subscribes to change notifications (storing `m_sourceEventToken`), and performs the initial `Refresh()`. `owner` is the enclosing `TableViewSource2`'s `ITrackerHandleManager const*`. The constructor takes a private `PrivateToken`, so it is reachable by `make_shared` but not by callers; there is no default constructor. |
| `Refresh()` | Public | Re-enumerates `m_sourceAccessor` and reconciles against existing records (initial load, `Reset`, or explicit `Refresh()` for non-observable sources). No-op when detached. |
| `Clear()` | Public | Teardown, called by `~TableViewSource2()` on the UI thread while the owner is alive. Nulls both handlers (raises nothing), unsubscribes from the source and every child collection via `safe_get`, clears the lookups and key index wholesale, destroys the child feeds and their records, then every root record, resets `m_sourceAccessor`, and nulls `m_trackerOwner`. Idempotent. Afterwards the table is detached. |
| `TopLevel()` / `SetParentKeyHierarchy` / `SetChildCollectionHierarchy` / `ClearHierarchy` | Public | Source tree. See *Hierarchy: the source tree in `SourceTable`*. |
| `IsDetached()` | Public | True after `Clear()`. Every public entry point and event lambda returns early when detached. |
| `SetSourceChangedHandler` | Public | Single handler (the coordinator) for normalized, record-level `SourceChange` notifications. See *Source change notification* below. |
| `SetItemChangedHandler` | Public | Single handler (the coordinator) for per-record `PropertyChanged` notifications (`(SourceRecord*, hstring const& propertyName)`). See *Item property change notification* below. |
| `SetLiveShaping` / `IsLiveShaping` | Public | Enables or disables per-record `INotifyPropertyChanged` subscriptions across the table, irrespective of whether any shaping verbs are currently active. |
| `Reconcile` / `InsertAt` / `RemoveAt` / `ReplaceAt` / `Move` | Private | Driven internally by `Refresh()` and `OnSourceCollectionChanged` / `OnSourceVectorChanged` / `OnSourceBindableVectorChanged`. Each ends in `RaiseSourceChanged`. |
| `ValidateItems` / `ValidateIncoming` | Private | Reject null and duplicate items before any mutation. See *Null and duplicate items* below. |
| `RaiseSourceChanged` / `DrainPendingChanges` | Private | Raise (or queue while dispatching) the change, then destroy the removed records; deliver queued changes after the outermost dispatch returns. |
| `SubscribeRecordPropertyChanged` / `UnsubscribeRecordPropertyChanged` / `OnRecordPropertyChanged` | Private | Manage each record's `PropertyChangedToken` and `WatchState`, and forward `PropertyChanged` to `m_itemChanged`. |

**Source change notification:**

`SourceTable` is the only subscriber to the source. Downstream stages never subscribe to the
source themselves: a second subscription could see a change before the table has applied it, so
the table and the rows would disagree. Instead the table reports each change in terms of records.

```cpp
// Add, Remove, Replace, Move, Reset
using SourceChangeKind = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

struct SourceChange
{
    SourceChangeKind Kind;
    SourceRecord* OldParent;                          // Remove, Replace, Move; nullptr = top level
    SourceRecord* NewParent;                          // Add, Replace, Move; nullptr = top level
    uint32_t OldIndex;                                // Remove, Replace, Move; within OldParent's children
    uint32_t NewIndex;                                // Add, Replace, Move; within NewParent's children
    std::vector<SourceRecord*> const& Added;          // Add, Replace (subtree, pre-order); Reset (all new)
    std::vector<SourceRecord const*> const& Removed;  // Remove, Replace (subtree, pre-order); Reset (all gone)
};
```

Flat sources always report null parents, so the indices are source positions. See *Hierarchy: the
source tree in `SourceTable`* for hierarchical sources.

The change kind reuses `NotifyCollectionChangedAction`. Its five values match exactly, so the
table adds no enum of its own. `NotifyCollectionChangedEventArgs` itself is not reused:

- It carries raw items, not `SourceRecord*`, so every consumer would look each item up again.
- Its `Reset` carries no items, but this `Reset` carries what left and what arrived.
- It cannot guarantee that removed records stay readable until the handler returns.
- It is a WinRT runtime class, so every change would cost an activation and two vectors. `SourceChange` is a stack struct.
- It would suggest the source's event is passed through. It isn't: `IVectorChangedEventArgs` (which has no Move) and multi-item, re-entrant or out-of-sync notifications are all converted to this one shape.

| Rule | Detail |
|---|---|
| Ordering | Update `m_records` and the lookups → raise `SourceChanged` → destroy removed records. The handler sees the updated table; removed records' `Item` and `Identity()` stay readable so they can be found among the visible rows. |
| Single, synchronous handler | Raised on the UI thread. Only the coordinator subscribes, and it fans out to engine, projection, and owner (selection, focus, sort-rank cleanup) in a fixed order. A C++ callback, not a WinRT event. |
| `Reset` | Carries what left and what arrived, not moves. Consumers reshape from `Records()` without re-reading the source. `Refresh()` raises `Reset`. |
| Multi-item notifications | Split into single-item changes, or reported as one `Reset`. Downstream only handles the simple cases. |
| Re-entrancy | App code run inside the handler (filter predicate, comparer) may change the source again. The table applies the edit immediately but defers its notification; deferred changes are coalesced into one `Reset` (`m_pendingReset`, `m_pendingRemoved`) and delivered after the outermost dispatch returns. |

**Item property change notification (live shaping):**

`SourceTable` also owns per-record `INotifyPropertyChanged` subscriptions. Subscriptions depend only
on `IsLiveShaping`, **never on which shaping verbs are active**:

| Rule | Detail |
|---|---|
| Verb-independent subscription | `SetLiveShaping(true)` subscribes every record in `m_records`, and `CreateRecord` subscribes newly added records while `m_liveShapingEnabled` is true. Adding or clearing verbs (`Filter`, `Sort`, `GroupBy`, `ParentBy`, `ChildrenBy`) never walks records to subscribe or unsubscribe. |
| `WatchState` caching | First probe sets `WatchState::Subscribed` (storing `PropertyChangedToken`) or `WatchState::NotObservable`. `SetLiveShaping(false)` revokes tokens and resets `Subscribed` → `Unchecked`, but preserves `NotObservable` so re-enabling does not re-QI non-observable items. |
| Automatic teardown | Removing a record (or `Clear()`) destroys the `SourceRecord`, whose destructor revokes `PropertyChangedToken` through `Item.safe_get()` (skipped if the item was already collected) before its `tracker_ref` handle is deleted. |
| Callback contract | Each subscription lambda captures `weak_from_this()` (`weak_ptr<SourceTable>`), `SourceRecordWeakPtr` (`weak_ptr<SourceRecord>`), `m_ownerQueue`, `m_ownerThreadId` and `m_itemChangeDropped`. On the owning thread it locks the table and returns if it has expired; `OnRecordPropertyChanged` then locks the record (dropping stale notifications for removed records) and invokes `m_itemChanged(record, propertyName)`. Off the owning thread it marshals first (see *Threading*). Downstream (the coordinator) decides whether the changed property affects any active verb. |
| Table lifetime | `SourceTable` derives from `enable_shared_from_this` and is always owned by `shared_ptr` (`Create()`). Items belong to the app and can outlive the table; a notification already in flight when `~SourceRecord()` revokes, or a revoke that fails on a torn-down event source, would otherwise call into a freed table. The source-change lambdas (`CollectionChanged` / `VectorChanged`) use the same `weak_from_this()` guard, mirroring v1 `ShapedItemsSource`. Because `weak_from_this()` is empty in the constructor, all subscriptions start in `AttachSource()` / `SetLiveShaping()`, never in the constructor. Weak pointers are locked only on the owning UI thread: a lock on a worker thread could become the last strong reference and run `~SourceTable()` (and its UI-affine revokes) there — v1 `TrackFlatItemPropertyChanged` has the same rule. |
| Empty property name | Per the `INotifyPropertyChanged` convention, an empty or null `PropertyName` means "all properties changed". It is forwarded unchanged; the coordinator treats it as a change to every dependency of the active verbs. |
| Null record | `ItemChanged(nullptr, L"")` means "unknown items changed": an off-thread change was dropped (see *Threading*). The coordinator re-evaluates every record against the active verbs. |
| Reuse keeps subscriptions | `Reconcile` reuses records by raw pointer or address, so a `Reset` or `Refresh()` does not drop and re-create subscriptions for items that are still in the source. |
| Cost trade-off | With `IsLiveShaping` true and no verbs active, every observable item is still subscribed and every `PropertyChanged` is forwarded. This is deliberate: verb changes never walk the table to (un)subscribe, and the coordinator drops events cheaply when no verb reads the property. Apps that do not need live shaping leave `IsLiveShaping` false (the default) and pay nothing. |

**Reference tracking and teardown:**

**Rule: every app-supplied object or delegate held natively goes through `tracker_ref`** bound to
the owning `TableViewSource2`. This covers:
- `CollectionAccessor2` (root and child feeds) and every `SourceRecord::Item`;
- the hierarchy selectors (`m_keySelector`, `m_parentKeySelector`, `m_childrenSelector`);
- `ParentBy` object keys (`NodeKeys::KeyObject`);
- `GroupNode::Key`;
- the filter, sort and group delegates, once the shape spec stores them (Phase 2).

Why: a strong native reference to a .NET object (for example a lambda capturing the page or VM)
roots it, so the cycle back through `TableViewSource2` is invisible to the GC and leaks. v1 holds
its selectors strongly inside `std::function` and has this leak. XAML core does the same for event
handlers (`TrackerPtrFamily.h` wraps listener delegates in `TrackerPtr`), and `TableView` already
holds `tracker_ref<TableViewRowTemplateSelector>`. `tracker_ref` accepts any COM type, delegates
included.

Fields that can outlive their owner are `std::optional<tracker_ref<T>>` so they can be reset while
the owner is alive. Delegates are invoked only on live UI-thread paths, never during teardown.

The source subscription is a raw `event_token`, not `winrt::event_revoker`, which would hold an
untracked strong reference.

| Rule | Detail |
|---|---|
| Cost | Each record costs one `NewTrackerHandle` + `SetTrackerValue` on creation and one `DeleteTrackerHandle` on removal, the same as `Vector<IInspectable>`. A load or `Reset` of *n* items pays *n* of each. |
| Single source token | `SubscribeToSourceChanges` binds exactly one interface (`INotifyCollectionChanged` > `IObservableVector` > `IBindableObservableVector`). `UnsubscribeFromSourceChanges` re-derives it with the same precedence; no kind is stored. |
| No `tracker_ref` outlives the owner | `~tracker_ref` dereferences its owner even when empty. `Clear()` destroys every record, resets `m_sourceAccessor`, the selectors and the key index (all `std::optional` for this reason), and nulls `m_trackerOwner` while the owner is alive. |
| Detached table | After `Clear()` the table is empty and owner-free, so it is safe to outlive `TableViewSource2` (consumer `shared_ptr`s, or a posted off-thread lambda that has the table locked). Every entry point and event lambda checks `IsDetached()` and returns. `~SourceTable()` asserts it was detached and never touches `m_trackerOwner`. |
| GC-collected targets | `tracker_ref` holds no reference count; the owner's tracker handle does. When the GC collects a cycle (item ↔ VM ↔ `TableViewSource2`), items and the collection may already be collected when `final_release` → `~TableViewSource2()` → `Clear()` runs, and `tracker_ref::get()` would return a dangling pointer. So teardown never calls `get()`: handlers are nulled first (no consumer reads `Item` or `Identity()`), lookups are cleared wholesale (`RemoveFromLookups`, which reads `Item.get()`, is for incremental paths only), `~SourceRecord()` revokes via `Item.safe_get()`, and `UnsubscribeFromSourceChanges` uses the accessor's `AsXxx(true)`. Revoke failures are swallowed (`SafeRevoke`). |
| Address reuse | The owner's tracker handle keeps each item alive until the record is destroyed, so a COM address cannot be reused while its record is in the table. The only exception is GC collection of the whole cycle, where nothing remains to observe the reused address. |

**Threading** (same contract as v1 `ShapedItemsSource`):

| Rule | Detail |
|---|---|
| Owning thread | `Create()` must run on the UI thread. It captures `DispatcherQueue::GetForCurrentThread()` into `m_ownerQueue` and `GetCurrentThreadId()` into `m_ownerThreadId`. All table state and both handlers belong to that thread. |
| Source collection changes | Must arrive on the owning thread, the contract every XAML collection consumer imposes. Not marshalled: marshalling would let `m_records` drift from the live collection between the event and its delivery. |
| Item change on the owning thread | Handled inline: lock table → `OnRecordPropertyChanged` → lock record → `m_itemChanged`. |
| Item change off-thread, queue available | `m_ownerQueue.TryEnqueue` posts a lambda holding only the two weak pointers and the property name; they are locked on the owning thread. Order is preserved per raising thread only; a record removed before the post runs fails its lock and the change is dropped (correct, since it left the table). |
| Item change off-thread, cannot marshal | If `TryEnqueue` fails, or there is no queue and the thread differs, the change is dropped and `m_itemChangeDropped` (a `shared_ptr<atomic<bool>>`, so the lambda never locks the table off-thread) is set. |
| Recovery from a dropped change | The next `OnRecordPropertyChanged` on the owning thread does `m_itemChangeDropped->exchange(false)`; if it was set, it first raises `ItemChanged(nullptr, L"")` so the coordinator re-evaluates every record, then delivers the current change. `Refresh()` and `SetLiveShaping(false)` clear the flag, since both already resynchronize or stop tracking. |
| Coalescing | Not done in `SourceTable`. Repeated changes are coalesced per dispatcher turn by the coordinator (Phase 5). |

**Null and duplicate items:**

Rejected in **all modes**, shaped or not. Every lookup key therefore maps to at most one record,
and enabling the first verb needs no validation pass.

| Rule | Detail |
|---|---|
| Behaviour change from v1 | v1 allows null items and the same object on several rows while unshaped (`RebuildUnshapedRows`), and throws only once a verb is active (`ValidateRowIdentities`). v2 throws on attach / change regardless of verbs. Apps binding `[a, a, b]` or lists containing `null` must use distinct objects per row. |
| Validate before mutate | `ValidateItems` (whole materialization, scratch raw-pointer + address sets) and `ValidateIncoming` (one item against `m_rawLookup`, then `m_addressLookup`, ignoring the position being replaced) touch no table state. A rejected change leaves `m_records`, the lookups and subscriptions as they were, and raises no `SourceChanged`. |
| Error | `hresult_invalid_argument`, reusing v1 messages: "The same item object appears in the source more than once. Rows are identified by object identity, so two rows backed by one object cannot be told apart. Use a distinct object per row." / "A row could not be given a stable identity: null item". |
| `Create()` / `Refresh()` | Validated upfront; the throw reaches the caller of `From(items)` or `Refresh()`. |
| Incremental change (`InsertAt` / `ReplaceAt`) | The throw propagates out of the `CollectionChanged` / `VectorChanged` handler, i.e. into the app's `Add()` / `Insert()` / indexer. `ReplaceAt` with the same object at the same index is allowed. |
| Out-of-sync recovery | After a rejected incremental change the source has already diverged from `m_records`. `m_outOfSync` is set and every later notification is handled as a `Reset` (re-enumerate + `ValidateItems`) instead of a positional delta. The first one that validates raises a single `Reset` and clears the flag; `Refresh()` does the same. Positional deltas are never applied on top of a diverged table. |
| `Move` / `RemoveAt` | Cannot introduce null or duplicates; not validated. |

**Lookups:**

| Lookup | Notes |
|---|---|
| `TryFindByRawPointer` | Always on (`m_rawLookup`). Exact `get_abi(item)` hit, no QI. |
| `TryFindByAddress` | Always on (`m_addressLookup`). Canonical `IUnknown` address hit; also backs upfront duplicate-object detection in all modes. |
| `TryFindByItem` | Always O(1): checks `m_rawLookup` first, falling back to one QI + `m_addressLookup` when an item is reached through a different interface pointer. |
| `EnableIdentityIndex` / `DisableIdentityIndex` / `HasIdentityIndex` | Builds `m_identityIndex` in O(n) with no QI when the first shaping verb turns on; drops it when the last verb turns off so unshaped tables keep `Identity()` string formatting lazy. |
| `TryFindByIdentity` | Logical identity lookup (`m_identityIndex`). |

### Rules and differences from the identity spec

| Spec | Here | Why |
|---|---|---|
| D9: types in `TabularShaping` | In `TableViewSource2` | v1 stays untouched. The types can move down a layer when the hierarchy or rank code needs to hold records. |
| D5/D6: `addressSet` for duplicates only | `m_addressLookup` (`ObjectAddress` → `SourceRecord*`) always on | Serves duplicate-object detection, `Reconcile` pass 3, and O(1) `TryFindByItem` fallback in one map; only `m_identityIndex` is toggled on/off with shaping. |
| v1: null / duplicate items allowed while unshaped | Rejected in all modes | Keeps every lookup one-to-one (no multimap, no per-key chains or counters) and removes the mode-dependent validation pass when the first verb turns on. Accepted breaking change. |
| D7/D9: projection stored inside `SourceRecordTable` | Split out of `SourceTable` | `SourceTable` strictly manages input source records, addresses, identities, and lookups; output projection belongs to the downstream stage. |
| D10/D21: co-owned `shared_ptr` + `Retired` flag | Sole strong `shared_ptr` in `SourceTable`, no `Retired` flag | Downstream stages hold `SourceRecord*` and purge synchronously before destruction; `weak_ptr::lock()` alone rejects late async callbacks. |
| Original draft §5.2: `ItemPropertyObserver` owned by the coordinator, subscribing only items read by live-enabled steps | Item subscriptions owned by `SourceTable`, gated only by `IsLiveShaping` | Tokens live on the record and are revoked by `~SourceRecord()`, so subscription lifetime matches record lifetime with no separate tracker. Property filtering by dependency moves to the coordinator's `ItemChanged` handler. |
| v1 `ShapedItemsSource` & `CollectionAccessor`: plain `IInspectable` references to source and items | `tracker_ref` bound to `TableViewSource2` (`ITrackerHandleManager`) for both `CollectionAccessor2` and `SourceRecord::Item`, plus raw `event_token` instead of `winrt::event_revoker` | Matches `InspectingDataSource` and `Vector<IInspectable>`: lets the .NET GC collect cycles when the collection or row items hold back-references to the page/VM/source. `~TableViewSource2()` calls `m_shapingManager->Clear()` (which calls `SourceTable::Clear()`) so tracker handles are deleted while the owner is still alive. |
| `ObjectAddress` type | Alias for `uintptr_t` | This branch has no `ObjectAddress` type. |

### Hierarchy: the source tree in `SourceTable`

With a hierarchy the tree must exist before any other verb runs:

- **Filter** keeps matches plus their ancestors, so it walks parent chains.
- **Sort** orders each sibling set separately.
- **`GroupBy`** buckets the top-level records.

So the tree is part of the source, not of shaping:
`SourceTable (records + source tree) → filter / sort / group over the tree → projection`.
Shaping reads only `TopLevel()` and `SourceRecord::Parent` / `Children` / `Depth`, and does not care
whether `ParentBy` or `ChildrenBy` built the tree.

The tree lives **on `SourceRecord` and in `SourceTable`**; there is no separate tree object. Keeping
it in the table gives one set of lookups, one validation path, one re-entrancy gate and queue, one
`SourceChanged` and one `Clear()`.

**Record fields.** The record carries the *source* tree only; everything shaped lives in the shaped
tree (below).

| Member | Meaning | Who reads it |
|---|---|---|
| `SourceRecord* Parent` | Source parent; `nullptr` for top-level records | Reparent, ancestor walk for filter-with-ancestors |
| `std::vector<SourceRecord*> Children` | Non-owning, source order | Shaped tree reads it directly when no filter or sort is active (no copy) |
| `uint16_t Depth` | Top level = 0; never changed by grouping | Indent = spacer offset + per-level indent × `Depth`; `ShapedDepth()` |
| `HasChildren()` | Derived from `Children` | Source-tree checks only. Not expandability: under a filter it stays true when every child is filtered out; rows use `ShapedRef::ShapedChildCount()` |

Not in the source tree (decided by the shaped tree section): expanded state (`ShapeFlags::Expanded`
on the record's engine slot; the group expansion model keyed by `GroupNode::Identity`), group membership and group level (`GroupNode`), visible flag (presence in the visible
list), shaped depth (`ShapedDepth()` getter). Filtered and sorted children, filter flags, cached keys
and the visible-descendant count live in the engine-owned `ShapeState` slot on the record (see
"Engine state storage").

**`SourceTable` additions** (`Source/SourceTable.h`):

| Member | Purpose |
|---|---|
| `TopLevel()` / `m_topLevel` | Top-level records in source order (non-owning). Flat: mirrors `m_records`. |
| `GetHierarchyKind()` / `HierarchyKind` | `None`, `ParentKey` (`ParentBy`), `ChildCollections` (`ChildrenBy`). |
| `SetParentKeyHierarchy(key, parentKey)` | `ParentBy`. Validates the whole tree and throws before touching any record; then links every record and raises one `Reset`. |
| `SetChildCollectionHierarchy(children)` | `ChildrenBy`. Enumerates every child collection eagerly and recursively, validates the whole tree, then creates child records, subscribes to every observable child collection and raises one `Reset`. |
| `ClearHierarchy()` | Back to flat; resets the selectors; one `Reset`. |
| `SiblingsOf(parent)` (private) | `parent ? parent->Children : m_topLevel`; tree maintenance only. |
| `m_keyIndex`, `m_nodeKeys` (private) | `ParentBy` key index and each record's cached keys. Object keys are held (as `optional<tracker_ref<IInspectable>>`) while the tree is current so their addresses cannot be reused (v1 §3.1). Kept off `SourceRecord` so flat tables pay nothing. |
| `ChildFeed`, `m_childFeeds` (private) | `ChildrenBy`: one feed per record with a non-null child collection. A feed holds a weak `Owner`, an `optional<CollectionAccessor2>`, an `event_token`, the sole strong `SourceRecordPtr`s for that collection in collection order, and an `OutOfSync` flag. |
| Selectors (private) | `m_keySelector`, `m_parentKeySelector`, `m_childrenSelector`; each an `optional<tracker_ref<…>>` (see *Reference tracking and teardown*). |

`Records()` / `Size()` / `At()` stay the root collection (owning, source order). `ChildrenBy` records
are owned by their feed and are not in `Records()`, but they are in the lookups, the identity index,
validation and live shaping. Shaping walks `TopLevel()` and `Children`, never `Records()`.

| Decision | Detail |
|---|---|
| Eager materialization | `ChildrenBy` enumerates every child collection up front, recursively. Filter and sort always cover the whole tree. No load-on-expand. |
| One object, one position | Null and duplicate items are rejected across the whole tree, including child collections. Identity stays the item itself; there is no path-based identity. |
| Last writer wins (assumed) | A new `ParentBy` / `ChildrenBy` replaces the previous tree. |
| Verb change rewrites links | Setting or clearing the hierarchy rewrites `Parent` / `Children` / `Depth` on every record (O(n)) and raises one `Reset`. `Filter`, `Sort` and `GroupBy` never write to records. |
| Ownership per collection | Under `ParentBy` source order differs from tree order, so `m_records` keeps owning in source order and `Parent` / `Children` are non-owning links. Under `ChildrenBy` the child feed owns. |
| Reparent | A `ParentBy` key change moves the record and updates `Depth` for the whole moved subtree. |
| Child collections are source | Their changes are tracked whether or not live shaping is on, through the same normalization, validation, re-entrancy gate and queue as the root collection. A rejected change sets that feed's `OutOfSync`; its next notification rebuilds the subtree as a `Reset` of that subtree. |
| Key and children-property edits | A change to an existing item's key, parent key or children property is seen only through live shaping. `OnRecordPropertyChanged` re-evaluates it (reparent, or swap the feed) and raises the resulting `SourceChange` before forwarding `ItemChanged`. |

**`ParentBy` key rules** follow v1 (`Hierarchical-Data-Rows-Parent-Key-Design.md` §3):

- Value keys compare by value; object keys compare by reference.
- A null or empty parent key, or an orphan (no item has that key), makes a top-level record.
- At the verb (`ParentBy`), a null or empty key, a duplicate key, an item that is its own
  parent, or a cycle throws `hresult_invalid_argument` before any mutation. Cycles are found by
  counting the records reachable from the top level. A selector exception propagates as is (v1
  swallows it into a null key); the whole tree is evaluated before any record is linked, so the
  old tree stays.
- After the verb (source events, live key edits), one bad record never blocks the rest: the
  change is applied with that record excluded or moved, and the exception still propagates (see
  §5, *Misbehaving records*). v1 instead re-validates the whole table on every change, so one bad
  item makes every later change throw until it is fixed.

**`SourceChange` with a tree.** One struct for flat and hierarchical sources. Positions are within the parent's children
(`TopLevel()` when the parent is null), so a flat source reports source positions and a
hierarchical one reports tree positions.

```cpp
struct SourceChange
{
    SourceChangeKind Kind;
    SourceRecord* OldParent;                          // nullptr = top level
    SourceRecord* NewParent;                          // OldParent != NewParent on Move = reparent
    uint32_t OldIndex, NewIndex;                      // within the parent's children
    std::vector<SourceRecord*> const& Added;          // subtree in pre-order; Reset: every record
    std::vector<SourceRecord const*> const& Removed;  // subtree in pre-order; alive for the handler
};
```

**Threading, lifetime and teardown** follow the existing `SourceTable` contracts:

- Child-collection notifications must arrive on the UI thread, like the root collection's.
- Feed lambdas capture `weak_from_this()` plus the owner record's `weak_ptr`, and lock them only on
  the UI thread.
- Feed accessors, the selectors and object keys hold `tracker_ref`s bound to `m_trackerOwner`.
- `Clear()` unsubscribes every feed via `safe_get`, destroys the child feeds and their records
  first, then the root records, then resets `m_sourceAccessor` and nulls `m_trackerOwner`.

**Source root: no sentinel record.** Top-level records have `Parent == nullptr`, and `SourceTable`
keeps `m_topLevel` in source order. Every `SourceRecord` keeps its item invariants (`Address`,
`Identity()`, token, watch), and up-walks stop at `nullptr`.

- `SiblingsOf(parent)` is used only by tree maintenance: insert, remove, reparent, and
  `OldIndex` / `NewIndex` for `SourceChange`. `nullptr` means "top level" in those paths.
- Grouping is unaffected: it buckets `TopLevel()` under the shaped root `GroupNode`. A record's
  group comes from its `ShapeState::Group` (top-level record → bottom-level `GroupNode`, written by
  the engine), never from `record->Parent`; a nested record's group is that of its top-level ancestor.
- The shaped root is always an engine-owned `GroupNode`; there is no source root record.
- `ShapedRef` tags the low pointer bit, so `SourceRecord` and `GroupNode` must have `alignof >= 2`
  (`static_assert` next to `ShapedRef`).
- Rejected: a sentinel `SourceRecord` with no item. It removes the one branch in `SiblingsOf`, but
  moves an `IsRoot()` check into every reader (filter-with-ancestors, find-group-of-ancestor, live
  shaping, validation, lookups, `Records()` / `Size()`), and a missed check treats the root as data.
  It breaks the same invariants cited below for rejecting `GroupRecord`.

**Open (hierarchy):**

- **Orphan adoption (decided).** One source change can produce several tree changes: under
  `ParentBy`, inserting an item can adopt existing orphans, each a Move from the top level to the
  new item. Adopting one orphan is surgical (the insert plus one move); adopting more than one goes
  through `Reapply()` (see *Bulk threshold* in §6 *Change pipeline*).
- **Visible-descendant counts for source parents (decided).** The visible list needs a count of
  visible descendants per node to find a node's flat index and splice only its range (see §6
  *Change pipeline*). `GroupNode` holds it for groups; source
  parents hold it in their `ShapeState` slot (see "Engine state storage").
- **Indent without per-row writes (spacer column).** Applying or removing grouping must not write a
  level to every row. Data rows store only `SourceRecord.Depth`, which never changes with grouping.
  Group-level indent is one shared offset, like Avalonia's `RowGroupSpacerColumn`:
  - `EnsureRowGroupSpacerColumnWidth(groupLevelCount)` sets one width from the cumulative
    `RowGroupSublevelIndents[]`.
  - `EnsureRowGrouping(false)` removes the column.
  - No row is touched in either case.

  Row indent = spacer width (group-level count) + per-level indent × `Depth`. Group headers carry
  their own group level. The work that remains is the O(n) group rebuild, the same as WPF
  `PrepareGroups` and Avalonia. Contrast with AG Grid:
  - Its `resetGrouping` writes `level = 0` and clears the shaped child arrays on every leaf.
  - Its `FlattenStage` calls `setUiLevel` on every visible row on every run.

#### Shaped tree representation (decided: option A, tagged reference)

The shaped tree does not copy the source tree. Only group headers are new nodes; rows are
references to existing `SourceRecord`s.

Declared in `Engine/ShapedTree.h`:

```cpp
// One slot in a shaped child list: a group or a source record (low pointer bit = tag).
struct ShapedRef
{
    static ShapedRef FromRecord(SourceRecord*) noexcept;
    static ShapedRef FromGroup(GroupNode*) noexcept;
    bool IsNull() const noexcept;
    bool IsGroup() const noexcept;
    GroupNode* Group() const noexcept;      // null if this slot holds a record
    SourceRecord* Record() const noexcept;  // null if this slot holds a group
    ShapedRef ShapedParent() const noexcept;     // shaped parent, O(1); null only for the root
    uint32_t ShapedChildCount() const noexcept;  // shaped children, O(1); see below
    bool operator==(ShapedRef const&) const noexcept;  // same tagged pointer (and !=)
private:
    uintptr_t m_bits;
};
// static_assert: alignof(SourceRecord) >= 2, alignof(GroupNode) >= 2, sizeof(ShapedRef) == pointer

struct GroupNode                       // engine-owned; headers exist only while grouping is active
{
    optional<tracker_ref<IInspectable>> Key;  // empty on the shaped root
    hstring Identity;                  // group key path (parent's Identity + this Key); empty on the root
    GroupNode* Parent;                 // nullptr only on the shaped root
    uint16_t GroupLevel;               // top-level groups = 0; the header's ShapedDepth()
    std::vector<ShapedRef> Children;   // subgroups, or top-level records after filter and sort
    uint32_t ItemCount;                // bucketed top-level records (descendants not counted)
    uint32_t VisibleDescendantCount;   // own subtree's rows, ignoring ancestors; 0 when collapsed
    bool IsRoot() const noexcept;      // Parent == nullptr
};
```

- Engine-owned in a container with stable addresses, so `GroupNode*` and `ShapedRef` stay valid
  until the engine drops the group.
- `ItemCount` counts bucketed top-level records only, matching v1 `GroupItemCount`.
- **Bottom level is derived by the engine**, not stored or asked of the node: a group is
  bottom-level when `GroupLevel + 1 == groupLevelCount`; the shaped root is when
  `groupLevelCount == 0`. Inspecting `Children` cannot classify an empty group.
- A `GroupNode` is surfaced to consumers as `TableViewGroupInfo` (Key, ItemCount,
  Level = `GroupLevel`).
- **Record → group link (decided: `ShapeState::Group`, replaces an `m_groupOf` map).** The source
  tree has no group links (a top-level record's `Parent` is null), so a top-level record's only
  upward link into the shaped tree is `GroupNode* Group` in its `ShapeState`: the bottom-level
  group while grouping is active, the shaped root otherwise; null for nested records and for a
  top-level record not in the shaped tree (filtered out).
  - Top-level records only; descendants reach their shaped parent through `SourceRecord::Parent`
    (groups bucket `TopLevel()` only).
  - Written by the engine when it places or moves a top-level record (bucketing already visits each
    one once, so no extra pass; a `GroupBy` change rewrites it to the new group or the root).
  - Needed by the engine: removing a record, or re-bucketing it after `ItemChanged` changed its
    group key, must find the old group when the item is gone or its old key is no longer
    computable.
  - Needed by the flattener: every upward walk (`IndexOf`, `IsShown`, count propagation) crosses
    from a top-level record into its group exactly once. `ShapedRef::ShapedParent()` makes that
    O(1); a map would cost a hash (and an engine-internal dependency) on every walk.
  - A row's owning group is then: header → `GroupNode::Parent`; record → climb `Parent` to the
    top-level record (O(depth)), then `Shape.Group`. Used for group-scoped actions (collapse this
    row's group, sticky header, group selection).
  - Rejected: an engine `unordered_map<SourceRecord*, GroupNode*> m_groupOf` (a hash per walk; the
    View layer would query an engine-internal map), a `ShapedParentOf` query on the sink (a virtual
    call plus a hash per ancestor step), and a flattener-owned copy of the map (two maps that can
    drift).

- **The shaped root is always a `GroupNode`.** Without grouping, `_root` (shaped) holds
  `ShapedRef`s to the filtered and sorted top-level records and no other group exists. There is
  no grouped/ungrouped mode: the same walk handles both.
- **Children of a source parent, after filter and sort:**
  - With no filter and no sort, they are read straight from `SourceRecord::Children` (no copy).
  - Otherwise they live in the parent's `ShapeState` (`vector<SourceRecord*>`), built **eagerly**
    for every parent, expanded or not, and dropped when the verbs are cleared (decided). Lazy
    shaping would make expansion run the comparer (app code that can throw) and leave adds under a
    never-expanded parent without a shaped index. Cost: every child list is sorted on each verb,
    O(n log n) overall, the same order as the top-level sort. Matches *Eager materialization*.
- **Engine state storage (decided: on the record).** Survey: side maps (Qt, JavaFX, Glazed
  Lists, WPF `_internalList`) stay simple only for flat data; with hierarchy they become a second
  tree held in a hash (Qt's per-parent `Mapping` with `mapped_children` and parent links). State on
  the node (AG Grid `RowNode.childrenAfterFilter/AfterSort`, VS Code `visible`/`renderNodeCount`)
  and parallel nodes (WPF `LiveShapingItem`, Avalonia `HierarchicalRow`, Glazed Lists
  `TreeList.Node`) remain. We put the state on the record:
  - Each `SourceRecord` carries one engine-owned `ShapeState Shape` (`Engine/ShapeState.h`). It
    holds `ShapeFlags` (`PassesFilter`, `HasPassingDescendant` so ancestors of matches stay
    visible, `ChildrenShaped`, `SortKeysValid`), `SortKeys` (one cached key per sort level;
    comparer levels hold none), `ShapedChildren` (the filtered and sorted children) and
    `VisibleDescendantCount`. Tree links come from the record's existing `Parent`/`Children`.
    Group membership is the `Group` pointer (top-level records only); the key stays on
    `GroupNode::Key`, so no group key is cached on the record.
  - Why not parallel `ShapedRecord` nodes: they must mirror every add, remove, reparent and
    Reset, and need a record-to-node back-pointer for `ItemChanged`, removal and selection.
    `ShapedRef`, the visible list and identity (record address) stay unchanged with state on the record.
  - What we give up: building a new shaped tree beside the live one and swapping (not needed: a
    failing full apply stages its results in a temporary buffer and moves them into each record's
    `Shape` only at commit, see *Misbehaving records*; the flattener's old visible list gives the
    delta), and several shaped
    views over one source (not a requirement: one `SourceTable` per `TableViewSource2`). If
    several views become a requirement, move to parallel nodes.
  - Keys are cached (unlike Qt/JavaFX/Glazed Lists): path keys go through a binding and key
    selectors cross into app code, so each key is read once per record per level and refreshed
    on `ItemChanged` and on the affected `ShapeDelta`.
  - **Stored inline (decided, for now).** The large parts are vectors that allocate nothing until
    used, so a lazily allocated `unique_ptr` would save only the fixed fields while adding a heap
    allocation per record when shaping starts, a pointer hop and null check on every access, and a
    rule for when to free. `Engine/ShapeState.h` is a leaf header (standard library, WinRT,
    `tracker_ref`), so `Source/` depends on it and not on the engine. Revisit if the fixed size
    grows.
- **Group vs. source node:** `ShapedRef::IsGroup()`.
  - A source parent (`ShapedChildCount() > 0`) is expandable but is not a group.
  - **`ShapedRef::ShapedChildCount()` (decided).** The node's child count in the shaped tree, O(1),
    derived and never stored: `Group()->Children.size()` for a group or the shaped root;
    `Record()->Shape.ShapedChildren.size()` when the record's `ChildrenShaped` flag is set;
    `Record()->Children.size()` otherwise. It is the single rule for `IsExpandable`
    (`ShapedChildCount() > 0`), a record row's `ChildCount`, `SizeOfSet` (on `ShapedParent()`) and the
    flattener's `Expandable` state. `Record()->HasChildren()` is not used for these: it reads the
    source tree, so a parent whose children are all filtered out would show an empty expander.
  - This avoids AG Grid's mix-up, where `rowNode.group` is also true for tree-data parents and
    user rows need a separate `sourceRowIndex >= 0` check.
- **`SourceRecord` and `SourceTable` are unchanged by grouping.** No source-tree link or level
  changes. Applying or removing grouping rebuilds `GroupNode`s (O(n) bucketing) and writes only the
  engine-owned `ShapeState::Group` of each top-level record during that same visit; no shaped
  level is stored anywhere.
- **`ShapedRef::ShapedParent()` (decided).** The node's parent in the shaped tree, O(1), never
  stored: `GroupNode::Parent` for a group, `SourceRecord::Parent` for a nested record,
  `Shape.Group` for a top-level record. Null only for the shaped root. It is the single upward
  step for the flattener's walks, set size, parent identity and "go to parent".

**Visible list (flattener output).** One list for every case: flat, grouped, hierarchical, and
grouped + hierarchical.

```cpp
std::vector<ShapedRef> m_rows;  // Flattener; one entry per visible row: data context, identity,
                                // children, selection target
```

- **Entries are bare `ShapedRef`s (decided).** A `VisibleRow { ShapedRef Ref; }` wrapper was
  dropped: it held one public field, so it hid nothing and added only a name. `GetAt` returns the
  pointer-sized `ShapedRef` by value. Per-row cached fields (depth, sibling index) were already
  rejected because every insert or remove would rewrite them, so no wrapper is needed for them.

- **The shaped parent is not stored (decided).** `ShapedParent()` is O(1) for every kind of
  node, so set size, parent identity and "go to parent" take one path for every row without a
  stored field.
- **The owning group is not stored.** It is found through `Shape.Group` (see "Record → group link");
  only group-scoped actions need it.
- **Set size is not stored.** It is `ShapedParent().ShapedChildCount()`, O(1). Storing it
  would mean rewriting every visible sibling on each insert or remove.
- **Position in set is not stored (decided).** It is found on demand by searching the
  shaped parent's children, O(siblings); only UIA reads it, per query and for realized rows.
  Storing a sibling index would make every insert or remove rewrite all later siblings' entries
  (up to 10k writes per edit under a large group, each locating its row in `m_rows`) and is a
  source of stale values. The parent was briefly kept stored because a top-level record's group
  needed an engine map; with `ShapeState::Group` it is O(1) to derive, so it is not stored either.
  Rejected: storing `Parent` and `IndexInParent` (O(1) `GetRowInfo`, O(siblings) writes per
  edit).
- Only visible rows have entries; a collapsed node's descendants have none.

**Expansion (decided).**
- **Records:** expansion is `ShapeFlags::Expanded` on the record's `ShapeState`. Records survive
  reshapes and `Reapply()`, and `Reconcile` reuses them across `Reset`/`Refresh`, so the flag needs
  no side model. A new record starts collapsed. A record that leaves takes its state with it.
  Confirmed: per-record flag, default collapsed (matches v1 and WCT tree rows). `ExpandAllRows` on
  records is O(n) (one flag write per record), accepted because it publishes a `Reset` anyway.
  Rejected: a baseline-plus-exceptions model by identity like groups (a hash lookup per row, to
  keep state for a record that is removed and re-added), and a configurable default.
- **Groups: an expansion model, for groups only.** `GroupNode`s are rebuilt on every `GroupKey`
  change, so their state is kept outside the node, keyed by `GroupNode::Identity`. Owned by
  `ShapingManager` (`m_groupExpansion`), read and written by the flattener.
- **`GroupExpansion` (decided: new v2 type, `View/GroupExpansion.h`).** Stores only exceptions to
  a baseline (default expanded, as v1), so `ExpandAllGroups`/`CollapseAllGroups` (`SetAll`) are O(1) and a
  group that does not exist yet gets the baseline. Prune operations: `RetainOnly` (full regroup),
  `Erase` (one group dropped, O(1)), `Clear` (new `GroupBy` selector; drops exceptions, keeps the
  baseline). **Decided: `Clear` keeps the baseline**, so a `CollapseAllGroups` is a user preference that
  survives a new selector and the new groups start collapsed. v1 resets to expanded on regroup;
  rejected because it silently undoes an explicit `CollapseAllGroups`. A caller-chosen
  `Clear(bool resetBaseline)` was also rejected (no caller needs the reset). Same scheme as v1 `ShapingHelpers::RowExpansionModel`; rejected: reusing that type
  (with an added `Erase`, or calling `RetainOnly` per commit), to keep v2 independent of v1
  `TabularShaping` and drop the change handler v2 does not use.
- **`GroupNode::Identity`** is the group key path: the parent group's `Identity` concatenated with
  this group's key in string form, top level down. Two groups with the same key under different
  parents differ. It is set when the engine creates the group.
  - **Segment form (decided):** v1 `ValueKey::ToObjectLookupKey(key)`: `value:<typed value>`
    (`value:i32:5`, `value:s:Eng`; the type prefix keeps `5` and `"5"` apart), `object:<IUnknown
    address>` for reference keys, `null:` for a null key. Each segment is length-prefixed so a
    value containing the separator cannot collide.
  - **Reference keys use the address (decided).** Bucketing already groups reference keys by
    `IUnknown` address (as v1), so the address is stable whenever grouping works: a selector
    returning an existing object (`item.Department`) returns the same object on every call, and
    CsWinRT keeps one CCW per managed object. A selector that creates a new object per call
    already gives one group per record, so identity cannot make it worse.
  - **Not the header text.** The header shows `StringifyGroupKey` (`IStringable`, culture-formatted
    `IPropertyValue`, `ICustomPropertyProvider` `Key`, else "(group)"), which collides (every
    `Dept` without a `ToString` override shows its type name) and changes with culture. v1 keeps
    the same split (`StringifyGroupKey` vs `GetGroupKeyIdentity`).
- **Expansion key is the `hstring` `Identity` (decided).** The set is keyed by the same
  `GroupNode::Identity` the repeater keys use, so there is one representation of a group's
  identity. Copies share the reference-counted `HSTRING` buffer, and lookups are per group (one
  per rebuild, one per toggle), never per row, so hashing the full path is negligible. Rejected: a
  64-bit hash (a collision silently gives a group the wrong state, and the node still needs the
  string), a structured segment key (custom hash and equality, duplicate identity logic) and
  `GroupNode*` (nodes are rebuilt on every regroup).
- **Group expansion entries are pruned (decided).** The model holds entries only for groups that
  exist:
  - Full regroup: build the new groups while the old tree still holds its keys, carry over the
    entries whose `Identity` matches a new group, drop the rest, then release the old tree.
    Holding the old keys until the swap means a freed key address cannot be reused mid-rebuild,
    which closes the address-reuse hazard.
  - Incremental: a group that empties and is dropped erases its entry.
  - `GroupBy` with a new selector clears every entry (old identities mean other keys).
  - Consequence: a group that disappears and returns (a filter hides all its rows, or live
    shaping empties it) comes back in the default state.
  - Rejected: keep entries past their group with a `weak_ref` to object keys (state survives a
    group vanishing and returning), because entries for value keys never expire and need a cap,
    keys without `IWeakReferenceSource` need a fallback, and it adds cleanup. Can be added later
    without changing `Identity`.
- **`VisibleDescendantCount`** (records and groups): the rows a node's own subtree contributes
  below it, given its own and its descendants' expansion and ignoring its ancestors'. 0 when the
  node is collapsed. A hidden node keeps its count, so toggling it updates counts only up to the
  first collapsed ancestor and publishes nothing, and re-showing an ancestor costs one read.
- **Expansion never involves the engine.** Child lists are shaped eagerly, so expand and collapse
  are flattener-only: they cannot run app code or throw.

**Visible-list entry vs. `TableViewRowInfo`.** They are not the same thing:
- The stored entry of the visible list is one pointer-sized `ShapedRef` into the shaped
  tree, kept per visible row and internal to the flattener and the visible list.
- `TableViewRowInfo` is the consumer-facing metadata snapshot returned by
  `ITableViewRowMetadataProvider::GetRowInfo(index)`. The v2 provider builds it on demand, in O(1),
  from the `ShapedRef` at that index (`Ref` below); it is never stored per row (storing it would duplicate state
  and bring back per-row writes when grouping or expansion changes).

| `TableViewRowInfo` field | Derived from |
|---|---|
| Kind | `Ref.IsGroup()` |
| Level | `ShapedDepth()` |
| IsExpandable | `Ref.ShapedChildCount() > 0` (always true for a group header: empty groups are dropped) |
| IsExpanded | record: `ShapeFlags::Expanded`; group: expansion model, by `GroupNode::Identity` |
| ChildCount | `GroupNode::ItemCount`, or `Ref.ShapedChildCount()` for a record |
| ParentIdentity | `Ref.ShapedParent().Record()->Identity()` when the shaped parent is a record; empty otherwise |
| PositionInSet | index of `Ref` among `Ref.ShapedParent()`'s shaped children + 1 (searched on demand, O(siblings)) |
| SizeOfSet | `Ref.ShapedParent().ShapedChildCount()` |

Everything else is read through `Ref`:
- Kind comes from the tag.
- `Depth` and expandability come from the record or group.
- Expanded comes from the record's `ShapeFlags::Expanded`, or the group expansion model by `GroupNode::Identity`.
- Indent = spacer offset (group-level count) + per-level indent × `Depth`.
- `ShapedDepth()` (getter, never stored) = depth in the shaped tree, 0-based below the shaped root:
  - Group header: `GroupNode::GroupLevel` (top-level groups = 0).
  - Row: `groupLevelCount + record->Depth`. Every row sits under a bottom-level group, so the group
    part is the same for all rows; ungrouped, `groupLevelCount` is 0 and this equals `Depth`.
  - O(1), computed from the flattener's active group-level count. Not stored on `SourceRecord`,
    because it changes when grouping changes and that would bring back per-row writes.
  - Used by the UIA `Level` property (`ShapedDepth() + 1`) and by keyboard navigation to the parent
    or child. Indentation does not need it: it uses spacer width + `Depth`.
- Each node keeps a count of its visible descendants, so a node's position in the visible list is
  found by summing the spans of its earlier siblings at each level, O(depth × siblings), letting
  changes splice only their range instead of rebuilding (see §6 *Change pipeline*).

**Effect on downstream code.** `ShapedRef` and `GroupNode` stay native: the engine, the
flattener, the WinRT visible list and the row metadata provider. `TableView` sees one list of
items plus `TableViewRowInfo` / `TableViewGroupInfo`, and branches on group vs. row, never on whether
grouping is on:
- Container and template choice, UIA peer: header or row.
- Selection, editing, copy: skip headers.
- Keyboard: walks the same list.

The v1 `IsTableViewSourceGrouped()` branches, and the O(n) scan in
`GetDataRowPositionInSetInternal`, go away.

**Difference from the v1 flat list:**

| | v1 | v2 |
|---|---|---|
| Visible-list types | Five, by `ProjectionKind` (`m_rows`, plus the grouped and hierarchical adapters' `Entries()`) | One `ShapedRef` list |
| Duplicate state | `m_rows` is kept up to date even when grouped or hierarchical, though not displayed | Shaped tree plus visible list only |
| Row metadata | A provider per kind; some values found by scanning | Read through the `ShapedRef`, built on demand |
| Source change when grouped or hierarchical | Full `Refresh()`: re-read the source, new provider, possibly a new view object | Shaped tree is updated; only the affected range is spliced |
| Object the repeater is bound to | Can be swapped on `Refresh` | Same object for the control's lifetime |

**Alternatives rejected:**
- **Mirror every row with a shaped node.** n extra nodes, rebuilt on every filter or sort, and
  stale after records are recreated by Replace or Refresh. AG Grid is close to this: it stores
  `childrenAfterFilter` and `childrenAfterSort` on every `RowNode`, so its `resetGrouping` must
  clear every leaf.
- **`GroupRecord : SourceRecord` under a separate `_groupRoot`** (AG Grid style).
  - Compile-time safety is lost: selection, editing, live shaping and `SourceChange` could be
    handed a group as an item.
  - `SourceTable` invariants break: groups have no `Address`, `Identity()`,
    `PropertyChangedToken` or `Watch`.
  - Polymorphism adds a vtable pointer to every record.
  - A row → group link needs either rewriting `Parent` (corrupting the source tree) or a
    `ShapedParent` written on every row when grouping changes.
- **Shared `ShapedNodeBase { Kind }` base for `SourceRecord` and `GroupNode`.** Viable, but adds
  1 byte plus padding to every record for no gain over the tag bit.

## 5. Shape spec

The shape spec is the stored set of verbs currently in force. The engine, the projection and the
coordinator all read it. It is split in two (`Spec/`):

- **`ShapeSpec`** (`Spec/ShapeSpec.h`): plain data saying *what* is applied. No COM references, so
  it needs no `tracker_ref`, is copyable and comparable with `==`, and paths stay readable strings.

  ```cpp
  enum class DelegateId : uint32_t { None = 0 };          // plain value; does not keep the delegate alive
  struct DelegateRef { DelegateId Id; uint32_t Version; }; // new Version on every Register call

  struct FilterSpec { DelegateRef Predicate; };
  struct SortSpec   { Kind SortKind /* Path | Key | Comparer */; hstring Path; DelegateRef Delegate;
                      SortDirection2 Direction /* None for Comparer */; bool Matches(SortSpec const&) const; };
  struct GroupSpec  { DelegateRef Key; SortDirection2 Direction; };
  struct ShapeSpec  { vector<FilterSpec> Filters; optional<GroupSpec> Group; vector<SortSpec> Sorts; };

  enum class ShapeDelta { None, Sort, GroupOrder, GroupKey, Filter };   // flags
  ShapeDelta Diff(ShapeSpec const& before, ShapeSpec const& after);  // pure; == means no work
  ```

- **`ShapeDelegates`** (`Spec/ShapeDelegates.h`): the registry holding *how*: the app's
  predicates, key selectors and comparers (`tracker_ref`, optional, destroyed by `Clear()` while
  the owner is alive), deduplicated by `IUnknown` address so the same delegate object always gets
  the same `DelegateId`, and every `Register` call returns a new `Version`. `Retain(specsInUse)`
  releases delegates no spec in use refers to. UI-thread affine.
- **`PathEvaluator`** (`Spec/PathEvaluator.h`): reads the keys of `Path` sort levels. A path is a
  string, not a delegate, so it is kept out of `ShapeDelegates`. Uses v1's `SortMemberPathResolver`
  (a one-time `{Binding}`), so v1 and v2 agree on what a path means; one resolver cached per path,
  and `Retain(specsInUse)` drops resolvers for paths no spec uses. Holds no app objects between
  calls, so no `tracker_ref`. UI-thread affine (drives the binding engine), so path sorts read
  their keys on the UI thread.

| `ShapeDelta` | Engine work |
|---|---|
| `None` | Nothing |
| `Sort` | Re-sort rows inside each group |
| `GroupOrder` | Reorder groups only |
| `GroupKey` | Re-bucket into groups, then re-sort |
| `Filter` | Re-filter from `SourceTable` (no source re-read), then regroup and re-sort |

Compared with the original redesign (`ShapeWork` + `ShapePlan`, Original §4.4): `Sort`,
`GroupOrder`, `GroupKey` and `Filter` match `ReorderSiblings`, `ReorderGroups`, `Regroup` and
`Refilter`. There is no `Rehierarchy` (hierarchy is a `SourceTable` Reset) and no `Full` (no first
shape or `KeyPolicy` in the spec). `ShapeDelta` is flags rather than one "most expensive" value,
so independent changes such as group order plus row sort combine. `ShapePlan`'s key-cache
invalidation flags are added only if the engine caches per-row keys. The per-entry `Version`
follows the original's `StepVersion`.

Decisions and open questions:

- **Sort levels (decided):** each `Sort` call appends a lower-priority level.
- **Repeated sort (decided, same rules as v1):** a `Sort` call that matches an existing level
  replaces it in place (keeping its priority) instead of appending.
  - **Path:** matched by string, so `Sort("Name", Asc)` then `Sort("Name", Desc)` flips the
    direction. v1 does the same with its `"path:"` axis token (`SortAxisTokenForPath`).
  - **Key selector / comparer:** matched by delegate object (`IUnknown` pointer). Passing the same
    object again replaces; a new lambda, even with identical code, is a different object and
    appends. Apps that want to change a key level's direction keep the delegate in a field. The
    `ShapeDelegates` registry dedupes by address, so a level matches on `(kind, path or DelegateId)`.
  - **`None`:** removes the matching level (path or key); with no match it is a no-op. A comparer
    level has no direction, so only `ClearSort` removes it.
  - **Why not always append:** with two different lambdas for the same key, the second level never
    decides anything (the first has already ordered every pair it can), so a direction flip
    would silently do nothing.
  - `TableView` column headers will not depend on delegate identity: like v1's internal
    `SortReplacing` with `"column:<address>"` tokens, they will match by column.
- **Path and key levels stay separate (decided):** a path level stores the property path as a
  string, a key level stores the `TableViewKeySelector2`, and a comparer level stores the
  `TableViewComparer2`. Keeping the path as data lets column headers and saved settings read it.
  Inside the engine, path and key levels both produce a key per row and share one comparison.
- **Sort comparer (decided):** `Sort(comparer)` adds a level that compares whole items with a
  `TableViewComparer2`. It is one level like any other, so it can be mixed with path and key
  levels (`Sort(path).Sort(comparer)` breaks path ties with the comparer). It takes no direction;
  an app wanting the reverse order inverts its comparer. Unlike WPF's `CustomSort`, it does not
  replace the other levels. Key levels keep the engine's default key comparison.
- **Sort and group ordering (decided):** explicit. `GroupBy`'s `groupDirection` orders groups by
  key; the `Sort` levels order only rows within groups. v1's declaration-order rule is dropped.
  - **Logical order:** filter → group → sort within each group, as in TanStack Table, MUI X and
    AG Grid.
  - **Filter first:** groups are built only from passing rows, so a group never appears empty.
  - **Group order:** `Ascending` / `Descending` order groups by key. `None` keeps the order in
    which groups first appear in the filtered source.
  - **Row sort never moves groups:** changing `Sort` reorders rows inside groups but never group
    positions (AG Grid's `groupMaintainOrder` behaviour). WPF and Avalonia differ: they sort
    before grouping, so with no group sort the groups follow their first row in the sorted
    list, and they swap when the sort changes. We don't adopt that.
  - **Engine freedom:** the engine may physically sort before bucketing for efficiency, as long
    as the result matches this logical order.
- **Filter (decided):** the spec holds a list of predicates combined with AND, evaluated in order
  and stopping at the first rejection; a throwing predicate rejects the item. This matches v1's
  engine (`ShapingPipeline::m_filters`, `PassesFilter`). The public API keeps v1's behaviour: a
  `Filter` call replaces the existing filter, and `ClearFilter` removes it, so today the list
  holds at most one entry. The list leaves room for per-column filters later (v1's engine keys
  them by an axis token that no public verb exposes).
- **Hierarchy:** `ParentBy` and `ChildrenBy` are mutually exclusive. Assumed: the last call replaces
  the earlier one (see *Hierarchy: the source tree in `SourceTable`*). The tree itself lives in
  `SourceTable` and on `SourceRecord`, not in the spec. **Decided:** the selectors stay in
  `SourceTable` too, so a hierarchy change never produces a `ShapeDelta`; `SourceTable` raises a
  `Reset` `SourceChange` and the coordinator rebuilds from it.
- **Re-declaration (decided, same as v1):** calling a verb again with the same delegate object
  (filter, key selector, comparer or group key), even with the same direction, counts as a change
  to that verb. A delegate can read app state that has moved since (for example, a filter
  comparing against a `_minAge` field), and the delegate cannot reveal that. v1 does the same
  (`ShapingPipeline::SetFilter` re-mints the description id on every declaration). Recorded in
  the spec itself: every `DelegateRef` carries a `Version` that `Register` bumps on each call
  (like the original redesign's `StepVersion`), so `Diff` stays a pure comparison of two specs and
  nothing is lost when intermediate specs are dropped.
  - Only that verb's step re-runs: re-declaring `Filter` re-filters from `SourceTable`; it does
    not re-read the source like `Refresh()`.
  - Re-declaring a path sort with the same direction is a no-op: a path reads only the item's own
    property, which live shaping already tracks.
- **Diffing:** whether the spec is a comparable value, so the engine can pick the cheapest update.
  **Decided:** yes. Because re-declarations bump a `Version`, `==` means no work.
- **Spec ownership, mutability and application (decided, WPF model).** Survey: WPF, Avalonia,
  UWP and the Community Toolkit keep one spec edited in place and defer only the recompute
  (`_deferLevel` counter, one refresh at the outermost end); TanStack/MUI replace caller-owned
  state and memoize derived row models; DynamicData is latest-wins with no rollback; only JavaFX
  `TableView.sort` rolls back, and only for a returned failure. No framework keeps a separate
  pending copy or rolls back on an exception. Decision:
  - **Desired spec in the coordinator, applied spec in the engine.** Verbs edit the desired spec
    held by `ShapingManager` (§6) in place. The engine keeps a copy of the last applied spec,
    which `Diff(applied, desired)` needs anyway. There is never more than one desired spec, so intermediate states are never applied.
  - **When it is applied.** Synchronously after each verb. Batching several verbs into one apply
    (`DeferRefresh`) is postponed; see *Later design sessions: `DeferRefresh`*.
  - **Apply can fail, but never leaves shaped state half-written.** An exception from app code
    propagates; every call into app code runs before any write, so a failed apply leaves the
    shaped state as it was. See *Misbehaving records* below. Invalid arguments are rejected
    synchronously at the verb call.
  - **Re-entrancy.** A verb (or `Refresh`) called while apply runs, for example
    from inside a delegate, throws `InvalidOperationException`, like WPF's edit guards.
  - **Readers.** Properties that describe the spec itself (the declared sort levels,
    `IsGrouped`) read the desired spec, as WPF's `SortDescriptions` do. Sort indicators in column
    headers read the applied spec, so the arrow never claims an order the rows do not have; this
    follows Avalonia TreeDataGrid, which sets `SortDirection` only after the sort succeeds. Rows,
    groups and row metadata reflect only the applied spec. Source changes arriving inside a batch
    are shaped with the applied spec.
  - `DelegateId` stays a plain value; `ShapeDelegates`/`PathEvaluator` `Retain` the desired and
    applied specs after each apply, so a delegate the applied spec uses is never released early.
- **Misbehaving records (decided, except where marked open).** Survey: WPF, Avalonia, the
  Community Toolkit, AG Grid callbacks, TanStack/MUI and JavaFX let delegate exceptions propagate
  and never roll back. They differ in what state the throw leaves behind:
  - The incremental add is safe in all of them, because the filter runs before the insert.
  - WPF's full refresh builds a new list and swaps it in, so a throw leaves the old rows.
  - The Toolkit's full refresh clears and refills in place, and WPF does the same with live
    shaping. A throw leaves a partial view and no `Reset`, so the control goes out of sync.
  - Avalonia inserts into the flat list before computing the group key. It also clears its
    grouping flag before the refresh `try`, and a range add that throws keeps the items before
    the failure and loses the rest.
  - WPF rejects range notifications outright (`NotSupportedException`).

  Rule: **propagate, and compute before commit**:
  - **Filter predicate throws:** the exception propagates; the record is treated as not passing
    (nothing is written for it).
  - **Key selector or comparer throws:** the exception propagates.
  - **Path cannot be evaluated:** the key is `null`, no throw (WPF, v1 `SortMemberPathResolver`).
  - **Null-key ordering (decided).** `null` compares lower than every non-null key, and
    descending negates the whole comparison, so nulls come first ascending and last descending.
    The same rule orders groups, so the null-key group moves with `GroupDirection`. This matches
    WPF (`Comparer.Default`, `PropertyGroupDescription` name comparers), Avalonia, the Toolkit,
    AG Grid and MUI X; WinForms `DataGridView` is the inverse and TanStack places `undefined` only
    when `sortUndefined` is set. A per-level override is not provided.
  - **Comparer inconsistent (not a strict weak ordering):** must not be undefined behaviour; sort
    with an algorithm that is safe for any comparer. The resulting order is unspecified.
  - **Compute, then commit, per unit of work.** Every call into app code for the unit runs before
    any write: filter, then keys, then group key, then positions by binary search, which calls
    the comparer. Only then are `Shape`, `ShapedChildren`, counts, groups and the flat delta
    written. The commit phase calls no app code, so it cannot fail.
  - **Full re-apply** (`Reapply()`, §6; a verb or `Reset`): evaluate every record into
    a staging buffer, build new `ShapedChildren` (sorted as copies) and a new group tree, then
    commit by moving the staged state into each record's `Shape`. A throw discards the buffer.
    The old rows stay, nothing is published, applied stays old while desired is new, and the next
    verb or `Refresh` retries. **The desired spec is not reverted (decided).** No surveyed
    framework reverts: WPF, Avalonia and the Toolkit write `Filter`/`SortDescriptions` before
    applying and leave them set, so the next `Refresh` retries; TanStack leaves `sorting` set but
    its memo stores the new inputs before computing, so it does not retry until state changes.
    Our engine always retries on the next verb or `Refresh`, because applied stays old.
  - **Range notifications are all-or-nothing (decided, for now).** For a range event (add,
    remove, move or replace of several items), app code runs for all of the records first. A
    throw anywhere discards the shaped part of the whole event, and the exception propagates
    into the source's raiser. The source and `SourceTable` already hold the change and cannot be
    undone, so the new records exist but behave as filtered out until something re-evaluates them
    (`ItemChanged`, `Refresh`, the next verb). The alternative was to skip the bad record, commit
    the rest and rethrow. No collection-view framework does either deliberately.
  - Where the exception surfaces: the verb call; the app's own `Add`/`Insert` for source events (as in WPF); the property setter for live shaping.
    A delegate error can surface later than the verb that declared it. Keys are computed only for
    records placed into `ShapedChildren`, so `Sort` succeeding does not prove the comparer or key
    selector works for every record. A record that starts passing later (removing or relaxing the
    filter, `ItemChanged`, an add) gets its keys then, and a throw surfaces from that operation.
    WPF behaves the same: `PrepareLocalArray` sorts only the items that pass the filter, so
    setting `Filter = null` is where a bad comparer first throws.
  - **Group key selector throws or returns an unusable key (decided).** The record goes into the
    null-key group, the same group that holds records whose key is `null`; a throw and a `null`
    key are not distinguished. The exception does not escape. The null-key group is an ordinary
    group, not pinned first or last: like WPF (`CollectionViewGroupRoot.AddToSubgroup` creates a
    group with `Name == null` and inserts it like any other), its position comes from the group
    ordering under `GroupDirection`. Where `null` orders relative to other keys follows the
    null-key ordering rule shared with sort (`null` lowest, flips with direction). This deviates from WPF only for throws: WPF's
    `GroupNameFromItem` lets a converter exception escape, while a path it cannot evaluate already
    yields a null-like name.
  - **Hierarchy selectors (decided).** At the verb, the rules in
    *`ParentBy` key rules* apply: invalid data throws before any change, and a selector
    exception propagates with the old tree kept. After the verb, a bad record is placed as below
    and the exception propagates from the app's `Add`/`Insert` (source events) or the property
    setter (live edits), so a single record never freezes the table:

    | Case (after the verb) | Placement |
    |---|---|
    | `ParentBy` duplicate key (add or key edit) | The record that already holds the key keeps it; the newcomer is excluded. |
    | `ParentBy` null or empty key | Excluded. |
    | `ParentBy` parent-key selector throws | Top level, like an orphan. |
    | `ParentBy` edit making a cycle or a self-parent | The edited record moves to the top level with its subtree; it moves to its proper parent once the key becomes valid. |
    | `ChildrenBy` children selector throws | Treated as a leaf (no child feed). |
    | `ChildrenBy` item already elsewhere in the tree (including an ancestor, or twice in one collection) | The *Null and duplicate items* rule applies: the change is rejected (nothing written), and that feed's `OutOfSync` is set. Every later notification on that collection is handled as a `Reset` of its subtree and re-validated, so the collection stops updating until the app removes the duplicate. The root and other feeds stay live. Rejected alternative: skip only the duplicate edge, which keeps the rest of the collection live but needs a source-to-feed position map per feed and rules for promoting a skipped item when the original leaves, and gives flat and hierarchical sources two rules for one violation. |

    Excluded records behave as filtered out and are re-evaluated on `ItemChanged`, `Refresh` or
    the next hierarchy verb. Under `ParentBy` a record has one parent key, so it is linked under
    one parent only; a record can never appear twice. The top-level placement of an orphan or a
    cycle-breaking record is the only place the tree disagrees with the item's key. Precedent: AG
    Grid moves orphans and cycles to the root with a warning; TanStack, Avalonia TreeDataGrid and
    MUI X propagate selector exceptions.
  - **Reporting:** nothing beyond the propagated exception (no framework reports separately).
- **Path evaluation: revisit `PathEvaluator` and `SortMemberPathResolver` (open).** The current
  declaration reuses v1's resolver (a hidden `ContentControl` with a `OneTime {Binding}`, one
  per path) so v1 and v2 agree on what a path means. Questions to settle:
  - Keep the binding-based resolver, or use a cheaper evaluator (for example reflection through
    `ICustomPropertyProvider`, or a cached property getter) and accept differences from cell
    bindings?
  - The binding engine ties path evaluation to the UI thread; does that fit the engine's threading
    model, or must path keys be extracted on the UI thread before off-thread work?
  - Cost: each read is a `DataContext` write plus a binding evaluation, so keys must be read once
    per record per level into a key cache (engine phase), not per comparison.
  - Errors: the resolver swallows exceptions and returns `null`; confirm against the engine's error
    rule.
  - Live shaping: a dotted path changes when a nested object raises `PropertyChanged`, which the
    row item does not report. Re-read every path key on `ItemChanged`, or match the first segment
    against the property name?
  - Ownership: should `PathEvaluator` be shared with v1 / the column-header sort path, and who
    calls `Retain` (after each apply, per the ownership decision above)?

## 6. Coordinator: `ShapingManager`

`Shaping/ShapingManager.h`. A native C++ class that owns every shaping part behind
`TableViewSource2` and wires them together.

- **Decision: a native coordinator, named `ShapingManager` (decided).** Options considered:
  - A. `TableViewSource2` owns and wires everything. Rejected: the WinRT class would hold the
    verbs, the event handling, the pipeline and the metadata provider, which is v1's god object.
  - B. The visible list (the WinRT collection the repeater binds to) owns `SourceTable`, the
    engine and the flattener, as v1 `ShapedItemsSource` does. Rejected: the same god object one
    layer down, and a view that shapes data.
  - C. A separate native coordinator. Chosen.
  - D. `TableViewSource2` owns `SourceTable`; `ShapingManager` holds a reference and owns the
    rest. Rejected (revisited): source and shaping would be peer layers, hierarchy verbs would
    skip forwarding, and lazy shaping would become possible, but it needs build and teardown
    ordering across two owners, makes "only subscriber" a convention, splits the re-entrancy
    guard between `TableViewSource2` and the manager, leaves a stale reference on source swap,
    and blurs `shared_ptr` ownership. Worth it only if `SourceTable` must outlive or predate
    shaping; in v2 one source means one table and one manager, cleared together.
    A read-only `ShapingManager::Source()` accessor (returning the `shared_ptr`) would give
    non-shaping consumers the table without moving ownership. Deferred until a consumer needs it.
- **Name: `ShapingManager` (decided, for now).** Follows ItemsRepeater's internal native helpers
  (`ElementManager`, `ViewManager`, `ViewportManager`): plain C++, owned by the control, built with
  the owner's `ITrackerHandleManager`. Rejected: `ShapedItemsSource2` (reads as v1's collection,
  which this is not), `ShapingCoordinator`, `TableViewModel` (TreeView's `ViewModel` is itself the
  bound vector, and the name clashes with app MVVM code), `TableCollectionView` (WPF's
  `CollectionView` is the bound view; this hands one out through `View()`). The visible list in
  `stash@{0}` (`View/ShapedItemsSource2.h`) can keep its name.
- **Ownership.**

  | Owner | Owns |
  |---|---|
  | `TableViewSource2` | `m_items` and `std::unique_ptr<ShapingManager>`. Validates verb arguments and forwards; returns itself for chaining. |
  | `ShapingManager` | `SourceTable` (`shared_ptr`), the desired `ShapeSpec`, `ShapeDelegates`, `PathEvaluator`, the cached `ItemsSourceView` (`m_view`), the group expansion model (`m_groupExpansion`, a `GroupExpansion`), the `Flattener` (`m_flattener`). Later: the engine (applied spec, shaped tree) and the WinRT visible list. |

- **Pipeline.** Spec verbs edit `m_desired`, then `ApplyDesiredSpec()` runs
  `Diff(applied, desired)` → engine apply → flat delta → publish to the visible list, then `Retain()`s
  the delegates and paths both specs use. Hierarchy verbs forward to `SourceTable`, whose
  `Reset` comes back through `OnSourceChanged`. How the engine and the flattener exchange changes
  is in *Change pipeline* below.
- **`Refresh` vs `Reapply` (decided).** Two distinct names so `Refresh` always means "read the
  source":
  - `Refresh()` (public) is the only path that re-enumerates the root source on request. It calls
    `SourceTable::Refresh()`, whose `Reset` comes back through `OnSourceChanged` and runs
    `Reapply()` once.
  - `Reapply()` (private) rebuilds the shaped tree from `SourceTable::Records()` with the desired
    spec and never reads the source. It runs for a `SourceTable` `Reset`, a `Diff` that falls back
    to a full rebuild, and dropped-`ItemChanged` recovery. Verbs, live shaping and incremental
    source changes never re-read the source either; unlike v1, nothing internal calls `Refresh`.
  - `SourceTable` still enumerates without a `Refresh()` call in three cases: the initial load in
    `Create()`, a `Reset` raised by the source, and out-of-sync recovery. `ChildrenBy` enumerates
    the child collections it attaches, not the root.
- **Only subscriber.** `ShapingManager` is the single `SourceChanged` and `ItemChanged`
  handler, and fans out to the engine, the visible list and the owner in a fixed order.
- **Not the WinRT collection.** Unlike v1 `ShapedItemsSource`, it is not the collection the
  repeater binds to. That collection is the visible list, not declared yet.
- **`View()` returns an `ItemsSourceView`** wrapping the visible list, as v1
  `TableViewSource::GetItemsSourceView()` does. It is created on first call and cached in
  `m_view` (`std::optional<tracker_ref<ItemsSourceView>>`, reset by `Clear()`), and stays the same
  object for the coordinator's lifetime: reshapes raise `CollectionChanged` on it rather than
  replacing it. Returns `nullptr` after `Clear()`.
- **Row metadata and selection.** `TableView` talks only to `TableViewSource2`, and through it to
  the coordinator, never to `SourceTable`. Identity lookups go through `SourceTable` and index
  lookups through the flattener, both owned here.
- **Reference tracking.** Holds no app object itself; `SourceTable`, `ShapeDelegates` and `m_view`
  bind their `tracker_ref`s to `TableViewSource2`. `~TableViewSource2()` calls `Clear()` while it is
  still alive.
- **Re-entrancy.** `m_isApplying`: a verb or `Refresh` while an apply runs throws (§5).
- **Testability.** Plain C++, so it can be tested without WinRT activation.

### Change pipeline: engine → flattener → visible list (decided: hybrid)

How a change to the shaped tree becomes a change to the visible list. Two paths: a **bulk** path
for changes that reshape everything, and a **surgical** path for local changes, so a single add,
remove, reparent or expand does not reset the list.

- **Options considered.**
  - A. Tree deltas everywhere: every change, including a regroup, is a list of node edits.
    Rejected: a regroup or a new sort touches every node, so the edit list is O(n) anyway and much
    harder to produce correctly than a rebuild.
  - B. Re-flatten and diff the old and new visible lists. Rejected: O(n) per change, even for one
    add, and a list diff cannot tell a move from a remove plus an insert.
  - C. Hybrid. Chosen. Bulk changes rebuild the visible list and publish a **stable `Reset`**; local
    changes are reported as `ShapedEdit`s and spliced.
- **Rejected: bulk only for the first version.** Simpler, but it regresses v1, which splices a
  single-item add, remove or replace on a flat unfiltered source (sorted or not) and splices a
  single-group expand or collapse (`GroupedSourceAdapter`). Bulk-only v2 would publish a `Reset` for
  all of those.

**Which path a change takes.**

| Change | Path |
|---|---|
| Spec verb that reshapes (`Filter`, `GroupBy`, `ClearGroupBy`, a `Sort` change), a hierarchy verb, a `SourceTable` `Reset`/`Refresh`, out-of-sync recovery, `ItemChanged(nullptr)`, a retry after a failed apply | Bulk: `Reapply()` → `Rebuild` |
| `ExpandAllGroups` / `CollapseAllGroups` / `ExpandAllRows` / `CollapseAllRows` | Bulk: `Rebuild` only (no engine) |
| Range source event with k > 1 items; an insert that adopts more than one orphan | Bulk |
| Single-item source add, remove, replace or move (k = 1, any subtree size); live-shaping filter, sort-key or group-key change; reparent; adoption of one orphan | Surgical: `ShapedEdit`s |
| Expand or collapse of one node | Surgical, flattener only (no engine) |

**Types.**

```cpp
// Engine → flattener: one structural change to the shaped tree.
enum class ShapedEditKind : uint8_t { Insert, Remove };

struct ShapedEdit
{
    ShapedEditKind Kind;
    ShapedRef Node;          // record or group header; its subtree moves with it
    ShapedRef Parent;        // shaped parent at that location (the shaped root at top level)
    uint32_t IndexInParent;  // position among Parent's shaped children
};

// Flattener → visible list: a contiguous range replaced.
struct FlatSplice
{
    uint32_t Index;          // in the list as it stands after the earlier splices of this delta
    uint32_t RemovedCount;
    uint32_t InsertedCount;
};

// Flattener → owner: row chrome to refresh, no rows moved.
enum class RowStateField : uint8_t
{
    None       = 0x0,
    ItemCount  = 0x1,        // group header count text
    Expandable = 0x2,        // shaped child count went 0 <-> 1
    Expanded   = 0x4,        // the node was toggled
};

struct RowStateEdit
{
    uint32_t Index;          // in the final list
    RowStateField Fields;
};

struct FlatDelta
{
    bool IsReset = false;                 // bulk: stable Reset; Splices and States are empty
    std::vector<FlatSplice> Splices;      // applied in order
    std::vector<RowStateEdit> States;     // after all splices
};
```

- **`RowStateField` set (decided: three flags).** Checked against every `TableViewRowInfo` field
  and the group header: Kind never changes in place; Level, `ParentIdentity` and the group key
  change only through a regroup (`Reset`) or a move (the row is realized again); position and size
  in set are read on demand; item content updates through INPC. That leaves `ItemCount` (group
  header count; groups only), `Expandable` (0 <-> 1) and `Expanded`. A record's `ChildCount`
  (2 -> 3) is not flagged: only UIA reads it, on demand, and the visible chrome reacts only to the
  0 <-> 1 change that `Expandable` covers.
- **Dirty tracking (decided: `m_dirty`, a map owned by the flattener).** Needed because
  `RowStateEdit.Index` is in the final list but marks happen mid-commit while later splices shift
  indexes (so nodes are recorded and resolved in `EndCommit`); because one commit can mark the same
  node many times (100 inserts into a group mark it, and its ancestor groups, 100 times; one edit
  each, fields merged); and because a node marked and then removed or hidden in the same commit must
  be dropped. Rejected: dirty bits on `ShapeState`/`GroupNode` plus a vector (a dropped
  `GroupNode` is destroyed mid-commit, leaving a dangling entry; and View-layer state on engine
  types); emitting edits immediately and shifting their indexes per later splice
  (O(states x splices), still needs dedup and removal); and no tracking, with the owner refreshing
  every realized row after each surgical delta (v1-like and simpler, but O(realized) refreshes on
  every commit, even for an unrelated single-item add).
- **Splice shape (decided: general, no merging yet).** `FlatSplice` keeps both counts, but the
  flattener emits one-sided splices only (`{i, n, 0}` from `OnRemoving`, `{i, 0, n}` from
  `OnInserted` and expansion) and never merges them. Merging an adjacent remove and insert at the
  same index into `{i, r, a}` (published as a `Replace` when `r == a`) only pays off if the owner
  treats a replaced row differently, for example keeping selection on it; revisited with owner
  notifications. Rejected: a one-sided `{Index, Count, Kind}` type, which would have to change to
  add merging later.
- **`ShapedEdit` fields (decided).** `Parent` and `IndexInParent` are kept: on a remove they
  describe the old location, which the node can no longer report once it is unlinked, and they let
  the flattener validate its walk. Upward walks elsewhere use `ShapedRef::ShapedParent()`, which
  relies on `ShapeState::Group` for top-level records (see "Record → group link").
- **Bulk threshold (decided: k > 1 → bulk).** k is the number of items in one source
  notification: `NewItems`/`OldItems` size for a range event, or the number of orphans an insert
  adopts under `ParentBy`. A record's descendants never count (a `ChildrenBy` subtree is one
  `ShapedEdit`), nor does filtering (decided before it). k = 1, the only case `ObservableCollection`
  raises, is spliced; k ≥ 2 goes through `Reapply()` and one stable `Reset`. Held in
  `ShapingManager::c_bulkThreshold = 1` so it can be tuned.
  - Why: splicing k items costs k `CollectionChanged` events to the repeater (its per-event work
    dominates) plus an O(n) `m_rows` shift each; a rebuild is O(n log n) and one event. Matches v1
    `ShapedItemsSource`, which splices only `Size() == 1` changes and refreshes otherwise; WPF
    `ListCollectionView` rejects range actions.
  - Depends on the *Stable `Reset` keeps scroll* spike; if it fails, revisit with a larger fixed
    threshold.
  - Rejected: a larger fixed threshold (arbitrary, k events to the repeater, another path to
    test), a threshold relative to n (behavior varies with list size) and always surgical (10k
    events and O(k·n) shifting for a 10k range).
- **A move is a `Remove` then an `Insert`.** A reparent, a live re-sort or a group-key change moves
  the node with its subtree. The repeater has no `Move` (`ViewManager` handles Add, Replace,
  Remove and Reset), so a move is published as a remove and an add either way; with
  `IKeyIndexMapping` the element is recycled by identity.
- **A group created or dropped** is an `Insert` or `Remove` of the `GroupNode`, with its subtree. A
  drop also erases its expansion entry (*Expansion*, prune rules).
- **Replace from the source** is a `Remove` of the old record and an `Insert` of the new one.
- **`RowStateEdit` is derived by the flattener, never emitted by the engine:**

  | State | How the flattener derives it |
  |---|---|
  | Group `ItemCount` | Any insert or remove of a record under group G marks G and its ancestor groups |
  | `Expandable` | After an insert or remove under parent P, P is marked if its shaped child count is now 1 or 0 |
  | `Expanded` | The flattener toggles expansion itself and marks the node |
  | Position in set / size of set | **Not emitted.** Every later sibling shifts, so it would cost O(siblings) per edit; UIA reads both on demand (a search of the shaped parent's children, and `ShapedParent().ShapedChildCount()`) |
  | Item content (`ItemChanged` with no move) | Not emitted: the row's bindings update through `INotifyPropertyChanged` |

**Sink.** The engine reports to the flattener through a narrow interface, so the engine never
sees the visible list:

```cpp
struct IShapedEditSink
{
    virtual void OnRemoving(ShapedEdit const&) = 0;  // before the engine unlinks the node (old tree)
    virtual void OnInserted(ShapedEdit const&) = 0;  // after the engine links the node (new tree)
    virtual void OnRebuilt(GroupNode& root, uint16_t groupLevelCount) = 0;  // after a full re-apply
};
```

`groupLevelCount` (active `GroupBy` levels, 0 when ungrouped) feeds `ShapedDepth()` and the
bottom-level test; the flattener keeps it until the next rebuild.

- **Decided: `groupLevelCount` is a parameter on `OnRebuilt`.** The count changes only on a
  regroup, which always takes the bulk path, so the rebuild is the only point where it must be
  delivered. The engine supplies it from the applied `GroupBy`; the flattener stores it and serves it
  as `GroupLevelCount()` to `TableViewRowInfo` (`Level`), the spacer width
  (`EnsureRowGroupSpacerColumnWidth`) and UIA. Rejected:
  - *Derive from the tree* (walk to the first record): reads 0 when grouped but the result is empty
    (everything filtered out), and hierarchical records sit at mixed depths.
  - *Field on the root `GroupNode`*: meaningful only on the root, and an extra invariant the engine
    must keep in sync.
  - *Flattener queries the engine or applied spec*: makes the View layer depend on engine and spec
    types, and risks reading a pending spec instead of the applied one.
  - *Read it from `TableViewRowInfo` / `TableViewGroupInfo`*: circular. Both are built from the
    count; a group's `Level` is its own depth, and knowing a group is bottom-level already requires
    the count.

- **Root lifetime (decided: one stable root).** The engine owns the shaped root by value for its
  whole lifetime; its address never changes. A rebuild builds the new child groups aside, then
  swaps them under the same root and resets the root's own counts and flags, so the "old tree kept
  on throw" rule still holds. The flattener binds `m_root` on the first `OnRebuilt`; later calls
  assert the same root. Records in `m_rows` and `m_dirty` stay valid because `OnRemoving` and
  `OnRebuilt` run inside the `SourceChanged` handler, before `SourceTable` destroys any record.
  Rejected: a new root per rebuild (`OnRebuilt` must re-point the flattener before the old tree is
  freed, and any held root `ShapedRef` goes stale) and no stored root (an engine query on every
  walk couples the View layer to the engine).
- **Teardown order (decided).** `ShapingManager::Clear()` calls `Flattener::Clear()` first: it
  empties `m_rows`, `m_dirty` and `m_pending` and nulls `m_root`. Only then does
  `SourceTable::Clear()` destroy the records. A consumer that still holds the `ItemsSourceView`
  afterwards sees `Size` 0, and `GetAt` throws out of bounds.

- **Two callbacks, not one `OnEdit`.** For a remove the flattener reads the **old** tree, before
  the unlink, to find the node's flat index and span; for an insert it reads the **new** tree.
- **Called only during commit.** All app code runs in the compute phase (*Misbehaving records*),
  so the sink is never called for a unit of work that later throws, and the flattener never sees
  a half-applied change.

**Flattener (declared in `View/Flattener.h`).**

```cpp
class Flattener final : public IShapedEditSink
{
public:
    explicit Flattener(GroupExpansion& groupExpansion);

    uint32_t Size() const noexcept;
    ShapedRef GetAt(uint32_t index) const;             // O(1)
    std::optional<uint32_t> IndexOf(ShapedRef) const;  // nullopt when an ancestor is collapsed
    uint16_t GroupLevelCount() const noexcept;

    void Rebuild(GroupNode& root, uint16_t groupLevelCount);  // depth-first; fills m_rows and every VisibleDescendantCount
    void BeginCommit();
    void OnRemoving(ShapedEdit const&) override;
    void OnInserted(ShapedEdit const&) override;
    void OnRebuilt(GroupNode& root, uint16_t groupLevelCount) override;  // Rebuild, then the delta is IsReset
    FlatDelta EndCommit();                             // resolves m_dirty into RowStateEdits
    FlatDelta Expand(ShapedRef);
    FlatDelta Collapse(ShapedRef);
    FlatDelta ExpandAllGroups();                       // move the baseline, Rebuild; IsReset
    FlatDelta CollapseAllGroups();
    FlatDelta ExpandAllRows();                         // set every record's flag (O(n)), Rebuild; IsReset
    FlatDelta CollapseAllRows();
    void Clear();                                      // teardown: drop rows, dirty set, root

private:
    bool IsShown(ShapedRef) const;                     // every ancestor expanded; O(depth)
    uint32_t FlatIndexOf(ShapedRef) const;             // precondition: shown
    std::vector<ShapedRef> m_rows;                     // visible list
    FlatDelta m_pending;
    std::unordered_map<ShapedRef, RowStateField, ShapedRefHash> m_dirty;  // nodes, not indexes
    GroupExpansion& m_groupExpansion;                  // owned by ShapingManager
    GroupNode* m_root;                                 // engine's stable root; bound once
    uint16_t m_groupLevelCount;
    bool m_inCommit;                                   // expansion asserts it is false
};
```

**Algorithms.**

- **`IndexOf(node)` (decided: option a, linear sum).** Start at the parent's index + 1 (−1 + 1 = 0
  for children of the shaped root, which is not displayed), then add `1 + VisibleDescendantCount`
  for each earlier sibling; repeat up the ancestors. O(depth × siblings).
  - Later, if profiling needs it: a Fenwick tree over sibling spans (O(depth × log siblings)), or a
    cached index invalidated by splices. Neither changes the sink or the delta types.
- **`IndexOf` is public and returns `std::optional<uint32_t>` (decided).** Public because only the
  flattener can map a node to its row without an O(n) scan, and callers outside it need that:
  `TryGetIndexForIdentity`, `IKeyIndexMapping::IndexFromKey`, selection/focus/scroll restore after a
  `Reset`, UIA `Navigate(Parent)` and keyboard parent navigation. `nullopt` when an ancestor is
  collapsed, so a hidden row cannot be mistaken for a valid index; the visible list converts it to
  `-1` at the WinRT boundary. Internal callers that already know the node is shown use the private
  `FlatIndexOf`. Rejected: a `uint32_t`/`int32_t` sentinel (easy to skip the check) and v1-style
  `bool TryIndexOf(ref, uint32_t&)` (wordier, mixes styles in v2).
- **`IsShown(node)`:** every ancestor is expanded. O(depth). **Private (decided):** only the
  flattener's own algorithms need "hidden?" without the index; outside callers always need the
  index too and use `IndexOf`.
- **Expand-all verbs (decided): one per axis, no root parameter.** `ExpandAllGroups`,
  `CollapseAllGroups`, `ExpandAllRows`, `CollapseAllRows`, matching v1
  `ITableViewRowMetadataProvider`. v1 split them so expanding groups would not materialize the
  tree; in v2 shaped children are eager, but opening every group and expanding every tree node
  remain different user actions. They act on `m_root` (a root parameter was redundant and could
  desync the flattener from the tree). Rejected: a single `ExpandAll` for both axes.
- **`OnRemoving(e)`:**
  1. `span = 1 + VisibleDescendantCount(node)`.
  2. If shown: `i = FlatIndexOf(node)`, erase `m_rows[i, i + span)`, append `{i, span, 0}`.
  3. Subtract `span` from `VisibleDescendantCount` on each ancestor, stopping at the first
     collapsed one (a collapsed ancestor's own count is 0 and does not change).
  4. Mark the parent dirty (`ItemCount` up the group chain for a record, `Expandable` if its shaped
     child count is now 0).
  5. Drop the node and its subtree from `m_dirty`.
- **`OnInserted(e)`:** the mirror.
  1. Compute the node's `VisibleDescendantCount` from its subtree.
  2. If shown: insert its rows at `FlatIndexOf(node)`, append `{i, 0, span}`.
  3. Add `span` up the ancestor chain, stopping at the first collapsed ancestor.
  4. Mark the parent dirty (`ItemCount`; `Expandable` if the count is now 1).
- **Later siblings are not touched.** The visible list stores no sibling index, so an insert or remove
  writes only the spliced range (see *Visible list*).
- **`Expand(node)`:**
  1. Set the flag: `ShapeFlags::Expanded` for a record, the expansion model for a group.
  2. `VisibleDescendantCount = Σ(1 + VisibleDescendantCount(child))` over the direct children.
     Children keep their counts while hidden, so this is O(children).
  3. If shown: insert the rows at `FlatIndexOf(node) + 1`, append the splice, add the count up the
     ancestors.
  4. If shown, mark the node (`Expanded`).
- **`Collapse(node)`:** the reverse; the count drops to 0 and the rows are erased.
- **Expanding a hidden node (decided: flag only).** When an ancestor is collapsed, `Expand` /
  `Collapse` set the flag and the node's own count, the walk up stops at the collapsed ancestor,
  nothing is spliced or marked, and the delta is empty. The state is kept: when the ancestor is
  later expanded, its O(children) sum already includes the node's subtree, so one splice inserts it.
  Matches WinUI `TreeView` / WPF `TreeViewItem.IsExpanded`. Making a node visible (expand the
  ancestors, then scroll) is a separate reveal verb in the expansion API, not a side effect of
  `Expand`. Rejected: `Expand` also expands every collapsed ancestor (changes state the caller did
  not ask for; one splice per ancestor).
- **Publishing expansion deltas (decided: own unit of work, one publish path).** `Expand`,
  `Collapse` and the expand-all verbs bracket themselves (begin, splice and mark, end) and return the
  `FlatDelta`. `ShapingManager::Publish(FlatDelta)` is the only path to consumers for every delta,
  whatever produced it: splices to the visible list's `CollectionChanged`, then `RowStateEdit`s to
  the owner. An expansion call while a commit is open asserts (`m_inCommit`), so it never merges
  into an engine delta. Each expand publishes one delta; if batching is ever needed (for example a
  multi-step reveal), it is a scope private to `ShapingManager`, not a public bracket. Rejected:
  writing into `m_pending` with the caller bracketing `BeginCommit`/`EndCommit` (the protocol
  becomes public, and a missed bracket leaves stale dirty entries or an unpublished delta), and the
  flattener raising events itself (it would depend on the visible list and the owner, splices would
  go out while `m_rows` is half-updated, and engine commits would need the same sinks).
- **`EndCommit()`:** for each dirty node that is still shown, add `{FlatIndexOf(node), fields}` to
  `States`; return `m_pending`. Splice indexes refer to the list after the earlier splices of the
  same delta; state indexes refer to the final list.

**Order of a surgical change.**

```
compute (engine; app code; may throw → nothing written, nothing published)
  → BeginCommit
  → commit (engine writes Shape, groups, counts; calls OnRemoving / OnInserted; no app code)
  → EndCommit → FlatDelta
  → publish (visible list raises CollectionChanged per splice: Remove range, then Add range)
  → owner notification (RowStateEdits; selection, focus and rank purge of removed records)
```

Bulk is the same with `OnRebuilt` in the commit step and one `Reset` in the publish step.

- **Stable `Reset`.** The visible list implements `IKeyIndexMapping` (key = record `Identity()`,
  or `GroupNode::Identity` for headers), so the repeater pools elements by key and reuses them
  across a `Reset`. `TableView` restores selection, focus and scroll anchor by identity.
- **`RowStateEdit`s are not collection changes.** They go to the owner, which refreshes the chrome
  of realized rows at those indexes; unrealized rows read the state when realized.

## 7. Implementation plan

Each phase is designed (headers first) and agreed before its `.cpp` implementation is wired up.

### Phase 1 — Public API & source tracking (headers done, `.cpp` pending)
- [x] **1.1 Public IDL & runtimeclass shell** (`TableViewSource2.idl`, `TableViewSource2.h/.cpp`):
      verbs (`Filter`, `GroupBy`, `Sort`, `ParentBy`, `ChildrenBy`, `Clear*`), `Refresh()`, `IsLiveShaping`.
- [x] **1.2 Source collection resolution** (`Source/CollectionAccessor2.h`):
      single-pass resolution of read (`IVector` / `IVectorView` / `IBindableVector`) and notification
      (`INotifyCollectionChanged` / `IObservableVector` / `IBindableObservableVector`) interfaces.
- [x] **1.3 Source record & table** (`Source/SourceRecord.h`, `Source/SourceTable.h`):
      `SourceRecord` (item, canonical `Address`, lazy `Identity()`, live-shaping watch state) and
      `SourceTable` (owns `CollectionAccessor2` + the source `event_token`, maintains `m_records`, `m_rawLookup`,
      `m_addressLookup`, shaped-only `m_identityIndex`, verb-independent per-record `INotifyPropertyChanged`
      subscriptions via `SetLiveShaping`, and the `SourceChanged` + `ItemChanged` callbacks).
- [ ] **1.4 Implement Phase 1 `.cpp` files**:
  - `CollectionAccessor2.cpp`: port `Count`, `TryGetAt`, `Enumerate`, `GetAtUnchecked`.
  - `SourceRecord.cpp`: canonical `IUnknown` QI, lazy hex `Identity()`, `~SourceRecord()` token revoke.
  - `SourceTable.cpp`: `SourceTable::Create` / `AttachSource`,
    upfront null/duplicate validation (`ValidateItems`, `ValidateIncoming`, `m_outOfSync` recovery), 3-pass `Reconcile`
    (position → raw pointer → address), incremental `InsertAt`/`RemoveAt`/`ReplaceAt`/`Move`,
    `RaiseSourceChanged` ordering and re-entrant queueing, source change subscriptions, and
    `SetLiveShaping` / per-record `INotifyPropertyChanged` subscription + `ItemChanged` forwarding,
    off-thread marshalling to `m_ownerQueue` and dropped-change recovery, and `Clear()` detach
    (optional accessor reset, `m_trackerOwner` nulled, `IsDetached()` guards) with `safe_get`-only
    teardown in `UnsubscribeFromSourceChanges`.
  - `TableViewSource2.cpp`: construct `m_shapingManager` (`ShapingManager(this, items)`, which
    calls `SourceTable::Create(this, items)`) in `TableViewSource2(items)`, forward `Refresh()` and
    `IsLiveShaping` to it, and call `m_shapingManager->Clear()` in `~TableViewSource2()`.

- [ ] **1.5 Source tree** (`Source/SourceRecord.h`, `Source/SourceTable.h`): declared on `SourceRecord` (`Parent`,
      `Children`, `Depth`, `HasChildren()`) and `SourceTable` (`TopLevel()`,
      `SetParentKeyHierarchy`, `SetChildCollectionHierarchy`, `ClearHierarchy`, `ChildFeed`s,
      key index, `SiblingsOf`). Implement, then wire `TableViewSource2` (`ParentBy`, `ChildrenBy`,
      `ClearHierarchy`) through `ShapingManager`.

### Phase 2 — Shape spec (`Spec/ShapeSpec.h/.cpp`, `Spec/ShapeDelegates.h/.cpp`, `Spec/PathEvaluator.h/.cpp`)
- [x] Declare how active verbs (`Filter` list, `Sort` levels, `GroupBy`) are stored (`ShapeSpec`,
      `FilterSpec`, `SortSpec`, `GroupSpec`, `DelegateRef`). Hierarchy selectors stay in `SourceTable`.
- [x] Declare spec diffing (`ShapeDelta`, `Diff`) so the coordinator knows whether
      a verb change is a no-op, flat re-sort, or filter/group reshape from `SourceTable` (no source
      re-read). A delegate re-declaration always marks its verb changed; hierarchy changes arrive as a
      `SourceTable` Reset.
- [x] Declare the delegate registry (`ShapeDelegates`) and the path evaluator (`PathEvaluator`).
- [ ] Implement `SortSpec::Matches`, `Diff`, `ShapeDelegates` and `PathEvaluator`.
- [ ] Revisit path evaluation (`PathEvaluator`, `SortMemberPathResolver`; open, see §5).
- [x] Decide spec ownership and application (§5: desired spec in the coordinator, applied in the engine).
- [x] Decide the per-verb rule for misbehaving records (§5: propagate, compute before commit,
  ranges all-or-nothing, group key failures go to the null-key group, desired spec not reverted
  on a failed apply, `null` keys lowest and flipping with direction, hierarchy selector errors
  placed per record after the verb, a `ChildrenBy` item already in the tree rejected with
  per-feed out-of-sync recovery).
- [x] Add the desired spec to the coordinator (`ShapingManager::m_desired`, declared).
- [ ] Wire `TableViewSource2` verb methods to forward to `ShapingManager`, which updates `m_desired`.

### Phase 3 — Projection output & row metadata (`Projection/Projection.h/.cpp`, `RowMetadataProvider`)
- [ ] Define the projected row store (`std::vector<SourceRecord*>` + `Identity` → projected index for
      flat views; group/hierarchy adapters for grouped/tree views) separated from `SourceTable`.
- [ ] Expose the WinRT `IObservableVector<IInspectable>` view consumed by `ItemsRepeater`.
- [ ] Wire `RowMetadataProvider` to read row identities and indices from the projection and shared
      `std::shared_ptr<SourceTable>` without recomputing `Identity()`.

### Phase 4 — Shaping engine (`Engine/ShapingEngine.h/.cpp`)
- [ ] Shaped tree node types `ShapedRef`, `GroupNode` (`Engine/ShapedTree.h`): declared, no
      implementation.
- [x] Decide where engine state lives: an engine-owned `ShapeState` slot on `SourceRecord`.
- [x] Declare `ShapeState` (`Engine/ShapeState.h`, stored inline) and add `SourceRecord::Shape`.
- [ ] Run filter, sort, group, and hierarchy over `SourceTable::TopLevel()` and
      `SourceRecord::Children` without re-enumerating the underlying collection.
- [ ] Map shaped output back to `SourceRecord*` in O(1) via `SourceTable::TryFindByRawPointer`.

### Phase 5 — Coordinator (`Shaping/ShapingManager.h/.cpp`)

- [x] Decide who owns the shaping parts (§6: native coordinator `ShapingManager`, owned by
      `TableViewSource2`).
- [x] Declare `ShapingManager` (verbs, `Refresh`, `IsLiveShaping`, `Clear`, handlers, owned
      parts) and replace `TableViewSource2::m_sourceTable` with `m_shapingManager`.
- [x] Declare `View()` (returns the cached `ItemsSourceView`, `m_view`).
- [x] Decide how the engine and the flattener exchange changes (§6 *Change pipeline*: hybrid,
      `ShapedEdit` → `FlatSplice` + `RowStateEdit` → `FlatDelta`, `IShapedEditSink`).
- [x] Declare `ShapedEdit`, `IShapedEditSink` (`Engine/ShapedEdit.h`), `FlatSplice`,
      `RowStateEdit`, `FlatDelta`, `GroupExpansion` (`View/`) and the `Flattener` (`View/Flattener.h`).
- [x] Declare the flattener and group expansion members (`m_flattener`, `m_groupExpansion`).
- [ ] Declare the engine and WinRT visible-list members.
- [ ] Implement the `Flattener` (`View/Flattener.cpp`) and `ShapedRef::ShapedChildCount`,
      `operator==`, `ShapedRefHash`.

The coordinator reacts to live-shaping events but does not own subscriptions; those live in
`SourceTable` (Phase 1).

- [ ] Handle `SourceTable::SourceChanged` (the coordinator is its only subscriber): incremental
      projection splices (when eligible) or cached reshapes over `SourceTable::Records()` on `Reset`.
- [ ] Handle `SourceTable::ItemChanged` (`(SourceRecord*, propertyName)`): filter by active
      `ShapeSpec` property dependencies, coalesce per dispatcher turn, and re-evaluate affected
      records (every record for `ItemChanged(nullptr, L"")`) without managing `INotifyPropertyChanged` subscriptions in the coordinator.
- [ ] Fan out `SourceChanged` to `TableView` owner callbacks (selection/focus/rank purge of removed
      records, projection rebuild notifications).

### Phase 6 — `TableView` integration & validation
- [ ] Switch `TableView` to accept `TableViewSource2` alongside `TableViewSource`.
- [ ] Build and run API + interaction test suites (`TableViewSource_APITests`, `TableView_Sorting_APITests`,
      `TableView_LiveShaping_APITests`, `TableView_HierarchicalRows_APITests`).

## Later design sessions: `DeferRefresh`

Postponed, not dropped. This records what is already settled so the later session starts from it.

- **What it is.** `DeferRefresh()` returns a handle; verbs called while a handle is open edit the
  desired spec but do not apply it. The outermost handle's `Dispose` applies
  `Diff(applied, desired)` once, giving one reshape and one `FlatDelta` for several verbs.
- **Nesting (proposed, WPF model).** A depth counter: every verb is gated (unlike the Community
  Toolkit's `Filter`), inner ends never refresh, and disposing a handle twice is a no-op.
- **Errors.** An apply exception from app code surfaces from the outermost `Dispose`, with the same
  rules as a verb (see *Misbehaving records*: desired not reverted, applied stays old).
- **Hierarchy verbs (decided).** `ParentBy`/`ChildrenBy` act on `SourceTable`, not the spec, so a
  spec-only deferral would not batch them: the tree would rebuild and reshape with the old spec,
  then reshape again at `Dispose`. `SourceTable` therefore gets its own deferral counter, and an
  open `DeferRefresh` holds it too. At the outermost `Dispose`, `SourceTable` rebuilds the tree
  once and the engine reshapes once with the new spec. Rejected: leaving hierarchy verbs
  undeferred (two reshapes, an intermediate state visible), and moving the hierarchy selectors
  into the spec (contradicts `SourceTable` owning the tree).
- **Open for that session.** Whether source changes arriving while a handle is open are shaped
  immediately with the applied spec or queued; how a held `SourceTable` deferral interacts with
  `m_pendingReset`; whether `Refresh()` inside a handle is deferred too.
- **Checklist (when picked up).**
  - [ ] Declare `DeferRefresh()` in the IDL and `TableViewSource2`.
  - [ ] Add the `DeferRefresh` depth counter to `TableViewSource2` (or the coordinator).
  - [ ] Add the `SourceTable` deferral counter and hold it from `DeferRefresh`.

## Open items

- **Stable `Reset` keeps scroll (to verify).** Spike that the repeater keeps realized elements and
  the scroll position on a `Reset` when the source implements `IKeyIndexMapping`. Bulk changes
  depend on it.
- **Owner notifications (open).** Also decides whether the flattener merges an adjacent remove and
  insert into one `{i, r, a}` splice published as a `Replace`. The callback that carries `RowStateEdit`s and removed records to
  `TableView`, replacing v1 `ShapingChanged(reorderOnly)`.
- **Source change during an apply (open).** A source event raised from app code in the compute
  phase.
- **Revisit the flattener after the engine (later).** The flattener declarations
  (`View/Flattener.h`, `FlatDelta.h`, `GroupExpansion.h`, the `IShapedEditSink` contract) were
  settled before the engine was designed. Once the engine is declared, re-check the sink calls,
  root lifetime, `groupLevelCount` handoff, `ShapedEdit` fields and dirty-state derivation against
  what the engine actually produces.
- **Expansion API (open).** Three parts:
  - **Reveal verb:** expand a node's ancestors so it becomes visible. v1 has none: `Select(index)`
    takes only visible indexes, and `ExpandSubtree` expands downward.
  - **Filter auto-expand:** v1 shows the ancestors of matching rows as expanded "context rows"
    (rows only, not groups). Toggling a context row goes to a temporary overlay
    (`m_filterOverlayCollapsed`). Clearing the filter restores the user's earlier state, but
    `CollapseAllRows` under a filter persists. The flattener has no overlay yet.
  - **Public per-item expand/collapse:** v1 has only the four expand-all verbs, plus toggling
    through the UI, keyboard, UIA and the group header's `IsExpanded` DP.

- `Refresh` and `IsLiveShaping` are declared in the IDL and the header, with no `.cpp` stubs yet (Phase 1.4).
- `m_shapingManager` is never created yet (the constructor does not allocate it), and
  `Shaping/ShapingManager.h` has no `.cpp` (Phase 1.4 / Phase 5).
- `Source/SourceRecord.h`, `Source/SourceTable.h` and `Source/CollectionAccessor2.h` have no `.cpp` files yet (Phase 1.4).
- The source-tree members of `SourceTable` are declared only (Phase 1.5).
- `Engine/ShapedTree.h` (`ShapedRef`, `GroupNode`) and `Engine/ShapeState.h` (`ShapeState`) are declared
  only (Phase 4).
- `Engine/ShapedEdit.h`, `View/FlatDelta.h`, `View/Flattener.h` and
  `View/GroupExpansion.h` are declared
  only (Phase 5).
- `Spec/ShapeSpec.h`, `Spec/ShapeDelegates.h` and `Spec/PathEvaluator.h` are declared only
  (Phase 2); `ShapingManager` declares the members that own them (§6).
- **`ShapingManager::Source()` accessor (later).** Add only if a non-shaping consumer (column
  autogeneration, source-keyed selection) needs `SourceTable` directly (§6, option D).
- **Split hierarchy out of `SourceTable` (later).** `SourceTable` mixes always-on code with code
  used only by one feature (ParentBy, ChildrenBy, live shaping, identity index), which hurts
  readability and single responsibility. Proposed: a `SourceHierarchy` strategy owned by
  `SourceTable` (`std::unique_ptr`, null = flat) with `ParentKeyHierarchy` and
  `ChildCollectionsHierarchy` implementations; hooks `Prepare`/`Commit` (validate, then build),
  `CollectIncoming`, `OnInserted`/`OnRemoving`/`OnMoved`, `Reevaluate`, `Release`/`Clear`.
  Not a `HierarchicalSourceTable` subclass: hierarchy is set and cleared on a live table, and
  lookups, live shaping and removal all need hierarchy hooks. Callback models considered: a thin
  `ISourceHierarchyHost` (`CreateRecord`, `ReleaseRecord`, `RaiseSourceChanged`) plus extracted
  `SourceLookups` (with object validation) and `SourceTree` (top level and links) passed by
  reference; or strategies as `friend`s holding `SourceTable&`. Live shaping and its threading
  state could move to an `ItemWatcher` the same way. Undecided; done after the file split
  (`SourceRecord.h` / `SourceTable.h`).
- **Re-entrant `ItemChanged` (parked).** A `PropertyChanged` raised while `SourceChanged` or
  `ItemChanged` is dispatching (for example, a setter called from a comparer) would reach the
  coordinator mid-reshape. Leading candidate: route it through the same `m_isDispatching` gate,
  queue `(SourceRecordWeakPtr, propertyName)` in `m_pendingItemChanges`, and drain after pending
  source changes; plus a `DeferNotifications()` RAII scope the coordinator wraps verb-driven
  reshapes in. Alternatives: collapse to `ItemChanged(nullptr, L"")`, or a coordinator-side guard.
