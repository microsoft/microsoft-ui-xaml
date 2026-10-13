# TableView shaping: internal data model and pipeline

This document describes how `ShapedItemsSource` turns an app-supplied items source into the row
axis a `TableView` renders, with and without a hierarchy (`ParentBy`). It covers:

1. The layers involved.
2. The common rebuild entry point.
3. The data model and pipeline when **no hierarchy** is declared (Unshaped / Flat / Grouped).
4. The data model and pipeline when a **hierarchy** is declared (Hierarchical / GroupedHierarchical).
5. How the result reaches the renderer.
6. A reference of every data structure, and which flows use it.

Source files referenced below live under `controls\dev\`.

---

## 1. Layers

```
App items source (IVector / IObservableVector / IBindableVector / IIterable ...)
        |
        v
TableViewSource                      TableView\TableViewSource.cpp
  - fluent verbs (Filter / SortBy / GroupBy / ParentBy)
  - OnProjectionRebuilt(): picks ItemsSourceView + RowMetadataProvider by ProjectionKind
        |
        v
ShapedItemsSource  (the engine)      ShapedItemsSource\ShapedItemsSource.{h,cpp}
  - owns the authoritative source, the verb recipe and the projection
  - Refresh(): materialize -> filter -> sort/group/hierarchy -> publish
        |
        +-- ShapingPipeline           TabularShaping\ShapingPipeline.h   (recipe: filter/sort axes, group order)
        +-- ShapingHelpers            TabularShaping\ShapingHelpers.h    (bucketize, comparers, ...)
        +-- ParentKeyIndex            TabularShaping\ParentKeyIndex.h    (hierarchy only)
        +-- GroupedSourceAdapter      GroupedSourceAdapter\              (grouped row axis + headers)
        +-- HierarchicalSourceAdapter HierarchicalSourceAdapter\         (tree row axis, hierarchy only)
        |
        v
ItemsSourceView + RowMetadataProvider  -> ItemsRepeater / TableViewRow / TableViewGroupHeader
```

The engine never mutates the app's source. Every projection is derived from it.

### Verb order

Verbs are recorded in `ShapingPipeline`. Each sort axis carries an `Order` in the same verb
sequence as `GroupBy`:

- Filters always run first and are conjunctive. They have no order among themselves.
- Sorts declared **before** `GroupBy` order the **groups** (header order).
- Sorts declared **after** `GroupBy` order rows **within** each group.
- `ParentBy` is not part of the spec diff. It is tracked separately (`m_hierarchyAxisDirty`), and
  any change to it forces a full rebuild.

---

## 2. Entry point: `ShapedItemsSource::Refresh()`

Every full rebuild goes through `Refresh()` (`ShapedItemsSource.cpp`). Common prefix for all
shapes:

1. **Re-entrancy guard.** A rebuild requested while one is in flight (or while groups are being
   re-sliced) sets `m_pendingRefresh` and runs once after the outer pass unwinds.
2. **Materialize.** `Materialize(m_source)` enumerates the source into a
   `std::vector<IInspectable>` in source order.
3. **Live shaping.** `RefreshLiveShapingSubscriptions(rows)` reconciles per-item
   `PropertyChanged` subscriptions (`LiveShapingTracker`).
4. **No verb?** If no Filter / Sort / GroupBy / ParentBy is in force (`HasAnyShapingVerb()`),
   `RebuildUnshapedRows` publishes a 1:1 mirror (`ProjectionKind::Unshaped`) and stops.
5. **Filter (non-hierarchy only).** `ApplyFilter(rows)`. With a hierarchy this step is skipped,
   because the hierarchy index needs the unfiltered parent chain to keep ancestors of matches.
6. **Row identity.** `ValidateRowIdentities(rows)`. Identity is object identity
   (`RowIdentity::MakeObjectIdentitySelector`). The same object on two rows throws
   `E_INVALIDARG`. Skipped when a retained hierarchy structure is reused (already validated).
7. **Dispatch** on declared verbs:

| `m_parentKeySelector` | `m_groupSelector` | Rebuild function | Resulting `ProjectionKind` |
|---|---|---|---|
| null | null | `RebuildFlat` | `Flat` |
| null | set | `RebuildGrouped` | `Grouped` |
| set | null | `RebuildHierarchical` | `Hierarchical` |
| set | set | `RebuildGroupedHierarchical` | `GroupedHierarchical` |

8. **Publish.** Each rebuild calls `PublishProjection()`, which raises `ProjectionRebuilt`.
   `RaiseShapeSwapped()` also fires when grouped-ness changed.

Not every change goes through `Refresh()`:

- `TryApplyShapingDeltaInPlace` handles an **ungrouped, non-hierarchical, sort-only** change by
  re-sorting the retained `ShapingState::FilteredSource`.
- `ApplyIncrementalChange` / `TryApplyIncrementalSortedChange` splice single source
  inserts/removes/replaces into a **flat** projection.

Grouped and hierarchical projections always fall back to a full `Refresh()`.

---

## 3. No hierarchy

### 3.1 Data model

| Member | Type | Meaning |
|---|---|---|
| `m_source` | `IInspectable` | Authoritative app source. Never mutated. |
| `m_pipeline` | `ShapingPipeline` | Recipe: `FilterAxis` list, `SortAxis` list, group verb order. |
| `m_shapingState` | `ShapingHelpers::ShapingState` | Retained layer-1 state for the flat path (see below). |
| `m_rows` | `IObservableVector<IInspectable>` | Flat shaped rows (no headers). Presented axis for Flat/Unshaped. |
| `m_flatRowIdentities`, `m_flatRowIdentityToIndex` | set / map of `hstring` | Identity -> index for the flat incremental fast path. |
| `m_groupSelector`, `m_groupIdentitySelector` | `KeySelector`, `IdentitySelector` | GroupBy key and optional identity. |
| `m_groupCache` | `unordered_map<hstring, com_ptr<ShapedGroup>>` | Group objects reused across reshapes, keyed by group identity. |
| `m_groupSource` | `IObservableVector<IInspectable>` | Ordered list of `ShapedGroup`s fed to the grouped adapter. |
| `m_groupedAdapter` | `GroupedSourceAdapter` | Materializes `[header, rows..., header, rows...]`. |
| `m_kind`, `m_projectedAsGrouped` | `ProjectionKind`, `bool` | What was actually produced. |

`ShapingState` (`ShapingDescriptions.h`):

| Field | Meaning |
|---|---|
| `Source` | Unshaped input, source order. |
| `FilteredSource` | Post-filter rows in **source** order. A later sort-only change re-sorts from this, so ties break exactly as a full rebuild would. |
| `Items` | Shaped output. |
| `Buckets` | `KeyedBucket`s when grouped through layer 1. |
| `IsGrouped`, `HasProjection` | Validity flags. In-place paths require `HasProjection`. |

### 3.2 Flat pipeline (`RebuildFlat`)

```
source --Materialize--> rows (source order)
       --ApplyFilter--> rows (kept, source order)      -> m_shapingState.FilteredSource
       --ApplySort----> rows (stable, all sort axes)    -> m_shapingState.Items
       --ReplaceAll---> m_rows
       --RebuildFlatRowIdentityTracking
       --ReleaseHierarchyProjection / ReleaseGroupedProjection
       --PublishProjection (Kind = Flat)
```

### 3.3 Grouped pipeline (`RebuildGrouped`)

```
rows (filtered, source order)
  1. ApplySort(rows, -1, GroupOrder)        sorts declared BEFORE GroupBy -> group order
  2. BucketizeGroupsOrThrow(rows)           -> vector<KeyedBucket>, first-seen key order
                                              (throws on invalid / colliding group identity)
  3. for each bucket:
       ApplySort(bucket.Items, GroupOrder, -1)   sorts declared AFTER GroupBy -> within group
       AcquireCachedGroup(bucket)                -> ShapedGroup (reused by identity)
       group->SetItems(bucket.Items)
  4. PruneGroupCache(liveKeys)
  5. AttachGroupSource(groups)              m_groupSource.ReplaceAll + adapter->Source(...)
  6. m_rows.ReplaceAll(flatRows)            flat shaped rows, group order, no headers
  7. PublishProjection (Kind = Grouped, projectedAsGrouped = true)
```

`ShapingState` is invalidated: grouped projections are not re-sorted in place.

`GroupedSourceAdapter` then builds `m_entries`:

```
[GroupedEntry(US, count, expanded)] item item item [GroupedEntry(EU, ...)] item ...
```

A header row is a `GroupedEntry`. A data row is the app item itself. A collapsed group contributes
only its header. Group expand/collapse state lives in the adapter's `RowExpansionModel`, keyed by
group identity.

### 3.4 Example

Source: `{Name, Region}` rows. Spec: `GroupBy(Region)` then `SortBy(Name asc)`.

```
FilteredSource : [Zoe/US, Bob/EU, Amy/US, Kim/EU]   (source order)
Buckets        : US -> [Amy, Zoe]   EU -> [Bob, Kim] (first-seen order, sorted within)
m_groupSource  : [ShapedGroup(US), ShapedGroup(EU)]
Entries        : [Hdr US] Amy Zoe [Hdr EU] Bob Kim
m_rows         : Amy Zoe Bob Kim                     (no headers)
```

---

## 4. With hierarchy (`ParentBy`)

`SetParent(key, parentKey)` declares a self-referencing tree. `key` is each item's unique id;
`parentKey` points at its parent (null / empty / unknown = root). A parent **is** a data row;
there are no synthetic rows except group headers.

### 4.1 Data model

Two hierarchy-only layers sit between the materialized rows and the adapter:

```
rows (UNFILTERED, source order)
   --BuildParentStructure-->  ParentStructure   (validated tree; independent of filter/sort)
   --BuildParentKeyIndex--->  ParentKeyIndex    (one filtered + sorted reading of the tree)
   --SetIndex-------------->  HierarchicalSourceAdapter (visible rows by expansion)
```

#### `ParentStructure` (`ParentKeyIndex.h`)

Built once per relation over the **unfiltered** source and cached in `m_hierarchyStructure`.
`TryReuseHierarchyStructure` reuses it when the declaration generation, source stamp, rows and
key/parent values are unchanged, so a filter or sort change does not re-run the app's selectors.

| Field | Meaning |
|---|---|
| `Rows` | All rows, source order. Keeps keyed objects alive. |
| `KeyObjects` | Object keys held for the structure's lifetime (prevents address reuse). |
| `KeyByItem` | Item ABI pointer -> node key (`"node:" + key`). |
| `IndexByKey` | Node key -> position in `Rows`. |
| `NodeKeys` | Per row: pointer to its node key. |
| `ParentIndex` | Per row: parent position, or `Root`. |
| `ChildIndices` | Per row: children positions, source order. |
| `RootIndices` | Root positions, source order. |

`BuildParentStructure` throws `E_INVALIDARG` on a duplicate/null key, self-parent, or cycle.

#### `ParentKeyIndex` (`ParentKeyIndex.h`)

| Field | Meaning |
|---|---|
| `Roots` | Kept roots. Sorted when ungrouped. Under grouping, rewritten in bucket order. |
| `Children` | Parent node key -> kept children, sorted. No entry for a row with no kept children. |
| `ContextKeys` | Rows kept only because a descendant matched the filter. |
| `Structure` | `shared_ptr` to the `ParentStructure` it reads. |

`BuildParentKeyIndex(structure, filter, sort, sortRoots, out)`:

1. **Filter.** Each row gets state `1` (match) or `0` (out). For every match, walk up the parent
   chain and mark `0` ancestors as `2` (context). The walk stops at the first ancestor already
   kept. Descendants of a match are **not** kept.
2. **Roots.** Collect kept roots in source order. Sort them only if `sortRoots`.
3. **Children.** For each kept row, collect kept children in source order, then sort the sibling
   list (stable, so ties keep source order).

#### `HierarchicalSourceAdapter` (`HierarchicalSourceAdapter.h`)

| Member | Meaning |
|---|---|
| `m_index` | Current `ParentKeyIndex`. |
| `m_rootSegments` | Roots per group bucket. Empty = one segment. |
| `m_expansion` | `RowExpansionModel`: per-node expand intent. Baseline is **collapsed**. |
| `m_filterOverlayCollapsed` | Context rows the user collapsed under the current filter. Context rows default to **expanded**; the overlay is cleared on filter change. |
| `m_entries` / `Entries()` | Visible rows (raw app items) as an `ItemsSourceView`. |
| `m_descriptors` | `NodeRow` per visible row, index-aligned with `m_entries`. |
| `m_indexByNodeKey`, `m_indexByItem` | Lazy lookup caches. |

`NodeRow`: `Item`, `NodeKey`, `ParentKey`, `Depth` (0 = root), `ChildCount`,
`SiblingIndex` / `SiblingCount` (per bucket for roots under grouping), `HasChildren`,
`IsExpanded`, `IsContext`.

`Rebuild()` walks `Roots` segment by segment with `EmitRange` (explicit stack, depth-first,
unbounded depth). A node's children are emitted only when it is effectively expanded. A single
expand/collapse splices one subtree in or out (`TryApplyExpansionSplice`) instead of rebuilding.

#### Hierarchy-only members on `ShapedItemsSource`

| Member | Meaning |
|---|---|
| `m_keySelector`, `m_parentKeySelector` | Declared relation (both set or both null). |
| `m_hierarchyStructure` (+ `...Generation`, `...SourceStamp`) | Cached `ParentStructure` and what it was built from. |
| `m_parentDeclarationGeneration`, `m_parentRelationRedeclared`, `m_hierarchyAxisDirty` | Detect relation changes, including mid-build. |
| `m_hierarchicalAdapter` | The tree adapter. |
| `m_hierarchyGroups` | Groups in bucket order (grouped hierarchy only), for re-slicing. |
| `m_rootBucketIndex` | Root ABI pointer -> bucket index. |
| `m_hierarchyGeneration`, `m_reslicingGroups` | Staleness / re-entrancy guards for re-slicing. |
| `m_pendingHierarchyTeardown`, `m_releasedHierarchyRows` | Deferred `ClearParentBy` teardown. |

### 4.2 Hierarchical pipeline (`RebuildHierarchical`)

```
rows (UNFILTERED, source order)
  1. structure = TryReuseHierarchyStructure(...)  or  BuildParentStructure(rows, key, parentKey)
  2. index = BuildParentKeyIndex(structure, filter, sort, sortRoots = true)
       filter: matches + ancestors; sort: every sibling list including roots
  3. InvalidateShapingState, ClearFlatRowIdentityTracking, ReleaseGroupedProjection
  4. PublishHierarchyIndex(index, {})              adapter->SetIndex -> Rebuild -> Entries
  5. m_rows.ReplaceAll(VisibleHierarchicalRows())  snapshot; does not track later expand/collapse
  6. PublishProjection (Kind = Hierarchical, projectedAsGrouped = false)
```

If an app selector re-declares the relation during the build, `BuildHierarchyIndex` returns null,
queues a `Refresh`, and nothing is published.

### 4.3 Grouped hierarchical pipeline (`RebuildGroupedHierarchical`)

Grouping applies to **roots only**. Descendants are never bucketed; they follow their root.

```
rows (UNFILTERED, source order)
  1. index = BuildParentKeyIndex(structure, filter, sort, sortRoots = false)
       roots stay in source order; child sibling lists are sorted
  2. roots = index->Roots
     ApplySort(roots, -1, GroupOrder)              pre-GroupBy sorts -> group order
  3. BucketizeGroupsOrThrow(roots)                 first-seen order; group key read from roots only
  4. for each bucket:
       ApplySort(bucket.Items, GroupOrder, -1)     post-GroupBy sorts -> roots within group
       rootBucketIndex[root] = bucketIndex
     index->Roots = concatenated bucket roots
     segments     = [roots per bucket]
  5. PublishHierarchyIndex(index, segments)        adapter emits one segment per bucket;
                                                   registers ProjectionChanged -> ResliceGroupsFromHierarchy
  6. AcquireCachedGroup per bucket -> m_hierarchyGroups; PruneGroupCache
  7. ResliceGroupsFromHierarchy():
       walk adapter Entries; a depth-0 row opens its bucket (via m_rootBucketIndex),
       following deeper rows join that bucket's slice
       group->GroupChildCount(rootCount); group->SetItems(slice)
  8. AttachGroupSource(groups)                     grouped adapter adds headers
  9. m_rows.ReplaceAll(VisibleHierarchicalRows())  visible rows, group order, no headers
 10. PublishProjection (Kind = GroupedHierarchical, projectedAsGrouped = true)
```

Expanding or collapsing a node changes only the hierarchy adapter. Its `ProjectionChanged` callback
re-runs step 7, which updates each `ShapedGroup`'s items; the grouped adapter then refreshes. The
index is not rebuilt.

### 4.4 Example (depth 4)

Source (14 rows, source order). `Region` matters only for roots.

| Pos | Id | ParentId | Name | Region |
|---|---|---|---|---|
| 0 | 1 | - | Corp | US |
| 1 | 2 | 1 | Sales | |
| 2 | 3 | 1 | Eng | |
| 3 | 4 | 3 | UI | |
| 4 | 5 | 3 | Core | |
| 5 | 6 | 4 | Zoe | |
| 6 | 7 | 5 | Amy | |
| 7 | 8 | 2 | West | |
| 8 | 9 | 8 | Ann | |
| 9 | 10 | 4 | Bob | |
| 10 | 11 | - | Lab | EU |
| 11 | 12 | 11 | Kit | |
| 12 | 13 | - | Hub | US |
| 13 | 14 | 13 | Dev | |

```
Corp
├─ Sales
│  └─ West
│     └─ Ann
└─ Eng
   ├─ UI
   │  ├─ Zoe
   │  └─ Bob
   └─ Core
      └─ Amy
Lab
└─ Kit
Hub
└─ Dev
```

Node keys are written as `node:<Id>` below (the real form comes from `MakeNodeKey`).

**`ParentStructure`**

```
RootIndices  = [0, 10, 12]
ChildIndices = 0:[1,2]  1:[7]  2:[3,4]  3:[5,9]  4:[6]  7:[8]  10:[11]  12:[13]
ParentIndex  = 1->0  2->0  3->2  4->2  5->3  6->4  7->1  8->7  9->3  11->10  13->12
```

**Hierarchical, `SortBy(Name asc)`, no filter**

```
Roots    = [Corp, Hub, Lab]
Children = node:1 -> [Eng, Sales]   node:3 -> [Core, UI]   node:4 -> [Bob, Zoe]
           node:5 -> [Amy]          node:2 -> [West]       node:8 -> [Ann]
           node:11 -> [Kit]         node:13 -> [Dev]
ContextKeys = {}
Entries (all expanded) = Corp Eng Core Amy UI Bob Zoe Sales West Ann Hub Dev Lab Kit
```

**Hierarchical, filter `Name starts with "A"`, `SortBy(Name asc)`**

```
matches : Amy, Ann
context : Core, Eng, Corp (from Amy); West, Sales (from Ann; stops at Corp)
Roots    = [Corp]
Children = node:1 -> [Eng, Sales]   node:3 -> [Core]   node:5 -> [Amy]
           node:2 -> [West]         node:8 -> [Ann]
ContextKeys = {node:1, node:3, node:5, node:2, node:8}
```

A filter matching only `UI` keeps `UI`, `Eng` and `Corp`. `UI`'s children are dropped, and `UI`
shows as a leaf.

**Grouped hierarchical, `GroupBy(Region)` then `SortBy(Name asc)`**

```
index (sortRoots=false).Roots = [Corp, Lab, Hub]
buckets                       = US -> [Corp, Hub]   EU -> [Lab]
index->Roots                  = [Corp, Hub, Lab]
segments                      = [2, 1]
m_rootBucketIndex             = {Corp->0, Hub->0, Lab->1}
slices (all expanded)         = US: Corp Eng Core Amy UI Bob Zoe Sales West Ann Hub Dev  (GroupChildCount 2)
                                EU: Lab Kit                                              (GroupChildCount 1)
Grouped Entries               = [Hdr US] Corp ... Dev [Hdr EU] Lab Kit
```

If the sort is declared **before** `GroupBy` (descending), roots are sorted first to
`[Lab, Hub, Corp]`, so EU is seen first and becomes the first group.

---

## 5. Handing off to render

`TableViewSource::OnProjectionRebuilt()` reads `m_engine->Kind()` and selects the row axis and the
row metadata provider:

| `ProjectionKind` | `ItemsSourceView` given to the repeater | `RowMetadataProvider` |
|---|---|---|
| `None` | null | null |
| `Unshaped` | `ItemsSourceView(m_rows)` | null (no shaped identity) |
| `Flat` | `ItemsSourceView(m_rows)` | `CreateForFlatRows` |
| `Grouped` | `GroupedSourceAdapter::Entries()` | `CreateForGroupedRows` |
| `Hierarchical` | `HierarchicalSourceAdapter::Entries()` | `CreateForHierarchicalRows` |
| `GroupedHierarchical` | `GroupedSourceAdapter::Entries()` | `CreateForGroupedHierarchicalRows`: row kind and identity from the grouped axis; level and expansion from the hierarchy adapter via `TryGetNodeRowForItem` |

`RowMetadataProvider::GetRowInfo(index)` returns a `TableViewRowInfo` for each presented index:
row kind (group header vs data), identity, level (`NodeRow::Depth + 1` for hierarchy),
expandability and expansion. Group header counts use `IGroupChildCount` when set (root count under
a hierarchy), otherwise the group's row count.

The element factory distinguishes a header from a data row by value alone:
`TryGetGroupedEntry(value)` succeeds only for headers. Expand/collapse requests from the UI go
back through `RowMetadataProvider::Expand/Collapse/Toggle`, keyed `"group:..."` (grouped adapter)
or `"node:..."` (hierarchy adapter).

---

## 6. Data structure reference

| Structure | Defined in | No hierarchy | Hierarchy |
|---|---|---|---|
| `ShapingPipeline`, `SortAxis`, `FilterAxis` | `TabularShaping\ShapingPipeline.h` | yes | yes |
| `ShapingSpec`, `ShapingDelta`, `Sort/Group/FilterDescription` | `TabularShaping\ShapingDescriptions.h` | yes | spec diff only; hierarchy axis tracked separately |
| `ShapingState` | `TabularShaping\ShapingDescriptions.h` | Flat path only | invalidated |
| `KeyedBucket` | `TabularShaping\ShapingHelpers.h` | Grouped | GroupedHierarchical (roots) |
| `ShapedGroup` (`ICollectionViewGroup`, `IGroupIdentity`, `IGroupChildCount`) | `ShapedItemsSource\ShapedGroup.h` | Grouped (`GroupChildCount = -1`) | GroupedHierarchical (`GroupChildCount` = root count) |
| `GroupedSourceAdapter`, `GroupedEntry` | `GroupedSourceAdapter\` | Grouped | GroupedHierarchical |
| `RowExpansionModel` | `TabularShaping\RowExpansionModel.h` | group expansion (grouped adapter) | group expansion + node expansion (hierarchy adapter) |
| `LiveShapingTracker`, `LiveShapeSnapshot` | `ShapedItemsSource\` | yes | yes (snapshot also carries `NodeKey` / `ParentKey`) |
| `ParentStructure` | `TabularShaping\ParentKeyIndex.h` | - | yes |
| `ParentKeyIndex` | `TabularShaping\ParentKeyIndex.h` | - | yes |
| `ParentKeyFilter`, `SiblingSorter` | `TabularShaping\ParentKeyIndex.h` | - | yes |
| `HierarchicalSourceAdapter`, `NodeRow` | `HierarchicalSourceAdapter\` | - | yes |
| `ProjectionKind::Hierarchical`, `::GroupedHierarchical` | `ShapedItemsSource\ShapedItemsSource.h` | - | yes |
| `RowMetadataProvider` | `TableView\RowMetadataProvider.h` | Flat / Grouped | Hierarchical / GroupedHierarchical |
