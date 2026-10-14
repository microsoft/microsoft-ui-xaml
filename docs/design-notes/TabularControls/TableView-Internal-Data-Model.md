# TableView internal data model

This note describes how data supplied to a `TableViewSource` is stored, shaped and flattened
before the `ItemsRepeater` inside `TableView` renders it. It covers both the non-hierarchical
projections (unshaped, flat, grouped) and the hierarchical projections (hierarchical,
grouped + hierarchical) introduced by `WithChildren(...)`.

Related: [Hierarchical-Data-Rows-Design.md](Hierarchical-Data-Rows-Design.md),
[TableView-dev-spec.md](TableView-dev-spec.md).

## 1. Layers

| Layer | Type | Source | Responsibility |
|---|---|---|---|
| Public API | `TableViewSource` | `controls\dev\TableView\TableViewSource.*` | Fluent verbs (`Filter`, `SortBy`, `GroupBy`, `WithChildren`, ...). Owns the engine; picks the `ItemsSourceView` and row-metadata provider after each rebuild. |
| Shaping engine | `ShapedItemsSource` | `controls\dev\ShapedItemsSource\` | Holds the app source and the shaping recipe; runs filter / sort / bucketize; chooses a `ProjectionKind`; drives the adapters. |
| Shaping primitives | `ShapingPipeline`, `ShapingState`, `KeyedBucket`, `RowExpansionModel`, `RowIdentity` | `controls\dev\TabularShaping\` | Pure helpers: spec diffing, reshape, bucketing, expansion-intent storage, identity. |
| Group flattener | `GroupedSourceAdapter` | `controls\dev\GroupedSourceAdapter\` | Turns a list of groups into `[header, rows..., header, rows...]`. |
| Hierarchy flattener | `HierarchicalSourceAdapter` | `controls\dev\HierarchicalSourceAdapter\` | Walks the app's tree via callbacks and materializes the currently visible rows. |
| Row metadata | `RowMetadataProvider` (`ITableViewRowMetadataProvider`) | `controls\dev\TableView\RowMetadataProvider.*` | Per-index `TableViewRowInfo` (kind, level, expandable, expanded, child count), identity, expand/collapse routing. |
| Control | `TableView` + `ItemsRepeater` | `controls\dev\TableView\` | Consumes the `ItemsSourceView` and the metadata provider. |

Two principles hold in every projection:

- **Data rows are always the app's own objects.** No per-row wrapper is ever allocated. The only
  synthetic row object is `GroupedEntry`, used for group headers.
- **Row structure lives beside the rows, never on them.** Level, expandability and expansion
  live in side tables (`NodeRow`, `GroupedEntry`, `RowExpansionModel`) and are surfaced through
  `RowMetadataProvider`.

## 2. Projection kinds

`ShapedItemsSource::Refresh()` picks exactly one projection per rebuild:

| `ProjectionKind` | Condition | Builder | Presented `ItemsSourceView` | Metadata provider |
|---|---|---|---|---|
| `None` | No source | – | `nullptr` | `nullptr` |
| `Unshaped` | No shaping verb in force | `RebuildUnshapedRows` | `ItemsSourceView(m_rows)` | `nullptr` |
| `Flat` | Filter and/or sort only | `RebuildFlat` | `ItemsSourceView(m_rows)` | `CreateForFlatRows` |
| `Grouped` | `GroupBy`, no children | `RebuildGrouped` | `GroupedSourceAdapter::Entries()` | `CreateForGroupedRows` |
| `Hierarchical` | `WithChildren`, no grouping | `RebuildHierarchical` | `HierarchicalSourceAdapter::Entries()` | `CreateForHierarchicalRows` |
| `GroupedHierarchical` | `WithChildren` + `GroupBy` | `RebuildGroupedHierarchical` | `GroupedSourceAdapter::Entries()` | `CreateForGroupedHierarchicalRows` |

The mapping from kind to view/provider is done in `TableViewSource::OnProjectionRebuilt()`.

## 3. Common front of the pipeline

Every rebuild starts the same way (`ShapedItemsSource::Refresh()`):

1. **Materialize.** `Materialize(m_source)` enumerates the app collection into a local
   `std::vector<IInspectable> rows`. With a hierarchy these are the **roots** only.
2. **No-verb short circuit.** If no filter / sort / group / children is in force,
   `RebuildUnshapedRows` mirrors `rows` into `m_rows` and stops.
3. **Filter.**
   - Without a hierarchy: `ApplyFilter(rows)` runs the `ShapingPipeline` predicates.
   - With a hierarchy and a filter: `ComputeFilterRetention(rows)` (see §5.2), then `rows` is
     trimmed to roots in `m_filterKept`.
4. **Identity validation.** `ValidateRowIdentities(rows)` rejects null items and the same object
   appearing twice. Row identity is the object's canonical `IUnknown` pointer
   (`RowIdentity::MakeObjectIdentitySelector`).
5. **Dispatch** to one of the `Rebuild*` functions below.

The engine always reports its result through `RaiseProjectionRebuilt()`, which lands in
`TableViewSource::OnProjectionRebuilt()`. If the grouped-ness of the presented axis flips,
`RaiseShapeSwapped()` additionally tells the owning `TableView` to drop anything it cached.

## 4. Without hierarchy

### 4.1 Unshaped

```
app source ──Materialize──► vector rows ──ReplaceAll──► m_rows ──► ItemsSourceView ──► ItemsRepeater
```

- `m_rows` is a plain mirror. No identity tracking, no `ShapingState`, no metadata provider
  (consumers must not read sort/identity semantics off an unshaped projection).

### 4.2 Flat (filter / sort)

```
app source ─Materialize─► rows ─ApplyFilter─► rows
   ├─► m_shapingState.FilteredSource   (post-filter, source order)
   ─ApplySort─► rows
   ├─► m_shapingState.Items            (shaped output)
   ├─► m_rows.ReplaceAll(rows)         (presented rows)
   └─► m_flatRowIdentities / m_flatRowIdentityToIndex
TableViewSource: ItemsSourceView(m_rows) + RowMetadataProvider::CreateForFlatRows
```

Stored state (`ShapedItemsSource`):

| Member | Type | Purpose |
|---|---|---|
| `m_source` | `IInspectable` | The app's collection; never mutated by shaping. |
| `m_pipeline` | `ShapingHelpers::ShapingPipeline` | The recipe: filters, sorts, group order. Diffs committed specs into a `ShapingDelta`. |
| `m_shapingState` | `ShapingHelpers::ShapingState` | `FilteredSource`, `Items`, `Buckets`, `IsGrouped`, `HasProjection`. Lets a sort-only change re-sort in place (`TryApplyShapingDeltaInPlace`) with the same tie-breaking as a full rebuild. |
| `m_rows` | `IObservableVector<IInspectable>` | Presented flat rows. Also exposed as `Rows()`. |
| `m_flatRowIdentities` | `unordered_set<hstring>` | Identities currently in `m_rows`. |
| `m_flatRowIdentityToIndex` | `unordered_map<hstring, uint32_t>` | Identity → index, for O(1) locate during incremental sorted insert/remove. |

Incremental paths available only here:

- **Sort-only spec change**: `TryApplyShapingDeltaInPlace` → `Reshape(m_shapingState, ...)` →
  `m_rows.ReplaceAll`.
- **Single-item source change with an active sort**: `ApplyIncrementalChange` does binary-search
  insert / find-remove directly on `m_rows`, shifting `m_flatRowIdentityToIndex`.

Any grouped or hierarchical projection disables both and falls back to `Refresh()`.

### 4.3 Grouped

```
rows (filtered)
 ─ApplySort(sorts declared before GroupBy)─► rows            (establishes group order)
 ─BucketizeToGroups(m_groupSelector)──────► vector<KeyedBucket>{Key, Identity, Items}
 per bucket: ApplySort(sorts declared after GroupBy) on bucket.Items
 per bucket: ShapedGroup (reuse from m_groupCache by Identity)  .SetItems(bucket.Items)
 m_groupSource.ReplaceAll(groups) ─► GroupedSourceAdapter.Source(m_groupSource)
 GroupedSourceAdapter::Rebuild:
     for each group: GroupedEntry header; if expanded, the group's items (raw app objects)
     m_entries.ReplaceAll(built)
 m_rows.ReplaceAll(flatRows)   (all shaped data rows, no headers, ignores expansion)
TableViewSource: GroupedSourceAdapter::Entries() + RowMetadataProvider::CreateForGroupedRows
```

Stored state:

| Where | Member | Type | Purpose |
|---|---|---|---|
| `ShapedItemsSource` | `m_groupSelector` | `KeySelector` | App's group key selector. |
| | `m_groupIdentitySelector` | `IdentitySelector` | Optional key → stable identity. |
| | `m_groupCache` | `unordered_map<hstring, com_ptr<ShapedGroup>>` | Reuses group objects across reshapes so `Group()` does not churn. |
| | `m_groupSource` | `IObservableVector<IInspectable>` | Ordered `ShapedGroup`s; source for the grouped adapter. |
| | `m_rows` | `IObservableVector<IInspectable>` | Flat shaped data (no headers) for `Rows()`. Not the presented axis. |
| `ShapedGroup` | `m_key` | `IInspectable` | App group key (`ICollectionViewGroup::Group`). |
| | `m_groupKey` | `hstring` | Stable identity (`IGroupIdentity`). |
| | `m_items` | `IObservableVector<IInspectable>` | Group members (`GroupItems`). |
| | `m_groupChildCount` | `int32_t` | `-1` in plain grouping (`IGroupChildCount`). |
| `GroupedSourceAdapter` | `m_entries` / `m_entriesView` | `IObservableVector` / `ItemsSourceView` | The presented rows: headers + data. View identity is stable for the adapter's lifetime. |
| | `m_expansion` | `RowExpansionModel` | Group expand/collapse intent keyed by `"identity:" + GroupKey`. Default expanded. |
| | `m_liveGroupsByIdentityKey` | `unordered_map<hstring, IInspectable>` | Identity → live group, for key-based addressing. |
| | `m_innerSubscriptions` | `vector<InnerGroupSubscription>` | One per group's items vector; any change rebuilds. |
| `GroupedEntry` | `m_group`, `m_groupItemCount`, `m_isExpanded` | | The header row object. The element factory picks header vs. data template from the row value alone. |

A single group toggle is spliced in place (`TryApplyExpansionSplice`): the header is re-minted
with the new flag and that group's data rows are inserted/removed. Bulk toggles are one `Reset`.

## 5. With hierarchy

`TableViewSource::WithChildren(childrenSelector[, hasChildrenSelector])` stores two callbacks in
the engine (`m_childrenSelector`, `m_hasChildrenSelector`) and marks `m_hierarchyAxisDirty`, which
forces a full `Refresh()` (no in-place path can switch projection kinds).

**The control never stores a tree.** The tree is the app's object graph; it is re-read through
`ChildrenSelector` whenever it has to be walked. Only flat structures are stored.

### 5.1 Hierarchy contract (`HierarchyContract.h`)

| Callback | Signature | Role |
|---|---|---|
| `ChildrenFn` | `IInspectable(IInspectable item)` | Returns the item's children collection. |
| `HasChildrenFn` | `bool(IInspectable item)` | Optional. Answers expandability without enumerating, keeping collapsed subtrees unrealized. |
| `ShapeSiblingsFn` | `void(vector<IInspectable>& siblings)` | Supplied by the engine. Filters and sorts each child sibling set as the walk reaches it. |

### 5.2 Filter with ancestor retention

When a filter and a hierarchy are both in force, a node must stay visible if it matches **or any
descendant matches**. `ComputeFilterRetention(roots)` pre-walks the **whole** tree (post-order,
memoized, cycle- and depth-guarded) through `ChildrenSelector` and records:

| Member | Type | Key | Meaning |
|---|---|---|---|
| `m_filterRetentionActive` | `bool` | – | Retention is in force. |
| `m_filterKept` | `unordered_set<void*>` | item ABI pointer | Node matches, or a descendant does. |
| `m_filterContext` | `unordered_set<void*>` | item ABI pointer | Kept **only** because of a descendant. Subset of kept. |
| `m_filterKeepAlive` | `vector<IInspectable>` | – | Strong references so no pointer in the sets can be reused by another object. |
| `m_retentionSubscriptions` | `vector<RetentionSubscription>` | – | One per children collection read. Any change → `Refresh()`. |

These sets are not copied into the adapter. They are consulted through two engine-owned lambdas
(each holding a `weak_ptr` to the engine):

- `ShapeSiblings`: drops siblings not in `m_filterKept`, then `ApplySort`.
- `ContextSelector`: returns `m_filterContext.contains(item)`; the adapter records it as
  `NodeRow::IsContext`. Context rows present expanded by default from a separate overlay that
  never writes persistent expansion intent.

Without a filter, `ShapeSiblings` runs `ApplyFilter` (no-op) + `ApplySort`.

### 5.3 `HierarchicalSourceAdapter` — the hierarchy flattener

`Source(roots)` triggers `Rebuild()`, which calls the recursive `Emit` for each root:

1. Compute `selfId` (canonical `IUnknown` string) and `PathKey = parentPath + "/" + selfId`.
   Reject cycles (item is its own ancestor), duplicate siblings, and depth ≥ 256.
2. Decide expandability: `HasChildrenSelector(item)` if provided (`ChildCount = -1`); otherwise
   enumerate `ChildrenSelector(item)` and use its size. Subscribe to the children collection.
3. `IsExpanded = HasChildren && (IsContext ? contextOverlay(path) : m_expansion.IsExpanded(path))`.
4. Append the item to the local `built` vector and a `NodeRow` to the local `descriptors` vector.
5. If expanded: realize children, run `ShapeSiblings` on them, recurse with `depth + 1`.

The locals are committed with a single `m_entries.ReplaceAll(built)` only after the walk succeeds,
so a throwing walk leaves the previously published projection intact.

Stored state:

| Member | Type | Purpose |
|---|---|---|
| `m_source` | `IInspectable` | Root sibling set (engine's `m_hierarchySource`). |
| `m_childrenSelector`, `m_hasChildrenSelector`, `m_shapeSiblings`, `m_isContext` | callbacks | Contract + engine shaping hooks. |
| `m_entries` / `m_entriesView` | `IObservableVector<IInspectable>` / `ItemsSourceView` | Visible rows in depth-first order: raw app items. |
| `m_descriptors` | `vector<NodeRow>` | 1:1 by index with `m_entries`. |
| `m_indexByPathKey` | `unordered_map<hstring, int32_t>` | Path key → row index. Kept exact across splices. |
| `m_indexByItem` | `unordered_map<void*, int32_t>` (lazy) | Item ABI pointer → row index. Used by the grouped + hierarchical metadata lookup. |
| `m_expansion` | `RowExpansionModel` | Node intent keyed by path key. Baseline **collapsed**. |
| `m_contextDefaultExpanded`, `m_contextOverrides` | `bool`, `unordered_set<hstring>` | Context-row overlay. |
| `m_nodeSubscriptions` | `vector<NodeSubscription>` | One per realized children collection of a visible node. |
| `m_projectionChanged` | `function<void()>` | Raised once per coherent publish (used by grouping, §5.5). |

```cpp
struct NodeRow
{
    IInspectable Item;   // the app object
    hstring PathKey;     // "node:/id(root)/.../id(self)"
    int32_t Depth;       // 0-based; RowMetadataProvider reports Level = Depth + 1
    int32_t ChildCount;  // -1 = not enumerated (lazy HasChildrenSelector case)
    bool HasChildren;
    bool IsExpanded;
    bool IsContext;      // kept only for a matching descendant
};

struct NodeSubscription
{
    hstring PathKey;
    IInspectable ItemsForRevocation;
    event_token CollectionToken, Token, BindableToken;
};
```

Parent/child structure is implicit: a node's visible subtree is the contiguous run of rows after
it whose `Depth` is greater than its own. A single node toggle (`TryApplyExpansionSplice`) removes
that run, or emits the subtree into a temporary vector and inserts it, then fixes up
`m_indexByPathKey`. `ExpandAll` / `CollapseAll` move the baseline and do one full `Rebuild`.

### 5.4 Hierarchical (no grouping)

```
roots ─Materialize─► rows
 ─(filter) ComputeFilterRetention / trim roots to m_filterKept
 ─ApplySort─► rows                                   (root sibling set)
 ─EnsureHierarchicalAdapter(rows, shapeRoots=true)
      m_hierarchySource.ReplaceAll(rows)
      adapter: ShapeSiblings, ContextSelector, ChildrenSelector, HasChildrenSelector
      adapter.Source(m_hierarchySource) ─► Emit walk ─► m_entries + m_descriptors
 m_rows.ReplaceAll(VisibleHierarchicalRows())        (snapshot; does not track later toggles)
TableViewSource: HierarchicalSourceAdapter::Entries() + CreateForHierarchicalRows
```

### 5.5 Grouped + hierarchical

Grouping applies to the **roots**. The hierarchy is flattened **before** the groups receive
their items, so each group's items are already a flat slice. The grouped adapter still sees
only two levels: groups and rows.

```
roots (filtered)
 A. ApplySort(sorts before GroupBy) ─► BucketizeToGroups ─► vector<KeyedBucket> (roots only)
    per bucket: ApplySort(sorts after GroupBy)
    ─► orderedRoots           (bucket by bucket, contiguous)
    ─► m_rootBucketIndex      (root ABI pointer → bucket index)
 B. per bucket: ShapedGroup from m_groupCache ─► m_hierarchyGroups  (items NOT set yet)
 C. EnsureHierarchicalAdapter(orderedRoots, shapeRoots=false)
       Emit walk ─► adapter m_entries (root, subtree, root, subtree, ...) + m_descriptors
       ProjectionChanged ─► ResliceGroupsFromHierarchy
 D. ResliceGroupsFromHierarchy:
       one pass over adapter rows; each Depth==0 row looks up m_rootBucketIndex and opens that
       bucket; following rows go to the same bucket until the next Depth==0 row
       per group: GroupChildCount(rootCount); SetItems(slice)
 E. m_groupSource.ReplaceAll(groups) ─► GroupedSourceAdapter.Source(...)
       ─► [GroupedEntry(group, sliceSize, expanded), slice rows...] × N
 F. m_rows.ReplaceAll(VisibleHierarchicalRows())     (snapshot, no headers)
TableViewSource: GroupedSourceAdapter::Entries()
               + CreateForGroupedHierarchicalRows(groupedAdapter, hierarchicalAdapter)
```

Notes:

- `shapeRoots=false` because the root order is the group order; re-sorting roots among
  themselves would interleave buckets and break the re-slice.
- `ShapedGroup.m_items` holds the **same object references** as the hierarchy adapter's entries;
  depth is not stored on the group.
- Two counts on a header:
  - `ShapedGroup::GroupChildCount` is the number of roots in the group (its immediate children).
  - `GroupedEntry::GroupItemCount` is the slice size (every visible row the group spans).
- **Node toggle under grouping:** the hierarchy adapter splices its own rows → raises
  `ProjectionChanged` → re-slice → `ShapedGroup::SetItems` → the grouped adapter does a **full**
  `Rebuild` (one `Reset`). Only group-header toggles are spliced in the grouped adapter.

## 6. Row metadata

`TableViewSource` hands the control a `TableViewRowMetadataProvider`
(`shared_ptr<ITableViewRowMetadataProvider>`), created by `RowMetadataProvider::CreateFor*`.

```cpp
struct TableViewRowInfo
{
    TableViewRowKind Kind;   // Data | GroupHeader
    int32_t Level;           // 1-based
    bool IsExpandable;
    bool IsExpanded;
    int32_t ChildCount;
};
```

| `SourceKind` | `GetRowInfo(index)` derives from |
|---|---|
| `Flat` | `m_flatRows[index]` → Data, level 0. |
| `Grouped` | `GroupedEntry` at index → GroupHeader; otherwise Data at level 1. |
| `Hierarchical` | `HierarchicalSourceAdapter::TryGetNodeRow(index)` → `Level = Depth + 1`, expandability/expansion from `NodeRow`. |
| `GroupedHierarchical` | Header if `GroupedEntry` (`ChildCount` = `IGroupChildCount` root count, expandable if slice is non-empty); otherwise `TryGetNodeRowForItem(item)` on the hierarchy adapter (`Level = Depth + 1`), because grouped indices interleave headers and do not address the hierarchy adapter. |

Expansion keys route by prefix: group keys go to `GroupedSourceAdapter`, node path keys
(`"node:"`) go to `HierarchicalSourceAdapter`. `m_identityToIndex` is a lazily rebuilt
identity → index map over the presented rows.

## 7. Data structures by projection

| Structure | Unshaped | Flat | Grouped | Hierarchical | Grouped + hierarchical |
|---|:-:|:-:|:-:|:-:|:-:|
| `m_source`, `m_rows` | ✓ | ✓ | ✓ | ✓ (snapshot) | ✓ (snapshot) |
| `ShapingPipeline` | | ✓ | ✓ | ✓ | ✓ |
| `ShapingState` (retained) | | ✓ | | | |
| Flat identity set/map | | ✓ | | | |
| `KeyedBucket` (transient) | | | ✓ | | ✓ (roots) |
| `ShapedGroup`, `m_groupCache`, `m_groupSource` | | | ✓ | | ✓ |
| `GroupedSourceAdapter`, `GroupedEntry` | | | ✓ | | ✓ |
| `HierarchicalSourceAdapter`, `NodeRow`, `NodeSubscription` | | | | ✓ | ✓ |
| `m_hierarchySource` | | | | ✓ | ✓ |
| Filter retention sets, `RetentionSubscription` | | | | ✓ (if filtered) | ✓ (if filtered) |
| `m_rootBucketIndex`, `m_hierarchyGroups` | | | | | ✓ |
| `IGroupChildCount` set (≠ -1) | | | | | ✓ |

Types that exist only because of hierarchy support:

- Public: `TableViewChildrenSelector`, `TableViewHasChildrenPredicate`, `WithChildren`,
  `ClearChildren`.
- `ShapingHelpers::ChildrenFn`, `HasChildrenFn`, `ShapeSiblingsFn`.
- `HierarchicalSourceAdapter`, `NodeRow`, `NodeSubscription`.
- `ShapedItemsSource::RetentionSubscription`; `ProjectionKind::Hierarchical`,
  `ProjectionKind::GroupedHierarchical`.
- `ShapingHelpers::IGroupChildCount` (implemented by `ShapedGroup`, only meaningful under a
  hierarchy).
- `RowMetadataProvider::SourceKind::Hierarchical` / `GroupedHierarchical` and their factories;
  `ITableViewRowMetadataProvider::IsHierarchicalSource()`.
- `TableView` `HierarchyInfo` (cached `TableViewRowIndentSize`) and
  `TableViewRowAutomationPeer::IsHierarchicalRow()`.

## 8. Identity keys in use

| Purpose | Key | Where |
|---|---|---|
| Flat row identity | canonical `IUnknown` pointer as string | `m_flatRowIdentityToIndex`, `RowMetadataProvider` |
| Group identity / expansion | group identity string (`"identity:"` prefix for intent) | `m_groupCache`, `GroupedSourceAdapter::m_expansion` |
| Node expansion / lookup | path key `"node:/id(root)/.../id(self)"` (canonical `IUnknown`) | `HierarchicalSourceAdapter::m_expansion`, `m_indexByPathKey` |
| Filter retention, root → bucket, item → node row | raw ABI pointer (`winrt::get_abi(item)`) | `m_filterKept`, `m_filterContext`, `m_rootBucketIndex`, `m_indexByItem` |

The last row relies on each object always being delivered through the same `IInspectable`
pointer; path keys use the canonical `IUnknown` and do not have that dependency.
