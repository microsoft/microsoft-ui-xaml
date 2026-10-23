# TableView Row Identity and Index Maps

Reference notes on how TableView identifies rows, groups, and tree nodes, and which internal
identity/index maps exist for each data shape. Use this to reason about selection retention,
expansion persistence, incremental updates, and live shaping.

Stack: `TableView` → `TableViewSource` → `ShapedItemsSource` (engine) →
`GroupedSourceAdapter` / `HierarchicalSourceAdapter` → `RowMetadataProvider`.

This describes the implementation after T1-T8 and T11 of the
[unification plan](TableView-Object-Address-Unification-Plan.md). T12 (hierarchy row identity)
and T13 (address-reuse lifetime audit) are not implemented yet.

## 1. Identity kinds

| Identity | Format | Produced by | Stable across re-created item objects? |
|---|---|---|---|
| Row (object) identity | `"0x<IUnknown address>"` | `RowIdentity::MakeObjectIdentitySelector()` → `FormatObjectIdentity(GetObjectAddress(item))` (`controls/dev/ShapedItemsSource/RowIdentity.cpp`) | No |
| Source-supplied row key | app key | `ItemsSourceView.HasKeyIndexMapping()` / `KeyFromIndex` in `RowMetadataProvider` (flat only) | **Unreachable today**: the only caller (`TableViewSource`) passes `ItemsSourceView{ m_engine->Rows() }`, a `single_threaded_observable_vector` without `IKeyIndexMapping`, so an app's `IKeyIndexMapping` is never used as identity |
| Group row identity | `"group:" + canonical group key` | `RowMetadataProvider::GetGroupExpansionKey` | Yes for value-typed keys / with `groupIdentitySelector` |
| Group expansion intent key | `"identity:" + key`, else group object lookup key | `GroupedSourceAdapter::GetGroupIntentKey` | Yes for value-typed keys |
| Node (hierarchy) identity | `"node:" + app key lookup form` | `ShapingHelpers::MakeNodeKey` (`TabularShaping/ParentKeyIndex.cpp`) | Yes, unless the app key is itself an object (`node:object:...`, see `IsObjectNodeKey`) |
| Physical object address | `ShapingHelpers::ObjectAddress` (strong enum over `uintptr_t`) | `ShapingHelpers::GetObjectAddress(item)` queries canonical `::IUnknown` | No |

Notes:

- Physical bookkeeping uses `ObjectAddress`, computed through the shared, `noexcept`
  `ShapingHelpers::GetObjectAddress`. Null input or a failed query returns `ObjectAddress::Null`.
  The enum adds no boxing and owns no object reference.
- Logical row identity remains a separate string-selector path. It currently formats the object
  address, but could later accept an app key; subscriptions, snapshots, ranks, dirty rows and
  physical item lookups must still use the object address. External row identity is not added by
  this unification. Hierarchy logical identity remains node-key-based until T12.
- Row object identity is the canonical `IUnknown` pointer (COM identity). Two references to the
  same object always match; two distinct objects (including two boxes of the same value) never do.
- There is no app-supplied row identity for flat/grouped data: COM object identity is the only
  source. Apps that re-create item objects get new identities and therefore a full rebuild, and
  selection based on object identity is lost.
- Engine (`ShapedItemsSource`, map #1) and `RowMetadataProvider` use the **same selector
  instance**: `IdentitySelector()` (`m_intrinsicKeySelector`), which
  `TableViewSource::MakeIdentitySelector()` only unboxes. A given item therefore has the same
  `"0x..."` identity in both. Differences: map #1 is populated only when `IsIdentityRequired()`
  (filter, sort, group, parent, or mutable source); `RowMetadataProvider` computes on demand and
  returns `""` on failure instead of a diagnostic reason.
- Group keys: value types (string, numbers, enums, ...) canonicalize through
  `ValueKey::TryGetStablePropertyKey`. A reference-typed group key without a
  `GroupBy(key, groupIdentitySelector)` throws `E_INVALIDARG`; grouping never silently degrades.
- A source with no usable row identity degrades to `ProjectionKind::Unshaped` (a 1:1 mirror) and
  `RowMetadataProvider` is null.

## 2. Identity per data shape

### Unshaped (raw ItemsSource or degraded projection)

- No shaped identity; no row metadata.
- Selection: `SelectionModel` index, plus `m_stickySelectedItem` (object identity via
  `TableView::SameInspectableIdentity`) restored after a `Reset`.

### Flat (filter / sort, no grouping or hierarchy)

- Row identity: COM object identity (the `KeyFromIndex` branch is unreachable, see section 1).
- Incremental updates: `m_flatRowIdentities` + `m_flatRowIdentityToIndex` give O(1) lookup;
  indices are shifted on insert/remove; any mismatch falls back to a full reprojection.
- Selection: sticky item restored by object identity after a reshape.

### Grouped

- Data rows: COM object identity.
- Header rows: `group:` + canonical group key.
- Expansion: `RowExpansionModel` keyed by group intent key, so state survives regroup/re-sort as
  long as the key value is the same.
- Selection: sticky item by object identity.

### Hierarchy (`ParentBy(key, parentKey)`)

- Row identity: node key (`node:` + app key), which survives item re-creation.
- Expansion: `HierarchicalSourceAdapter` intent keyed by node key with a baseline
  (Expand/CollapseAll); filter context rows use a separate overlay (`m_filterOverlayCollapsed`).
- Selection: `TableView::m_stickySelectedIdentity` (node key) restores selection across a `Reset`
  even when the row object is re-created.
- Grouped + hierarchy: headers use `group:`, data rows use the node key (falls back to item key
  mid-reshape).

### Live data operations

- Collection changes: only a live flat projection is updated incrementally
  (`ShapedItemsSource::ApplyIncrementalChange`); grouped and hierarchical projections always do a
  full `Refresh()`.
- Sorted flat without live shaping: sort-key edits raise no collection notification, so
  `INotifyPropertyChanged` rows are observed (`m_flatItemPropertyChangedRevokers`, keyed by
  `IUnknown` address) and a sort-key property change marks the row in `m_sortDirtyRows`; the row
  is re-placed on the next collection change. With live shaping on, `LiveShapingTracker` owns this
  instead (one handler per row).
- Live shaping (`SetLiveShaping(true)`): per-item `INotifyPropertyChanged` subscriptions keyed by
  `ObjectAddress` through the shared helper. `CaptureLiveShapeSnapshot` records sort keys,
  group key, node/parent keys, and filter result; a differing snapshot marks the projection dirty
  and posts one coalesced `RestoreLiveShaping` per UI turn.
- Hierarchy: `ParentStructure::KeyByItem` maps item → node key; a changed parent key triggers a
  live reparent and invalidates the retained structure.

## 3. Inventory of identity-keyed maps

Physical keys in maps #4, #6-#8 and #12-#14, plus S3, share the `ObjectAddress` type and
`ShapingHelpers::GetObjectAddress`. Logical string keys in the other maps are unchanged.

### Identity → position

| # | Map | Key → value | Location |
|---|---|---|---|
| 1 | `m_flatRowIdentityToIndex` (+ set `m_flatRowIdentities`) | `"0x..."` → `uint32` | `ShapedItemsSource` |
| 2 | `m_identityToIndex` | row identity → `int32` (lazy, rebuilt wholesale) | `RowMetadataProvider` |
| 3 | `m_indexByNodeKey` | node key → visible index (lazy) | `HierarchicalSourceAdapter` |
| 4 | `m_indexByItem` | `ObjectAddress` item → visible index (lazy) | `HierarchicalSourceAdapter` |
| 5 | `ParentStructure::IndexByKey` | node key → source position | `TabularShaping/ParentKeyIndex.h` |
| 6 | `m_rootBucketIndex` | `ObjectAddress` root → group bucket | `ShapedItemsSource` |
| 7 | `m_rankByIdentity` | `ObjectAddress` item → sort rank | `ShapingHelpers::CustomSortRankAdapter` |

### Identity → key / object / state

| # | Map | Key → value | Location |
|---|---|---|---|
| 8 | `ParentStructure::KeyByItem` | `ObjectAddress` item → node key | `TabularShaping/ParentKeyIndex.h` |
| 9 | `ParentKeyIndex::Children` | parent node key → children | `TabularShaping/ParentKeyIndex.h` |
| 10 | `m_liveGroupsByIdentityKey` | group identity → live group | `GroupedSourceAdapter` |
| 11 | `m_groupCache` | group identity → `ShapedGroup` | `ShapedItemsSource` |
| 12 | `m_liveShapeSnapshots` | `ObjectAddress` item → `LiveShapeSnapshot` | `ShapedItemsSource` |
| 13 | `m_subscriptions` | `ObjectAddress` item → `PropertyChanged` subscription | `LiveShapingTracker` |
| 14 | `m_flatItemPropertyChangedRevokers` | `ObjectAddress` item → `PropertyChanged` subscription (holds `Item` strongly) | `ShapedItemsSource` |

### Identity sets

- `m_flatRowIdentities` (`ShapedItemsSource`)
- `m_sortDirtyRows` (`ObjectAddress` item addresses whose sort-key property changed; `ShapedItemsSource`)
- `RowExpansionModel::m_nonDefault` (expansion intent; one per grouped / hierarchical adapter)
- `m_filterOverlayCollapsed` (`HierarchicalSourceAdapter`)
- `ParentKeyIndex::ContextKeys`
- `m_ambiguousLiveGroupIdentityKeys` (`GroupedSourceAdapter`)

Excluded (not row identity): `TableViewCellsPanel` column-width maps keyed by column pointer.

## 4. Which maps are used in which case

GH = grouped + hierarchical projection.

| # | Map / set | Unshaped | Flat | Grouped | Hierarchy | GH | Gate |
|---|---|:-:|:-:|:-:|:-:|:-:|---|
| 1 | `m_flatRowIdentityToIndex` + `m_flatRowIdentities` | | ✅ | | | | Flat only. Built in `RebuildFlat` / in-place reshape; cleared by every other rebuild |
| 2 | `RowMetadataProvider::m_identityToIndex` | | ✅ | ✅ | ✅ | ✅ | Every shaped kind (no provider when Unshaped) |
| 3 | `m_indexByNodeKey` | | | | ✅ | ✅ | Hierarchy |
| 4 | `m_indexByItem` | | | | | ✅ | GH only (`TryGetNodeRowForItem`) |
| 5 | `ParentStructure::IndexByKey` | | | | ✅ | ✅ | Hierarchy |
| 6 | `m_rootBucketIndex` | | | | | ✅ | GH only |
| 7 | `m_rankByIdentity` | | ✅ | ✅ | ✅ | ✅ | Only with a custom pairwise comparer sort (`TableView_Sort.cpp`) |
| 8 | `ParentStructure::KeyByItem` | | | | ✅ | ✅ | Hierarchy |
| 9 | `ParentKeyIndex::Children` + `ContextKeys` | | | | ✅ | ✅ | Hierarchy (`ContextKeys` only matters with a filter) |
| 10 | `m_liveGroupsByIdentityKey` + ambiguous set | | | ✅ | | ✅ | Grouping |
| 11 | `m_groupCache` | | | ✅ | | ✅ | Grouping |
| 12 | `m_liveShapeSnapshots` | | ✅ | ✅ | ✅ | ✅ | Only with live shaping enabled |
| 13 | `LiveShapingTracker::m_subscriptions` | | ✅ | ✅ | ✅ | ✅ | Only with live shaping enabled |
| 14 | `m_flatItemPropertyChangedRevokers` | | ✅ | | | | Sorted flat projection with live shaping **off** (`ShouldTrackSortKeyChanges`) |
| S1 | `RowExpansionModel::m_nonDefault` | | | ✅ | ✅ | ✅ (×2) | Any expandable shape |
| S2 | `m_filterOverlayCollapsed` | | | | ✅ | ✅ | Hierarchy with an active filter |
| S3 | `m_sortDirtyRows` | | ✅ | | | | Same gate as #14; applied on the next collection change |

Summary:

- Flat only: #1; sorted flat without live shaping: #14, S3.
- Hierarchy (pure + GH): #3, #5, #8, #9, S2.
- GH only: #4, #6.
- Grouping (pure + GH): #10, #11.
- All shaped kinds: #2; S1 in every expandable shape.
- Feature-gated: #7 (custom comparer), #12 and #13 (live shaping).
- Unshaped: none.

## 5. Pointer identity and memory relocation

Object identities are not affected when a backing vector reallocates (for example, capacity
growth):

- `IVector` / `std::vector<IInspectable>` stores interface pointers. Reallocation copies or moves
  those pointer values; the COM objects stay at their heap addresses.
- Identity is the address of the object's canonical `IUnknown`, not of the vector slot. The
  selector returns the same string and the `ObjectAddress` keys still match, so nothing is re-created.
- No map stores a slot address or iterator into the vector, so there is no dangling access.
- .NET apps: a growing `List<T>` or a GC compaction moves managed objects, but COM identity is the
  CCW, which is native and fixed; `ComWrappers` caches one CCW per live object.

What does invalidate identity-related state:

- Insert / remove / move changes positions, so maps 1–7 are either shifted (map 1) or invalidated
  and lazily rebuilt (maps 2–4). Map 1 falls back to a full reprojection on any mismatch.
- Address reuse after an object is freed can make a new object look like an old one. Mitigations:
  `ParentStructure::Rows`, `RankEntry::Item`, and `FlatItemPropertyChangedSubscription::Item`
  hold strong references; `LiveShapingTracker` keeps
  a weak reference to detect recycled addresses; `node:object:` keys are treated as unprovable
  (`IsObjectNodeKey`). `m_liveShapeSnapshots` holds no item reference, so its entries rely on
  being erased when items leave.
- Re-created objects and boxed values get new identities, which means a full rebuild for flat and
  grouped data and loss of object-identity selection.

`ObjectAddress` keys are never dereferenced. A stale key can nevertheless match a different object
allocated at the same address and incorrectly reuse its state.

### Snapshot cleanup finding (T4)

- `ApplyIncrementalChange` calls `ApplyLiveShapingDelta` before projection splices. Remove and
  Replace notifications remove old-item subscriptions through `RemoveLiveShapingSubscription`,
  which also erases the item's snapshot.
- `RefreshLiveShapingSubscriptions` builds a fresh snapshot map and replaces the old map, so items
  absent from the materialized source lose their snapshots on refresh, including silent removals.
- `ClearLiveShapingSubscriptions` unsubscribes and clears all snapshots.

No missing snapshot erase was found in these normal paths. This is not a complete lifetime or
reentrancy audit: T13 must still verify purge-before-release ordering across remove/replace,
rebuild, and source swap/detach, as well as stale dirty rows and extra strong references.

### Intentional exceptions (T7)

- `TryReuseHierarchyStructure` keeps raw `get_abi` equality for its source-vector comparison.
  Equal interface pointers already prove a match; different pointers only reject reuse and force
  a rebuild. Canonicalizing every row would add QI to this conservative fast path.
- `ValueKey::ToString` and `ToObjectLookupKey` use the shared address helper for their reference
  fallbacks but retain exact `"o:%016llX"` / `"object:%p"` formats. These are string key encodings,
  not physical map keys. `ToString` still prefers `IStringable`; `ToObjectLookupKey` does not.
- Logical identity selectors, `RowMetadataProvider`, and `TableViewSource::MakeIdentitySelector`
  keep their existing contracts. T8 removes the redundant private `EffectiveIdentitySelector`
  accessor; internal and external consumers use `IdentitySelector` directly.
